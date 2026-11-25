//  Copyright (c) 2026 Ishaan400
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// #8129: hpx::ranges::contains (without an execution policy) and the two-range
// hpx::ranges::transform (without an execution policy) did not compile when
// the end of a range was given as a sentinel of a different type than its
// iterator. The two-range transform also read a single-pass second range to
// its end before transforming it.

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <sstream>
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

struct negate
{
    int operator()(int x) const
    {
        return -x;
    }
};

// compares one call of the two-range transform with std::ranges::transform
template <typename Call>
void test_transform(std::vector<int> a, std::vector<int> b, Call&& call)
{
    std::vector<int> expected(a.size() + b.size(), -1);
    auto const exp = std::ranges::transform(
        a, b, expected.begin(), std::plus<>{}, negate{}, negate{});

    std::vector<int> dest(a.size() + b.size(), -1);
    auto const r = call(a, b, dest);

    HPX_TEST(r.in1 == a.begin() + (exp.in1 - a.begin()));
    HPX_TEST(r.in2 == b.begin() + (exp.in2 - b.begin()));
    HPX_TEST(r.out == dest.begin() + (exp.out - expected.begin()));
    HPX_TEST(dest == expected);
}

template <typename Sent>
void test(std::vector<int> c, std::vector<int> other)
{
    for (int val : {0, 1, 3, 96})
    {
        HPX_TEST_EQ(hpx::ranges::contains(c.begin(), Sent{c.end()}, val),
            (std::ranges::find(c, val) != c.end()));
    }

    // both ranges end with a sentinel, with and without projections
    test_transform(c, other, [](auto& a, auto& b, auto& dest) {
        return hpx::ranges::transform(a.begin(), Sent{a.end()}, b.begin(),
            Sent{b.end()}, dest.begin(), std::plus<>{}, negate{}, negate{});
    });

    std::vector<int> expected(c.size() + other.size(), -1);
    auto const exp =
        std::ranges::transform(c, other, expected.begin(), std::plus<>{});

    std::vector<int> dest(c.size() + other.size(), -1);
    auto const r = hpx::ranges::transform(c.begin(), Sent{c.end()},
        other.begin(), Sent{other.end()}, dest.begin(), std::plus<>{});
    HPX_TEST(r.in1 == c.begin() + (exp.in1 - c.begin()));
    HPX_TEST(r.in2 == other.begin() + (exp.in2 - other.begin()));
    HPX_TEST(r.out == dest.begin() + (exp.out - expected.begin()));
    HPX_TEST(dest == expected);
}

// the overloads that take an execution policy need a sized sentinel
template <typename ExPolicy>
void test_policy(ExPolicy policy, std::vector<int> c, std::vector<int> other)
{
    using Sent = sentinel<true>;

    for (int val : {0, 1, 3, 96})
    {
        HPX_TEST_EQ(
            get(hpx::ranges::contains(policy, c.begin(), Sent{c.end()}, val)),
            (std::ranges::find(c, val) != c.end()));
    }

    test_transform(c, other, [&](auto& a, auto& b, auto& dest) {
        return get(hpx::ranges::transform(policy, a.begin(), Sent{a.end()},
            b.begin(), Sent{b.end()}, dest.begin(), std::plus<>{}, negate{},
            negate{}));
    });
}

// a single-pass second range must not be read ahead to find its end
template <typename Sent>
void test_single_pass(std::vector<int> c)
{
    std::istringstream is("10 20 30");
    std::vector<int> dest(c.size(), -1);
    auto const r = hpx::ranges::transform(c.begin(), c.end(),
        std::istream_iterator<int>(is), Sent{}, dest.begin(), std::plus<>{});

    std::vector<int> const b = {10, 20, 30};
    std::vector<int> expected(c.size(), -1);
    auto const exp =
        std::ranges::transform(c, b, expected.begin(), std::plus<>{});

    HPX_TEST(r.in1 == c.begin() + (exp.in1 - c.begin()));
    HPX_TEST(r.out == dest.begin() + (exp.out - expected.begin()));
    HPX_TEST(dest == expected);
}

int hpx_main()
{
    using namespace hpx::execution;

    for (auto const& c :
        std::vector<std::vector<int>>{{}, {1, 2}, {1, 2, 3}, {1, 2, 3, 4}})
    {
        test_single_pass<std::istream_iterator<int>>(c);
        test_single_pass<std::default_sentinel_t>(c);
    }

    std::vector<int> large(10007);
    for (std::size_t i = 0; i != large.size(); ++i)
        large[i] = static_cast<int>(i % 97);

    // the second range is shorter, as long, or longer than the first one
    std::vector<std::vector<int>> const inputs = {
        {}, {1}, {1, 2}, {3, 2, 1}, {5, 1, 4, 2, 3}, large};

    for (auto const& c : inputs)
    {
        for (auto const& other : inputs)
        {
            test<sentinel<true>>(c, other);
            test<sentinel<false>>(c, other);

            test_policy(seq, c, other);
            test_policy(par, c, other);
            test_policy(par_unseq, c, other);
            test_policy(seq(task), c, other);
            test_policy(par(task), c, other);
        }
    }

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
