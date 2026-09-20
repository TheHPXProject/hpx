//  Copyright (c) 2026 Bharath Kollanur
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <hpx/assert.hpp>
#include <hpx/functional/invoke.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/segmented_algorithms/detail/capture_dispatch.hpp>
#include <hpx/parallel/util/detail/handle_local_exceptions.hpp>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iterator>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    // segmented merge

    /// \cond NOINTERNAL

    // Stable Merge Path diagonal partitioning.
    // Find the stable merge-path co-rank (a, b) for output position k,
    // where a + b == k. The selected boundary satisfies:
    //
    //     A[a - 1] <= B[b]
    //     B[b - 1] <  A[a]
    //
    // with boundary checks for a == 0, b == 0, a == len1, and
    // b == len2. The asymmetric comparisons preserve merge stability:
    // elements from the first input precede equivalent elements from
    // the second input.
    template <typename Iter1, typename Iter2, typename Comp, typename Proj1,
        typename Proj2>
    std::pair<std::size_t, std::size_t> segmented_diagonal_intersection(
        Iter1 first1, std::size_t len1, Iter2 first2, std::size_t len2,
        std::size_t k, Comp& comp, Proj1& proj1, Proj2& proj2)
    {
        HPX_TRACING_MARK_EVENT("get diagonal intersection");
        if (len1 == 0)
            return {0, (std::min) (k, len2)};
        if (len2 == 0)
            return {(std::min) (k, len1), 0};
        auto a_low = (k > len2) ? (k - len2) : 0;
        auto a_high = (k < len1) ? k : len1;
        if (a_low == a_high)
        {
            auto a = a_low;
            return {a, k - a};    // Only one valid position
        }

        while (a_low <= a_high)
        {
            auto a = (a_low + a_high) / 2;
            auto b = k - a;
            // cond1: a==0 || b==len2 || A[a-1] <= B[b]
            bool cond1 = (a == 0) || (b == len2) ||
                !HPX_INVOKE(comp, HPX_INVOKE(proj2, *std::next(first2, b)),
                    HPX_INVOKE(proj1, *std::next(first1, a - 1)));

            // cond2: b==0 || a==len1 || B[b-1] < A[a]
            bool cond2 = (b == 0) || (a == len1) ||
                HPX_INVOKE(comp, HPX_INVOKE(proj2, *std::next(first2, b - 1)),
                    HPX_INVOKE(proj1, *std::next(first1, a)));

            if (cond1 && cond2)
                return {a, b};

            if (!cond1)
            {
                a_high = a - 1;
            }
            else
            {
                a_low = a + 1;
            }
        }
        return {a_high, k - a_high};
    }

    template <typename Chunk>
    struct destination_chunk_batch
    {
        hpx::id_type locality_id;
        hpx::id_type routing_partition_id;
        std::vector<Chunk> chunks;
    };

    template <typename Traits3, typename Iter3, typename F>
    HPX_FORCEINLINE auto for_each_output_chunk(
        Traits3, Iter3 const& dest, std::size_t total_size, F&& handle_chunk)
        -> std::pair<typename Traits3::segment_iterator,
            typename Traits3::local_iterator>
    {
        using segment_iterator_out = typename Traits3::segment_iterator;
        using out_local_iterator_type = typename Traits3::local_iterator;

        segment_iterator_out seg_out = Traits3::segment(dest);
        out_local_iterator_type loc_output = Traits3::local(dest);

        std::size_t global_offset = 0;

        while (global_offset != total_size)
        {
            out_local_iterator_type partition_end = Traits3::end(seg_out);

            if (loc_output == partition_end)
            {
                ++seg_out;
                loc_output = Traits3::begin(seg_out);
                continue;
            }

            std::size_t const available = static_cast<std::size_t>(
                std::distance(loc_output, partition_end));
            std::size_t const remaining = total_size - global_offset;
            std::size_t const chunk_size = (std::min) (available, remaining);

            HPX_ASSERT(chunk_size != 0);

            std::size_t const k0 = global_offset;
            std::size_t const k1 = k0 + chunk_size;

            HPX_INVOKE(handle_chunk, seg_out, loc_output, k0, k1);

            global_offset = k1;

            if (global_offset != total_size)
            {
                ++seg_out;
                loc_output = Traits3::begin(seg_out);
            }
        }
        return {HPX_MOVE(seg_out), HPX_MOVE(loc_output)};
    }

    template <typename OutIterator>
    struct captured_merge
      : algorithm<captured_merge<OutIterator>,
            util::in_in_out_result<hpx::util::unused_type,
                hpx::util::unused_type, OutIterator>>
    {
        using result_type = util::in_in_out_result<hpx::util::unused_type,
            hpx::util::unused_type, OutIterator>;

        captured_merge()
          : algorithm<captured_merge<OutIterator>, result_type>(
                "captured_merge")
        {
        }

        template <typename ExPolicy, typename Iter1, typename Sent1,
            typename Iter2, typename Sent2, typename Iter3, typename Comp,
            typename Proj1, typename Proj2>
        static auto sequential(ExPolicy, Iter1 first1, Sent1 last1,
            Iter2 first2, Sent2 last2, Iter3 dest, Comp&& comp, Proj1&& proj1,
            Proj2&& proj2) -> util::in_in_out_result<hpx::util::unused_type,
            hpx::util::unused_type, Iter3>
        {
            auto result = sequential_merge(first1, last1, first2, last2, dest,
                HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj1, proj1),
                HPX_FORWARD(Proj2, proj2));

            return {hpx::util::unused_type{}, hpx::util::unused_type{},
                HPX_MOVE(result.out)};
        }

        template <typename ExPolicy, typename Iter1, typename Sent1,
            typename Iter2, typename Sent2, typename Iter3, typename Comp,
            typename Proj1, typename Proj2>
        static parallel::util::detail::algorithm_result_t<ExPolicy,
            util::in_in_out_result<hpx::util::unused_type,
                hpx::util::unused_type, Iter3>>
        parallel(ExPolicy&& policy, Iter1 first1, Sent1 last1, Iter2 first2,
            Sent2 last2, Iter3 dest, Comp&& comp, Proj1&& proj1, Proj2&& proj2)
        {
            using actual_result_type =
                util::in_in_out_result<Iter1, Iter2, Iter3>;

            using captured_result_type =
                util::in_in_out_result<hpx::util::unused_type,
                    hpx::util::unused_type, Iter3>;

            auto operation = detail::merge<actual_result_type>{}.call2(
                HPX_FORWARD(ExPolicy, policy), std::false_type{}, first1, last1,
                first2, last2, dest, HPX_FORWARD(Comp, comp),
                HPX_FORWARD(Proj1, proj1), HPX_FORWARD(Proj2, proj2));

            if constexpr (hpx::is_async_execution_policy_v<
                              std::decay_t<ExPolicy>>)
            {
                return HPX_MOVE(operation).then(
                    [](auto ready) mutable -> captured_result_type {
                        auto result = ready.get();

                        return {hpx::util::unused_type{},
                            hpx::util::unused_type{}, HPX_MOVE(result.out)};
                    });
            }
            else
            {
                return {hpx::util::unused_type{}, hpx::util::unused_type{},
                    HPX_MOVE(operation.out)};
            }
        }
    };

    template <typename ExPolicy, typename T>
    HPX_FORCEINLINE hpx::future<T> make_policy_exceptional_future(
        std::exception_ptr exception)
    {
        using policy_type = std::decay_t<ExPolicy>;

        try
        {
            hpx::parallel::util::detail::handle_local_exceptions<
                policy_type>::call(exception);
        }
        catch (...)
        {
            // The future contains either:
            // - hpx::exception_list for ordinary exceptions, or
            // - std::bad_alloc, which HPX treats specially.
            return hpx::make_exceptional_future<T>(std::current_exception());
        }

        // handle_local_exceptions::call is expected not to return.
        std::terminate();
    }

    template <typename Traits, typename Chunk>
    struct destination_chunk_batches
    {
        using segment_iterator = typename Traits::segment_iterator;
        using batch_type = destination_chunk_batch<Chunk>;

        std::vector<batch_type> batches;
        std::optional<segment_iterator> final_segment;
        std::size_t final_batch_index = 0;
        std::size_t final_chunk_position = 0;
    };

    template <typename Traits3, typename Chunk, typename Iter1, typename Iter2,
        typename Iter3, typename Comp, typename Proj1, typename Proj2>
    destination_chunk_batches<Traits3, Chunk> make_destination_chunk_batches(
        Traits3, Iter1 const& first1, std::size_t len1, Iter2 const& first2,
        std::size_t len2, Iter3 const& dest, Comp& comp, Proj1& proj1,
        Proj2& proj2)
    {
        using chunk_batches_type = destination_chunk_batches<Traits3, Chunk>;
        using batch_type = typename chunk_batches_type::batch_type;
        using segment_iterator = typename Traits3::segment_iterator;
        using local_iterator = typename Traits3::local_iterator;

        chunk_batches_type chunk_batches;

        auto output_position = for_each_output_chunk(Traits3{}, dest,
            len1 + len2,
            [&](segment_iterator segment, local_iterator output_first,
                std::size_t k0, std::size_t k1) {
                auto const [a0, b0] = segmented_diagonal_intersection(
                    first1, len1, first2, len2, k0, comp, proj1, proj2);
                auto const [a1, b1] = segmented_diagonal_intersection(
                    first1, len1, first2, len2, k1, comp, proj1, proj2);

                Chunk chunk{a1 - a0, b1 - b0,
                    make_partition_ranges(
                        std::next(first1, a0), std::next(first1, a1)),
                    make_partition_ranges(
                        std::next(first2, b0), std::next(first2, b1)),
                    HPX_MOVE(output_first)};

                hpx::id_type const partition_id = Traits3::get_id(segment);
                hpx::id_type const locality_id =
                    get_partition_locality(partition_id);

                auto batch = std::find_if(chunk_batches.batches.begin(),
                    chunk_batches.batches.end(),
                    [&locality_id](batch_type const& candidate) {
                        return candidate.locality_id == locality_id;
                    });

                if (batch == chunk_batches.batches.end())
                {
                    chunk_batches.batches.push_back(
                        batch_type{locality_id, partition_id, {}});
                    batch = std::prev(chunk_batches.batches.end());
                }

                batch->chunks.push_back(HPX_MOVE(chunk));
                chunk_batches.final_batch_index = static_cast<std::size_t>(
                    std::distance(chunk_batches.batches.begin(), batch));
                chunk_batches.final_chunk_position = batch->chunks.size() - 1;
            });

        HPX_ASSERT(!chunk_batches.batches.empty());
        chunk_batches.final_segment.emplace(HPX_MOVE(output_position.first));
        return chunk_batches;
    }

    template <typename Result, typename Iter1, typename Iter2, typename Iter3>
    hpx::future<Result> make_merge_result_future(
        hpx::future<Iter3>&& end_dest, Iter1 last1, Iter2 last2)
    {
        return HPX_MOVE(end_dest).then(
            [last1 = HPX_MOVE(last1), last2 = HPX_MOVE(last2)](
                hpx::future<Iter3> ready) mutable -> Result {
                return Result{HPX_MOVE(last1), HPX_MOVE(last2), ready.get()};
            });
    }

    template <typename Iter1, typename Iter2, typename Iter3>
    struct segmented_merge_types
    {
        using destination_traits =
            hpx::traits::segmented_iterator_traits<Iter3>;
        using local_iterator = typename destination_traits::local_iterator;
        using value_type1 = typename std::iterator_traits<Iter1>::value_type;
        using value_type2 = typename std::iterator_traits<Iter2>::value_type;
        using range_list1_type = decltype(make_partition_ranges(
            std::declval<Iter1>(), std::declval<Iter1>()));
        using range_list2_type = decltype(make_partition_ranges(
            std::declval<Iter2>(), std::declval<Iter2>()));
        using chunk_type = capture_dispatch_chunk<range_list1_type,
            range_list2_type, local_iterator>;
        using chunk_batches_type =
            destination_chunk_batches<destination_traits, chunk_type>;
        using batch_type = typename chunk_batches_type::batch_type;
        using batch_result_type = std::vector<local_iterator>;
    };

    template <typename ExPolicy, typename Result>
    util::detail::algorithm_result_t<ExPolicy, Result>
    handle_merge_planning_exception(std::exception_ptr exception)
    {
        using policy_type = std::decay_t<ExPolicy>;

        if constexpr (hpx::is_async_execution_policy_v<policy_type>)
        {
            return make_policy_exceptional_future<policy_type, Result>(
                HPX_MOVE(exception));
        }
        else
        {
            hpx::parallel::util::detail::handle_local_exceptions<
                policy_type>::call(exception);
            std::terminate();
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Sequential remote implementation

    template <typename Algo, typename ExPolicy, typename Iter1, typename Iter2,
        typename Iter3, typename Comp, typename Proj1, typename Proj2>
    HPX_FORCEINLINE util::detail::algorithm_result_t<ExPolicy,
        util::in_in_out_result<Iter1, Iter2, Iter3>>
    segmented_merge_sequential(Algo&& algo, ExPolicy policy, Iter1 first1,
        Iter1 last1, Iter2 first2, Iter2 last2, Iter3 dest, Comp&& comp,
        Proj1&& proj1, Proj2&& proj2, std::size_t len1, std::size_t len2)
    {
        using types = segmented_merge_types<Iter1, Iter2, Iter3>;
        using traits3 = typename types::destination_traits;
        using local_iterator = typename types::local_iterator;
        using value_type1 = typename types::value_type1;
        using value_type2 = typename types::value_type2;
        using chunk_batches_type = typename types::chunk_batches_type;
        using batch_type = typename types::batch_type;
        using batch_result_type = typename types::batch_result_type;
        using policy_type = std::decay_t<ExPolicy>;
        using result_value_type = util::in_in_out_result<Iter1, Iter2, Iter3>;
        using result =
            util::detail::algorithm_result<ExPolicy, result_value_type>;

        static constexpr bool is_task_policy =
            hpx::is_async_execution_policy_v<policy_type>;

        std::optional<chunk_batches_type> chunk_batches;

        try
        {
            chunk_batches.emplace(make_destination_chunk_batches<traits3,
                typename types::chunk_type>(traits3{}, first1, len1, first2,
                len2, dest, comp, proj1, proj2));
        }
        catch (...)
        {
            return handle_merge_planning_exception<ExPolicy, result_value_type>(
                std::current_exception());
        }

        HPX_ASSERT(chunk_batches.has_value());
        HPX_ASSERT(chunk_batches->final_segment.has_value());

        if constexpr (is_task_policy)
        {
            using algo_type = std::decay_t<Algo>;
            using comp_type = std::decay_t<Comp>;
            using proj1_type = std::decay_t<Proj1>;
            using proj2_type = std::decay_t<Proj2>;

            struct task_state
            {
                algo_type algorithm;
                policy_type execution_policy;
                comp_type comparator;
                proj1_type projection1;
                proj2_type projection2;
            };

            auto state =
                std::make_shared<task_state>(task_state{HPX_FORWARD(Algo, algo),
                    HPX_MOVE(policy), HPX_FORWARD(Comp, comp),
                    HPX_FORWARD(Proj1, proj1), HPX_FORWARD(Proj2, proj2)});

            auto final_local =
                std::make_shared<std::optional<local_iterator>>();

            hpx::future<void> chain = hpx::make_ready_future();

            for (std::size_t index = 0; index != chunk_batches->batches.size();
                ++index)
            {
                bool const is_final =
                    (index == chunk_batches->final_batch_index);
                batch_type batch = HPX_MOVE(chunk_batches->batches[index]);
                std::size_t const final_position =
                    chunk_batches->final_chunk_position;

                chain = HPX_MOVE(chain).then(
                    [state, batch = HPX_MOVE(batch), final_local, is_final,
                        final_position](hpx::future<void> previous) mutable
                        -> hpx::future<void> {
                        previous.get();

                        auto operation =
                            capture_dispatch_batch_async<value_type1,
                                value_type2>(batch.routing_partition_id,
                                state->algorithm, state->execution_policy,
                                std::true_type{}, HPX_MOVE(batch.chunks),
                                state->comparator, state->projection1,
                                state->projection2);

                        return HPX_MOVE(operation).then(
                            [final_local, is_final, final_position](
                                hpx::future<batch_result_type> ready) {
                                auto values = ready.get();

                                if (is_final)
                                {
                                    HPX_ASSERT(final_position < values.size());
                                    final_local->emplace(
                                        HPX_MOVE(values[final_position]));
                                }
                            });
                    });
            }

            auto end_dest = HPX_MOVE(chain).then(
                [final_segment = HPX_MOVE(*chunk_batches->final_segment),
                    final_local](hpx::future<void> ready) mutable -> Iter3 {
                    ready.get();
                    HPX_ASSERT(final_local->has_value());
                    return traits3::compose(
                        final_segment, HPX_MOVE(**final_local));
                });

            return make_merge_result_future<result_value_type>(
                HPX_MOVE(end_dest), HPX_MOVE(last1), HPX_MOVE(last2));
        }
        else
        {
            std::optional<local_iterator> final_local;

            for (std::size_t index = 0; index != chunk_batches->batches.size();
                ++index)
            {
                auto& batch = chunk_batches->batches[index];

                auto values =
                    capture_dispatch_batch_async<value_type1, value_type2>(
                        batch.routing_partition_id, std::decay_t<Algo>(algo),
                        policy_type(policy), std::true_type{},
                        HPX_MOVE(batch.chunks), std::decay_t<Comp>(comp),
                        std::decay_t<Proj1>(proj1), std::decay_t<Proj2>(proj2))
                        .get();

                if (index == chunk_batches->final_batch_index)
                {
                    HPX_ASSERT(
                        chunk_batches->final_chunk_position < values.size());
                    final_local.emplace(
                        HPX_MOVE(values[chunk_batches->final_chunk_position]));
                }
            }

            HPX_ASSERT(final_local.has_value());

            Iter3 end_dest = traits3::compose(
                *chunk_batches->final_segment, HPX_MOVE(*final_local));

            return result::get(result_value_type{
                HPX_MOVE(last1), HPX_MOVE(last2), HPX_MOVE(end_dest)});
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Parallel remote implementation

    template <typename Algo, typename ExPolicy, typename Iter1, typename Iter2,
        typename Iter3, typename Comp, typename Proj1, typename Proj2>
    HPX_FORCEINLINE util::detail::algorithm_result_t<ExPolicy,
        util::in_in_out_result<Iter1, Iter2, Iter3>>
    segmented_merge_parallel(Algo&& algo, ExPolicy policy, Iter1 first1,
        Iter1 last1, Iter2 first2, Iter2 last2, Iter3 dest, Comp&& comp,
        Proj1&& proj1, Proj2&& proj2, std::size_t len1, std::size_t len2)
    {
        using types = segmented_merge_types<Iter1, Iter2, Iter3>;
        using traits3 = typename types::destination_traits;
        using value_type1 = typename types::value_type1;
        using value_type2 = typename types::value_type2;
        using chunk_batches_type = typename types::chunk_batches_type;
        using batch_result_type = typename types::batch_result_type;
        using policy_type = std::decay_t<ExPolicy>;
        using result_value_type = util::in_in_out_result<Iter1, Iter2, Iter3>;

        static constexpr bool is_task_policy =
            hpx::is_async_execution_policy_v<policy_type>;

        std::optional<chunk_batches_type> chunk_batches;

        try
        {
            chunk_batches.emplace(make_destination_chunk_batches<traits3,
                typename types::chunk_type>(traits3{}, first1, len1, first2,
                len2, dest, comp, proj1, proj2));
        }
        catch (...)
        {
            return handle_merge_planning_exception<ExPolicy, result_value_type>(
                std::current_exception());
        }

        HPX_ASSERT(chunk_batches.has_value());
        HPX_ASSERT(chunk_batches->final_segment.has_value());

        std::vector<hpx::future<batch_result_type>> operations;
        operations.reserve(chunk_batches->batches.size());

        for (auto& batch : chunk_batches->batches)
        {
            operations.push_back(
                capture_dispatch_batch_async<value_type1, value_type2>(
                    batch.routing_partition_id, std::decay_t<Algo>(algo),
                    policy_type(policy), std::false_type{},
                    HPX_MOVE(batch.chunks), std::decay_t<Comp>(comp),
                    std::decay_t<Proj1>(proj1), std::decay_t<Proj2>(proj2)));
        }

        auto end_dest =
            hpx::when_all(HPX_MOVE(operations))
                .then([final_segment = HPX_MOVE(*chunk_batches->final_segment),
                          final_batch = chunk_batches->final_batch_index,
                          final_chunk = chunk_batches->final_chunk_position](
                          auto ready) mutable -> Iter3 {
                    auto completed =
                        get_capture_results<policy_type>(ready.get());

                    HPX_ASSERT(final_batch < completed.size());
                    auto final_values = HPX_MOVE(completed[final_batch]);
                    HPX_ASSERT(final_chunk < final_values.size());

                    return traits3::compose(
                        final_segment, HPX_MOVE(final_values[final_chunk]));
                });

        auto operation = make_merge_result_future<result_value_type>(
            HPX_MOVE(end_dest), HPX_MOVE(last1), HPX_MOVE(last2));

        if constexpr (is_task_policy)
        {
            return operation;
        }
        else
        {
            return operation.get();
        }
    }

    ///////////////////////////////////////////////////////////////////////////

    template <typename Algo, typename ExPolicy, typename Iter1, typename Iter2,
        typename Iter3, typename Comp, typename Proj1, typename Proj2>
    util::detail::algorithm_result_t<ExPolicy,
        util::in_in_out_result<Iter1, Iter2, Iter3>>
    segmented_merge(Algo&& algo, ExPolicy policy, Iter1 first1, Iter1 last1,
        Iter2 first2, Iter2 last2, Iter3 dest, Comp&& comp, Proj1&& proj1,
        Proj2&& proj2)
    {
        using policy_type = std::decay_t<ExPolicy>;
        using is_seq = hpx::is_sequenced_execution_policy<policy_type>;
        using result_value_type = util::in_in_out_result<Iter1, Iter2, Iter3>;
        using result =
            util::detail::algorithm_result<ExPolicy, result_value_type>;

        std::size_t const len1 =
            static_cast<std::size_t>(std::distance(first1, last1));
        std::size_t const len2 =
            static_cast<std::size_t>(std::distance(first2, last2));

        if (first1 == last1 && first2 == last2)
        {
            return result::get(result_value_type{
                HPX_MOVE(last1), HPX_MOVE(last2), HPX_MOVE(dest)});
        }

        if constexpr (is_seq::value)
        {
            return segmented_merge_sequential(HPX_FORWARD(Algo, algo),
                HPX_MOVE(policy), HPX_MOVE(first1), HPX_MOVE(last1),
                HPX_MOVE(first2), HPX_MOVE(last2), HPX_MOVE(dest),
                HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj1, proj1),
                HPX_FORWARD(Proj2, proj2), len1, len2);
        }
        else
        {
            return segmented_merge_parallel(HPX_FORWARD(Algo, algo),
                HPX_MOVE(policy), HPX_MOVE(first1), HPX_MOVE(last1),
                HPX_MOVE(first2), HPX_MOVE(last2), HPX_MOVE(dest),
                HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj1, proj1),
                HPX_FORWARD(Proj2, proj2), len1, len2);
        }
    }

    /// \endcond
}    // namespace hpx::parallel::detail

// The segmented iterators we support all live in namespace hpx::segmented
namespace hpx::segmented {

    namespace detail {

        template <typename Iter>
        concept segmented_merge_iterator = hpx::traits::is_iterator_v<Iter> &&
            hpx::traits::is_segmented_iterator_v<Iter> &&
            std::random_access_iterator<Iter>;
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename Iter1, typename Iter2,
        typename Iter3, typename Comp = hpx::parallel::detail::less>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            detail::segmented_merge_iterator<Iter1> &&
            detail::segmented_merge_iterator<Iter2> &&
            detail::segmented_merge_iterator<Iter3>)
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, Iter3> hpx_invoke(
        hpx::merge_t, ExPolicy&& policy, Iter1 first1, Iter1 last1,
        Iter2 first2, Iter2 last2, Iter3 dest, Comp comp = Comp())
    {
        using traits3 = hpx::traits::segmented_iterator_traits<Iter3>;
        using output_local_iterator = typename traits3::local_iterator;

        return hpx::parallel::util::get_third_element(
            hpx::parallel::detail::segmented_merge(
                hpx::parallel::detail::captured_merge<output_local_iterator>{},
                HPX_FORWARD(ExPolicy, policy), first1, last1, first2, last2,
                dest, HPX_MOVE(comp), hpx::identity_v, hpx::identity_v));
    }

    HPX_CXX_EXPORT template <typename Iter1, typename Iter2, typename Iter3,
        typename Comp = hpx::parallel::detail::less>
        requires(detail::segmented_merge_iterator<Iter1> &&
            detail::segmented_merge_iterator<Iter2> &&
            detail::segmented_merge_iterator<Iter3>)
    Iter3 hpx_invoke(hpx::merge_t, Iter1 first1, Iter1 last1, Iter2 first2,
        Iter2 last2, Iter3 dest, Comp comp = Comp())
    {
        using traits3 = hpx::traits::segmented_iterator_traits<Iter3>;
        using output_local_iterator = typename traits3::local_iterator;

        return hpx::parallel::util::get_third_element(
            hpx::parallel::detail::segmented_merge(
                hpx::parallel::detail::captured_merge<output_local_iterator>{},
                hpx::execution::seq, first1, last1, first2, last2, dest,
                HPX_MOVE(comp), hpx::identity_v, hpx::identity_v));
    }

    HPX_CXX_EXPORT template <typename ExPolicy, typename Iter1, typename Iter2,
        typename Iter3, typename Comp = hpx::ranges::less,
        typename Proj1 = hpx::identity, typename Proj2 = hpx::identity>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            detail::segmented_merge_iterator<Iter1> &&
            detail::segmented_merge_iterator<Iter2> &&
            detail::segmented_merge_iterator<Iter3>)
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
        hpx::ranges::merge_result<Iter1, Iter2, Iter3>>
    hpx_invoke(hpx::ranges::merge_t, ExPolicy&& policy, Iter1 first1,
        Iter1 last1, Iter2 first2, Iter2 last2, Iter3 dest, Comp comp = Comp(),
        Proj1 proj1 = Proj1(), Proj2 proj2 = Proj2())
    {
        using traits3 = hpx::traits::segmented_iterator_traits<Iter3>;
        using output_local_iterator = typename traits3::local_iterator;

        return hpx::parallel::detail::segmented_merge(
            hpx::parallel::detail::captured_merge<output_local_iterator>{},
            HPX_FORWARD(ExPolicy, policy), first1, last1, first2, last2, dest,
            HPX_MOVE(comp), HPX_MOVE(proj1), HPX_MOVE(proj2));
    }

    HPX_CXX_EXPORT template <typename Iter1, typename Iter2, typename Iter3,
        typename Comp = hpx::ranges::less, typename Proj1 = hpx::identity,
        typename Proj2 = hpx::identity>
        requires(detail::segmented_merge_iterator<Iter1> &&
            detail::segmented_merge_iterator<Iter2> &&
            detail::segmented_merge_iterator<Iter3>)
    hpx::ranges::merge_result<Iter1, Iter2, Iter3> hpx_invoke(
        hpx::ranges::merge_t, Iter1 first1, Iter1 last1, Iter2 first2,
        Iter2 last2, Iter3 dest, Comp comp = Comp(), Proj1 proj1 = Proj1(),
        Proj2 proj2 = Proj2())
    {
        using traits3 = hpx::traits::segmented_iterator_traits<Iter3>;
        using output_local_iterator = typename traits3::local_iterator;

        return hpx::parallel::detail::segmented_merge(
            hpx::parallel::detail::captured_merge<output_local_iterator>{},
            hpx::execution::seq, first1, last1, first2, last2, dest,
            HPX_MOVE(comp), HPX_MOVE(proj1), HPX_MOVE(proj2));
    }

}    // namespace hpx::segmented
