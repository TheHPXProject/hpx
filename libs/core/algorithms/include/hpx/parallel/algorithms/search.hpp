//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2014 Grant Mercer
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/search.hpp
/// \page hpx::search, hpx::search_n
/// \headerfile hpx/algorithm.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/config/export_definitions.hpp>
#include <hpx/config/move.hpp>
#include <hpx/execution/algorithms/detail/predicates.hpp>
#include <hpx/execution/traits/is_execution_policy.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/algorithms/detail/search.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/type_support/identity.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>

#if defined(DOXYGEN)

namespace hpx {

    /// Searches the range [first, last) for any elements in the range [s_first, s_last).
    /// Uses a provided predicate to compare elements.
    ///
    /// \note   Complexity: at most (S*N) comparisons where
    ///         \a S = distance(s_first, s_last) and
    ///         \a N = distance(first, last).
    ///
    /// \tparam FwdIter     The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a search requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to \a std::equal_to<>
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param s_first      Refers to the beginning of the sequence of elements
    ///                     the algorithm will be searching for.
    /// \param s_last       Refers to the end of the sequence of elements of
    ///                     the algorithm will be searching for.
    /// \param op           Refers to the binary predicate which returns true if the
    ///                     elements should be treated as equal. the signature of
    ///                     the function should be equivalent to
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such
    ///                     that objects of types \a FwdIter1 and \a FwdIter2 can
    ///                     be dereferenced and then implicitly converted to
    ///                     \a Type1 and \a Type2 respectively
    ///
    /// The comparison operations in the parallel \a search algorithm execute
    /// in sequential order in the calling thread.
    ///
    /// \returns  The \a search algorithm returns a \a hpx::future<FwdIter> if the
    ///           execution policy is of type \a task_execution_policy and
    ///           returns \a FwdIter otherwise.
    ///           The \a search algorithm returns an iterator to the beginning of
    ///           the first subsequence [s_first, s_last) in range [first, last).
    ///           If the length of the subsequence [s_first, s_last) is greater
    ///           than the length of the range [first, last), \a last is returned.
    ///           Additionally if the size of the subsequence is empty \a first is
    ///           returned. If no subsequence is found, \a last is returned.
    ///
    template <typename FwdIter, typename FwdIter2,
        typename Pred = parallel::detail::equal_to>
    FwdIter search(FwdIter first, FwdIter last, FwdIter2 s_first,
        FwdIter2 s_last, Pred&& op = Pred());

    /// Searches the range [first, last) for any elements in the range [s_first, s_last).
    /// Uses a provided predicate to compare elements. Executed according to the policy.
    ///
    /// \note   Complexity: at most (S*N) comparisons where
    ///         \a S = distance(s_first, s_last) and
    ///         \a N = distance(first, last).
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a search requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to \a std::equal_to<>.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param s_first      Refers to the beginning of the sequence of elements
    ///                     the algorithm will be searching for.
    /// \param s_last       Refers to the end of the sequence of elements of
    ///                     the algorithm will be searching for.
    /// \param op           Refers to the binary predicate which returns true if the
    ///                     elements should be treated as equal. the signature of
    ///                     the function should be equivalent to
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such
    ///                     that objects of types \a FwdIter1 and \a FwdIter2 can
    ///                     be dereferenced and then implicitly converted to
    ///                     \a Type1 and \a Type2 respectively
    ///
    /// The comparison operations in the parallel \a search algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a search algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a search algorithm returns a \a hpx::future<FwdIter> if the
    ///           execution policy is of type \a task_execution_policy and
    ///           returns \a FwdIter otherwise.
    ///           The \a search algorithm returns an iterator to the beginning of
    ///           the first subsequence [s_first, s_last) in range [first, last).
    ///           If the length of the subsequence [s_first, s_last) is greater
    ///           than the length of the range [first, last), \a last is returned.
    ///           Additionally if the size of the subsequence is empty \a first is
    ///           returned. If no subsequence is found, \a last is returned.
    ///
    template <typename ExPolicy, typename FwdIter, typename FwdIter2,
        typename Pred = parallel::detail::equal_to>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter> search(
        ExPolicy&& policy, FwdIter first, FwdIter last, FwdIter2 s_first,
        FwdIter2 s_last, Pred&& op = Pred());

    /// Searches the range [first, last) using a searcher object.
    ///
    /// \tparam FwdIter   The type of the source iterators (deduced).
    ///                   This iterator type must meet the requirements of a
    ///                   forward iterator.
    /// \tparam Searcher  A callable searcher object meeting the C++ Searcher
    ///                   requirements (e.g. std::boyer_moore_searcher).
    ///
    /// \param first      Refers to the beginning of the sequence to search.
    /// \param last       Refers to the end of the sequence to search.
    /// \param searcher   A callable object performing the search.
    ///
    /// \returns An iterator to the beginning of the found subsequence, or
    ///          \a last if no subsequence is found.
    ///
    template <typename FwdIter, typename Searcher>
    FwdIter search(FwdIter first, FwdIter last, Searcher&& searcher);

    /// Searches the range [first, last) using a searcher object.
    /// Executed according to the policy.
    ///
    /// \tparam ExPolicy  The type of the execution policy to use (deduced).
    /// \tparam FwdIter   The type of the source iterators (deduced).
    /// \tparam Searcher  A callable searcher object meeting the C++ Searcher
    ///                   requirements.
    ///
    /// \param policy    The execution policy to use.
    /// \param first     Refers to the beginning of the sequence to search.
    /// \param last      Refers to the end of the sequence to search.
    /// \param searcher  A callable object performing the search.
    ///
    /// \returns  The algorithm returns a \a hpx::future<FwdIter> if the
    ///           execution policy is a task policy, or \a FwdIter otherwise.
    ///
    template <typename ExPolicy, typename FwdIter, typename Searcher>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter> search(
        ExPolicy&& policy, FwdIter first, FwdIter last, Searcher&& searcher);

    /// Searches the range [first, last) for any elements in the range [s_first, s_last).
    /// Uses a provided predicate to compare elements.
    ///
    /// \note   Complexity: at most (S*N) comparisons where
    ///         \a S = distance(s_first, s_last) and
    ///         \a N = count.
    ///
    /// \tparam FwdIter     The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a search_n requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to \a std::equal_to<>
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param count        Refers to the range of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param s_first      Refers to the beginning of the sequence of elements
    ///                     the algorithm will be searching for.
    /// \param s_last       Refers to the end of the sequence of elements of
    ///                     the algorithm will be searching for.
    /// \param op           Refers to the binary predicate which returns true if the
    ///                     elements should be treated as equal. the signature of
    ///                     the function should be equivalent to
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such
    ///                     that objects of types \a FwdIter1 and \a FwdIter2 can
    ///                     be dereferenced and then implicitly converted to
    ///                     \a Type1 and \a Type2 respectively
    ///
    /// The comparison operations in the parallel \a search_n algorithm execute
    /// in sequential order in the calling thread.
    ///
    /// \returns  The \a search_n algorithm returns \a FwdIter.
    ///           The \a search_n algorithm returns an iterator to the beginning of
    ///           the last subsequence [s_first, s_last) in range [first, first+count).
    ///           If the length of the subsequence [s_first, s_last) is greater
    ///           than the length of the range [first, first+count),
    ///           \a first is returned.
    ///           Additionally if the size of the subsequence is empty or no subsequence
    ///           is found, \a first is also returned.
    ///
    template <typename FwdIter, typename FwdIter2,
        typename Pred = parallel::detail::equal_to>
    FwdIter search_n(FwdIter first, std::size_t count, FwdIter2 s_first,
        FwdIter2 s_last, Pred&& op = Pred());

    /// Searches the range [first, last) for any elements in the range [s_first, s_last).
    /// Uses a provided predicate to compare elements. Executed according to the policy.
    ///
    /// \note   Complexity: at most (S*N) comparisons where
    ///         \a S = distance(s_first, s_last) and
    ///         \a N = count.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter     The type of the source iterators used for the
    ///                     first range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     second range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Pred        The type of an optional function/function object to use.
    ///                     Unlike its sequential form, the parallel
    ///                     overload of \a search_n requires \a Pred to meet the
    ///                     requirements of \a CopyConstructible. This defaults
    ///                     to \a std::equal_to<>
    ///
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the first range the algorithm will be applied to.
    /// \param count        Refers to the range of elements of
    ///                     the first range the algorithm will be applied to.
    /// \param s_first      Refers to the beginning of the sequence of elements
    ///                     the algorithm will be searching for.
    /// \param s_last       Refers to the end of the sequence of elements of
    ///                     the algorithm will be searching for.
    /// \param op           Refers to the binary predicate which returns true if the
    ///                     elements should be treated as equal. the signature of
    ///                     the function should be equivalent to
    ///                     \code
    ///                     bool pred(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1 and \a Type2 must be such
    ///                     that objects of types \a FwdIter1 and \a FwdIter2 can
    ///                     be dereferenced and then implicitly converted to
    ///                     \a Type1 and \a Type2 respectively
    ///
    /// The comparison operations in the parallel \a search_n algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The comparison operations in the parallel \a search_n algorithm invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a search_n algorithm returns a \a hpx::future<FwdIter> if the
    ///           execution policy is of type \a task_execution_policy and
    ///           returns \a FwdIter otherwise.
    ///           The \a search_n algorithm returns an iterator to the beginning of
    ///           the last subsequence [s_first, s_last) in range [first, first+count).
    ///           If the length of the subsequence [s_first, s_last) is greater
    ///           than the length of the range [first, first+count),
    ///           \a first is returned.
    ///           Additionally if the size of the subsequence is empty or no subsequence
    ///           is found, \a first is also returned.
    ///
    template <typename ExPolicy, typename FwdIter, typename FwdIter2,
        typename Pred = parallel::detail::equal_to>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter> search_n(
        ExPolicy&& policy, FwdIter first, std::size_t count, FwdIter2 s_first,
        FwdIter2 s_last, Pred&& op = Pred());
}    // namespace hpx

#else

namespace hpx {

    HPX_CXX_CORE_EXPORT inline constexpr struct search_t final
      : hpx::detail::tag_dispatch<search_t,
            hpx::detail::tag_parallel_algorithm<search_t>>
    {
        template <typename FwdIter, typename FwdIter2,
            typename Pred = parallel::detail::equal_to>
        // clang-format off
            requires (
                std::forward_iterator<FwdIter> &&
                std::forward_iterator<FwdIter2> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits <FwdIter>::value_type,
                    typename std::iterator_traits <FwdIter2>::value_type
                >
            )
        // clang-format on
        static FwdIter invoke_default(FwdIter first, FwdIter last,
            FwdIter2 s_first, FwdIter2 s_last, Pred op = Pred())
        {
            return hpx::parallel::detail::search<FwdIter, FwdIter>().call(
                hpx::execution::seq, first, last, s_first, s_last, HPX_MOVE(op),
                hpx::identity_v, hpx::identity_v);
        }

        template <typename ExPolicy, typename FwdIter, typename FwdIter2,
            typename Pred = parallel::detail::equal_to>
        // clang-format off
            requires (
                is_execution_policy<ExPolicy>::value &&
                std::forward_iterator<FwdIter> &&
                std::forward_iterator<FwdIter2> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits <FwdIter>::value_type,
                    typename std::iterator_traits <FwdIter2>::value_type
                >
            )
        // clang-format on
        static decltype(auto) invoke_default(ExPolicy&& policy, FwdIter first,
            FwdIter last, FwdIter2 s_first, FwdIter2 s_last, Pred op = Pred())
        {
            return hpx::parallel::detail::search<FwdIter, FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, s_first, s_last,
                HPX_MOVE(op), hpx::identity_v, hpx::identity_v);
        }

        template <typename FwdIter, typename Searcher>
        // clang-format off
        requires (
            std::forward_iterator<FwdIter> &&
            std::is_invocable_v<Searcher&, FwdIter, FwdIter>)
        // clang-format on
        static FwdIter invoke_default(
            FwdIter first, FwdIter last, Searcher&& searcher)
        {
            auto result = HPX_FORWARD(Searcher, searcher)(first, last);
            return result.first;
        }

        template <typename ExPolicy, typename FwdIter, typename Searcher>
        // clang-format off
        requires (
            is_execution_policy<ExPolicy>::value &&
            std::forward_iterator<FwdIter> &&
            std::is_invocable_v<Searcher&, FwdIter, FwdIter>)
        // clang-format on
        static FwdIter invoke_default(
            ExPolicy&&, FwdIter first, FwdIter last, Searcher&& searcher)
        {
            auto result = HPX_FORWARD(Searcher, searcher)(first, last);
            return result.first;
        }
    } search{};

    HPX_CXX_CORE_EXPORT inline constexpr struct search_n_t final
      : hpx::detail::tag_dispatch<search_n_t,
            hpx::detail::tag_parallel_algorithm<search_n_t>>
    {
        template <typename FwdIter, typename Size, typename T,
            typename Pred = parallel::detail::equal_to,
            typename Proj = hpx::identity>
        // clang-format off
            requires(std::forward_iterator<FwdIter> &&
                std::is_integral_v<Size> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<FwdIter>::value_type, T>)
        // clang-format on
        static FwdIter invoke_default(FwdIter first, FwdIter last, Size count,
            T const& value, Pred pred = Pred(), Proj proj = Proj())
        {
            return hpx::parallel::detail::search_n<FwdIter, FwdIter>().call(
                hpx::execution::seq, first, last, count, value, HPX_MOVE(pred),
                HPX_MOVE(proj));
        }    // namespace hpx

        template <typename ExPolicy, typename FwdIter, typename Size,
            typename T, typename Pred = parallel::detail::equal_to,
            typename Proj = hpx::identity>
        // clang-format off
            requires(is_execution_policy<ExPolicy>::value &&
                std::forward_iterator<FwdIter> &&
                std::is_integral_v<Size> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<FwdIter>::value_type, T>)
        // clang-format on
        static typename parallel::util::detail::algorithm_result<ExPolicy,
            FwdIter>::type
        invoke_default(ExPolicy&& policy, FwdIter first, FwdIter last,
            Size count, T const& value, Pred pred = Pred(), Proj proj = Proj())
        {
            return hpx::parallel::detail::search_n<FwdIter, FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, count, value,
                HPX_MOVE(pred), HPX_MOVE(proj));
        }
    } search_n{};
}    // namespace hpx

#endif
