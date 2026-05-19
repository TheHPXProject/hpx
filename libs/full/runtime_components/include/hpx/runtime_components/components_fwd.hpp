//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

namespace hpx {

    /// \namespace components
    namespace components {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_EXPORT template <typename Component>
        struct component_factory;

        HPX_CXX_EXPORT class runtime_support;

        namespace stubs {

            HPX_CXX_EXPORT struct runtime_support;
        }    // namespace stubs

        namespace server {

            HPX_CXX_EXPORT class HPX_EXPORT runtime_support;
        }    // namespace server

    }    // namespace components

    HPX_CXX_EXPORT HPX_EXPORT components::server::runtime_support*
    get_runtime_support_ptr();
}    // namespace hpx
