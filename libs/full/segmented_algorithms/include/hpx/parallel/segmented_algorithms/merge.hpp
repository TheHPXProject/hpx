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
#include <cstdint>
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

    template <typename LocalIterator>
    struct segmented_range_table
    {
        using local_iterator = LocalIterator;

        std::vector<partition_range<local_iterator>> ranges;
        std::vector<std::size_t> partition_ends;
        std::vector<hpx::id_type> locality_ids;

        std::size_t size() const noexcept
        {
            return partition_ends.empty() ? 0 : partition_ends.back();
        }
    };

    // Build searchable metadata for a global segmented input range.
    //
    // make_partition_ranges supplies the partition-relative ranges. For each
    // range, this function records its cumulative global end offset and hosting
    // locality. The resulting table maps a logical input index to a partition,
    // locality, and local iterator without transferring the element itself.

    template <typename Iterator>
    auto make_segmented_range_table(Iterator first, Iterator last)
    {
        using traits = hpx::traits::segmented_iterator_traits<Iterator>;
        using local_iterator = traits::local_iterator;

        segmented_range_table<local_iterator> table;

        table.ranges = make_partition_ranges(first, last);
        table.partition_ends.reserve(table.ranges.size());
        table.locality_ids.reserve(table.ranges.size());

        std::size_t offset = 0;

        for (auto const& range : table.ranges)
        {
            offset += static_cast<std::size_t>(
                std::distance(range.first, range.last));

            table.partition_ends.push_back(offset);

            table.locality_ids.push_back(
                get_partition_locality(range.partition_id));
        }
        return table;
    }

    template <typename LocalIterator>
    struct partition_position
    {
        hpx::id_type locality_id;
        hpx::id_type partition_id;
        LocalIterator position;
    };

    // Translate a zero-based logical input index into its partition-local
    // position.
    //
    // upper_bound locates the first cumulative partition end greater than
    // index. Subtracting the preceding cumulative end gives the offset inside
    // that partition. The result contains all information needed to route a
    // projected value request and dereference the position on its owning
    // locality.

    template <typename LocalIterator>
    partition_position<LocalIterator> find_partition_position(
        segmented_range_table<LocalIterator> const& table, std::size_t index)
    {
        HPX_ASSERT(index < table.size());

        auto position = std::upper_bound(
            table.partition_ends.begin(), table.partition_ends.end(), index);

        std::size_t const partition_index = static_cast<std::size_t>(
            std::distance(table.partition_ends.begin(), position));

        std::size_t const partition_begin = partition_index == 0 ?
            0 :
            table.partition_ends[partition_index - 1];

        std::size_t const local_offset = index - partition_begin;

        auto const& range = table.ranges[partition_index];

        return {table.locality_ids[partition_index], range.partition_id,
            std::next(range.first, local_offset)};
    }

    template <typename LocalIterator>
    struct locality_probe_batch
    {
        hpx::id_type locality_id;
        hpx::id_type routing_partition_id;

        std::vector<projected_value_request<LocalIterator>> requests;
    };

    // Add one projected-value request to the batch for its source locality.
    //
    // Requests are first coalesced by locality so all required positions on
    // that locality can be obtained with one action. Within the locality batch,
    // identical partition positions are deduplicated.
    //
    // A deduplicated request stores multiple targets, allowing one transported
    // key to satisfy several diagonal searches or both boundaries of adjacent
    // output chunks.

    template <typename LocalIterator>
    void append_probe(std::vector<locality_probe_batch<LocalIterator>>& batches,
        partition_position<LocalIterator> position, std::size_t search_index,
        std::uint8_t operand_index)
    {
        auto batch = std::find_if(
            batches.begin(), batches.end(), [&position](auto const& candidate) {
                return candidate.locality_id == position.locality_id;
            });

        if (batch == batches.end())
        {
            batches.push_back(
                {position.locality_id, position.partition_id, {}});
            batch = std::prev(batches.end());
        }

        auto request = std::find_if(batch->requests.begin(),
            batch->requests.end(), [&position](auto const& candidate) {
                return candidate.partition_id == position.partition_id &&
                    candidate.position == position.position;
            });

        projected_value_target const target{search_index, operand_index};

        if (request == batch->requests.end())
        {
            batch->requests.emplace_back(projected_value_request<LocalIterator>{
                position.partition_id, {target}, HPX_MOVE(position.position)});
        }
        else
        {
            request->targets.emplace_back(target);
        }
    }

    struct diagonal_search_state
    {
        std::size_t k;
        std::size_t a_low;
        std::size_t a_high;

        std::size_t a = 0;
        std::size_t b = 0;

        bool complete = false;
    };

    // Initialize the binary-search state for merge diagonal k.
    //
    // The desired intersection satisfies a + b == k, where a and b are the
    // numbers consumed from the first and second inputs. The initial bounds
    // restrict a so both a and b remain within their respective input ranges.
    //
    // If the bounds already identify one possible value, the intersection is
    // complete and no projected-value probes are required.

    HPX_FORCEINLINE diagonal_search_state make_diagonal_state(
        std::size_t len1, std::size_t len2, std::size_t k)
    {
        diagonal_search_state state{
            k, k > len2 ? k - len2 : 0, (std::min) (k, len1)};

        if (state.a_low == state.a_high)
        {
            state.a = state.a_low;
            state.b = k - state.a;
            state.complete = true;
        }

        return state;
    }

    enum : std::uint8_t
    {
        input_previous = 0,
        input_current = 1
    };

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

    template <typename Key1, typename Key2>
    struct diagonal_probe_values
    {
        std::shared_ptr<Key1> a_previous;
        std::shared_ptr<Key1> a_current;
        std::shared_ptr<Key2> b_previous;
        std::shared_ptr<Key2> b_current;

        void reset()
        {
            a_previous.reset();
            a_current.reset();
            b_previous.reset();
            b_current.reset();
        }
    };

    // Prepare the projected values needed for one diagonal-search iteration.
    //
    // The midpoint a is chosen inside the current search interval and b is
    // derived from a + b == k. Depending on range boundaries, the stable merge
    // conditions require up to four values:
    //
    //     A[a - 1], A[a], B[b - 1], and B[b].
    //
    // Instead of dereferencing global segmented iterators, the required
    // positions are translated through the range tables and appended to
    // locality batches. Boundary values that cannot be referenced are omitted.

    template <typename Table1, typename Table2, typename Key1, typename Key2>
    void prepare_diagonal_probes(diagonal_search_state& state,
        std::size_t search_index, std::size_t len1, std::size_t len2,
        Table1 const& table1, Table2 const& table2,
        std::vector<locality_probe_batch<typename Table1::local_iterator>>&
            batches1,
        std::vector<locality_probe_batch<typename Table2::local_iterator>>&
            batches2,
        diagonal_probe_values<Key1, Key2>& values)
    {
        values.reset();

        if (state.complete)
        {
            return;
        }

        HPX_ASSERT(state.a_low <= state.a_high);
        if (state.a_low == state.a_high)
        {
            state.a = state.a_low;
            state.b = state.k - state.a;
            state.complete = true;
            return;
        }

        state.a = (state.a_low + state.a_high) / 2;
        state.b = state.k - state.a;

        if (state.a != 0 && state.b != len2)
        {
            append_probe(batches1, find_partition_position(table1, state.a - 1),
                search_index, input_previous);
            append_probe(batches2, find_partition_position(table2, state.b),
                search_index, input_current);
        }

        if (state.b != 0 && state.a != len1)
        {
            append_probe(batches2, find_partition_position(table2, state.b - 1),
                search_index, input_previous);
            append_probe(batches1, find_partition_position(table1, state.a),
                search_index, input_current);
        }
    }

    // Distribute projected keys returned from the first input to their targets.
    //
    // A source position may have been requested by several diagonal searches.
    // One shared key object is therefore created and assigned to every
    // referenced target, avoiding additional key copies.
    //
    // Each result must contain exactly one key; the one-element vector supports
    // construction-aware deserialization of non-default-constructible key
    // types.

    template <typename Key1, typename Key2>
    void store_input1_probe_results(
        std::vector<projected_value_result<Key1>> results,
        std::vector<diagonal_probe_values<Key1, Key2>>& values)
    {
        for (auto& result : results)
        {
            HPX_ASSERT(result.values.size() == 1);

            auto shared_value =
                std::make_shared<Key1>(HPX_MOVE(result.values.front()));

            for (auto const& target : result.targets)
            {
                HPX_ASSERT(target.search_index < values.size());
                auto& search_values = values[target.search_index];

                if (target.operand_index == input_previous)
                {
                    search_values.a_previous = shared_value;
                }
                else
                {
                    HPX_ASSERT(target.operand_index == input_current);
                    search_values.a_current = shared_value;
                }
            }
        }
    }

    // Distribute projected keys returned from the second input to their
    // targets.
    //
    // This is the second-input counterpart of store_input1_probe_results. The
    // target metadata determines whether the key represents B[b - 1] or B[b]
    // for the corresponding diagonal-search state.

    template <typename Key1, typename Key2>
    void store_input2_probe_results(
        std::vector<projected_value_result<Key2>> results,
        std::vector<diagonal_probe_values<Key1, Key2>>& values)
    {
        for (auto& result : results)
        {
            HPX_ASSERT(result.values.size() == 1);

            auto shared_value =
                std::make_shared<Key2>(HPX_MOVE(result.values.front()));

            for (auto const& target : result.targets)
            {
                HPX_ASSERT(target.search_index < values.size());
                auto& search_values = values[target.search_index];

                if (target.operand_index == input_previous)
                {
                    search_values.b_previous = shared_value;
                }
                else
                {
                    HPX_ASSERT(target.operand_index == input_current);
                    search_values.b_current = shared_value;
                }
            }
        }
    }

    // Apply the stable merge-path boundary conditions to one search state.
    //
    // cond1 checks that A[a - 1] must not follow B[b]. cond2 checks that B[b -
    // 1] must strictly precede A[a]. Their asymmetry ensures that equivalent
    // values from the first input remain before equivalent values from the
    // second input.
    //
    // If both conditions hold, (a, b) is the required intersection. Otherwise
    // the binary-search bounds are reduced in the direction indicated by the
    // failed condition.

    template <typename Key1, typename Key2, typename Comp>
    void update_diagonal_state(diagonal_search_state& state, std::size_t len1,
        std::size_t len2, diagonal_probe_values<Key1, Key2> const& values,
        Comp& comp)
    {
        if (state.complete)
        {
            return;
        }

        bool cond1 = state.a == 0 || state.b == len2;
        if (!cond1)
        {
            HPX_ASSERT(values.a_previous);
            HPX_ASSERT(values.b_current);
            cond1 = !HPX_INVOKE(comp, *values.b_current, *values.a_previous);
        }

        bool cond2 = state.b == 0 || state.a == len1;
        if (!cond2)
        {
            HPX_ASSERT(values.b_previous);
            HPX_ASSERT(values.a_current);
            cond2 = HPX_INVOKE(comp, *values.b_previous, *values.a_current);
        }

        if (cond1 && cond2)
        {
            state.complete = true;
        }
        else if (!cond1)
        {
            state.a_high = state.a - 1;
        }
        else
        {
            state.a_low = state.a + 1;
        }
    }

    // Resolve all requested diagonal intersections sequentially.
    //
    // Each diagonal search is completed before moving to the next. During one
    // binary-search iteration, required projected values are grouped by source
    // locality and fetched through projected-value actions.
    //
    // This path minimizes parallel scheduling overhead for sequenced policies
    // while still supporting remote partitions.

    template <typename ExPolicy, typename Key1, typename Key2, typename Table1,
        typename Table2, typename Comp, typename Proj1, typename Proj2>
    void resolve_diagonal_intersections(
        std::vector<diagonal_search_state>& states, std::size_t len1,
        std::size_t len2, Table1 const& table1, Table2 const& table2,
        Comp& comp, Proj1& proj1, Proj2& proj2, std::true_type)
    {
        using local_iterator1 = Table1::local_iterator;
        using local_iterator2 = Table2::local_iterator;
        using values_type = diagonal_probe_values<Key1, Key2>;

        std::vector<values_type> values(states.size());

        for (std::size_t search_index = 0; search_index != states.size();
            ++search_index)
        {
            auto& state = states[search_index];

            while (!state.complete)
            {
                std::vector<locality_probe_batch<local_iterator1>> batches1;
                std::vector<locality_probe_batch<local_iterator2>> batches2;

                prepare_diagonal_probes(state, search_index, len1, len2, table1,
                    table2, batches1, batches2, values[search_index]);

                if (state.complete)
                {
                    break;
                }

                for (auto& batch : batches1)
                {
                    auto results = capture_projected_values_async<ExPolicy,
                        Key1, local_iterator1>(batch.routing_partition_id,
                        HPX_MOVE(batch.requests), proj1)
                                       .get();

                    store_input1_probe_results<Key1, Key2>(
                        HPX_MOVE(results), values);
                }

                for (auto& batch : batches2)
                {
                    auto results = capture_projected_values_async<ExPolicy,
                        Key2, local_iterator2>(batch.routing_partition_id,
                        HPX_MOVE(batch.requests), proj2)
                                       .get();

                    store_input2_probe_results<Key1, Key2>(
                        HPX_MOVE(results), values);
                }

                update_diagonal_state(
                    state, len1, len2, values[search_index], comp);
            }
        }
    }

    // Resolve multiple diagonal intersections in parallel search rounds.
    //
    // Each round prepares probes for every incomplete diagonal. Requests are
    // coalesced by source locality across all searches, and the resulting
    // remote actions are launched before waiting.
    //
    // After all projected keys for the round arrive, every search state
    // advances one binary-search step. Rounds continue until all intersections
    // are complete.

    template <typename ExPolicy, typename Key1, typename Key2, typename Table1,
        typename Table2, typename Comp, typename Proj1, typename Proj2>
    void resolve_diagonal_intersections(
        std::vector<diagonal_search_state>& states, std::size_t len1,
        std::size_t len2, Table1 const& table1, Table2 const& table2,
        Comp& comp, Proj1& proj1, Proj2& proj2, std::false_type)
    {
        using local_iterator1 = Table1::local_iterator;
        using local_iterator2 = Table2::local_iterator;
        using values_type = diagonal_probe_values<Key1, Key2>;
        using result_type1 = std::vector<projected_value_result<Key1>>;
        using result_type2 = std::vector<projected_value_result<Key2>>;

        std::vector<values_type> values(states.size());

        for (;;)
        {
            bool all_complete = true;
            std::vector<locality_probe_batch<local_iterator1>> batches1;
            std::vector<locality_probe_batch<local_iterator2>> batches2;

            for (std::size_t search_index = 0; search_index != states.size();
                ++search_index)
            {
                if (!states[search_index].complete)
                {
                    all_complete = false;
                    prepare_diagonal_probes(states[search_index], search_index,
                        len1, len2, table1, table2, batches1, batches2,
                        values[search_index]);
                }
            }

            if (all_complete)
            {
                break;
            }

            std::vector<hpx::future<result_type1>> operations1;
            std::vector<hpx::future<result_type2>> operations2;
            operations1.reserve(batches1.size());
            operations2.reserve(batches2.size());

            for (auto& batch : batches1)
            {
                operations1.push_back(capture_projected_values_async<ExPolicy,
                    Key1, local_iterator1>(batch.routing_partition_id,
                    HPX_MOVE(batch.requests), proj1));
            }

            for (auto& batch : batches2)
            {
                operations2.push_back(capture_projected_values_async<ExPolicy,
                    Key2, local_iterator2>(batch.routing_partition_id,
                    HPX_MOVE(batch.requests), proj2));
            }

            auto completed1 =
                get_capture_results<ExPolicy>(HPX_MOVE(operations1));
            auto completed2 =
                get_capture_results<ExPolicy>(HPX_MOVE(operations2));

            for (auto& results : completed1)
            {
                store_input1_probe_results<Key1, Key2>(
                    HPX_MOVE(results), values);
            }
            for (auto& results : completed2)
            {
                store_input2_probe_results<Key1, Key2>(
                    HPX_MOVE(results), values);
            }

            for (std::size_t search_index = 0; search_index != states.size();
                ++search_index)
            {
                update_diagonal_state(states[search_index], len1, len2,
                    values[search_index], comp);
            }
        }
    }

    template <typename SegmentIterator, typename LocalIterator>
    struct output_chunk_position
    {
        SegmentIterator segment;
        LocalIterator dest;
        std::size_t k0;
        std::size_t k1;
    };

    template <typename Chunk>
    struct destination_chunk_batch
    {
        hpx::id_type locality_id;
        hpx::id_type routing_partition_id;
        std::vector<Chunk> chunks;
    };

    // Divide the destination range at destination-partition boundaries.
    //
    // Every emitted chunk is a half-open output diagonal interval [k0, k1) and
    // starts at a partition-relative destination iterator. Chunks never cross a
    // destination partition, allowing each one to execute on the locality
    // owning its output.
    //
    // The callback records or processes each chunk while this function advances
    // through the segmented destination range.

    template <typename Traits3, typename Iter3, typename F>
    HPX_FORCEINLINE auto for_each_output_chunk(
        Traits3, Iter3 const& dest, std::size_t total_size, F&& handle_chunk)
        -> std::pair<typename Traits3::segment_iterator,
            typename Traits3::local_iterator>
    {
        using segment_iterator_out = Traits3::segment_iterator;
        using out_local_iterator_type = Traits3::local_iterator;

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
        using segment_iterator = Traits::segment_iterator;
        using batch_type = destination_chunk_batch<Chunk>;

        std::vector<batch_type> batches;
        segment_iterator final_segment;
        std::size_t final_batch_index = 0;
        std::size_t final_chunk_position = 0;
    };

    template <typename Iter1, typename Iter2, typename Iter3>
    struct segmented_merge_types
    {
        using input_traits1 =
            hpx::traits::segmented_iterator_traits<std::decay_t<Iter1>>;
        using input_local_iterator1 = input_traits1::local_iterator;
        using input_local_traits1 =
            hpx::traits::segmented_local_iterator_traits<input_local_iterator1>;
        using input_raw_iterator1 = input_local_traits1::local_raw_iterator;
        using input_reference1 =
            std::iterator_traits<input_raw_iterator1>::reference;
        using input_traits2 =
            hpx::traits::segmented_iterator_traits<std::decay_t<Iter2>>;
        using input_local_iterator2 = input_traits2::local_iterator;
        using input_local_traits2 =
            hpx::traits::segmented_local_iterator_traits<input_local_iterator2>;
        using input_raw_iterator2 = input_local_traits2::local_raw_iterator;
        using input_reference2 =
            std::iterator_traits<input_raw_iterator2>::reference;
        template <typename Proj>
        using projected_key_type1 =
            std::decay_t<std::invoke_result_t<Proj&, input_reference1>>;
        template <typename Proj>
        using projected_key_type2 =
            std::decay_t<std::invoke_result_t<Proj&, input_reference2>>;
        using destination_traits =
            hpx::traits::segmented_iterator_traits<std::decay_t<Iter3>>;
        using local_iterator = destination_traits::local_iterator;
        using value_type1 = std::iterator_traits<Iter1>::value_type;
        using value_type2 = std::iterator_traits<Iter2>::value_type;
        using range_list1_type = decltype(make_partition_ranges(
            std::declval<Iter1>(), std::declval<Iter1>()));
        using range_list2_type = decltype(make_partition_ranges(
            std::declval<Iter2>(), std::declval<Iter2>()));
        using chunk_type = capture_dispatch_chunk<range_list1_type,
            range_list2_type, local_iterator>;
        using chunk_batches_type =
            destination_chunk_batches<destination_traits, chunk_type>;
        using batch_type = chunk_batches_type::batch_type;
        using batch_result_type = std::vector<local_iterator>;
    };

    // Construct all remote work required for the segmented merge.
    //
    // Destination partitions first define output chunks and their merge
    // diagonals. All diagonal intersections are then resolved to determine
    // exactly which portion of each input contributes to every chunk.
    //
    // Each chunk stores its input sizes, partition-relative input ranges, and
    // local destination iterator. Chunks are grouped by destination locality so
    // one remote action can process multiple destination partitions on that
    // locality.
    //
    // final_batch_index and final_chunk_position identify the chunk producing
    // the algorithm's final output iterator.

    template <typename ExPolicy, typename Traits3, typename Chunk,
        typename Iter1, typename Iter2, typename Iter3, typename Comp,
        typename Proj1, typename Proj2, typename IsSeq>
    destination_chunk_batches<Traits3, Chunk> make_destination_chunk_batches(
        Traits3, Iter1 const& first1, std::size_t len1, Iter2 const& first2,
        std::size_t len2, Iter3 const& dest, Comp& comp, Proj1& proj1,
        Proj2& proj2, IsSeq is_seq)
    {
        using chunk_batches_type = destination_chunk_batches<Traits3, Chunk>;
        using batch_type = chunk_batches_type::batch_type;
        using segment_iterator = Traits3::segment_iterator;
        using local_iterator = Traits3::local_iterator;
        using output_position_type =
            output_chunk_position<segment_iterator, local_iterator>;
        using merge_types = segmented_merge_types<Iter1, Iter2, Iter3>;
        using key_type1 = merge_types::template projected_key_type1<Proj1>;
        using key_type2 = merge_types::template projected_key_type2<Proj2>;

        auto table1 =
            make_segmented_range_table(first1, std::next(first1, len1));
        auto table2 =
            make_segmented_range_table(first2, std::next(first2, len2));

        std::vector<output_position_type> output_positions;

        auto output_position =
            for_each_output_chunk(Traits3{}, dest, len1 + len2,
                [&](segment_iterator segment, local_iterator output_first,
                    std::size_t k0, std::size_t k1) {
                    output_positions.push_back(output_position_type{
                        HPX_MOVE(segment), HPX_MOVE(output_first), k0, k1});
                });

        chunk_batches_type chunk_batches{{}, HPX_MOVE(output_position.first)};

        std::vector<diagonal_search_state> states;
        states.reserve(output_positions.size() + 1);
        states.push_back(make_diagonal_state(len1, len2, 0));

        for (auto const& position : output_positions)
        {
            states.push_back(make_diagonal_state(len1, len2, position.k1));
        }

        resolve_diagonal_intersections<ExPolicy, key_type1, key_type2>(
            states, len1, len2, table1, table2, comp, proj1, proj2, is_seq);

        HPX_ASSERT(states.size() == output_positions.size() + 1);

        for (std::size_t index = 0; index != output_positions.size(); ++index)
        {
            auto const& first_position = states[index];
            auto const& last_position = states[index + 1];
            auto& output = output_positions[index];

            HPX_ASSERT(first_position.complete);
            HPX_ASSERT(last_position.complete);

            Chunk chunk{last_position.a - first_position.a,
                last_position.b - first_position.b,
                make_partition_ranges(std::next(first1, first_position.a),
                    std::next(first1, last_position.a)),
                make_partition_ranges(std::next(first2, first_position.b),
                    std::next(first2, last_position.b)),
                HPX_MOVE(output.dest)};

            hpx::id_type const partition_id = Traits3::get_id(output.segment);
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
        }
        HPX_ASSERT(!chunk_batches.batches.empty());

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
        using traits3 = types::destination_traits;
        using local_iterator = types::local_iterator;
        using value_type1 = types::value_type1;
        using value_type2 = types::value_type2;
        using chunk_batches_type = types::chunk_batches_type;
        using batch_type = types::batch_type;
        using batch_result_type = types::batch_result_type;
        using policy_type = std::decay_t<ExPolicy>;
        using result_value_type = util::in_in_out_result<Iter1, Iter2, Iter3>;
        using result =
            util::detail::algorithm_result<ExPolicy, result_value_type>;

        static constexpr bool is_task_policy =
            hpx::is_async_execution_policy_v<policy_type>;

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

            auto planning =
                hpx::async([state, first1, first2, dest, len1,
                               len2]() mutable -> chunk_batches_type {
                    try
                    {
                        return make_destination_chunk_batches<policy_type,
                            traits3, typename types::chunk_type>(traits3{},
                            first1, len1, first2, len2, dest, state->comparator,
                            state->projection1, state->projection2,
                            std::true_type{});
                    }
                    catch (...)
                    {
                        hpx::parallel::util::detail::handle_local_exceptions<
                            policy_type>::call(std::current_exception());
                        std::terminate();
                    }
                });

            return HPX_MOVE(planning).then(
                [state, last1 = HPX_MOVE(last1), last2 = HPX_MOVE(last2)](
                    hpx::future<chunk_batches_type> ready_batches) mutable
                    -> hpx::future<result_value_type> {
                    auto chunk_batches = ready_batches.get();

                    auto final_local =
                        std::make_shared<std::optional<local_iterator>>();

                    hpx::future<void> chain = hpx::make_ready_future();

                    for (std::size_t index = 0;
                        index != chunk_batches.batches.size(); ++index)
                    {
                        bool const is_final =
                            index == chunk_batches.final_batch_index;
                        batch_type batch =
                            HPX_MOVE(chunk_batches.batches[index]);
                        std::size_t const final_position =
                            chunk_batches.final_chunk_position;

                        chain = HPX_MOVE(chain).then(
                            [state, batch = HPX_MOVE(batch), final_local,
                                is_final, final_position](
                                hpx::future<void> previous) mutable
                                -> hpx::future<void> {
                                previous.get();

                                auto operation =
                                    capture_dispatch_batch_async<value_type1,
                                        value_type2>(batch.routing_partition_id,
                                        state->algorithm,
                                        state->execution_policy,
                                        std::true_type{},
                                        HPX_MOVE(batch.chunks),
                                        state->comparator, state->projection1,
                                        state->projection2);

                                return HPX_MOVE(operation).then(
                                    [final_local, is_final, final_position](
                                        hpx::future<batch_result_type> ready) {
                                        auto values = ready.get();

                                        if (is_final)
                                        {
                                            HPX_ASSERT(
                                                final_position < values.size());
                                            final_local->emplace(HPX_MOVE(
                                                values[final_position]));
                                        }
                                    });
                            });
                    }

                    auto end_dest = HPX_MOVE(chain).then(
                        [final_segment = HPX_MOVE(chunk_batches.final_segment),
                            final_local](
                            hpx::future<void> ready) mutable -> Iter3 {
                            ready.get();
                            HPX_ASSERT(final_local->has_value());
                            return traits3::compose(
                                final_segment, HPX_MOVE(**final_local));
                        });

                    return make_merge_result_future<result_value_type>(
                        HPX_MOVE(end_dest), HPX_MOVE(last1), HPX_MOVE(last2));
                });
        }
        else
        {
            std::optional<chunk_batches_type> chunk_batches;

            try
            {
                chunk_batches.emplace(
                    make_destination_chunk_batches<policy_type, traits3,
                        typename types::chunk_type>(traits3{}, first1, len1,
                        first2, len2, dest, comp, proj1, proj2,
                        std::true_type{}));
            }
            catch (...)
            {
                return handle_merge_planning_exception<ExPolicy,
                    result_value_type>(std::current_exception());
            }

            HPX_ASSERT(chunk_batches.has_value());

            std::optional<local_iterator> final_local;

            for (std::size_t index = 0; index != chunk_batches->batches.size();
                ++index)
            {
                auto& batch = chunk_batches->batches[index];

                auto values =
                    capture_dispatch_batch_async<value_type1, value_type2>(
                        batch.routing_partition_id, algo, policy,
                        std::true_type{}, HPX_MOVE(batch.chunks), comp, proj1,
                        proj2)
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
                chunk_batches->final_segment, HPX_MOVE(*final_local));

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
        using traits3 = types::destination_traits;
        using value_type1 = types::value_type1;
        using value_type2 = types::value_type2;
        using chunk_batches_type = types::chunk_batches_type;
        using batch_result_type = types::batch_result_type;
        using policy_type = std::decay_t<ExPolicy>;
        using result_value_type = util::in_in_out_result<Iter1, Iter2, Iter3>;

        static constexpr bool is_task_policy =
            hpx::is_async_execution_policy_v<policy_type>;

        auto execute_batches =
            [last1 = HPX_MOVE(last1), last2 = HPX_MOVE(last2)](auto& algorithm,
                auto& execution_policy, auto& comparator, auto& projection1,
                auto& projection2, chunk_batches_type chunk_batches) mutable
            -> hpx::future<result_value_type> {
            std::vector<hpx::future<batch_result_type>> operations;
            operations.reserve(chunk_batches.batches.size());

            for (auto& batch : chunk_batches.batches)
            {
                operations.push_back(
                    capture_dispatch_batch_async<value_type1, value_type2>(
                        batch.routing_partition_id, algorithm, execution_policy,
                        std::false_type{}, HPX_MOVE(batch.chunks), comparator,
                        projection1, projection2));
            }

            auto end_dest =
                hpx::when_all(HPX_MOVE(operations))
                    .then([final_segment =
                                  HPX_MOVE(chunk_batches.final_segment),
                              final_batch = chunk_batches.final_batch_index,
                              final_chunk = chunk_batches.final_chunk_position](
                              auto ready) mutable -> Iter3 {
                        auto completed =
                            get_capture_results<policy_type>(ready.get());

                        HPX_ASSERT(final_batch < completed.size());
                        auto final_values = HPX_MOVE(completed[final_batch]);
                        HPX_ASSERT(final_chunk < final_values.size());

                        return traits3::compose(
                            final_segment, HPX_MOVE(final_values[final_chunk]));
                    });

            return make_merge_result_future<result_value_type>(
                HPX_MOVE(end_dest), HPX_MOVE(last1), HPX_MOVE(last2));
        };

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

            auto planning =
                hpx::async([state, first1, first2, dest, len1,
                               len2]() mutable -> chunk_batches_type {
                    try
                    {
                        return make_destination_chunk_batches<policy_type,
                            traits3, typename types::chunk_type>(traits3{},
                            first1, len1, first2, len2, dest, state->comparator,
                            state->projection1, state->projection2,
                            std::false_type{});
                    }
                    catch (...)
                    {
                        hpx::parallel::util::detail::handle_local_exceptions<
                            policy_type>::call(std::current_exception());
                        std::terminate();
                    }
                });

            return HPX_MOVE(planning).then(
                [state, execute_batches = HPX_MOVE(execute_batches)](
                    hpx::future<chunk_batches_type> ready_batches) mutable
                    -> hpx::future<result_value_type> {
                    return execute_batches(state->algorithm,
                        state->execution_policy, state->comparator,
                        state->projection1, state->projection2,
                        ready_batches.get());
                });
        }
        else
        {
            std::optional<chunk_batches_type> chunk_batches;

            try
            {
                chunk_batches.emplace(
                    make_destination_chunk_batches<policy_type, traits3,
                        typename types::chunk_type>(traits3{}, first1, len1,
                        first2, len2, dest, comp, proj1, proj2,
                        std::false_type{}));
            }
            catch (...)
            {
                return handle_merge_planning_exception<ExPolicy,
                    result_value_type>(std::current_exception());
            }

            HPX_ASSERT(chunk_batches.has_value());

            auto operation = execute_batches(
                algo, policy, comp, proj1, proj2, HPX_MOVE(*chunk_batches));

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
        using output_local_iterator = traits3::local_iterator;

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
        using output_local_iterator = traits3::local_iterator;

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
        using output_local_iterator = traits3::local_iterator;

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
        using output_local_iterator = traits3::local_iterator;

        return hpx::parallel::detail::segmented_merge(
            hpx::parallel::detail::captured_merge<output_local_iterator>{},
            hpx::execution::seq, first1, last1, first2, last2, dest,
            HPX_MOVE(comp), HPX_MOVE(proj1), HPX_MOVE(proj2));
    }

}    // namespace hpx::segmented
