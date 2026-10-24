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
#include <cstddef>
#include <cstdlib>
#include <numeric>
#include <vector>

void test_rotate_sender(std::size_t mid_pos)
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

int hpx_main()
{
    // Reproduce seed from issue #7861
    std::srand(1792861068);
    std::size_t const mid_pos_seed = std::rand() % 10007;

    test_rotate_sender(mid_pos_seed);
    test_rotate_sender(1);
    test_rotate_sender(10007 / 2);
    test_rotate_sender(10007 - 2);
    test_rotate_sender(10007 - 1);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
