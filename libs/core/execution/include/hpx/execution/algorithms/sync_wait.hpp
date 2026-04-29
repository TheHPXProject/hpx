//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2022 Hartmut Kaiser
//  Copyright (c) 2022 Chuanqiu He
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <hpx/execution_base/stdexec_forward.hpp>

namespace hpx::this_thread::experimental {

    HPX_CXX_CORE_EXPORT using hpx::execution::experimental::sync_wait;
    HPX_CXX_CORE_EXPORT using hpx::execution::experimental::sync_wait_t;

    HPX_CXX_CORE_EXPORT using hpx::execution::experimental::
        sync_wait_with_variant;
    HPX_CXX_CORE_EXPORT using hpx::execution::experimental::
        sync_wait_with_variant_t;
}    // namespace hpx::this_thread::experimental
