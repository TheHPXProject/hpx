//  Copyright (c) 2014-2023 Hartmut Kaiser
//  Copyright (c) 2023 Isidoros Tsaousis-Seiras
//  Copyright (c) 2026 Arivoli Ramamoorthy
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/uninitialized_relocate.hpp
/// \page hpx::uninitialized_relocate, hpx::uninitialized_relocate_n
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx {

    /// Relocates the elements in the range, defined by [first, last), to an
    /// uninitialized memory area beginning at \a dest. If an exception is
    /// thrown during the move-construction of an element, all elements left
    /// in the input range, as well as all objects already constructed in the
    /// destination range are destroyed. After this algorithm completes, the
    /// source range should be freed or reused without destroying the objects.
    ///
    /// \note   Complexity: time: O(n), space: O(1)
    ///         1)  For "trivially relocatable" underlying types (T) and
    ///             a contiguous iterator range [first, last):
    ///             std::distance(first, last)*sizeof(T) bytes are copied.
    ///         2)  For "trivially relocatable" underlying types (T) and
    ///             a non-contiguous iterator range [first, last):
    ///             std::distance(first, last) memory copies of sizeof(T)
    ///             bytes each are performed.
    ///         3)  For "non-trivially relocatable" underlying types (T):
    ///             std::distance(first, last) move assignments and
    ///             destructions are performed.
    ///
    /// \note   Declare a type as "trivially relocatable" using the
    ///         `HPX_DECLARE_TRIVIALLY_RELOCATABLE` macros found in
    ///         <hpx/type_support/is_trivially_relocatable.hpp>.
    ///
    /// \tparam InIter1     The type of the source iterator first (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam InIter2     The type of the source iterator last (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam FwdIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    ///
    /// The assignments in the parallel \a uninitialized_relocate algorithm invoked
    /// without an execution policy object will execute in sequential order in
    /// the calling thread.
    ///
    /// \returns  The \a uninitialized_relocate algorithm returns \a FwdIter.
    ///           The \a uninitialized_relocate algorithm returns the output
    ///           iterator to the element in the destination range, one past
    ///           the last element relocated.
    ///
    template <typename InIter1, typename InIter2, typename FwdIter>
    FwdIter uninitialized_relocate(InIter1 first, InIter2 last, FwdIter dest);

    /// Relocates the elements in the range defined by [first, last), to an
    /// uninitialized memory area beginning at \a dest. If an exception is
    /// thrown during the move-construction of an element, all elements left
    /// in the input range, as well as all objects already constructed in the
    /// destination range are destroyed. After this algorithm completes, the
    /// source range should be freed or reused without destroying the objects.
    ///
    /// \note   Complexity: time: O(n), space: O(1)
    ///         1)  For "trivially relocatable" underlying types (T) and
    ///             a contiguous iterator range [first, last):
    ///             std::distance(first, last)*sizeof(T) bytes are copied.
    ///         2)  For "trivially relocatable" underlying types (T) and
    ///             a non-contiguous iterator range [first, last):
    ///             std::distance(first, last) memory copies of sizeof(T)
    ///             bytes each are performed.
    ///         3)  For "non-trivially relocatable" underlying types (T):
    ///             std::distance(first, last) move assignments and
    ///             destructions are performed.
    ///
    /// \note   Declare a type as "trivially relocatable" using the
    ///         `HPX_DECLARE_TRIVIALLY_RELOCATABLE` macros found in
    ///         <hpx/type_support/is_trivially_relocatable.hpp>.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam InIter1     The type of the source iterator first (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam InIter2     The type of the source iterator last (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam FwdIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of a
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
    //// The assignments in the parallel \a uninitialized_relocate_n algorithm
    /// invoked with an execution policy object of type
    /// \a sequenced_policy execute in sequential order in the
    /// calling thread.
    ///
    /// The assignments in the parallel \a uninitialized_relocate algorithm invoked
    /// with an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an
    /// unordered fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \note   Parallel execution requires both \a first and \a dest to be
    ///         contiguous iterators. If either iterator is non-contiguous,
    ///         the algorithm cannot determine whether the source and destination
    ///         ranges overlap, and will silently fall back to sequential
    ///         execution regardless of the execution policy provided.
    ///
    /// \returns  The \a uninitialized_relocate algorithm returns a
    ///           \a hpx::future<FwdIter>, if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter otherwise.
    ///           The \a uninitialized_relocate algorithm returns the output
    ///           iterator to the element in the destination range, one past
    ///           the last element relocated.
    ///
    template <typename ExPolicy, typename InIter1, typename InIter2,
        typename FwdIter>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter>
    uninitialized_relocate(
        ExPolicy&& policy, InIter1 first, InIter2 last, FwdIter dest);

    /// Relocates the elements in the range, defined by [first, last), to an
    /// uninitialized memory area ending at \a dest_last. The objects are
    /// processed in reverse order. If an exception is thrown during the
    /// the move-construction of an element, all elements left in the
    /// input range, as well as all objects already constructed in the
    /// destination range are destroyed. After this algorithm completes, the
    /// source range should be freed or reused without destroying the objects.
    ///
    /// \note   Complexity: time: O(n), space: O(1)
    ///         1)  For "trivially relocatable" underlying types (T) and
    ///             a contiguous iterator range [first, last):
    ///             std::distance(first, last)*sizeof(T) bytes are copied.
    ///         2)  For "trivially relocatable" underlying types (T) and
    ///             a non-contiguous iterator range [first, last):
    ///             std::distance(first, last) memory copies of sizeof(T)
    ///             bytes each are performed.
    ///         3)  For "non-trivially relocatable" underlying types (T):
    ///             std::distance(first, last) move assignments and
    ///             destructions are performed.
    ///
    /// \note   Declare a type as "trivially relocatable" using the
    ///         `HPX_DECLARE_TRIVIALLY_RELOCATABLE` macros found in
    ///         <hpx/type_support/is_trivially_relocatable.hpp>.
    ///
    /// \tparam BiIter1     The type of the source range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     Bidirectional iterator.
    /// \tparam BiIter2     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     Bidirectional iterator.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest_last    Refers to the beginning of the destination range.
    ///
    /// The assignments in the parallel \a uninitialized_relocate algorithm invoked
    /// without an execution policy object will execute in sequential order in
    /// the calling thread.
    ///
    /// \returns  The \a uninitialized_relocate_backward algorithm returns \a BiIter2.
    ///           The \a uninitialized_relocate_backward algorithm returns the
    ///           bidirectional iterator to the first element in the destination range.
    ///
    template <typename BiIter1, typename BiIter2>
    BiIter2 uninitialized_relocate_backward(
        BiIter1 first, BiIter1 last, BiIter2 dest_last);

    /// Relocates the elements in the range, defined by [first, last), to an
    /// uninitialized memory area ending at \a dest_last. The order of the
    /// relocation of the objects depends on the execution policy. If an
    /// exception is thrown during the  the move-construction of an element,
    /// all elements left in the input range, as well as all objects already
    /// constructed in the destination range are destroyed. After this algorithm
    /// completes, the source range should be freed or reused without destroying
    /// the objects.
    ///
    /// \note   Using the \a uninitialized_relocate_backward algorithm with the
    ///         with a non-sequenced execution policy, will not guarantee the
    ///         order of the relocation of the objects.
    ///
    /// \note   Complexity: time: O(n), space: O(1)
    ///         1)  For "trivially relocatable" underlying types (T) and
    ///             a contiguous iterator range [first, last):
    ///             std::distance(first, last)*sizeof(T) bytes are copied.
    ///         2)  For "trivially relocatable" underlying types (T) and
    ///             a non-contiguous iterator range [first, last):
    ///             std::distance(first, last) memory copies of sizeof(T)
    ///             bytes each are performed.
    ///         3)  For "non-trivially relocatable" underlying types (T):
    ///             std::distance(first, last) move assignments and
    ///             destructions are performed.
    ///
    /// \note   Declare a type as "trivially relocatable" using the
    ///         `HPX_DECLARE_TRIVIALLY_RELOCATABLE` macros found in
    ///         <hpx/type_support/is_trivially_relocatable.hpp>.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam BiIter1     The type of the source range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     Bidirectional iterator.
    /// \tparam BiIter2     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     Bidirectional iterator.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param dest_last    Refers to the end of the destination range.
    ///
    /// The assignments in the parallel \a uninitialized_relocate_backward algorithm invoked
    /// with an execution policy object of type \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an
    /// unordered fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \note   Parallel execution requires both \a first and \a dest_last to
    ///         be contiguous iterators. If either iterator is non-contiguous,
    ///         the algorithm cannot determine whether the source and destination
    ///         ranges overlap, and will silently fall back to sequential
    ///         execution regardless of the execution policy provided.
    ///
    /// \returns  The \a uninitialized_relocate_backward algorithm returns a
    ///           \a hpx::future<FwdIter>, if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a BiIter2 otherwise.
    ///           The \a uninitialized_relocate_backward algorithm returns the
    ///           bidirectional iterator to the first element in the destination
    ///           range.
    ///
    template <typename ExPolicy, typename BiIter1, typename BiIter2>
    hpx::parallel::util::detail::algorithm_result<ExPolicy, BiIter2>
    uninitialized_relocate_backward(
        ExPolicy&& policy, BiIter1 first, BiIter1 last, BiIter2 dest_last);

    /// Relocates the elements in the range, defined by [first, last), to an
    /// uninitialized memory area beginning at \a dest. If an exception is
    /// thrown during the move-construction of an element, all elements left
    /// in the input range, as well as all objects already constructed in the
    /// destination range are destroyed. After this algorithm completes, the
    /// source range should be freed or reused without destroying the objects.
    ///
    /// \note   Complexity: time: O(n), space: O(1)
    ///         1)  For "trivially relocatable" underlying types (T) and
    ///             a contiguous iterator range [first, first+count):
    ///             `count*sizeof(T)` bytes are copied.
    ///         2)  For "trivially relocatable" underlying types (T) and
    ///             a non-contiguous iterator range [first, first+count):
    ///             `count` memory copies of sizeof(T) bytes each are performed.
    ///         3)  For "non-trivially relocatable" underlying types (T):
    ///             `count` move assignments and destructions are performed.
    ///
    /// \note   Declare a type as "trivially relocatable" using the
    ///         `HPX_DECLARE_TRIVIALLY_RELOCATABLE` macros found in
    ///         <hpx/type_support/is_trivially_relocatable.hpp>.
    ///
    /// \tparam InIter      The type of the source iterator first (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam Size        The type of the argument specifying the number of
    ///                     elements to relocate.
    /// \tparam FwdIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     forward iterator.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param count        Refers to the number of elements starting at
    ///                     \a first the algorithm will be applied to.
    /// \param dest         Refers to the beginning of the destination range.
    ///
    /// The assignments in the parallel \a uninitialized_relocate_n algorithm
    /// invoked without an execution policy object will execute in sequential
    /// order in the calling thread.
    ///
    /// \returns  The \a uninitialized_relocate_n algorithm returns \a FwdIter.
    ///           The \a uninitialized_relocate_n algorithm returns the output
    ///           iterator to the element in the destination range, one past
    ///           the last element relocated.
    ///
    template <typename InIter, typename Size, typename FwdIter>
    FwdIter uninitialized_relocate_n(InIter first, Size count, FwdIter dest);

    /// Relocates the elements in the range, defined by [first, last), to an
    /// uninitialized memory area beginning at \a dest. If an exception is
    /// thrown during the move-construction of an element, all elements left
    /// in the input range, as well as all objects already constructed in the
    /// destination range are destroyed. After this algorithm completes, the
    /// source range should be freed or reused without destroying the objects.
    ///
    /// \note   Complexity: time: O(n), space: O(1)
    ///         1)  For "trivially relocatable" underlying types (T) and
    ///             a contiguous iterator range [first, first+count):
    ///             `count*sizeof(T)` bytes are copied.
    ///         2)  For "trivially relocatable" underlying types (T) and
    ///             a non-contiguous iterator range [first, first+count):
    ///             `count` memory copies of sizeof(T) bytes each are performed.
    ///         3)  For "non-trivially relocatable" underlying types (T):
    ///             `count` move assignments and destructions are performed.
    ///
    /// \note   Declare a type as "trivially relocatable" using the
    ///         `HPX_DECLARE_TRIVIALLY_RELOCATABLE` macros found in
    ///         <hpx/type_support/is_trivially_relocatable.hpp>.
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam InIter      The type of the source iterator first (deduced).
    ///                     This iterator type must meet the requirements of an
    ///                     input iterator.
    /// \tparam Size        The type of the argument specifying the number of
    ///                     elements to relocate.
    /// \tparam FwdIter     The type of the iterator representing the
    ///                     destination range (deduced).
    ///                     This iterator type must meet the requirements of a
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
    /// The assignments in the parallel \a uninitialized_relocate_n algorithm
    /// invoked with an execution policy object of type
    /// \a sequenced_policy execute in sequential order in the
    /// calling thread.
    ///
    /// The assignments in the parallel \a uninitialized_relocate_n algorithm
    /// invoked with an execution policy object of type
    /// \a parallel_policy or
    /// \a parallel_task_policy are permitted to execute in an
    /// unordered fashion in unspecified threads, and indeterminately sequenced
    /// within each thread.
    ///
    /// \note   Parallel execution requires both \a first and \a dest to be
    ///         contiguous iterators. If either iterator is non-contiguous,
    ///         the algorithm cannot determine whether the source and destination
    ///         ranges overlap, and will silently fall back to sequential
    ///         execution regardless of the execution policy provided.
    ///
    /// \returns  The \a uninitialized_relocate_n algorithm returns a
    ///           \a hpx::future<FwdIter> if the execution policy is of type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and
    ///           returns \a FwdIter otherwise.
    ///           The \a uninitialized_relocate_n algorithm returns the output
    ///           iterator to the element in the destination range, one past
    ///           the last element relocated.
    ///
    template <typename ExPolicy, typename InIter, typename Size,
        typename FwdIter>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter>
    uninitialized_relocate_n(
        ExPolicy&& policy, InIter first, Size count, FwdIter dest);
}    // namespace hpx

#else    // DOXYGEN

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/pointer_category.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/unseq/loop.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/clear_container.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/partitioner_with_cleanup.hpp>
#include <hpx/parallel/util/result_types.hpp>
#include <hpx/parallel/util/zip_iterator.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace hpx::parallel {

    ///////////////////////////////////////////////////////////////////////////
    // uninitialized_relocate
    namespace detail {
        /// \cond NOINTERNAL

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter,
            typename FwdIter, typename Size>
            requires(std::input_iterator<InIter> &&
                std::forward_iterator<FwdIter> && std::is_integral_v<Size>)
        decltype(auto) parallel_uninitialized_relocate_n(
            ExPolicy&& policy, InIter first, Size count, FwdIter dest)
        {
            constexpr bool has_scheduler_executor =
                hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

            if constexpr (!has_scheduler_executor)
            {
                if (count == 0)
                {
                    return util::detail::algorithm_result<ExPolicy,
                        util::in_out_result<InIter, FwdIter>>::
                        get(util::in_out_result<InIter, FwdIter>{first, dest});
                }
            }

            using zip_iter = ::hpx::util::zip_iterator<InIter, FwdIter>;
            using partition_result_type = std::pair<FwdIter, FwdIter>;

            return util::partitioner_with_cleanup<ExPolicy,
                util::in_out_result<InIter, FwdIter>, partition_result_type>::
                call(
                    HPX_FORWARD(ExPolicy, policy), zip_iter(first, dest), count,
                    [](zip_iter t, std::size_t part_size) mutable
                        -> partition_result_type {
                        using hpx::get;

                        auto iters = t.get_iterator_tuple();

                        InIter part_source = get<0>(iters);
                        FwdIter part_dest = get<1>(iters);

                        auto [part_source_advanced, part_dest_advanced] =
                            hpx::experimental::util::
                                uninitialized_relocate_n_primitive(
                                    part_source, part_size, part_dest);

                        // returns (dest begin, dest end)
                        return std::make_pair(part_dest, part_dest_advanced);
                    },
                    // finalize, called once if no error occurred
                    [first, dest, count](auto&&... data) mutable
                        -> util::in_out_result<InIter, FwdIter> {
                        static_assert(sizeof...(data) < 2);

                        // make sure iterators embedded in function object that is
                        // attached to futures are invalidated
                        util::detail::clear_container(data...);

                        std::advance(first, count);
                        std::advance(dest, count);
                        return util::in_out_result<InIter, FwdIter>{
                            first, dest};
                    },
                    // cleanup function, called for each partition which
                    // didn't fail, but only if at least one failed
                    [](partition_result_type&& r) -> void {
                        std::destroy(r.first, r.second);
                    });
        }

        ///////////////////////////////////////////////////////////////////////
        // parallel_uninitialized_relocate_n_overlap
        //
        // Decomposes [first, first+n) into d independent chains, where d is
        // the overlap distance (distance from first to dest). Chain c covers
        // elements at positions c, c+d, c+2d, .... Elements within a chain
        // are relocated in order (front-to-back) to avoid overwriting a
        // source before it has been relocated. Chains are independent of each
        // other and are distributed across threads by the partitioner.
        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter,
            typename FwdIter>
            requires(hpx::traits::is_contiguous_iterator_v<InIter> &&
                hpx::traits::is_contiguous_iterator_v<FwdIter>)
        decltype(auto) parallel_uninitialized_relocate_n_overlap(
            ExPolicy&& policy, InIter first, std::size_t n, FwdIter dest,
            std::size_t d)
        {
            using value_type = std::iter_value_t<InIter>;
            value_type* const first_ptr = std::to_address(first);
            value_type* const dest_ptr = std::to_address(dest);

            struct chain_group_result
            {
                std::size_t chain_start;
                std::size_t chains_done;
            };

            using counting_iter = hpx::util::counting_iterator<std::size_t>;

            constexpr bool has_scheduler_executor =
                hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

            if constexpr (!has_scheduler_executor)
            {
                if (n == 0)
                {
                    return util::detail::algorithm_result<ExPolicy,
                        util::in_out_result<InIter, FwdIter>>::
                        get(util::in_out_result<InIter, FwdIter>{first, dest});
                }
            }

            // Relocates assigned chains sequentially; handles cleanup on throw.
            auto processor =
                [first_ptr, dest_ptr, n, d](counting_iter it,
                    std::size_t num_chains) mutable -> chain_group_result {
                std::size_t chain_start = *it;
                std::size_t chains_done = 0;

                for (std::size_t ci = 0; ci < num_chains; ++ci)
                {
                    std::size_t c = chain_start + ci;
                    std::size_t partial = 0;

                    try
                    {
                        for (std::size_t j = c; j < n; j += d, ++partial)
                        {
                            hpx::experimental::detail::relocate_at_helper(
                                first_ptr + j, dest_ptr + j);
                        }
                    }
                    catch (...)
                    {
                        for (std::size_t k = 0; k < partial; ++k)
                        {
                            std::destroy_at(dest_ptr + c + k * d);
                        }
                        for (std::size_t j = c + (partial + 1) * d; j < n;
                            j += d)
                        {
                            std::destroy_at(first_ptr + j);
                        }
                        for (std::size_t done_ci = 0; done_ci < chains_done;
                            ++done_ci)
                        {
                            std::size_t done_c = chain_start + done_ci;
                            for (std::size_t j = done_c; j < n; j += d)
                            {
                                std::destroy_at(dest_ptr + j);
                            }
                        }
                        for (std::size_t todo_ci = ci + 1; todo_ci < num_chains;
                            ++todo_ci)
                        {
                            std::size_t todo_c = chain_start + todo_ci;
                            for (std::size_t j = todo_c; j < n; j += d)
                            {
                                std::destroy_at(first_ptr + j);
                            }
                        }
                        throw;
                    }

                    ++chains_done;
                }

                return {chain_start, chains_done};
            };

            // Builds the return iterators once all partitions succeed.
            auto finalizer = [first, dest, n](auto&&... data) mutable
                -> util::in_out_result<InIter, FwdIter> {
                static_assert(sizeof...(data) < 2);
                util::detail::clear_container(data...);
                return util::in_out_result<InIter, FwdIter>{
                    std::next(first, n), std::next(dest, n)};
            };

            // Destroys relocated elements of a completed partition on failure.
            auto cleanup = [dest_ptr, n, d](chain_group_result&& r) -> void {
                for (std::size_t ci = 0; ci < r.chains_done; ++ci)
                {
                    std::size_t c = r.chain_start + ci;
                    for (std::size_t j = c; j < n; j += d)
                    {
                        std::destroy_at(dest_ptr + j);
                    }
                }
            };

            return util::partitioner_with_cleanup<ExPolicy,
                util::in_out_result<InIter, FwdIter>,
                chain_group_result>::call(HPX_FORWARD(ExPolicy, policy),
                counting_iter(0), d, processor, finalizer, cleanup);
        }

        ///////////////////////////////////////////////////////////////////////
        // parallel_uninitialized_relocate_n_bwd_overlap
        //
        // Same chain decomposition as parallel_uninitialized_relocate_n_overlap,
        // but elements within each chain are relocated back-to-front (from the
        // last element of the chain down to chain index c) to avoid overwriting
        // a source before it has been relocated in a right-shift overlap.
        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename BiIter1,
            typename BiIter2>
            requires(hpx::traits::is_contiguous_iterator_v<BiIter1> &&
                hpx::traits::is_contiguous_iterator_v<BiIter2>)
        decltype(auto) parallel_uninitialized_relocate_n_bwd_overlap(
            ExPolicy&& policy, BiIter1 first, std::size_t n, BiIter2 dest_last,
            std::size_t d)
        {
            using value_type = std::iter_value_t<BiIter1>;
            value_type* const first_ptr = std::to_address(first);
            value_type* const dest_first_ptr =
                std::to_address(dest_last) - static_cast<std::ptrdiff_t>(n);

            struct chain_group_result
            {
                std::size_t chain_start;
                std::size_t chains_done;
            };

            using counting_iter = hpx::util::counting_iterator<std::size_t>;

            constexpr bool has_scheduler_executor =
                hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

            if constexpr (!has_scheduler_executor)
            {
                if (n == 0)
                {
                    auto dest_first = std::prev(dest_last, n);
                    return util::detail::algorithm_result<ExPolicy,
                        util::in_out_result<BiIter1, BiIter2>>::get(util::
                            in_out_result<BiIter1, BiIter2>{first, dest_first});
                }
            }

            // Relocates assigned chains back-to-front; handles cleanup on throw.
            auto processor =
                [first_ptr, dest_first_ptr, n, d](counting_iter it,
                    std::size_t num_chains) mutable -> chain_group_result {
                std::size_t chain_start = *it;
                std::size_t chains_done = 0;

                for (std::size_t ci = 0; ci < num_chains; ++ci)
                {
                    std::size_t c = chain_start + ci;

                    std::size_t k_max = (n - 1 - c) / d;
                    std::size_t partial = 0;

                    try
                    {
                        for (std::size_t k = k_max + 1; k-- > 0; ++partial)
                        {
                            std::size_t j = c + k * d;
                            hpx::experimental::detail::relocate_at_helper(
                                first_ptr + j, dest_first_ptr + j);
                        }
                    }
                    catch (...)
                    {
                        for (std::size_t k = 0; k < partial; ++k)
                        {
                            std::size_t j = c + (k_max - k) * d;
                            std::destroy_at(dest_first_ptr + j);
                        }
                        if (partial < k_max + 1)
                        {
                            for (std::size_t k = 0; k < k_max - partial; ++k)
                            {
                                std::size_t j = c + k * d;
                                std::destroy_at(first_ptr + j);
                            }
                        }
                        for (std::size_t done_ci = 0; done_ci < chains_done;
                            ++done_ci)
                        {
                            std::size_t done_c = chain_start + done_ci;
                            for (std::size_t j = done_c; j < n; j += d)
                            {
                                std::destroy_at(dest_first_ptr + j);
                            }
                        }
                        for (std::size_t todo_ci = ci + 1; todo_ci < num_chains;
                            ++todo_ci)
                        {
                            std::size_t todo_c = chain_start + todo_ci;
                            for (std::size_t j = todo_c; j < n; j += d)
                            {
                                std::destroy_at(first_ptr + j);
                            }
                        }
                        throw;
                    }

                    ++chains_done;
                }

                return {chain_start, chains_done};
            };

            // Builds the return iterators once all partitions succeed.
            auto finalizer = [first, dest_last, n](auto&&... data) mutable
                -> util::in_out_result<BiIter1, BiIter2> {
                static_assert(sizeof...(data) < 2);
                util::detail::clear_container(data...);
                return util::in_out_result<BiIter1, BiIter2>{
                    first, std::prev(dest_last, n)};
            };

            // Destroys relocated elements of a completed partition on failure.
            auto cleanup = [dest_first_ptr, n, d](
                               chain_group_result&& r) -> void {
                for (std::size_t ci = 0; ci < r.chains_done; ++ci)
                {
                    std::size_t c = r.chain_start + ci;
                    for (std::size_t j = c; j < n; j += d)
                    {
                        std::destroy_at(dest_first_ptr + j);
                    }
                }
            };

            return util::partitioner_with_cleanup<ExPolicy,
                util::in_out_result<BiIter1, BiIter2>,
                chain_group_result>::call(HPX_FORWARD(ExPolicy, policy),
                counting_iter(0), d, processor, finalizer, cleanup);
        }

        ///////////////////////////////////////////////////////////////////////
        // parallel_uninitialized_relocate_n_backward
        HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename BiIter,
            typename FwdIter, typename Size>
            requires(hpx::traits::is_input_iterator_v<BiIter> &&
                hpx::traits::is_forward_iterator_v<FwdIter> &&
                std::is_integral_v<Size>)
        decltype(auto) parallel_uninitialized_relocate_n_backward(
            ExPolicy&& policy, BiIter first, Size count, FwdIter dest_last)
        {
            constexpr bool has_scheduler_executor =
                hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

            auto dest_first = std::prev(dest_last, count);

            if constexpr (!has_scheduler_executor)
            {
                if (count == 0)
                {
                    return util::detail::algorithm_result<ExPolicy,
                        util::in_out_result<BiIter, FwdIter>>::get(util::
                            in_out_result<BiIter, FwdIter>{first, dest_first});
                }
            }

            using zip_iter = ::hpx::util::zip_iterator<BiIter, FwdIter>;
            using partition_result_type = std::pair<FwdIter, FwdIter>;

            return util::partitioner_with_cleanup<ExPolicy,
                util::in_out_result<BiIter, FwdIter>, partition_result_type>::
                call(
                    HPX_FORWARD(ExPolicy, policy), zip_iter(first, dest_first),
                    count,
                    [](zip_iter t, std::size_t part_size) mutable
                        -> partition_result_type {
                        using hpx::get;

                        auto iters = t.get_iterator_tuple();

                        BiIter part_source = get<0>(iters);
                        FwdIter part_dest = get<1>(iters);

                        auto [part_source_advanced, part_dest_advanced] =
                            hpx::experimental::util::
                                uninitialized_relocate_n_primitive(
                                    part_source, part_size, part_dest);

                        return std::make_pair(part_dest, part_dest_advanced);
                    },
                    [first, dest_first](auto&&... data) mutable
                        -> util::in_out_result<BiIter, FwdIter> {
                        static_assert(sizeof...(data) < 2);

                        util::detail::clear_container(data...);

                        return util::in_out_result<BiIter, FwdIter>{
                            first, dest_first};
                    },
                    [](partition_result_type&& r) -> void {
                        std::destroy(r.first, r.second);
                    });
        }

        /////////////////////////////////////////////////////////////////////////////
        // uninitialized_relocate_n

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT template <typename IterPair>
        struct uninitialized_relocate_n
          : public algorithm<uninitialized_relocate_n<IterPair>, IterPair>
        {
            constexpr uninitialized_relocate_n() noexcept
              : algorithm<uninitialized_relocate_n, IterPair>(
                    "uninitialized_relocate_n")
            {
            }

            // non vectorized overload

            template <typename ExPolicy, typename InIter, typename FwdIter>
            // clang-format off
                requires (
                    hpx::is_sequenced_execution_policy_v<ExPolicy> &&
                    std::input_iterator<InIter> &&
                    std::forward_iterator<FwdIter>
                )
            // clang-format on
            static util::in_out_result<InIter, FwdIter> sequential(ExPolicy&&,
                InIter first, std::size_t count, FwdIter dest) noexcept(hpx::
                    experimental::util::detail::relocation_traits<InIter,
                        FwdIter>::is_noexcept_relocatable_v)
            {
                auto [first_advanced, dest_advanced] =
                    hpx::experimental::util::uninitialized_relocate_n_primitive(
                        first, count, dest);

                return util::in_out_result<InIter, FwdIter>{
                    first_advanced, dest_advanced};
            }

            template <typename ExPolicy, typename InIter, typename FwdIter>
            // clang-format off
                requires (
                    hpx::is_execution_policy_v<ExPolicy> &&
                    std::input_iterator<InIter> &&
                    std::forward_iterator<FwdIter>
                )
            // clang-format on
            static decltype(auto) parallel(ExPolicy&& policy, InIter first,
                std::size_t count, FwdIter dest) noexcept(hpx::experimental::
                    util::detail::relocation_traits<InIter,
                        FwdIter>::is_noexcept_relocatable_v)
            {
                return parallel_uninitialized_relocate_n(
                    HPX_FORWARD(ExPolicy, policy), first, count, dest);
            }
        };
        /// \endcond

        /////////////////////////////////////////////////////////////////////////////
        // uninitialized_relocate
        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT template <typename IterPair>
        struct uninitialized_relocate
          : public algorithm<uninitialized_relocate<IterPair>, IterPair>
        {
            constexpr uninitialized_relocate() noexcept
              : algorithm<uninitialized_relocate, IterPair>(
                    "uninitialized_relocate")
            {
            }

            // non vectorized overload
            template <typename ExPolicy, typename InIter1, typename InIter2,
                typename FwdIter>
            // clang-format off
                requires (
                    hpx::is_sequenced_execution_policy_v<ExPolicy>&&
                    std::input_iterator<InIter1>&&
                    std::input_iterator<InIter2>&&
                    std::forward_iterator<FwdIter>
                )
                //  clang-format on
            static util::in_out_result<InIter1, FwdIter> sequential(
                ExPolicy&&, InIter1 first, InIter2 last,
                FwdIter dest) noexcept(hpx::experimental::util::detail::relocation_traits<
                    InIter1, FwdIter>::is_noexcept_relocatable_v)
            {
                auto [first_advanced, dest_advanced] =
                    hpx::experimental::util::uninitialized_relocate_primitive(
                        first, last, dest);

                return util::in_out_result<InIter1, FwdIter>{first_advanced,
                                dest_advanced};
            }

            template <typename ExPolicy, typename InIter1, typename InIter2,
                typename FwdIter>
            // clang-format off
                requires (
                    hpx::is_execution_policy_v<ExPolicy>&&
                    std::input_iterator<InIter1>&&
                    std::input_iterator<InIter2>&&
                    std::forward_iterator<FwdIter>
                )
            // clang-format on
            static decltype(auto) parallel(ExPolicy&& policy, InIter1 first,
                InIter2 last, FwdIter dest) noexcept(hpx::experimental::util::
                    detail::relocation_traits<InIter1,
                        FwdIter>::is_noexcept_relocatable_v)
            {
                auto count = hpx::parallel::detail::distance(first, last);

                return parallel_uninitialized_relocate_n(
                    HPX_FORWARD(ExPolicy, policy), first, count, dest);
            }
        };
        /// \endcond

        /////////////////////////////////////////////////////////////////////////////
        // uninitialized_relocate_backward
        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT template <typename IterPair>
        struct uninitialized_relocate_backward
          : public algorithm<uninitialized_relocate_backward<IterPair>,
                IterPair>
        {
            constexpr uninitialized_relocate_backward() noexcept
              : algorithm<uninitialized_relocate_backward, IterPair>(
                    "uninitialized_relocate_backward")
            {
            }

            // non vectorized overload
            template <typename ExPolicy, typename BiIter1, typename BiIter2>
            // clang-format off
                requires (
                    hpx::is_sequenced_execution_policy_v<ExPolicy> &&
                    std::bidirectional_iterator<BiIter1> &&
                    std::bidirectional_iterator<BiIter2>
                )
            //  clang-format on
            static util::in_out_result<BiIter1, BiIter2> sequential(
                ExPolicy&&, BiIter1 first, BiIter1 last,
                BiIter2 dest_last) noexcept(hpx::experimental::util::detail::
                relocation_traits<BiIter1, BiIter2>::is_noexcept_relocatable_v)
            {
                auto [last_advanced, dest_last_advanced] =
                hpx::experimental::util::uninitialized_relocate_backward_primitive(
                        first, last, dest_last);

                return util::in_out_result<BiIter1, BiIter2>{last_advanced,
                            dest_last_advanced};
            }

            template <typename ExPolicy, typename BiIter1, typename BiIter2>
            // clang-format off
                requires (
                    hpx::is_execution_policy_v<ExPolicy>&&
                    std::bidirectional_iterator<BiIter1>&&
                    std::bidirectional_iterator<BiIter2>
                )
            // clang-format on
            static decltype(auto) parallel(ExPolicy&& policy, BiIter1 first,
                BiIter1 last, BiIter2 dest_last) noexcept(hpx::experimental::
                    util::detail::relocation_traits<BiIter1,
                        BiIter2>::is_noexcept_relocatable_v)
            {
                auto count = hpx::parallel::detail::distance(first, last);

                return parallel_uninitialized_relocate_n_backward(
                    HPX_FORWARD(ExPolicy, policy), first, count, dest_last);
            }
        };
        /// \endcond
    }    // namespace detail
}    // namespace hpx::parallel

namespace hpx::experimental {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::uninitialized_relocate_n
    HPX_CXX_CORE_EXPORT inline constexpr struct uninitialized_relocate_n_t final
      : hpx::detail::tag_parallel_algorithm<uninitialized_relocate_n_t>
    {
        template <typename InIter, typename Size, typename FwdIter>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<InIter> &&
                hpx::traits::is_iterator_v<FwdIter> &&
                std::is_integral_v<Size>
            )
        // clang-format on
        friend FwdIter tag_fallback_invoke(uninitialized_relocate_n_t,
            InIter first, Size count,
            FwdIter dest) noexcept(util::detail::relocation_traits<InIter,
            FwdIter>::is_noexcept_relocatable_v)
        {
            static_assert(std::input_iterator<InIter>,
                "The 'first' argument must meet the requirements "
                "of an input iterator.");
            static_assert(std::forward_iterator<FwdIter>,
                "The 'dest' argument must meet the requirements of a forward "
                "iterator.");
            static_assert(util::detail::relocation_traits<InIter,
                              FwdIter>::valid_relocation,
                "Relocating from this source type to this destination type is "
                "ill-formed");

            // if count is representing a negative value, we do nothing
            if (hpx::parallel::detail::is_negative(count))
            {
                return dest;
            }

            return parallel::util::get_second_element(
                hpx::parallel::detail::uninitialized_relocate_n<
                    parallel::util::in_out_result<InIter, FwdIter>>()
                    .call(hpx::execution::seq, first,
                        static_cast<std::size_t>(count), dest));
        }

        template <typename ExPolicy, typename InIter, typename Size,
            typename FwdIter>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<InIter> &&
                hpx::traits::is_iterator_v<FwdIter> &&
                std::is_integral_v<Size>
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(uninitialized_relocate_n_t,
            ExPolicy&& policy, InIter first, Size count,
            FwdIter dest) noexcept(util::detail::relocation_traits<InIter,
            FwdIter>::is_noexcept_relocatable_v)
        {
            static_assert(std::input_iterator<InIter>,
                "The 'first' argument must meet the requirements "
                "of an input iterator.");
            static_assert(std::forward_iterator<FwdIter>,
                "The 'dest' argument must meet the requirements of a forward "
                "iterator.");
            static_assert(util::detail::relocation_traits<InIter,
                              FwdIter>::valid_relocation,
                "Relocating from this source type to this destination type is "
                "ill-formed");

            constexpr bool has_scheduler_executor =
                hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

            // if count is representing a negative value, we do nothing
            if (hpx::parallel::detail::is_negative(count))
            {
                if constexpr (has_scheduler_executor)
                {
                    count = static_cast<std::size_t>(0);
                }
                else
                {
                    return parallel::util::detail::algorithm_result<ExPolicy,
                        FwdIter>::get(HPX_MOVE(dest));
                }
            }

            // right-shift overlap with trivially relocatable must use memmove
            if constexpr (hpx::traits::is_contiguous_iterator_v<InIter> &&
                hpx::traits::is_contiguous_iterator_v<FwdIter>)
            {
                constexpr bool is_trivially_relocatable =
                    hpx::experimental::util::detail::relocation_traits<InIter,
                        FwdIter>::is_memcpyable;
                if constexpr (is_trivially_relocatable)
                {
                    auto last = std::next(first, count);
                    if (first < dest && dest < last)
                    {
                        using value_type = std::iter_value_t<InIter>;
                        // NOLINTNEXTLINE(bugprone-undefined-memory-manipulation)
                        std::memmove(static_cast<void*>(std::to_address(dest)),
                            std::to_address(first), count * sizeof(value_type));

                        auto result =
                            parallel::util::detail::algorithm_result<ExPolicy,
                                FwdIter>::get(std::next(dest, count));
                        if constexpr (has_scheduler_executor)
                        {
                            namespace ex = hpx::execution::experimental;
                            return ex::unique_any_sender<FwdIter>(
                                HPX_MOVE(result));
                        }
                        else
                        {
                            return result;
                        }
                    }
                }
            }

            if constexpr (!hpx::is_sequenced_execution_policy_v<ExPolicy>)
            {
                bool has_overlap = false;
                if constexpr (hpx::traits::is_contiguous_iterator_v<InIter> &&
                    hpx::traits::is_contiguous_iterator_v<FwdIter>)
                {
                    auto last = std::next(first, count);
                    auto dest_last = std::next(dest, count);
                    has_overlap = (first < dest_last) && (dest_last < last);

                    if (has_overlap)
                    {
                        constexpr bool is_trivially_relocatable =
                            hpx::experimental::util::detail::relocation_traits<
                                InIter, FwdIter>::is_memcpyable;
                        if constexpr (!is_trivially_relocatable)
                        {
                            using value_type = std::iter_value_t<InIter>;
                            // only safe to parallelize if the move constructor
                            // is noexcept; otherwise fall back to sequential
                            if constexpr (std::is_nothrow_move_constructible_v<
                                              value_type>)
                            {
                                auto d = static_cast<std::size_t>(
                                    std::to_address(first) -
                                    std::to_address(dest));
                                auto result = parallel::util::get_second_element(
                                    hpx::parallel::detail::
                                        parallel_uninitialized_relocate_n_overlap(
                                            HPX_FORWARD(ExPolicy, policy),
                                            first, count, dest, d));
                                if constexpr (has_scheduler_executor)
                                {
                                    namespace ex = hpx::execution::experimental;
                                    return ex::unique_any_sender<FwdIter>(
                                        HPX_MOVE(result));
                                }
                                else
                                {
                                    return result;
                                }
                            }
                        }
                    }
                }

                if (!has_overlap)
                {
                    auto result = parallel::util::get_second_element(
                        hpx::parallel::detail::uninitialized_relocate_n<
                            parallel::util::in_out_result<InIter, FwdIter>>()
                            .call(HPX_FORWARD(ExPolicy, policy), first, count,
                                dest));
                    if constexpr (has_scheduler_executor)
                    {
                        namespace ex = hpx::execution::experimental;
                        return ex::unique_any_sender<FwdIter>(HPX_MOVE(result));
                    }
                    else
                    {
                        return result;
                    }
                }
            }

            auto result = parallel::util::get_second_element(
                hpx::parallel::detail::uninitialized_relocate_n<
                    parallel::util::in_out_result<InIter, FwdIter>>()
                    .call(hpx::execution::seq, first,
                        static_cast<std::size_t>(count), dest));
            if constexpr (has_scheduler_executor)
            {
                namespace ex = hpx::execution::experimental;
                return ex::unique_any_sender<FwdIter>(
                    ex::just(HPX_MOVE(result)));
            }
            else
            {
                return result;
            }
        }
    } uninitialized_relocate_n{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::uninitialized_relocate
    HPX_CXX_CORE_EXPORT inline constexpr struct uninitialized_relocate_t final
      : hpx::detail::tag_parallel_algorithm<uninitialized_relocate_t>
    {
        template <typename InIter1, typename InIter2, typename FwdIter>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<InIter1> &&
                hpx::traits::is_iterator_v<InIter2> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        friend FwdIter tag_fallback_invoke(uninitialized_relocate_t,
            InIter1 first, InIter2 last,
            FwdIter dest) noexcept(util::detail::relocation_traits<InIter1,
            FwdIter>::is_noexcept_relocatable_v)
        {
            static_assert(
                std::input_iterator<InIter1> && std::input_iterator<InIter2>,
                "The 'first' and 'last' arguments must meet the requirements "
                "of input iterators.");
            static_assert(std::forward_iterator<FwdIter>,
                "The 'dest' argument must meet the requirements of a forward "
                "iterator.");
            static_assert(util::detail::relocation_traits<InIter1,
                              FwdIter>::valid_relocation,
                "Relocating from this source type to this destination type is "
                "ill-formed");
            // if count is representing a negative value, we do nothing
            if (hpx::parallel::detail::is_negative(
                    hpx::parallel::detail::distance(first, last)))
            {
                return dest;
            }

            return parallel::util::get_second_element(
                hpx::parallel::detail::uninitialized_relocate<
                    parallel::util::in_out_result<InIter1, FwdIter>>()
                    .call(hpx::execution::seq, first, last, dest));
        }

        template <typename ExPolicy, typename InIter1, typename InIter2,
            typename FwdIter>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<InIter1> &&
                hpx::traits::is_iterator_v<InIter2> &&
                hpx::traits::is_iterator_v<FwdIter>
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(uninitialized_relocate_t,
            ExPolicy&& policy, InIter1 first, InIter2 last,
            FwdIter dest) noexcept(util::detail::relocation_traits<InIter1,
            FwdIter>::is_noexcept_relocatable_v)
        {
            static_assert(
                std::input_iterator<InIter1> && std::input_iterator<InIter2>,
                "The 'first' and 'last' arguments must meet the requirements "
                "of input iterators.");
            static_assert(std::forward_iterator<FwdIter>,
                "The 'dest' argument must meet the requirements of a forward "
                "iterator.");
            static_assert(util::detail::relocation_traits<InIter1,
                              FwdIter>::valid_relocation,
                "Relocating from this source type to this destination type is "
                "ill-formed");

            auto count = hpx::parallel::detail::distance(first, last);
            constexpr bool has_scheduler_executor =
                hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

            // if count is representing a negative value, we do nothing
            if (hpx::parallel::detail::is_negative(count))
            {
                if constexpr (has_scheduler_executor)
                {
                    count = static_cast<std::size_t>(0);
                }
                else
                {
                    return parallel::util::detail::algorithm_result<ExPolicy,
                        FwdIter>::get(HPX_MOVE(dest));
                }
            }

            // right-shift overlap with trivially relocatable must use memmove
            if constexpr (hpx::traits::is_contiguous_iterator_v<InIter1> &&
                hpx::traits::is_contiguous_iterator_v<InIter2> &&
                hpx::traits::is_contiguous_iterator_v<FwdIter>)
            {
                constexpr bool is_trivially_relocatable =
                    hpx::experimental::util::detail::relocation_traits<InIter1,
                        FwdIter>::is_memcpyable;
                if constexpr (is_trivially_relocatable)
                {
                    auto src_last = std::next(first, count);
                    if (first < dest && dest < src_last)
                    {
                        using value_type = std::iter_value_t<InIter1>;
                        // NOLINTNEXTLINE(bugprone-undefined-memory-manipulation)
                        std::memmove(static_cast<void*>(std::to_address(dest)),
                            std::to_address(first), count * sizeof(value_type));

                        if constexpr (has_scheduler_executor)
                        {
                            namespace ex = hpx::execution::experimental;
                            return ex::unique_any_sender<FwdIter>(
                                parallel::util::detail::algorithm_result<
                                    ExPolicy, FwdIter>::get(std::next(dest,
                                    count)));
                        }
                        else
                        {
                            return parallel::util::detail::algorithm_result<
                                ExPolicy, FwdIter>::get(std::next(dest, count));
                        }
                    }
                }
            }

            if constexpr (!hpx::is_sequenced_execution_policy_v<ExPolicy>)
            {
                bool has_overlap = false;
                if constexpr (hpx::traits::is_contiguous_iterator_v<InIter1> &&
                    hpx::traits::is_contiguous_iterator_v<InIter2> &&
                    hpx::traits::is_contiguous_iterator_v<FwdIter>)
                {
                    auto src_last = std::next(first, count);
                    auto dest_last = std::next(dest, count);
                    has_overlap = (first < dest_last) && (dest_last < src_last);

                    if (has_overlap)
                    {
                        constexpr bool is_trivially_relocatable =
                            hpx::experimental::util::detail::relocation_traits<
                                InIter1, FwdIter>::is_memcpyable;
                        if constexpr (!is_trivially_relocatable)
                        {
                            using value_type = std::iter_value_t<InIter1>;
                            // only safe to parallelize if the move constructor
                            // is noexcept; otherwise fall back to sequential
                            if constexpr (std::is_nothrow_move_constructible_v<
                                              value_type>)
                            {
                                auto d = static_cast<std::size_t>(
                                    std::to_address(first) -
                                    std::to_address(dest));
                                auto result = parallel::util::get_second_element(
                                    hpx::parallel::detail::
                                        parallel_uninitialized_relocate_n_overlap(
                                            HPX_FORWARD(ExPolicy, policy),
                                            first, count, dest, d));
                                if constexpr (has_scheduler_executor)
                                {
                                    namespace ex = hpx::execution::experimental;
                                    return ex::unique_any_sender<FwdIter>(
                                        HPX_MOVE(result));
                                }
                                else
                                {
                                    return result;
                                }
                            }
                        }
                    }
                }

                if (!has_overlap)
                {
                    auto result = parallel::util::get_second_element(
                        hpx::parallel::detail::uninitialized_relocate_n<
                            parallel::util::in_out_result<InIter1, FwdIter>>()
                            .call(HPX_FORWARD(ExPolicy, policy), first, count,
                                dest));
                    if constexpr (has_scheduler_executor)
                    {
                        namespace ex = hpx::execution::experimental;
                        return ex::unique_any_sender<FwdIter>(HPX_MOVE(result));
                    }
                    else
                    {
                        return result;
                    }
                }
            }

            auto result = parallel::util::get_second_element(
                hpx::parallel::detail::uninitialized_relocate_n<
                    parallel::util::in_out_result<InIter1, FwdIter>>()
                    .call(hpx::execution::seq, first, count, dest));
            if constexpr (has_scheduler_executor)
            {
                namespace ex = hpx::execution::experimental;
                return ex::unique_any_sender<FwdIter>(
                    ex::just(HPX_MOVE(result)));
            }
            else
            {
                return result;
            }
        }
    } uninitialized_relocate{};

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::uninitialized_relocate_backward
    HPX_CXX_CORE_EXPORT inline constexpr struct
        uninitialized_relocate_backward_t final
      : hpx::detail::tag_parallel_algorithm<uninitialized_relocate_backward_t>
    {
        template <typename BiIter1, typename BiIter2>
        // clang-format off
            requires (
                hpx::traits::is_iterator_v<BiIter1> &&
                hpx::traits::is_iterator_v<BiIter2>
            )
        // clang-format on
        friend BiIter2 tag_fallback_invoke(uninitialized_relocate_backward_t,
            BiIter1 first, BiIter1 last,
            BiIter2 dest_last) noexcept(util::detail::relocation_traits<BiIter1,
            BiIter2>::is_noexcept_relocatable_v)
        {
            static_assert(std::bidirectional_iterator<BiIter1>,
                "The 'first' and 'last' arguments must meet the requirements "
                "of bidirectional iterators.");
            static_assert(std::bidirectional_iterator<BiIter2>,
                "The 'dest_last' argument must meet the requirements of a "
                "bidirectional iterator.");
            static_assert(util::detail::relocation_traits<BiIter1,
                              BiIter2>::valid_relocation,
                "Relocating from this source type to this destination type is "
                "ill-formed");
            // if count is representing a negative value, we do nothing
            if (first == last)
            {
                return dest_last;
            }

            return parallel::util::get_second_element(
                hpx::parallel::detail::uninitialized_relocate_backward<
                    parallel::util::in_out_result<BiIter1, BiIter2>>()
                    .call(hpx::execution::seq, first, last, dest_last));
        }

        template <typename ExPolicy, typename BiIter1, typename BiIter2>
        // clang-format off
            requires (
                hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<BiIter1> &&
                hpx::traits::is_iterator_v<BiIter2>
            )
        // clang-format on
        friend decltype(auto) tag_fallback_invoke(
            uninitialized_relocate_backward_t, ExPolicy&& policy, BiIter1 first,
            BiIter1 last,
            BiIter2 dest_last) noexcept(util::detail::relocation_traits<BiIter1,
            BiIter2>::is_noexcept_relocatable_v)
        {
            static_assert(std::bidirectional_iterator<BiIter1>,
                "The 'first' and 'last' arguments must meet the requirements "
                "of bidirectional iterators.");
            static_assert(std::bidirectional_iterator<BiIter2>,
                "The 'dest' argument must meet the requirements of a "
                "bidirectional iterator.");
            static_assert(util::detail::relocation_traits<BiIter1,
                              BiIter2>::valid_relocation,
                "Relocating from this source type to this destination type is "
                "ill-formed");

            auto count = hpx::parallel::detail::distance(first, last);
            constexpr bool has_scheduler_executor =
                hpx::execution_policy_has_scheduler_executor_v<ExPolicy>;

            // if count is representing a negative value, we do nothing
            if (hpx::parallel::detail::is_negative(count))
            {
                if constexpr (has_scheduler_executor)
                {
                    count = static_cast<std::size_t>(0);
                }
                else
                {
                    return parallel::util::detail::algorithm_result<ExPolicy,
                        BiIter2>::get(HPX_MOVE(dest_last));
                }
            }

            if constexpr (!hpx::is_sequenced_execution_policy_v<ExPolicy>)
            {
                bool has_overlap = false;
                if constexpr (hpx::traits::is_contiguous_iterator_v<BiIter1> &&
                    hpx::traits::is_contiguous_iterator_v<BiIter2>)
                {
                    auto dest_first = std::prev(dest_last, count);
                    has_overlap = (first < dest_first) && (dest_first < last);

                    if (has_overlap)
                    {
                        constexpr bool is_trivially_relocatable =
                            hpx::experimental::util::detail::relocation_traits<
                                BiIter1, BiIter2>::is_memcpyable;
                        if constexpr (!is_trivially_relocatable)
                        {
                            using value_type = std::iter_value_t<BiIter1>;
                            // only safe to parallelize if the move constructor
                            // is noexcept; otherwise fall back to sequential
                            if constexpr (std::is_nothrow_move_constructible_v<
                                              value_type>)
                            {
                                auto d = static_cast<std::size_t>(
                                    std::to_address(dest_first) -
                                    std::to_address(first));

                                auto result = parallel::util::get_second_element(
                                    hpx::parallel::detail::
                                        parallel_uninitialized_relocate_n_bwd_overlap(
                                            HPX_FORWARD(ExPolicy, policy),
                                            first, count, dest_last, d));

                                if constexpr (has_scheduler_executor)
                                {
                                    namespace ex = hpx::execution::experimental;
                                    return ex::unique_any_sender<BiIter2>(
                                        HPX_MOVE(result));
                                }
                                else
                                {
                                    return result;
                                }
                            }
                        }
                    }
                }

                if (!has_overlap)
                {
                    auto result = parallel::util::get_second_element(
                        hpx::parallel::detail::uninitialized_relocate_backward<
                            parallel::util::in_out_result<BiIter1, BiIter2>>()
                            .call(HPX_FORWARD(ExPolicy, policy), first, last,
                                dest_last));
                    if constexpr (has_scheduler_executor)
                    {
                        namespace ex = hpx::execution::experimental;
                        return ex::unique_any_sender<BiIter2>(HPX_MOVE(result));
                    }
                    else
                    {
                        return result;
                    }
                }
            }

            auto result = parallel::util::get_second_element(
                hpx::parallel::detail::uninitialized_relocate_backward<
                    parallel::util::in_out_result<BiIter1, BiIter2>>()
                    .call(hpx::execution::seq, first, last, dest_last));
            if constexpr (has_scheduler_executor)
            {
                namespace ex = hpx::execution::experimental;
                return ex::unique_any_sender<BiIter2>(
                    ex::just(HPX_MOVE(result)));
            }
            else
            {
                return result;
            }
        }
    } uninitialized_relocate_backward{};
}    // namespace hpx::experimental

#endif    // DOXYGEN
