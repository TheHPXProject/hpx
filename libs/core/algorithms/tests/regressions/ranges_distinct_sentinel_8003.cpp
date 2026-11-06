//  Copyright (c) 2026 Ishaan400
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// #8003: hpx::ranges::adjacent_find, is_sorted_until, is_heap_until, includes,
// find_end, find_first_of and move did not compile (for some or all execution
// policies) when the end of a range was given as a sentinel of a different
// type than its iterator.

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <string>
#include <type_traits>
#include <vector>

using iterator = std::vector<int>::iterator;

// a sentinel that is not an iterator; Sized adds the differences needed to
// make it a sized sentinel
template <bool Sized>
struct sentinel
{
    iterator end;

    friend bool operator==(iterator const& it, sentinel const& s)
    {
        return it == s.end;
    }

    friend std::ptrdiff_t operator-(sentinel const& s, iterator const& it)
        requires(Sized)
    {
        return s.end - it;
    }

    friend std::ptrdiff_t operator-(iterator const& it, sentinel const& s)
        requires(Sized)
    {
        return it - s.end;
    }
};

template <typename Result>
auto get(Result&& r)
{
    if constexpr (hpx::traits::is_future_v<std::decay_t<Result>>)
        return r.get();
    else
        return r;
}

template <typename Sent, typename ExPolicy>
void test(ExPolicy policy, std::vector<int> c, std::vector<int> const& s)
{
    std::vector<int> needle = s;
    auto first = c.begin();
    Sent last{c.end()};
    auto nfirst = needle.begin();
    Sent nlast{needle.end()};

    HPX_TEST(get(hpx::ranges::adjacent_find(policy, first, last)) ==
        std::ranges::adjacent_find(c));
    HPX_TEST(get(hpx::ranges::is_sorted_until(policy, first, last)) ==
        std::ranges::is_sorted_until(c));
    HPX_TEST(get(hpx::ranges::is_heap_until(policy, first, last)) ==
        std::ranges::is_heap_until(c));
    HPX_TEST(get(hpx::ranges::find_end(policy, first, last, nfirst, nlast)) ==
        std::ranges::find_end(c, needle).begin());
    HPX_TEST(get(hpx::ranges::find_first_of(policy, first, last, nfirst,
                 nlast)) == std::ranges::find_first_of(c, needle));

    std::vector<int> sorted = c;
    std::ranges::sort(sorted);
    std::vector<int> sorted_needle = needle;
    std::ranges::sort(sorted_needle);
    HPX_TEST_EQ(
        get(hpx::ranges::includes(policy, sorted.begin(), Sent{sorted.end()},
            sorted_needle.begin(), Sent{sorted_needle.end()})),
        std::ranges::includes(sorted, sorted_needle));

    std::vector<int> dest(c.size());
    auto r = get(hpx::ranges::move(policy, first, last, dest.begin()));
    HPX_TEST(r.in == c.end());
    HPX_TEST(r.out == dest.end());
    HPX_TEST(dest == c);
}

template <typename Sent, typename ExPolicy>
void test(ExPolicy policy)
{
    std::vector<int> large(10007);
    for (std::size_t i = 0; i != large.size(); ++i)
        large[i] = static_cast<int>(i % 97);

    std::vector<std::vector<int>> const inputs = {{}, {1}, {1, 2}, {1, 2, 2, 3},
        {3, 2, 1}, {5, 1, 4, 2, 3}, {1, 2, 3, 1, 2, 3}, large};
    std::vector<std::vector<int>> const needles = {
        {}, {2}, {2, 3}, {9}, {1, 2, 3}};

    for (auto const& c : inputs)
    {
        for (auto const& s : needles)
        {
            test<Sent>(policy, c, s);
        }
    }
}

template <typename ExPolicy>
void test(ExPolicy policy)
{
    test<sentinel<true>>(policy);
    test<sentinel<false>>(policy);
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
