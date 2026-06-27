//  Copyright (c) 2016 Minh-Khanh Do
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
// The vector types to be used are defined in partitioned_vector module.
// HPX_REGISTER_PARTITIONED_VECTOR(double)
// HPX_REGISTER_PARTITIONED_VECTOR(int)

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

template <typename T>
struct sub_op
{
    T operator()(T v1, T v2) const
    {
        return v1 - v2;
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
void exclusive_scan_algo_tests_with_policy(std::size_t size,
    DistPolicy const& dist_policy, hpx::partitioned_vector<T>& in,
    std::vector<T> const& ver, ExPolicy const& policy)
{
    msg7(typeid(ExPolicy).name(), typeid(DistPolicy).name(), typeid(T).name(),
        regular, size, dist_policy.get_num_partitions(),
        dist_policy.get_localities().size());
    hpx::chrono::high_resolution_timer t1;

    std::vector<T> out(in.size());
    T val(0);

    double e1 = t1.elapsed();
    t1.restart();

    hpx::exclusive_scan(
        policy, in.begin(), in.end(), out.begin(), val, opt<T>());

    double e2 = t1.elapsed();
    t1.restart();

    HPX_TEST(std::equal(out.begin(), out.end(), ver.begin()));

    double e3 = t1.elapsed();
    std::cout << std::setprecision(4) << "\t" << e1 << " " << e2 << " " << e3
              << "\n";
}

template <typename T, typename DistPolicy>
void exclusive_scan_algo_tests_segmented_out_with_policy_seq(std::size_t size,
    DistPolicy const& in_dist_policy, DistPolicy const& out_dist_policy,
    hpx::partitioned_vector<T>& in, hpx::partitioned_vector<T> out,
    std::vector<T> const& ver)
{
    msg9(typeid(hpx::execution::seq).name(), typeid(DistPolicy).name(),
        typeid(T).name(), segmented, size, in_dist_policy.get_num_partitions(),
        in_dist_policy.get_localities().size(),
        out_dist_policy.get_num_partitions(),
        out_dist_policy.get_localities().size());
    hpx::chrono::high_resolution_timer t1;

    T val(0);

    double e1 = t1.elapsed();
    t1.restart();

    hpx::exclusive_scan(in.begin(), in.end(), out.begin(), val, opt<T>());

    double e2 = t1.elapsed();
    t1.restart();

    verify_values(out, ver);

    double e3 = t1.elapsed();
    std::cout << std::setprecision(4) << "\t" << e1 << " " << e2 << " " << e3
              << "\n";
}

template <typename T, typename DistPolicy, typename ExPolicy>
void exclusive_scan_algo_tests_segmented_out_with_policy(std::size_t size,
    DistPolicy const& in_dist_policy, DistPolicy const& out_dist_policy,
    hpx::partitioned_vector<T>& in, hpx::partitioned_vector<T> out,
    std::vector<T> const& ver, ExPolicy const& policy)
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

    hpx::exclusive_scan(
        policy, in.begin(), in.end(), out.begin(), val, opt<T>());

    double e2 = t1.elapsed();
    t1.restart();

    verify_values(out, ver);

    double e3 = t1.elapsed();
    std::cout << std::setprecision(4) << "\t" << e1 << " " << e2 << " " << e3
              << "\n";
}

template <typename T, typename DistPolicy, typename ExPolicy>
void exclusive_scan_algo_tests_inplace_with_policy(std::size_t size,
    DistPolicy const& dist_policy, std::vector<T> const& ver,
    ExPolicy const& policy)
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

    hpx::exclusive_scan(
        policy, in.begin(), in.end(), in.begin(), val, opt<T>());

    double e2 = t1.elapsed();
    t1.restart();

    verify_values(in, ver);

    double e3 = t1.elapsed();
    std::cout << std::setprecision(4) << "\t" << e1 << " " << e2 << " " << e3
              << "\n";
}

///////////////////////////////////////////////////////////////////////////////

template <typename T, typename DistPolicy, typename ExPolicy>
void exclusive_scan_algo_tests_with_policy_async(std::size_t size,
    DistPolicy const& dist_policy, hpx::partitioned_vector<T>& in,
    std::vector<T> const& ver, ExPolicy const& policy)
{
    msg7(typeid(ExPolicy).name(), typeid(DistPolicy).name(), typeid(T).name(),
        async, size, dist_policy.get_num_partitions(),
        dist_policy.get_localities().size());
    hpx::chrono::high_resolution_timer t1;

    std::vector<T> out(in.size());
    T val(0);

    double e1 = t1.elapsed();
    t1.restart();

    auto res = hpx::exclusive_scan(
        policy, in.begin(), in.end(), out.begin(), val, opt<T>());
    res.get();

    double e2 = t1.elapsed();
    t1.restart();

    HPX_TEST(std::equal(out.begin(), out.end(), ver.begin()));

    double e3 = t1.elapsed();
    std::cout << std::setprecision(4) << "\t" << e1 << " " << e2 << " " << e3
              << "\n";
}

template <typename T, typename DistPolicy, typename ExPolicy>
void exclusive_scan_algo_tests_segmented_out_with_policy_async(std::size_t size,
    DistPolicy const& in_dist_policy, DistPolicy const& out_dist_policy,
    hpx::partitioned_vector<T>& in, hpx::partitioned_vector<T> out,
    std::vector<T> const& ver, ExPolicy const& policy)
{
    msg9(typeid(ExPolicy).name(), typeid(DistPolicy).name(), typeid(T).name(),
        async_segmented, size, in_dist_policy.get_num_partitions(),
        in_dist_policy.get_localities().size(),
        out_dist_policy.get_num_partitions(),
        out_dist_policy.get_localities().size());
    hpx::chrono::high_resolution_timer t1;

    t1.restart();
    T val(0);

    double e1 = t1.elapsed();
    t1.restart();

    auto res = hpx::exclusive_scan(
        policy, in.begin(), in.end(), out.begin(), val, opt<T>());
    res.get();

    double e2 = t1.elapsed();
    t1.restart();

    verify_values(out, ver);

    double e3 = t1.elapsed();
    std::cout << std::setprecision(4) << "\t" << e1 << " " << e2 << " " << e3
              << "\n";
}

template <typename T, typename DistPolicy, typename ExPolicy>
void exclusive_scan_algo_tests_inplace_with_policy_async(std::size_t size,
    DistPolicy const& dist_policy, std::vector<T> const& ver,
    ExPolicy const& policy)
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

    auto res = hpx::exclusive_scan(
        policy, in.begin(), in.end(), in.begin(), val, opt<T>());
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
void exclusive_scan_tests_with_policy(
    std::size_t size, DistPolicy const& policy)
{
    using namespace hpx::execution;

    // setup partitioned vector to test
    hpx::partitioned_vector<T> in(size, policy);
    iota_vector(in, T(1));

    std::vector<T> ver(in.size());
    std::iota(ver.begin(), ver.end(), T(1));
    T val(0);

    hpx::parallel::detail::sequential_exclusive_scan(
        ver.begin(), ver.end(), ver.begin(), val, opt<T>());

    //sync
    exclusive_scan_algo_tests_with_policy<T>(size, policy, in, ver, seq);
    exclusive_scan_algo_tests_with_policy<T>(size, policy, in, ver, par);

    //async
    exclusive_scan_algo_tests_with_policy_async<T>(
        size, policy, in, ver, seq(task));
    exclusive_scan_algo_tests_with_policy_async<T>(
        size, policy, in, ver, par(task));
}

template <typename T, typename DistPolicy>
void exclusive_scan_tests_segmented_out_with_policy(
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

    hpx::parallel::detail::sequential_exclusive_scan(
        ver.begin(), ver.end(), ver.begin(), val, opt<T>());

    exclusive_scan_algo_tests_segmented_out_with_policy_seq<T>(
        size, in_policy, out_policy, in, out, ver);

    //sync
    exclusive_scan_algo_tests_segmented_out_with_policy<T>(
        size, in_policy, out_policy, in, out, ver, seq);
    exclusive_scan_algo_tests_segmented_out_with_policy<T>(
        size, in_policy, out_policy, in, out, ver, par);

    //async
    exclusive_scan_algo_tests_segmented_out_with_policy_async<T>(
        size, in_policy, out_policy, in, out, ver, seq(task));
    exclusive_scan_algo_tests_segmented_out_with_policy_async<T>(
        size, in_policy, out_policy, in, out, ver, par(task));
}

template <typename T, typename DistPolicy>
void exclusive_scan_tests_inplace_with_policy(
    std::size_t size, DistPolicy const& policy)
{
    using namespace hpx::execution;

    // setup verification vector
    std::vector<T> ver(size);
    std::iota(ver.begin(), ver.end(), T(1));
    T val(0);

    hpx::parallel::detail::sequential_exclusive_scan(
        ver.begin(), ver.end(), ver.begin(), val, opt<T>());

    // sync
    exclusive_scan_algo_tests_inplace_with_policy<T>(size, policy, ver, seq);
    exclusive_scan_algo_tests_inplace_with_policy<T>(size, policy, ver, par);

    // async
    exclusive_scan_algo_tests_inplace_with_policy_async<T>(
        size, policy, ver, seq(task));
    exclusive_scan_algo_tests_inplace_with_policy_async<T>(
        size, policy, ver, par(task));
}

///////////////////////////////////////////////////////////////////////////////

template <typename T>
void exclusive_scan_tests(std::vector<hpx::id_type>& localities)
{
#if defined(HPX_DEBUG)
    std::size_t const length = 1000;
#else
    std::size_t const length = 10000;
#endif

    exclusive_scan_tests_with_policy<T>(length, hpx::container_layout);
    exclusive_scan_tests_with_policy<T>(length, hpx::container_layout(3));
    exclusive_scan_tests_with_policy<T>(
        length, hpx::container_layout(3, localities));
    exclusive_scan_tests_with_policy<T>(
        length, hpx::container_layout(localities));

    exclusive_scan_tests_with_policy<T>(1000, hpx::container_layout(100));

    // multiple localities needed for the following tests
    exclusive_scan_tests_segmented_out_with_policy<T>(length,
        hpx::container_layout(localities), hpx::container_layout(localities));

    exclusive_scan_tests_segmented_out_with_policy<T>(
        length, hpx::container_layout(localities), hpx::container_layout(3));

    exclusive_scan_tests_segmented_out_with_policy<T>(
        length, hpx::container_layout(localities), hpx::container_layout(10));

    exclusive_scan_tests_inplace_with_policy<T>(
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
                expected[dest_offset + i] = acc;
                acc = acc + T(first_offset + i);
            }

            std::size_t idx = 0;
            for (auto it = out.begin(); it != out.end(); ++it, ++idx)
            {
                HPX_TEST_EQ(*it, expected[idx]);
            }
            HPX_TEST_EQ(idx, n);
        };

        hpx::fill(hpx::execution::seq, out.begin(), out.end(), T(-1));
        hpx::exclusive_scan(
            hpx::execution::seq, first, last, dest, T(0), opt<T>());
        verify_subrange_result();

        hpx::fill(hpx::execution::seq, out.begin(), out.end(), T(-1));
        hpx::exclusive_scan(
            hpx::execution::par, first, last, dest, T(0), opt<T>());
        verify_subrange_result();

        hpx::fill(hpx::execution::seq, out.begin(), out.end(), T(-1));
        hpx::exclusive_scan(hpx::execution::seq(hpx::execution::task), first,
            last, dest, T(0), opt<T>())
            .get();
        verify_subrange_result();

        hpx::fill(hpx::execution::seq, out.begin(), out.end(), T(-1));
        hpx::exclusive_scan(hpx::execution::par(hpx::execution::task), first,
            last, dest, T(0), opt<T>())
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

        hpx::exclusive_scan(hpx::execution::seq, in.begin(), in.end(),
            out_seq.begin(), init, op);
        hpx::exclusive_scan(hpx::execution::par, in.begin(), in.end(),
            out_par.begin(), init, op);

        auto seq_it = out_seq.begin();
        auto par_it = out_par.begin();
        for (std::size_t i = 0; i < n; ++i, ++seq_it, ++par_it)
        {
            S seq_val = *seq_it;
            S par_val = *par_it;
            HPX_TEST_EQ(seq_val, par_val);
        }
    }

    // regression: same-segment subrange must not fall back to non-segmented
    // scan path when output has more remaining room than input range
    {
        constexpr std::size_t n = 10;
        constexpr std::size_t first_offset = 2;
        constexpr std::size_t range_size = 4;
        constexpr std::size_t dest_offset = 3;

        hpx::partitioned_vector<T> in(n, hpx::container_layout(1));
        iota_vector(in, T(1));

        hpx::partitioned_vector<T> out(n, hpx::container_layout(1));
        hpx::fill(hpx::execution::seq, out.begin(), out.end(), T(-1));

        auto first = in.begin();
        std::advance(first, first_offset);
        auto last = first;
        std::advance(last, range_size);
        auto dest = out.begin();
        std::advance(dest, dest_offset);

        T const init = T(100);
        hpx::exclusive_scan(
            hpx::execution::seq, first, last, dest, init, sub_op<T>{});

        std::vector<T> expected(n, T(-1));
        T acc = init;
        for (std::size_t i = 0; i < range_size; ++i)
        {
            expected[dest_offset + i] = acc;
            acc = acc - T(first_offset + i + 1);
        }

        std::size_t idx = 0;
        for (auto it = out.begin(); it != out.end(); ++it, ++idx)
        {
            HPX_TEST_EQ(*it, expected[idx]);
        }
        HPX_TEST_EQ(idx, n);
    }
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    std::vector<hpx::id_type> localities = hpx::find_all_localities();
    exclusive_scan_tests<long long>(localities);

    return hpx::util::report_errors();
}
#endif
