//  Copyright (c) 2015 John Biddiscombe
//  Copyright (c) 2015-2024 Hartmut Kaiser
//  Copyright (c) 2015-2019 Francisco Jose Tapia
//  Copyright (c) 2018 Taeguk Kwon
//  Copyright (c) 2021 Akhil J Nair
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \page hpx::sort
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx {
    // clang-format off

    ///////////////////////////////////////////////////////////////////////////
    /// Sorts the elements in the range [first, last) in ascending order. The
    /// order of equal elements is not guaranteed to be preserved. The function
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
    ///                     INVOKE operation applied to an object of type \a Comp,
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
    /// The assignments in the parallel \a sort algorithm invoked without
    /// an execution policy object execute in sequential order in the
    /// calling thread.
    ///
    /// \returns  The \a sort algorithm returns \a void.
    ///
    template <typename RandomIt,
        typename Comp = hpx::parallel::detail::less,
        typename Proj = hpx::identity>
    void sort(RandomIt first, RandomIt last, Comp&& comp, Proj&& proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Sorts the elements in the range [first, last) in ascending order. The
    /// order of equal elements is not guaranteed to be preserved. The function
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
    ///                     INVOKE operation applied to an object of type \a Comp,
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
    /// \returns  The \a sort algorithm returns a
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
    sort(ExPolicy&& policy, RandomIt first, RandomIt last, Comp&& comp,
        Proj&& proj);

    // clang-format on
}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected.hpp>
#include <hpx/assert.hpp>
#include <hpx/contracts.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/advance_to_sentinel.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/is_sorted.hpp>
#include <hpx/parallel/algorithms/detail/pivot.hpp>
#include <hpx/parallel/util/compare_projected.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/chunk_size.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iterator>
#include <list>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // sort
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT inline constexpr std::size_t sort_limit_per_task =
            65536ul;

        // \brief this function is the work assigned to each thread in the
        //        parallel process
        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename RandomIt,
            typename Comp>
        hpx::future<RandomIt> sort_thread(ExPolicy&& policy, RandomIt first,
            RandomIt last, Comp comp, std::size_t chunk_size)
        {
            std::ptrdiff_t const N = last - first;
            if (static_cast<std::size_t>(N) <= chunk_size)
            {
                return execution::async_execute(policy.executor(),
                    [first, last, comp = HPX_MOVE(comp)]() -> RandomIt {
                        std::sort(first, last, comp);
                        return last;
                    });
            }

            // check if sorted
            if (detail::is_sorted_sequential(first, last, comp))
            {
                return hpx::make_ready_future(last);
            }

            // pivot selections
            pivot9(first, last, comp);

            using value_type =
                typename std::iterator_traits<RandomIt>::value_type;

            value_type val = *first;
            RandomIt c_first = first + 1, c_last = last - 1;

            while (c_first < last && comp(*c_first, val))
            {
                ++c_first;
            }
            while (c_last > first && comp(val, *c_last))
            {
                --c_last;
            }
            while (c_first < c_last)
            {
                std::ranges::iter_swap(c_first++, c_last--);
                while (c_first < last && comp(*c_first, val))
                {
                    ++c_first;
                }
                while (c_last > first && comp(val, *c_last))
                {
                    --c_last;
                }
            }

            std::ranges::iter_swap(first, c_last);

            // spawn tasks for each sub section
            hpx::future<RandomIt> left = execution::async_execute(
                policy.executor(), &sort_thread<ExPolicy, RandomIt, Comp>,
                policy, first, c_last, comp, chunk_size);

            hpx::future<RandomIt> right = execution::async_execute(
                policy.executor(), &sort_thread<ExPolicy, RandomIt, Comp>,
                policy, c_first, last, comp, chunk_size);

            return hpx::dataflow(
                policy.executor(),
                [last](hpx::future<RandomIt>&& leftf,
                    hpx::future<RandomIt>&& rightf) -> RandomIt {
                    if (leftf.has_exception() || rightf.has_exception())
                    {
                        std::list<std::exception_ptr> errors;
                        if (leftf.has_exception())
                            errors.push_back(leftf.get_exception_ptr());
                        if (rightf.has_exception())
                            errors.push_back(rightf.get_exception_ptr());

                        throw exception_list(HPX_MOVE(errors));
                    }
                    return last;
                },
                HPX_MOVE(left), HPX_MOVE(right));
        }

        // policy : execution policy
        // [in] first   iterator to the first element to sort
        // [in] last    iterator to the next element after the last
        // [in] comp    object for to Comp
        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename RandomIt,
            typename Comp>
        hpx::future<RandomIt> parallel_sort_async(
            ExPolicy&& policy, RandomIt first, RandomIt last, Comp&& comp)
        {
            // number of elements to sort
            std::size_t count = last - first;
            if (count == 0)
            {
                return hpx::make_ready_future(last);
            }

            // figure out the chunk size to use
            std::size_t const cores =
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
            chunk_size = (std::max) (chunk_size, sort_limit_per_task);

            if (count < chunk_size)
            {
                std::sort(first, last, comp);
                return hpx::make_ready_future(last);
            }

            // check if already sorted
            if (detail::is_sorted_sequential(first, last, comp))
            {
                return hpx::make_ready_future(last);
            }

            return execution::async_execute(policy.executor(),
                &sort_thread<std::decay_t<ExPolicy>, RandomIt, Comp>,
                HPX_FORWARD(ExPolicy, policy), first, last,
                HPX_FORWARD(Comp, comp), chunk_size);
        }

        ///////////////////////////////////////////////////////////////////////
        // sort
        HPX_CXX_CORE_EXPORT template <typename RandomIt>
        struct sort : public algorithm<sort<RandomIt>, RandomIt>
        {
            constexpr sort() noexcept
              : algorithm<sort, RandomIt>("sort")
            {
            }

            template <typename ExPolicy, typename Sent, typename Comp,
                typename Proj>
            static constexpr RandomIt sequential(
                ExPolicy, RandomIt first, Sent last, Comp&& comp, Proj&& proj)
            {
                auto last_iter = detail::advance_to_sentinel(first, last);
                std::sort(first, last_iter,
                    util::compare_projected<Comp&, Proj&>(comp, proj));
                return last_iter;
            }

            template <typename ExPolicy, typename Sent, typename Comp,
                typename Proj>
            static util::detail::algorithm_result_t<ExPolicy, RandomIt>
            parallel(ExPolicy&& policy, RandomIt first, Sent last_s,
                Comp&& comp, Proj&& proj)
            {
                auto last = detail::advance_to_sentinel(first, last_s);
                typedef util::detail::algorithm_result<ExPolicy, RandomIt>
                    algorithm_result;

                try
                {
                    // call the sort routine and return the right type,
                    // depending on execution policy
                    return algorithm_result::get(parallel_sort_async(
                        HPX_FORWARD(ExPolicy, policy), first, last,
                        util::compare_projected<Comp&, Proj&>(comp, proj)));
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
    // CPO for hpx::sort
    HPX_CXX_CORE_EXPORT inline constexpr struct sort_t final
      : hpx::detail::tag_parallel_algorithm<sort_t>
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
        friend void tag_fallback_invoke(hpx::sort_t, RandomIt first,
            RandomIt last, Comp comp = Comp()) HPX_PRE(first <= last)
        {
            static_assert(std::random_access_iterator<RandomIt>,
                "Requires a random access iterator.");

            hpx::parallel::detail::sort<RandomIt>().call(hpx::execution::seq,
                first, last, HPX_MOVE(comp), hpx::identity_v);
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
        friend parallel::util::detail::algorithm_result_t<ExPolicy>
        tag_fallback_invoke(hpx::sort_t, ExPolicy&& policy, RandomIt first,
            RandomIt last, Comp comp = Comp()) HPX_PRE(first <= last)
        {
            static_assert(std::random_access_iterator<RandomIt>,
                "Requires a random access iterator.");

            using result_type =
                typename hpx::parallel::util::detail::algorithm_result<
                    ExPolicy>::type;

            return hpx::util::void_guard<result_type>(),
                   hpx::parallel::detail::sort<RandomIt>().call(
                       HPX_FORWARD(ExPolicy, policy), first, last,
                       HPX_MOVE(comp), hpx::identity_v);
        }
    } sort{};
}    // namespace hpx

#endif    // DOXYGEN
