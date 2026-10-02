// Copyright (c) 2026 the-ivii
//
// SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>

#include <iostream>

int hpx_main(int, char*[])
{
    std::cout << "Hello World!\n";
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return hpx::local::init(&hpx_main, argc, argv);
}
