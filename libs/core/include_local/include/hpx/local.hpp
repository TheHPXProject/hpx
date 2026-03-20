//  Copyright (c) 2026 Hartmut Kaiser
//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/local.hpp
/// \brief Single-include convenience header for single-node HPX usage.
///
/// This header bundles the most commonly used HPX facilities for local
/// (single-node) execution. It is intended to reduce include boilerplate
/// for quick prototyping, browser-based compilers (Compiler Explorer), and
/// educational examples.
///
/// For implicit main() wrapping (so you can write a plain main() that runs
/// inside the HPX runtime), also include \c \<hpx/hpx_main.hpp\> before
/// this header.

#pragma once

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/numeric.hpp>
