//  Copyright (c) 2014-2017 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_main.hpp>
#include <hpx/include/parallel_for_each.hpp>
#include <hpx/include/parallel_move.hpp>
#include <hpx/include/partitioned_vector_predef.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/segmented_algorithms.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// The vector types to be used are defined in partitioned_vector module.
// HPX_REGISTER_PARTITIONED_VECTOR(double)
// HPX_REGISTER_PARTITIONED_VECTOR(int)
//
// typedef std::string string;
// HPX_REGISTER_PARTITIONED_VECTOR(string)

///////////////////////////////////////////////////////////////////////////////
template <typename T>
void fill_vector(hpx::partitioned_vector<T>& v, T const& val)
{
    typename hpx::partitioned_vector<T>::iterator it = v.begin(), end = v.end();
    for (/**/; it != end; ++it)
        *it = val;
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
void compare_vectors(hpx::partitioned_vector<T> const& v1,
    hpx::partitioned_vector<T> const& v2, bool must_be_equal = true)
{
    typedef typename hpx::partitioned_vector<T>::const_iterator const_iterator;

    HPX_TEST_EQ(v1.size(), v2.size());

    const_iterator it1 = v1.begin(), it2 = v2.begin();
    const_iterator end1 = v1.end(), end2 = v2.end();
    for (/**/; it1 != end1 && it2 != end2; ++it1, ++it2)
    {
        if (must_be_equal)
        {
            HPX_TEST_EQ(*it1, *it2);
        }
        else
        {
            HPX_TEST_NEQ(*it1, *it2);
        }
    }
}

template <typename T>
void move_tests(hpx::partitioned_vector<T> const& v1, T value)
{
    hpx::partitioned_vector<T> v2(v1);
    compare_vectors(v1, v2);

    hpx::partitioned_vector<T> v3(std::move(v2));
    compare_vectors(v1, v3);

    fill_vector(v3, T(value));
    compare_vectors(v1, v3, false);

    hpx::partitioned_vector<T> v4;
    v4 = v1;
    compare_vectors(v1, v4);

    hpx::partitioned_vector<T> v5;
    v5 = std::move(v4);
    compare_vectors(v1, v5);

    fill_vector(v5, T(value));
    compare_vectors(v1, v5, false);
}

///////////////////////////////////////////////////////////////////////////////
template <typename T, typename DistPolicy, typename ExPolicy>
void move_algo_tests_with_policy(std::size_t size, std::size_t /* localities */,
    DistPolicy const& policy, ExPolicy const& move_policy, T value)
{
    hpx::partitioned_vector<T> v1(size, policy);
    fill_vector(v1, T(value));

    hpx::partitioned_vector<T> v2(v1);
    compare_vectors(v1, v2);

    hpx::partitioned_vector<T> v3(size, policy);
    auto p = hpx::move(move_policy, v2.begin(), v2.end(), v3.begin());
    HPX_TEST(p == v3.end());
    compare_vectors(v1, v3);
}

template <typename T, typename DistPolicy, typename ExPolicy>
void move_algo_tests_with_policy_async(std::size_t size,
    std::size_t /* localities */, DistPolicy const& policy,
    ExPolicy const& move_policy, T value)
{
    hpx::partitioned_vector<T> v1(size, policy);
    fill_vector(v1, T(value));

    hpx::partitioned_vector<T> v2(v1);
    compare_vectors(v1, v2);

    using hpx::execution::task;

    hpx::partitioned_vector<T> v3(size, policy);
    auto f = hpx::move(move_policy(task), v2.begin(), v2.end(), v3.begin());

    HPX_TEST(f.get() == v3.end());
    compare_vectors(v1, v3);
}

template <typename T, typename DistPolicy>
void move_tests_with_policy(
    std::size_t size, std::size_t localities, DistPolicy const& policy, T value)
{
    hpx::partitioned_vector<T> v1(size, policy);

    hpx::partitioned_vector<T> v2(v1);
    compare_vectors(v1, v2);

    hpx::partitioned_vector<T> v3(std::move(v2));
    compare_vectors(v1, v3);

    fill_vector(v3, T(value));
    compare_vectors(v1, v3, false);

    hpx::partitioned_vector<T> v4;
    v4 = v1;
    compare_vectors(v1, v4);

    hpx::partitioned_vector<T> v5;
    v5 = std::move(v4);
    compare_vectors(v1, v5);

    fill_vector(v5, T(value));
    compare_vectors(v1, v5, false);

    using namespace hpx::execution;

    move_algo_tests_with_policy<T>(size, localities, policy, seq, value);
    move_algo_tests_with_policy<T>(size, localities, policy, par, value);

    move_algo_tests_with_policy_async<T>(size, localities, policy, seq, value);
    move_algo_tests_with_policy_async<T>(size, localities, policy, par, value);
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
void move_tests(T value, T other_value)
{
    std::size_t const length = 12;
    std::vector<hpx::id_type> localities = hpx::find_all_localities();

    {
        hpx::partitioned_vector<T> v;
        move_tests(v, value);
    }

    {
        hpx::partitioned_vector<T> v(length);
        move_tests(v, value);
    }

    {
        hpx::partitioned_vector<T> v(length, T(other_value));
        move_tests(v, value);
    }

    move_tests_with_policy<T>(length, 1, hpx::container_layout, value);
    move_tests_with_policy<T>(length, 3, hpx::container_layout(3), value);
    move_tests_with_policy<T>(
        length, 3, hpx::container_layout(3, localities), value);
    move_tests_with_policy<T>(
        length, localities.size(), hpx::container_layout(localities), value);
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    move_tests<double>(43, 42);
    move_tests<int>(43, 42);
    move_tests<std::string>("test", "not_test");

    return hpx::util::report_errors();
}
#endif
