//  Copyright (c) 2015-2017 Francisco Jose Tapia
//  Copyright (c) 2020-2024 Hartmut Kaiser
//  Copyright (c) 2021 Akhil J Nair
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \page hpx::stable_sort
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx {
    // clang-format off

    ///////////////////////////////////////////////////////////////////////////
    /// Sorts the elements in the range [first, last) in ascending order. The
    /// relative order of equal elements is preserved. The function
    /// uses the given comparison function object comp (defaults to using
    /// operator<()).
    ///
    /// \note   Complexity: O(N log(N)), where N = std::distance(first, last)
    ///                     comparisons.
    ///
    /// A sequence is sorted with respect to a comparator \a comp and a
    /// projection \a proj if for every iterator i pointing to the sequence and
    /// every non-negative integer n such that i + n is a valid iterator
    /// pointing to an element of the sequence, and
    /// INVOKE(comp, INVOKE(proj, *(i + n)), INVOKE(proj, *i)) == false.
    ///
    /// \tparam RandomIt    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     random access iterator.
    /// \tparam Comp        The type of the function/function object to use
    ///                     (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to
    ///                     \a hpx::identity.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param comp         comp is a callable object. The return value of the
    ///                     INVOKE operation applied to an object of type Comp,
    ///                     when contextually converted to bool, yields true if
    ///                     the first argument of the call is less than the
    ///                     second, and false otherwise. It is assumed that comp
    ///                     will not apply any non-constant function through the
    ///                     dereferenced iterator.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each pair of elements as a
    ///                     projection operation before the actual predicate
    ///                     \a comp is invoked.
    ///
    /// \a comp has to induce a strict weak ordering on the values.
    ///
    /// The assignments in the parallel \a stable_sort algorithm invoked without
    /// an execution policy object execute in sequential order in the
    /// calling thread.
    ///
    /// \returns  The \a stable_sort algorithm returns \a void.
    ///
    template <typename RandomIt,
            typename Comp = hpx::parallel::detail::less,
            typename Proj = hpx::identity>
    void stable_sort(RandomIt first, RandomIt last, Comp&& comp = Comp(),
        Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Sorts the elements in the range [first, last) in ascending order. The
    /// relative order of equal elements is preserved. The function
    /// uses the given comparison function object comp (defaults to using
    /// operator<()). Executed according to the policy.
    ///
    /// \note   Complexity: O(N log(N)), where N = std::distance(first, last)
    ///                     comparisons.
    ///
    /// A sequence is sorted with respect to a comparator \a comp and a
    /// projection \a proj if for every iterator i pointing to the sequence and
    /// every non-negative integer n such that i + n is a valid iterator
    /// pointing to an element of the sequence, and
    /// INVOKE(comp, INVOKE(proj, *(i + n)), INVOKE(proj, *i)) == false.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it applies user-provided function objects.
    /// \tparam RandomIt    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     random access iterator.
    /// \tparam Comp        The type of the function/function object to use
    ///                     (deduced).
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to
    ///                     \a hpx::identity.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param comp         comp is a callable object. The return value of the
    ///                     INVOKE operation applied to an object of type Comp,
    ///                     when contextually converted to bool, yields true if
    ///                     the first argument of the call is less than the
    ///                     second, and false otherwise. It is assumed that comp
    ///                     will not apply any non-constant function through the
    ///                     dereferenced iterator.
    /// \param proj         Specifies the function (or function object) which
    ///                     will be invoked for each pair of elements as a
    ///                     projection operation before the actual predicate
    ///                     \a comp is invoked.
    ///
    /// \a comp has to induce a strict weak ordering on the values.
    ///
    /// The application of function objects in parallel algorithm
    /// invoked with an execution policy object of type
    /// \a sequenced_policy execute in sequential order in the
    /// calling thread.
    ///
    /// The application of function objects in parallel algorithm
    /// invoked with an execution policy object of type
    /// \a parallel_policy or \a parallel_task_policy are
    /// permitted to execute in an unordered fashion in unspecified
    /// threads, and indeterminately sequenced within each thread.
    ///
    /// \returns  The \a stable_sort algorithm returns a
    ///           \a hpx::future<void> if the execution policy is of
    ///           type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and returns \a void
    ///           otherwise.
    ///
    template <typename ExPolicy, typename RandomIt,
        typename Comp = hpx::parallel::detail::less,
        typename Proj = hpx::identity>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy>
    stable_sort(ExPolicy&& policy, RandomIt first, RandomIt last,
        Comp&& comp = Comp(), Proj&& proj = Proj());

    // clang-format on
}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected.hpp>
#include <hpx/contracts.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/advance_and_get_distance.hpp>
#include <hpx/parallel/algorithms/detail/advance_to_sentinel.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/parallel_stable_sort.hpp>
#include <hpx/parallel/algorithms/detail/spin_sort.hpp>
#include <hpx/parallel/util/compare_projected.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/chunk_size.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // stable_sort
    namespace detail {
        /// \cond NOINTERNAL

        ///////////////////////////////////////////////////////////////////////
        // stable_sort
        HPX_CXX_CORE_EXPORT template <typename RandomIt>
        struct stable_sort : public algorithm<stable_sort<RandomIt>, RandomIt>
        {
            constexpr stable_sort() noexcept
              : algorithm<stable_sort, RandomIt>("stable_sort")
            {
            }

            template <typename ExPolicy, typename Sentinel, typename Compare,
                typename Proj>
            static constexpr RandomIt sequential(ExPolicy, RandomIt first,
                Sentinel last, Compare&& comp, Proj&& proj)
            {
                using compare_type = util::compare_projected<Compare&, Proj&>;

                auto last_iter = detail::advance_to_sentinel(first, last);

                spin_sort(first, last_iter, compare_type(comp, proj));
                return last_iter;
            }

            template <typename ExPolicy, typename Sentinel, typename Compare,
                typename Proj>
            static util::detail::algorithm_result_t<ExPolicy, RandomIt>
            parallel(ExPolicy&& policy, RandomIt first, Sentinel last,
                Compare&& compare, Proj&& proj)
            {
                using algorithm_result =
                    util::detail::algorithm_result<ExPolicy, RandomIt>;
                using compare_type = util::compare_projected<Compare&, Proj&>;

                // number of elements to sort
                auto last_iter = first;
                std::size_t count =
                    detail::advance_and_get_distance(last_iter, last);
                if (count == 0)
                {
                    return algorithm_result::get(HPX_MOVE(last_iter));
                }

                // figure out the chunk size to use
                std::size_t cores =
                    hpx::execution::experimental::processing_units_count(
                        policy.parameters(), policy.executor(),
                        hpx::chrono::null_duration, count);

                std::size_t max_chunks =
                    hpx::execution::experimental::maximal_number_of_chunks(
                        policy.parameters(), policy.executor(), cores, count);

                std::size_t chunk_size =
                    hpx::execution::experimental::get_chunk_size(
                        policy.parameters(), policy.executor(),
                        hpx::chrono::null_duration, cores, count);

                util::detail::adjust_chunk_size_and_max_chunks(
                    cores, count, max_chunks, chunk_size);

                // we should not get smaller than our sort_limit_per_task
                chunk_size =
                    (std::max) (chunk_size, stable_sort_limit_per_task);

                try
                {
                    // call the sort routine and return the right type,
                    // depending on execution policy
                    compare_type comp(compare, proj);

                    return algorithm_result::get(
                        parallel_stable_sort(policy.executor(), first,
                            last_iter, cores, chunk_size, HPX_MOVE(comp)));
                }
                catch (...)
                {
                    return algorithm_result::get(
                        detail::handle_exception<ExPolicy, RandomIt>::call(
                            std::current_exception()));
                }
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::stable_sort
    HPX_CXX_CORE_EXPORT inline constexpr struct stable_sort_t final
      : hpx::detail::tag_parallel_algorithm<stable_sort_t>
    {
        template <typename RandomIt,
            typename Comp = hpx::parallel::detail::less>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<RandomIt> &&
                hpx::is_invocable_v<Comp,
                    hpx::traits::iter_value_t<RandomIt>,
                    hpx::traits::iter_value_t<RandomIt>
                >
            )
        // clang-format on
        friend void tag_fallback_invoke(hpx::stable_sort_t, RandomIt first,
            RandomIt last, Comp comp = Comp()) HPX_PRE(first <= last)
        {
            static_assert(std::random_access_iterator<RandomIt>,
                "Requires a random access iterator.");

            hpx::parallel::detail::stable_sort<RandomIt>().call(
                hpx::execution::seq, first, last, HPX_MOVE(comp),
                hpx::identity_v);
        }

        template <typename ExPolicy, typename RandomIt,
            typename Comp = hpx::parallel::detail::less>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<RandomIt> &&
                hpx::is_invocable_v<Comp,
                    hpx::traits::iter_value_t<RandomIt>,
                    hpx::traits::iter_value_t<RandomIt>
                >
            )
        // clang-format on
        friend hpx::parallel::util::detail::algorithm_result_t<ExPolicy>
        tag_fallback_invoke(hpx::stable_sort_t, ExPolicy&& policy,
            RandomIt first, RandomIt last, Comp comp = Comp())
            HPX_PRE(first <= last)
        {
            static_assert(std::random_access_iterator<RandomIt>,
                "Requires a random access iterator.");

            using result_type =
                hpx::parallel::util::detail::algorithm_result_t<ExPolicy>;

            return hpx::util::void_guard<result_type>(),
                   hpx::parallel::detail::stable_sort<RandomIt>().call(
                       HPX_FORWARD(ExPolicy, policy), first, last,
                       HPX_MOVE(comp), hpx::identity_v);
        }
    } stable_sort{};
}    // namespace hpx

#endif
