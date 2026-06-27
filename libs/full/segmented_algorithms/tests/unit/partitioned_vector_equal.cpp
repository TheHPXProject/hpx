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
// equal tests
///////////////////////////////////////////////////////////////////////////////
template <typename T, typename DistPolicy, typename ExPolicy>
void equal_algo_tests_with_policy(
    std::size_t size, DistPolicy const& policy, ExPolicy const& equal_policy)
{
    hpx::partitioned_vector<T> v1(size, policy);
    hpx::partitioned_vector<T> v2(size, policy);

    // Fill both vectors identically
    iota_vector(v1, T(0));
    iota_vector(v2, T(0));

    // Test Match
    bool result = hpx::equal(equal_policy, v1.begin(), v1.end(), v2.begin());
    HPX_TEST(result);

    // Mutate v2 to cause a mismatch
    auto it = v2.begin();
    std::advance(it, size / 2);
    *it = T(9999);

    // Test Mismatch
    result = hpx::equal(equal_policy, v1.begin(), v1.end(), v2.begin());
    HPX_TEST(!result);
}

template <typename T, typename DistPolicy, typename ExPolicy>
void equal_algo_tests_with_policy_async(
    std::size_t size, DistPolicy const& policy, ExPolicy const& equal_policy)
{
    hpx::partitioned_vector<T> v1(size, policy);
    hpx::partitioned_vector<T> v2(size, policy);

    iota_vector(v1, T(0));
    iota_vector(v2, T(0));

    // Test Match (Async)
    auto f1 = hpx::equal(equal_policy, v1.begin(), v1.end(), v2.begin());
    HPX_TEST(f1.get());

    // Mutate
    auto it = v2.begin();
    std::advance(it, size / 2);
    *it = T(9999);

    // Test Mismatch (Async)
    auto f2 = hpx::equal(equal_policy, v1.begin(), v1.end(), v2.begin());
    HPX_TEST(!f2.get());
}

///////////////////////////////////////////////////////////////////////////////
template <typename T, typename DistPolicy>
void equal_tests_with_policy(
    std::size_t size, std::size_t, DistPolicy const& policy)
{
    using namespace hpx::execution;

    equal_algo_tests_with_policy<T>(size, policy, seq);
    equal_algo_tests_with_policy<T>(size, policy, par);

    equal_algo_tests_with_policy_async<T>(size, policy, seq(task));
    equal_algo_tests_with_policy_async<T>(size, policy, par(task));
}

template <typename T>
void equal_empty_tests()
{
    hpx::partitioned_vector<T> v1;
    hpx::partitioned_vector<T> v2;

    using namespace hpx::execution;

    HPX_TEST(hpx::equal(seq, v1.begin(), v1.end(), v2.begin()));
    HPX_TEST(hpx::equal(par, v1.begin(), v1.end(), v2.begin()));
    HPX_TEST(hpx::equal(seq(task), v1.begin(), v1.end(), v2.begin()).get());
    HPX_TEST(hpx::equal(par(task), v1.begin(), v1.end(), v2.begin()).get());
}

template <typename T>
void equal_tests()
{
    equal_empty_tests<T>();

    std::size_t const length = 64;
    std::vector<hpx::id_type> localities = hpx::find_all_localities();

    equal_tests_with_policy<T>(length, 1, hpx::container_layout);
    equal_tests_with_policy<T>(length, 3, hpx::container_layout(3));
    equal_tests_with_policy<T>(length, 3, hpx::container_layout(3, localities));
    equal_tests_with_policy<T>(
        length, localities.size(), hpx::container_layout(localities));
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    equal_tests<int>();
    equal_tests<double>();

    return hpx::util::report_errors();
}

#endif
