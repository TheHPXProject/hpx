//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/for_each.hpp
/// \page hpx::for_each, hpx::for_each_n
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {
    /// Applies \a f to the result of dereferencing every iterator in the
    /// range [first, last).
    ///
    /// \note   Complexity: Applies \a f exactly \a last - \a first times.
    ///
    /// If \a f returns a result, the result is ignored.
    ///
    /// If the type of \a first satisfies the requirements of a mutable
    /// iterator, \a f may apply non-constant functions through the
    /// dereferenced iterator.
    ///
    /// \tparam InIter      The type of the source begin and end iterator used
    ///                     (deduced). This iterator type must meet the
    ///                     requirements of an input iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). F must meet requirements of
    ///                     \a MoveConstructible.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).
    ///                     The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     <ignored> pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&. The
    ///                     type \a Type must be such that an object of
    ///                     type \a InIter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///
    /// \returns            \a f.
    ///
    template <typename InIter, typename F>
    F for_each(InIter first, InIter last, F&& f);

    /// Applies \a f to the result of dereferencing every iterator in the
    /// range [first, last). Executed according to the policy.
    ///
    /// \note   Complexity: Applies \a f exactly \a last - \a first times.
    ///
    /// If \a f returns a result, the result is ignored.
    ///
    /// If the type of \a first satisfies the requirements of a mutable
    /// iterator, \a f may apply non-constant functions through the
    /// dereferenced iterator.
    ///
    /// Unlike its sequential form, the parallel overload of
    /// \a for_each does not return a copy of its \a Function parameter,
    /// since parallelization may not permit efficient state
    /// accumulation.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it applies user-provided function objects.
    /// \tparam FwdIter     The type of the source begin and end iterator used
    ///                     (deduced). This iterator type must meet the
    ///                     requirements of a forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a for_each requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).
    ///                     The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     <ignored> pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&. The
    ///                     type \a Type must be such that an object of
    ///                     type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to Type.
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
    /// \returns  The \a for_each algorithm returns a
    ///           \a hpx::future<void> if the execution policy is of
    ///           type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and returns void otherwise.
    ///
    template <typename ExPolicy, typename FwdIter, typename F>
    util::detail::algorithm_result_t<ExPolicy, void> for_each(
        ExPolicy&& policy, FwdIter first, FwdIter last, F&& f);

    /// Applies \a f to the result of dereferencing every iterator in the range
    /// [first, first + count), starting from first and proceeding to
    /// first + count - 1.
    ///
    /// \note   Complexity: Applies \a f exactly \a count times.
    ///
    /// If \a f returns a result, the result is ignored.
    ///
    /// If the type of \a first satisfies the requirements of a mutable
    /// iterator, \a f may apply non-constant functions through the
    /// dereferenced iterator.
    ///
    /// \tparam InIter      The type of the source begin and end iterator used
    ///                     (deduced). This iterator type must meet the
    ///                     requirements of an input iterator.
    /// \tparam Size        The type of the argument specifying the number of
    ///                     elements to apply \a f to.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). F must meet requirements of
    ///                     \a MoveConstructible.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param count        Refers to the number of elements starting at
    ///                     \a first the algorithm will be applied to.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).
    ///                     The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     <ignored> pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&. The
    ///                     type \a Type must be such that an object of
    ///                     type \a InIter can be dereferenced and then
    ///                     implicitly converted to Type.
    ///
    /// \returns            \a first + \a count for non-negative values of
    ///                     \a count and \a first for negative values.
    ///
    template <typename InIter, typename Size, typename F>
    InIter for_each_n(InIter first, Size count, F&& f);

    /// Applies \a f to the result of dereferencing every iterator in the range
    /// [first, first + count), starting from first and proceeding to
    /// first + count - 1. Executed according to the policy.
    ///
    /// \note   Complexity: Applies \a f exactly \a count times.
    ///
    /// If \a f returns a result, the result is ignored.
    ///
    /// If the type of \a first satisfies the requirements of a mutable
    /// iterator, \a f may apply non-constant functions through the
    /// dereferenced iterator.
    ///
    /// Unlike its sequential form, the parallel overload of
    /// \a for_each_n does not return a copy of its \a Function parameter,
    /// since parallelization may not permit efficient state
    /// accumulation.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it applies user-provided function objects.
    /// \tparam FwdIter     The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     forward iterator.
    /// \tparam Size        The type of the argument specifying the number of
    ///                     elements to apply \a f to.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a for_each_n requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param count        Refers to the number of elements starting at
    ///                     \a first the algorithm will be applied to.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).
    ///                     The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     <ignored> pred(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&. The
    ///                     type \a Type must be such that an object of
    ///                     type \a FwdIter can be dereferenced and then
    ///                     implicitly converted to Type.
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
    /// \returns  The \a for_each_n algorithm returns a
    ///           \a hpx::future<FwdIter> if the execution policy is of
    ///           type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and returns \a FwdIter
    ///           otherwise.
    ///           It returns \a first + \a count for non-negative values of
    ///           \a count and \a first for negative values.
    ///
    template <typename ExPolicy, typename FwdIter, typename Size, typename F>
    util::detail::algorithm_result_t<ExPolicy, FwdIter> for_each_n(
        ExPolicy&& policy, FwdIter first, Size count, F&& f);

}    // namespace hpx

#else

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/is_value_proxy.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/foreach_partitioner.hpp>
#include <hpx/parallel/util/invoke_projected.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/partitioner.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // for_each_n
    namespace detail {

        /// \cond NOINTERNAL

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename F,
            typename Proj = hpx::identity>
        struct for_each_iteration
        {
            using execution_policy_type = std::decay_t<ExPolicy>;
            using fun_type = std::decay_t<F>;
            using proj_type = Proj;

            fun_type f_;
            proj_type proj_;

            template <typename F_, typename Proj_>
            HPX_HOST_DEVICE for_each_iteration(F_&& f, Proj_&& proj)
              : f_(HPX_FORWARD(F_, f))
              , proj_(HPX_FORWARD(Proj_, proj))
            {
            }

#if !defined(__NVCC__) && !defined(__CUDACC__)
            for_each_iteration(for_each_iteration const&) = default;
            for_each_iteration(for_each_iteration&&) = default;
#else
            HPX_HOST_DEVICE for_each_iteration(for_each_iteration const& rhs)
              : f_(rhs.f_)
              , proj_(rhs.proj_)
            {
            }

            HPX_HOST_DEVICE for_each_iteration(for_each_iteration&& rhs)
              : f_(HPX_MOVE(rhs.f_))
              , proj_(HPX_MOVE(rhs.proj_))
            {
            }
#endif
            for_each_iteration& operator=(for_each_iteration const&) = default;
            for_each_iteration& operator=(for_each_iteration&&) = default;

            ~for_each_iteration() = default;

            template <typename Iter>
            HPX_HOST_DEVICE HPX_FORCEINLINE constexpr void operator()(
                Iter part_begin, std::size_t part_size)
            {
                util::loop_n<execution_policy_type>(part_begin, part_size,
                    util::invoke_projected_ind<fun_type, proj_type>{f_, proj_});
            }

            template <typename Iter>
            HPX_HOST_DEVICE HPX_FORCEINLINE constexpr void operator()(
                Iter part_begin, std::size_t part_size, std::size_t)
            {
                util::loop_n<execution_policy_type>(part_begin, part_size,
                    util::invoke_projected_ind<fun_type, proj_type>{f_, proj_});
            }
        };

        template <typename ExPolicy, typename F>
        struct for_each_iteration<ExPolicy, F, hpx::identity>
        {
            using execution_policy_type = std::decay_t<ExPolicy>;
            using fun_type = std::decay_t<F>;

            fun_type f_;

            template <typename F_,
                typename = std::enable_if_t<
                    !std::is_same_v<std::decay_t<F_>, for_each_iteration>>>
            HPX_HOST_DEVICE explicit for_each_iteration(F_&& f)
              : f_(HPX_FORWARD(F_, f))
            {
            }

            template <typename F_, typename Proj_>
            HPX_HOST_DEVICE for_each_iteration(F_&& f, Proj_&&)
              : f_(HPX_FORWARD(F_, f))
            {
            }

#if !defined(__NVCC__) && !defined(__CUDACC__)
            for_each_iteration(for_each_iteration const&) = default;
            for_each_iteration(for_each_iteration&&) = default;
#else
            HPX_HOST_DEVICE for_each_iteration(for_each_iteration const& rhs)
              : f_(rhs.f_)
            {
            }

            HPX_HOST_DEVICE for_each_iteration(for_each_iteration&& rhs)
              : f_(HPX_MOVE(rhs.f_))
            {
            }
#endif
            for_each_iteration& operator=(for_each_iteration const&) = default;
            for_each_iteration& operator=(for_each_iteration&&) = default;

            ~for_each_iteration() = default;

            template <typename Iter>
            HPX_HOST_DEVICE HPX_FORCEINLINE constexpr void operator()(
                Iter part_begin, std::size_t part_size)
            {
                using value_type = hpx::traits::iter_reference_t<Iter>;
                if constexpr (hpx::traits::is_value_proxy_v<value_type>)
                {
                    util::loop_n<execution_policy_type>(part_begin, part_size,
                        util::invoke_projected_ind<F>(f_));
                }
                else
                {
                    util::loop_n_ind<execution_policy_type>(
                        part_begin, part_size, f_);
                }
            }

            template <typename Iter>
            HPX_HOST_DEVICE HPX_FORCEINLINE constexpr void operator()(
                Iter part_begin, std::size_t part_size, std::size_t)
            {
                return (*this)(part_begin, part_size);
            }
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename Iter>
        struct for_each_n : public algorithm<for_each_n<Iter>, Iter>
        {
            constexpr for_each_n() noexcept
              : algorithm<for_each_n, Iter>("for_each_n")
            {
            }

            template <typename ExPolicy, typename InIter, typename F,
                typename Proj>
            HPX_HOST_DEVICE static constexpr Iter sequential(
                ExPolicy&&, InIter first, std::size_t count, F&& f, Proj&& proj)
            {
                return util::loop_n<std::decay_t<ExPolicy>>(first, count,
                    util::invoke_projected_ind<F, std::decay_t<Proj>>{f, proj});
            }

            template <typename ExPolicy, typename InIter, typename F>
            HPX_HOST_DEVICE static constexpr Iter sequential(ExPolicy&&,
                InIter first, std::size_t count, F&& f, hpx::identity)
            {
                return util::loop_n_ind<std::decay_t<ExPolicy>>(
                    first, count, HPX_FORWARD(F, f));
            }

            template <typename ExPolicy, typename FwdIter, typename F,
                typename Proj = hpx::identity>
            static decltype(auto) parallel(ExPolicy&& policy, FwdIter first,
                std::size_t count, F&& f, Proj&& proj /* = Proj()*/)
            {
                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                if constexpr (!has_scheduler_executor)
                {
                    if (count == 0)
                    {
                        return util::detail::algorithm_result<ExPolicy,
                            FwdIter>::get(HPX_MOVE(first));
                    }
                }

                auto f1 = for_each_iteration<ExPolicy, F, std::decay_t<Proj>>(
                    HPX_FORWARD(F, f), HPX_FORWARD(Proj, proj));

                return util::foreach_partitioner<ExPolicy>::call(
                    HPX_FORWARD(ExPolicy, policy), first, count, HPX_MOVE(f1),
                    hpx::identity_v);
            }
        };
        /// \endcond
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // for_each
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT template <typename Iter>
        struct for_each : public algorithm<for_each<Iter>, Iter>
        {
            constexpr for_each() noexcept
              : algorithm<for_each, Iter>("for_each")
            {
            }

            template <typename ExPolicy, typename InIterB, typename InIterE,
                typename F, typename Proj>
            static constexpr InIterB sequential(
                [[maybe_unused]] ExPolicy&& policy, InIterB first, InIterE last,
                F&& f, Proj&& proj)
            {
                if constexpr (std::random_access_iterator<InIterB>)
                {
                    return util::loop_n<std::decay_t<ExPolicy>>(first,
                        static_cast<std::size_t>(detail::distance(first, last)),
                        util::invoke_projected_ind<F, std::decay_t<Proj>>{
                            f, proj});
                }
                else
                {
                    return util::loop(HPX_FORWARD(ExPolicy, policy), first,
                        last,
                        util::invoke_projected_ind<F, std::decay_t<Proj>>{
                            f, proj});
                }
            }

            template <typename ExPolicy, typename InIterB, typename InIterE,
                typename F>
            static constexpr InIterB sequential(
                ExPolicy&&, InIterB first, InIterE last, F&& f, hpx::identity)
            {
                if constexpr (std::random_access_iterator<InIterB>)
                {
                    return util::loop_n_ind<std::decay_t<ExPolicy>>(first,
                        static_cast<std::size_t>(detail::distance(first, last)),
                        HPX_FORWARD(F, f));
                }
                else
                {
                    return util::loop_ind<std::decay_t<ExPolicy>>(
                        first, last, HPX_FORWARD(F, f));
                }
            }

            template <typename ExPolicy, typename FwdIterB, typename FwdIterE,
                typename F, typename Proj>
            static decltype(auto) parallel(ExPolicy&& policy, FwdIterB first,
                FwdIterE last, F&& f, Proj&& proj)
            {
                using result_t =
                    hpx::parallel::util::detail::algorithm_result<ExPolicy,
                        FwdIterB>;

                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                if constexpr (!has_scheduler_executor)
                {
                    if (first == last)
                    {
                        return result_t::get(HPX_MOVE(first));
                    }
                }

                auto f1 = for_each_iteration<ExPolicy, F, std::decay_t<Proj>>(
                    HPX_FORWARD(F, f), HPX_FORWARD(Proj, proj));

                return result_t::get(util::foreach_partitioner<ExPolicy>::call(
                    HPX_FORWARD(ExPolicy, policy), first,
                    detail::distance(first, last), HPX_MOVE(f1),
                    hpx::identity_v));
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx {

    // Note: The implementation of the non-segmented algorithms here relies on
    //       tag_fallback_invoke. For this reason the tag_invoke overloads for
    //       the segmented algorithms (and other specializations) take
    //       precedence over the implementations here. This has the advantage
    //       that the non-segmented algorithms do not need to be explicitly
    //       disabled for other, possibly external specializations.
    //
    HPX_CXX_CORE_EXPORT inline constexpr struct for_each_t final
      : hpx::detail::tag_parallel_algorithm<for_each_t>
    {
    private:
        template <typename InIter, typename F>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<InIter>
            )
        // clang-format on
        friend F tag_fallback_invoke(
            hpx::for_each_t, InIter first, InIter last, F f)
        {
            static_assert(std::input_iterator<InIter>,
                "Requires at least input iterator.");

            if (first != last)
            {
                hpx::parallel::detail::for_each<InIter>().call(
                    hpx::execution::seq, first, last, std::ref(f),
                    hpx::identity_v);
            }
            return HPX_FORWARD(F, f);
        }

        template <typename ExPolicy, typename FwdIter, typename F>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(hpx::for_each_t,
            ExPolicy&& policy, FwdIter first, FwdIter last, F f)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            return hpx::parallel::util::detail::algorithm_result<ExPolicy,
                FwdIter>::get(hpx::parallel::detail::for_each<FwdIter>()
                    .call(HPX_FORWARD(ExPolicy, policy), first, last,
                        HPX_MOVE(f), hpx::identity_v));
        }
    } for_each{};

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct for_each_n_t final
      : hpx::detail::tag_parallel_algorithm<for_each_n_t>
    {
    private:
        template <typename InIter, typename Size, typename F>
        // clang-format off
            requires (
                std::input_iterator<InIter> &&
                std::is_integral_v<Size>
            )
        // clang-format on
        friend InIter tag_fallback_invoke(
            hpx::for_each_n_t, InIter first, Size count, F f)
        {
            static_assert(std::input_iterator<InIter>,
                "Requires at least input iterator.");

            // if count is representing a negative value, we do nothing
            if (parallel::detail::is_negative(count))
            {
                return first;
            }

            return hpx::parallel::detail::for_each_n<InIter>().call(
                hpx::execution::seq, first, static_cast<std::size_t>(count),
                HPX_MOVE(f), hpx::identity_v);
        }

        template <typename ExPolicy, typename FwdIter, typename Size,
            typename F>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                std::forward_iterator<FwdIter> &&
                std::is_integral_v<Size>
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(hpx::for_each_n_t,
            ExPolicy&& policy, FwdIter first, Size count, F f)
        {
            static_assert(std::forward_iterator<FwdIter>,
                "Requires at least forward iterator.");

            constexpr bool has_scheduler_executor =
                hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

            // if count is representing a negative value, we do nothing
            if (parallel::detail::is_negative(count))
            {
                if constexpr (has_scheduler_executor)
                {
                    count = static_cast<Size>(0);
                }
                else
                {
                    using result =
                        parallel::util::detail::algorithm_result<ExPolicy,
                            FwdIter>;
                    return result::get(HPX_MOVE(first));
                }
            }

            return hpx::parallel::detail::for_each_n<FwdIter>().call(
                HPX_FORWARD(ExPolicy, policy), first,
                static_cast<std::size_t>(count), HPX_MOVE(f), hpx::identity_v);
        }
    } for_each_n{};
}    // namespace hpx

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
namespace hpx::traits {

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename F, typename Proj>
    struct get_function_address<
        parallel::detail::for_each_iteration<ExPolicy, F, Proj>>
    {
        [[nodiscard]] static constexpr std::size_t call(
            parallel::detail::for_each_iteration<ExPolicy, F, Proj> const&
                f) noexcept
        {
            return get_function_address<std::decay_t<F>>::call(f.f_);
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename F, typename Proj>
    struct get_function_annotation<
        parallel::detail::for_each_iteration<ExPolicy, F, Proj>>
    {
        [[nodiscard]] static constexpr char const* call(
            parallel::detail::for_each_iteration<ExPolicy, F, Proj> const&
                f) noexcept
        {
            return get_function_annotation<std::decay_t<F>>::call(f.f_);
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename F, typename Proj>
    struct get_function_annotation_tracing<
        parallel::detail::for_each_iteration<ExPolicy, F, Proj>>
    {
        [[nodiscard]] static hpx::tracing::annotation_handle call(
            parallel::detail::for_each_iteration<ExPolicy, F, Proj> const&
                f) noexcept
        {
            return get_function_annotation_tracing<std::decay_t<F>>::call(f.f_);
        }
    };

}    // namespace hpx::traits

#endif

#endif
