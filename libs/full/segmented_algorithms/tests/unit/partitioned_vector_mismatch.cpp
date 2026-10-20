//  Copyright (c) 2026 Abir Roy
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_main.hpp>
#include <hpx/include/partitioned_vector_predef.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/segmented_algorithms.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <iterator>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
template <typename T>
void iota_vector(hpx::partitioned_vector<T>& v, T val)
{
    typename hpx::partitioned_vector<T>::iterator it = v.begin(), end = v.end();
    for (; it != end; ++it)
        *it = val++;
}

///////////////////////////////////////////////////////////////////////////////
// mismatch tests
///////////////////////////////////////////////////////////////////////////////
template <typename T, typename DistPolicy, typename ExPolicy>
void mismatch_algo_tests_with_policy(
    std::size_t size, DistPolicy const& policy, ExPolicy const& mismatch_policy)
{
    hpx::partitioned_vector<T> v1(size, policy);
    hpx::partitioned_vector<T> v2(size, policy);

    // Fill both vectors identically
    iota_vector(v1, T(0));
    iota_vector(v2, T(0));

    // Test Match
    auto res3 =
        hpx::mismatch(mismatch_policy, v1.begin(), v1.end(), v2.begin());
    HPX_TEST(res3.first == v1.end() && res3.second == v2.end());

    auto res4 = hpx::mismatch(
        mismatch_policy, v1.begin(), v1.end(), v2.begin(), v2.end());
    HPX_TEST(res4.first == v1.end() && res4.second == v2.end());

    // Calculate exact midpoint iterators
    auto it1 = v1.begin();
    std::advance(it1, size / 2);
    auto it2 = v2.begin();
    std::advance(it2, size / 2);

    // Mutate v2 to cause a mismatch at the midpoint
    *it2 = T(9999);

    // Test Mismatch
    res3 = hpx::mismatch(mismatch_policy, v1.begin(), v1.end(), v2.begin());
    HPX_TEST(res3.first == it1 && res3.second == it2);

    res4 = hpx::mismatch(
        mismatch_policy, v1.begin(), v1.end(), v2.begin(), v2.end());
    HPX_TEST(res4.first == it1 && res4.second == it2);
}

template <typename T, typename DistPolicy, typename ExPolicy>
void mismatch_algo_tests_with_policy_async(
    std::size_t size, DistPolicy const& policy, ExPolicy const& mismatch_policy)
{
    hpx::partitioned_vector<T> v1(size, policy);
    hpx::partitioned_vector<T> v2(size, policy);

    iota_vector(v1, T(0));
    iota_vector(v2, T(0));

    // Test Match (Async)
    auto f1_3 =
        hpx::mismatch(mismatch_policy, v1.begin(), v1.end(), v2.begin());
    auto res1_3 = f1_3.get();
    HPX_TEST(res1_3.first == v1.end() && res1_3.second == v2.end());

    auto f1_4 = hpx::mismatch(
        mismatch_policy, v1.begin(), v1.end(), v2.begin(), v2.end());
    auto res1_4 = f1_4.get();
    HPX_TEST(res1_4.first == v1.end() && res1_4.second == v2.end());

    // Mutate
    auto it1 = v1.begin();
    std::advance(it1, size / 2);
    auto it2 = v2.begin();
    std::advance(it2, size / 2);
    *it2 = T(9999);

    // Test Mismatch (Async)
    auto f2_3 =
        hpx::mismatch(mismatch_policy, v1.begin(), v1.end(), v2.begin());
    auto res2_3 = f2_3.get();
    HPX_TEST(res2_3.first == it1 && res2_3.second == it2);

    auto f2_4 = hpx::mismatch(
        mismatch_policy, v1.begin(), v1.end(), v2.begin(), v2.end());
    auto res2_4 = f2_4.get();
    HPX_TEST(res2_4.first == it1 && res2_4.second == it2);
}

///////////////////////////////////////////////////////////////////////////////
template <typename T, typename DistPolicy>
void mismatch_tests_with_policy(
    std::size_t size, std::size_t, DistPolicy const& policy)
{
    using namespace hpx::execution;

    mismatch_algo_tests_with_policy<T>(size, policy, seq);
    mismatch_algo_tests_with_policy<T>(size, policy, par);

    mismatch_algo_tests_with_policy_async<T>(size, policy, seq(task));
    mismatch_algo_tests_with_policy_async<T>(size, policy, par(task));
}

template <typename T>
void mismatch_empty_tests()
{
    hpx::partitioned_vector<T> v1;
    hpx::partitioned_vector<T> v2;

    using namespace hpx::execution;

    auto r_seq = hpx::mismatch(seq, v1.begin(), v1.end(), v2.begin(), v2.end());
    HPX_TEST(r_seq.first == v1.end() && r_seq.second == v2.end());

    auto r_par = hpx::mismatch(par, v1.begin(), v1.end(), v2.begin(), v2.end());
    HPX_TEST(r_par.first == v1.end() && r_par.second == v2.end());

    auto r_seq_t =
        hpx::mismatch(seq(task), v1.begin(), v1.end(), v2.begin(), v2.end())
            .get();
    HPX_TEST(r_seq_t.first == v1.end() && r_seq_t.second == v2.end());

    auto r_par_t =
        hpx::mismatch(par(task), v1.begin(), v1.end(), v2.begin(), v2.end())
            .get();
    HPX_TEST(r_par_t.first == v1.end() && r_par_t.second == v2.end());
}

template <typename T>
void mismatch_subrange_tests(std::size_t first_partitions,
    std::size_t second_partitions, std::ptrdiff_t first_offset,
    std::ptrdiff_t second_offset,
    std::vector<hpx::id_type> const& first_localities,
    std::vector<hpx::id_type> const& second_localities)
{
    hpx::partitioned_vector<T> v1(
        8, hpx::container_layout(first_partitions, first_localities));
    hpx::partitioned_vector<T> v2(
        8, hpx::container_layout(second_partitions, second_localities));
    iota_vector(v1, T(0));
    iota_vector(v2, T(first_offset) - T(second_offset));

    auto first1 = std::next(v1.begin(), first_offset);
    auto last1 = std::next(first1, 6);
    auto first2 = std::next(v2.begin(), second_offset);
    using namespace hpx::execution;

    auto check = [&](auto expected1, auto expected2, auto... pred) {
        auto check_result = [&](auto const& result) {
            HPX_TEST(result.first == expected1);
            HPX_TEST(result.second == expected2);
        };
        check_result(hpx::mismatch(seq, first1, last1, first2, pred...));
        check_result(hpx::mismatch(par, first1, last1, first2, pred...));
        check_result(
            hpx::mismatch(seq(task), first1, last1, first2, pred...).get());
        check_result(
            hpx::mismatch(par(task), first1, last1, first2, pred...).get());
    };

    check(last1, std::next(first2, 6));
    *std::next(first2, 6) = T(9999);
    check(last1, std::next(first2, 6));

    *std::next(first2, 5) = T(9999);
    check(std::next(first1, 5), std::next(first2, 5));
    *std::next(first2, 3) = T(9999);
    check(std::next(first1, 3), std::next(first2, 3));
    *first2 = T(9999);
    check(first1, first2);

    iota_vector(v2, T(first_offset) - T(second_offset) + T(100));
    check(last1, std::next(first2, 6),
        [](T left, T right) { return left + T(100) == right; });
}

template <typename T>
void mismatch_tests()
{
    mismatch_empty_tests<T>();
    auto check_subranges =
        [](std::vector<hpx::id_type> const& first_localities,
            std::vector<hpx::id_type> const& second_localities) {
        mismatch_subrange_tests<T>(
            2, 2, 0, 0, first_localities, second_localities);
        mismatch_subrange_tests<T>(
            2, 2, 1, 0, first_localities, second_localities);
        mismatch_subrange_tests<T>(
            2, 2, 0, 1, first_localities, second_localities);
        mismatch_subrange_tests<T>(
            1, 3, 1, 0, first_localities, second_localities);
        mismatch_subrange_tests<T>(
            3, 1, 0, 1, first_localities, second_localities);
        mismatch_subrange_tests<T>(
            3, 2, 1, 0, first_localities, second_localities);
    };
    check_subranges({hpx::find_here()}, {hpx::find_here()});

    std::size_t const length = 64;
    std::vector<hpx::id_type> localities = hpx::find_all_localities();
    if (localities.size() >= 2)
    {
        check_subranges(localities, localities);
        check_subranges({localities[0]}, {localities[1]});
        check_subranges({localities[1]}, {localities[0]});
    }

    mismatch_tests_with_policy<T>(length, 1, hpx::container_layout);
    mismatch_tests_with_policy<T>(length, 3, hpx::container_layout(3));
    mismatch_tests_with_policy<T>(
        length, 3, hpx::container_layout(3, localities));
    mismatch_tests_with_policy<T>(
        length, localities.size(), hpx::container_layout(localities));
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    mismatch_tests<int>();
    mismatch_tests<double>();

    return hpx::util::report_errors();
}

#endif
