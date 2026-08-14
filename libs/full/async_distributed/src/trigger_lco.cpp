//  Copyright (c) 2007-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/naming_base.hpp>

#include <hpx/async_distributed/base_lco.hpp>
#include <hpx/async_distributed/post.hpp>
#include <hpx/async_distributed/trigger_lco.hpp>
#if defined(HPX_MSVC) && !defined(HPX_DEBUG)
#include <hpx/async_distributed/base_lco_with_value.hpp>
#endif

#include <exception>
#include <utility>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx {

    void trigger_lco_event([[maybe_unused]] hpx::id_type id,
        [[maybe_unused]] naming::address&& addr,
        [[maybe_unused]] bool move_credits)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        using set_action = lcos::base_lco::set_event_action;

        constexpr launch::async_policy policy(
            actions::action_priority<set_action>(),
            actions::action_stacksize<set_action>());
        if (move_credits &&
            id.get_management_type() !=
                hpx::id_type::management_type::unmanaged)
        {
            hpx::id_type const target(
                id.get_gid(), id_type::management_type::managed_move_credit);
            id.make_unmanaged();

            detail::post_impl<set_action>(target, HPX_MOVE(addr), policy);
        }
        else
        {
            detail::post_impl<set_action>(id, HPX_MOVE(addr), policy);
        }
#else
        HPX_ASSERT(false);
#endif
    }

    void trigger_lco_event([[maybe_unused]] hpx::id_type id,
        [[maybe_unused]] naming::address&& addr,
        [[maybe_unused]] hpx::id_type const& cont,
        [[maybe_unused]] bool move_credits)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        using set_action = lcos::base_lco::set_event_action;
        using local_result_type =
            hpx::traits::extract_action<set_action>::local_result_type;
        using remote_result_type =
            hpx::traits::extract_action<set_action>::remote_result_type;

        constexpr launch::async_policy policy(
            actions::action_priority<set_action>(),
            actions::action_stacksize<set_action>());
        if (move_credits &&
            id.get_management_type() !=
                hpx::id_type::management_type::unmanaged)
        {
            hpx::id_type const target(
                id.get_gid(), id_type::management_type::managed_move_credit);
            id.make_unmanaged();

            detail::post_impl<set_action>(
                actions::typed_continuation<local_result_type,
                    remote_result_type>(cont),
                target, HPX_MOVE(addr), policy);
        }
        else
        {
            detail::post_impl<set_action>(
                actions::typed_continuation<local_result_type,
                    remote_result_type>(cont),
                id, HPX_MOVE(addr), policy);
        }
#else
        HPX_ASSERT(false);
#endif
    }

    void set_lco_error([[maybe_unused]] hpx::id_type id,
        [[maybe_unused]] naming::address&& addr,
        [[maybe_unused]] std::exception_ptr const& e,
        [[maybe_unused]] bool move_credits)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        using set_action = lcos::base_lco::set_exception_action;

        constexpr launch::async_policy policy(
            actions::action_priority<set_action>(),
            actions::action_stacksize<set_action>());
        if (move_credits &&
            id.get_management_type() !=
                hpx::id_type::management_type::unmanaged)
        {
            hpx::id_type const target(
                id.get_gid(), id_type::management_type::managed_move_credit);
            id.make_unmanaged();

            detail::post_impl<set_action>(target, HPX_MOVE(addr), policy, e);
        }
        else
        {
            detail::post_impl<set_action>(id, HPX_MOVE(addr), policy, e);
        }
#else
        HPX_ASSERT(false);
#endif
    }

    void set_lco_error([[maybe_unused]] hpx::id_type id,
        [[maybe_unused]] naming::address&& addr,
        [[maybe_unused]] std::exception_ptr&& e,
        [[maybe_unused]] bool move_credits)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        using set_action = lcos::base_lco::set_exception_action;

        constexpr launch::async_policy policy(
            actions::action_priority<set_action>(),
            actions::action_stacksize<set_action>());
        if (move_credits &&
            id.get_management_type() !=
                hpx::id_type::management_type::unmanaged)
        {
            hpx::id_type const target(
                id.get_gid(), id_type::management_type::managed_move_credit);
            id.make_unmanaged();

            detail::post_impl<set_action>(
                target, HPX_MOVE(addr), policy, HPX_MOVE(e));
        }
        else
        {
            detail::post_impl<set_action>(
                id, HPX_MOVE(addr), policy, HPX_MOVE(e));
        }
#else
        HPX_ASSERT(false);
#endif
    }

    void set_lco_error([[maybe_unused]] hpx::id_type id,
        [[maybe_unused]] naming::address&& addr,
        [[maybe_unused]] std::exception_ptr const& e,
        [[maybe_unused]] hpx::id_type const& cont,
        [[maybe_unused]] bool move_credits)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        using set_action = lcos::base_lco::set_exception_action;
        using local_result_type =
            hpx::traits::extract_action<set_action>::local_result_type;
        using remote_result_type =
            hpx::traits::extract_action<set_action>::remote_result_type;

        constexpr launch::async_policy policy(
            actions::action_priority<set_action>(),
            actions::action_stacksize<set_action>());
        if (move_credits &&
            id.get_management_type() !=
                hpx::id_type::management_type::unmanaged)
        {
            hpx::id_type const target(
                id.get_gid(), id_type::management_type::managed_move_credit);
            id.make_unmanaged();

            detail::post_impl<set_action>(
                actions::typed_continuation<local_result_type,
                    remote_result_type>(cont),
                target, HPX_MOVE(addr), policy, e);
        }
        else
        {
            detail::post_impl<set_action>(
                actions::typed_continuation<local_result_type,
                    remote_result_type>(cont),
                id, HPX_MOVE(addr), policy, e);
        }
#else
        HPX_ASSERT(false);
#endif
    }

    void set_lco_error([[maybe_unused]] hpx::id_type id,
        [[maybe_unused]] naming::address&& addr,
        [[maybe_unused]] std::exception_ptr&& e,
        [[maybe_unused]] hpx::id_type const& cont,
        [[maybe_unused]] bool move_credits)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        using set_action = lcos::base_lco::set_exception_action;
        using local_result_type =
            hpx::traits::extract_action<set_action>::local_result_type;
        using remote_result_type =
            hpx::traits::extract_action<set_action>::remote_result_type;

        constexpr launch::async_policy policy(
            actions::action_priority<set_action>(),
            actions::action_stacksize<set_action>());
        if (move_credits &&
            id.get_management_type() !=
                hpx::id_type::management_type::unmanaged)
        {
            hpx::id_type const target(
                id.get_gid(), id_type::management_type::managed_move_credit);
            id.make_unmanaged();

            detail::post_impl<set_action>(
                actions::typed_continuation<local_result_type,
                    remote_result_type>(cont),
                target, HPX_MOVE(addr), policy, HPX_MOVE(e));
        }
        else
        {
            detail::post_impl<set_action>(
                actions::typed_continuation<local_result_type,
                    remote_result_type>(cont),
                id, HPX_MOVE(addr), policy, HPX_MOVE(e));
        }
#else
        HPX_ASSERT(false);
#endif
    }

#if defined(HPX_MSVC) && !defined(HPX_DEBUG)
    ///////////////////////////////////////////////////////////////////////////
    // Explicitly instantiate specific post needed for set_lco_value for MSVC
    // (in release mode only, leads to missing symbols otherwise).
    template bool post<lcos::base_lco_with_value<util::unused_type,
                           util::unused_type>::set_value_action,
        util::unused_type>(hpx::id_type const&, util::unused_type&&);
#endif
}    // namespace hpx
