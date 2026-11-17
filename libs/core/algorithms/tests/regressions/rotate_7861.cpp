//  Copyright (c) 2026 Mohammad Izaan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// #7861: rotate_sender test aborts with std::bad_array_new_length

#include <hpx/config.hpp>
#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <numeric>
#include <vector>

void test_rotate_sender(std::ptrdiff_t mid_pos)
{
    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;

    std::vector<std::size_t> c(10007);
    std::iota(c.begin(), c.end(), 0);
    std::vector<std::size_t> d = c;

    using scheduler_t =
        ex::thread_pool_policy_scheduler<hpx::launch::async_policy>;
    auto exec =
        ex::explicit_scheduler_executor(scheduler_t(hpx::launch::async));

    hpx::rotate(
        hpx::execution::par.on(exec), c.begin(), c.begin() + mid_pos, c.end());

    std::rotate(d.begin(), d.begin() + mid_pos, d.end());

    HPX_TEST(std::equal(c.begin(), c.end(), d.begin()));

    // Test sender pipeline (task policy)
    tt::sync_wait(
        hpx::rotate(hpx::execution::par(hpx::execution::task).on(exec),
            c.begin(), c.begin() + mid_pos, c.end()));

    std::rotate(d.begin(), d.begin() + mid_pos, d.end());

    HPX_TEST(std::equal(c.begin(), c.end(), d.begin()));
}

void test_with_first_core()
{
    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;

    // Test scheduler configured with with_first_core on its own
    // (without an explicit with_processing_units_count)
    using scheduler_t =
        ex::thread_pool_policy_scheduler<hpx::launch::async_policy>;
    auto sched = ex::with_first_core(scheduler_t(hpx::launch::async), 1);

    int const n = 10;
    std::atomic<int> count{0};
    tt::sync_wait(
        ex::schedule(sched) | ex::bulk(n, [&count](int) { ++count; }));
    HPX_TEST_EQ(count.load(), n);

    std::vector<std::size_t> v(100, 0);
    auto exec = ex::explicit_scheduler_executor(sched);
    hpx::for_each(hpx::execution::par.on(exec), v.begin(), v.end(),
        [](std::size_t& x) { x = 42; });
    HPX_TEST(
        std::all_of(v.begin(), v.end(), [](std::size_t x) { return x == 42; }));
}

int hpx_main()
{
    // Test mid positions, including 4246 (the split point reproduced
    // from issue #7861 with 10007 elements on 2 cores)
    test_rotate_sender(1);
    test_rotate_sender(4246);
    test_rotate_sender(10007 / 2);
    test_rotate_sender(10007 - 2);
    test_rotate_sender(10007 - 1);

    test_with_first_core();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
