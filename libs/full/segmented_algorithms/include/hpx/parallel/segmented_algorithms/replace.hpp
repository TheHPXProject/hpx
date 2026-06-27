//  Copyright (c) 2026 Abhishek Bansal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/tag_invoke.hpp>

#include <hpx/parallel/segmented_algorithms/for_each.hpp>

#include <algorithm>
#include <exception>
#include <iterator>
#include <list>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    // segmented_replace

    /// \cond NOINTERNAL

    template <typename T>
    struct replace_function
    {
        replace_function() = default;

        template <typename T1, typename T2>
        explicit replace_function(T1&& old_val, T2&& new_val)
          : old_value_(HPX_FORWARD(T1, old_val))
          , new_value_(HPX_FORWARD(T2, new_val))
        {
        }

        T old_value_ = T();
        T new_value_ = T();

        void operator()(T& val) const
        {
            if (val == old_value_)
            {
                val = new_value_;
            }
        }
    };

    template <typename T>
    replace_function(T, T) -> replace_function<std::decay_t<T>>;

    template <typename T, typename F>
    struct replace_if_function
    {
        replace_if_function() = default;

        template <typename T_, typename F_>
        explicit replace_if_function(T_&& new_val, F_&& pred)
          : pred_(HPX_FORWARD(F_, pred))
          , new_value_(HPX_FORWARD(T_, new_val))
        {
        }

        F pred_ = F();
        T new_value_ = T();

        void operator()(T& val) const
        {
            if (HPX_INVOKE(pred_, val))
            {
                val = new_value_;
            }
        }
    };

    template <typename T, typename F>
    replace_if_function(T&&, F&&)
        -> replace_if_function<std::decay_t<T>, std::decay_t<F>>;
    /// \endcond
}    // namespace hpx::parallel::detail

namespace hpx::segmented {

    // segmented replace
    HPX_CXX_EXPORT template <typename SegIter, typename T>
        requires(hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter>)
    SegIter tag_invoke(hpx::replace_t, SegIter first, SegIter last,
        T const& old_value, T const& new_value)
    {
        static_assert(hpx::traits::is_forward_iterator_v<SegIter>,
            "Requires at least forward iterator.");

        if (first == last)
        {
            return HPX_MOVE(first);
        }

        using iterator_traits = hpx::traits::segmented_iterator_traits<SegIter>;

        return hpx::parallel::detail::segmented_for_each(
            hpx::parallel::detail::for_each<
                typename iterator_traits::local_iterator>(),
            hpx::execution::seq, first, last,
            hpx::parallel::detail::replace_function(old_value, new_value),
            hpx::identity_v, std::true_type{});
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename SegIter, typename T>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter>)
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, SegIter>
    tag_invoke(hpx::replace_t, ExPolicy&& policy, SegIter first, SegIter last,
        T const& old_value, T const& new_value)
    {
        static_assert(hpx::traits::is_forward_iterator_v<SegIter>,
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

        return segmented_for_each(
            hpx::parallel::detail::for_each<
                typename iterator_traits::local_iterator>(),
            HPX_FORWARD(ExPolicy, policy), first, last,
            hpx::parallel::detail::replace_function(old_value, new_value),
            hpx::identity_v, is_seq());
    }

    // segmented replace_if
    HPX_CXX_EXPORT template <typename SegIter, typename Pred, typename T>
        requires(hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter>)
    SegIter tag_invoke(hpx::replace_if_t, SegIter first, SegIter last,
        Pred&& pred, T const& new_value)
    {
        static_assert(hpx::traits::is_forward_iterator_v<SegIter>,
            "Requires at least forward iterator.");

        if (first == last)
        {
            return HPX_MOVE(first);
        }

        using iterator_traits = hpx::traits::segmented_iterator_traits<SegIter>;

        return hpx::parallel::detail::segmented_for_each(
            hpx::parallel::detail::for_each<
                typename iterator_traits::local_iterator>(),
            hpx::execution::seq, first, last,
            hpx::parallel::detail::replace_if_function(
                new_value, HPX_FORWARD(Pred, pred)),
            hpx::identity_v, std::true_type{});
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename SegIter, typename Pred,
        typename T>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter>)
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, SegIter>
    tag_invoke(hpx::replace_if_t, ExPolicy&& policy, SegIter first,
        SegIter last, Pred&& pred, T const& new_value)
    {
        static_assert(hpx::traits::is_forward_iterator_v<SegIter>,
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

        return segmented_for_each(
            hpx::parallel::detail::for_each<
                typename iterator_traits::local_iterator>(),
            HPX_FORWARD(ExPolicy, policy), first, last,
            hpx::parallel::detail::replace_if_function(
                new_value, HPX_FORWARD(Pred, pred)),
            hpx::identity_v, is_seq());
    }
}    // namespace hpx::segmented
