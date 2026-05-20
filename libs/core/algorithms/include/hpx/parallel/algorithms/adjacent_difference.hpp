//  Copyright (c) 2015 Daniel Bourgeois
//  Copyright (c) 2021 Karame M.Shokooh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/adjacent_difference.hpp
/// \page hpx::adjacent_difference
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx {
    ////////////////////////////////////////////////////////////////////////////
    /// Assigns each value in the range given by result its corresponding
    /// element in the range [first, last] and the one preceding it except
    /// *result, which is assigned *first.
    ///
    /// \note   Complexity: Exactly (last - first) - 1 application of the
    ///                     binary operator and (last - first) assignments.
    ///
    /// \tparam FwdIter1    The type of the source iterators used for the
    ///                     input range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     output range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the range the algorithm will be applied to.
    /// \param dest         Refers to the beginning of the sequence of elements
    ///                     the results will be assigned to.
    ///
    /// \returns  The \a adjacent_difference algorithm returns a \a FwdIter2.
    ///           The \a adjacent_difference algorithm returns an iterator to
    ///           the element past the last element written.
    ///
    template <typename FwdIter1, typename FwdIter2>
    FwdIter2 adjacent_difference(FwdIter1 first, FwdIter1 last, FwdIter2 dest);

    ////////////////////////////////////////////////////////////////////////////
    /// Assigns each value in the range given by result its corresponding
    /// element in the range [first, last] and the one preceding it except
    /// *result, which is assigned *first. Executed according to the policy.
    ///
    /// \note   Complexity: Exactly (last - first) - 1 application of the
    ///                     binary operator and (last - first) assignments.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the source iterators used for the
    ///                     input range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     output range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the range the algorithm will be applied to.
    /// \param dest         Refers to the beginning of the sequence of elements
    ///                     the results will be assigned to.
    ///
    /// The difference operations in the parallel \a adjacent_difference invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The difference operations in the parallel \a adjacent_difference invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an
    /// unordered fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a adjacent_difference algorithm returns a
    ///           \a hpx::future<FwdIter2> if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter2 otherwise.
    ///           The \a adjacent_difference algorithm returns an iterator to
    ///           the element past the last element written.
    ///
    template <typename ExPolicy, typename FwdIter1, typename FwdIter2>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2>
    adjacent_difference(
        ExPolicy&& policy, FwdIter1 first, FwdIter1 last, FwdIter2 dest);

    ///////////////////////////////////////////////////////////////////////////
    /// Assigns each value in the range given by result its corresponding
    /// element in the range [first, last] and the one preceding it except
    /// *result, which is assigned *first
    ///
    /// \note   Complexity: Exactly (last - first) - 1 application of the
    ///                     binary operator and (last - first) assignments.
    ///
    /// \tparam FwdIter1    The type of the source iterators used for the
    ///                     input range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     output range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Op          The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a adjacent_difference requires \a Op
    ///                     to meet the requirements of \a CopyConstructible.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the range the algorithm will be applied to.
    /// \param dest         Refers to the beginning of the sequence of elements
    ///                     the results will be assigned to.
    /// \param op           The binary operator which returns the difference
    ///                     of elements. The signature should be equivalent
    ///                     to the following:
    ///                     \code
    ///                     bool op(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1  must be such
    ///                     that objects of type \a FwdIter1 can be dereferenced
    ///                     and then implicitly converted to the dereferenced
    ///                     type of \a dest.
    ///
    /// \returns  The \a adjacent_difference algorithm returns \a FwdIter2.
    ///           The \a adjacent_difference algorithm returns an iterator to
    ///           the element past the last element written.
    ///
    template <typename FwdIter1, typename FwdIter2, typename Op>
    FwdIter2 adjacent_difference(
        FwdIter1 first, FwdIter1 last, FwdIter2 dest, Op&& op);

    ///////////////////////////////////////////////////////////////////////////
    /// Assigns each value in the range given by result its corresponding
    /// element in the range [first, last] and the one preceding it except
    /// *result, which is assigned *first
    ///
    /// \note   Complexity: Exactly (last - first) - 1 application of the
    ///                     binary operator and (last - first) assignments.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam FwdIter1    The type of the source iterators used for the
    ///                     input range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the source iterators used for the
    ///                     output range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam Op          The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a adjacent_difference requires \a Op
    ///                     to meet the requirements of \a CopyConstructible.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     of the range the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements of
    ///                     the range the algorithm will be applied to.
    /// \param dest         Refers to the beginning of the sequence of elements
    ///                     the results will be assigned to.
    /// \param op           The binary operator which returns the difference
    ///                     of elements. The signature should be equivalent
    ///                     to the following:
    ///                     \code
    ///                     bool op(const Type1 &a, const Type1 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const &, but
    ///                     the function must not modify the objects passed to
    ///                     it. The types \a Type1  must be such
    ///                     that objects of type \a FwdIter1 can be dereferenced
    ///                     and then implicitly converted to the dereferenced
    ///                     type of \a dest.
    ///
    /// The difference operations in the parallel \a adjacent_difference invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The difference operations in the parallel \a adjacent_difference invoked
    /// with an execution policy object of type \a parallel_policy
    /// or \a parallel_task_policy are permitted to execute in an
    /// unordered fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a adjacent_difference algorithm returns a
    ///           \a hpx::future<FwdIter2> if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter2 otherwise.
    ///           The \a adjacent_difference algorithm returns an iterator to
    ///           the element past the last element written.
    ///
    ///
    template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
        typename Op>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2>
    adjacent_difference(ExPolicy&& policy, FwdIter1 first, FwdIter1 last,
        FwdIter2 dest, Op&& op);
}    // namespace hpx

#else

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/is_value_proxy.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/adjacent_difference.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/clear_container.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/partitioner.hpp>

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // adjacent_difference
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT template <typename Iter>
        struct adjacent_difference
          : public algorithm<adjacent_difference<Iter>, Iter>
        {
            constexpr adjacent_difference() noexcept
              : algorithm<adjacent_difference, Iter>("adjacent_difference")
            {
            }

            template <typename ExPolicy, typename InIter, typename Sent,
                typename OutIter, typename Op>
            static constexpr OutIter sequential(
                ExPolicy, InIter first, Sent last, OutIter dest, Op&& op)
            {
                return sequential_adjacent_difference<ExPolicy>(
                    first, last, dest, HPX_FORWARD(Op, op));
            }

            template <typename ExPolicy, typename FwdIter1, typename Sent,
                typename FwdIter2, typename Op>
            static decltype(auto) parallel(ExPolicy&& policy, FwdIter1 first,
                Sent last, FwdIter2 dest, Op&& op)
            {
                using zip_iterator =
                    hpx::util::zip_iterator<FwdIter1, FwdIter1, FwdIter2>;
                using result =
                    util::detail::algorithm_result<ExPolicy, FwdIter2>;
                using difference_type =
                    typename std::iterator_traits<FwdIter1>::difference_type;
                constexpr bool has_scheduler_policy =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                FwdIter1 prev = first;
                difference_type count;

                if (first == last)
                {
                    if constexpr (!has_scheduler_policy)
                    {
                        return result::get(HPX_MOVE(dest));
                    }
                    else
                    {
                        count = static_cast<difference_type>(0);
                    }
                }
                else
                {
                    count = detail::distance(first, last) - 1;

                    hpx::traits::proxy_value_t<
                        typename std::iterator_traits<FwdIter1>::value_type>
                        tmp = *first++;
                    *dest++ = HPX_MOVE(tmp);
                }

                if constexpr (!has_scheduler_policy)
                {
                    if (count == 0)
                    {
                        return result::get(HPX_MOVE(dest));
                    }
                }

                auto f1 = [op = HPX_FORWARD(Op, op)](zip_iterator part_begin,
                              std::size_t part_size) mutable {
                    // VS2015RC bails out when op is captured by ref
                    using hpx::get;
                    util::loop_n<std::decay_t<ExPolicy>>(
                        part_begin, part_size, [op](auto&& it) mutable {
                            get<2>(*it) =
                                HPX_INVOKE(op, get<0>(*it), get<1>(*it));
                        });
                };

                auto f2 = [dest, count](auto&&... data) mutable -> FwdIter2 {
                    static_assert(sizeof...(data) < 2);

                    // make sure iterators embedded in function object that
                    // is attached to futures are invalidated
                    util::detail::clear_container(data...);

                    std::advance(dest, count);
                    return dest;
                };

                return util::partitioner<ExPolicy, FwdIter2, void>::call(
                    HPX_FORWARD(ExPolicy, policy),
                    hpx::util::zip_iterator(first, prev, dest), count,
                    HPX_MOVE(f1), HPX_MOVE(f2));
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::adjacent_difference
    HPX_CXX_CORE_EXPORT inline constexpr struct adjacent_difference_t final
      : hpx::detail::tag_parallel_algorithm<adjacent_difference_t>
    {
    private:
        template <std::input_iterator FwdIter1,
            std::output_iterator<hpx::traits::iter_value_t<FwdIter1>> FwdIter2>
        friend FwdIter2 tag_fallback_invoke(hpx::adjacent_difference_t,
            FwdIter1 first, FwdIter1 last, FwdIter2 dest)
        {
            return hpx::parallel::detail::adjacent_difference<FwdIter2>().call(
                hpx::execution::sequenced_policy{}, first, last, dest,
                std::minus<>());
        }

        template <hpx::execution_policy ExPolicy,
            std::forward_iterator FwdIter1, std::forward_iterator FwdIter2>
        friend decltype(auto) tag_fallback_invoke(hpx::adjacent_difference_t,
            ExPolicy&& policy, FwdIter1 first, FwdIter1 last, FwdIter2 dest)
        {
            return hpx::parallel::detail::adjacent_difference<FwdIter2>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, dest,
                std::minus<>());
        }

        template <std::input_iterator FwdIter1,
            std::output_iterator<hpx::traits::iter_value_t<FwdIter1>> FwdIter2,
            typename Op>
        friend FwdIter2 tag_fallback_invoke(hpx::adjacent_difference_t,
            FwdIter1 first, FwdIter1 last, FwdIter2 dest, Op op)
        {
            return hpx::parallel::detail::adjacent_difference<FwdIter2>().call(
                hpx::execution::sequenced_policy{}, first, last, dest,
                HPX_MOVE(op));
        }

        template <hpx::execution_policy ExPolicy,
            std::forward_iterator FwdIter1, std::forward_iterator FwdIter2,
            typename Op>
        friend decltype(auto) tag_fallback_invoke(hpx::adjacent_difference_t,
            ExPolicy&& policy, FwdIter1 first, FwdIter1 last, FwdIter2 dest,
            Op op)
        {
            return hpx::parallel::detail::adjacent_difference<FwdIter2>().call(
                HPX_FORWARD(ExPolicy, policy), first, last, dest, HPX_MOVE(op));
        }
    } adjacent_difference{};
}    // namespace hpx

#endif
