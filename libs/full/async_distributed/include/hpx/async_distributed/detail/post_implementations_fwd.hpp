//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/parcelset_base.hpp>

#include <system_error>
#include <type_traits>

namespace hpx::detail {

    ///////////////////////////////////////////////////////////////////////////
    // forward declaration only
    HPX_CXX_EXPORT template <typename Action, typename Continuation,
        typename... Ts>
    bool post_l_p(Continuation&& c, hpx::id_type const& target,
        naming::address&& addr, hpx::launch policy, Ts&&... vs);

    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    bool post_l_p(hpx::id_type const& target, naming::address&& addr,
        hpx::launch policy, Ts&&... vs);

    HPX_CXX_EXPORT template <typename Action, typename Continuation,
        typename... Ts>
    bool post_r_p(naming::address&& addr, Continuation&& c,
        hpx::id_type const& id, hpx::launch policy, Ts&&... vs);

    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    bool post_r_p(naming::address&& addr, hpx::id_type const& id,
        hpx::launch policy, Ts&&... vs);

    HPX_CXX_EXPORT template <typename Action, typename Continuation,
        typename Callback, typename... Ts>
    bool post_r_p_cb(naming::address&& addr, Continuation&& c,
        hpx::id_type const& id, hpx::launch policy, Callback&& cb, Ts&&... vs);

    HPX_CXX_EXPORT template <typename Action, typename Callback, typename... Ts>
    bool post_r_p_cb(naming::address&& addr, hpx::id_type const& id,
        hpx::launch policy, Callback&& cb, Ts&&... vs);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename Continuation,
        typename... Ts>
        requires(traits::is_continuation_v<Continuation>)
    bool post_impl(Continuation&& c, hpx::id_type const& id, hpx::launch policy,
        Ts&&... vs);

    HPX_CXX_EXPORT template <typename Action, typename Continuation,
        typename... Ts>
        requires(traits::is_continuation_v<Continuation>)
    bool post_impl(Continuation&& c, hpx::id_type const& id,
        naming::address&& addr, hpx::launch policy, Ts&&... vs);

    HPX_CXX_EXPORT template <typename Action, typename Continuation,
        typename Callback, typename... Ts>
        requires(traits::is_continuation_v<Continuation>)
    bool post_cb_impl(Continuation&& c, hpx::id_type const& id,
        hpx::launch policy, Callback&& cb, Ts&&... vs);

    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    bool post_impl(hpx::id_type const& id, hpx::launch policy, Ts&&... vs);

    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    bool post_impl(hpx::id_type const& id, naming::address&&,
        hpx::launch policy, Ts&&... vs);

    HPX_CXX_EXPORT template <typename Action, typename Callback, typename... Ts>
    bool post_cb_impl(
        hpx::id_type const& id, hpx::launch policy, Callback&& cb, Ts&&... vs);

    ///////////////////////////////////////////////////////////////////////////
    template <typename Component>
    constexpr bool pin_count_is_valid(naming::address_type lva) noexcept
    {
        auto const pin_count =
            traits::component_pin_support<Component>::pin_count(
                get_lva<Component>::call(lva));
        return pin_count != ~0x0u && pin_count != 0;
    }

    template <typename Callback>
    void invoke_callback(Callback&& cb)
    {
        // invoke callback
#if defined(HPX_HAVE_NETWORKING)
        cb(std::error_code(), parcelset::empty_parcel);
#else
        cb();
#endif
    }
}    // namespace hpx::detail
