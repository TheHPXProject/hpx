//  Copyright (c) 2014-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_main.hpp>
#include <hpx/include/parallel_copy.hpp>
#include <hpx/include/partitioned_vector_predef.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/segmented_algorithms.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// The vector types to be used are defined in partitioned_vector module.
// HPX_REGISTER_PARTITIONED_VECTOR(double)
// HPX_REGISTER_PARTITIONED_VECTOR(int)

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
void copy_tests(hpx::partitioned_vector<T> const& v1)
{
    hpx::partitioned_vector<T> v2(v1);
    compare_vectors(v1, v2);

    fill_vector(v2, T(43));
    compare_vectors(v1, v2, false);

    hpx::partitioned_vector<T> v3;
    v3 = v1;
    compare_vectors(v1, v3);

    fill_vector(v3, T(43));
    compare_vectors(v1, v3, false);
}

///////////////////////////////////////////////////////////////////////////////
template <typename T, typename DistPolicy, typename ExPolicy>
void copy_algo_tests_with_policy(std::size_t size, std::size_t /* localities */,
    DistPolicy const& policy, ExPolicy const& copy_policy)
{
    hpx::partitioned_vector<T> v1(size, policy);
    fill_vector(v1, T(43));

    hpx::partitioned_vector<T> v2(size, policy);
    auto p = hpx::ranges::copy(copy_policy, v1.begin(), v1.end(), v2.begin());
    HPX_TEST(p.out == v2.end());
    compare_vectors(v1, v2);
}

template <typename T, typename DistPolicy, typename ExPolicy>
void copy_algo_tests_with_policy_async(std::size_t size,
    std::size_t /* localities */, DistPolicy const& policy,
    ExPolicy const& copy_policy)
{
    hpx::partitioned_vector<T> v1(size, policy);
    fill_vector(v1, T(43));

    using hpx::execution::task;

    hpx::partitioned_vector<T> v2(size, policy);
    auto f =
        hpx::ranges::copy(copy_policy(task), v1.begin(), v1.end(), v2.begin());

    HPX_TEST(f.get().out == v2.end());
    compare_vectors(v1, v2);
}

template <typename T, typename DistPolicy>
void copy_tests_with_policy(
    std::size_t size, std::size_t localities, DistPolicy const& policy)
{
    hpx::partitioned_vector<T> v1(size, policy);

    hpx::partitioned_vector<T> v2(v1);
    compare_vectors(v1, v2);

    fill_vector(v2, T(43));
    compare_vectors(v1, v2, false);

    hpx::partitioned_vector<T> v3;
    v3 = v1;
    compare_vectors(v1, v3);

    fill_vector(v3, T(43));
    compare_vectors(v1, v3, false);

    using namespace hpx::execution;

    copy_algo_tests_with_policy<T>(size, localities, policy, seq);
    copy_algo_tests_with_policy<T>(size, localities, policy, par);

    copy_algo_tests_with_policy_async<T>(size, localities, policy, seq);
    copy_algo_tests_with_policy_async<T>(size, localities, policy, par);
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
void copy_tests()
{
    std::size_t const length = 12;
    std::vector<hpx::id_type> localities = hpx::find_all_localities();

    {
        hpx::partitioned_vector<T> v;
        copy_tests(v);
    }

    {
        hpx::partitioned_vector<T> v(length);
        copy_tests(v);
    }

    {
        hpx::partitioned_vector<T> v(length, T(42));
        copy_tests(v);
    }

    {
        std::vector<T> data(length, T(42));
        hpx::partitioned_vector<T> v(data.begin(), data.end());
        copy_tests(v);
    }

    copy_tests_with_policy<T>(length, 1, hpx::container_layout);
    copy_tests_with_policy<T>(length, 3, hpx::container_layout(3));
    copy_tests_with_policy<T>(length, 3, hpx::container_layout(3, localities));
    copy_tests_with_policy<T>(
        length, localities.size(), hpx::container_layout(localities));
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    copy_tests<double>();
    copy_tests<int>();

    return hpx::util::report_errors();
}

#endif
