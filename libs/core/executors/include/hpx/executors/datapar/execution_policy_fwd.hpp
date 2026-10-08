//  Copyright (c) 2016-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)
#include <hpx/executors/execution_policy_fwd.hpp>

#include <cstddef>

namespace hpx::execution::detail {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <std::size_t N = 0>
    struct simd_policy_shim;

    HPX_CXX_CORE_EXPORT template <std::size_t N = 0>
    struct simd_task_policy_shim;

    HPX_CXX_CORE_EXPORT template <std::size_t N = 0>
    struct par_simd_policy_shim;

    HPX_CXX_CORE_EXPORT template <std::size_t N = 0>
    struct par_simd_task_policy_shim;
}    // namespace hpx::execution::detail

#endif
