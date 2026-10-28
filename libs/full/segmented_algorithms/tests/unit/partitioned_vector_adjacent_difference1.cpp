//  Copyright (c) 2017 Ajai V George
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_main.hpp>
#include <hpx/include/parallel_adjacent_difference.hpp>
#include <hpx/include/parallel_count.hpp>
#include <hpx/include/parallel_scan.hpp>
#include <hpx/include/partitioned_vector_predef.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <functional>
#include <iterator>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy, typename T>
void verify_values(
    ExPolicy&&, hpx::partitioned_vector<T> const& v, T const& val)
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

template <typename ExPolicy, typename T>
void verify_values(ExPolicy&&, hpx::partitioned_vector<T> const& v)
{
    typedef typename hpx::partitioned_vector<T>::const_iterator const_iterator;
    std::size_t size = 0;
    T val = 1;
    const_iterator end = v.end();
    for (const_iterator it = v.begin(); it != end; ++it, ++size)
    {
        HPX_TEST_EQ(*it, val++);
    }
    HPX_TEST_EQ(size, v.size());
}

template <typename ExPolicy, typename T>
void test_adjacent_difference(ExPolicy&& policy, hpx::partitioned_vector<T>& v,
    hpx::partitioned_vector<T>& w, T val)
{
    hpx::adjacent_difference(policy, v.begin(), v.end(), w.begin());

    verify_values(policy, w, val);
    verify_values(policy, v);
}

template <typename ExPolicy, typename T>
void test_adjacent_difference_async(ExPolicy&& policy,
    hpx::partitioned_vector<T>& v, hpx::partitioned_vector<T>& w, T val)
{
    hpx::adjacent_difference(policy, v.begin(), v.end(), w.begin()).get();

    verify_values(policy, w, val);
    verify_values(policy, v);
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
void adjacent_difference_tests(std::vector<hpx::id_type>& localities)
{
    {
        hpx::partitioned_vector<T> in_place(
            8, T(0), hpx::container_layout(2));
        auto it = in_place.begin();
        for (T value = T(10); it != in_place.end(); ++it, ++value)
        {
            *it = value;
        }

        hpx::adjacent_difference(in_place.begin(), in_place.end(),
            in_place.begin(), std::minus<T>{});

        std::vector<T> const expected{T(10), T(1), T(1), T(1),
            T(1), T(1), T(1), T(1)};
        auto verify = [&]() {
            auto result = in_place.begin();
            for (T value : expected)
            {
                HPX_TEST_EQ(*result, value);
                ++result;
            }
        };

        verify();

        auto reset = [&]() {
            auto current = in_place.begin();
            for (T value = T(10); current != in_place.end(); ++current, ++value)
            {
                *current = value;
            }
        };

        reset();
        hpx::adjacent_difference(hpx::execution::par, in_place.begin(),
            in_place.end(), in_place.begin(), std::minus<T>{});
        verify();

        reset();
        hpx::adjacent_difference(hpx::execution::par(hpx::execution::task),
            in_place.begin(), in_place.end(), in_place.begin(), std::minus<T>{})
            .get();
        verify();
    }

    {
        hpx::partitioned_vector<T> empty(0, hpx::container_layout(1));
        auto const dest = empty.begin();
        HPX_TEST(hpx::adjacent_difference(
                     hpx::execution::seq, empty.begin(), empty.end(), dest) ==
            dest);
        HPX_TEST(hpx::adjacent_difference(hpx::execution::seq(
                                                 hpx::execution::task),
                     empty.begin(), empty.end(), dest)
                     .get() == dest);
        HPX_TEST(hpx::adjacent_difference(
                     hpx::execution::par, empty.begin(), empty.end(), dest) ==
            dest);
        HPX_TEST(hpx::adjacent_difference(hpx::execution::par(
                                                 hpx::execution::task),
                     empty.begin(), empty.end(), dest)
                     .get() == dest);
    }

    {
        constexpr std::size_t length = 12;
        std::vector<std::size_t> const src_sizes{3, 0, 4, 0, 5};
        std::vector<std::size_t> const out_sizes{5, 0, 7};
        std::vector<hpx::id_type> src_localities;
        std::vector<hpx::id_type> out_localities;
        src_localities.reserve(src_sizes.size());
        out_localities.reserve(out_sizes.size());
        for (std::size_t i = 0; i != src_sizes.size(); ++i)
        {
            src_localities.push_back(localities[i % localities.size()]);
        }
        for (std::size_t i = 0; i != out_sizes.size(); ++i)
        {
            out_localities.push_back(
                localities[(i + 1) % localities.size()]);
        }
        hpx::partitioned_vector<T> src(
            length, hpx::explicit_container_layout(src_sizes, src_localities));
        hpx::partitioned_vector<T> out(
            length, T(-1),
            hpx::explicit_container_layout(out_sizes, out_localities));

        auto it = src.begin();
        for (T value = T(0); it != src.end(); ++it, value = T(value + 1))
        {
            *it = value;
        }

        auto verify = [&]() {
            auto result = out.begin();
            HPX_TEST_EQ(*result, T(0));
            ++result;
            for (; result != out.end(); ++result)
            {
                HPX_TEST_EQ(*result, T(1));
            }
        };

        hpx::adjacent_difference(hpx::execution::seq, src.begin(), src.end(),
            out.begin());
        verify();

        hpx::fill(hpx::execution::seq, out.begin(), out.end(), T(-1));
        hpx::adjacent_difference(hpx::execution::seq(hpx::execution::task),
            src.begin(), src.end(), out.begin())
            .get();
        verify();

        hpx::fill(hpx::execution::seq, out.begin(), out.end(), T(-1));
        hpx::adjacent_difference(hpx::execution::par, src.begin(), src.end(),
            out.begin());
        verify();

        hpx::fill(hpx::execution::seq, out.begin(), out.end(), T(-1));
        hpx::adjacent_difference(hpx::execution::par(hpx::execution::task),
            src.begin(), src.end(), out.begin())
            .get();
        verify();
    }

    std::size_t const length = 12;

    hpx::partitioned_vector<T> v(
        length, T(1), hpx::container_layout(localities));
    hpx::inclusive_scan(hpx::execution::seq, v.begin(), v.end(), v.begin());
    hpx::partitioned_vector<T> w(length, hpx::container_layout(localities));
    test_adjacent_difference(hpx::execution::seq, v, w, T(1));
    test_adjacent_difference(hpx::execution::par, v, w, T(1));
    test_adjacent_difference_async(
        hpx::execution::seq(hpx::execution::task), v, w, T(1));
    test_adjacent_difference_async(
        hpx::execution::par(hpx::execution::task), v, w, T(1));

    // subrange regression test in a single segment (sit == send path)
    {
        constexpr std::size_t n = 16;
        constexpr std::size_t first_offset = 3;
        constexpr std::size_t range_size = 8;
        constexpr std::size_t dest_offset = 2;

        hpx::partitioned_vector<T> src(n, hpx::container_layout(1));
        hpx::partitioned_vector<T> out(n, T(-1), hpx::container_layout(1));

        auto it = src.begin();
        for (T value = T(0); it != src.end(); ++it, value = T(value + 1))
        {
            *it = value;
        }

        auto first = src.begin();
        std::advance(first, first_offset);
        auto last = first;
        std::advance(last, range_size);
        auto dest = out.begin();
        std::advance(dest, dest_offset);

        auto verify_subrange_result = [&]() {
            std::vector<T> expected(n, T(-1));
            expected[dest_offset] = T(first_offset);
            for (std::size_t i = 1; i < range_size; ++i)
            {
                expected[dest_offset + i] = T(1);
            }

            std::size_t idx = 0;
            for (auto oit = out.begin(); oit != out.end(); ++oit, ++idx)
            {
                HPX_TEST_EQ(*oit, expected[idx]);
            }
            HPX_TEST_EQ(idx, n);
        };

        hpx::fill(hpx::execution::seq, out.begin(), out.end(), T(-1));
        hpx::adjacent_difference(hpx::execution::seq, first, last, dest);
        verify_subrange_result();

        hpx::fill(hpx::execution::seq, out.begin(), out.end(), T(-1));
        hpx::adjacent_difference(hpx::execution::par, first, last, dest);
        verify_subrange_result();

        hpx::fill(hpx::execution::seq, out.begin(), out.end(), T(-1));
        hpx::adjacent_difference(
            hpx::execution::seq(hpx::execution::task), first, last, dest)
            .get();
        verify_subrange_result();

        hpx::fill(hpx::execution::seq, out.begin(), out.end(), T(-1));
        hpx::adjacent_difference(
            hpx::execution::par(hpx::execution::task), first, last, dest)
            .get();
        verify_subrange_result();
    }
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    std::vector<hpx::id_type> localities = hpx::find_all_localities();
    adjacent_difference_tests<int>(localities);
    return hpx::util::report_errors();
}
#endif
