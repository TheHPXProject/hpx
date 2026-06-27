//  Copyright (c) 2022 Bhumit Attarde
//  Copyright (c) 2015 Grant Mercer
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/make_heap.hpp
/// \page hpx::make_heap
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {
    // clang-format off

    /// Constructs a \a max \a heap in the range [first, last).
    /// Executed according to the policy.
    ///
    /// \note Complexity: at most (3*N) comparisons where
    ///       \a N = distance(first, last).
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution of
    ///                     the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam RndIter     The type of the source iterators used for algorithm.
    ///                     This iterator must meet the requirements for a
    ///                     random access iterator.
    /// \tparam Comp        Comparison function object (i.e. an object that
    ///                     satisfies the requirements of Compare) which returns
    ///                     true if the first argument is less than the second.
    ///                     The signature of the comparison function should be
    ///                     equivalent to the following:
    ///                     \code
    ///                     bool cmp(const Type1 &a, const Type2 &b);
    ///                     \endcode
    ///                     While the signature does not need to have const &,
    ///                     the function must not modify the objects passed to
    ///                     it and must be able to accept all values of type
    ///                     (possibly \a const) \a Type1 and \a Type2 regardless
    ///                     of value category (thus, \a Type1 & is not allowed,
    ///                     nor is \a Type1 unless for \a Type1 a move is
    ///                     equivalent to a copy. The types \a Type1 and \a Type2
    ///                     must be such that an object of type \a RandomIt can
    ///                     be dereferenced and then implicitly converted to both
    ///                     of them.
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of that the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     that the algorithm will be applied to.
    /// \param comp         Refers to the binary predicate which returns true
    ///                     if the first argument should be treated as less than
    ///                     the second. The signature of the function should be
    ///                     equivalent to
    ///                     \code
    ///                     bool comp(const Type &a, const Type &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that objects of
    ///                     types \a RndIter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///
    /// The predicate operations in the parallel \a make_heap algorithm invoked
    /// with an execution policy object of type \a sequential_execution_policy
    /// executes in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a make_heap algorithm invoked
    /// with an execution policy object of type \a parallel_execution_policy
    /// or \a parallel_task_execution_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a make_heap algorithm returns a \a hpx::future<void>
    ///           if the execution policy is of type \a task_execution_policy
    ///           and returns \a void otherwise.
    ///
    template <typename ExPolicy, typename RndIter, typename Comp>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy> make_heap(
        ExPolicy&& policy, RndIter first, RndIter last, Comp&& comp);

    /// Constructs a \a max \a heap in the range [first, last). Uses the
    /// operator \a < for comparisons. Executed according to the policy.
    ///
    /// \note Complexity: at most (3*N) comparisons where
    ///       \a N = distance(first, last).
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution of
    ///                     the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam RndIter     The type of the source iterators used for algorithm.
    ///                     This iterator must meet the requirements for a
    ///                     random access iterator.
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of that the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     that the algorithm will be applied to.
    ///
    /// The predicate operations in the parallel \a make_heap algorithm invoked
    /// with an execution policy object of type \a sequential_execution_policy
    /// executes in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a make_heap algorithm invoked
    /// with an execution policy object of type \a parallel_execution_policy
    /// or \a parallel_task_execution_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a make_heap algorithm returns a \a hpx::future<void>
    ///           if the execution policy is of type \a task_execution_policy
    ///           and returns \a void otherwise.
    ///
    template <typename ExPolicy, typename RndIter>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy>
    make_heap(ExPolicy&& policy, RndIter first, RndIter last);

    /// Constructs a \a max \a heap in the range [first, last).
    ///
    /// \note Complexity: at most (3*N) comparisons where
    ///       \a N = distance(first, last).
    ///
    /// \tparam RndIter     The type of the source iterators used for algorithm.
    ///                     This iterator must meet the requirements for a
    ///                     random access iterator.
    /// \tparam Comp        Comparison function object (i.e. an object that
    ///                     satisfies the requirements of Compare) which returns
    ///                     true if the first argument is less than the second.
    ///                     The signature of the comparison function should be
    ///                     equivalent to the following:
    ///                     \code
    ///                     bool cmp(const Type1 &a, const Type2 &b);
    ///                     \endcode
    ///                     While the signature does not need to have const &,
    ///                     the function must not modify the objects passed to
    ///                     it and must be able to accept all values of type
    ///                     (possibly \a const) \a Type1 and \a Type2 regardless
    ///                     of value category (thus, \a Type1 & is not allowed,
    ///                     nor is \a Type1 unless for \a Type1 a move is
    ///                     equivalent to a copy. The types \a Type1 and \a Type2
    ///                     must be such that an object of type \a RandomIt can
    ///                     be dereferenced and then implicitly converted to both
    ///                     of them.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of that the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     that the algorithm will be applied to.
    /// \param comp         Refers to the binary predicate which returns true
    ///                     if the first argument should be treated as less than
    ///                     the second. The signature of the function should be
    ///                     equivalent to
    ///                     \code
    ///                     bool comp(const Type &a, const Type &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that objects of
    ///                     types \a RndIter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///
    /// \returns  The \a make_heap algorithm returns a \a void.
    ///
    template <typename RndIter, typename Comp>
    void make_heap(RndIter first, RndIter last, Comp&& comp);

    /// Constructs a \a max \a heap in the range [first, last).
    ///
    /// \note Complexity: at most (3*N) comparisons where
    ///       \a N = distance(first, last).
    ///
    /// \tparam RndIter     The type of the source iterators used for algorithm.
    ///                     This iterator must meet the requirements for a
    ///                     random access iterator.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of that the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     that the algorithm will be applied to.
    ///
    /// \returns  The \a make_heap algorithm returns a \a void.
    ///
    template <typename RndIter>
    void make_heap(RndIter first, RndIter last);
    // clang-format on
}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/chunk_size.hpp>
#include <hpx/parallel/util/detail/handle_local_exceptions.hpp>
#include <hpx/parallel/util/detail/scoped_executor_parameters.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <exception>
#include <functional>
#include <iterator>
#include <list>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parallel {

    //////////////////////////////////////////////////////////////////////
    // make_heap
    namespace detail {

        // Perform bottom up heap construction given a range of elements.
        // sift_down_range will take a range from [start,start-count) and
        // apply sift_down to each element in the range
        HPX_CXX_CORE_EXPORT template <typename RndIter, typename Comp,
            typename Proj>
        constexpr void sift_down(RndIter first, Comp&& comp, Proj&& proj,
            typename std::iterator_traits<RndIter>::difference_type len,
            RndIter start)
        {
            typename std::iterator_traits<RndIter>::difference_type child =
                start - first;

            if (len < 2 || (len - 2) / 2 < child)
                return;

            child = 2 * child + 1;
            RndIter child_i = first + child;

            if (child + 1 < len &&
                HPX_INVOKE(comp, HPX_INVOKE(proj, *child_i),
                    HPX_INVOKE(proj, *(child_i + 1))))
            {
                ++child_i;
                ++child;
            }

            if (HPX_INVOKE(
                    comp, HPX_INVOKE(proj, *child_i), HPX_INVOKE(proj, *start)))
            {
                return;
            }

            typename std::iterator_traits<RndIter>::value_type top = *start;
            do
            {
                *start = *child_i;
                start = child_i;

                if ((len - 2) / 2 < child)
                    break;

                child = 2 * child + 1;
                child_i = first + child;

                if (child + 1 < len &&
                    HPX_INVOKE(comp, HPX_INVOKE(proj, *child_i),
                        HPX_INVOKE(proj, *(child_i + 1))))
                {
                    ++child_i;
                    ++child;
                }

            } while (!HPX_INVOKE(
                comp, HPX_INVOKE(proj, *child_i), HPX_INVOKE(proj, top)));

            *start = top;
        }

        HPX_CXX_CORE_EXPORT template <typename RndIter, typename Comp,
            typename Proj>
        constexpr void sift_down_range(RndIter first, Comp&& comp, Proj&& proj,
            typename std::iterator_traits<RndIter>::difference_type len,
            RndIter start, std::size_t count)
        {
            for (std::size_t i = 0; i != count; ++i)
            {
                sift_down(first, comp, proj, len, start - i);
            }
        }

        HPX_CXX_CORE_EXPORT template <typename Iter, typename Sent,
            typename Comp, typename Proj>
        constexpr Iter sequential_make_heap(
            Iter first, Sent last, Comp&& comp, Proj&& proj)
        {
            using difference_type =
                typename std::iterator_traits<Iter>::difference_type;

            difference_type n = last - first;
            if (n > 1)
            {
                for (difference_type start = (n - 2) / 2; start >= 0; --start)
                {
                    sift_down(first, comp, proj, n, first + start);
                }
                return first + n;
            }
            return first;
        }

        //////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iter>
        struct make_heap : public algorithm<make_heap<Iter>, Iter>
        {
            constexpr make_heap() noexcept
              : algorithm<make_heap, Iter>("make_heap")
            {
            }

            template <typename ExPolicy, typename RndIter, typename Sent,
                typename Comp, typename Proj>
            static constexpr RndIter sequential(
                ExPolicy, RndIter first, Sent last, Comp&& comp, Proj&& proj)
            {
                return sequential_make_heap(first, last,
                    HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
            }

            template <typename ExPolicy, typename RndIter, typename Sent,
                typename Comp, typename Proj>
            static util::detail::algorithm_result_t<ExPolicy, RndIter>
            make_heap_thread(ExPolicy&& policy, RndIter first, Sent last,
                Comp&& comp, Proj&& proj)
            {
                typename std::iterator_traits<RndIter>::difference_type n =
                    last - first;
                if (n <= 1)
                {
                    return util::detail::algorithm_result<ExPolicy,
                        RndIter>::get(HPX_MOVE(first));
                }

                using execution_policy = std::decay_t<ExPolicy>;
                using parameters_type =
                    typename execution_policy::executor_parameters_type;
                using executor_type = typename execution_policy::executor_type;

                using scoped_executor_parameters =
                    util::detail::scoped_executor_parameters_ref<
                        parameters_type, executor_type>;

                scoped_executor_parameters scoped_params(
                    policy.parameters(), policy.executor());

                std::list<std::exception_ptr> errors;

                using tuple_type = hpx::tuple<RndIter, std::size_t>;

                auto op = [=](tuple_type const& t) {
                    sift_down_range(first, comp, proj,
                        static_cast<std::size_t>(n), hpx::get<0>(t),
                        hpx::get<1>(t));
                };

                std::size_t const cores =
                    hpx::execution::experimental::processing_units_count(
                        policy.parameters(), policy.executor(),
                        hpx::chrono::null_duration, n);

                // Take a standard chunk size (amount of work / cores), and only
                // take a quarter of that. If our chunk size is too large a LOT
                // of the work will be done sequentially due to the level
                // barrier of heap parallelism. 1/4 of the standard chunk size
                // is an estimate to lower the average number of levels done
                // sequentially
                std::size_t chunk_size =
                    hpx::execution::experimental::get_chunk_size(
                        policy.parameters(), policy.executor(),
                        hpx::chrono::null_duration, cores, n);
                chunk_size /= 4;

                std::size_t max_chunks =
                    hpx::execution::experimental::maximal_number_of_chunks(
                        policy.parameters(), policy.executor(), cores, n);

                util::detail::adjust_chunk_size_and_max_chunks(
                    cores, n, chunk_size, max_chunks);

                try
                {
                    // parent of last node
                    // last node has index (n-1)
                    // parent of x is floor of (x-1)/2
                    std::size_t start = (n - 2) / 2;
                    while (start > 0)
                    {
                        /*
                            helps find number of elements
                            in parent level of start
                            The following are indexes of
                            elements in the heap, not the
                            value of the elements
                                    0
                                1       2
                              3   4   5   6
                            (size of heap) n = 7
                            (parent of last element) start = 2
                            end_exclusive =>
                                last element index in
                                    parent level of start
                        */
                        std::size_t end_exclusive = static_cast<std::size_t>(
                            std::pow(2, std::floor(std::log2(start))));
                        if (end_exclusive >= 2)
                        {
                            end_exclusive -= static_cast<std::size_t>(2);
                        }
                        else
                        {
                            end_exclusive = static_cast<std::size_t>(0);
                        }
                        std::size_t level_items = start - end_exclusive;

                        // If we can't at least run two chunks in parallel,
                        // don't bother parallelizing and simply run
                        // sequentially
                        if (chunk_size * 2 > level_items)
                        {
                            op(hpx::make_tuple(first + start, level_items));
                        }
                        else
                        {
                            std::vector<tuple_type> shapes;
                            shapes.reserve(level_items / chunk_size + 1);

                            std::size_t cnt = 0;
                            while (cnt + chunk_size < level_items)
                            {
                                shapes.push_back(hpx::make_tuple(
                                    first + start - cnt, chunk_size));
                                cnt += chunk_size;
                            }

                            // Schedule any left-over work
                            if (cnt < level_items)
                            {
                                shapes.push_back(hpx::make_tuple(
                                    first + start - cnt, level_items - cnt));
                            }

                            // Reserve items/chunk_size spaces for async calls
                            auto&& workitems = execution::bulk_async_execute(
                                policy.executor(), op, shapes);

                            // Required synchronization per level
                            if (hpx::wait_all_nothrow(workitems))
                            {
                                // collect exceptions
                                util::detail::handle_local_exceptions<
                                    ExPolicy>::call(workitems, errors, false);
                            }
                        }

                        if (!errors.empty())
                            break;    // stop on errors

                        start = end_exclusive;
                    }

                    if (errors.empty())
                    {
                        scoped_params.mark_end_of_scheduling();

                        // Perform sift down for the head node
                        sift_down(first, comp = HPX_FORWARD(Comp, comp),
                            HPX_FORWARD(Proj, proj), n, first);
                    }
                }
                catch ([[maybe_unused]] std::bad_alloc const& e)
                {
                    if constexpr (hpx::is_async_execution_policy_v<ExPolicy>)
                    {
                        return hpx::make_exceptional_future<RndIter>(e);
                    }
                    else
                    {
                        throw;
                    }
                }
                catch (...)
                {
                    util::detail::handle_local_exceptions<ExPolicy>::call(
                        std::current_exception(), errors);
                }

                // rethrow exceptions, if any
                if (!errors.empty())
                {
                    if constexpr (hpx::is_async_execution_policy_v<ExPolicy>)
                    {
                        return hpx::make_exceptional_future<RndIter>(
                            hpx::exception_list(HPX_MOVE(errors)));
                    }
                    else
                    {
                        util::detail::handle_local_exceptions<ExPolicy>::call(
                            errors);
                    }
                }

                std::advance(first, n);
                return util::detail::algorithm_result<ExPolicy, RndIter>::get(
                    HPX_MOVE(first));
            }

            template <typename ExPolicy, typename RndIter, typename Sent,
                typename Comp, typename Proj>
            static util::detail::algorithm_result_t<ExPolicy, RndIter> parallel(
                ExPolicy&& policy, RndIter first, Sent last, Comp&& comp,
                Proj&& proj)
            {
                return make_heap_thread(HPX_FORWARD(ExPolicy, policy), first,
                    last, HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
            }

            template <typename RndIter, typename Sent, typename Comp,
                typename Proj>
            static typename util::detail::algorithm_result<
                hpx::execution::parallel_task_policy, RndIter>::type
            parallel(hpx::execution::parallel_task_policy policy, RndIter first,
                Sent last, Comp&& comp, Proj&& proj)
            {
                return execution::async_execute(policy.executor(),
                    [=, comp = HPX_FORWARD(Comp, comp),
                        proj = HPX_FORWARD(Proj, proj)]() mutable {
                        return make_heap_thread(policy, first, last,
                            HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
                    });
            }
        };
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::make_heap
    HPX_CXX_CORE_EXPORT inline constexpr struct make_heap_t final
      : hpx::detail::tag_parallel_algorithm<make_heap_t>
    {
    private:
        template <typename ExPolicy, typename RndIter, typename Comp>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<RndIter> &&
                hpx::is_invocable_v<Comp,
                    typename std::iterator_traits<RndIter>::value_type,
                    typename std::iterator_traits<RndIter>::value_type
                >
            )
        // clang-format on
        friend hpx::parallel::util::detail::algorithm_result_t<ExPolicy>
        tag_fallback_invoke(make_heap_t, ExPolicy&& policy, RndIter first,
            RndIter last, Comp comp)
        {
            static_assert(std::random_access_iterator<RndIter>,
                "Requires random access iterator.");

            return hpx::parallel::util::detail::algorithm_result<ExPolicy>::get(
                hpx::parallel::detail::make_heap<RndIter>().call(
                    HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(comp),
                    hpx::identity_v));
        }

        template <typename ExPolicy, typename RndIter>
        // clang-format off
        requires (
            hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<RndIter>
        )
        // clang-format on
        friend hpx::parallel::util::detail::algorithm_result_t<ExPolicy>
        tag_fallback_invoke(
            make_heap_t, ExPolicy&& policy, RndIter first, RndIter last)
        {
            static_assert(std::random_access_iterator<RndIter>,
                "Requires random access iterator.");

            using value_type =
                typename std::iterator_traits<RndIter>::value_type;

            return hpx::parallel::util::detail::algorithm_result<ExPolicy>::get(
                hpx::parallel::detail::make_heap<RndIter>().call(
                    HPX_FORWARD(ExPolicy, policy), first, last,
                    std::less<value_type>(), hpx::identity_v));
        }

        template <typename RndIter, typename Comp>
        // clang-format off
        requires (
            hpx::traits::is_iterator_v<RndIter> &&
            hpx::is_invocable_v<Comp,
                hpx::traits::iter_value_t<RndIter>,
                hpx::traits::iter_value_t<RndIter>
            >
        )
        // clang-format on
        friend void tag_fallback_invoke(
            make_heap_t, RndIter first, RndIter last, Comp comp)
        {
            static_assert(std::random_access_iterator<RndIter>,
                "Requires random access iterator.");

            hpx::parallel::detail::make_heap<RndIter>().call(
                hpx::execution::seq, first, last, HPX_MOVE(comp),
                hpx::identity_v);
        }

        template <typename RndIter>
            requires(hpx::traits::is_iterator_v<RndIter>)
        friend void tag_fallback_invoke(
            make_heap_t, RndIter first, RndIter last)
        {
            static_assert(std::random_access_iterator<RndIter>,
                "Requires random access iterator.");

            using value_type =
                typename std::iterator_traits<RndIter>::value_type;

            hpx::parallel::detail::make_heap<RndIter>().call(
                hpx::execution::seq, first, last, std::less<value_type>(),
                hpx::identity_v);
        }
    } make_heap{};
}    // namespace hpx

#endif    // DOXYGEN
