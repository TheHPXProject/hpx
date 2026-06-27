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
template <typename T, typename InIter>
void verify_values(
    InIter first, InIter last, T const& val, bool must_be_equal = true)
{
    for (InIter it = first; it != last; ++it)
    {
        if (must_be_equal)
        {
            HPX_TEST_EQ(*it, val);
        }
        else
        {
            HPX_TEST_NEQ(*it, val);
        }
    }
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
// replace tests
///////////////////////////////////////////////////////////////////////////////
template <typename T, typename DistPolicy, typename ExPolicy>
void replace_algo_tests_with_policy(
    std::size_t size, DistPolicy const& policy, ExPolicy const& replace_policy)
{
    hpx::partitioned_vector<T> c(size, policy);

    // fill with known value and replace all
    T const initial(42);
    T const replacement(99);
    hpx::fill(hpx::execution::seq, c.begin(), c.end(), initial);
    hpx::replace(replace_policy, c.begin(), c.end(), initial, replacement);
    verify_vector(c, replacement);

    // replace on a sub-range
    hpx::fill(hpx::execution::seq, c.begin(), c.end(), initial);
    hpx::replace(
        replace_policy, c.begin() + 1, c.end() - 1, initial, replacement);
    verify_values(c.begin() + 1, c.end() - 1, replacement);
    verify_values(c.begin(), c.begin() + 1, replacement, false);
    verify_values(c.end() - 1, c.end(), replacement, false);

    // replace with value that doesn't match (nothing should change)
    hpx::fill(hpx::execution::seq, c.begin(), c.end(), initial);
    T const non_matching(12);
    hpx::replace(replace_policy, c.begin(), c.end(), non_matching, replacement);
    verify_vector(c, initial);
}

template <typename T, typename DistPolicy, typename ExPolicy>
void replace_algo_tests_with_policy_async(
    std::size_t size, DistPolicy const& policy, ExPolicy const& replace_policy)
{
    hpx::partitioned_vector<T> c(size, policy);

    T const initial(42);
    T const replacement(99);
    hpx::fill(hpx::execution::seq, c.begin(), c.end(), initial);
    hpx::future<void> f =
        hpx::replace(replace_policy, c.begin(), c.end(), initial, replacement);
    f.wait();
    verify_vector(c, replacement);

    hpx::fill(hpx::execution::seq, c.begin(), c.end(), initial);
    hpx::future<void> f1 = hpx::replace(
        replace_policy, c.begin() + 1, c.end() - 1, initial, replacement);
    f1.wait();
    verify_values(c.begin() + 1, c.end() - 1, replacement);
    verify_values(c.begin(), c.begin() + 1, replacement, false);
    verify_values(c.end() - 1, c.end(), replacement, false);
}

///////////////////////////////////////////////////////////////////////////////
// replace_if tests
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
void replace_if_algo_tests_with_policy(
    std::size_t size, DistPolicy const& policy, ExPolicy const& replace_policy)
{
    hpx::partitioned_vector<T> c(size, policy);
    iota_vector(c, T(0));

    T const replacement(99);

    // replace_if: replace all even values
    hpx::replace_if(
        replace_policy, c.begin(), c.end(), is_even<T>{}, replacement);

    // verify: all even positions should be replacement, odd positions unchanged
    T expected_val(0);
    for (auto it = c.begin(); it != c.end();
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
void replace_if_algo_tests_with_policy_async(
    std::size_t size, DistPolicy const& policy, ExPolicy const& replace_policy)
{
    hpx::partitioned_vector<T> c(size, policy);
    iota_vector(c, T(0));

    T const replacement(99);

    hpx::future<void> f = hpx::replace_if(
        replace_policy, c.begin(), c.end(), is_even<T>{}, replacement);
    f.wait();

    T expected_val(0);
    for (auto it = c.begin(); it != c.end();
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
void replace_tests_with_policy(
    std::size_t size, std::size_t /* localities */, DistPolicy const& policy)
{
    using namespace hpx::execution;

    replace_algo_tests_with_policy<T>(size, policy, seq);
    replace_algo_tests_with_policy<T>(size, policy, par);

    replace_algo_tests_with_policy_async<T>(size, policy, seq(task));
    replace_algo_tests_with_policy_async<T>(size, policy, par(task));

    replace_if_algo_tests_with_policy<T>(size, policy, seq);
    replace_if_algo_tests_with_policy<T>(size, policy, par);

    replace_if_algo_tests_with_policy_async<T>(size, policy, seq(task));
    replace_if_algo_tests_with_policy_async<T>(size, policy, par(task));
}

template <typename T>
void replace_tests()
{
    std::size_t const length = 12;
    std::vector<hpx::id_type> localities = hpx::find_all_localities();

    replace_tests_with_policy<T>(length, 1, hpx::container_layout);
    replace_tests_with_policy<T>(length, 3, hpx::container_layout(3));
    replace_tests_with_policy<T>(
        length, 3, hpx::container_layout(3, localities));
    replace_tests_with_policy<T>(
        length, localities.size(), hpx::container_layout(localities));
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    replace_tests<int>();
    replace_tests<double>();

    return hpx::util::report_errors();
}
#endif
