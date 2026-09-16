//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_main.hpp>
#include <hpx/include/parallel_is_sorted.hpp>
#include <hpx/include/parallel_sort.hpp>
#include <hpx/include/partitioned_vector_predef.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/segmented_algorithms.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
#define SIZE 64

template <typename T>
std::vector<T> copy_values(hpx::partitioned_vector<T> const& values)
{
    std::vector<T> result;
    result.reserve(values.size());
    typename hpx::partitioned_vector<T>::const_iterator it = values.begin();
    typename hpx::partitioned_vector<T>::const_iterator end = values.end();
    for (/**/; it != end; ++it)
    {
        result.push_back(*it);
    }
    return result;
}

template <typename T>
void verify_sorted(
    hpx::partitioned_vector<T> const& values, std::vector<T> expected)
{
    std::sort(expected.begin(), expected.end());
    HPX_TEST(hpx::is_sorted(values.begin(), values.end()));

    std::vector<T> const got = copy_values(values);
    HPX_TEST_EQ(got.size(), expected.size());
    for (std::size_t i = 0; i != got.size(); ++i)
    {
        HPX_TEST_EQ(got[i], expected[i]);
    }
}

template <typename T, typename Comp>
void verify_sorted(hpx::partitioned_vector<T> const& values,
    std::vector<T> expected, Comp comp)
{
    std::sort(expected.begin(), expected.end(), comp);
    HPX_TEST(hpx::is_sorted(values.begin(), values.end(), comp));

    std::vector<T> const got = copy_values(values);
    HPX_TEST_EQ(got.size(), expected.size());
    for (std::size_t i = 0; i != got.size(); ++i)
    {
        HPX_TEST_EQ(got[i], expected[i]);
    }
}

template <typename T>
void initialize_reverse(hpx::partitioned_vector<T>& values)
{
    typename hpx::partitioned_vector<T>::iterator it = values.begin();
    for (int i = 0; i < SIZE; ++i, ++it)
        *it = T(SIZE - i);
}

template <typename T>
void initialize_mixed(hpx::partitioned_vector<T>& values)
{
    typename hpx::partitioned_vector<T>::iterator it = values.begin();
    for (int i = 0; i < SIZE; ++i, ++it)
        *it = T((i * 17 + 3) % SIZE);
}

template <typename T>
void initialize_sorted(hpx::partitioned_vector<T>& values)
{
    typename hpx::partitioned_vector<T>::iterator it = values.begin();
    for (int i = 0; i < SIZE; ++i, ++it)
        *it = T(i);
}

template <typename T>
void initialize_duplicates(hpx::partitioned_vector<T>& values)
{
    typename hpx::partitioned_vector<T>::iterator it = values.begin();
    for (int i = 0; i < SIZE; ++i, ++it)
        *it = T(i % 4);
}

template <typename T>
void test_sort_once(hpx::partitioned_vector<T>& values)
{
    std::vector<T> const expected = copy_values(values);
    hpx::sort(values.begin(), values.end());
    verify_sorted(values, expected);
}

template <typename ExPolicy, typename T>
void test_sort_once(ExPolicy&& policy, hpx::partitioned_vector<T>& values)
{
    std::vector<T> const expected = copy_values(values);
    hpx::sort(HPX_FORWARD(ExPolicy, policy), values.begin(), values.end());
    verify_sorted(values, expected);
}

template <typename ExPolicy, typename T>
void test_sort_once_async(ExPolicy&& policy, hpx::partitioned_vector<T>& values)
{
    std::vector<T> const expected = copy_values(values);
    hpx::sort(HPX_FORWARD(ExPolicy, policy), values.begin(), values.end())
        .get();
    verify_sorted(values, expected);
}

template <typename T>
void test_sort_greater(hpx::partitioned_vector<T>& values)
{
    std::vector<T> const expected = copy_values(values);
    std::greater<T> comp;
    hpx::sort(values.begin(), values.end(), comp);
    verify_sorted(values, expected, comp);
}

template <typename T>
void run_cases(std::vector<hpx::id_type>& localities)
{
    hpx::partitioned_vector<T> values(
        SIZE, T(0), hpx::container_layout(localities));

    initialize_reverse(values);
    test_sort_once(values);

    initialize_mixed(values);
    test_sort_once(hpx::execution::seq, values);

    initialize_mixed(values);
    test_sort_once(hpx::execution::par, values);

    initialize_mixed(values);
    test_sort_once_async(hpx::execution::seq(hpx::execution::task), values);

    initialize_mixed(values);
    test_sort_once_async(hpx::execution::par(hpx::execution::task), values);

    initialize_sorted(values);
    test_sort_once(values);

    initialize_duplicates(values);
    test_sort_once(values);

    initialize_mixed(values);
    test_sort_greater(values);

    hpx::partitioned_vector<T> empty(
        0, T(0), hpx::container_layout(localities));
    test_sort_once(empty);
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    std::vector<hpx::id_type> localities = hpx::find_all_localities();
    run_cases<int>(localities);
    return hpx::util::report_errors();
}
#endif
