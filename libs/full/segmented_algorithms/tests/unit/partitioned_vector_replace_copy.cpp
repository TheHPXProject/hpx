//  Copyright (c) 2026 Abhishek Bansal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_main.hpp>
#include <hpx/include/parallel_replace.hpp>
#include <hpx/include/partitioned_vector_predef.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <iterator>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// The vector types to be used are defined in partitioned_vector module.
// HPX_REGISTER_PARTITIONED_VECTOR(double)
// HPX_REGISTER_PARTITIONED_VECTOR(int)

///////////////////////////////////////////////////////////////////////////////
template <typename T>
void iota_vector(hpx::partitioned_vector<T>& v, T val)
{
    typename hpx::partitioned_vector<T>::iterator it = v.begin(), end = v.end();
    for (/**/; it != end; ++it)
        *it = val++;
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
void verify_vector(hpx::partitioned_vector<T> const& v, T const& val)
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

///////////////////////////////////////////////////////////////////////////////
// replace_copy tests
///////////////////////////////////////////////////////////////////////////////
template <typename T, typename DistPolicy, typename ExPolicy>
void replace_copy_algo_tests_with_policy(std::size_t size,
    DistPolicy const& policy, ExPolicy const& replace_copy_policy)
{
    hpx::partitioned_vector<T> src(size, policy);
    hpx::partitioned_vector<T> dest(size, policy);

    T const initial(42);
    T const replacement(99);

    // fill source with initial value, then replace_copy to dest
    hpx::fill(hpx::execution::seq, src.begin(), src.end(), initial);
    hpx::fill(hpx::execution::seq, dest.begin(), dest.end(), T(0));

    hpx::replace_copy(replace_copy_policy, src.begin(), src.end(), dest.begin(),
        initial, replacement);

    // source should be unchanged
    verify_vector(src, initial);
    // dest should have the replaced values
    verify_vector(dest, replacement);

    // replace_copy with non-matching value (dest should be copy of source)
    hpx::fill(hpx::execution::seq, dest.begin(), dest.end(), T(0));
    T const non_matching(12);
    hpx::replace_copy(replace_copy_policy, src.begin(), src.end(), dest.begin(),
        non_matching, replacement);

    // source unchanged
    verify_vector(src, initial);
    // dest should be a copy of source (no replacement happened)
    verify_vector(dest, initial);
}

template <typename T, typename DistPolicy, typename ExPolicy>
void replace_copy_algo_tests_with_policy_async(std::size_t size,
    DistPolicy const& policy, ExPolicy const& replace_copy_policy)
{
    hpx::partitioned_vector<T> src(size, policy);
    hpx::partitioned_vector<T> dest(size, policy);

    T const initial(42);
    T const replacement(99);

    hpx::fill(hpx::execution::seq, src.begin(), src.end(), initial);
    hpx::fill(hpx::execution::seq, dest.begin(), dest.end(), T(0));

    auto f = hpx::replace_copy(replace_copy_policy, src.begin(), src.end(),
        dest.begin(), initial, replacement);
    f.wait();

    verify_vector(src, initial);
    verify_vector(dest, replacement);
}

///////////////////////////////////////////////////////////////////////////////
// replace_copy_if tests
///////////////////////////////////////////////////////////////////////////////
template <typename T>
struct is_even
{
    bool operator()(T const& val) const
    {
        return static_cast<int>(val) % 2 == 0;
    }

    template <typename Archive>
    void serialize(Archive& /* ar */, unsigned /* version */)
    {
    }
};

template <typename T, typename DistPolicy, typename ExPolicy>
void replace_copy_if_algo_tests_with_policy(std::size_t size,
    DistPolicy const& policy, ExPolicy const& replace_copy_policy)
{
    hpx::partitioned_vector<T> src(size, policy);
    hpx::partitioned_vector<T> dest(size, policy);

    iota_vector(src, T(0));
    hpx::fill(hpx::execution::seq, dest.begin(), dest.end(), T(0));

    T const replacement(99);

    hpx::replace_copy_if(replace_copy_policy, src.begin(), src.end(),
        dest.begin(), is_even<T>{}, replacement);

    // verify source is unchanged
    T expected_src(0);
    for (auto it = src.begin(); it != src.end();
        ++it, expected_src = expected_src + T(1))
    {
        HPX_TEST_EQ(*it, expected_src);
    }

    // verify dest: evens replaced, odds copied
    T expected_val(0);
    for (auto it = dest.begin(); it != dest.end();
        ++it, expected_val = expected_val + T(1))
    {
        if (static_cast<int>(expected_val) % 2 == 0)
        {
            HPX_TEST_EQ(*it, replacement);
        }
        else
        {
            HPX_TEST_EQ(*it, expected_val);
        }
    }
}

template <typename T, typename DistPolicy, typename ExPolicy>
void replace_copy_if_algo_tests_with_policy_async(std::size_t size,
    DistPolicy const& policy, ExPolicy const& replace_copy_policy)
{
    hpx::partitioned_vector<T> src(size, policy);
    hpx::partitioned_vector<T> dest(size, policy);

    iota_vector(src, T(0));
    hpx::fill(hpx::execution::seq, dest.begin(), dest.end(), T(0));

    T const replacement(99);

    auto f = hpx::replace_copy_if(replace_copy_policy, src.begin(), src.end(),
        dest.begin(), is_even<T>{}, replacement);
    f.wait();

    // verify dest: evens replaced, odds copied
    T expected_val(0);
    for (auto it = dest.begin(); it != dest.end();
        ++it, expected_val = expected_val + T(1))
    {
        if (static_cast<int>(expected_val) % 2 == 0)
        {
            HPX_TEST_EQ(*it, replacement);
        }
        else
        {
            HPX_TEST_EQ(*it, expected_val);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
template <typename T, typename DistPolicy>
void replace_copy_tests_with_policy(
    std::size_t size, std::size_t /* localities */, DistPolicy const& policy)
{
    using namespace hpx::execution;

    replace_copy_algo_tests_with_policy<T>(size, policy, seq);
    replace_copy_algo_tests_with_policy<T>(size, policy, par);

    replace_copy_algo_tests_with_policy_async<T>(size, policy, seq(task));
    replace_copy_algo_tests_with_policy_async<T>(size, policy, par(task));

    replace_copy_if_algo_tests_with_policy<T>(size, policy, seq);
    replace_copy_if_algo_tests_with_policy<T>(size, policy, par);

    replace_copy_if_algo_tests_with_policy_async<T>(size, policy, seq(task));
    replace_copy_if_algo_tests_with_policy_async<T>(size, policy, par(task));
}

///////////////////////////////////////////////////////////////////////////////
// regressions for subrange handling
///////////////////////////////////////////////////////////////////////////////
template <typename T>
void replace_copy_subrange_regressions()
{
    using namespace hpx::execution;

    // replace_copy subrange regression in a single segment (sit == send path)
    {
        constexpr std::size_t n = 16;
        constexpr std::size_t first_offset = 3;
        constexpr std::size_t range_size = 8;
        constexpr std::size_t dest_offset = 2;

        hpx::partitioned_vector<T> src(n, hpx::container_layout(1));
        hpx::partitioned_vector<T> out(n, T(-1), hpx::container_layout(1));

        iota_vector(src, T(0));

        auto first = src.begin();
        std::advance(first, first_offset);
        auto last = first;
        std::advance(last, range_size);
        auto dest = out.begin();
        std::advance(dest, dest_offset);

        auto verify_subrange_result = [&]() {
            std::vector<T> expected(n, T(-1));
            for (std::size_t i = 0; i < range_size; ++i)
            {
                T v = T(first_offset + i);
                expected[dest_offset + i] = (v == T(5)) ? T(50) : v;
            }

            std::size_t idx = 0;
            for (auto oit = out.begin(); oit != out.end(); ++oit, ++idx)
            {
                HPX_TEST_EQ(*oit, expected[idx]);
            }
            HPX_TEST_EQ(idx, n);
        };

        hpx::fill(seq, out.begin(), out.end(), T(-1));
        hpx::replace_copy(seq, first, last, dest, T(5), T(50));
        verify_subrange_result();

        hpx::fill(seq, out.begin(), out.end(), T(-1));
        hpx::replace_copy(par, first, last, dest, T(5), T(50));
        verify_subrange_result();

        hpx::fill(seq, out.begin(), out.end(), T(-1));
        hpx::replace_copy(seq(task), first, last, dest, T(5), T(50)).get();
        verify_subrange_result();

        hpx::fill(seq, out.begin(), out.end(), T(-1));
        hpx::replace_copy(par(task), first, last, dest, T(5), T(50)).get();
        verify_subrange_result();
    }
}

template <typename T>
void replace_copy_empty_tests()
{
    hpx::partitioned_vector<T> src;
    hpx::partitioned_vector<T> dest;

    T const initial(42);
    T const replacement(99);

    using namespace hpx::execution;

    // replace_copy
    hpx::replace_copy(
        seq, src.begin(), src.end(), dest.begin(), initial, replacement);
    hpx::replace_copy(
        par, src.begin(), src.end(), dest.begin(), initial, replacement);
    hpx::replace_copy(
        seq(task), src.begin(), src.end(), dest.begin(), initial, replacement)
        .wait();
    hpx::replace_copy(
        par(task), src.begin(), src.end(), dest.begin(), initial, replacement)
        .wait();

    // replace_copy_if
    hpx::replace_copy_if(
        seq, src.begin(), src.end(), dest.begin(), is_even<T>{}, replacement);
    hpx::replace_copy_if(
        par, src.begin(), src.end(), dest.begin(), is_even<T>{}, replacement);
    hpx::replace_copy_if(seq(task), src.begin(), src.end(), dest.begin(),
        is_even<T>{}, replacement)
        .wait();
    hpx::replace_copy_if(par(task), src.begin(), src.end(), dest.begin(),
        is_even<T>{}, replacement)
        .wait();

    HPX_TEST_EQ(src.size(), std::size_t(0));
    HPX_TEST_EQ(dest.size(), std::size_t(0));
}

template <typename T>
void replace_copy_tests()
{
    replace_copy_empty_tests<T>();

    std::size_t const length = 12;
    std::vector<hpx::id_type> localities = hpx::find_all_localities();

    replace_copy_tests_with_policy<T>(length, 1, hpx::container_layout);
    replace_copy_tests_with_policy<T>(length, 3, hpx::container_layout(3));
    replace_copy_tests_with_policy<T>(
        length, 3, hpx::container_layout(3, localities));
    replace_copy_tests_with_policy<T>(
        length, localities.size(), hpx::container_layout(localities));
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    replace_copy_tests<int>();
    replace_copy_tests<double>();

    replace_copy_subrange_regressions<int>();

    return hpx::util::report_errors();
}
#endif
