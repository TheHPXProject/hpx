//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR_EVE)

#include <eve/eve.hpp>
#include <eve/std.hpp>

namespace hpx::datapar::experimental {

    HPX_CXX_CORE_EXPORT using namespace eve::experimental;
}    // namespace hpx::datapar::experimental

#endif
