//  Copyright (c) 2014-2017 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_main.hpp>
#include <hpx/include/parallel_generate.hpp>
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

template <typename T>
struct gen
{
    T operator()() const
    {
        return T(42);
    }
};

template <typename T>
struct gen1
{
    T operator()() const
    {
        return T(43);
    }
};

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
void verify_vector(hpx::partitioned_vector<T> const& v, T const& val,
    bool /* must_be_equal */ = true)
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
template <typename T>
void verify_against_std(
    hpx::partitioned_vector<T>& part_v, std::vector<T>& std_v)
{
    auto std_it = std_v.begin();
    HPX_ASSERT(part_v.size() == std_v.size());
    for (auto part_it = part_v.begin(); part_it != part_v.end();
        part_it++, std_it++)
    {
        HPX_TEST_EQ(*part_it, *std_it);
    }
}
///////////////////////////////////////////////////////////////////////////////
template <typename T, typename DistPolicy, typename ExPolicy>
void generate_algo_tests_with_policy(
    std::size_t size, DistPolicy const& policy, ExPolicy const& generate_policy)
{
    hpx::partitioned_vector<T> c(size, policy);
    iota_vector(c, T(1234));

    hpx::generate(generate_policy, c.begin(), c.end(), gen<T>{});
    verify_vector(c, T(42));

    hpx::generate(generate_policy, c.begin() + 1, c.end() - 1, gen1<T>{});
    verify_values(c.begin() + 1, c.end() - 1, T(43));
    verify_values(c.begin(), c.begin() + 1, T(43), false);
    verify_values(c.end() - 1, c.end(), T(43), false);
}

template <typename T, typename DistPolicy, typename Count, typename ExPolicy>
void generate_n_algo_tests_with_policy(std::size_t size, Count count,
    DistPolicy const& policy, ExPolicy const& generate_policy)
{
    hpx::partitioned_vector<T> c(size, policy);
    std::vector<T> l(size);
    iota_vector(c, T(1234));
    std::iota(l.begin(), l.end(), T(1234));

    hpx::generate_n(generate_policy, c.begin(), count, gen<T>{});
    std::generate_n(l.begin(), count, gen<T>{});
    verify_against_std(c, l);
}

template <typename T, typename DistPolicy, typename ExPolicy>
void generate_algo_tests_with_policy_async(
    std::size_t size, DistPolicy const& policy, ExPolicy const& generate_policy)
{
    hpx::partitioned_vector<T> c(size, policy);
    iota_vector(c, T(1234));

    hpx::future<void> f =
        hpx::generate(generate_policy, c.begin(), c.end(), gen<T>{});
    f.wait();

    verify_vector(c, T(42));

    hpx::future<void> f1 =
        hpx::generate(generate_policy, c.begin() + 1, c.end() - 1, gen1<T>{});
    f1.wait();

    verify_values(c.begin() + 1, c.end() - 1, T(43));
    verify_values(c.begin(), c.begin() + 1, T(43), false);
    verify_values(c.end() - 1, c.end(), T(43), false);
}
template <typename T, typename DistPolicy, typename Count, typename ExPolicy>
void generate_n_algo_tests_with_policy_async(std::size_t size, Count count,
    DistPolicy const& policy, ExPolicy const& generate_policy)
{
    hpx::partitioned_vector<T> c(size, policy);
    std::vector<T> l(size);
    iota_vector(c, T(1234));
    std::iota(l.begin(), l.end(), T(1234));

    hpx::future<void> f =
        hpx::generate_n(generate_policy, c.begin(), count, gen<T>{});
    f.wait();
    std::generate_n(l.begin(), count, gen<T>{});
    verify_against_std(c, l);
}

template <typename T, typename DistPolicy>
void generate_tests_with_policy(
    std::size_t size, std::size_t /* localities */, DistPolicy const& policy)
{
    using namespace hpx::execution;

    generate_algo_tests_with_policy<T>(size, policy, seq);
    generate_algo_tests_with_policy<T>(size, policy, par);

    //async
    generate_algo_tests_with_policy_async<T>(size, policy, seq(task));
    generate_algo_tests_with_policy_async<T>(size, policy, par(task));
}

template <typename T, typename DistPolicy>
void generate_n_tests_with_policy(std::size_t size, std::size_t count_range,
    std::size_t /* localities */, DistPolicy const& policy)
{
    using namespace hpx::execution;
    for (std::size_t count = 0; count < count_range; count++)
    {
        generate_n_algo_tests_with_policy<T>(size, count, policy, seq);
        generate_n_algo_tests_with_policy<T>(size, count, policy, par);

        //async
        generate_n_algo_tests_with_policy_async<T>(
            size, count, policy, seq(task));
        generate_n_algo_tests_with_policy_async<T>(
            size, count, policy, par(task));
    }
}

template <typename T>
void generate_tests()
{
    std::size_t const length = 12;
    std::vector<hpx::id_type> localities = hpx::find_all_localities();

    generate_tests_with_policy<T>(length, 1, hpx::container_layout);
    generate_tests_with_policy<T>(length, 3, hpx::container_layout(3));
    generate_tests_with_policy<T>(
        length, 3, hpx::container_layout(3, localities));
    generate_tests_with_policy<T>(
        length, localities.size(), hpx::container_layout(localities));
}

template <typename T>
void generate_n_tests()
{
    std::size_t const length = 12;
    std::vector<hpx::id_type> localities = hpx::find_all_localities();

    generate_n_tests_with_policy<T>(length, 8, 1, hpx::container_layout);
    generate_n_tests_with_policy<T>(length, 8, 3, hpx::container_layout(3));
    generate_n_tests_with_policy<T>(
        length, 8, 3, hpx::container_layout(3, localities));
    generate_n_tests_with_policy<T>(
        length, 8, localities.size(), hpx::container_layout(localities));
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    generate_tests<double>();
    generate_tests<int>();
    generate_n_tests<double>();
    generate_n_tests<int>();

    return hpx::util::report_errors();
}
#endif
