//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/naming_base.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    HPX_CXX_EXPORT template <typename Action, typename Cont, typename... Ts>
    bool post_continue(Cont&& cont, hpx::id_type const& gid, Ts&&... vs);

    HPX_CXX_EXPORT template <typename Component, typename Signature,
        typename Derived, typename Cont, typename... Ts>
    bool post_continue(
        hpx::actions::basic_action<Component, Signature, Derived> /*act*/,
        Cont&& cont, hpx::id_type const& gid, Ts&&... vs);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    bool post_continue(
        hpx::id_type const& cont, hpx::id_type const& gid, Ts&&... vs);

    HPX_CXX_EXPORT template <typename Component, typename Signature,
        typename Derived, typename... Ts>
    bool post_continue(
        hpx::actions::basic_action<Component, Signature, Derived> /*act*/,
        hpx::id_type const& cont, hpx::id_type const& gid, Ts&&... vs);
}    // namespace hpx
