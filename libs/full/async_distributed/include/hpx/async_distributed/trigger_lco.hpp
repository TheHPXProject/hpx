//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/async_distributed/trigger_lco.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/async_distributed/continuation_fwd.hpp>
#include <hpx/async_distributed/detail/post_continue_fwd.hpp>
#include <hpx/async_distributed/detail/post_implementations_fwd.hpp>
#include <hpx/async_distributed/lcos_fwd.hpp>
#include <hpx/async_distributed/trigger_lco_fwd.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/naming_base.hpp>

#include <type_traits>
#include <utility>

namespace hpx {

    /// \cond NOINTERNAL

    //////////////////////////////////////////////////////////////////////////
    // forward declare the required overload of post.
    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    bool post(hpx::id_type const& gid, Ts&&... vs);
    /// \endcond

    /// \cond NOINTERNAL
    namespace detail {

        template <typename T>
        struct make_rvalue_impl
        {
            using type = T&&;

            template <typename U>
            HPX_FORCEINLINE static T&& call(U& u) noexcept
            {
                return HPX_MOVE(u);
            }
        };

        template <typename T>
        struct make_rvalue_impl<T const>
        {
            using type = T;

            template <typename U>
            HPX_FORCEINLINE static T call(U const& u)
            {
                return u;
            }
        };

        template <typename T>
        struct make_rvalue_impl<T&>
        {
            using type = T;

            HPX_FORCEINLINE static T call(T& u)
            {
                return u;
            }
        };

        template <typename T>
        struct make_rvalue_impl<T const&>
        {
            using type = T;

            HPX_FORCEINLINE static T call(T const& u)
            {
                return u;
            }
        };

        template <typename T>
        HPX_FORCEINLINE detail::make_rvalue_impl<T>::type make_rvalue(
            std::remove_reference_t<T>& v)
        {
            return detail::make_rvalue_impl<T>::call(v);
        }

        template <typename T>
        HPX_FORCEINLINE detail::make_rvalue_impl<T>::type make_rvalue(
            std::remove_reference_t<T>&& v)
        {
            return detail::make_rvalue_impl<T>::call(v);
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename Action, typename Result>
        void set_lco_value(hpx::id_type id, naming::address&& addr, Result&& t,
            bool move_credits)
        {
            constexpr launch::async_policy policy(
                actions::action_priority<Action>(),
                actions::action_stacksize<Action>());
            if (move_credits &&
                id.get_management_type() !=
                    hpx::id_type::management_type::unmanaged)
            {
                hpx::id_type target(id.get_gid(),
                    hpx::id_type::management_type::managed_move_credit);
                id.make_unmanaged();

                detail::post_impl<Action>(target, HPX_MOVE(addr), policy,
                    detail::make_rvalue<Result>(t));
            }
            else
            {
                detail::post_impl<Action>(
                    id, HPX_MOVE(addr), policy, detail::make_rvalue<Result>(t));
            }
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename LocalResult, typename RemoteResult, typename Action,
            typename Result>
        void set_lco_value(hpx::id_type id, naming::address&& addr, Result&& t,
            hpx::id_type const& cont, bool move_credits)
        {
            if (move_credits &&
                id.get_management_type() !=
                    hpx::id_type::management_type::unmanaged)
            {
                hpx::id_type target(id.get_gid(),
                    hpx::id_type::management_type::managed_move_credit);
                id.make_unmanaged();

                detail::post_impl<Action>(
                    actions::typed_continuation<LocalResult, RemoteResult>(
                        cont),
                    target, HPX_MOVE(addr), detail::make_rvalue<Result>(t));
            }
            else
            {
                detail::post_impl<Action>(
                    actions::typed_continuation<LocalResult, RemoteResult>(
                        cont),
                    id, HPX_MOVE(addr), detail::make_rvalue<Result>(t));
            }
        }
    }    // namespace detail
    /// \endcond

    /// \cond NOINTERNAL
    HPX_CXX_EXPORT template <typename Result>
    void set_lco_value([[maybe_unused]] hpx::id_type id,
        [[maybe_unused]] naming::address&& addr, [[maybe_unused]] Result&& t,
        [[maybe_unused]] bool move_credits)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        using remote_result_type = std::decay_t<Result>;
        using local_result_type =
            traits::promise_local_result<remote_result_type>::type;

        if (components::get_base_type(addr.type_) ==
            to_int(
                components::component_enum_type::base_lco_with_value_unmanaged))
        {
            using set_value_action =
                lcos::base_lco_with_value<local_result_type, remote_result_type,
                    traits::detail::component_tag>::set_value_action;

            detail::set_lco_value<set_value_action>(
                id, HPX_MOVE(addr), HPX_FORWARD(Result, t), move_credits);
        }
        else
        {
            // NOLINTNEXTLINE(bugprone-use-after-move)
            HPX_ASSERT(!addr ||
                components::get_base_type(addr.type_) ==
                    to_int(hpx::components::component_enum_type::
                            base_lco_with_value));

            using set_value_action =
                lcos::base_lco_with_value<local_result_type, remote_result_type,
                    traits::detail::managed_component_tag>::set_value_action;

            detail::set_lco_value<set_value_action>(
                id, HPX_MOVE(addr), HPX_FORWARD(Result, t), move_credits);
        }
#else
        HPX_ASSERT(false);
#endif
    }

    HPX_CXX_EXPORT template <typename Result>
    void set_lco_value([[maybe_unused]] hpx::id_type id,
        [[maybe_unused]] naming::address&& addr, [[maybe_unused]] Result&& t,
        [[maybe_unused]] hpx::id_type const& cont,
        [[maybe_unused]] bool move_credits)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        using remote_result_type = std::decay_t<Result>;
        using local_result_type =
            traits::promise_local_result<remote_result_type>::type;

        if (components::get_base_type(addr.type_) ==
            to_int(
                components::component_enum_type::base_lco_with_value_unmanaged))
        {
            using set_value_action =
                lcos::base_lco_with_value<local_result_type, remote_result_type,
                    traits::detail::component_tag>::set_value_action;

            detail::set_lco_value<local_result_type, remote_result_type,
                set_value_action>(
                id, HPX_MOVE(addr), HPX_FORWARD(Result, t), cont, move_credits);
        }
        else
        {
            HPX_ASSERT(!addr ||
                components::get_base_type(addr.type_) ==
                    to_int(
                        components::component_enum_type::base_lco_with_value));

            using set_value_action =
                lcos::base_lco_with_value<local_result_type, remote_result_type,
                    traits::detail::managed_component_tag>::set_value_action;

            detail::set_lco_value<local_result_type, remote_result_type,
                set_value_action>(
                id, HPX_MOVE(addr), HPX_FORWARD(Result, t), cont, move_credits);
        }
#else
        HPX_ASSERT(false);
#endif
    }
    /// \endcond
}    // namespace hpx

#include <hpx/async_distributed/detail/post.hpp>
