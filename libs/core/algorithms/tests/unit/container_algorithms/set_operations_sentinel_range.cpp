//  Copyright (c) 2026 Arnab Chatterjee
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The iterator/sentinel overloads of the ranges set operations must accept a
// sentinel of a different type than the iterator for every execution policy,
// not only for the sequential ones.

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <string>
#include <vector>

using iterator = std::vector<int>::iterator;

struct sentinel
{
    iterator end;

    friend bool operator==(iterator const& it, sentinel const& s)
    {
        return it == s.end;
    }

    friend std::ptrdiff_t operator-(sentinel const& s, iterator const& it)
    {
        return s.end - it;
    }

    friend std::ptrdiff_t operator-(iterator const& it, sentinel const& s)
    {
        return it - s.end;
    }
};

static_assert(std::sized_sentinel_for<sentinel, iterator>);

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename Op, typename StdOp>
void test_set_operation(ExPolicy&& policy, Op op, StdOp std_op)
{
    // two sorted sequences that overlap in every sixth element
    std::vector<int> a(10007);
    std::vector<int> b(5003);
    for (std::size_t i = 0; i != a.size(); ++i)
        a[i] = static_cast<int>(2 * i);
    for (std::size_t i = 0; i != b.size(); ++i)
        b[i] = static_cast<int>(3 * i);

    std::vector<int> expected;
    std_op(a.begin(), a.end(), b.begin(), b.end(),
        std::back_inserter(expected));

    std::vector<int> out(a.size() + b.size());
    auto result = op(policy, a.begin(), sentinel{a.end()}, b.begin(),
        sentinel{b.end()}, out.begin());
    if constexpr (hpx::is_async_execution_policy_v<std::decay_t<ExPolicy>>)
    {
        out.resize(static_cast<std::size_t>(result.get().out - out.begin()));
    }
    else
    {
        out.resize(static_cast<std::size_t>(result.out - out.begin()));
    }

    HPX_TEST(out == expected);
}

template <typename Op, typename StdOp>
void test_all_policies(Op op, StdOp std_op)
{
    test_set_operation(hpx::execution::seq, op, std_op);
    test_set_operation(hpx::execution::par, op, std_op);
    test_set_operation(hpx::execution::seq(hpx::execution::task), op, std_op);
    test_set_operation(hpx::execution::par(hpx::execution::task), op, std_op);
}

void set_operations_sentinel_test()
{
    test_all_policies(
        [](auto&&... args) {
            return hpx::ranges::set_union(
                std::forward<decltype(args)>(args)...);
        },
        [](auto... args) { return std::set_union(args...); });

    test_all_policies(
        [](auto&&... args) {
            return hpx::ranges::set_intersection(
                std::forward<decltype(args)>(args)...);
        },
        [](auto... args) { return std::set_intersection(args...); });

    test_all_policies(
        [](auto&&... args) {
            return hpx::ranges::set_difference(
                std::forward<decltype(args)>(args)...);
        },
        [](auto... args) { return std::set_difference(args...); });

    test_all_policies(
        [](auto&&... args) {
            return hpx::ranges::set_symmetric_difference(
                std::forward<decltype(args)>(args)...);
        },
        [](auto... args) { return std::set_symmetric_difference(args...); });
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    set_operations_sentinel_test();

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
