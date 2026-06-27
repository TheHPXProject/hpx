//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>

///////////////////////////////////////////////////////////////////////////////
template <typename Executor>
void test_executor(Executor&& ex)
{
    auto exec = hpx::execution::experimental::fork_join_executor_from(
        HPX_FORWARD(Executor, ex));

    auto const uint32_max =
        static_cast<std::size_t>((std::numeric_limits<std::uint32_t>::max)());

    std::size_t const low = uint32_max - 500;
    std::size_t const high = uint32_max + 500;

    // sum of integers [lo, hi) = n*(lo + hi - 1)/2
    std::size_t const n = high - low;
    std::size_t const expected_sum = n * (low + high - 1) / 2;

    std::atomic<std::size_t> sum{0};

    hpx::for_each(hpx::execution::par.on(exec),
        hpx::util::counting_iterator<std::size_t>(low),
        hpx::util::counting_iterator<std::size_t>(high),
        [&sum](std::size_t i) { sum.fetch_add(i, std::memory_order_acq_rel); });

    HPX_TEST_EQ(sum.load(), expected_sum);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_executor(hpx::execution::seq.executor());
    test_executor(hpx::execution::par.executor());
    test_executor(hpx::execution::experimental::fork_join_executor());

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // Initialize and run HPX
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
