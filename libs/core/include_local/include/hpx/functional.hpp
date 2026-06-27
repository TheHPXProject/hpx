//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2022-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/threading_base.hpp>

namespace hpx {

    HPX_CXX_CORE_EXPORT using hpx::invoke_fused;
    HPX_CXX_CORE_EXPORT using hpx::mem_fn;
}    // namespace hpx
