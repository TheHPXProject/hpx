//  Copyright (c) 2007-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/assert.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace hpx::actions::detail {

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action, int N>
    struct action_decorate_function_semaphore
    {
        using semaphore_type = hpx::counting_semaphore_var<hpx::spinlock, N>;

        struct tag
        {
        };

        static semaphore_type& get_sem()
        {
            util::static_<semaphore_type, tag> sem;
            return sem.get();
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action, int N>
    struct action_decorate_function
    {
        static constexpr bool value = true;

        // This wrapper is needed to stop infinite recursion when
        // trying to get the possible additional function decoration
        // from the component
        struct action_wrapper
        {
            using component_type = typename Action::component_type;
        };

        static_assert(!Action::direct_execution::value,
            "explicit decoration of direct actions is not supported");

        using construct_semaphore_type =
            action_decorate_function_semaphore<Action, N>;

        // If the action returns something which is not a future, we inject
        // a semaphore into the call graph.
        static threads::thread_result_type thread_function(
            threads::thread_restart_state state,
            threads::thread_function_type f)
        {
            construct_semaphore_type::get_sem().wait();
            auto on_exit = hpx::experimental::scope_exit(
                [] { construct_semaphore_type::get_sem().signal(); });

            return f(state);
        }

        // If the action returns a future we wait on the semaphore as well,
        // however it will be signaled once the future gets ready only.
        static threads::thread_result_type thread_function_future(
            threads::thread_restart_state state,
            threads::thread_function_type f)
        {
            construct_semaphore_type::get_sem().wait();
            return f(state);
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename F>
        static threads::thread_function_type call(
            naming::address::address_type lva, F&& f)
        {
            typedef typename Action::result_type result_type;
            if constexpr (traits::is_future_v<result_type>)
            {
                return util::one_shot(hpx::bind_back(
                    &action_decorate_function::thread_function_future,
                    traits::action_decorate_function<action_wrapper>::call(
                        lva, HPX_FORWARD(F, f))));
            }
            else
            {
                return util::one_shot(
                    hpx::bind_back(&action_decorate_function::thread_function,
                        traits::action_decorate_function<action_wrapper>::call(
                            lva, HPX_FORWARD(F, f))));
            }
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action, int N>
    struct wrapped_continuation
    {
        typedef action_decorate_function_semaphore<Action, N>
            construct_semaphore_type;

        naming::address addr_;

        template <typename T>
        void operator()(hpx::id_type const& id, T&& t)
        {
            if (id)
            {
                hpx::set_lco_value(id, std::move(addr_), HPX_FORWARD(T, t));
            }
            construct_semaphore_type::get_sem().signal();
        }

        void operator()(hpx::id_type const& id)
        {
            if (id)
            {
                trigger_lco_event(id, std::move(addr_));
            }
            construct_semaphore_type::get_sem().signal();
        }

        template <typename Archive>
        void serialize(Archive& ar, unsigned int)
        {
            // clang-format off
            ar & addr_;
            // clang-format on
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action, int N>
    struct action_decorate_continuation
    {
        static_assert(!Action::direct_execution::value,
            "explicit decoration of direct actions is not supported");

        typedef action_decorate_function_semaphore<Action, N>
            construct_semaphore_type;

        typedef typename traits::action_continuation<Action>::type
            continuation_type;

        ///////////////////////////////////////////////////////////////////////
        static bool call(continuation_type& cont)
        {
            typedef typename Action::result_type result_type;
            if constexpr (traits::is_future_v<result_type>)
            {
                // If the action returns a future we wrap the given continuation
                // to be able to signal the semaphore after the wrapped action
                // has returned.
                cont = continuation_type(cont.get_id(),
                    wrapped_continuation<Action, N>{cont.get_addr()});
                return true;
            }
            else
            {
                // If the action returns something which is not a future, we do
                // nothing special.
                return false;
            }
        }
    };
}    // namespace hpx::actions::detail

///////////////////////////////////////////////////////////////////////////////
#define HPX_ACTION_INVOKE_NO_MORE_THAN(action, maxnum)                         \
    namespace hpx::traits {                                                    \
        template <>                                                            \
        struct action_decorate_function<action>                                \
          : hpx::actions::detail::action_decorate_function<action, maxnum>     \
        {                                                                      \
        };                                                                     \
                                                                               \
        template <>                                                            \
        struct action_decorate_continuation<action>                            \
          : hpx::actions::detail::action_decorate_continuation<action, maxnum> \
        {                                                                      \
        };                                                                     \
    }                                                                          \
    /**/

#endif
