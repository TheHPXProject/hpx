//  Copyright (c) 2016-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/compute.hpp>
#include <hpx/modules/compute_local.hpp>

#if defined(HPX_HAVE_CUDA) || defined(HPX_HAVE_HIP)
#include <hpx/modules/async_cuda.hpp>
#endif
#if defined(HPX_HAVE_SYCL)
#include <hpx/modules/async_sycl.hpp>
#endif
