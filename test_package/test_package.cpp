//  Copyright (c) 2026 Pratyksh Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx_init.hpp>
#include <hpx/iostream.hpp>
#include <hpx/future.hpp>

int hpx_main()
{
    // Print a simple message using HPX
    hpx::cout << "Hello from HPX! HPX is working correctly." << hpx::endl;

    // Test basic future functionality
    hpx::future<int> f = hpx::async([]() { return 42; });
    hpx::cout << "The answer is: " << f.get() << hpx::endl;

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    return hpx::init(argc, argv);
}
