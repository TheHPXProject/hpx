//  Copyright (c) 2022 Bhumit Attarde
//  Copyright (c) 2014 Grant Mercer
//  Copyright (c) 2015 Daniel Bourgeois
//  Copyright (c) 2016-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/copy.hpp
/// \page hpx::copy, hpx::copy_n, hpx::copy_if
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {
    // clang-format off

    /// Copies the elements in the range, defined by [first, last), to another
    /// range beginning at \a dest. Executed according to the policy.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    ///
    /// The assignments in the parallel \a copy algorithm invoked with an
    /// execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a copy algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a copy algorithm returns a
    ///           \a hpx::future<FwdIter2> >
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter2> otherwise.
    ///           The \a copy algorithm returns the pair of the input iterator
    ///           \a last and the output iterator to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename ExPolicy, typename FwdIter1, typename FwdIter2>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2>
    copy(ExPolicy&& policy, FwdIter1 first, FwdIter1 last, FwdIter2 dest);

    /// Copies the elements in the range, defined by [first, last), to another
    /// range beginning at \a dest.
    ///
    /// \note   Complexity: Performs exactly \a last - \a first assignments.
    ///
    /// \tparam FwdIter1    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    ///
    /// \returns  The \a copy algorithm returns a \a FwdIter2 .
    ///           The \a copy algorithm returns the pair of the input iterator
    ///           \a last and the output iterator to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename FwdIter1, typename FwdIter2>
    FwdIter2 copy(FwdIter1 first, FwdIter1 last, FwdIter2 dest);


    /// Copies the elements in the range [first, first + count), starting from
    /// first and proceeding to first + count - 1., to another range beginning
    /// at dest. Executed according to the policy.
    ///
    /// \note   Complexity: Performs exactly \a count assignments, if
    ///         count > 0, no assignments otherwise.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Size        The type of the argument specifying the number of
    ///                     elements to apply \a f to.
    /// \tparam FwdIter2    The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param count        Refers to the number of elements starting at
    ///                     \a first the algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    ///
    /// The assignments in the parallel \a copy_n algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a copy_n algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a copy_n algorithm returns a
    ///           \a hpx::future<FwdIter2>
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter2
    ///           otherwise.
    ///           The \a copy_n algorithm returns Iterator in the destination range,
    ///           pointing past the last element copied if count>0 or result
    ///           otherwise.
    ///
    template <typename ExPolicy, typename FwdIter1, typename Size, typename FwdIter2>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2>
    copy_n(ExPolicy&& policy, FwdIter1 first, Size count, FwdIter2 dest);

    /// Copies the elements in the range [first, first + count), starting from
    /// first and proceeding to first + count - 1., to another range beginning
    /// at dest.
    ///
    /// \note   Complexity: Performs exactly \a count assignments, if
    ///         count > 0, no assignments otherwise.
    ///
    /// \tparam FwdIter1    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Size        The type of the argument specifying the number of
    ///                     elements to apply \a f to.
    /// \tparam FwdIter2    The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param count        Refers to the number of elements starting at
    ///                     \a first the algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    ///
    /// \returns  The \a copy_n algorithm returns a \a FwdIter2 .
    ///           The \a copy_n algorithm returns Iterator in the destination range,
    ///           pointing past the last element copied if count>0 or result
    ///           otherwise.
    ///
    template <typename FwdIter1, typename Size, typename FwdIter2>
    FwdIter2 copy_n(FwdIter1 first, Size count, FwdIter2 dest);


    /// Copies the elements in the range, defined by [first, last), to another
    /// range beginning at \a dest. Copies only the elements for which the
    /// predicate \a f returns true. The order of the elements that are not
    /// removed is preserved. Executed according to the policy.
    ///
    /// \note   Complexity: Performs not more than \a last - \a first
    ///         assignments, exactly \a last - \a first applications of the
    ///         predicate \a f.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a copy_if requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param pred         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).This is an
    ///                     unary predicate which returns \a true for the
    ///                     required elements. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a FwdIter1 can be dereferenced and then
    ///                     implicitly converted to Type.
    ///
    /// The assignments in the parallel \a copy_if algorithm invoked with
    /// an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The assignments in the parallel \a copy_if algorithm invoked with
    /// an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a copy_if algorithm returns a
    ///           \a hpx::future<FwdIter2> >
    ///           if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter2 otherwise.
    ///           The \a copy_if algorithm returns output iterator to the element in
    ///           the destination range, one past the last element copied.
    ///
    template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
        typename Pred>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2> >
    copy_if(ExPolicy&& policy, FwdIter1 first, FwdIter1 last, FwdIter2 dest,
        Pred&& pred);

    /// Copies the elements in the range, defined by [first, last), to another
    /// range beginning at \a dest. Copies only the elements for which the
    /// predicate \a f returns true. The order of the elements that are not
    /// removed is preserved.
    ///
    /// \note   Complexity: Performs not more than \a last - \a first
    ///         assignments, exactly \a last - \a first applications of the
    ///         predicate \a f.
    ///
    /// \tparam FwdIter1    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Pred        The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a copy_if requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param pred         Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).This is an
    ///                     unary predicate which returns \a true for the
    ///                     required elements. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     bool pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&, but
    ///                     the function must not modify the objects passed to
    ///                     it. The type \a Type must be such that an object of
    ///                     type \a FwdIter1 can be dereferenced and then
    ///                     implicitly converted to Type.
    ///
    /// \returns  The \a copy_if algorithm returns a \a FwdIter2 .
    ///           The \a copy_if algorithm returns output iterator to the element in
    ///           the destination range, one past the last element copied.
    ///
    template <typename FwdIter1, typename FwdIter2, typename Pred>
    FwdIter2 copy_if(FwdIter1 first, FwdIter1 last, FwdIter2 dest,
        Pred&& pred);

    // clang-format on
}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>

#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/algorithms/detail/transfer.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/clear_container.hpp>
#include <hpx/parallel/util/foreach_partitioner.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/result_types.hpp>
#include <hpx/parallel/util/scan_partitioner.hpp>
#include <hpx/parallel/util/transfer.hpp>
#include <hpx/parallel/util/zip_iterator.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // copy
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename ExPolicy>
        struct copy_iteration
        {
            using execution_policy_type = std::decay_t<ExPolicy>;

            template <typename Iter>
            HPX_HOST_DEVICE HPX_FORCEINLINE constexpr void operator()(
                Iter part_begin, std::size_t part_size, std::size_t) const
            {
                using hpx::get;
                auto iters = part_begin.get_iterator_tuple();
                util::copy_n<execution_policy_type>(
                    get<0>(iters), part_size, get<1>(iters));
            }
        };

        HPX_CXX_CORE_EXPORT template <typename IterPair>
        struct copy : public algorithm<copy<IterPair>, IterPair>
        {
            constexpr copy() noexcept
              : algorithm<copy, IterPair>("copy")
            {
            }

            template <typename ExPolicy, typename InIter, typename Sent,
                typename OutIter>
            static constexpr std::enable_if_t<
                !std::random_access_iterator<InIter>,
                util::in_out_result<InIter, OutIter>>
            sequential(ExPolicy, InIter first, Sent last, OutIter dest)
            {
                util::in_out_result<InIter, OutIter> result =
                    util::copy(first, last, dest);
                util::copy_synchronize(first, dest);
                return result;
            }

            template <typename ExPolicy, typename InIter, typename Sent,
                typename OutIter>
            static constexpr std::enable_if_t<
                std::random_access_iterator<InIter>,
                util::in_out_result<InIter, OutIter>>
            sequential(ExPolicy, InIter first, Sent last, OutIter dest)
            {
                util::in_out_result<InIter, OutIter> result =
                    util::copy_n<ExPolicy>(
                        first, detail::distance(first, last), dest);
                util::copy_synchronize(first, dest);
                return result;
            }

            template <typename ExPolicy, typename FwdIter1, typename Sent1,
                typename FwdIter2>
            static decltype(auto) parallel([[maybe_unused]] ExPolicy&& policy,
                [[maybe_unused]] FwdIter1 first, [[maybe_unused]] Sent1 last,
                [[maybe_unused]] FwdIter2 dest)
            {
#if defined(HPX_COMPUTE_DEVICE_CODE)
                HPX_ASSERT(false);
                util::detail::algorithm_result_t<ExPolicy,
                    util::in_out_result<FwdIter1, FwdIter2>>* dummy = nullptr;
                return HPX_MOVE(*dummy);
#else
                using zip_iterator =
                    hpx::util::zip_iterator<FwdIter1, FwdIter2>;

                return util::detail::get_in_out_result(
                    util::foreach_partitioner<ExPolicy>::call(
                        HPX_FORWARD(ExPolicy, policy),
                        zip_iterator(first, dest),
                        detail::distance(first, last),
                        copy_iteration<ExPolicy>(),
                        [](zip_iterator&& zlast) -> zip_iterator {
                            using hpx::get;
                            auto iters = zlast.get_iterator_tuple();
                            util::copy_synchronize(
                                get<0>(iters), get<1>(iters));
                            return HPX_MOVE(zlast);
                        }));
#endif
            }
        };

#if defined(HPX_COMPUTE_DEVICE_CODE)
        template <typename FwdIter1, typename FwdIter2, typename Enable = void>
        struct copy_iter : public copy<util::in_out_result<FwdIter1, FwdIter2>>
        {
        };
#else
        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename FwdIter1, typename FwdIter2,
            typename Enable = void>
        struct copy_iter;

        template <typename FwdIter1, typename FwdIter2>
        struct copy_iter<FwdIter1, FwdIter2,
            std::enable_if_t<
                iterators_are_segmented<FwdIter1, FwdIter2>::value>>
          : public copy<util::in_out_result<
                typename hpx::traits::segmented_iterator_traits<
                    FwdIter1>::local_iterator,
                typename hpx::traits::segmented_iterator_traits<
                    FwdIter2>::local_iterator>>
        {
        };

        template <typename FwdIter1, typename FwdIter2>
        struct copy_iter<FwdIter1, FwdIter2,
            std::enable_if_t<
                iterators_are_not_segmented<FwdIter1, FwdIter2>::value>>
          : public copy<util::in_out_result<FwdIter1, FwdIter2>>
        {
        };
#endif
    }    // namespace detail

    /////////////////////////////////////////////////////////////////////////////
    // copy_n
    namespace detail {

        // sequential copy_n
        HPX_CXX_CORE_EXPORT template <typename IterPair>
        struct copy_n : public algorithm<copy_n<IterPair>, IterPair>
        {
            constexpr copy_n() noexcept
              : algorithm<copy_n, IterPair>("copy_n")
            {
            }

            template <typename ExPolicy, typename InIter, typename OutIter>
            static constexpr util::in_out_result<InIter, OutIter> sequential(
                ExPolicy, InIter first, std::size_t count, OutIter dest)
            {
                util::in_out_result<InIter, OutIter> result =
                    util::copy_n<ExPolicy>(first, count, dest);
                util::copy_synchronize(first, dest);
                return result;
            }

            template <typename ExPolicy, typename FwdIter1, typename FwdIter2>
            static decltype(auto) parallel(ExPolicy&& policy, FwdIter1 first,
                std::size_t count, FwdIter2 dest)
            {
                using zip_iterator =
                    hpx::util::zip_iterator<FwdIter1, FwdIter2>;

                return util::detail::get_in_out_result(
                    util::foreach_partitioner<ExPolicy>::call(
                        HPX_FORWARD(ExPolicy, policy),
                        zip_iterator(first, dest), count,
                        [](zip_iterator part_begin, std::size_t part_size,
                            std::size_t) {
                            auto iters = part_begin.get_iterator_tuple();
                            util::copy_n<ExPolicy>(hpx::get<0>(iters),
                                part_size, hpx::get<1>(iters));
                        },
                        [](zip_iterator&& last) -> zip_iterator {
                            auto iters = last.get_iterator_tuple();
                            util::copy_synchronize(
                                hpx::get<0>(iters), hpx::get<1>(iters));
                            return HPX_MOVE(last);
                        }));
            }
        };
    }    // namespace detail

    /////////////////////////////////////////////////////////////////////////////
    // copy_if
    namespace detail {

        // sequential copy_if with projection function
        HPX_CXX_CORE_EXPORT template <typename InIter1, typename InIter2,
            typename OutIter, typename Pred, typename Proj>
        constexpr util::in_out_result<InIter1, OutIter> sequential_copy_if(
            InIter1 first, InIter2 last, OutIter dest, Pred&& pred, Proj&& proj)
        {
            while (first != last)
            {
                if (HPX_INVOKE(pred, HPX_INVOKE(proj, *first)))
                    *dest++ = *first;
                ++first;
            }
            return util::in_out_result<InIter1, OutIter>{
                HPX_MOVE(first), HPX_MOVE(dest)};
        }

        HPX_CXX_CORE_EXPORT template <typename IterPair>
        struct copy_if : public algorithm<copy_if<IterPair>, IterPair>
        {
            constexpr copy_if() noexcept
              : algorithm<copy_if, IterPair>("copy_if")
            {
            }

            template <typename ExPolicy, typename InIter1, typename InIter2,
                typename OutIter, typename Pred, typename Proj = hpx::identity>
            static constexpr util::in_out_result<InIter1, OutIter> sequential(
                ExPolicy, InIter1 first, InIter2 last, OutIter dest,
                Pred&& pred, Proj&& proj /* = Proj()*/)
            {
                return sequential_copy_if(first, last, dest,
                    HPX_FORWARD(Pred, pred), HPX_FORWARD(Proj, proj));
            }

            template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
                typename FwdIter3, typename Pred, typename Proj = hpx::identity>
            static typename util::detail::algorithm_result<ExPolicy,
                util::in_out_result<FwdIter1, FwdIter3>>::type
            parallel(ExPolicy&& policy, FwdIter1 first, FwdIter2 last,
                FwdIter3 dest, Pred&& pred, Proj&& proj /* = Proj()*/)
            {
                typedef hpx::util::zip_iterator<FwdIter1, bool*> zip_iterator;
                typedef util::detail::algorithm_result<ExPolicy,
                    util::in_out_result<FwdIter1, FwdIter3>>
                    result;
                typedef typename std::iterator_traits<FwdIter1>::difference_type
                    difference_type;

                if (first == last)
                {
                    return result::get(util::in_out_result<FwdIter1, FwdIter3>{
                        HPX_MOVE(first), HPX_MOVE(dest)});
                }

                difference_type count = detail::distance(first, last);

                std::shared_ptr<bool[]> flags(new bool[count]);

                std::size_t init = 0;

                using hpx::get;
                typedef util::scan_partitioner<ExPolicy,
                    util::in_out_result<FwdIter1, FwdIter3>, std::size_t>
                    scan_partitioner_type;

                auto f1 = [pred = HPX_FORWARD(Pred, pred),
                              proj = HPX_FORWARD(decltype(proj), proj)](
                              zip_iterator part_begin,
                              std::size_t part_size) -> std::size_t {
                    std::size_t curr = 0;

                    // Note: replacing the invoke() with HPX_INVOKE()
                    // below makes gcc generate errors

                    // MSVC complains if proj is captured by ref below
                    util::const_loop_n<std::decay_t<ExPolicy>>(part_begin,
                        part_size,
                        [&pred, proj, &curr](zip_iterator it) mutable -> void {
                            bool f = hpx::invoke(
                                pred, hpx::invoke(proj, get<0>(*it)));

                            // NOLINTNEXTLINE(bugprone-assignment-in-if-condition)
                            if ((get<1>(*it) = f))
                                ++curr;
                        });

                    return curr;
                };
                auto f3 = [dest, flags](zip_iterator part_begin,
                              std::size_t part_size, std::size_t val) mutable {
                    HPX_UNUSED(flags);
                    std::advance(dest, val);
                    util::const_loop_n<std::decay_t<ExPolicy>>(part_begin,
                        part_size, [&dest](zip_iterator it) mutable {
                            if (get<1>(*it))
                                *dest++ = get<0>(*it);
                        });
                };

                auto f4 = [first, count, dest, flags](
                              std::vector<std::size_t>&& items,
                              std::vector<hpx::future<void>>&& data) mutable
                    -> util::in_out_result<FwdIter1, FwdIter3> {
                    HPX_UNUSED(flags);

                    auto dist = items.back();
                    std::advance(first, count);
                    std::advance(dest, dist);

                    // make sure iterators embedded in function object that is
                    // attached to futures are invalidated
                    util::detail::clear_container(data);

                    return util::in_out_result<FwdIter1, FwdIter3>{
                        HPX_MOVE(first), HPX_MOVE(dest)};
                };

                return scan_partitioner_type::call(
                    HPX_FORWARD(ExPolicy, policy),
                    zip_iterator(first, flags.get()), count, init,
                    // step 1 performs first part of scan algorithm
                    HPX_MOVE(f1),
                    // step 2 propagates the partition results from left
                    // to right
                    std::plus<std::size_t>(),
                    // step 3 runs final accumulation on each partition
                    HPX_MOVE(f3),
                    // step 4 use this return value
                    HPX_MOVE(f4));
            }
        };
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::copy
    HPX_CXX_CORE_EXPORT inline constexpr struct copy_t final
      : hpx::detail::tag_parallel_algorithm<copy_t>
    {
    private:
        template <typename ExPolicy, typename FwdIter1, typename FwdIter2>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2>
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(hpx::copy_t,
            ExPolicy&& policy, FwdIter1 first, FwdIter1 last, FwdIter2 dest)
        {
            return parallel::util::get_second_element(
                parallel::detail::transfer<
                    parallel::detail::copy_iter<FwdIter1, FwdIter2>>(
                    HPX_FORWARD(ExPolicy, policy), first, last, dest));
        }

        template <typename FwdIter1, typename FwdIter2>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2>
            )
        // clang-format on
        friend FwdIter2 tag_fallback_invoke(
            hpx::copy_t, FwdIter1 first, FwdIter1 last, FwdIter2 dest)
        {
            return parallel::util::get_second_element(
                parallel::detail::transfer<
                    parallel::detail::copy_iter<FwdIter1, FwdIter2>>(
                    hpx::execution::seq, first, last, dest));
        }
    } copy{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::copy_n
    HPX_CXX_CORE_EXPORT inline constexpr struct copy_n_t final
      : hpx::detail::tag_parallel_algorithm<copy_n_t>
    {
    private:
        template <typename ExPolicy, typename FwdIter1, typename Size,
            typename FwdIter2>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2>
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(hpx::copy_n_t,
            ExPolicy&& policy, FwdIter1 first, Size count, FwdIter2 dest)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Required at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2> ||
                    (hpx::is_sequenced_execution_policy_v<ExPolicy> &&
                        std::output_iterator<FwdIter2,
                            hpx::traits::iter_value_t<FwdIter1>>),
                "Requires at least forward iterator or sequential execution.");

            constexpr bool has_scheduler_executor =
                hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

            if (hpx::parallel::detail::is_negative(count))
            {
                if constexpr (has_scheduler_executor)
                {
                    count = static_cast<Size>(0);
                }
                else
                {
                    // if count is representing a negative value, we do nothing
                    return hpx::parallel::util::detail::algorithm_result<
                        ExPolicy, FwdIter2>::get(HPX_MOVE(dest));
                }
            }

            return hpx::parallel::util::get_second_element(
                hpx::parallel::detail::copy_n<
                    hpx::parallel::util::in_out_result<FwdIter1, FwdIter2>>()
                    .call(HPX_FORWARD(ExPolicy, policy), first,
                        static_cast<std::size_t>(count), dest));
        }

        template <typename FwdIter1, typename Size, typename FwdIter2>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2>
            )
        // clang-format on
        friend FwdIter2 tag_fallback_invoke(
            hpx::copy_n_t, FwdIter1 first, Size count, FwdIter2 dest)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Required at least forward iterator.");
            static_assert(std::output_iterator<FwdIter2,
                              hpx::traits::iter_value_t<FwdIter1>>,
                "Requires at least output iterator.");

            // if count is representing a negative value, we do nothing
            if (hpx::parallel::detail::is_negative(count))
            {
                return hpx::parallel::util::detail::algorithm_result<
                    hpx::execution::sequenced_policy,
                    FwdIter2>::get(HPX_MOVE(dest));
            }

            return hpx::parallel::util::get_second_element(
                hpx::parallel::detail::copy_n<
                    hpx::parallel::util::in_out_result<FwdIter1, FwdIter2>>()
                    .call(hpx::execution::seq, first,
                        static_cast<std::size_t>(count), dest));
        }
    } copy_n{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::copy_if
    HPX_CXX_CORE_EXPORT inline constexpr struct copy_if_t final
      : hpx::detail::tag_parallel_algorithm<copy_if_t>
    {
    private:
        template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
            typename Pred>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<FwdIter1>::value_type
                >
            )
        // clang-format on
        friend parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2>
        tag_fallback_invoke(hpx::copy_if_t, ExPolicy&& policy, FwdIter1 first,
            FwdIter1 last, FwdIter2 dest, Pred pred)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Required at least forward iterator.");
            static_assert(std::forward_iterator<FwdIter2> ||
                    (hpx::is_sequenced_execution_policy_v<ExPolicy> &&
                        std::output_iterator<FwdIter2,
                            hpx::traits::iter_value_t<FwdIter1>>),
                "Requires at least forward iterator or sequential execution.");

            return hpx::parallel::util::get_second_element(
                hpx::parallel::detail::copy_if<
                    hpx::parallel::util::in_out_result<FwdIter1, FwdIter2>>()
                    .call(HPX_FORWARD(ExPolicy, policy), first, last, dest,
                        HPX_MOVE(pred), hpx::identity_v));
        }

        template <typename FwdIter1, typename FwdIter2, typename Pred>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                hpx::is_invocable_v<Pred,
                    typename std::iterator_traits<FwdIter1>::value_type
                >
            )
        // clang-format on
        friend FwdIter2 tag_fallback_invoke(hpx::copy_if_t, FwdIter1 first,
            FwdIter1 last, FwdIter2 dest, Pred pred)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Required at least forward iterator.");
            static_assert(std::output_iterator<FwdIter2,
                              hpx::traits::iter_value_t<FwdIter1>>,
                "Requires at least output iterator.");

            return hpx::parallel::util::get_second_element(
                hpx::parallel::detail::copy_if<
                    hpx::parallel::util::in_out_result<FwdIter1, FwdIter2>>()
                    .call(hpx::execution::seq, first, last, dest,
                        HPX_MOVE(pred), hpx::identity_v));
        }
    } copy_if{};
}    // namespace hpx

#endif    // DOXYGEN
