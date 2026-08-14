//  Copyright (c) 2007-2023 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/actions/post_helper_fwd.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/threading_base.hpp>

#include <chrono>
#include <exception>
#include <thread>
#include <utility>

namespace hpx::detail {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    void call_async(threads::thread_init_data&& data,
        hpx::id_type const& target, naming::address::address_type lva,
        naming::address::component_type comptype, Ts&&... vs)
    {
        using continuation_type = traits::action_continuation_t<Action>;

        continuation_type cont;
        if (traits::action_decorate_continuation<Action>::call(cont))    //-V614
        {
            data.func = Action::construct_thread_function(
                target, HPX_MOVE(cont), lva, comptype, HPX_FORWARD(Ts, vs)...);
        }
        else
        {
            data.func = Action::construct_thread_function(
                target, lva, comptype, HPX_FORWARD(Ts, vs)...);
        }

        while (!threads::threadmanager_is_at_least(hpx::state::running))
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(HPX_NETWORK_RETRIES_SLEEP));
        }

        traits::action_schedule_thread<Action>::call(lva, comptype, data);
    }

    HPX_CXX_EXPORT template <typename Action, typename Continuation,
        typename... Ts>
    void call_async(threads::thread_init_data&& data, Continuation&& cont,
        hpx::id_type const& target, naming::address::address_type lva,
        naming::address::component_type comptype, Ts&&... vs)
    {
        // first decorate the continuation
        traits::action_decorate_continuation<Action>::call(cont);

        // now, schedule the thread
        data.func = Action::construct_thread_function(target,
            HPX_FORWARD(Continuation, cont), lva, comptype,
            HPX_FORWARD(Ts, vs)...);

        while (!threads::threadmanager_is_at_least(hpx::state::running))
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(HPX_NETWORK_RETRIES_SLEEP));
        }

        traits::action_schedule_thread<Action>::call(lva, comptype, data);
    }

    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    HPX_FORCEINLINE void call_sync(naming::address::address_type lva,
        naming::address::component_type comptype, Ts&&... vs)
    {
        Action::execute_function(lva, comptype, HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Action, typename Continuation,
        typename... Ts>
    HPX_FORCEINLINE void call_sync(Continuation&& cont,
        naming::address::address_type lva,
        naming::address::component_type comptype, Ts&&... vs)
    {
        try
        {
            cont.trigger_value(Action::execute_function(
                lva, comptype, HPX_FORWARD(Ts, vs)...));
        }
        catch (...)
        {
            // make sure hpx::exceptions are propagated back to the
            // client
            cont.trigger_error(std::current_exception());
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action>
    struct post_helper<Action, /*DirectExecute=*/false>
    {
        template <typename... Ts>
        static void call(threads::thread_init_data&& data,
            hpx::id_type const& target, naming::address::address_type lva,
            naming::address::component_type comptype, Ts&&... vs)
        {
            // route launch policy through component
            launch const policy =
                traits::action_select_direct_execution<Action>::call(
                    launch::async, lva);

            if (policy == launch::async)
            {
                call_async<Action>(HPX_MOVE(data), target, lva, comptype,
                    HPX_FORWARD(Ts, vs)...);
            }
            else
            {
                call_sync<Action>(lva, comptype, HPX_FORWARD(Ts, vs)...);
            }
        }

        template <typename Continuation, typename... Ts>
        static void call(threads::thread_init_data&& data, Continuation&& cont,
            hpx::id_type const& target, naming::address::address_type lva,
            naming::address::component_type comptype, Ts&&... vs)
        {
            // route launch policy through component
            launch const policy =
                traits::action_select_direct_execution<Action>::call(
                    launch::async, lva);

            if (policy == launch::async)
            {
                call_async<Action>(HPX_MOVE(data),
                    HPX_FORWARD(Continuation, cont), target, lva, comptype,
                    HPX_FORWARD(Ts, vs)...);
            }
            else
            {
                call_sync<Action>(HPX_FORWARD(Continuation, cont), lva,
                    comptype, HPX_FORWARD(Ts, vs)...);
            }
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action>
    struct post_helper<Action, /*DirectExecute=*/true>
    {
        // If local and to be directly executed, just call the function
        template <typename... Ts>
        HPX_FORCEINLINE static void call(threads::thread_init_data&& data,
            hpx::id_type const& target, naming::address::address_type lva,
            naming::address::component_type comptype, Ts&&... vs)
        {
            // Direct actions should be able to be executed from a non-HPX
            // thread as well
            if (this_thread::has_sufficient_stack_space() ||
                !threads::threadmanager_is_at_least(hpx::state::running))
            {
                call_sync<Action>(lva, comptype, HPX_FORWARD(Ts, vs)...);
            }
            else
            {
                call_async<Action>(HPX_MOVE(data), target, lva, comptype,
                    HPX_FORWARD(Ts, vs)...);
            }
        }

        template <typename Continuation, typename... Ts>
        HPX_FORCEINLINE static void call(threads::thread_init_data&& data,
            Continuation&& cont, hpx::id_type const& target,
            naming::address::address_type lva,
            naming::address::component_type comptype, Ts&&... vs)
        {
            // Direct actions should be able to be executed from a non-HPX
            // thread as well
            if (this_thread::has_sufficient_stack_space() ||
                !threads::threadmanager_is_at_least(hpx::state::running))
            {
                call_sync<Action>(HPX_FORWARD(Continuation, cont), lva,
                    comptype, HPX_FORWARD(Ts, vs)...);
            }
            else
            {
                call_async<Action>(HPX_MOVE(data),
                    HPX_FORWARD(Continuation, cont), target, lva, comptype,
                    HPX_FORWARD(Ts, vs)...);
            }
        }
    };
}    // namespace hpx::detail
