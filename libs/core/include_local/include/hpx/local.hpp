//  Copyright (c) 2020-2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/local.hpp
/// \brief Single-include convenience header for single-node HPX usage.
///
/// This header bundles the **Standard Parallel Toolkit** — the most commonly
/// used HPX facilities for local (single-node) execution:
///
///   - \c hpx/algorithm.hpp  — Parallel STL algorithms (for_each, sort, ...)
///   - \c hpx/execution.hpp  — Execution policies (par, par_unseq, seq)
///   - \c hpx/future.hpp     — Async primitives (hpx::async, hpx::future)
///   - \c hpx/numeric.hpp    — Parallel numeric algorithms (reduce, ...)
///
/// **Selection criteria**: each header is part of the HPX core module,
/// provides ISO C++ Standard Library parallel equivalents, and has no
/// dependency on the distributed runtime or networking layer.
///
/// In local-only builds (HPX_WITH_DISTRIBUTED_RUNTIME=OFF), this header
/// also pulls in \c hpx/hpx_main.hpp for implicit main() wrapping, so
/// users can write a plain \c main() that runs inside the HPX runtime.

#pragma once

#include <hpx/config.hpp>

// --- Standard Parallel Toolkit (core, no networking dependency) ---
#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/numeric.hpp>

// In local-only builds the wrap module is part of core, so we can safely
// include hpx_main.hpp for zero-boilerplate usage. In full (distributed)
// builds, hpx_main.hpp lives in the 'full' runtime layer and including
// it from a core header would create a circular module dependency.
#if !defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
#include <hpx/hpx_main.hpp>
#endif
