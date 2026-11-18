//  Copyright (c) 2026 Ishaan400
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// hpx::ranges::reverse_copy returned first instead of last as the input
// iterator of its result, and hpx::ranges::rotate_copy with a parallel policy
// returned middle instead of last.

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <string>
#include <type_traits>
#include <vector>

template <typename Result>
auto get(Result&& r)
{
    if constexpr (hpx::traits::is_future_v<std::decay_t<Result>>)
        return r.get();
    else
        return r;
}

template <typename ExPolicy>
void test_reverse_copy(ExPolicy policy, std::size_t size)
{
    std::vector<int> c(size);
    std::iota(c.begin(), c.end(), 0);

    std::vector<int> d(size);
    auto r = get(hpx::ranges::reverse_copy(policy, c, d.begin()));
    HPX_TEST(r.in == c.end());
    HPX_TEST(r.out == d.end());
    HPX_TEST(std::equal(c.rbegin(), c.rend(), d.begin()));

    std::vector<int> e(size);
    auto r2 =
        get(hpx::ranges::reverse_copy(policy, c.begin(), c.end(), e.begin()));
    HPX_TEST(r2.in == c.end());
    HPX_TEST(r2.out == e.end());
    HPX_TEST(d == e);
}

template <typename ExPolicy>
void test_rotate_copy(ExPolicy policy, std::size_t size)
{
    std::vector<int> c(size);
    std::iota(c.begin(), c.end(), 0);

    for (std::size_t mid : {std::size_t(0), size / 3, size})
    {
        auto middle = c.begin() + static_cast<std::ptrdiff_t>(mid);

        std::vector<int> expected(size);
        std::rotate_copy(c.begin(), middle, c.end(), expected.begin());

        std::vector<int> d(size);
        auto r = get(hpx::ranges::rotate_copy(policy, c, middle, d.begin()));
        HPX_TEST(r.in == c.end());
        HPX_TEST(r.out == d.end());
        HPX_TEST(d == expected);

        std::vector<int> e(size);
        auto r2 = get(hpx::ranges::rotate_copy(
            policy, c.begin(), middle, c.end(), e.begin()));
        HPX_TEST(r2.in == c.end());
        HPX_TEST(r2.out == e.end());
        HPX_TEST(e == expected);
    }
}

template <typename ExPolicy>
void test(ExPolicy policy)
{
    for (std::size_t size : {0, 1, 2, 5, 10007})
    {
        test_reverse_copy(policy, size);
        test_rotate_copy(policy, size);
    }
}

int hpx_main()
{
    using namespace hpx::execution;

    test(seq);
    test(par);
    test(par_unseq);
    test(seq(task));
    test(par(task));

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    hpx::local::init_params init_args;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
