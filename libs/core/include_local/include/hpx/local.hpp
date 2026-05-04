//  Copyright (c) 2020-2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/local.hpp
/// \brief Single-include convenience header for single-node HPX usage.
///
/// This header bundles the **Standard Parallel Toolkit** -- the most commonly
/// used HPX facilities for local (single-node) execution:
///
///   - \c hpx/modules/algorithms.hpp -- Parallel algorithms (for_each, sort, ...)
///   - \c hpx/modules/execution.hpp  -- Execution policies (par, par_unseq, seq)
///   - \c hpx/modules/futures.hpp    -- Futures and dataflow
///   - \c hpx/numeric.hpp            -- Parallel numeric (reduce, transform_reduce, ...)
///
/// **Selection criteria**: each header is part of the HPX core module,
/// provides ISO C++ Standard Library parallel equivalents, and has no
/// dependency on the distributed runtime or networking layer.
///
/// \note  This header intentionally does NOT include hpx/hpx_main.hpp.
///        Including hpx_main.hpp has observable side effects: it emits
///        non-weak symbol definitions and redefines 'main' via a
///        preprocessor macro. Users who need the zero-boilerplate HPX
///        runtime entry-point should include hpx/hpx_main.hpp explicitly.

#pragma once

#include <hpx/config.hpp>

// --- Standard Parallel Toolkit (core, no networking dependency) ---
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/numeric.hpp>
