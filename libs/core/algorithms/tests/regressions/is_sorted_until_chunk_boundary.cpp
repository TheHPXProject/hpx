//  Copyright (c) 2026 Mo'men Samir
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <numeric>
#include <vector>

void test_is_sorted_until_chunk_boundary()
{
    constexpr std::size_t size = 64;
    constexpr std::size_t boundary = 32;

    std::vector<std::size_t> c(size);
    std::iota(c.begin(), c.end(), 0);

    // Create inversion at the chunk boundary:
    // compare c[32] with c[31].
    c[boundary] = c[boundary - 1] - 1;

    auto seq_it = hpx::is_sorted_until(hpx::execution::seq, c.begin(), c.end());
    HPX_TEST_EQ(std::size_t(std::distance(c.begin(), seq_it)), boundary);

    auto par_policy = hpx::execution::par.with(
        hpx::execution::experimental::static_chunk_size(boundary));
    auto par_it = hpx::is_sorted_until(par_policy, c.begin(), c.end());
    HPX_TEST_EQ(std::size_t(std::distance(c.begin(), par_it)), boundary);
}

int hpx_main()
{
    test_is_sorted_until_chunk_boundary();
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
