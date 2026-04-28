//  Copyright (c) 2015 Daniel Bourgeois
//  Copyright (c) 2017-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/is_sorted.hpp
/// \page hpx::is_sorted, hpx::is_sorted_until
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx {
    /// Determines if the range [first, last) is sorted. Uses pred to
    /// compare elements.
    ///
    /// \note   Complexity: at most (N+S-1) comparisons where
    ///         \a N = distance(first, last).
    ///         \a S = number of partitions
    ///
    /// \tparam FwdIter     The type of the source iterators used for the
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of that the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     that the algorithm will be applied to.
    /// \param pred         Refers to the binary predicate which returns true
    ///                     if the first argument should be treated as less than
    ///                     the second argument. The signature of the function
    ///                     should be equivalent to
    ///                     \code
    ///                     bool pred(const Type &a, const Type &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that objects of
    ///                     types \a FwdIter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///
    /// The comparison operations in the parallel \a is_sorted algorithm
    /// executes in sequential order in the calling thread.
    ///
    /// \returns  The \a is_sorted algorithm returns a \a bool.
    ///           The \a is_sorted algorithm returns true if each element in
    ///           the sequence [first, last) satisfies the predicate passed.
    ///           If the range [first, last) contains less than two elements,
    ///           the function always returns true.
    ///
    template <typename FwdIter, typename Pred = hpx::parallel::detail::less>
    bool is_sorted(FwdIter first, FwdIter last, Pred&& pred = Pred());

    /// Determines if the range [first, last) is sorted. Uses pred to
    /// compare elements. Executed according to the policy.
    ///
    /// \note   Complexity: at most (N+S-1) comparisons where
    ///         \a N = distance(first, last).
    ///         \a S = number of partitions
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used for the
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a is_sorted requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to std::less<>
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of that the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     that the algorithm will be applied to.
    /// \param pred         Refers to the binary predicate which returns true
    ///                     if the first argument should be treated as less than
    ///                     the second argument. The signature of the function
    ///                     should be equivalent to
    ///                     \code
    ///                     bool pred(const Type &a, const Type &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that objects of
    ///                     types \a FwdIter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///
    /// The comparison operations in the parallel \a is_sorted algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// executes in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a is_sorted algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a is_sorted algorithm returns a \a hpx::future<bool>
    ///           if the execution policy is of type \a task_execution_policy
    ///           and returns \a bool otherwise.
    ///           The \a is_sorted algorithm returns a bool if each element in
    ///           the sequence [first, last) satisfies the predicate passed.
    ///           If the range [first, last) contains less than two elements,
    ///           the function always returns true.
    ///
    template <typename ExPolicy, typename FwdIter,
        typename Pred = hpx::parallel::detail::less>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, bool> is_sorted(
        ExPolicy&& policy, FwdIter first, FwdIter last, Pred&& pred = Pred());

    /// Returns the first element in the range [first, last) that is not sorted.
    /// Uses a predicate to compare elements or the less than operator.
    ///
    /// \note   Complexity: at most (N+S-1) comparisons where
    ///         \a N = distance(first, last).
    ///         \a S = number of partitions
    ///
    /// \tparam FwdIter     The type of the source iterators used for the
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of that the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     that the algorithm will be applied to.
    /// \param pred         Refers to the binary predicate which returns true
    ///                     if the first argument should be treated as less than
    ///                     the second argument. The signature of the function
    ///                     should be equivalent to
    ///                     \code
    ///                     bool pred(const Type &a, const Type &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that objects of
    ///                     types \a FwdIter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///
    /// The comparison operations in the parallel \a is_sorted_until algorithm
    /// execute in sequential order in the calling thread.
    ///
    /// \returns  The \a is_sorted_until algorithm returns a \a FwdIter.
    ///           The \a is_sorted_until algorithm returns the first unsorted
    ///           element. If the sequence has less than two elements or the
    ///           sequence is sorted, last is returned.
    ///
    template <typename FwdIter, typename Pred = hpx::parallel::detail::less>
    FwdIter is_sorted_until(FwdIter first, FwdIter last, Pred&& pred = Pred());

    /// Returns the first element in the range [first, last) that is not sorted.
    /// Uses a predicate to compare elements or the less than operator.
    /// Executed according to the policy.
    ///
    /// \note   Complexity: at most (N+S-1) comparisons where
    ///         \a N = distance(first, last).
    ///         \a S = number of partitions
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used for the
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a is_sorted_until requires \a Pred to meet
    ///                     the requirements of \a CopyConstructible. This defaults
    ///                     to std::less<>
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of that the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     that the algorithm will be applied to.
    /// \param pred         Refers to the binary predicate which returns true
    ///                     if the first argument should be treated as less than
    ///                     the second argument. The signature of the function
    ///                     should be equivalent to
    ///                     \code
    ///                     bool pred(const Type &a, const Type &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that objects of
    ///                     types \a FwdIter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///
    /// The comparison operations in the parallel \a is_sorted_until algorithm
    /// invoked with an execution policy object of type
    /// \a sequenced_policy executes in sequential order in the
    /// calling thread.
    ///
    /// The comparison operations in the parallel \a is_sorted_until algorithm
    /// invoked with an execution policy object of type
    /// \a parallel_policy or \a parallel_task_policy are
    /// permitted to execute in an unordered fashion in unspecified threads,
    /// and indeterminately sequenced within each thread.
    ///
    /// \returns  The \a is_sorted_until algorithm returns a \a hpx::future<FwdIter>
    ///           if the execution policy is of type \a task_execution_policy
    ///           and returns \a FwdIter otherwise.
    ///           The \a is_sorted_until algorithm returns the first unsorted
    ///           element. If the sequence has less than two elements or the
    ///           sequence is sorted, last is returned.
    ///
    template <typename ExPolicy, typename FwdIter,
        typename Pred = hpx::parallel::detail::less>
    typename hpx::parallel::util::detail::algorithm_result<ExPolicy,
        FwdIter>::type
    is_sorted_until(
        ExPolicy&& policy, FwdIter first, FwdIter last, Pred&& pred = Pred());
}    // namespace hpx

#else

#include <hpx/config.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/is_sorted.hpp>
#include <hpx/parallel/util/adapt_placement_mode.hpp>
#include <hpx/parallel/util/cancellation_token.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/clear_container.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/invoke_projected.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/partitioner.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    ////////////////////////////////////////////////////////////////////////////
    // is_sorted
    namespace detail {

        /// \cond NOINTERNAL
        template <typename FwdIter, typename Sent>
        struct is_sorted : public algorithm<is_sorted<FwdIter, Sent>, bool>
        {
            constexpr is_sorted() noexcept
              : algorithm<is_sorted, bool>("is_sorted")
            {
            }

            template <typename ExPolicy, typename FwdIter_, typename Sent_,
                typename Pred, typename Proj>
            static constexpr bool sequential(
                ExPolicy, FwdIter_ first, Sent_ last, Pred&& pred, Proj&& proj)
            {
                return is_sorted_sequential(first, last,
                    HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
            }

            template <typename ExPolicy, typename FwdIter_, typename Sent_,
                typename Pred, typename Proj>
            static decltype(auto) parallel(ExPolicy&& policy, FwdIter_ first,
                Sent_ last, Pred&& pred, Proj&& proj)
            {
                using difference_type =
                    typename std::iterator_traits<FwdIter_>::difference_type;
                using result =
                    typename util::detail::algorithm_result<ExPolicy, bool>;
                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                difference_type count = detail::distance(first, last);

                if constexpr (!has_scheduler_executor)
                {
                    if (count <= 1)
                        return result::get(true);
                }

                util::invoke_projected<Pred, Proj> pred_projected{
                    HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj)};
                hpx::parallel::util::cancellation_token<> tok;
                using intermediate_result_t = std::uint8_t;

                // Note: replacing the invoke() with HPX_INVOKE()
                // below makes gcc generate errors
                auto f1 =
                    [tok, last, pred_projected = HPX_MOVE(pred_projected)](
                        FwdIter_ part_begin, std::size_t part_size) mutable
                    -> intermediate_result_t {
                    FwdIter_ trail = part_begin++;
                    util::const_loop_n<std::decay_t<ExPolicy>>(part_begin,
                        part_size - 1,
                        [&trail, &tok, &pred_projected](
                            FwdIter_ it) mutable -> void {
                            if (hpx::invoke(pred_projected, *it, *trail++))
                            {
                                tok.cancel();
                            }
                        });

                    FwdIter_ i = trail++;

                    // trail now points one past the current grouping unless
                    // canceled

                    if (!tok.was_cancelled() && trail != last)
                    {
                        return !hpx::invoke(pred_projected, *trail, *i);
                    }

                    return !tok.was_cancelled();
                };

                auto f2 = [](auto&& results) {
                    return std::all_of(hpx::util::begin(results),
                        hpx::util::end(results), hpx::functional::unwrap{});
                };

                return util::partitioner<ExPolicy, bool,
                    intermediate_result_t>::call(HPX_FORWARD(ExPolicy, policy),
                    first, count, HPX_MOVE(f1), HPX_MOVE(f2));
            }
        };
        /// \endcond
    }    // namespace detail

    ////////////////////////////////////////////////////////////////////////////
    // is_sorted_until
    namespace detail {

        /// \cond NOINTERNAL
        template <typename FwdIter, typename Sent>
        struct is_sorted_until
          : public algorithm<is_sorted_until<FwdIter, Sent>, FwdIter>
        {
            constexpr is_sorted_until() noexcept
              : algorithm<is_sorted_until, FwdIter>("is_sorted_until")
            {
            }

            template <typename ExPolicy, typename FwdIter_, typename Sent_,
                typename Pred, typename Proj>
            static constexpr FwdIter_ sequential(
                ExPolicy, FwdIter_ first, Sent_ last, Pred&& pred, Proj&& proj)
            {
                return is_sorted_until_sequential(first, last,
                    HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
            }

            template <typename ExPolicy, typename FwdIter_, typename Sent_,
                typename Pred, typename Proj>
            static decltype(auto) parallel(ExPolicy&& orgpolicy, FwdIter_ first,
                Sent_ last, Pred&& pred, Proj&& proj)
            {
                using reference =
                    typename std::iterator_traits<FwdIter_>::reference;
                using difference_type =
                    typename std::iterator_traits<FwdIter_>::difference_type;
                using result =
                    typename util::detail::algorithm_result<ExPolicy, FwdIter_>;
                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                difference_type count = detail::distance(first, last);
                if constexpr (!has_scheduler_executor)
                {
                    if (count <= 1)
                        return result::get(HPX_MOVE(last));
                }

                util::invoke_projected<Pred, Proj> pred_projected{
                    HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj)};

                decltype(auto) policy =
                    hpx::execution::experimental::adapt_placement_mode(
                        HPX_FORWARD(ExPolicy, orgpolicy),
                        hpx::threads::thread_placement_hint::breadth_first);

                using policy_type = std::decay_t<decltype(policy)>;

                hpx::parallel::util::cancellation_token<difference_type> tok(
                    count);

                // Note: replacing the invoke() with HPX_INVOKE() below makes
                // gcc generate errors
                auto f1 = [tok, last,
                              pred_projected = HPX_MOVE(pred_projected)](
                              FwdIter_ part_begin, std::size_t part_size,
                              std::size_t base_idx) mutable -> void {
                    std::size_t const cross_idx = base_idx + part_size;

                    FwdIter_ trail = part_begin++;
                    util::loop_idx_n<policy_type>(++base_idx, part_begin,
                        part_size - 1, tok,
                        [&trail, &tok, &pred_projected](
                            reference& v, std::size_t ind) -> void {
                            if (hpx::invoke(pred_projected, v, *trail++))
                            {
                                tok.cancel(ind);
                            }
                        });

                    FwdIter_ i = trail++;

                    // trail now points one past the current grouping unless
                    // canceled

                    if (!tok.was_cancelled(cross_idx) && trail != last)
                    {
                        if (HPX_INVOKE(pred_projected, *trail, *i))
                        {
                            tok.cancel(cross_idx);
                        }
                    }
                };

                auto f2 = [first, tok](auto&&... data) mutable -> FwdIter_ {
                    static_assert(sizeof...(data) < 2);

                    // make sure iterators embedded in function object that is
                    // attached to futures are invalidated
                    util::detail::clear_container(data...);

                    difference_type loc = tok.get_data();
                    std::advance(first, loc);
                    return first;
                };

                using partitioner_type =
                    util::partitioner<policy_type, FwdIter_, void>;
                return partitioner_type::call_with_index(
                    HPX_FORWARD(decltype(policy), policy), first, count, 1,
                    HPX_MOVE(f1), HPX_MOVE(f2));
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    HPX_CXX_CORE_EXPORT inline constexpr struct is_sorted_t final
      : hpx::detail::tag_parallel_algorithm<is_sorted_t>
    {
    private:
        template <typename FwdIter, typename Pred = hpx::parallel::detail::less,
            typename Proj = hpx::identity>
        // clang-format off
            requires (
                std::forward_iterator<FwdIter> &&
                hpx::parallel::traits::is_projected_v<Proj, FwdIter> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    hpx::parallel::traits::projected<Proj, FwdIter>,
                    hpx::parallel::traits::projected<Proj, FwdIter>
                >
            )
        // clang-format on
        friend bool tag_fallback_invoke(hpx::is_sorted_t, FwdIter first,
            FwdIter last, Pred pred = Pred(), Proj proj = Proj())
        {
            return hpx::parallel::detail::is_sorted<FwdIter, FwdIter>().call(
                hpx::execution::seq, first, last, HPX_MOVE(pred),
                HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename FwdIter,
            typename Pred = hpx::parallel::detail::less,
            typename Proj = hpx::identity>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                std::forward_iterator<FwdIter> &&
                hpx::parallel::traits::is_projected_v<Proj, FwdIter> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    hpx::parallel::traits::projected<Proj, FwdIter>,
                    hpx::parallel::traits::projected<Proj, FwdIter>
                >
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(hpx::is_sorted_t,
            ExPolicy&& policy, FwdIter first, FwdIter last, Pred pred = Pred(),
            Proj proj = Proj())
        {
            return hpx::parallel::detail::is_sorted<FwdIter, FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(pred),
                HPX_MOVE(proj));
        }
    } is_sorted{};

    HPX_CXX_CORE_EXPORT inline constexpr struct is_sorted_until_t final
      : hpx::detail::tag_parallel_algorithm<is_sorted_until_t>
    {
    private:
        template <typename FwdIter, typename Pred = hpx::parallel::detail::less,
            typename Proj = hpx::identity>
        // clang-format off
            requires (
                std::forward_iterator<FwdIter> &&
                hpx::parallel::traits::is_projected_v<Proj, FwdIter> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    hpx::parallel::traits::projected<Proj, FwdIter>,
                    hpx::parallel::traits::projected<Proj, FwdIter>
                >
            )
        // clang-format on
        friend FwdIter tag_fallback_invoke(hpx::is_sorted_until_t,
            FwdIter first, FwdIter last, Pred pred = Pred(), Proj proj = Proj())
        {
            return hpx::parallel::detail::is_sorted_until<FwdIter, FwdIter>()
                .call(hpx::execution::seq, first, last, HPX_MOVE(pred),
                    HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename FwdIter,
            typename Pred = hpx::parallel::detail::less,
            typename Proj = hpx::identity>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                std::forward_iterator<FwdIter> &&
                hpx::parallel::traits::is_projected_v<Proj, FwdIter> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    hpx::parallel::traits::projected<Proj, FwdIter>,
                    hpx::parallel::traits::projected<Proj, FwdIter>
                >
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(hpx::is_sorted_until_t,
            ExPolicy&& policy, FwdIter first, FwdIter last, Pred pred = Pred(),
            Proj proj = Proj())
        {
            return hpx::parallel::detail::is_sorted_until<FwdIter, FwdIter>()
                .call(HPX_FORWARD(ExPolicy, policy), first, last,
                    HPX_MOVE(pred), HPX_MOVE(proj));
        }
    } is_sorted_until{};
}    // namespace hpx

#endif
