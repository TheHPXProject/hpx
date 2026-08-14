//  Copyright (c) 2025-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// test for availability of C++26 contracts (compiler and library support)

#include <version>
#include <contracts>

#if !defined(__cpp_contracts)
#error "__cpp_contracts not defined, assume contracts are not supported"
#endif

#if !defined(__cpp_lib_contracts)
#error "__cpp_lib_contracts not defined, assume contracts are not supported"
#endif

// Test actual contract syntax support (for experimental implementations)
int main() pre(true) post(r : r == 0)
{
    contract_assert(true);
    return 0;
}
