// Copyright (c) 2026 the-ivii
//
// SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx_main.hpp>
#include <hpx/experimental/sandbox.hpp>

#include <iostream>

int main()
{
    hpx::experimental::sandbox::describe_environment(std::cout);
    std::cout << "Hello wrap-main\n";
    return 0;
}
