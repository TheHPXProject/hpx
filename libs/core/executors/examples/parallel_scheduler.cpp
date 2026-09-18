// Copyright (c) 2026 the-ivii
//
// SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The purpose of this example is to show the P2079R10 parallel_scheduler
// entry point: schedule work with get_parallel_scheduler() and wait for it.

#include <hpx/execution.hpp>
#include <hpx/init.hpp>

#include <iostream>
#include <utility>

int hpx_main()
{
    namespace ex = hpx::execution::experimental;

    auto snd = ex::schedule(ex::get_parallel_scheduler()) |
        ex::then([] { return 42; });
    auto [val] = ex::sync_wait(std::move(snd)).value();

    std::cout << "parallel_scheduler ran, got " << val << "\n";

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return hpx::local::init(hpx_main, argc, argv);
}
