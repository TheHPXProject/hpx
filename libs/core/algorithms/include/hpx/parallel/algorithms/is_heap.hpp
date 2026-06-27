//  Copyright (c) 2017 Taeguk Kwon
//  Copyright (c) 2020-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \page hpx::is_heap, hpx::is_heap_until
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {
    // clang-format off

    /// Returns whether the range is max heap. That is, true if the range is
    /// max heap, false otherwise. The function uses the given comparison
    /// function object \a comp (defaults to using operator<()).
    /// Executed according to the policy.
    ///
    /// \note   Complexity:
    ///         Linear in the distance between \a first and \a last.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam RandIter    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     random access iterator.
    /// \tparam Comp        The type of the function/function object to use
    ///                     (deduced).
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param comp         \a comp is a callable object. The return value of the
    ///                     INVOKE operation applied to an object of type \a Comp,
    ///                     when contextually converted to bool, yields true if
    ///                     the first argument of the call is less than the
    ///                     second, and false otherwise. It is assumed that comp
    ///                     will not apply any non-constant function through the
    ///                     dereferenced iterator.
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
    /// \returns  The \a is_heap algorithm returns a \a hpx::future<bool>
    ///           if the execution policy is of type \a sequenced_task_policy or
    ///           \a parallel_task_policy and returns \a bool otherwise.
    ///           The \a is_heap algorithm returns whether the range is max heap.
    ///           That is, true if the range is max heap, false otherwise.
    ///
    template <typename ExPolicy, typename RandIter,
        typename Comp = hpx::parallel::detail::less>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, bool>
    is_heap(ExPolicy&& policy, RandIter first, RandIter last,
        Comp&& comp = Comp());

    /// Returns whether the range is max heap. That is, true if the range is
    /// max heap, false otherwise. The function uses the given comparison
    /// function object \a comp (defaults to using operator<()).
    ///
    /// \note   Complexity:
    ///         Linear in the distance between \a first and \a last.
    ///
    /// \tparam RandIter    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     random access iterator.
    /// \tparam Comp        The type of the function/function object to use
    ///                     (deduced).
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param comp         \a comp is a callable object. The return value of the
    ///                     INVOKE operation applied to an object of type \a Comp,
    ///                     when contextually converted to bool, yields true if
    ///                     the first argument of the call is less than the
    ///                     second, and false otherwise. It is assumed that comp
    ///                     will not apply any non-constant function through the
    ///                     dereferenced iterator.
    ///
    /// \a comp has to induce a strict weak ordering on the values.
    ///
    /// \returns  The \a is_heap a \a bool.
    ///           The \a is_heap algorithm returns whether the range is max heap.
    ///           That is, true if the range is max heap, false otherwise.
    ///
    template <typename RandIter, typename Comp = hpx::parallel::detail::less>
    bool is_heap(RandIter first, RandIter last, Comp&& comp = Comp());

    /// Returns the upper bound of the largest range beginning at \a first
    /// which is a max heap. That is, the last iterator \a it for
    /// which range [first, it) is a max heap. The function
    /// uses the given comparison function object \a comp (defaults to using
    /// operator<()). Executed according to the policy.
    ///
    /// \note   Complexity:
    ///         Linear in the distance between \a first and \a last.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam RandIter    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     random access iterator.
    /// \tparam Comp        The type of the function/function object to use
    ///                     (deduced).
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param comp         \a comp is a callable object. The return value of the
    ///                     INVOKE operation applied to an object of type \a Comp,
    ///                     when contextually converted to bool, yields true if
    ///                     the first argument of the call is less than the
    ///                     second, and false otherwise. It is assumed that comp
    ///                     will not apply any non-constant function through the
    ///                     dereferenced iterator.
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
    /// \returns  The \a is_heap_until algorithm returns a \a hpx::future<RandIter>
    ///           if the execution policy is of type \a sequenced_task_policy or
    ///           \a parallel_task_policy and returns \a RandIter otherwise.
    ///           The \a is_heap_until algorithm returns the upper bound
    ///           of the largest range beginning at first which is a max heap.
    ///           That is, the last iterator \a it for which range [first, it)
    ///           is a max heap.
    ///
    template <typename ExPolicy, typename RandIter,
        typename Comp = hpx::parallel::detail::less>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, RandIter>
    is_heap_until(ExPolicy&& policy, RandIter first, RandIter last,
        Comp&& comp = Comp());

    /// Returns the upper bound of the largest range beginning at \a first
    /// which is a max heap. That is, the last iterator \a it for
    /// which range [first, it) is a max heap. The function
    /// uses the given comparison function object \a comp (defaults to using
    /// operator<()).
    ///
    /// \note   Complexity:
    ///         Linear in the distance between \a first and \a last.
    ///
    /// \tparam RandIter    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     random access iterator.
    /// \tparam Comp        The type of the function/function object to use
    ///                     (deduced).
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param comp         \a comp is a callable object. The return value of the
    ///                     INVOKE operation applied to an object of type \a Comp,
    ///                     when contextually converted to bool, yields true if
    ///                     the first argument of the call is less than the
    ///                     second, and false otherwise. It is assumed that comp
    ///                     will not apply any non-constant function through the
    ///                     dereferenced iterator.
    ///
    /// \a comp has to induce a strict weak ordering on the values.
    ///
    /// \returns  The \a is_heap_until algorithm returns a \a RandIter.
    ///           The \a is_heap_until algorithm returns the upper bound
    ///           of the largest range beginning at first which is a max heap.
    ///           That is, the last iterator \a it for which range [first, it)
    ///           is a max heap.
    ///
    template <typename RandIter, typename Comp = hpx::parallel::detail::less>
    RandIter is_heap_until(RandIter first, RandIter last, Comp&& comp = Comp());

    // clang-format on
}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/algorithms/traits/projected.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/util/adapt_placement_mode.hpp>
#include <hpx/parallel/util/cancellation_token.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/clear_container.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/partitioner.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // is_heap
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename Iter, typename Sent,
            typename Comp, typename Proj>
        constexpr Iter sequential_is_heap_until(
            Iter first, Sent last, Comp&& comp, Proj&& proj)
        {
            Iter child = first;
            if (child == last)
            {
                return child;
            }

            while (true)
            {
                ++child;
                if (child == last ||
                    HPX_INVOKE(comp, HPX_INVOKE(proj, *first),
                        HPX_INVOKE(proj, *child)))
                {
                    break;
                }

                ++child;
                if (child == last ||
                    HPX_INVOKE(comp, HPX_INVOKE(proj, *first),
                        HPX_INVOKE(proj, *child)))
                {
                    break;
                }

                ++first;
            }

            return child;
        }

        HPX_CXX_CORE_EXPORT template <typename Iter, typename Sent,
            typename Comp>
        constexpr Iter sequential_is_heap_until(
            Iter first, Sent last, Comp&& comp, hpx::identity)
        {
            Iter child = first;
            if (child == last)
            {
                return child;
            }

            while (true)
            {
                ++child;
                if (child == last || HPX_INVOKE(comp, *first, *child))
                {
                    break;
                }

                ++child;
                if (child == last || HPX_INVOKE(comp, *first, *child))
                {
                    break;
                }

                ++first;
            }

            return child;
        }

        // sequential is_heap with projection function
        HPX_CXX_CORE_EXPORT template <typename Iter, typename Sent,
            typename Comp, typename Proj>
        constexpr bool sequential_is_heap(
            Iter first, Sent last, Comp&& comp, Proj&& proj)
        {
            return sequential_is_heap_until(first, last,
                       HPX_FORWARD(Comp, comp),
                       HPX_FORWARD(Proj, proj)) == last;
        }

        struct is_heap_helper
        {
            template <typename ExPolicy, typename Iter, typename Sent,
                typename Comp, typename Proj>
            decltype(auto) operator()(ExPolicy&& org_policy, Iter first,
                Sent last, Comp&& comp, Proj&& proj)
            {
                using result = util::detail::algorithm_result<ExPolicy, bool>;
                using type = std::iterator_traits<Iter>::value_type;
                using difference_type =
                    std::iterator_traits<Iter>::difference_type;
                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                difference_type count = detail::distance(first, last);

                if constexpr (has_scheduler_executor)
                {
                    if (count == 0)
                    {
                        // underflow prevention
                        ++count;
                    }
                }
                else
                {
                    if (count <= 1)
                    {
                        return result::get(true);
                    }
                }

                Iter second = first + 1;
                --count;

                decltype(auto) policy =
                    hpx::execution::experimental::adapt_placement_mode(
                        HPX_FORWARD(ExPolicy, org_policy),
                        hpx::threads::thread_placement_hint::breadth_first);

                using policy_type = std::decay_t<decltype(policy)>;

                hpx::parallel::util::cancellation_token<std::size_t> tok(count);

                // Note: replacing the invoke() with HPX_INVOKE()
                // below makes gcc generate errors
                auto f1 = [tok, first, comp = HPX_FORWARD(Comp, comp),
                              proj = HPX_FORWARD(Proj, proj)](Iter it,
                              std::size_t part_size,
                              std::size_t base_idx) mutable -> void {
                    bool cancelled = false;
                    util::loop_idx_n<policy_type>(base_idx, it, part_size, tok,
                        [&cancelled, first, &comp, &proj](type const& v,
                            std::size_t const i) mutable -> void {
                            if constexpr (std::is_same_v<hpx::identity,
                                              std::decay_t<Proj>>)
                            {
                                HPX_UNUSED(proj);
                                if (!cancelled &&
                                    hpx::invoke(comp, *(first + i / 2), v))
                                {
                                    cancelled = true;
                                }
                            }
                            else
                            {
                                if (!cancelled &&
                                    hpx::invoke(comp,
                                        hpx::invoke(proj, *(first + i / 2)),
                                        hpx::invoke(proj, v)))
                                {
                                    cancelled = true;
                                }
                            }
                        });
                    if (cancelled)
                    {
                        tok.cancel(0);
                    }
                };

                auto f2 = [tok, count](auto&&... data) mutable -> bool {
                    static_assert(sizeof...(data) < 2);

                    // make sure iterators embedded in function object that
                    // is attached to futures are invalidated
                    util::detail::clear_container(data...);

                    difference_type find_res =
                        static_cast<difference_type>(tok.get_data());

                    return find_res == count;
                };

                using partitioner_type =
                    util::partitioner<policy_type, bool, void>;
                return partitioner_type::call_with_index(
                    HPX_FORWARD(decltype(policy), policy), second, count, 1,
                    HPX_MOVE(f1), HPX_MOVE(f2));
            }
        };

        HPX_CXX_CORE_EXPORT template <typename RandIter>
        struct is_heap : algorithm<is_heap<RandIter>, bool>
        {
            constexpr is_heap() noexcept
              : algorithm<is_heap, bool>("is_heap")
            {
            }

            template <typename ExPolicy, typename Iter, typename Sent,
                typename Comp, typename Proj>
            static constexpr bool sequential(
                ExPolicy&&, Iter first, Sent last, Comp&& comp, Proj&& proj)
            {
                return sequential_is_heap(first, last, HPX_FORWARD(Comp, comp),
                    HPX_FORWARD(Proj, proj));
            }

            template <typename ExPolicy, typename Iter, typename Sent,
                typename Comp, typename Proj>
            static decltype(auto) parallel(ExPolicy&& policy, Iter first,
                Sent last, Comp&& comp, Proj&& proj)
            {
                return is_heap_helper()(HPX_FORWARD(ExPolicy, policy), first,
                    last, HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // is_heap_until
    namespace detail {

        struct is_heap_until_helper
        {
            template <typename ExPolicy, typename Iter, typename Sent,
                typename Comp, typename Proj>
            decltype(auto) operator()(ExPolicy&& org_policy, Iter first,
                Sent last, Comp comp, Proj proj)
            {
                using result = util::detail::algorithm_result<ExPolicy, Iter>;
                using type = std::iterator_traits<Iter>::value_type;
                using difference_type =
                    std::iterator_traits<Iter>::difference_type;
                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                difference_type count = detail::distance(first, last);

                Iter second;
                if (count <= 1)
                {
                    if constexpr (has_scheduler_executor)
                    {
                        count = static_cast<difference_type>(0);
                        second = last;
                    }
                    else
                    {
                        return result::get(HPX_MOVE(last));
                    }
                }
                else
                {
                    second = first + 1;
                    --count;
                }

                decltype(auto) policy =
                    hpx::execution::experimental::adapt_placement_mode(
                        HPX_FORWARD(ExPolicy, org_policy),
                        hpx::threads::thread_placement_hint::breadth_first);

                using policy_type = std::decay_t<decltype(policy)>;

                hpx::parallel::util::cancellation_token<std::size_t> tok(count);

                // Note: replacing the invoke() with HPX_INVOKE()
                // below makes gcc generate errors
                auto f1 = [tok, first, comp = HPX_FORWARD(Comp, comp),
                              proj = HPX_FORWARD(Proj, proj)](Iter it,
                              std::size_t part_size,
                              std::size_t base_idx) mutable {
                    std::size_t cancelled = static_cast<std::size_t>(-1);
                    util::loop_idx_n<policy_type>(base_idx, it, part_size, tok,
                        [&cancelled, first, &comp, &proj](
                            type const& v, std::size_t const i) -> void {
                            if constexpr (std::is_same_v<hpx::identity,
                                              std::decay_t<Proj>>)
                            {
                                HPX_UNUSED(proj);
                                if (cancelled == static_cast<std::size_t>(-1))
                                {
                                    if (hpx::invoke(comp, *(first + i / 2), v))
                                    {
                                        cancelled = i;
                                    }
                                }
                            }
                            else
                            {
                                if (cancelled == static_cast<std::size_t>(-1))
                                {
                                    if (hpx::invoke(comp,
                                            hpx::invoke(proj, *(first + i / 2)),
                                            hpx::invoke(proj, v)))
                                    {
                                        cancelled = i;
                                    }
                                }
                            }
                        });
                    if (cancelled != static_cast<std::size_t>(-1))
                    {
                        tok.cancel(cancelled);
                    }
                };

                auto f2 = [tok, second](auto&&... data) mutable -> Iter {
                    static_assert(sizeof...(data) < 2);

                    // make sure iterators embedded in function object that is
                    // attached to futures are invalidated
                    util::detail::clear_container(data...);

                    difference_type find_res =
                        static_cast<difference_type>(tok.get_data());

                    std::advance(second, find_res);

                    return second;
                };

                using partitioner_type =
                    util::partitioner<policy_type, Iter, void>;
                return partitioner_type::call_with_index(
                    HPX_FORWARD(decltype(policy), policy), second, count, 1,
                    HPX_MOVE(f1), HPX_MOVE(f2));
            }
        };

        HPX_CXX_CORE_EXPORT template <typename RandIter>
        struct is_heap_until : algorithm<is_heap_until<RandIter>, RandIter>
        {
            constexpr is_heap_until() noexcept
              : algorithm<is_heap_until, RandIter>("is_heap_until")
            {
            }

            template <typename ExPolicy, typename Iter, typename Sent,
                typename Comp, typename Proj>
            static constexpr Iter sequential(
                ExPolicy&&, Iter first, Sent last, Comp&& comp, Proj&& proj)
            {
                return sequential_is_heap_until(first, last,
                    HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
            }

            template <typename ExPolicy, typename Iter, typename Sent,
                typename Comp, typename Proj>
            static decltype(auto) parallel(ExPolicy&& policy, Iter first,
                Sent last, Comp&& comp, Proj&& proj)
            {
                return is_heap_until_helper()(HPX_FORWARD(ExPolicy, policy),
                    first, last, HPX_FORWARD(Comp, comp),
                    HPX_FORWARD(Proj, proj));
            }
        };
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::is_heap
    HPX_CXX_CORE_EXPORT inline constexpr struct is_heap_t final
      : hpx::detail::tag_parallel_algorithm<is_heap_t>
    {
    private:
        template <typename ExPolicy, typename RandIter,
            typename Comp = hpx::parallel::detail::less>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<RandIter> &&
                hpx::is_invocable_v<Comp,
                    typename std::iterator_traits<RandIter>::value_type,
                    typename std::iterator_traits<RandIter>::value_type
                >
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(is_heap_t, ExPolicy&& policy,
            RandIter first, RandIter last, Comp comp = Comp())
        {
            static_assert(std::random_access_iterator<RandIter>,
                "Requires a random access iterator.");

            return hpx::parallel::detail::is_heap<RandIter>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(comp),
                hpx::identity_v);
        }

        template <typename RandIter,
            typename Comp = hpx::parallel::detail::less>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<RandIter> &&
                hpx::is_invocable_v<Comp,
                    typename std::iterator_traits<RandIter>::value_type,
                    typename std::iterator_traits<RandIter>::value_type
                >
            )
        // clang-format on
        friend bool tag_fallback_invoke(
            is_heap_t, RandIter first, RandIter last, Comp comp = Comp())
        {
            static_assert(std::random_access_iterator<RandIter>,
                "Requires a random access iterator.");

            return hpx::parallel::detail::is_heap<RandIter>().call(
                hpx::execution::seq, first, last, HPX_MOVE(comp),
                hpx::identity_v);
        }
    } is_heap{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::is_heap_until
    HPX_CXX_CORE_EXPORT inline constexpr struct is_heap_until_t final
      : hpx::detail::tag_parallel_algorithm<is_heap_until_t>
    {
    private:
        template <typename ExPolicy, typename RandIter,
            typename Comp = hpx::parallel::detail::less>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<RandIter> &&
                hpx::is_invocable_v<Comp,
                    typename std::iterator_traits<RandIter>::value_type,
                    typename std::iterator_traits<RandIter>::value_type
                >
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(is_heap_until_t,
            ExPolicy&& policy, RandIter first, RandIter last,
            Comp comp = Comp())
        {
            static_assert(std::random_access_iterator<RandIter>,
                "Requires a random access iterator.");

            return hpx::parallel::detail::is_heap_until<RandIter>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, HPX_MOVE(comp),
                hpx::identity_v);
        }

        template <typename RandIter,
            typename Comp = hpx::parallel::detail::less>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<RandIter> &&
                hpx::is_invocable_v<Comp,
                    typename std::iterator_traits<RandIter>::value_type,
                    typename std::iterator_traits<RandIter>::value_type
                >
            )
        // clang-format on
        friend RandIter tag_fallback_invoke(
            is_heap_until_t, RandIter first, RandIter last, Comp comp = Comp())
        {
            static_assert(std::random_access_iterator<RandIter>,
                "Requires a random access iterator.");

            return hpx::parallel::detail::is_heap_until<RandIter>().call(
                hpx::execution::seq, first, last, HPX_MOVE(comp),
                hpx::identity_v);
        }
    } is_heap_until{};
}    // namespace hpx

#endif    // DOXYGEN
