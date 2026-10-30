//  Copyright (c) 2026 Vaatsalya Srivastava
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// A static chunk size that is larger than the number of elements used to make
// chunk_size_iterator report additional, empty chunks (and iterators that never
// reached the end of the range). Algorithms that don't expect empty chunks
// (reduce, transform_reduce, inclusive_scan, exclusive_scan) crashed, and
// min_element returned wrong results.

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/numeric.hpp>

#include <cstddef>
#include <functional>
#include <numeric>
#include <vector>

template <typename ExPolicy>
void test_larger_chunk_size(ExPolicy&& policy, std::size_t count)
{
    std::vector<int> v(count);
    std::iota(v.begin(), v.end(), 1);

    HPX_TEST_EQ(hpx::reduce(policy, v.begin(), v.end(), 0),
        std::accumulate(v.begin(), v.end(), 0));

    HPX_TEST_EQ(hpx::transform_reduce(policy, v.begin(), v.end(), 0,
                    std::plus<>(), [](int x) { return 2 * x; }),
        2 * std::accumulate(v.begin(), v.end(), 0));

    {
        std::vector<int> d1(count, -1);
        std::vector<int> d2(count, -1);
        hpx::inclusive_scan(policy, v.begin(), v.end(), d1.begin());
        std::inclusive_scan(v.begin(), v.end(), d2.begin());
        HPX_TEST(d1 == d2);
    }

    {
        std::vector<int> d1(count, -1);
        std::vector<int> d2(count, -1);
        hpx::exclusive_scan(policy, v.begin(), v.end(), d1.begin(), 5);
        std::exclusive_scan(v.begin(), v.end(), d2.begin(), 5);
        HPX_TEST(d1 == d2);
    }

    // the smallest element is the first one
    HPX_TEST(hpx::min_element(policy, v.begin(), v.end()) == v.begin());
}

int hpx_main()
{
    std::size_t const chunk_sizes[] = {2, 3, 4, 10, 64};

    for (std::size_t count = 1; count != 11; ++count)
    {
        for (std::size_t const chunk_size : chunk_sizes)
        {
            test_larger_chunk_size(
                hpx::execution::par.with(
                    hpx::execution::experimental::static_chunk_size(
                        chunk_size)),
                count);
        }
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
