//  Copyright (c) 2017 Ajai V George
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/include/parallel_count.hpp>
#include <hpx/include/parallel_transform.hpp>
#include <hpx/include/partitioned_vector_predef.hpp>

#include <cstddef>
#include <iterator>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
template <typename U>
struct pfo
{
    template <typename T>
    U operator()(T& val) const
    {
        return U((T(2) * val));
    }
};

template <typename U>
struct add
{
    template <typename T1, typename T2>
    U operator()(T1 const& v1, T2 const& v2) const
    {
        return U(v1 + v2);
    }
};

template <typename T>
struct cmp
{
    cmp(T const& val = T())
      : value_(val)
    {
    }

    template <typename T_>
    bool operator()(T_ const& val) const
    {
        return val == value_;
    }

    T value_;

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        // clang-format off
        ar & value_;
        // clang-format on
    }
};

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename T, typename U = T>
void verify_values(
    ExPolicy&&, hpx::partitioned_vector<T> const& v, U const& val)
{
    typedef typename hpx::partitioned_vector<T>::const_iterator const_iterator;

    std::size_t size = 0;

    const_iterator end = v.end();
    for (const_iterator it = v.begin(); it != end; ++it, ++size)
    {
        HPX_TEST_EQ(*it, val);
    }

    HPX_TEST_EQ(size, v.size());
}

template <typename ExPolicy, typename T, typename U = T>
void verify_values_count(
    ExPolicy&& policy, hpx::partitioned_vector<T> const& v, U const& val)
{
    HPX_TEST_EQ(
        std::size_t(hpx::count(policy, v.begin(), v.end(), val)), v.size());
    HPX_TEST_EQ(
        std::size_t(hpx::count_if(policy, v.begin(), v.end(), cmp<T>(val))),
        v.size());
}

template <typename ExPolicy, typename T, typename U = T, typename V = T>
void test_transform_binary(ExPolicy&& policy, hpx::partitioned_vector<T>& v,
    hpx::partitioned_vector<U>& w, hpx::partitioned_vector<V>& x, V val)
{
    verify_values(policy, v, val);
    verify_values_count(policy, v, val);
    verify_values(policy, w, val);
    verify_values_count(policy, w, val);

    hpx::transform(policy, v.begin(), v.end(), w.begin(), x.begin(), add<V>());

    verify_values(policy, x, 2 * val);
    verify_values_count(policy, x, 2 * val);
}

template <typename ExPolicy, typename T, typename U = T>
void verify_values_count_async(
    ExPolicy&& policy, hpx::partitioned_vector<T> const& v, U const& val)
{
    HPX_TEST_EQ(std::size_t(hpx::count(policy, v.begin(), v.end(), val).get()),
        v.size());
    HPX_TEST_EQ(
        std::size_t(
            hpx::count_if(policy, v.begin(), v.end(), cmp<T>(val)).get()),
        v.size());
}

template <typename ExPolicy, typename T, typename U = T, typename V = T>
void test_transform_binary_async(ExPolicy&& policy,
    hpx::partitioned_vector<T>& v, hpx::partitioned_vector<U>& w,
    hpx::partitioned_vector<V>& x, V val)
{
    verify_values(policy, v, val);
    verify_values_count_async(policy, v, val);
    verify_values(policy, w, val);
    verify_values_count_async(policy, w, val);

    hpx::transform(policy, v.begin(), v.end(), w.begin(), x.begin(), add<V>())
        .get();

    verify_values(policy, x, 2 * val);
    verify_values_count_async(policy, x, 2 * val);
}

template <typename T, typename U = T, typename V = T>
void transform_binary_tests(std::vector<hpx::id_type>& localities)
{
    std::size_t const length = 12;

    {
        hpx::partitioned_vector<T> v;
        hpx::partitioned_vector<U> w;
        hpx::partitioned_vector<V> x;
        hpx::transform(hpx::execution::seq, v.begin(), v.end(), w.begin(),
            x.begin(), add<V>());
        hpx::transform(hpx::execution::par, v.begin(), v.end(), w.begin(),
            x.begin(), add<V>());
        hpx::transform(hpx::execution::seq(hpx::execution::task), v.begin(),
            v.end(), w.begin(), x.begin(), add<V>())
            .get();
        hpx::transform(hpx::execution::par(hpx::execution::task), v.begin(),
            v.end(), w.begin(), x.begin(), add<V>())
            .get();
    }

    {
        hpx::partitioned_vector<T> v(
            length, T(1), hpx::container_layout(localities));
        hpx::partitioned_vector<U> w(
            length, U(1), hpx::container_layout(localities));
        hpx::partitioned_vector<V> x(length, hpx::container_layout(localities));
        test_transform_binary(hpx::execution::seq, v, w, x, V(1));
        test_transform_binary(hpx::execution::par, v, w, x, V(1));
        test_transform_binary_async(
            hpx::execution::seq(hpx::execution::task), v, w, x, V(1));
        test_transform_binary_async(
            hpx::execution::par(hpx::execution::task), v, w, x, V(1));
    }

    // subrange regression test in a single segment (sit == send path)
    {
        constexpr std::size_t n = 16;
        constexpr std::size_t first_offset = 3;
        constexpr std::size_t range_size = 8;
        constexpr std::size_t second_offset = 3;
        constexpr std::size_t dest_offset = 2;

        hpx::partitioned_vector<T> v(n, hpx::container_layout(1));
        hpx::partitioned_vector<U> w(n, hpx::container_layout(1));
        hpx::partitioned_vector<V> x(n, V(-1), hpx::container_layout(1));

        auto itv = v.begin();
        auto itw = w.begin();
        for (std::size_t i = 0; i < n; ++i, ++itv, ++itw)
        {
            *itv = T(i);
            *itw = U(i + 1);
        }

        auto first1 = v.begin();
        std::advance(first1, first_offset);
        auto last1 = first1;
        std::advance(last1, range_size);
        auto first2 = w.begin();
        std::advance(first2, second_offset);
        auto dest = x.begin();
        std::advance(dest, dest_offset);

        auto verify_subrange_result = [&]() {
            std::vector<V> expected(n, V(-1));
            for (std::size_t i = 0; i < range_size; ++i)
            {
                expected[dest_offset + i] =
                    V(T(first_offset + i) + U(second_offset + 1 + i));
            }

            std::size_t idx = 0;
            for (auto it = x.begin(); it != x.end(); ++it, ++idx)
            {
                HPX_TEST_EQ(*it, expected[idx]);
            }
            HPX_TEST_EQ(idx, n);
        };

        hpx::fill(hpx::execution::seq, x.begin(), x.end(), V(-1));
        hpx::transform(
            hpx::execution::seq, first1, last1, first2, dest, add<V>());
        verify_subrange_result();

        hpx::fill(hpx::execution::seq, x.begin(), x.end(), V(-1));
        hpx::transform(
            hpx::execution::par, first1, last1, first2, dest, add<V>());
        verify_subrange_result();

        hpx::fill(hpx::execution::seq, x.begin(), x.end(), V(-1));
        hpx::transform(hpx::execution::seq(hpx::execution::task), first1, last1,
            first2, dest, add<V>())
            .get();
        verify_subrange_result();

        hpx::fill(hpx::execution::seq, x.begin(), x.end(), V(-1));
        hpx::transform(hpx::execution::par(hpx::execution::task), first1, last1,
            first2, dest, add<V>())
            .get();
        verify_subrange_result();
    }
}
#endif
