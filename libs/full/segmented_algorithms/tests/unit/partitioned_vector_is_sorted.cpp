//  Copyright (c) 2026 Mo'men Samir
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_main.hpp>
#include <hpx/include/parallel_is_sorted.hpp>

#include <hpx/include/partitioned_vector_predef.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <functional>
#include <iostream>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
#define SIZE 64

template <typename T>
void initialize_sorted(hpx::partitioned_vector<T>& xvalues)
{
    typename hpx::partitioned_vector<T>::iterator it = xvalues.begin();
    for (int i = 0; i < SIZE; ++i, ++it)
        *it = T(i + 1);
}

template <typename T>
void initialize_unsorted(hpx::partitioned_vector<T>& xvalues)
{
    typename hpx::partitioned_vector<T>::iterator it = xvalues.begin();
    for (int i = 0; i < SIZE; ++i, ++it)
    {
        *it = T(i + 1);
        if (i == 32)
            *it = T(30);
    }
}

template <typename T>
void test_is_sorted(hpx::partitioned_vector<T>& sorted_vals,
    hpx::partitioned_vector<T>& unsorted_vals)
{
    HPX_TEST(hpx::is_sorted(sorted_vals.begin(), sorted_vals.end()));
    HPX_TEST(!hpx::is_sorted(unsorted_vals.begin(), unsorted_vals.end()));
    HPX_TEST(!hpx::is_sorted(
        sorted_vals.begin(), sorted_vals.end(), std::greater<T>()));
}

// Policy overload
template <typename ExPolicy, typename T>
void test_is_sorted(ExPolicy&& policy, hpx::partitioned_vector<T>& sorted_vals,
    hpx::partitioned_vector<T>& unsorted_vals)
{
    HPX_TEST(hpx::is_sorted(policy, sorted_vals.begin(), sorted_vals.end()));
    HPX_TEST(
        !hpx::is_sorted(policy, unsorted_vals.begin(), unsorted_vals.end()));
    HPX_TEST(!hpx::is_sorted(
        policy, sorted_vals.begin(), sorted_vals.end(), std::greater<T>()));
}

template <typename ExPolicy, typename T>
void test_is_sorted_async(ExPolicy&& policy,
    hpx::partitioned_vector<T>& sorted_vals,
    hpx::partitioned_vector<T>& unsorted_vals)
{
    HPX_TEST(
        hpx::is_sorted(policy, sorted_vals.begin(), sorted_vals.end()).get());
    HPX_TEST(!hpx::is_sorted(policy, unsorted_vals.begin(), unsorted_vals.end())
            .get());
    HPX_TEST(!hpx::is_sorted(
        policy, sorted_vals.begin(), sorted_vals.end(), std::greater<T>())
            .get());
}

template <typename T>
void test_is_sorted_until(hpx::partitioned_vector<T>& sorted_vals,
    hpx::partitioned_vector<T>& unsorted_vals)
{
    auto result = hpx::is_sorted_until(sorted_vals.begin(), sorted_vals.end());
    HPX_TEST(result == sorted_vals.end());

    result = hpx::is_sorted_until(unsorted_vals.begin(), unsorted_vals.end());
    HPX_TEST_EQ(std::distance(unsorted_vals.begin(), result), 32L);

    result = hpx::is_sorted_until(
        sorted_vals.begin(), sorted_vals.end(), std::greater<T>());
    HPX_TEST_EQ(std::distance(sorted_vals.begin(), result), 1L);
}

template <typename ExPolicy, typename T>
void test_is_sorted_until(ExPolicy&& policy,
    hpx::partitioned_vector<T>& sorted_vals,
    hpx::partitioned_vector<T>& unsorted_vals)
{
    auto result =
        hpx::is_sorted_until(policy, sorted_vals.begin(), sorted_vals.end());
    HPX_TEST(result == sorted_vals.end());

    result = hpx::is_sorted_until(
        policy, unsorted_vals.begin(), unsorted_vals.end());
    HPX_TEST_EQ(std::distance(unsorted_vals.begin(), result), 32L);

    result = hpx::is_sorted_until(
        policy, sorted_vals.begin(), sorted_vals.end(), std::greater<T>());
    HPX_TEST_EQ(std::distance(sorted_vals.begin(), result), 1L);
}

template <typename ExPolicy, typename T>
void test_is_sorted_until_async(ExPolicy&& policy,
    hpx::partitioned_vector<T>& sorted_vals,
    hpx::partitioned_vector<T>& unsorted_vals)
{
    auto result =
        hpx::is_sorted_until(policy, sorted_vals.begin(), sorted_vals.end())
            .get();
    HPX_TEST(result == sorted_vals.end());

    result =
        hpx::is_sorted_until(policy, unsorted_vals.begin(), unsorted_vals.end())
            .get();
    HPX_TEST_EQ(std::distance(unsorted_vals.begin(), result), 32L);

    result = hpx::is_sorted_until(
        policy, sorted_vals.begin(), sorted_vals.end(), std::greater<T>())
                 .get();
    HPX_TEST_EQ(std::distance(sorted_vals.begin(), result), 1L);
}

template <typename T>
void is_sorted_tests(std::vector<hpx::id_type>& localities)
{
    hpx::partitioned_vector<T> sorted_vals(
        SIZE, T(0), hpx::container_layout(localities));
    hpx::partitioned_vector<T> unsorted_vals(
        SIZE, T(0), hpx::container_layout(localities));

    initialize_sorted(sorted_vals);
    initialize_unsorted(unsorted_vals);

    test_is_sorted(sorted_vals, unsorted_vals);

    test_is_sorted(hpx::execution::seq, sorted_vals, unsorted_vals);
    test_is_sorted(hpx::execution::par, sorted_vals, unsorted_vals);

    test_is_sorted_async(
        hpx::execution::seq(hpx::execution::task), sorted_vals, unsorted_vals);
    test_is_sorted_async(
        hpx::execution::par(hpx::execution::task), sorted_vals, unsorted_vals);

    test_is_sorted_until(sorted_vals, unsorted_vals);

    test_is_sorted_until(hpx::execution::seq, sorted_vals, unsorted_vals);
    test_is_sorted_until(hpx::execution::par, sorted_vals, unsorted_vals);

    test_is_sorted_until_async(
        hpx::execution::seq(hpx::execution::task), sorted_vals, unsorted_vals);
    test_is_sorted_until_async(
        hpx::execution::par(hpx::execution::task), sorted_vals, unsorted_vals);
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    std::vector<hpx::id_type> localities = hpx::find_all_localities();
    is_sorted_tests<int>(localities);
    return hpx::util::report_errors();
}
#endif
