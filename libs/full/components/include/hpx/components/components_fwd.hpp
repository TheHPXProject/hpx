//  Copyright (c) 2017-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/components_base.hpp>

namespace hpx::components {

    HPX_CXX_EXPORT template <typename Derived, typename Stub,
        typename ClientData = void>
    class client_base;
}    // namespace hpx::components
