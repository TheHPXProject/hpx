//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2015 Daniel Bourgeois
//  Copyright (c) 2017-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \page hpx::is_partitioned
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {
    /// Determines if the range [first, last) is partitioned.
    ///
    /// \note   Complexity: at most (N) predicate evaluations where
    ///         \a N = distance(first, last).
    ///
    /// \tparam FwdIter     The type of the source iterators used for the
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced).
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of that the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     that the algorithm will be applied to.
    /// \param pred         Refers to the unary predicate which returns true
    ///                     for elements expected to be found in the beginning
    ///                     of the range. The signature of the function
    ///                     should be equivalent to
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that objects of
    ///                     types \a FwdIter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///
    /// \returns  The \a is_partitioned algorithm returns \a bool.
    ///           The \a is_partitioned algorithm returns true if each element
    ///           in the sequence for which pred returns true precedes those for
    ///           which pred returns false. Otherwise is_partitioned returns
    ///           false. If the range [first, last) contains less than two
    ///           elements, the function is always true.
    ///
    template <typename FwdIter, typename Pred>
    bool is_partitioned(FwdIter first, FwdIter last, Pred&& pred);

    /// Determines if the range [first, last) is partitioned.
    /// Executed according to the policy.
    ///
    /// \note   Complexity: at most (N) predicate evaluations where
    ///         \a N = distance(first, last).
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used for the
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). \a Pred must be \a CopyConstructible
    ///                     when using a parallel policy.
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of that the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     that the algorithm will be applied to.
    /// \param pred         Refers to the unary predicate which returns true
    ///                     for elements expected to be found in the beginning
    ///                     of the range. The signature of the function
    ///                     should be equivalent to
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that objects of
    ///                     types \a FwdIter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///
    /// The predicate operations in the parallel \a is_partitioned algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// executes in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a is_partitioned algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a is_partitioned algorithm returns a \a hpx::future<bool>
    ///           if the execution policy is of type \a task_execution_policy
    ///           and returns \a bool otherwise.
    ///           The \a is_partitioned algorithm returns true if each element
    ///           in the sequence for which pred returns true precedes those for
    ///           which pred returns false. Otherwise is_partitioned returns
    ///           false. If the range [first, last) contains less than two
    ///           elements, the function is always true.
    ///
    template <typename ExPolicy, typename FwdIter, typename Pred>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, bool>
    is_partitioned(ExPolicy&& policy, FwdIter first, FwdIter last, Pred&& pred);
}    // namespace hpx
#else

#include <hpx/config.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/util/cancellation_token.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/invoke_projected.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/partitioner.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parallel {

    ////////////////////////////////////////////////////////////////////////////
    // is_partitioned
    namespace detail {
        enum class partition_status : std::uint8_t
        {
            is_partitioned = 0,
            is_not_partitioned = 1,
            mixed = 2,
            cancelled = 3
        };

        /// \cond NOINTERNAL
        template <typename T>
        inline bool sequential_is_partitioned(std::vector<T>&& res)
        {
            auto first = res.begin();
            auto const last = res.end();
            while (first != last && hpx::unwrap(*first))
            {
                ++first;
            }
            if (first != last)
            {
                ++first;
                while (first != last)
                {
                    if (hpx::unwrap(*first))
                        return false;
                    ++first;
                }
            }
            return true;
        }

        template <typename Iter, typename Sent>
        struct is_partitioned
          : public algorithm<is_partitioned<Iter, Sent>, bool>
        {
            constexpr is_partitioned() noexcept
              : algorithm<is_partitioned, bool>("is_partitioned")
            {
            }

            template <typename ExPolicy, typename InIter, typename InSent,
                typename Pred, typename Proj>
            static constexpr bool sequential(
                ExPolicy, InIter first, InSent last, Pred&& pred, Proj&& proj)
            {
                return std::is_partitioned(first, last,
                    util::invoke_projected<Pred, Proj>(
                        HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj)));
            }

            template <typename ExPolicy, typename Iter_, typename Sent_,
                typename Pred, typename Proj>
            static decltype(auto) parallel(ExPolicy&& policy, Iter_ first,
                Sent_ last, Pred&& pred, Proj&& proj)
            {
                using difference_type = hpx::traits::iter_difference_t<Iter_>;
                using result = util::detail::algorithm_result<ExPolicy, bool>;
                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                difference_type count =
                    hpx::parallel::detail::distance(first, last);

                if constexpr (!has_scheduler_executor)
                {
                    if (count <= 1)
                        return result::get(true);
                }

                util::invoke_projected<Pred, Proj> pred_projected(
                    HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
                util::cancellation_token<> tok;
                using intermediate_result_t = partition_status;

                auto f1 = [tok, pred_projected = HPX_MOVE(pred_projected)](
                              Iter_ part_begin, std::size_t part_count) mutable
                    -> intermediate_result_t {
                    bool fst_bool = HPX_INVOKE(pred_projected, *part_begin);
                    if (part_count == 1)
                        return fst_bool ? partition_status::is_partitioned :
                                          partition_status::is_not_partitioned;

                    bool is_mixed = false;

                    util::const_loop_n<std::decay_t<ExPolicy>>(++part_begin,
                        --part_count, tok,
                        [&fst_bool, &is_mixed, &pred_projected, &tok](
                            Iter_ const& a) mutable -> void {
                            if (fst_bool != hpx::invoke(pred_projected, *a))
                            {
                                if (fst_bool)
                                {
                                    fst_bool = false;
                                    is_mixed = true;
                                }
                                else
                                {
                                    tok.cancel();
                                }
                            }
                        });

                    if (tok.was_cancelled())
                        return partition_status::cancelled;

                    return is_mixed ?
                        partition_status::mixed :
                        (fst_bool ? partition_status::is_partitioned :
                                    partition_status::is_not_partitioned);
                };

                auto f2 = [tok](auto&& results) -> bool {
                    if (tok.was_cancelled())
                        return false;

                    auto it = std::find_if(hpx::util::begin(results),
                        hpx::util::end(results), [](partition_status x) {
                            return x != partition_status::is_partitioned;
                        });

                    if (it == hpx::util::end(results))
                        return true;

                    if (*it == partition_status::mixed)
                        ++it;

                    return std::all_of(
                        it, hpx::util::end(results), [](partition_status x) {
                            return x == partition_status::is_not_partitioned;
                        });
                };

                return util::partitioner<ExPolicy, bool,
                    intermediate_result_t>::call(HPX_FORWARD(ExPolicy, policy),
                    first, count, HPX_MOVE(f1), HPX_MOVE(f2));
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    HPX_CXX_CORE_EXPORT inline constexpr struct is_partitioned_t final
      : hpx::detail::tag_parallel_algorithm<is_partitioned_t>
    {
    private:
        template <typename FwdIter, typename Pred,
            typename Proj = hpx::identity>
        // clang-format off
            requires (
                std::forward_iterator<FwdIter> &&
                hpx::parallel::traits::is_projected_v<Proj, FwdIter> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    hpx::parallel::traits::projected<Proj, FwdIter>
                >
            )
        // clang-format on
        friend bool tag_fallback_invoke(hpx::is_partitioned_t, FwdIter first,
            FwdIter last, Pred pred, Proj proj = Proj())
        {
            return hpx::parallel::detail::is_partitioned<FwdIter, FwdIter>()
                .call(hpx::execution::seq, first, last, HPX_MOVE(pred),
                    HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename FwdIter, typename Pred,
            typename Proj = hpx::identity>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                std::forward_iterator<FwdIter> &&
                hpx::parallel::traits::is_projected_v<Proj, FwdIter> &&
                hpx::parallel::traits::is_indirect_callable_v<
                    hpx::execution::sequenced_policy, Pred,
                    hpx::parallel::traits::projected<Proj, FwdIter>
                >
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(hpx::is_partitioned_t,
            ExPolicy&& policy, FwdIter first, FwdIter last, Pred pred,
            Proj proj = Proj())
        {
            return hpx::parallel::detail::is_partitioned<FwdIter, FwdIter>()
                .call(HPX_FORWARD(ExPolicy, policy), first, last,
                    HPX_MOVE(pred), HPX_MOVE(proj));
        }
    } is_partitioned{};
}    // namespace hpx

#endif
