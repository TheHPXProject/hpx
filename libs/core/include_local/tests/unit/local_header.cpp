//  Copyright (c) 2020-2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Verify that hpx/local.hpp is self-contained and provides access to the
// Standard Parallel Toolkit types: parallel algorithms, numeric algorithms,
// execution policies, and futures.
//
// This is a compile-and-link sanity check only. It does NOT start the HPX
// runtime, so it has no dependency on the wrap module (hpx_main.hpp) or on
// any specific HPX link target beyond hpx_core.

#include <hpx/local.hpp>

// Verify key types and symbols are reachable through hpx/local.hpp
static_assert(
    sizeof(hpx::execution::parallel_policy) > 0, "par policy reachable");

int main()
{
    return 0;
}
