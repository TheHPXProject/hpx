//  Copyright (c) 2017 Ajai V George
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_main.hpp>
#include <hpx/include/parallel_count.hpp>
#include <hpx/include/parallel_transform.hpp>
#include <hpx/include/partitioned_vector_predef.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/testing.hpp>

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

template <typename ExPolicy, typename T, typename U = T>
void test_transform(ExPolicy&& policy, hpx::partitioned_vector<T>& v,
    hpx::partitioned_vector<U>& w, U val)
{
    verify_values(policy, v, val);
    verify_values_count(policy, v, val);

    hpx::transform(policy, v.begin(), v.end(), w.begin(), pfo<U>());

    verify_values(policy, w, 2 * val);
    verify_values_count(policy, w, 2 * val);
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

template <typename ExPolicy, typename T, typename U = T>
void test_transform_async(ExPolicy&& policy, hpx::partitioned_vector<T>& v,
    hpx::partitioned_vector<U>& w, U val)
{
    verify_values(policy, v, val);
    verify_values_count_async(policy, v, val);

    hpx::transform(policy, v.begin(), v.end(), w.begin(), pfo<U>()).get();

    verify_values(policy, w, 2 * val);
    verify_values_count_async(policy, w, 2 * val);
}

///////////////////////////////////////////////////////////////////////////////
template <typename T, typename U = T>
void transform_tests(std::vector<hpx::id_type>& localities)
{
    std::size_t const length = 12;

    {
        hpx::partitioned_vector<T> v;
        hpx::partitioned_vector<U> w;
        hpx::transform(
            hpx::execution::seq, v.begin(), v.end(), w.begin(), pfo<U>());
        hpx::transform(
            hpx::execution::par, v.begin(), v.end(), w.begin(), pfo<U>());
        hpx::transform(hpx::execution::seq(hpx::execution::task), v.begin(),
            v.end(), w.begin(), pfo<U>())
            .get();
        hpx::transform(hpx::execution::par(hpx::execution::task), v.begin(),
            v.end(), w.begin(), pfo<U>())
            .get();
    }

    {
        hpx::partitioned_vector<T> v(
            length, T(1), hpx::container_layout(localities));
        hpx::partitioned_vector<U> w(length, hpx::container_layout(localities));
        test_transform(hpx::execution::seq, v, w, U(1));
        test_transform(hpx::execution::par, v, w, U(1));
        test_transform_async(
            hpx::execution::seq(hpx::execution::task), v, w, U(1));
        test_transform_async(
            hpx::execution::par(hpx::execution::task), v, w, U(1));
    }

    // subrange regression test in a single segment (sit == send path)
    {
        constexpr std::size_t n = 16;
        constexpr std::size_t first_offset = 3;
        constexpr std::size_t range_size = 8;
        constexpr std::size_t dest_offset = 2;

        hpx::partitioned_vector<T> v(n, hpx::container_layout(1));
        hpx::partitioned_vector<U> w(n, U(-1), hpx::container_layout(1));

        auto it = v.begin();
        for (T value = T(0); it != v.end(); ++it, value = T(value + 1))
        {
            *it = value;
        }

        auto first = v.begin();
        std::advance(first, first_offset);
        auto last = first;
        std::advance(last, range_size);
        auto dest = w.begin();
        std::advance(dest, dest_offset);

        auto verify_subrange_result = [&](U unchanged) {
            std::vector<U> expected(n, unchanged);
            for (std::size_t i = 0; i < range_size; ++i)
            {
                expected[dest_offset + i] = U(2 * T(first_offset + i));
            }

            std::size_t idx = 0;
            for (auto wit = w.begin(); wit != w.end(); ++wit, ++idx)
            {
                HPX_TEST_EQ(*wit, expected[idx]);
            }
            HPX_TEST_EQ(idx, n);
        };

        hpx::fill(hpx::execution::seq, w.begin(), w.end(), U(-1));
        hpx::transform(hpx::execution::seq, first, last, dest, pfo<U>());
        verify_subrange_result(U(-1));

        hpx::fill(hpx::execution::seq, w.begin(), w.end(), U(-1));
        hpx::transform(hpx::execution::par, first, last, dest, pfo<U>());
        verify_subrange_result(U(-1));

        hpx::fill(hpx::execution::seq, w.begin(), w.end(), U(-1));
        hpx::transform(hpx::execution::seq(hpx::execution::task), first, last,
            dest, pfo<U>())
            .get();
        verify_subrange_result(U(-1));

        hpx::fill(hpx::execution::seq, w.begin(), w.end(), U(-1));
        hpx::transform(hpx::execution::par(hpx::execution::task), first, last,
            dest, pfo<U>())
            .get();
        verify_subrange_result(U(-1));
    }
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    std::vector<hpx::id_type> localities = hpx::find_all_localities();

    transform_tests<int, int>(localities);

    return hpx::util::report_errors();
}
#endif
