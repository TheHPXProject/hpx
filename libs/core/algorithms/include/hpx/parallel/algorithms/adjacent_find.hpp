//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2014 Grant Mercer
//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/adjacent_find.hpp
/// \page hpx::adjacent_find
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx {
    /// Searches the range [first, last) for two consecutive identical elements.
    ///
    /// \note   Complexity: Exactly the smaller of (\a result - \a first) + 1 and
    ///                     (\a last - \a first) - 1 application of the predicate
    ///                     where \a result is the value returned
    ///
    /// \tparam InIter      The type of the source iterators used for the
    ///                     range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the range the algorithm will be applied to.
    /// \param pred         The binary predicate which returns \a true
    ///                     if the elements should be treated as equal. The
    ///                     signature should be equivalent to the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 must be such
    ///                     that objects of type \a InIter
    ///                     can be dereferenced and then implicitly converted
    ///                     to \a Type1 .
    ///
    /// \returns  The \a adjacent_find algorithm returns an iterator to the
    ///           first of the identical elements. If no such elements are
    ///           found, \a last is returned.
    template <typename InIter, typename Pred = hpx::parallel::detail::equal_to>
    InIter adjacent_find(InIter first, InIter last, Pred&& pred = Pred());

    /// Searches the range [first, last) for two consecutive identical elements.
    /// This version uses the given binary predicate pred
    ///
    /// \note   Complexity: Exactly the smaller of (\a result - \a first) + 1 and
    ///                     (\a last - \a first) - 1 application of the predicate
    ///                     where \a result is the value returned
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used for the
    ///                     range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a adjacent_find requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::equal_to<>
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the range the algorithm will be applied to.
    /// \param pred         The binary predicate which returns \a true
    ///                     if the elements should be treated as equal. The
    ///                     signature should be equivalent to the following:
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 must be such
    ///                     that objects of type \a FwdIter
    ///                     can be dereferenced and then implicitly converted
    ///                     to \a Type1 .
    ///
    /// The comparison operations in the parallel \a adjacent_find invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a adjacent_find invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an
    /// unordered fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a adjacent_find algorithm returns a \a hpx::future<FwdIter>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter otherwise.
    ///           The \a adjacent_find algorithm returns an iterator to the
    ///           first of the identical elements. If no such elements are
    ///           found, \a last is returned.
    ///
    ///           This overload of \a adjacent_find is available if the user
    ///           decides to provide their algorithm their own binary
    ///           predicate \a pred.
    ///
    template <typename ExPolicy, typename FwdIter,
        typename Pred = hpx::parallel::detail::equal_to>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter>
    adjacent_find(
        ExPolicy&& policy, FwdIter first, FwdIter last, Pred&& pred = Pred());
}    // namespace hpx

#else

#include <hpx/config.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/adjacent_find.hpp>
#include <hpx/parallel/algorithms/detail/adjacent_find.hpp>
#include <hpx/parallel/algorithms/detail/advance_to_sentinel.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/util/adapt_placement_mode.hpp>
#include <hpx/parallel/util/cancellation_token.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/clear_container.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/invoke_projected.hpp>
#include <hpx/parallel/util/partitioner.hpp>
#include <hpx/parallel/util/zip_iterator.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // adjacent_find
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT template <typename Iter, typename Sent>
        struct adjacent_find : public algorithm<adjacent_find<Iter, Sent>, Iter>
        {
            constexpr adjacent_find() noexcept
              : algorithm<adjacent_find, Iter>("adjacent_find")
            {
            }

            template <typename ExPolicy, typename InIter, typename Sent_,
                typename Pred, typename Proj>
            static InIter sequential(
                ExPolicy, InIter first, Sent_ last, Pred&& pred, Proj&& proj)
            {
                return sequential_adjacent_find<ExPolicy>(first, last,
                    util::invoke_projected<Pred, Proj>(
                        HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj)));
            }

            template <typename ExPolicy, typename FwdIter, typename Sent_,
                typename Pred, typename Proj>
            static decltype(auto) parallel(ExPolicy&& orgpolicy, FwdIter first,
                Sent_ last, Pred&& pred, Proj&& proj)
            {
                using zip_iterator = hpx::util::zip_iterator<FwdIter, FwdIter>;
                using difference_type =
                    typename std::iterator_traits<FwdIter>::difference_type;
                using result =
                    util::detail::algorithm_result<ExPolicy, FwdIter>;

                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                if constexpr (!has_scheduler_executor)
                {
                    if (first == last)
                    {
                        return result::get(HPX_MOVE(last));
                    }
                }

                decltype(auto) policy =
                    hpx::execution::experimental::adapt_placement_mode(
                        HPX_FORWARD(ExPolicy, orgpolicy),
                        hpx::threads::thread_placement_hint::breadth_first);

                using policy_type = std::decay_t<decltype(policy)>;

                difference_type count = detail::distance(first, last);

                FwdIter next = first;
                if (count > 0)
                {
                    ++next;
                }
                util::cancellation_token<difference_type> tok(count);

                util::invoke_projected<Pred, Proj> pred_projected{
                    HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj)};

                auto f1 = [pred_projected = HPX_MOVE(pred_projected), tok](
                              zip_iterator it, std::size_t part_size,
                              std::size_t base_idx) mutable {
                    sequential_adjacent_find<policy_type>(
                        base_idx, it, part_size, tok, pred_projected);
                };

                auto f2 = [tok, first](auto&&... data) mutable -> FwdIter {
                    static_assert(sizeof...(data) < 2);

                    // make sure iterators embedded in function object that
                    // is attached to futures are invalidated
                    util::detail::clear_container(data...);

                    difference_type adj_find_res = tok.get_data();
                    std::advance(first, adj_find_res);
                    return first;
                };

                if constexpr (has_scheduler_executor)
                {
                    // underflow prevention for the upcoming call
                    if (count == 0)
                        ++count;
                }

                using partitioner_type =
                    util::partitioner<policy_type, FwdIter, void>;
                return partitioner_type::call_with_index(
                    HPX_FORWARD(decltype(policy), policy),
                    hpx::util::zip_iterator(first, next), count - 1, 1,
                    HPX_MOVE(f1), HPX_MOVE(f2));
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    HPX_CXX_CORE_EXPORT inline constexpr struct adjacent_find_t final
      : hpx::detail::tag_parallel_algorithm<adjacent_find_t>
    {
    private:
        template <typename InIter,
            typename Pred = hpx::parallel::detail::equal_to>
            requires(std::input_iterator<InIter>)
        friend InIter tag_fallback_invoke(
            hpx::adjacent_find_t, InIter first, InIter last, Pred pred = Pred())
        {
            static_assert(std::input_iterator<InIter>,
                "Requires at least input iterator.");

            return parallel::detail::adjacent_find<InIter, InIter>().call(
                hpx::execution::seq, first, last, HPX_MOVE(pred),
                hpx::identity_v);
        }

        template <typename ExPolicy, typename FwdIter,
            typename Pred = hpx::parallel::detail::equal_to>
        // clang-format off
        requires (
            hpx::is_execution_policy_v<ExPolicy> &&
            std::forward_iterator<FwdIter>
        )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(hpx::adjacent_find_t,
            ExPolicy&& policy, FwdIter first, FwdIter last, Pred pred = Pred())
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least a forward iterator");

            return parallel::detail::adjacent_find<FwdIter, FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(pred),
                hpx::identity_v);
        }
    } adjacent_find{};
}    // namespace hpx

#endif
