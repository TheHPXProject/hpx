//  Copyright (c) 2026 Mohammad Izaan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>

#include <atomic>
#include <cstdint>
#include <vector>

std::atomic<std::uint64_t> count(0);

std::uint64_t fibonacci(std::uint64_t n)
{
    hpx::scoped_annotation annotate("fibonacci");
    ++count;

    if (n < 2)
        return n;

    hpx::future<std::uint64_t> n1 = hpx::async(&fibonacci, n - 1);
    hpx::future<std::uint64_t> n2 = hpx::async(&fibonacci, n - 2);

    return n1.get() + n2.get();
}

int hpx_main()
{
    {
        hpx::scoped_annotation annotate("main_work");
        std::uint64_t const result = fibonacci(10);
        HPX_TEST_EQ(result, std::uint64_t(55));
        HPX_TEST(count.load() > 0);
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
