//  Copyright (c) 2026 Bharath Kollanur
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <hpx/assert.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/async_colocated.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/distribution_policies.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/segmented_algorithms/detail/dispatch.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iterator>
#include <limits>
#include <list>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////

    template <typename ExPolicy, typename T>
    HPX_FORCEINLINE T get_capture_result(hpx::future<T>&& ready)
    {
        using policy_type = std::decay_t<ExPolicy>;

        if (ready.has_exception())
        {
            std::exception_ptr const exception = ready.get_exception_ptr();

            std::list<std::exception_ptr> errors;

            parallel::util::detail::handle_remote_exceptions<policy_type>::call(
                exception, errors);

            if (errors.empty())
            {
                std::rethrow_exception(exception);
            }

            throw hpx::exception_list(HPX_MOVE(errors));
        }

        return ready.get();
    }

    // Handles: future<T>
    template <typename ExPolicy, typename T>
    HPX_FORCEINLINE hpx::future<T> handle_capture_exceptions(
        hpx::future<T>&& operation)
    {
        return HPX_MOVE(operation).then([](hpx::future<T> ready) -> T {
            return get_capture_result<ExPolicy>(HPX_MOVE(ready));
        });
    }

    // Handles: future<future<T>> created by a task-policy action
    template <typename ExPolicy, typename T>
    HPX_FORCEINLINE hpx::future<T> handle_capture_exceptions(
        hpx::future<hpx::future<T>>&& operation)
    {
        hpx::future<T> flattened = HPX_MOVE(operation);

        return handle_capture_exceptions<ExPolicy>(HPX_MOVE(flattened));
    }

    template <typename ExPolicy, typename T>
    std::vector<T> get_capture_results(std::vector<hpx::future<T>> operations)
    {
        bool const has_exceptions = hpx::wait_all_nothrow(operations);

        if (has_exceptions)
        {
            std::list<std::exception_ptr> errors;
            parallel::util::detail::handle_remote_exceptions<
                std::decay_t<ExPolicy>>::call(operations, errors);

            if (!errors.empty())
            {
                throw hpx::exception_list(HPX_MOVE(errors));
            }

            // Defensive fallback: an exceptional future was detected,
            // but the policy handler neither threw nor recorded it.
            for (auto& operation : operations)
            {
                if (operation.has_exception())
                {
                    operation.get();    // rethrow the exception
                }
            }

            HPX_UNREACHABLE;
        }

        std::vector<T> results;
        results.reserve(operations.size());

        for (auto& operation : operations)
        {
            results.push_back(operation.get());
        }

        return results;
    }

    template <typename LocalIterator>
    struct partition_range
    {
        hpx::id_type partition_id;
        LocalIterator first;
        LocalIterator last;
    };

    template <typename RangeList1, typename RangeList2, typename OutIterator>
    struct capture_dispatch_chunk
    {
        std::size_t input1_size;
        std::size_t input2_size;

        RangeList1 ranges1;
        RangeList2 ranges2;

        OutIterator dest;
    };

    template <typename LocalIterator>
    struct indexed_partition_range
    {
        std::size_t original_index;
        partition_range<LocalIterator> range;
    };

    struct projected_value_target
    {
        std::size_t search_index;
        std::uint8_t operand_index;
    };

    template <typename LocalIterator>
    struct projected_value_request
    {
        hpx::id_type partition_id;
        std::vector<projected_value_target> targets;
        LocalIterator position;
    };

    template <typename Key>
    struct projected_value_result
    {
        static_assert(std::is_move_constructible_v<Key>,
            "The projected key type used by segmented merge must be "
            "move constructible.");

        std::vector<projected_value_target> targets;

        // Contains exactly one projected key.
        // Keeping the key in collection allows HPX
        // to use construction-aware deserialisation for Key
        std::vector<Key> values;
    };

    struct collected_range_slice
    {
        std::size_t original_index;
        std::size_t offset;
        std::size_t size;
    };

    template <typename Value>
    struct collected_partition_values
    {
        std::vector<Value> values;
        std::vector<collected_range_slice> slices;
    };

    template <typename LocalIterator>
    struct locality_range_batch
    {
        hpx::id_type locality_id;

        hpx::id_type routing_partition_id;

        std::vector<indexed_partition_range<LocalIterator>> ranges;
    };

    HPX_FORCEINLINE hpx::id_type get_partition_locality(
        hpx::id_type const& partition_id)
    {
        if (hpx::naming::detail::is_migratable(partition_id.get_gid()))
        {
            return hpx::get_colocation_id(hpx::launch::sync, partition_id);
        }

        return hpx::naming::get_locality_from_id(partition_id);
    }

    template <typename Value, typename LocalIterator>
    struct transmitter
    {
        using indexed_range_type = indexed_partition_range<LocalIterator>;
        using range_list_type = std::vector<indexed_range_type>;
        using result_type = collected_partition_values<Value>;

        static result_type send_values(range_list_type ranges)
        {
            using iterator_traits =
                hpx::traits::segmented_local_iterator_traits<LocalIterator>;

            result_type result;
            result.slices.reserve(ranges.size());

            std::size_t total_size = 0;

            for (auto const& indexed_range : ranges)
            {
                auto const& range = indexed_range.range;

                total_size += static_cast<std::size_t>(
                    std::distance(range.first, range.last));
            }

            result.values.reserve(total_size);

            for (auto& indexed_range : ranges)
            {
                auto& range = indexed_range.range;

                auto raw_first = iterator_traits::local(HPX_MOVE(range.first));
                auto raw_last = iterator_traits::local(HPX_MOVE(range.last));

                std::size_t const offset = result.values.size();

                result.values.insert(result.values.end(), raw_first, raw_last);

                std::size_t const size = result.values.size() - offset;

                result.slices.emplace_back(collected_range_slice{
                    indexed_range.original_index, offset, size});
            }
            return result;
        }
    };

    template <typename Value, typename LocalIterator>
    struct send_values_action
      : hpx::actions::make_action<
            collected_partition_values<Value> (*)(
                std::vector<indexed_partition_range<LocalIterator>>),
            &transmitter<Value, LocalIterator>::send_values,
            send_values_action<Value, LocalIterator>>::type
    {
    };

    template <typename Value, typename LocalIterator>
    hpx::future<collected_partition_values<Value>> capture_async(
        hpx::id_type const& routing_partition_id,
        std::vector<indexed_partition_range<LocalIterator>> ranges)
    {
        using iterator_type = std::decay_t<LocalIterator>;

        send_values_action<Value, iterator_type> act;

        return hpx::async(
            act, hpx::colocated(routing_partition_id), HPX_MOVE(ranges));
    }

    template <typename Key, typename LocalIterator, typename Proj>
    struct projected_value_collector
    {
        using request_type = projected_value_request<LocalIterator>;
        using result_type = projected_value_result<Key>;

        static std::vector<result_type> get_values(
            std::vector<request_type> requests, Proj projection)
        {
            using local_traits =
                hpx::traits::segmented_local_iterator_traits<LocalIterator>;

            std::vector<result_type> results;
            results.reserve(requests.size());

            for (auto& request : requests)
            {
                auto raw_position =
                    local_traits::local(HPX_MOVE(request.position));
                Key value = HPX_INVOKE(projection, *raw_position);

                std::vector<Key> values;
                values.reserve(1);
                values.emplace_back(HPX_MOVE(value));

                results.emplace_back(
                    result_type{HPX_MOVE(request.targets), HPX_MOVE(values)});
            }

            return results;
        }
    };

    template <typename Key, typename LocalIterator, typename Proj>
    struct get_projected_values_action
      : hpx::actions::make_action<
            std::vector<projected_value_result<Key>> (*)(
                std::vector<projected_value_request<LocalIterator>>, Proj),
            &projected_value_collector<Key, LocalIterator, Proj>::get_values,
            get_projected_values_action<Key, LocalIterator, Proj>>::type
    {
    };

    template <typename ExPolicy, typename Key, typename LocalIterator,
        typename Proj>
    hpx::future<std::vector<projected_value_result<Key>>>
    capture_projected_values_async(hpx::id_type const& routing_partition_id,
        std::vector<projected_value_request<std::decay_t<LocalIterator>>>
            requests,
        Proj&& projection)
    {
        using iterator_type = std::decay_t<LocalIterator>;
        using projection_type = std::decay_t<Proj>;

        get_projected_values_action<Key, iterator_type, projection_type> act;

        return handle_capture_exceptions<ExPolicy>(
            hpx::async(act, hpx::colocated(routing_partition_id),
                HPX_MOVE(requests), HPX_FORWARD(Proj, projection)));
    }

    template <typename Iterator>
    auto make_partition_ranges(Iterator first, Iterator last)
    {
        using traits = hpx::traits::segmented_iterator_traits<Iterator>;

        using segment_iterator = traits::segment_iterator;
        using local_iterator = traits::local_iterator;

        using range_type = partition_range<local_iterator>;

        std::vector<range_type> ranges;

        if (first == last)
        {
            return ranges;
        }

        Iterator final_element = std::prev(last);
        segment_iterator seg_first = traits::segment(first);
        segment_iterator seg_last = traits::segment(final_element);

        auto const segment_count =
            static_cast<std::size_t>(std::distance(seg_first, seg_last) + 1);

        ranges.reserve(segment_count);

        local_iterator local_first = traits::local(first);
        local_iterator final_last = traits::local(final_element);
        ++final_last;

        auto append_range = [&](segment_iterator const& segment,
                                local_iterator range_first,
                                local_iterator range_last) {
            if (range_first != range_last)
            {
                ranges.emplace_back(range_type{traits::get_id(segment),
                    HPX_MOVE(range_first), HPX_MOVE(range_last)});
            }
        };

        if (seg_first == seg_last)
        {
            append_range(
                seg_first, HPX_MOVE(local_first), HPX_MOVE(final_last));

            return ranges;
        }

        append_range(seg_first, HPX_MOVE(local_first), traits::end(seg_first));

        segment_iterator segment = seg_first;

        for (++segment; segment != seg_last; ++segment)
        {
            append_range(segment, traits::begin(segment), traits::end(segment));
        }

        append_range(seg_last, traits::begin(seg_last), HPX_MOVE(final_last));

        return ranges;
    }

    template <typename ExPolicy, typename IsSeq>
    struct range_collector
    {
        using policy_type = std::decay_t<ExPolicy>;
        using is_seq = std::decay_t<IsSeq>;

        template <typename Value, typename LocalIterator>
        static hpx::future<collected_partition_values<Value>>
        get_partition_values(hpx::id_type const& destination_locality,
            hpx::id_type const& source_locality,
            hpx::id_type const& routing_partition_id,
            std::vector<indexed_partition_range<LocalIterator>> ranges)
        {
            using iterator_type = std::decay_t<LocalIterator>;
            using result_type = collected_partition_values<Value>;

            if (source_locality == destination_locality)
            {
                auto copy_values =
                    [ranges = HPX_MOVE(ranges)]() mutable -> result_type {
                    return transmitter<Value, iterator_type>::send_values(
                        HPX_MOVE(ranges));
                };

                if constexpr (is_seq::value)
                {
                    return hpx::make_ready_future(copy_values());
                }
                else
                {
                    return hpx::async(HPX_MOVE(copy_values));
                }
            }

            return capture_async<Value, iterator_type>(
                routing_partition_id, HPX_MOVE(ranges));
        }

        template <typename Value, typename LocalIterator>
        static std::vector<Value> collect_range(
            std::vector<partition_range<LocalIterator>> ranges)
        {
            using values_type = std::vector<Value>;
            using indexed_range_type = indexed_partition_range<LocalIterator>;
            using batch_type = locality_range_batch<LocalIterator>;
            using collected_type = collected_partition_values<Value>;

            if (ranges.empty())
            {
                return values_type{};
            }

            std::size_t const number_of_ranges = ranges.size();

            hpx::id_type const destination_locality = hpx::find_here();

            std::vector<batch_type> batches;
            batches.reserve(ranges.size());

            for (std::size_t index = 0; index != ranges.size(); ++index)
            {
                auto& range = ranges[index];

                hpx::id_type const source_locality =
                    get_partition_locality(range.partition_id);

                auto batch = std::find_if(batches.begin(), batches.end(),
                    [&source_locality](batch_type const& candidate) {
                        return candidate.locality_id == source_locality;
                    });

                if (batch == batches.end())
                {
                    batches.emplace_back(
                        batch_type{source_locality, range.partition_id, {}});

                    batch = std::prev(batches.end());
                }

                batch->ranges.emplace_back(
                    indexed_range_type{index, HPX_MOVE(range)});
            }

            std::vector<collected_type> batch_results;

            if constexpr (is_seq::value)
            {
                batch_results.reserve(batches.size());

                for (auto& batch : batches)
                {
                    batch_results.emplace_back(get_partition_values<Value>(
                        destination_locality, batch.locality_id,
                        batch.routing_partition_id, HPX_MOVE(batch.ranges))
                            .get());
                }
            }
            else
            {
                std::vector<hpx::future<collected_type>> batch_futures;

                batch_futures.reserve(batches.size());

                for (auto& batch : batches)
                {
                    batch_futures.emplace_back(get_partition_values<Value>(
                        destination_locality, batch.locality_id,
                        batch.routing_partition_id, HPX_MOVE(batch.ranges)));
                }

                batch_results =
                    get_capture_results<policy_type>(HPX_MOVE(batch_futures));
            }

            struct range_location
            {
                std::size_t batch_index;
                std::size_t offset;
                std::size_t size;
            };

            constexpr std::size_t invalid_index =
                (std::numeric_limits<std::size_t>::max)();

            std::vector<range_location> locations(
                number_of_ranges, range_location{invalid_index, 0, 0});

            std::size_t total_size = 0;

            for (std::size_t batch_index = 0;
                batch_index != batch_results.size(); ++batch_index)
            {
                auto const& result = batch_results[batch_index];

                total_size += result.values.size();

                for (auto const& slice : result.slices)
                {
                    HPX_ASSERT(slice.original_index < locations.size());

                    auto& location = locations[slice.original_index];

                    HPX_ASSERT(location.batch_index == invalid_index);

                    location =
                        range_location{batch_index, slice.offset, slice.size};
                }
            }

            values_type values;
            values.reserve(total_size);

            for (auto const& location : locations)
            {
                HPX_ASSERT(location.batch_index != invalid_index);

                HPX_ASSERT(location.batch_index < batch_results.size());

                auto& source = batch_results[location.batch_index].values;

                HPX_ASSERT(location.offset < source.size());
                HPX_ASSERT(location.offset + location.size <= source.size());

                auto first = source.begin() + location.offset;

                auto last = first + location.size;

                values.insert(values.end(), std::make_move_iterator(first),
                    std::make_move_iterator(last));
            }

            return values;
        }
    };

    template <typename OutputIterator, typename IsSeq, typename Dispatcher,
        typename Algo, typename ExPolicy, typename... CallArgs>
    HPX_FORCEINLINE parallel::util::detail::algorithm_result_t<ExPolicy,
        std::decay_t<OutputIterator>>
    invoke_capture_dispatcher(
        Algo const& algo, ExPolicy policy, CallArgs&&... args)
    {
        using output_iterator = std::decay_t<OutputIterator>;

        auto complete_result = [&]() {
            if constexpr (std::decay_t<IsSeq>::value)
            {
                return Dispatcher::sequential(
                    algo, HPX_MOVE(policy), HPX_FORWARD(CallArgs, args)...);
            }
            else
            {
                return Dispatcher::parallel(
                    algo, HPX_MOVE(policy), HPX_FORWARD(CallArgs, args)...);
            }
        }();

        auto get_output = [](auto&& value) -> output_iterator {
            return HPX_MOVE(value.out);
        };

        if constexpr (hpx::is_async_execution_policy_v<std::decay_t<ExPolicy>>)
        {
            return hpx::make_future<output_iterator>(
                HPX_MOVE(complete_result), HPX_MOVE(get_output));
        }
        else
        {
            return get_output(HPX_MOVE(complete_result));
        }
    }

    template <typename Value1, typename Value2, typename Chunk, typename Algo,
        typename ExPolicy, typename IsSeq, typename... Args>
    struct batch_receiver
    {
        using chunk_type = std::decay_t<Chunk>;
        using chunk_list_type = std::vector<chunk_type>;

        using values_type1 = std::vector<Value1>;
        using values_type2 = std::vector<Value2>;

        using buffer_iterator1 = values_type1::iterator;
        using buffer_iterator2 = values_type2::iterator;

        using output_iterator = decltype(std::declval<chunk_type>().dest);

        using batch_result_type = std::vector<output_iterator>;

        using chunk_result_type =
            parallel::util::detail::algorithm_result_t<ExPolicy,
                output_iterator>;

        using result_type = parallel::util::detail::algorithm_result_t<ExPolicy,
            batch_result_type>;

        using is_seq = std::decay_t<IsSeq>;

        using range_list1_type =
            std::decay_t<decltype(std::declval<chunk_type>().ranges1)>;

        using range_list2_type =
            std::decay_t<decltype(std::declval<chunk_type>().ranges2)>;

        using dispatcher_type = dispatcher<std::decay_t<Algo>, ExPolicy,
            buffer_iterator1, buffer_iterator1, buffer_iterator2,
            buffer_iterator2, output_iterator, std::decay_t<Args>...>;

        template <typename... CallArgs>
        static auto invoke_dispatcher(
            Algo const& algo, ExPolicy policy, CallArgs&&... args)
        {
            return invoke_capture_dispatcher<output_iterator, is_seq,
                dispatcher_type>(
                algo, HPX_MOVE(policy), HPX_FORWARD(CallArgs, args)...);
        }

        template <typename InputIterator>
        static chunk_result_type copy_chunk(ExPolicy policy,
            InputIterator first, InputIterator last, output_iterator dest)
        {
            using output_traits =
                hpx::traits::segmented_local_iterator_traits<output_iterator>;

            auto raw_dest = output_traits::local(HPX_MOVE(dest));
            auto raw_result = hpx::copy(policy, first, last, raw_dest);

            if constexpr (hpx::is_async_execution_policy_v<
                              std::decay_t<ExPolicy>>)
            {
                return raw_result.then([](auto ready) {
                    return output_traits::remote(ready.get());
                });
            }
            else
            {
                return output_traits::remote(HPX_MOVE(raw_result));
                ;
            }
        }

        template <typename... CallArgs>
        static chunk_result_type invoke_chunk(Algo const& algo, ExPolicy policy,
            buffer_iterator1 first1, buffer_iterator1 last1,
            buffer_iterator2 first2, buffer_iterator2 last2,
            output_iterator dest, CallArgs&&... args)
        {
            if (first1 == last1)
            {
                return copy_chunk(policy, first2, last2, HPX_MOVE(dest));
            }

            if (first2 == last2)
            {
                return copy_chunk(policy, first1, last1, HPX_MOVE(dest));
            }

            return invoke_dispatcher(algo, HPX_MOVE(policy), first1, last1,
                first2, last2, HPX_MOVE(dest), HPX_FORWARD(CallArgs, args)...);
        }

        static result_type invoke_chunks_sequential(Algo const& algo,
            ExPolicy policy, chunk_list_type chunks,
            std::shared_ptr<values_type1> values1,
            std::shared_ptr<values_type2> values2, Args... args)
        {
            using policy_type = std::decay_t<ExPolicy>;

            static constexpr bool is_task_policy =
                hpx::is_async_execution_policy_v<policy_type>;

            if constexpr (!is_task_policy)
            {
                batch_result_type results;
                results.reserve(chunks.size());

                for_each_chunk(chunks, values1, values2,
                    [&](auto first1, auto last1, auto first2, auto last2,
                        output_iterator dest) {
                        results.emplace_back(invoke_chunk(algo, policy, first1,
                            last1, first2, last2, HPX_MOVE(dest), args...));
                    });

                return results;
            }
            else
            {
                hpx::future<batch_result_type> operation =
                    hpx::make_ready_future(batch_result_type{});

                for_each_chunk(chunks, values1, values2,
                    [&](auto first1, auto last1, auto first2, auto last2,
                        output_iterator dest) {
                        operation = HPX_MOVE(operation).then(
                            [algorithm = algo, operation_policy = policy,
                                first1, last1, first2, last2,
                                dest = HPX_MOVE(dest), values1, values2,
                                ... operation_args = args](
                                hpx::future<batch_result_type> previous) mutable
                                -> hpx::future<batch_result_type> {
                                auto results = previous.get();

                                auto chunk_operation =
                                    batch_receiver::invoke_chunk(algorithm,
                                        HPX_MOVE(operation_policy), first1,
                                        last1, first2, last2, HPX_MOVE(dest),
                                        HPX_MOVE(operation_args)...);

                                return HPX_MOVE(chunk_operation)
                                    .then([results = HPX_MOVE(results), values1,
                                              values2](
                                              hpx::future<output_iterator>
                                                  ready) mutable
                                              -> batch_result_type {
                                        results.push_back(ready.get());

                                        return HPX_MOVE(results);
                                    });
                            });
                    });
                return operation;
            }
        }

        static result_type invoke_chunks_parallel(Algo const& algo,
            ExPolicy policy, chunk_list_type chunks,
            std::shared_ptr<values_type1> values1,
            std::shared_ptr<values_type2> values2, Args... args)
        {
            using policy_type = std::decay_t<ExPolicy>;

            static constexpr bool is_task_policy =
                hpx::is_async_execution_policy_v<policy_type>;

            std::vector<hpx::future<output_iterator>> operations;

            operations.reserve(chunks.size());

            for_each_chunk(chunks, values1, values2,
                [&](auto first1, auto last1, auto first2, auto last2,
                    output_iterator dest) {
                    if constexpr (is_task_policy)
                    {
                        operations.push_back(
                            batch_receiver::invoke_chunk(algo, policy, first1,
                                last1, first2, last2, HPX_MOVE(dest), args...));
                    }
                    else
                    {
                        operations.push_back(hpx::async(
                            [algorithm = algo, operation_policy = policy,
                                first1, last1, first2, last2,
                                dest = HPX_MOVE(dest), values1, values2,
                                ... operation_args =
                                    args]() mutable -> output_iterator {
                                return batch_receiver::invoke_chunk(algorithm,
                                    HPX_MOVE(operation_policy), first1, last1,
                                    first2, last2, HPX_MOVE(dest),
                                    HPX_MOVE(operation_args)...);
                            }));
                    }
                });

            HPX_ASSERT(!operations.empty());

            auto complete =
                hpx::when_all(HPX_MOVE(operations))
                    .then([values1, values2](
                              auto ready) mutable -> batch_result_type {
                        return get_capture_results<policy_type>(ready.get());
                    });

            if constexpr (is_task_policy)
            {
                return complete;
            }
            else
            {
                return complete.get();
            }
        }

        static result_type invoke_chunks(Algo const& algo, ExPolicy policy,
            chunk_list_type chunks, std::shared_ptr<values_type1> values1,
            std::shared_ptr<values_type2> values2, Args... args)
        {
            HPX_ASSERT(!chunks.empty());

            validate_chunk_sizes(chunks, values1->size(), values2->size());

            if constexpr (is_seq::value)
            {
                return invoke_chunks_sequential(algo, HPX_MOVE(policy),
                    HPX_MOVE(chunks), HPX_MOVE(values1), HPX_MOVE(values2),
                    HPX_MOVE(args)...);
            }
            else
            {
                return invoke_chunks_parallel(algo, HPX_MOVE(policy),
                    HPX_MOVE(chunks), HPX_MOVE(values1), HPX_MOVE(values2),
                    HPX_MOVE(args)...);
            }
        }

        static void validate_chunk_sizes(
            chunk_list_type const& chunks, std::size_t size1, std::size_t size2)
        {
            std::size_t expected_size1 = 0;
            std::size_t expected_size2 = 0;

            for (auto const& chunk : chunks)
            {
                if (expected_size1 > size1 ||
                    chunk.input1_size > size1 - expected_size1 ||
                    expected_size2 > size2 ||
                    chunk.input2_size > size2 - expected_size2)
                {
                    HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                        "batch_receiver::validate_chunk_sizes",
                        "collected input sizes do not match chunk metadata");
                }

                expected_size1 += chunk.input1_size;
                expected_size2 += chunk.input2_size;
            }

            if (expected_size1 != size1 || expected_size2 != size2)
            {
                HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                    "batch_receiver::validate_chunk_sizes",
                    "collected input sizes do not match chunk metadata");
            }
        }

        template <typename F>
        static void for_each_chunk(chunk_list_type& chunks,
            std::shared_ptr<values_type1> const& values1,
            std::shared_ptr<values_type2> const& values2, F&& f)
        {
            std::size_t offset1 = 0;
            std::size_t offset2 = 0;

            for (auto& chunk : chunks)
            {
                auto first1 = values1->begin() + offset1;
                auto first2 = values2->begin() + offset2;

                offset1 += chunk.input1_size;
                offset2 += chunk.input2_size;

                HPX_INVOKE(f, first1, values1->begin() + offset1, first2,
                    values2->begin() + offset2, HPX_MOVE(chunk.dest));
            }
        }

        template <typename RangeList, typename GetRanges>
        static RangeList flatten_ranges(
            chunk_list_type& chunks, GetRanges&& get_ranges)
        {
            RangeList ranges;

            std::size_t count = 0;
            for (auto const& chunk : chunks)
            {
                count += HPX_INVOKE(get_ranges, chunk).size();
            }

            ranges.reserve(count);

            for (auto& chunk : chunks)
            {
                auto& chunk_ranges = HPX_INVOKE(get_ranges, chunk);
                ranges.insert(ranges.end(),
                    std::make_move_iterator(chunk_ranges.begin()),
                    std::make_move_iterator(chunk_ranges.end()));
            }

            return ranges;
        }

        static result_type getfrom_batch(Algo const& algo, ExPolicy policy,
            chunk_list_type chunks, Args... args)
        {
            HPX_ASSERT(!chunks.empty());

            auto ranges1 = flatten_ranges<range_list1_type>(
                chunks, [](auto& chunk) -> auto& { return chunk.ranges1; });
            auto ranges2 = flatten_ranges<range_list2_type>(
                chunks, [](auto& chunk) -> auto& { return chunk.ranges2; });

            if constexpr (is_seq::value)
            {
                auto shared1 = std::make_shared<values_type1>(
                    range_collector<ExPolicy, is_seq>::template collect_range<
                        Value1>(HPX_MOVE(ranges1)));
                auto shared2 = std::make_shared<values_type2>(
                    range_collector<ExPolicy, is_seq>::template collect_range<
                        Value2>(HPX_MOVE(ranges2)));

                return invoke_chunks(algo, HPX_MOVE(policy), HPX_MOVE(chunks),
                    HPX_MOVE(shared1), HPX_MOVE(shared2), HPX_MOVE(args)...);
            }
            else
            {
                auto values1_f = hpx::async(
                    [ranges = HPX_MOVE(ranges1)]() mutable -> values_type1 {
                        return range_collector<ExPolicy, is_seq>::
                            template collect_range<Value1>(HPX_MOVE(ranges));
                    });
                auto values2_f = hpx::async(
                    [ranges = HPX_MOVE(ranges2)]() mutable -> values_type2 {
                        return range_collector<ExPolicy, is_seq>::
                            template collect_range<Value2>(HPX_MOVE(ranges));
                    });

                static constexpr bool is_task_policy =
                    hpx::is_async_execution_policy_v<std::decay_t<ExPolicy>>;

                if constexpr (is_task_policy)
                {
                    return hpx::dataflow(
                        [algorithm = algo, policy = HPX_MOVE(policy),
                            chunks = HPX_MOVE(chunks),
                            ... stored_args = HPX_MOVE(args)](
                            hpx::future<values_type1> ready1,
                            hpx::future<values_type2> ready2) mutable
                            -> result_type {
                            auto shared1 =
                                std::make_shared<values_type1>(ready1.get());
                            auto shared2 =
                                std::make_shared<values_type2>(ready2.get());

                            return invoke_chunks(algorithm, HPX_MOVE(policy),
                                HPX_MOVE(chunks), HPX_MOVE(shared1),
                                HPX_MOVE(shared2), HPX_MOVE(stored_args)...);
                        },
                        HPX_MOVE(values1_f), HPX_MOVE(values2_f));
                }
                else
                {
                    auto shared1 =
                        std::make_shared<values_type1>(values1_f.get());
                    auto shared2 =
                        std::make_shared<values_type2>(values2_f.get());

                    return invoke_chunks(algo, HPX_MOVE(policy),
                        HPX_MOVE(chunks), HPX_MOVE(shared1), HPX_MOVE(shared2),
                        HPX_MOVE(args)...);
                }
            }
        }
    };

    template <typename Value1, typename Value2, typename Chunk, typename Algo,
        typename R, typename ExPolicy, typename IsSeq, typename... Args>
    struct get_values_from_chunk_batch_action
      : hpx::actions::make_action<R (*)(Algo const&, ExPolicy,
                                      std::vector<Chunk>, Args...),
            &batch_receiver<Value1, Value2, Chunk, Algo, ExPolicy, IsSeq,
                Args...>::getfrom_batch,
            get_values_from_chunk_batch_action<Value1, Value2, Chunk, Algo, R,
                ExPolicy, IsSeq, Args...>>::type
    {
    };

    template <typename Value1, typename Value2, typename Chunk, typename Algo,
        typename ExPolicy, typename IsSeq, typename... Args>
    HPX_FORCEINLINE
        hpx::future<std::vector<decltype(std::declval<Chunk>().dest)>>
        capture_dispatch_batch_async(hpx::id_type const& routing_partition_id,
            Algo&& algo, ExPolicy policy, IsSeq, std::vector<Chunk> chunks,
            Args&&... args)
    {
        HPX_ASSERT(!chunks.empty());

        using chunk_type = Chunk;
        using algo_type = std::decay_t<Algo>;

        using output_iterator = decltype(std::declval<chunk_type>().dest);

        using batch_result_type = std::vector<output_iterator>;

        using action_result_type =
            parallel::util::detail::algorithm_result_t<ExPolicy,
                batch_result_type>;

        get_values_from_chunk_batch_action<Value1, Value2, chunk_type,
            algo_type, action_result_type, ExPolicy, IsSeq,
            hpx::util::decay_unwrap_t<Args>...>
            act;

        return handle_capture_exceptions<ExPolicy>(hpx::async(act,
            hpx::colocated(routing_partition_id), HPX_FORWARD(Algo, algo),
            HPX_MOVE(policy), HPX_MOVE(chunks), HPX_FORWARD(Args, args)...));
    }
}    // namespace hpx::parallel::detail
