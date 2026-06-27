//  Copyright (c) 2016 Minh-Khanh Do
//  Copyright (c) 2022-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_main.hpp>
#include <hpx/include/parallel_scan.hpp>
#include <hpx/include/partitioned_vector_predef.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/timing.hpp>

#include "partitioned_vector_scan.hpp"

#include <cstddef>
#include <iomanip>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
#define msg7(a, b, c, d, e, f, g)                                              \
    std::cout << std::setw(60) << a << std::setw(40) << b << std::setw(10)     \
              << c << std::setw(6) << " " << #d << " " << e << " " << f << " " \
              << g << " ";
#define msg9(a, b, c, d, e, f, g, h, i)                                        \
    std::cout << std::setw(60) << a << std::setw(40) << b << std::setw(10)     \
              << c << std::setw(6) << " " << #d << " " << e << " " << f << " " \
              << g << " " << h << " " << i << " ";

///////////////////////////////////////////////////////////////////////////////
template <typename T>
struct opt
{
    T operator()(T v1, T v2) const
    {
        return v1 + v2;
    }
};

struct concat_op
{
    std::string operator()(std::string const& lhs, std::string const& rhs) const
    {
        return lhs + ">" + rhs;
    }
};

///////////////////////////////////////////////////////////////////////////////
template <typename T, typename DistPolicy, typename ExPolicy>
void inclusive_scan_algo_tests_with_policy(std::size_t size,
    DistPolicy const& dist_policy, hpx::partitioned_vector<T>& in,
    std::vector<T> ver, ExPolicy const& policy)
{
    msg7(typeid(ExPolicy).name(), typeid(DistPolicy).name(), typeid(T).name(),
        regular, size, dist_policy.get_num_partitions(),
        dist_policy.get_localities().size());
    hpx::chrono::high_resolution_timer t1;

    std::vector<T> out(in.size());
    T val(0);

    double e1 = t1.elapsed();
    t1.restart();

    hpx::inclusive_scan(
        policy, in.begin(), in.end(), out.begin(), opt<T>(), val);

    double e2 = t1.elapsed();
    t1.restart();

    HPX_TEST(std::equal(out.begin(), out.end(), ver.begin()));

    double e3 = t1.elapsed();
    std::cout << std::setprecision(4) << "\t" << e1 << " " << e2 << " " << e3
              << "\n";
}

template <typename T, typename DistPolicy, typename ExPolicy>
void inclusive_scan_algo_tests_segmented_out_with_policy(std::size_t size,
    DistPolicy const& in_dist_policy, DistPolicy const& out_dist_policy,
    hpx::partitioned_vector<T>& in, hpx::partitioned_vector<T>& out,
    std::vector<T> ver, ExPolicy const& policy)
{
    msg9(typeid(ExPolicy).name(), typeid(DistPolicy).name(), typeid(T).name(),
        segmented, size, in_dist_policy.get_num_partitions(),
        in_dist_policy.get_localities().size(),
        out_dist_policy.get_num_partitions(),
        out_dist_policy.get_localities().size());
    hpx::chrono::high_resolution_timer t1;

    T val(0);

    double e1 = t1.elapsed();
    t1.restart();

    hpx::inclusive_scan(
        policy, in.begin(), in.end(), out.begin(), opt<T>(), val);

    double e2 = t1.elapsed();
    t1.restart();

    verify_values(out, ver);

    double e3 = t1.elapsed();
    std::cout << std::setprecision(4) << "\t" << e1 << " " << e2 << " " << e3
              << "\n";
}

template <typename T, typename DistPolicy, typename ExPolicy>
void inclusive_scan_algo_tests_inplace_with_policy(std::size_t size,
    DistPolicy const& dist_policy, std::vector<T> ver, ExPolicy const& policy)
{
    msg7(typeid(ExPolicy).name(), typeid(DistPolicy).name(), typeid(T).name(),
        inplace, size, dist_policy.get_num_partitions(),
        dist_policy.get_localities().size());
    hpx::chrono::high_resolution_timer t1;

    hpx::partitioned_vector<T> in(size, dist_policy);
    iota_vector(in, T(1));

    T val(0);

    double e1 = t1.elapsed();
    t1.restart();

    hpx::inclusive_scan(
        policy, in.begin(), in.end(), in.begin(), opt<T>(), val);

    double e2 = t1.elapsed();
    t1.restart();

    verify_values(in, ver);

    double e3 = t1.elapsed();
    std::cout << std::setprecision(4) << "\t" << e1 << " " << e2 << " " << e3
              << "\n";
}

///////////////////////////////////////////////////////////////////////////////

template <typename T, typename DistPolicy, typename ExPolicy>
void inclusive_scan_algo_tests_with_policy_async(std::size_t size,
    DistPolicy const& dist_policy, hpx::partitioned_vector<T>& in,
    std::vector<T> ver, ExPolicy const& policy)
{
    msg7(typeid(ExPolicy).name(), typeid(DistPolicy).name(), typeid(T).name(),
        async, size, dist_policy.get_num_partitions(),
        dist_policy.get_localities().size());
    hpx::chrono::high_resolution_timer t1;

    std::vector<T> out(in.size());
    T val(0);

    double e1 = t1.elapsed();
    t1.restart();

    auto res = hpx::inclusive_scan(
        policy, in.begin(), in.end(), out.begin(), opt<T>(), val);
    res.get();

    double e2 = t1.elapsed();
    t1.restart();

    HPX_TEST(std::equal(out.begin(), out.end(), ver.begin()));

    double e3 = t1.elapsed();
    std::cout << std::setprecision(4) << "\t" << e1 << " " << e2 << " " << e3
              << "\n";
}

template <typename T, typename DistPolicy, typename ExPolicy>
void inclusive_scan_algo_tests_segmented_out_with_policy_async(std::size_t size,
    DistPolicy const& in_dist_policy, DistPolicy const& out_dist_policy,
    hpx::partitioned_vector<T>& in, hpx::partitioned_vector<T>& out,
    std::vector<T> ver, ExPolicy const& policy)
{
    msg9(typeid(ExPolicy).name(), typeid(DistPolicy).name(), typeid(T).name(),
        async_segmented, size, in_dist_policy.get_num_partitions(),
        in_dist_policy.get_localities().size(),
        out_dist_policy.get_num_partitions(),
        out_dist_policy.get_localities().size());
    hpx::chrono::high_resolution_timer t1;

    T val(0);

    double e1 = t1.elapsed();
    t1.restart();

    auto res = hpx::inclusive_scan(
        policy, in.begin(), in.end(), out.begin(), opt<T>(), val);
    res.get();

    double e2 = t1.elapsed();
    t1.restart();

    verify_values(out, ver);

    double e3 = t1.elapsed();
    std::cout << std::setprecision(4) << "\t" << e1 << " " << e2 << " " << e3
              << "\n";
}

template <typename T, typename DistPolicy, typename ExPolicy>
void inclusive_scan_algo_tests_inplace_with_policy_async(std::size_t size,
    DistPolicy const& dist_policy, std::vector<T> ver, ExPolicy const& policy)
{
    msg7(typeid(ExPolicy).name(), typeid(DistPolicy).name(), typeid(T).name(),
        async_inplace, size, dist_policy.get_num_partitions(),
        dist_policy.get_localities().size());
    hpx::chrono::high_resolution_timer t1;

    hpx::partitioned_vector<T> in(size, dist_policy);
    iota_vector(in, T(1));

    T val(0);

    double e1 = t1.elapsed();
    t1.restart();

    auto res = hpx::inclusive_scan(
        policy, in.begin(), in.end(), in.begin(), opt<T>(), val);
    res.get();

    double e2 = t1.elapsed();
    t1.restart();

    verify_values(in, ver);

    double e3 = t1.elapsed();
    std::cout << std::setprecision(4) << "\t" << e1 << " " << e2 << " " << e3
              << "\n";
}

///////////////////////////////////////////////////////////////////////////////

template <typename T, typename DistPolicy>
void inclusive_scan_tests_with_policy(
    std::size_t size, DistPolicy const& policy)
{
    using namespace hpx::execution;

    // setup partitioned vector to test
    hpx::partitioned_vector<T> in(size, policy);
    iota_vector(in, T(1));

    std::vector<T> ver(in.size());
    std::iota(ver.begin(), ver.end(), T(1));
    T val(0);

    hpx::parallel::detail::sequential_inclusive_scan(
        ver.begin(), ver.end(), ver.begin(), val, opt<T>());

    //sync
    inclusive_scan_algo_tests_with_policy<T>(size, policy, in, ver, seq);
    inclusive_scan_algo_tests_with_policy<T>(size, policy, in, ver, par);

    //async
    inclusive_scan_algo_tests_with_policy_async<T>(
        size, policy, in, ver, seq(task));
    inclusive_scan_algo_tests_with_policy_async<T>(
        size, policy, in, ver, par(task));
}

template <typename T, typename DistPolicy>
void inclusive_scan_tests_segmented_out_with_policy(
    std::size_t size, DistPolicy const& in_policy, DistPolicy const& out_policy)
{
    using namespace hpx::execution;

    // setup partitioned vector to test
    hpx::partitioned_vector<T> in(size, in_policy);
    iota_vector(in, T(1));

    hpx::partitioned_vector<T> out(size, out_policy);

    std::vector<T> ver(in.size());
    std::iota(ver.begin(), ver.end(), T(1));
    T val(0);

    hpx::parallel::detail::sequential_inclusive_scan(
        ver.begin(), ver.end(), ver.begin(), val, opt<T>());

    //sync
    inclusive_scan_algo_tests_segmented_out_with_policy<T>(
        size, in_policy, out_policy, in, out, ver, seq);
    inclusive_scan_algo_tests_segmented_out_with_policy<T>(
        size, in_policy, out_policy, in, out, ver, par);

    //async
    inclusive_scan_algo_tests_segmented_out_with_policy_async<T>(
        size, in_policy, out_policy, in, out, ver, seq(task));
    inclusive_scan_algo_tests_segmented_out_with_policy_async<T>(
        size, in_policy, out_policy, in, out, ver, par(task));
}

template <typename T, typename DistPolicy>
void inclusive_scan_tests_inplace_with_policy(
    std::size_t size, DistPolicy const& policy)
{
    using namespace hpx::execution;

    // setup verification vector
    std::vector<T> ver(size);
    std::iota(ver.begin(), ver.end(), T(1));
    T val(0);

    hpx::parallel::detail::sequential_inclusive_scan(
        ver.begin(), ver.end(), ver.begin(), val, opt<T>());

    // sync
    inclusive_scan_algo_tests_inplace_with_policy<T>(size, policy, ver, seq);
    inclusive_scan_algo_tests_inplace_with_policy<T>(size, policy, ver, par);

    // async
    inclusive_scan_algo_tests_inplace_with_policy_async<T>(
        size, policy, ver, seq(task));
    inclusive_scan_algo_tests_inplace_with_policy_async<T>(
        size, policy, ver, par(task));
}

///////////////////////////////////////////////////////////////////////////////

template <typename T>
void inclusive_scan_tests(std::vector<hpx::id_type>& localities)
{
#if defined(HPX_DEBUG)
    constexpr std::size_t length = 1000;
#else
    constexpr std::size_t length = 10000;
#endif

    inclusive_scan_tests_with_policy<T>(length, hpx::container_layout);
    inclusive_scan_tests_with_policy<T>(length, hpx::container_layout(3));
    inclusive_scan_tests_with_policy<T>(
        length, hpx::container_layout(3, localities));
    inclusive_scan_tests_with_policy<T>(
        length, hpx::container_layout(localities));

    inclusive_scan_tests_with_policy<T>(1000, hpx::container_layout(100));

    // multiple localities needed for the following tests
    inclusive_scan_tests_segmented_out_with_policy<T>(length,
        hpx::container_layout(localities), hpx::container_layout(localities));

    inclusive_scan_tests_segmented_out_with_policy<T>(
        length, hpx::container_layout(localities), hpx::container_layout(3));

    inclusive_scan_tests_segmented_out_with_policy<T>(
        length, hpx::container_layout(localities), hpx::container_layout(10));

    inclusive_scan_tests_inplace_with_policy<T>(
        length, hpx::container_layout(localities));

    // subrange regression test in a single segment (sit == send path)
    {
        constexpr std::size_t n = 16;
        constexpr std::size_t first_offset = 3;
        constexpr std::size_t range_size = 8;
        constexpr std::size_t dest_offset = 2;

        hpx::partitioned_vector<T> in(n, hpx::container_layout(1));
        iota_vector(in, T(0));

        hpx::partitioned_vector<T> out(n, hpx::container_layout(1));
        auto first = in.begin();
        std::advance(first, first_offset);
        auto last = first;
        std::advance(last, range_size);
        auto dest = out.begin();
        std::advance(dest, dest_offset);

        auto verify_subrange_result = [&]() {
            std::vector<T> expected(n, T(-1));
            T acc = T(0);
            for (std::size_t i = 0; i < range_size; ++i)
            {
                acc = acc + T(first_offset + i);
                expected[dest_offset + i] = acc;
            }

            std::size_t idx = 0;
            for (auto it = out.begin(); it != out.end(); ++it, ++idx)
            {
                HPX_TEST_EQ(*it, expected[idx]);
            }
            HPX_TEST_EQ(idx, n);
        };

        hpx::fill(hpx::execution::seq, out.begin(), out.end(), T(-1));
        hpx::inclusive_scan(
            hpx::execution::seq, first, last, dest, opt<T>(), T(0));
        verify_subrange_result();

        hpx::fill(hpx::execution::seq, out.begin(), out.end(), T(-1));
        hpx::inclusive_scan(
            hpx::execution::par, first, last, dest, opt<T>(), T(0));
        verify_subrange_result();

        hpx::fill(hpx::execution::seq, out.begin(), out.end(), T(-1));
        hpx::inclusive_scan(hpx::execution::seq(hpx::execution::task), first,
            last, dest, opt<T>(), T(0))
            .get();
        verify_subrange_result();

        hpx::fill(hpx::execution::seq, out.begin(), out.end(), T(-1));
        hpx::inclusive_scan(hpx::execution::par(hpx::execution::task), first,
            last, dest, opt<T>(), T(0))
            .get();
        verify_subrange_result();
    }

    // minimal regression for carry propagation order mismatch across segments
    {
        using S = std::string;

        constexpr std::size_t n = 3;
        hpx::partitioned_vector<S> in(n, hpx::container_layout(2, localities));
        hpx::partitioned_vector<S> out_seq(
            n, S(""), hpx::container_layout(2, localities));
        hpx::partitioned_vector<S> out_par(
            n, S(""), hpx::container_layout(2, localities));

        std::vector<S> vals = {"a", "b", "c"};
        auto it = in.begin();
        for (auto const& s : vals)
        {
            *it++ = s;
        }

        concat_op op;
        S init = "X";

        hpx::inclusive_scan(hpx::execution::seq, in.begin(), in.end(),
            out_seq.begin(), op, init);
        hpx::inclusive_scan(hpx::execution::par, in.begin(), in.end(),
            out_par.begin(), op, init);

        auto seq_it = out_seq.begin();
        auto par_it = out_par.begin();
        for (std::size_t i = 0; i < n; ++i, ++seq_it, ++par_it)
        {
            S seq_val = *seq_it;
            S par_val = *par_it;
            HPX_TEST_EQ(seq_val, par_val);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    std::vector<hpx::id_type> localities = hpx::find_all_localities();
    inclusive_scan_tests<long long>(localities);
    return hpx::util::report_errors();
}

#endif
