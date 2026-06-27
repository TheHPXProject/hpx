//  Copyright (c) 2016 Minh-Khanh Do
//  Copyright (c) 2024-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/executors.hpp>

#include <hpx/parallel/segmented_algorithms/for_each.hpp>

#include <iterator>
#include <type_traits>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    // segmented_fill

    /// \cond NOINTERNAL

    template <typename T>
    struct fill_function
    {
        fill_function(T val = T())
          : value_(val)
        {
        }

        T value_;

        void operator()(T& val) const
        {
            val = value_;
        }

        template <typename Archive>
        void serialize(Archive& ar, unsigned /* version */)
        {
            // clang-format off
                ar & value_;
            // clang-format on
        }
    };
    /// \endcond
}    // namespace hpx::parallel::detail

namespace hpx::segmented {

    HPX_CXX_EXPORT template <typename SegIter, typename T>
        requires(hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter>)
    SegIter tag_invoke(hpx::fill_t, SegIter first, SegIter last, T const& value)
    {
        static_assert(std::forward_iterator<SegIter>,
            "Requires at least forward iterator.");

        if (first == last)
        {
            return HPX_MOVE(first);
        }

        using iterator_traits = hpx::traits::segmented_iterator_traits<SegIter>;
        using value_type = typename std::iterator_traits<SegIter>::value_type;

        return hpx::parallel::detail::segmented_for_each(
            hpx::parallel::detail::for_each<
                typename iterator_traits::local_iterator>(),
            hpx::execution::seq, first, last,
            hpx::parallel::detail::fill_function<value_type>(value),
            hpx::identity_v, std::true_type{});
    }

    HPX_CXX_EXPORT template <typename SegIter, typename Size, typename T>
        requires(hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter> &&
            std::is_integral_v<Size>)
    SegIter tag_invoke(hpx::fill_n_t, SegIter first, Size count, T const& value)
    {
        static_assert(std::forward_iterator<SegIter>,
            "Requires at least forward iterator.");

        if (hpx::parallel::detail::is_negative(count) || count == 0)
        {
            return HPX_MOVE(first);
        }

        using iterator_traits = hpx::traits::segmented_iterator_traits<SegIter>;
        using value_type = typename std::iterator_traits<SegIter>::value_type;

        SegIter last = first;
        hpx::parallel::detail::advance(last, count);
        return hpx::parallel::detail::segmented_for_each(
            hpx::parallel::detail::for_each<
                typename iterator_traits::local_iterator>(),
            hpx::execution::seq, first, last,
            hpx::parallel::detail::fill_function<value_type>(value),
            hpx::identity_v, std::true_type{});
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename SegIter, typename T>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter>)
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, SegIter>
    tag_invoke(hpx::fill_t, ExPolicy&& policy, SegIter first, SegIter last,
        T const& value)
    {
        static_assert(std::forward_iterator<SegIter>,
            "Requires at least forward iterator.");

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;

        if (first == last)
        {
            using result =
                hpx::parallel::util::detail::algorithm_result<ExPolicy,
                    SegIter>;
            return result::get(HPX_MOVE(first));
        }

        using iterator_traits = hpx::traits::segmented_iterator_traits<SegIter>;
        using value_type = typename std::iterator_traits<SegIter>::value_type;

        return segmented_for_each(
            hpx::parallel::detail::for_each<
                typename iterator_traits::local_iterator>(),
            HPX_FORWARD(ExPolicy, policy), first, last,
            hpx::parallel::detail::fill_function<value_type>(value),
            hpx::identity_v, is_seq());
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename SegIter, typename Size,
        typename T>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter> &&
            std::is_integral_v<Size>)
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, SegIter>
    tag_invoke(hpx::fill_n_t, ExPolicy&& policy, SegIter first, Size count,
        T const& value)
    {
        static_assert(std::forward_iterator<SegIter>,
            "Requires at least forward iterator.");

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;

        if (hpx::parallel::detail::is_negative(count) || count == 0)
        {
            using result =
                hpx::parallel::util::detail::algorithm_result<ExPolicy,
                    SegIter>;
            return result::get(HPX_MOVE(first));
        }

        using iterator_traits = hpx::traits::segmented_iterator_traits<SegIter>;
        using value_type = typename std::iterator_traits<SegIter>::value_type;
        SegIter last = first;
        hpx::parallel::detail::advance(last, count);
        return segmented_for_each(
            hpx::parallel::detail::for_each<
                typename iterator_traits::local_iterator>(),
            HPX_FORWARD(ExPolicy, policy), first, last,
            hpx::parallel::detail::fill_function<value_type>(value),
            hpx::identity_v, is_seq());
    }
}    // namespace hpx::segmented
