//  Copyright (c) 2026 Vaatsalya Srivastava
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// With a parallel execution policy includes used to return true although the
// second range contained a run of equal elements that was not in the first
// range: a chunk that consisted only of elements covered by the previous chunk
// returned early, and the run starting at the next chunk was never verified.

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <vector>

using vec = std::vector<int>;

template <typename ExPolicy>
void test_includes(ExPolicy&& policy, vec const& a, vec const& b)
{
    HPX_TEST_EQ(hpx::includes(policy, a.begin(), a.end(), b.begin(), b.end()),
        std::includes(a.begin(), a.end(), b.begin(), b.end()));
}

void test_minimal_case()
{
    namespace ex = hpx::execution;

    // 3 does not occur in a, for a chunk size of 2 the second chunk consists
    // only of elements covered by the first chunk
    vec const a{0, 0, 1, 1};
    vec const b{0, 0, 1, 1, 3, 3};

    test_includes(ex::par.with(ex::experimental::static_chunk_size(2)), a, b);

    // the same for a chunk size of 3, 2 does not occur in a
    vec const a3{0, 1, 1, 1, 1, 1};
    vec const b3{0, 1, 1, 1, 1, 1, 2, 2};

    test_includes(ex::par.with(ex::experimental::static_chunk_size(3)), a3, b3);
}

// all sorted multisets over {0, 1, 2} with at most max_size elements
void generate(std::size_t max_size, int first_value, vec& current,
    std::vector<vec>& result)
{
    result.push_back(current);

    if (current.size() == max_size)
        return;

    for (int i = first_value; i <= 2; ++i)
    {
        current.push_back(i);
        generate(max_size, i, current, result);
        current.pop_back();
    }
}

void test_exhaustive()
{
    namespace ex = hpx::execution;

    std::vector<vec> first_ranges;
    std::vector<vec> second_ranges;
    vec current;

    generate(6, 0, current, first_ranges);
    generate(8, 0, current, second_ranges);

    for (std::size_t chunk_size = 1; chunk_size != 5; ++chunk_size)
    {
        auto const policy =
            ex::par.with(ex::experimental::static_chunk_size(chunk_size));

        for (vec const& a : first_ranges)
        {
            for (vec const& b : second_ranges)
            {
                test_includes(policy, a, b);
            }
        }
    }
}

int hpx_main()
{
    test_minimal_case();
    test_exhaustive();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
