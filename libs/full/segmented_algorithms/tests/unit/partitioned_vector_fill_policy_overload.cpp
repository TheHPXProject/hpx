//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// An hpx_invoke overload of hpx::fill for segmented iterators and one
// specific execution policy (as the Thrust policies have it) must be
// selected over the segmented fill for all execution policies, whatever the
// value category of the value is. It takes the same parameters and is
// constrained with a concept refining hpx::execution_policy.

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_main.hpp>
#include <hpx/include/parallel_fill.hpp>
#include <hpx/include/partitioned_vector_predef.hpp>
#include <hpx/modules/testing.hpp>

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
// The vector types to be used are defined in partitioned_vector module.
// HPX_REGISTER_PARTITIONED_VECTOR(int)

namespace test {

    // hpx::execution::policy_traits detects the is_policy member
    struct custom_policy
    {
        static constexpr bool is_policy = true;
    };

    template <typename Policy>
    concept custom_execution_policy = hpx::execution_policy<Policy> &&
        std::same_as<std::decay_t<Policy>, custom_policy>;

    int custom_fill_calls = 0;

    // found by ADL through custom_policy
    template <typename Policy, typename SegIter, typename T>
        requires(
            custom_execution_policy<Policy> && hpx::segmented_iterator<SegIter>)
    SegIter hpx_invoke(
        hpx::fill_t, Policy&&, SegIter first, SegIter last, T value)
    {
        ++custom_fill_calls;
        return hpx::fill(hpx::execution::par, first, last, value);
    }
}    // namespace test

///////////////////////////////////////////////////////////////////////////////
void verify_vector(hpx::partitioned_vector<int> const& v, int val)
{
    for (auto it = v.begin(); it != v.end(); ++it)
    {
        HPX_TEST_EQ(*it, val);
    }
}

void test_fill_policy_overload()
{
    hpx::partitioned_vector<int> v(
        107, hpx::container_layout(hpx::find_all_localities()));

    // an rvalue, a non-const lvalue, and a const lvalue value
    auto end = hpx::fill(test::custom_policy{}, v.begin(), v.end(), 1);
    HPX_TEST(end == v.end());
    HPX_TEST_EQ(test::custom_fill_calls, 1);
    verify_vector(v, 1);

    int value = 2;
    hpx::fill(test::custom_policy{}, v.begin(), v.end(), value);
    HPX_TEST_EQ(test::custom_fill_calls, 2);
    verify_vector(v, 2);

    int const const_value = 3;
    test::custom_policy const policy;
    hpx::fill(policy, v.begin(), v.end(), const_value);
    HPX_TEST_EQ(test::custom_fill_calls, 3);
    verify_vector(v, 3);

    // other execution policies still use the segmented fill
    hpx::fill(hpx::execution::seq, v.begin(), v.end(), 4);
    hpx::fill(hpx::execution::par, v.begin(), v.end(), const_value);
    hpx::fill(
        hpx::execution::par(hpx::execution::task), v.begin(), v.end(), value)
        .get();
    HPX_TEST_EQ(test::custom_fill_calls, 3);
    verify_vector(v, 2);
}

int main()
{
    test_fill_policy_overload();
    return hpx::util::report_errors();
}
#endif
