//  Copyright (c) 2007-2023 Hartmut Kaiser
//  Copyright (c) 2021 Giannis Gonidelis
//  Copyright (c) 2022 Bhumit Attarde
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/transform.hpp
/// \page hpx::transform
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {

    /// Applies the given function \a f to the range [first, last) and stores
    /// the result in another range, beginning at dest.
    ///
    /// \note   Complexity: Exactly \a last - \a first applications of \a f
    ///
    /// \tparam FwdIter1    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a transform requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).This is an
    ///                     unary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&.
    ///                     The type \a Type must be such that an object of
    ///                     type \a FwdIter1 can be dereferenced and then
    ///                     implicitly converted to \a Type. The type \a Ret
    ///                     must be such that an object of type \a FwdIter2 can
    ///                     be dereferenced and assigned a value of type
    ///                     \a Ret.
    ///
    /// \returns  The \a transform algorithm returns a \a FwdIter2.
    ///           The \a transform algorithm returns a tuple holding an iterator
    ///           referring to the first element after the input sequence and
    ///           the output iterator to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename FwdIter1, typename FwdIter2, typename F>
    FwdIter2 transform(FwdIter1 first, FwdIter1 last, FwdIter2 dest, F&& f);

    /// Applies the given function \a f to the range [first, last) and stores
    /// the result in another range, beginning at dest. Executed according to
    /// the policy.
    ///
    /// \note   Complexity: Exactly \a last - \a first applications of \a f
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the invocations of \a f.
    /// \tparam FwdIter1    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a transform requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).This is an
    ///                     unary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type &a);
    ///                     \endcode \n
    ///                     The signature does not need to have const&.
    ///                     The type \a Type must be such that an object of
    ///                     type \a FwdIter1 can be dereferenced and then
    ///                     implicitly converted to \a Type. The type \a Ret
    ///                     must be such that an object of type \a FwdIter2 can
    ///                     be dereferenced and assigned a value of type
    ///                     \a Ret.
    ///
    /// The invocations of \a f in the parallel \a transform algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The invocations of \a f in the parallel \a transform algorithm invoked
    /// with an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a transform algorithm returns a
    ///           \a hpx::future<FwdIter2>
    ///           if the execution policy is of type \a parallel_task_policy
    ///           and returns \a FwdIter2 otherwise.
    ///           The \a transform algorithm returns a tuple holding an iterator
    ///           referring to the first element after the input sequence and
    ///           the output iterator to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
        typename F>
    parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2> transform(
        ExPolicy&& policy, FwdIter1 first, FwdIter1 last, FwdIter2 dest, F&& f);

    /// Applies the given function \a f to pairs of elements from two ranges:
    /// one defined by [first1, last1) and the other beginning at first2, and
    /// stores the result in another range, beginning at dest.
    ///
    /// \note   Complexity: Exactly \a last - \a first applications of \a f
    ///
    /// \tparam FwdIter1    The type of the source iterators for the first
    ///                     range used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the source iterators for the second
    ///                     range used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam FwdIter3    The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a transform requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    ///
    /// \param first1       Refers to the beginning of the first sequence of
    ///                     elements the algorithm will be applied to.
    /// \param last1        Refers to the end of the first sequence of elements
    ///                     the algorithm will be applied to.
    /// \param first2       Refers to the beginning of the second sequence of
    ///                     elements the algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).This is a
    ///                     binary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&.
    ///                     The types \a Type1 and \a Type2 must be such that
    ///                     objects of types FwdIter1 and FwdIter2 can be
    ///                     dereferenced and then implicitly converted to
    ///                     \a Type1 and \a Type2 respectively. The type \a Ret
    ///                     must be such that an object of type \a FwdIter3 can
    ///                     be dereferenced and assigned a value of type
    ///                     \a Ret.
    ///
    /// \returns  The \a transform algorithm returns a \a FwdIter3.
    ///           The \a transform algorithm returns a tuple holding an iterator
    ///           referring to the first element after the first input sequence,
    ///           an iterator referring to the first element after the second
    ///           input sequence, and the output iterator referring to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename FwdIter1, typename FwdIter2, typename FwdIter3,
        typename F>
    FwdIter3 transform(
        FwdIter1 first1, FwdIter1 last1, FwdIter2 first2, FwdIter3 dest, F&& f);

    /// Applies the given function \a f to pairs of elements from two ranges:
    /// one defined by [first1, last1) and the other beginning at first2, and
    /// stores the result in another range, beginning at dest. Executed
    /// according to the policy.
    ///
    /// \note   Complexity: Exactly \a last - \a first applications of \a f
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the invocations of \a f.
    /// \tparam FwdIter1    The type of the source iterators for the first
    ///                     range used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam FwdIter2    The type of the source iterators for the second
    ///                     range used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam FwdIter3    The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    /// \tparam F           The type of the function/function object to use
    ///                     (deduced). Unlike its sequential form, the parallel
    ///                     overload of \a transform requires \a F to meet the
    ///                     requirements of \a CopyConstructible.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first1       Refers to the beginning of the first sequence of
    ///                     elements the algorithm will be applied to.
    /// \param last1        Refers to the end of the first sequence of elements
    ///                     the algorithm will be applied to.
    /// \param first2       Refers to the beginning of the second sequence of
    ///                     elements the algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    /// \param f            Specifies the function (or function object) which
    ///                     will be invoked for each of the elements in the
    ///                     sequence specified by [first, last).This is a
    ///                     binary predicate. The signature of this predicate
    ///                     should be equivalent to:
    ///                     \code
    ///                     Ret fun(const Type1 &a, const Type2 &b);
    ///                     \endcode \n
    ///                     The signature does not need to have const&.
    ///                     The types \a Type1 and \a Type2 must be such that
    ///                     objects of types FwdIter1 and FwdIter2 can be
    ///                     dereferenced and then implicitly converted to
    ///                     \a Type1 and \a Type2 respectively. The type \a Ret
    ///                     must be such that an object of type \a FwdIter3 can
    ///                     be dereferenced and assigned a value of type
    ///                     \a Ret.
    ///
    /// The invocations of \a f in the parallel \a transform algorithm invoked
    /// with an execution policy object of type \a sequenced_policy
    /// execute in sequential order in the calling thread.
    ///
    /// The invocations of \a f in the parallel \a transform algorithm invoked
    /// with an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an unordered
    /// fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \returns  The \a transform algorithm returns a \a hpx::future<FwdIter3>
    ///           if the execution policy is of type \a parallel_task_policy
    ///           and returns \a FwdIter3 otherwise.
    ///           The \a transform algorithm returns a tuple holding an iterator
    ///           referring to the first element after the first input sequence,
    ///           an iterator referring to the first element after the second
    ///           input sequence, and the output iterator referring to the
    ///           element in the destination range, one past the last element
    ///           copied.
    ///
    template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
        typename FwdIter3, typename F>
    parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter3> transform(
        ExPolicy&& policy, FwdIter1 first1, FwdIter1 last1, FwdIter2 first2,
        FwdIter3 dest, F&& f);

}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/foreach_partitioner.hpp>
#include <hpx/parallel/util/result_types.hpp>
#include <hpx/parallel/util/transform_loop.hpp>
#include <hpx/parallel/util/zip_iterator.hpp>

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
#include <hpx/modules/functional.hpp>
#endif

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // transform
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT template <typename F, typename Proj>
        struct transform_projected
        {
            std::decay_t<F> f_;
            std::decay_t<Proj> proj_;

            template <typename F_, typename Proj_>
            HPX_HOST_DEVICE constexpr transform_projected(
                F_&& f, Proj_&& proj) noexcept
              : f_(HPX_FORWARD(F_, f))
              , proj_(HPX_FORWARD(Proj_, proj))
            {
            }

            template <typename Iter>
            HPX_HOST_DEVICE HPX_FORCEINLINE constexpr auto operator()(Iter curr)
                -> decltype(HPX_INVOKE(f_, HPX_INVOKE(proj_, *curr)))
            {
                return HPX_INVOKE(f_, HPX_INVOKE(proj_, *curr));
            }
        };

        template <typename F>
        struct transform_projected<F, hpx::identity>
        {
            std::decay_t<F> f_{};

            template <typename F_>
            HPX_HOST_DEVICE constexpr transform_projected(
                F_&& f, hpx::identity) noexcept
              : f_(HPX_FORWARD(F_, f))
            {
            }

            template <typename Iter>
            HPX_HOST_DEVICE HPX_FORCEINLINE constexpr auto operator()(Iter curr)
                -> decltype(HPX_INVOKE(f_, *curr))
            {
                return HPX_INVOKE(f_, *curr);
            }
        };

        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename F,
            typename Proj>
        struct transform_iteration
        {
            using execution_policy_type = std::decay_t<ExPolicy>;
            using fun_type = std::decay_t<F>;
            using proj_type = std::decay_t<Proj>;

            fun_type f_;
            proj_type proj_;

            template <typename F_, typename Proj_>
            HPX_HOST_DEVICE transform_iteration(F_&& f, Proj_&& proj)
              : f_(HPX_FORWARD(F_, f))
              , proj_(HPX_FORWARD(Proj_, proj))
            {
            }

#if !defined(__NVCC__) && !defined(__CUDACC__)
            transform_iteration(transform_iteration const&) = default;
            transform_iteration(transform_iteration&&) = default;
#else
            HPX_HOST_DEVICE transform_iteration(transform_iteration const& rhs)
              : f_(rhs.f_)
              , proj_(rhs.proj_)
            {
            }

            HPX_HOST_DEVICE transform_iteration(transform_iteration&& rhs)
              : f_(HPX_MOVE(rhs.f_))
              , proj_(HPX_MOVE(rhs.proj_))
            {
            }
#endif
            transform_iteration& operator=(
                transform_iteration const&) = default;
            transform_iteration& operator=(transform_iteration&&) = default;

            ~transform_iteration() = default;

            template <typename Iter, typename F_ = fun_type>
            HPX_HOST_DEVICE HPX_FORCEINLINE
                std::pair<typename hpx::tuple_element<0,
                              typename Iter::iterator_tuple_type>::type,
                    typename hpx::tuple_element<1,
                        typename Iter::iterator_tuple_type>::type>
                operator()(Iter part_begin, std::size_t part_size, std::size_t)
            {
                auto iters = part_begin.get_iterator_tuple();
                return util::transform_loop_n<execution_policy_type>(
                    hpx::get<0>(iters), part_size, hpx::get<1>(iters),
                    transform_projected<F, Proj>(f_, proj_));
            }
        };

        template <typename ExPolicy, typename F>
        struct transform_iteration<ExPolicy, F, hpx::identity>
        {
            using execution_policy_type = std::decay_t<ExPolicy>;
            using fun_type = std::decay_t<F>;

            fun_type f_;

            template <typename F_>
            HPX_HOST_DEVICE transform_iteration(F_&& f, hpx::identity)
              : f_(HPX_FORWARD(F_, f))
            {
            }

#if !defined(__NVCC__) && !defined(__CUDACC__)
            transform_iteration(transform_iteration const&) = default;
            transform_iteration(transform_iteration&&) = default;
#else
            HPX_HOST_DEVICE transform_iteration(transform_iteration const& rhs)
              : f_(rhs.f_)
            {
            }

            HPX_HOST_DEVICE transform_iteration(transform_iteration&& rhs)
              : f_(HPX_MOVE(rhs.f_))
            {
            }
#endif
            transform_iteration& operator=(
                transform_iteration const&) = default;
            transform_iteration& operator=(transform_iteration&&) = default;

            ~transform_iteration() = default;

            template <typename Iter, typename F_ = fun_type>
            HPX_HOST_DEVICE HPX_FORCEINLINE
                std::pair<typename hpx::tuple_element<0,
                              typename Iter::iterator_tuple_type>::type,
                    typename hpx::tuple_element<1,
                        typename Iter::iterator_tuple_type>::type>
                operator()(Iter part_begin, std::size_t part_size, std::size_t)
            {
                auto iters = part_begin.get_iterator_tuple();
                return util::transform_loop_n_ind<execution_policy_type>(
                    hpx::get<0>(iters), part_size, hpx::get<1>(iters), f_);
            }
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename IterPair>
        struct transform : public algorithm<transform<IterPair>, IterPair>
        {
            constexpr transform() noexcept
              : algorithm<transform, IterPair>("transform")
            {
            }

            // sequential execution with non-trivial projection
            template <typename ExPolicy, typename InIterB, typename InIterE,
                typename OutIter, typename F, typename Proj>
            HPX_HOST_DEVICE static constexpr util::in_out_result<InIterB,
                OutIter>
            sequential(ExPolicy&& policy, InIterB first, InIterE last,
                OutIter dest, F&& f, Proj&& proj)
            {
                return util::transform_loop(HPX_FORWARD(ExPolicy, policy),
                    first, last, dest,
                    transform_projected<F, Proj>(
                        HPX_FORWARD(F, f), HPX_FORWARD(Proj, proj)));
            }

            // sequential execution without projection
            template <typename ExPolicy, typename InIterB, typename InIterE,
                typename OutIter, typename F>
            HPX_HOST_DEVICE static constexpr util::in_out_result<InIterB,
                OutIter>
            sequential(ExPolicy&& policy, InIterB first, InIterE last,
                OutIter dest, F&& f, hpx::identity)
            {
                return util::transform_loop_ind(HPX_FORWARD(ExPolicy, policy),
                    first, last, dest, HPX_FORWARD(F, f));
            }

            template <typename ExPolicy, typename FwdIter1B, typename FwdIter1E,
                typename FwdIter2, typename F, typename Proj>
            static decltype(auto) parallel(ExPolicy&& policy, FwdIter1B first,
                FwdIter1E last, FwdIter2 dest, F&& f, Proj&& proj)
            {
                using result_type = util::in_out_result<FwdIter1B, FwdIter2>;
                using algorithm_result =
                    util::detail::algorithm_result<ExPolicy, result_type>;

                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                if constexpr (!has_scheduler_executor)
                {
                    if (first == last)
                    {
                        return algorithm_result::get(
                            result_type{HPX_MOVE(first), HPX_MOVE(dest)});
                    }
                }

                auto f1 = transform_iteration<ExPolicy, F, Proj>(
                    HPX_FORWARD(F, f), HPX_FORWARD(Proj, proj));

                return util::detail::get_in_out_result(
                    util::foreach_partitioner<ExPolicy>::call(
                        HPX_FORWARD(ExPolicy, policy),
                        hpx::util::zip_iterator(first, dest),
                        detail::distance(first, last), HPX_MOVE(f1),
                        hpx::identity_v));
            }
        };
        /// \endcond
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // transform binary predicate
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT template <typename F, typename Proj1,
            typename Proj2>
        struct transform_binary_projected
        {
            std::decay_t<F> f_;
            std::decay_t<Proj1> proj1_;
            std::decay_t<Proj2> proj2_;

            template <typename F_, typename Proj1_, typename Proj2_>
            HPX_HOST_DEVICE constexpr transform_binary_projected(
                F_&& f, Proj1_&& proj1, Proj2_&& proj2)
              : f_(HPX_FORWARD(F_, f))
              , proj1_(HPX_FORWARD(Proj1_, proj1))
              , proj2_(HPX_FORWARD(Proj2_, proj2))
            {
            }

            template <typename Iter1, typename Iter2>
            HPX_HOST_DEVICE HPX_FORCEINLINE auto operator()(
                Iter1 curr1, Iter2 curr2)
            {
                return HPX_INVOKE(
                    f_, HPX_INVOKE(proj1_, *curr1), HPX_INVOKE(proj2_, *curr2));
            }
        };

        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename F,
            typename Proj1, typename Proj2>
        struct transform_binary_iteration
        {
            using execution_policy_type = std::decay_t<ExPolicy>;
            using fun_type = std::decay_t<F>;
            using proj1_type = std::decay_t<Proj1>;
            using proj2_type = std::decay_t<Proj2>;

            fun_type f_;
            proj1_type proj1_;
            proj2_type proj2_;

            template <typename F_, typename Proj1_, typename Proj2_>
            HPX_HOST_DEVICE transform_binary_iteration(
                F_&& f, Proj1_&& proj1, Proj2_&& proj2)
              : f_(HPX_FORWARD(F_, f))
              , proj1_(HPX_FORWARD(Proj1_, proj1))
              , proj2_(HPX_FORWARD(Proj2_, proj2))
            {
            }

#if !defined(__NVCC__) && !defined(__CUDACC__)
            transform_binary_iteration(
                transform_binary_iteration const&) = default;
            transform_binary_iteration(transform_binary_iteration&&) = default;
#else
            HPX_HOST_DEVICE
            transform_binary_iteration(transform_binary_iteration const& rhs)
              : f_(rhs.f_)
              , proj1_(rhs.proj1_)
              , proj2_(rhs.proj2_)
            {
            }

            HPX_HOST_DEVICE
            transform_binary_iteration(transform_binary_iteration&& rhs)
              : f_(HPX_MOVE(rhs.f_))
              , proj1_(HPX_MOVE(rhs.proj1_))
              , proj2_(HPX_MOVE(rhs.proj2_))
            {
            }
#endif
            transform_binary_iteration& operator=(
                transform_binary_iteration const&) = default;
            transform_binary_iteration& operator=(
                transform_binary_iteration&&) = default;

            ~transform_binary_iteration() = default;

            template <typename Iter, typename F_ = fun_type>
            HPX_HOST_DEVICE HPX_FORCEINLINE
                hpx::tuple<typename hpx::tuple_element<0,
                               typename Iter::iterator_tuple_type>::type,
                    typename hpx::tuple_element<1,
                        typename Iter::iterator_tuple_type>::type,
                    typename hpx::tuple_element<2,
                        typename Iter::iterator_tuple_type>::type>
                operator()(Iter part_begin, std::size_t part_size, std::size_t)
            {
                auto iters = part_begin.get_iterator_tuple();
                return util::transform_binary_loop_n<execution_policy_type>(
                    hpx::get<0>(iters), part_size, hpx::get<1>(iters),
                    hpx::get<2>(iters),
                    transform_binary_projected<F_, Proj1, Proj2>{
                        f_, proj1_, proj2_});
            }

            HPX_HOST_DEVICE HPX_FORCEINLINE constexpr void operator()(
                std::size_t) const noexcept
            {
                HPX_ASSERT(false);
            }
        };

        template <typename ExPolicy, typename F>
        struct transform_binary_iteration<ExPolicy, F, hpx::identity,
            hpx::identity>
        {
            using execution_policy_type = std::decay_t<ExPolicy>;
            using fun_type = std::decay_t<F>;

            fun_type f_;

            template <typename F_>
            HPX_HOST_DEVICE transform_binary_iteration(
                F_&& f, hpx::identity, hpx::identity)
              : f_(HPX_FORWARD(F_, f))
            {
            }

#if !defined(__NVCC__) && !defined(__CUDACC__)
            transform_binary_iteration(
                transform_binary_iteration const&) = default;
            transform_binary_iteration(transform_binary_iteration&&) = default;
#else
            HPX_HOST_DEVICE
            transform_binary_iteration(transform_binary_iteration const& rhs)
              : f_(rhs.f_)
            {
            }

            HPX_HOST_DEVICE
            transform_binary_iteration(transform_binary_iteration&& rhs)
              : f_(HPX_MOVE(rhs.f_))
            {
            }
#endif
            transform_binary_iteration& operator=(
                transform_binary_iteration const&) = default;
            transform_binary_iteration& operator=(
                transform_binary_iteration&&) = default;

            ~transform_binary_iteration() = default;

            template <typename Iter, typename F_ = fun_type>
            HPX_HOST_DEVICE HPX_FORCEINLINE
                hpx::tuple<typename hpx::tuple_element<0,
                               typename Iter::iterator_tuple_type>::type,
                    typename hpx::tuple_element<1,
                        typename Iter::iterator_tuple_type>::type,
                    typename hpx::tuple_element<2,
                        typename Iter::iterator_tuple_type>::type>
                operator()(Iter part_begin, std::size_t part_size, std::size_t)
            {
                auto iters = part_begin.get_iterator_tuple();
                return util::transform_binary_loop_ind_n<execution_policy_type>(
                    hpx::get<0>(iters), part_size, hpx::get<1>(iters),
                    hpx::get<2>(iters), f_);
            }

            HPX_HOST_DEVICE HPX_FORCEINLINE constexpr void operator()(
                std::size_t) const noexcept
            {
                HPX_ASSERT(false);
            }
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename IterTuple>
        struct transform_binary
          : public algorithm<transform_binary<IterTuple>, IterTuple>
        {
            constexpr transform_binary() noexcept
              : algorithm<transform_binary, IterTuple>("transform_binary")
            {
            }

            // sequential execution with non-trivial projection
            template <typename ExPolicy, typename InIter1, typename InIter2,
                typename OutIter, typename F, typename Proj1, typename Proj2>
            static constexpr util::in_in_out_result<InIter1, InIter2, OutIter>
            sequential(ExPolicy&&, InIter1 first1, InIter1 last1,
                InIter2 first2, OutIter dest, F&& f, Proj1&& proj1,
                Proj2&& proj2)
            {
                return util::transform_binary_loop<ExPolicy>(first1, last1,
                    first2, dest,
                    transform_binary_projected<F, Proj1, Proj2>{
                        f, proj1, proj2});
            }

            // sequential execution without projection
            template <typename ExPolicy, typename InIter1, typename InIter2,
                typename OutIter, typename F>
            static constexpr util::in_in_out_result<InIter1, InIter2, OutIter>
            sequential(ExPolicy&&, InIter1 first1, InIter1 last1,
                InIter2 first2, OutIter dest, F&& f, hpx::identity,
                hpx::identity)
            {
                return util::transform_binary_loop_ind<ExPolicy>(
                    first1, last1, first2, dest, f);
            }

            template <typename ExPolicy, typename FwdIter1B, typename FwdIter1E,
                typename FwdIter2, typename FwdIter3, typename F,
                typename Proj1, typename Proj2>
            static decltype(auto) parallel(ExPolicy&& policy, FwdIter1B first1,
                FwdIter1E last1, FwdIter2 first2, FwdIter3 dest, F&& f,
                Proj1&& proj1, Proj2&& proj2)
            {
                using result_type =
                    util::in_in_out_result<FwdIter1B, FwdIter2, FwdIter3>;
                using algorithm_result =
                    util::detail::algorithm_result<ExPolicy, result_type>;

                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                if constexpr (!has_scheduler_executor)
                {
                    if (first1 == last1)
                    {
                        return algorithm_result::get(
                            result_type{HPX_MOVE(first1), HPX_MOVE(first2),
                                HPX_MOVE(dest)});
                    }
                }

                auto f1 = transform_binary_iteration<ExPolicy, F, Proj1, Proj2>(
                    HPX_FORWARD(F, f), HPX_FORWARD(Proj1, proj1),
                    HPX_FORWARD(Proj2, proj2));

                return util::detail::get_in_in_out_result(
                    util::foreach_partitioner<ExPolicy>::call(
                        HPX_FORWARD(ExPolicy, policy),
                        hpx::util::zip_iterator(first1, first2, dest),
                        detail::distance(first1, last1), HPX_MOVE(f1),
                        hpx::identity_v));
            }
        };
        /// \endcond
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // transform binary predicate
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT template <typename IterTuple>
        struct transform_binary2
          : public algorithm<transform_binary2<IterTuple>, IterTuple>
        {
            constexpr transform_binary2() noexcept
              : algorithm<transform_binary2, IterTuple>("transform_binary")
            {
            }

            // sequential execution with non-trivial projection
            template <typename ExPolicy, typename InIter1, typename InIter2,
                typename OutIter, typename F, typename Proj1, typename Proj2>
            static constexpr util::in_in_out_result<InIter1, InIter2, OutIter>
            sequential(ExPolicy&&, InIter1 first1, InIter1 last1,
                InIter2 first2, InIter2 last2, OutIter dest, F&& f,
                Proj1&& proj1, Proj2&& proj2)
            {
                return util::transform_binary_loop<ExPolicy>(first1, last1,
                    first2, last2, dest,
                    transform_binary_projected<F, Proj1, Proj2>{
                        f, proj1, proj2});
            }

            // sequential execution without projection
            template <typename ExPolicy, typename InIter1, typename InIter2,
                typename OutIter, typename F>
            static constexpr util::in_in_out_result<InIter1, InIter2, OutIter>
            sequential(ExPolicy&&, InIter1 first1, InIter1 last1,
                InIter2 first2, InIter2 last2, OutIter dest, F&& f,
                hpx::identity, hpx::identity)
            {
                return util::transform_binary_loop_ind<ExPolicy>(
                    first1, last1, first2, last2, dest, f);
            }

            template <typename ExPolicy, typename FwdIter1B, typename FwdIter1E,
                typename FwdIter2B, typename FwdIter2E, typename FwdIter3,
                typename F, typename Proj1, typename Proj2>
            static decltype(auto) parallel(ExPolicy&& policy, FwdIter1B first1,
                FwdIter1E last1, FwdIter2B first2, FwdIter2E last2,
                FwdIter3 dest, F&& f, Proj1&& proj1, Proj2&& proj2)
            {
                using result_type =
                    util::in_in_out_result<FwdIter1B, FwdIter2B, FwdIter3>;
                using algorithm_result =
                    util::detail::algorithm_result<ExPolicy, result_type>;

                constexpr bool has_scheduler_executor =
                    hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

                if constexpr (!has_scheduler_executor)
                {
                    if (first1 == last1 || first2 == last2)
                    {
                        return algorithm_result::get(
                            result_type{HPX_MOVE(first1), HPX_MOVE(first2),
                                HPX_MOVE(dest)});
                    }
                }

                auto f1 = transform_binary_iteration<ExPolicy, F, Proj1, Proj2>(
                    HPX_FORWARD(F, f), HPX_FORWARD(Proj1, proj1),
                    HPX_FORWARD(Proj2, proj2));

                // different versions of clang-format do different things
                // clang-format off
                return util::detail::get_in_in_out_result(
                    util::foreach_partitioner<ExPolicy>::call(
                        HPX_FORWARD(ExPolicy, policy),
                        hpx::util::zip_iterator(first1, first2, dest),
                        (std::min) (detail::distance(first1, last1),
                            detail::distance(first2, last2)),
                        HPX_MOVE(f1), hpx::identity_v));
                // clang-format on
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
namespace hpx::traits {

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename F, typename Proj>
    struct get_function_address<
        parallel::detail::transform_iteration<ExPolicy, F, Proj>>
    {
        [[nodiscard]] static constexpr std::size_t call(
            parallel::detail::transform_iteration<ExPolicy, F, Proj> const&
                f) noexcept
        {
            return get_function_address<std::decay_t<F>>::call(f.f_);
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename F, typename Proj>
    struct get_function_annotation<
        parallel::detail::transform_iteration<ExPolicy, F, Proj>>
    {
        [[nodiscard]] static constexpr char const* call(
            parallel::detail::transform_iteration<ExPolicy, F, Proj> const&
                f) noexcept
        {
            return get_function_annotation<std::decay_t<F>>::call(f.f_);
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename F, typename Proj>
    struct get_function_annotation_tracing<
        parallel::detail::transform_iteration<ExPolicy, F, Proj>>
    {
        [[nodiscard]] static hpx::tracing::annotation_handle call(
            parallel::detail::transform_iteration<ExPolicy, F, Proj> const&
                f) noexcept
        {
            return get_function_annotation_tracing<std::decay_t<F>>::call(f.f_);
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename F, typename Proj1,
        typename Proj2>
    struct get_function_address<
        parallel::detail::transform_binary_iteration<ExPolicy, F, Proj1, Proj2>>
    {
        [[nodiscard]] static constexpr std::size_t call(
            parallel::detail::transform_binary_iteration<ExPolicy, F, Proj1,
                Proj2> const& f) noexcept
        {
            return get_function_address<std::decay_t<F>>::call(f.f_);
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename F, typename Proj1,
        typename Proj2>
    struct get_function_annotation<
        parallel::detail::transform_binary_iteration<ExPolicy, F, Proj1, Proj2>>
    {
        [[nodiscard]] static constexpr char const* call(
            parallel::detail::transform_binary_iteration<ExPolicy, F, Proj1,
                Proj2> const& f) noexcept
        {
            return get_function_annotation<std::decay_t<F>>::call(f.f_);
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename F, typename Proj1,
        typename Proj2>
    struct get_function_annotation_tracing<
        parallel::detail::transform_binary_iteration<ExPolicy, F, Proj1, Proj2>>
    {
        [[nodiscard]] static hpx::tracing::annotation_handle call(
            parallel::detail::transform_binary_iteration<ExPolicy, F, Proj1,
                Proj2> const& f) noexcept
        {
            return get_function_annotation_tracing<std::decay_t<F>>::call(f.f_);
        }
    };
}    // namespace hpx::traits

#endif

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::transform
    HPX_CXX_CORE_EXPORT inline constexpr struct transform_t final
      : hpx::detail::tag_parallel_algorithm<transform_t>
    {
    private:
        template <typename FwdIter1, typename FwdIter2, typename F>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2>
            )
        // clang-format on
        friend FwdIter2 tag_fallback_invoke(
            hpx::transform_t, FwdIter1 first, FwdIter1 last, FwdIter2 dest, F f)
        {
            static_assert(std::input_iterator<FwdIter1>,
                "Requires at least input iterator.");

            return parallel::util::get_second_element(
                parallel::detail::transform<
                    hpx::parallel::util::in_out_result<FwdIter1, FwdIter2>>()
                    .call(hpx::execution::seq, first, last, dest, HPX_MOVE(f),
                        hpx::identity_v));
        }

        template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
            typename F>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2>
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(hpx::transform_t,
            ExPolicy&& policy, FwdIter1 first, FwdIter1 last, FwdIter2 dest,
            F f)
        {
            static_assert(std::forward_iterator<FwdIter1>,
                "Requires at least forward iterator.");

            return parallel::util::get_second_element(
                parallel::detail::transform<
                    hpx::parallel::util::in_out_result<FwdIter1, FwdIter2>>()
                    .call(HPX_FORWARD(ExPolicy, policy), first, last, dest,
                        HPX_MOVE(f), hpx::identity_v));
        }

        template <typename FwdIter1, typename FwdIter2, typename FwdIter3,
            typename F>
            requires(hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                hpx::traits::is_iterator_v<FwdIter3>)
        // clang-format on
        friend FwdIter3 tag_fallback_invoke(hpx::transform_t, FwdIter1 first1,
            FwdIter1 last1, FwdIter2 first2, FwdIter3 dest, F f)
        {
            static_assert(
                std::input_iterator<FwdIter1> && std::input_iterator<FwdIter2>,
                "Requires at least input iterator.");

            using proj_id = hpx::identity;
            using result_type = hpx::parallel::util::in_in_out_result<FwdIter1,
                FwdIter2, FwdIter3>;

            return parallel::util::get_third_element(
                parallel::detail::transform_binary<result_type>().call(
                    hpx::execution::seq, first1, last1, first2, dest,
                    HPX_MOVE(f), proj_id(), proj_id()));
        }

        template <typename ExPolicy, typename FwdIter1, typename FwdIter2,
            typename FwdIter3, typename F>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<FwdIter1> &&
                hpx::traits::is_iterator_v<FwdIter2> &&
                hpx::traits::is_iterator_v<FwdIter3>
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(hpx::transform_t,
            ExPolicy&& policy, FwdIter1 first1, FwdIter1 last1, FwdIter2 first2,
            FwdIter3 dest, F f)
        {
            static_assert(
                std::input_iterator<FwdIter1> && std::input_iterator<FwdIter2>,
                "Requires at least input iterator.");

            using proj_id = hpx::identity;
            using result_type = hpx::parallel::util::in_in_out_result<FwdIter1,
                FwdIter2, FwdIter3>;

            return parallel::util::get_third_element(
                parallel::detail::transform_binary<result_type>().call(
                    HPX_FORWARD(ExPolicy, policy), first1, last1, first2, dest,
                    HPX_MOVE(f), proj_id(), proj_id()));
        }
    } transform{};
}    // namespace hpx

#endif    // DOXYGEN
