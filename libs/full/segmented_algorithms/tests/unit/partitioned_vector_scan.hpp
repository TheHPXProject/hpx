//  Copyright (c) 2016 Minh-Khanh Do
//  Copyright (c) 2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/algorithm.hpp>
#include <hpx/include/partitioned_vector.hpp>

#include <type_traits>
#include <vector>

struct iota : public hpx::parallel::detail::algorithm<iota>
{
    iota()
      : iota::algorithm("iota")
    {
    }

    template <typename ExPolicy, typename InIter, typename T>
    static hpx::util::unused_type sequential(
        ExPolicy&&, InIter first, InIter last, T&& init)
    {
        std::iota(first, last, init);
        return hpx::util::unused;
    }

    template <typename ExPolicy, typename InIter, typename T>
    static hpx::util::unused_type parallel(
        ExPolicy&& policy, InIter first, InIter last, T&& init)
    {
        return hpx::util::void_guard<result_type>(),
               sequential(policy, first, last, init);
    }
};

template <typename T>
void iota_vector(hpx::partitioned_vector<T>& v, T val)
{
    auto first = v.begin();
    auto last = v.end();

    typedef hpx::traits::segmented_iterator_traits<decltype(first)> traits;
    typedef typename traits::segment_iterator segment_iterator;
    typedef typename traits::local_iterator local_iterator_type;

    segment_iterator sit = traits::segment(first);
    segment_iterator send = traits::segment(last);

    T temp_val = val;

    for (; sit != send; ++sit)
    {
        local_iterator_type beg = traits::begin(sit);
        local_iterator_type end = traits::end(sit);

        hpx::parallel::detail::dispatch(traits::get_id(sit), iota(),
            hpx::execution::seq, std::true_type(), beg, end, temp_val);

        temp_val = T(temp_val + std::distance(beg, end));
    }
}

template <typename Value>
struct verify_ : public hpx::parallel::detail::algorithm<verify_<Value>, Value>
{
    verify_()
      : verify_::algorithm("verify")
    {
    }

    template <typename ExPolicy, typename SegIter, typename InIter>
    static Value sequential(ExPolicy&&, SegIter first, SegIter last, InIter in)
    {
        return std::equal(first, last, in.begin());
    }

    template <typename ExPolicy, typename SegIter, typename InIter>
    static Value parallel(
        ExPolicy&& policy, InIter first, InIter last, InIter in)
    {
        return sequential(policy, first, last, in);
    }
};

template <typename T>
void verify_values(
    hpx::partitioned_vector<T> const& v1, std::vector<T> const& v2)
{
    auto first = v1.begin();
    auto last = v1.end();

    typedef hpx::traits::segmented_iterator_traits<decltype(first)> traits;
    typedef typename traits::segment_iterator segment_iterator;
    typedef typename traits::local_iterator local_iterator_type;

    segment_iterator sit = traits::segment(first);
    segment_iterator send = traits::segment(last);

    auto beg2 = v2.begin();

    std::vector<bool> results;

    for (; sit != send; ++sit)
    {
        local_iterator_type beg = traits::begin(sit);
        local_iterator_type end = traits::end(sit);

        std::vector<T> test(std::distance(beg, end));
        std::copy_n(beg2, test.size(), test.begin());

        results.push_back(hpx::parallel::detail::dispatch(traits::get_id(sit),
            verify_<bool>(), hpx::execution::seq, std::true_type(), beg, end,
            test));

        beg2 += std::distance(beg, end);
    }
    bool final_result =
        std::all_of(results.begin(), results.end(), [](bool v) { return v; });

    HPX_TEST(final_result);
}

#endif
