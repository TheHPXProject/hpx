//  Copyright (c) 2026 Mo'men Samir
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#define HPX_HAVE_FORCE_NO_CXX_MODULES

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_main.hpp>
#include <hpx/include/parallel_is_partitioned.hpp>

#include <hpx/include/partitioned_vector_predef.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <iterator>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
#define SIZE 1007

namespace {
    template <typename T>
    struct is_even
    {
        bool operator()(T const& val) const
        {
            return val % T(2) == 0;
        }
    };

    ///////////////////////////////////////////////////////////////////////////////
    // Initialization helpers

    template <typename T>
    void initialize_partitioned(hpx::partitioned_vector<T>& v)
    {
        typename hpx::partitioned_vector<T>::iterator it = v.begin();
        std::size_t const mid = v.size() / 2;
        for (std::size_t i = 0; i < v.size(); ++i, ++it)
            *it = (i < mid) ? T(2) : T(1);
    }

    template <typename T>
    void initialize_all_odd(hpx::partitioned_vector<T>& v)
    {
        typename hpx::partitioned_vector<T>::iterator it = v.begin();
        for (std::size_t i = 0; i < v.size(); ++i, ++it)
            *it = T(1);
    }

    template <typename T>
    void initialize_all_even(hpx::partitioned_vector<T>& v)
    {
        typename hpx::partitioned_vector<T>::iterator it = v.begin();
        for (std::size_t i = 0; i < v.size(); ++i, ++it)
            *it = T(2);
    }

    template <typename T>
    void initialize_violation_at_begin(hpx::partitioned_vector<T>& v)
    {
        initialize_partitioned(v);
        *v.begin() = T(1);
    }

    template <typename T>
    void initialize_violation_at_end(hpx::partitioned_vector<T>& v)
    {
        initialize_partitioned(v);
        typename hpx::partitioned_vector<T>::iterator last = v.end();
        --last;
        *last = T(2);
    }

    template <typename T>
    void initialize_seg_boundary_valid(hpx::partitioned_vector<T>& v,
        std::size_t num_localities, std::size_t seg_size)
    {
        typename hpx::partitioned_vector<T>::iterator it = v.begin();
        for (std::size_t i = 0; i < num_localities * seg_size; ++i, ++it)
            *it = (i < seg_size) ? T(2) : T(1);
    }

    template <typename T>
    void initialize_seg_boundary_cross_violation(hpx::partitioned_vector<T>& v,
        std::size_t num_localities, std::size_t seg_size)
    {
        typename hpx::partitioned_vector<T>::iterator it = v.begin();
        for (std::size_t i = 0; i < num_localities * seg_size; ++i, ++it)
            *it = T(2);

        typename hpx::partitioned_vector<T>::iterator seg_end = v.begin();
        std::advance(seg_end, seg_size - 1);
        *seg_end = T(1);

        typename hpx::partitioned_vector<T>::iterator viol = v.begin();
        std::advance(viol, seg_size + 1);
        *viol = T(2);

        typename hpx::partitioned_vector<T>::iterator rest = v.begin();
        std::advance(rest, seg_size + 2);
        for (std::size_t i = seg_size + 2; i < num_localities * seg_size;
            ++i, ++rest)
            *rest = T(1);
    }

    template <typename T>
    void initialize_seg_boundary_false_then_true_in_last_seg(
        hpx::partitioned_vector<T>& v, std::size_t num_localities,
        std::size_t seg_size)
    {
        // Partition boundary falls between segment 0 (even) and the rest (odd)
        typename hpx::partitioned_vector<T>::iterator it = v.begin();
        for (std::size_t i = 0; i < num_localities * seg_size; ++i, ++it)
            *it = (i < seg_size) ? T(2) : T(1);

        // Set the last element to even to violate partition in last seg
        typename hpx::partitioned_vector<T>::iterator last = v.end();
        --last;
        *last = T(2);
    }

    ///////////////////////////////////////////////////////////////////////////////
    template <typename T>
    void test_is_partitioned(std::size_t /* num_localities */,
        hpx::partitioned_vector<T>& v_part, hpx::partitioned_vector<T>& v_odd,
        hpx::partitioned_vector<T>& v_even,
        hpx::partitioned_vector<T>& v_viol_beg,
        hpx::partitioned_vector<T>& v_viol_end,
        hpx::partitioned_vector<T>& v_seg_valid)
    {
        // Basic partitioned / uniform inputs
        HPX_TEST(
            hpx::is_partitioned(v_part.begin(), v_part.end(), is_even<T>()));
        HPX_TEST(hpx::is_partitioned(v_odd.begin(), v_odd.end(), is_even<T>()));
        HPX_TEST(
            hpx::is_partitioned(v_even.begin(), v_even.end(), is_even<T>()));

        // Violations at begin and end
        HPX_TEST(!hpx::is_partitioned(
            v_viol_beg.begin(), v_viol_beg.end(), is_even<T>()));
        HPX_TEST(!hpx::is_partitioned(
            v_viol_end.begin(), v_viol_end.end(), is_even<T>()));

        HPX_TEST(hpx::is_partitioned(
            v_seg_valid.begin(), v_seg_valid.end(), is_even<T>()));
    }

    template <typename ExPolicy, typename T>
    void test_is_partitioned(ExPolicy&& policy,
        std::size_t /* num_localities */, hpx::partitioned_vector<T>& v_part,
        hpx::partitioned_vector<T>& v_odd, hpx::partitioned_vector<T>& v_even,
        hpx::partitioned_vector<T>& v_viol_beg,
        hpx::partitioned_vector<T>& v_viol_end,
        hpx::partitioned_vector<T>& v_seg_valid)
    {
        HPX_TEST(hpx::is_partitioned(
            policy, v_part.begin(), v_part.end(), is_even<T>()));
        HPX_TEST(hpx::is_partitioned(
            policy, v_odd.begin(), v_odd.end(), is_even<T>()));
        HPX_TEST(hpx::is_partitioned(
            policy, v_even.begin(), v_even.end(), is_even<T>()));

        HPX_TEST(!hpx::is_partitioned(
            policy, v_viol_beg.begin(), v_viol_beg.end(), is_even<T>()));
        HPX_TEST(!hpx::is_partitioned(
            policy, v_viol_end.begin(), v_viol_end.end(), is_even<T>()));

        HPX_TEST(hpx::is_partitioned(
            policy, v_seg_valid.begin(), v_seg_valid.end(), is_even<T>()));
    }

    template <typename ExPolicy, typename T>
    void test_is_partitioned_async(ExPolicy&& policy,
        std::size_t /* num_localities */, hpx::partitioned_vector<T>& v_part,
        hpx::partitioned_vector<T>& v_odd, hpx::partitioned_vector<T>& v_even,
        hpx::partitioned_vector<T>& v_viol_beg,
        hpx::partitioned_vector<T>& v_viol_end,
        hpx::partitioned_vector<T>& v_seg_valid)
    {
        HPX_TEST(hpx::is_partitioned(
            policy, v_part.begin(), v_part.end(), is_even<T>())
                .get());
        HPX_TEST(hpx::is_partitioned(
            policy, v_odd.begin(), v_odd.end(), is_even<T>())
                .get());
        HPX_TEST(hpx::is_partitioned(
            policy, v_even.begin(), v_even.end(), is_even<T>())
                .get());

        HPX_TEST(!hpx::is_partitioned(
            policy, v_viol_beg.begin(), v_viol_beg.end(), is_even<T>())
                .get());
        HPX_TEST(!hpx::is_partitioned(
            policy, v_viol_end.begin(), v_viol_end.end(), is_even<T>())
                .get());

        HPX_TEST(hpx::is_partitioned(
            policy, v_seg_valid.begin(), v_seg_valid.end(), is_even<T>())
                .get());
    }

    ///////////////////////////////////////////////////////////////////////////////
    template <typename T>
    void is_partitioned_tests(std::vector<hpx::id_type>& localities)
    {
        std::size_t const num_localities = localities.size();
        std::size_t const seg_size = SIZE / num_localities;
        std::size_t const seg_total = num_localities * seg_size;

        hpx::partitioned_vector<T> v_part(
            SIZE, T(0), hpx::container_layout(localities));
        hpx::partitioned_vector<T> v_odd(
            SIZE, T(0), hpx::container_layout(localities));
        hpx::partitioned_vector<T> v_even(
            SIZE, T(0), hpx::container_layout(localities));
        hpx::partitioned_vector<T> v_viol_beg(
            SIZE, T(0), hpx::container_layout(localities));
        hpx::partitioned_vector<T> v_viol_end(
            SIZE, T(0), hpx::container_layout(localities));
        hpx::partitioned_vector<T> v_seg_valid(
            seg_total, T(0), hpx::container_layout(num_localities));

        initialize_partitioned(v_part);
        initialize_all_odd(v_odd);
        initialize_all_even(v_even);
        initialize_violation_at_begin(v_viol_beg);
        initialize_violation_at_end(v_viol_end);
        initialize_seg_boundary_valid(v_seg_valid, num_localities, seg_size);

        test_is_partitioned(num_localities, v_part, v_odd, v_even, v_viol_beg,
            v_viol_end, v_seg_valid);

        test_is_partitioned(hpx::execution::seq, num_localities, v_part, v_odd,
            v_even, v_viol_beg, v_viol_end, v_seg_valid);
        test_is_partitioned(hpx::execution::par, num_localities, v_part, v_odd,
            v_even, v_viol_beg, v_viol_end, v_seg_valid);

        test_is_partitioned_async(hpx::execution::seq(hpx::execution::task),
            num_localities, v_part, v_odd, v_even, v_viol_beg, v_viol_end,
            v_seg_valid);
        test_is_partitioned_async(hpx::execution::par(hpx::execution::task),
            num_localities, v_part, v_odd, v_even, v_viol_beg, v_viol_end,
            v_seg_valid);

        if (num_localities > 1)
        {
            hpx::partitioned_vector<T> v_seg_cross(
                seg_total, T(0), hpx::container_layout(num_localities));
            hpx::partitioned_vector<T> v_seg_last_viol(
                seg_total, T(0), hpx::container_layout(num_localities));

            initialize_seg_boundary_cross_violation(
                v_seg_cross, num_localities, seg_size);
            initialize_seg_boundary_false_then_true_in_last_seg(
                v_seg_last_viol, num_localities, seg_size);

            // no-policy
            HPX_TEST(!hpx::is_partitioned(
                v_seg_cross.begin(), v_seg_cross.end(), is_even<T>()));
            HPX_TEST(!hpx::is_partitioned(
                v_seg_last_viol.begin(), v_seg_last_viol.end(), is_even<T>()));

            // seq
            HPX_TEST(!hpx::is_partitioned(hpx::execution::seq,
                v_seg_cross.begin(), v_seg_cross.end(), is_even<T>()));
            HPX_TEST(!hpx::is_partitioned(hpx::execution::seq,
                v_seg_last_viol.begin(), v_seg_last_viol.end(), is_even<T>()));

            // par
            HPX_TEST(!hpx::is_partitioned(hpx::execution::par,
                v_seg_cross.begin(), v_seg_cross.end(), is_even<T>()));
            HPX_TEST(!hpx::is_partitioned(hpx::execution::par,
                v_seg_last_viol.begin(), v_seg_last_viol.end(), is_even<T>()));

            // seq(task)
            HPX_TEST(
                !hpx::is_partitioned(hpx::execution::seq(hpx::execution::task),
                    v_seg_cross.begin(), v_seg_cross.end(), is_even<T>())
                    .get());
            HPX_TEST(!hpx::is_partitioned(
                hpx::execution::seq(hpx::execution::task),
                v_seg_last_viol.begin(), v_seg_last_viol.end(), is_even<T>())
                    .get());

            // par(task)
            HPX_TEST(
                !hpx::is_partitioned(hpx::execution::par(hpx::execution::task),
                    v_seg_cross.begin(), v_seg_cross.end(), is_even<T>())
                    .get());
            HPX_TEST(!hpx::is_partitioned(
                hpx::execution::par(hpx::execution::task),
                v_seg_last_viol.begin(), v_seg_last_viol.end(), is_even<T>())
                    .get());
        }

        // Sub-range tests
        {
            hpx::partitioned_vector<T> v_sub(
                SIZE, T(0), hpx::container_layout(localities));
            initialize_partitioned(v_sub);

            typename hpx::partitioned_vector<T>::iterator mid = v_sub.begin();
            std::advance(mid, (v_sub.size() / 2) + 1);
            *std::prev(mid, 2) = T(1);
            *std::prev(mid) = T(2);

            // no-policy
            HPX_TEST(!hpx::is_partitioned(v_sub.begin(), mid, is_even<T>()));
            HPX_TEST(hpx::is_partitioned(
                v_sub.begin(), std::prev(mid), is_even<T>()));

            // seq
            HPX_TEST(!hpx::is_partitioned(
                hpx::execution::seq, v_sub.begin(), mid, is_even<T>()));
            HPX_TEST(hpx::is_partitioned(hpx::execution::seq, v_sub.begin(),
                std::prev(mid), is_even<T>()));

            // par
            HPX_TEST(!hpx::is_partitioned(
                hpx::execution::par, v_sub.begin(), mid, is_even<T>()));
            HPX_TEST(hpx::is_partitioned(hpx::execution::par, v_sub.begin(),
                std::prev(mid), is_even<T>()));

            // seq(task)
            HPX_TEST(
                !hpx::is_partitioned(hpx::execution::seq(hpx::execution::task),
                    v_sub.begin(), mid, is_even<T>())
                    .get());
            HPX_TEST(
                hpx::is_partitioned(hpx::execution::seq(hpx::execution::task),
                    v_sub.begin(), std::prev(mid), is_even<T>())
                    .get());

            // par(task)
            HPX_TEST(
                !hpx::is_partitioned(hpx::execution::par(hpx::execution::task),
                    v_sub.begin(), mid, is_even<T>())
                    .get());
            HPX_TEST(
                hpx::is_partitioned(hpx::execution::par(hpx::execution::task),
                    v_sub.begin(), std::prev(mid), is_even<T>())
                    .get());
        }
    }
}    // namespace

///////////////////////////////////////////////////////////////////////////////
int main()
{
    std::vector<hpx::id_type> localities = hpx::find_all_localities();
    is_partitioned_tests<int>(localities);
    return hpx::util::report_errors();
}

#endif
