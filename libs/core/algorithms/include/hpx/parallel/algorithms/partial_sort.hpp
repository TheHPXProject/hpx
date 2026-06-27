//  Copyright (c) 2020 Francisco Jose Tapia
//  Copyright (c) 2021 Akhil J Nair
//  Copyright (c) 2020-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/partial_sort.hpp
/// \page hpx::partial_sort
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {
    // clang-format off

    ///////////////////////////////////////////////////////////////////////////
    /// Places the first middle - first elements from the range [first, last)
    /// as sorted with respect to comp into the range [first, middle). The rest
    /// of the elements in the range [middle, last) are placed in an unspecified
    /// order.
    ///
    /// \note   Complexity: Approximately (last - first) * log(middle - first)
    ///         comparisons.
    ///
    /// \tparam RandIter    The type of the source begin, middle, and end
    ///                     iterators used (deduced). This iterator type must
    ///                     meet the requirements of a random access iterator.
    /// \tparam Comp        The type of the function/function object to use
    ///                     (deduced). Comp defaults to detail::less.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param middle       Refers to the middle of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param comp         comp is a callable object. The return value of the
    ///                     INVOKE operation applied to an object of type Comp,
    ///                     when contextually converted to bool, yields true if
    ///                     the first argument of the call is less than the
    ///                     second, and false otherwise. It is assumed that
    ///                     comp will not apply any non-constant function
    ///                     through the dereferenced iterator. It defaults to
    ///                     detail::less.
    ///
    /// \returns  The \a partial_sort algorithm returns a \a RandIter
    ///           that refers to \a last.
    ///
    template <typename RandIter, typename Comp = hpx::parallel::detail::less>
    RandIter partial_sort(RandIter first, RandIter middle, RandIter last,
        Comp&& comp = Comp());

    ///////////////////////////////////////////////////////////////////////////
    /// Places the first middle - first elements from the range [first, last)
    /// as sorted with respect to comp into the range [first, middle). The rest
    /// of the elements in the range [middle, last) are placed in an unspecified
    /// order.
    ///
    /// \note   Complexity: Approximately (last - first) * log(middle - first)
    ///         comparisons.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it applies user-provided function objects.
    /// \tparam RandIter    The type of the source begin, middle, and end
    ///                     iterators used (deduced). This iterator type must
    ///                     meet the requirements of a random access iterator.
    /// \tparam Comp        The type of the function/function object to use
    ///                     (deduced). Comp defaults to detail::less.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param middle       Refers to the middle of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param comp         comp is a callable object. The return value of the
    ///                     INVOKE operation applied to an object of type Comp,
    ///                     when contextually converted to bool, yields true if
    ///                     the first argument of the call is less than the
    ///                     second, and false otherwise. It is assumed that
    ///                     comp will not apply any non-constant function
    ///                     through the dereferenced iterator. It defaults to
    ///                     detail::less.
    ///
    /// \returns  The \a partial_sort algorithm returns a
    ///           \a hpx::future<RandIter> if the execution policy is of
    ///           type \a sequenced_task_policy or
    ///           \a parallel_task_policy and returns \a RandIter otherwise.
    ///           The iterator returned refers to \a last.
    ///
    template <typename ExPolicy, typename RandIter,
        typename Comp = hpx::parallel::detail::less>
    parallel::util::detail::algorithm_result_t<ExPolicy, RandIter> partial_sort(
        ExPolicy&& policy, RandIter first, RandIter middle, RandIter last,
        Comp&& comp = Comp());

    // clang-format on
}    // namespace hpx

#else

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/algorithms/detail/is_sorted.hpp>
#include <hpx/parallel/algorithms/sort.hpp>
#include <hpx/parallel/util/compare_projected.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
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

        //////////////////////////////////////////////////////////////////////
        ///
        /// \brief Obtain the position of the most significant bit set in N
        ///
        /// \param num : Number to examine
        ///
        /// \return Position of the first bit set
        ///
        HPX_CXX_CORE_EXPORT constexpr unsigned nbits64(
            std::uint64_t num) noexcept
        {
            unsigned nb = 0;
            if (num >= 1ull << 32)
            {
                nb += 32;
                num = num >> 32;
            }
            if (num >= 1ull << 16)
            {
                nb += 16;
                num = num >> 16;
            }
            if (num >= 1ull << 8)
            {
                nb += 8;
                num = num >> 8;
            }
            if (num >= 1ull << 4)
            {
                nb += 4;
                num = num >> 4;
            }
            if (num >= 1ull << 2)
            {
                nb += 2;
                num = num >> 2;
            }
            if (num >= 1ull << 1)
            {
                nb += 1;
            }
            return nb;
        }

        ///////////////////////////////////////////////////////////////////////
        ///
        /// Receive a range between first and last, obtain 9 values between the
        /// elements  including the first and the previous to the last. Obtain
        /// the iterator to the mid value and swap with the first position
        //
        /// \param first    iterator to the first element
        /// \param last     iterator to the last element
        /// \param comp     object to Comp two elements
        ///
        HPX_CXX_CORE_EXPORT template <typename Iter, typename Comp>
        constexpr void pivot3(Iter first, Iter last, Comp&& comp) noexcept
        {
            auto n2 = (last - first) / 2;
            Iter it_val =
                mid3(first + 1, first + n2, last - 1, HPX_FORWARD(Comp, comp));
            std::ranges::iter_swap(first, it_val);
        }

        ///////////////////////////////////////////////////////////////////////
        ///
        /// This function obtain a pivot in the range and filter the elements
        /// according the value of that pivot
        ///
        /// \param first : iterator to the first element
        /// \param end : iterator to the element after the last
        /// \param comp : object to Comp two elements
        ///
        /// \return iterator where is the pivot used in the filtering
        ///
        HPX_CXX_CORE_EXPORT template <typename Iter, typename Comp>
        constexpr inline Iter filter(Iter first, Iter end, Comp&& comp)
        {
            std::int64_t const nelem = end - first;
            if (nelem > 4096)
            {
                pivot9(first, end, comp);
            }
            else
            {
                pivot3(first, end, comp);
            }

            typename std::iterator_traits<Iter>::value_type const pivot =
                *first;

            Iter c_first = first + 1, c_last = end - 1;
            while (c_first < end && HPX_INVOKE(comp, *c_first, pivot))
            {
                ++c_first;
            }
            while (c_last > first && HPX_INVOKE(comp, pivot, *c_last))
            {
                --c_last;
            }

            while (c_first < c_last)
            {
                std::ranges::iter_swap(c_first++, c_last--);
                while (c_first < end && HPX_INVOKE(comp, *c_first, pivot))
                {
                    ++c_first;
                }
                while (c_last > first && HPX_INVOKE(comp, pivot, *c_last))
                {
                    --c_last;
                }
            }

            std::ranges::iter_swap(first, c_last);
            return c_last;
        }

        // Internal function to divide and sort the ranges
        //
        // first : iterator to the first element to be sorted
        // middle: iterator defining the last element to be sorted
        // end : iterator to the element after the end in the range
        // level : level of depth from the top level call
        // comp : object for to Comp elements
        HPX_CXX_CORE_EXPORT template <typename Iter, typename Comp>
        constexpr void recursive_partial_sort(
            Iter first, Iter middle, Iter end, std::uint32_t level, Comp&& comp)
        {
            std::int64_t const nelem = end - first;
            std::int64_t const nmid = middle - first;
            if (nelem == 0 || nmid == 0)
            {
                return;
            }

            constexpr std::uint32_t nmin = 24;
            if (nelem < nmin)
            {
                std::sort(first, end, comp);
                return;
            }

            if (nmid == 1)
            {
                for (Iter it = first + 1; it != end; ++it)
                {
                    if (HPX_INVOKE(comp, *it, *first))
                    {
                        std::ranges::iter_swap(it, first);
                    }
                }
                return;
            }

            if (level == 0)
            {
                std::make_heap(first, end, comp);
                std::sort_heap(first, middle, comp);
                return;
            }

            Iter c_last = filter(first, end, comp);
            if (middle >= c_last)
            {
                std::sort(first, c_last, comp);
                if (middle != c_last)
                {
                    recursive_partial_sort(c_last + 1, middle, end, level - 1,
                        HPX_FORWARD(Comp, comp));
                }
                return;
            }

            recursive_partial_sort(
                first, middle, c_last, level - 1, HPX_FORWARD(Comp, comp));
        }

        // Internal function to divide and sort the ranges
        //
        // policy : execution policy
        // first : iterator to the first element
        // middle: iterator defining the last element to be sorted
        // last : iterator to the element after the end in the range
        // level : level of depth from the top level call
        // comp : object for to Comp elements
        //
        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
            typename Comp>
        hpx::future<Iter> parallel_partial_sort(ExPolicy&& policy, Iter first,
            Iter middle, Iter last, std::uint32_t level, Comp&& comp = Comp());

        HPX_CXX_CORE_EXPORT struct sort_thread_helper
        {
            template <typename... Ts>
            decltype(auto) operator()(Ts&&... ts) const
            {
                return sort_thread(HPX_FORWARD(Ts, ts)...);
            }
        };

        HPX_CXX_CORE_EXPORT struct parallel_partial_sort_helper
        {
            template <typename... Ts>
            decltype(auto) operator()(Ts&&... ts) const
            {
                return parallel_partial_sort(HPX_FORWARD(Ts, ts)...);
            }
        };

        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
            typename Comp>
        hpx::future<Iter> parallel_partial_sort(ExPolicy&& policy, Iter first,
            Iter middle, Iter last, std::uint32_t level, Comp&& comp)
        {
            std::int64_t nelem = last - first;
            std::int64_t const nmid = middle - first;
            HPX_ASSERT(nelem >= nmid);

            if (nelem == 0 || nmid == 0)
            {
                return hpx::make_ready_future(last);
            }

            if (nmid < 4096 || level < 12)
            {
                recursive_partial_sort(first, middle, last, level, comp);
                return hpx::make_ready_future(last);
            }

            Iter c_last = filter(first, last, comp);
            if (middle >= c_last)
            {
                // figure out the chunk size to use
                std::size_t const cores =
                    hpx::execution::experimental::processing_units_count(
                        policy.parameters(), policy.executor(),
                        hpx::chrono::null_duration, nelem);

                // number of elements to sort
                std::size_t chunk_size =
                    hpx::execution::experimental::get_chunk_size(
                        policy.parameters(), policy.executor(),
                        hpx::chrono::null_duration, cores, nelem);

                hpx::future<Iter> left = execution::async_execute(
                    policy.executor(), sort_thread_helper(), policy, first,
                    c_last, comp, chunk_size);

                hpx::future<Iter> right;
                if (middle != c_last)
                {
                    right = execution::async_execute(policy.executor(),
                        parallel_partial_sort_helper(), policy, c_last + 1,
                        middle, last, level - 1, comp);
                }
                else
                {
                    right = hpx::make_ready_future(last);
                }

                return hpx::dataflow(
                    policy.executor(),
                    [last](hpx::future<Iter>&& leftf,
                        hpx::future<Iter>&& rightf) -> Iter {
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

            return parallel_partial_sort(HPX_FORWARD(ExPolicy, policy), first,
                middle, c_last, level - 1, HPX_FORWARD(Comp, comp));
        }
        /// \endcond NOINTERNAL
    }    // end namespace detail

    ///////////////////////////////////////////////////////////////////////////
    ///
    /// \brief : Rearranges elements such that the range [first, middle)
    ///          contains the sorted middle - first smallest elements in the
    ///          range [first, end).
    ///
    /// \param first : iterator to the first element
    /// \param middle: iterator defining the last element to be sorted
    /// \param end : iterator to the element after the end in the range
    /// \param comp : object for to Comp elements
    ///
    HPX_CXX_CORE_EXPORT template <typename Iter, typename Sent, typename Comp>
    Iter sequential_partial_sort(Iter first, Iter middle, Sent end, Comp&& comp)
    {
        std::int64_t const nelem = parallel::detail::distance(first, end);
        HPX_ASSERT(nelem >= 0);

        std::int64_t const nmid = middle - first;
        HPX_ASSERT(nmid >= 0 && nmid <= nelem);

        if (nmid > 1024)
        {
            if (detail::is_sorted_sequential(first, middle, comp))
            {
                return first + nelem;
            }
        }

        std::uint32_t level = detail::nbits64(nelem) * 2;
        detail::recursive_partial_sort(
            first, middle, first + nelem, level, HPX_FORWARD(Comp, comp));
        return first + nelem;
    }

    ///////////////////////////////////////////////////////////////////////////
    ///
    /// \brief : Rearranges elements such that the range [first, middle)
    ///          contains the sorted middle - first smallest elements in the
    ///          range [first, end).
    ///
    /// \param policy : execution policy
    /// \param first : iterator to the first element
    /// \param middle: iterator defining the last element to be sorted
    /// \param end : iterator to the element after the end in the range
    /// \param comp : object for to Comp elements
    ///
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename Sent, typename Comp>
    hpx::future<Iter> parallel_partial_sort(
        ExPolicy&& policy, Iter first, Iter middle, Sent end, Comp&& comp)
    {
        std::int64_t const nelem = parallel::detail::distance(first, end);
        HPX_ASSERT(nelem >= 0);

        std::int64_t const nmid = middle - first;
        HPX_ASSERT(nmid >= 0 && nmid <= nelem);

        if (nmid > 1024)
        {
            if (detail::is_sorted_sequential(first, middle, comp))
            {
                return hpx::make_ready_future(first + nelem);
            }
        }

        std::uint32_t level = parallel::detail::nbits64(nelem) * 2;
        return detail::parallel_partial_sort(HPX_FORWARD(ExPolicy, policy),
            first, middle, first + nelem, level, HPX_FORWARD(Comp, comp));
    }

    ///////////////////////////////////////////////////////////////////////
    // partial_sort
    HPX_CXX_CORE_EXPORT template <typename RandIter>
    struct partial_sort
      : public detail::algorithm<partial_sort<RandIter>, RandIter>
    {
        constexpr partial_sort() noexcept
          : detail::algorithm<partial_sort, RandIter>("partial_sort")
        {
        }

        template <typename ExPolicy, typename Iter, typename Sent,
            typename Comp, typename Proj>
        static constexpr Iter sequential(ExPolicy, Iter first, Iter middle,
            Sent last, Comp&& comp, Proj&& proj)
        {
            return sequential_partial_sort(first, middle, last,
                util::compare_projected<Comp&, Proj&>(comp, proj));
        }

        template <typename ExPolicy, typename Iter, typename Sent,
            typename Comp, typename Proj>
        static decltype(auto) parallel(ExPolicy&& policy, Iter first,
            Iter middle, Sent last, Comp&& comp, Proj&& proj)
        {
            constexpr bool has_scheduler_executor =
                hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

            if constexpr (has_scheduler_executor)
            {
                namespace ex = hpx::execution::experimental;
                return ex::just(first, middle, last) |
                    ex::then([comp = HPX_FORWARD(Comp, comp),
                                 proj = HPX_FORWARD(Proj, proj)](
                                 Iter first, Iter middle, Iter last) -> Iter {
                        return sequential_partial_sort(first, middle, last,
                            util::compare_projected<std::decay_t<Comp>,
                                std::decay_t<Proj>>(comp, proj));
                    });
            }
            else
            {
                using algorithm_result =
                    util::detail::algorithm_result<ExPolicy, Iter>;

                try
                {
                    // call the sort routine and return the right type,
                    // depending on execution policy
                    return algorithm_result::get(parallel_partial_sort(
                        HPX_FORWARD(ExPolicy, policy), first, middle, last,
                        util::compare_projected<Comp&, Proj&>(comp, proj)));
                }
                catch (...)
                {
                    return algorithm_result::get(
                        detail::handle_exception<ExPolicy, Iter>::call(
                            std::current_exception()));
                }
            }
        }
    };
}    // namespace hpx::parallel

namespace hpx {

    HPX_CXX_CORE_EXPORT inline constexpr struct partial_sort_t final
      : hpx::detail::tag_parallel_algorithm<partial_sort_t>
    {
    private:
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
        friend RandIter tag_fallback_invoke(hpx::partial_sort_t, RandIter first,
            RandIter middle, RandIter last, Comp comp = Comp())
        {
            static_assert(std::random_access_iterator<RandIter>,
                "Requires at least random access iterator.");

            return parallel::partial_sort<RandIter>().call(hpx::execution::seq,
                first, middle, last, HPX_MOVE(comp), hpx::identity_v);
        }

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
        friend decltype(auto) tag_fallback_invoke(hpx::partial_sort_t,
            ExPolicy&& policy, RandIter first, RandIter middle, RandIter last,
            Comp comp = Comp())
        {
            static_assert(std::random_access_iterator<RandIter>,
                "Requires at least random access iterator.");

            using algorithm_result =
                parallel::util::detail::algorithm_result<ExPolicy, RandIter>;

            return algorithm_result::get(
                parallel::partial_sort<RandIter>().call(
                    HPX_FORWARD(ExPolicy, policy), first, middle, last,
                    HPX_MOVE(comp), hpx::identity_v));
        }
    } partial_sort{};
}    // namespace hpx

#endif    // DOXYGEN
