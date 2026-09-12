//  Copyright (c) 2026 Bharath Kollanur
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <hpx/assert.hpp>
#include <hpx/async_distributed/async.hpp>
#include <hpx/errors/exception_list.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/async_colocated.hpp>
#include <hpx/modules/distribution_policies.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/segmented_algorithms/detail/dispatch.hpp>
#include <hpx/serialization/vector.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iterator>
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
            std::list<std::exception_ptr> errors;

            parallel::util::detail::handle_remote_exceptions<policy_type>::call(
                ready.get_exception_ptr(), errors);

            // handle_remote_exceptions is expected to throw.
            HPX_ASSERT(errors.empty());
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
        return HPX_MOVE(operation).then(
            [](hpx::future<hpx::future<T>> outer) -> hpx::future<T> {
                // Handles an exception thrown before the inner
                // operation was returned.
                hpx::future<T> inner =
                    get_capture_result<ExPolicy>(HPX_MOVE(outer));

                // Handles an exception stored in the inner future.
                return handle_capture_exceptions<ExPolicy>(HPX_MOVE(inner));
            });
    }

    template <typename Value, typename Iterator>
    struct transmitter
    {
        using iterator_type = std::decay_t<Iterator>;
        using result_type = std::vector<Value>;

        static result_type send_values(iterator_type first, iterator_type last)
        {
            using iterator_traits =
                hpx::traits::segmented_local_iterator_traits<iterator_type>;

            auto raw_first = iterator_traits::local(HPX_MOVE(first));
            auto raw_last = iterator_traits::local(HPX_MOVE(last));

            result_type values;
            values.insert(values.end(), raw_first, raw_last);

            return values;
        }
    };

    template <typename Value, typename Iterator>
    struct send_values_action
      : hpx::actions::make_action<std::vector<Value> (*)(std::decay_t<Iterator>,
                                      std::decay_t<Iterator>),
            &transmitter<Value, std::decay_t<Iterator>>::send_values,
            send_values_action<Value, std::decay_t<Iterator>>>::type
    {
    };

    template <typename Value, typename Iterator>
    hpx::future<std::vector<Value>> capture_async(
        hpx::id_type const& partition, Iterator first, Iterator last)
    {
        send_values_action<Value, std::decay_t<Iterator>> act;

        return hpx::async(
            act, hpx::colocated(partition), HPX_MOVE(first), HPX_MOVE(last));
    }

    template <typename LocalIterator>
    struct partition_range
    {
        hpx::id_type partition_id;
        LocalIterator first;
        LocalIterator last;

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            ar & partition_id;
            ar & first;
            ar & last;
        }
    };

    template <typename Iterator>
    auto make_partition_ranges(Iterator first, Iterator last)
    {
        using iterator_type = std::decay_t<Iterator>;
        using traits = hpx::traits::segmented_iterator_traits<iterator_type>;

        using segment_iterator = typename traits::segment_iterator;
        using local_iterator = typename traits::local_iterator;

        using range_type = partition_range<local_iterator>;

        std::vector<range_type> ranges;

        if (first == last)
        {
            return ranges;
        }

        Iterator final_element = std::prev(last);
        segment_iterator seg_first = traits::segment(first);
        segment_iterator seg_last = traits::segment(final_element);

        local_iterator local_first = traits::local(first);
        local_iterator final_last = traits::local(final_element);
        ++final_last;

        auto append_range = [&](segment_iterator const& segment,
                                local_iterator range_first,
                                local_iterator range_last) {
            if (range_first != range_last)
            {
                ranges.push_back(range_type{traits::get_id(segment),
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

    template <typename IsSeq>
    struct range_collector
    {
        using is_seq = std::decay_t<IsSeq>;

        template <typename Value, typename LocalIterator>
        static hpx::future<std::vector<Value>> get_partition_values(
            hpx::id_type const& destination_locality,
            hpx::id_type const& partition_id, LocalIterator local_first,
            LocalIterator local_last)
        {
            using values_type = std::vector<Value>;

            using local_traits = hpx::traits::segmented_local_iterator_traits<
                std::decay_t<LocalIterator>>;

            hpx::id_type const partition_locality =
                hpx::get_colocation_id(hpx::launch::sync, partition_id);

            if (partition_locality == destination_locality)
            {
                auto copy_values =
                    [local_first = HPX_MOVE(local_first),
                        local_last =
                            HPX_MOVE(local_last)]() mutable -> values_type {
                    auto raw_first = local_traits::local(HPX_MOVE(local_first));
                    auto raw_last = local_traits::local(HPX_MOVE(local_last));

                    values_type values;
                    values.insert(values.end(), raw_first, raw_last);
                    return values;
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

            return capture_async<Value>(
                partition_id, HPX_MOVE(local_first), HPX_MOVE(local_last));
        }

        template <typename Value, typename LocalIterator>
        static std::vector<Value> collect_range(
            std::vector<partition_range<LocalIterator>> ranges)
        {
            using values_type = std::vector<Value>;
            values_type values;

            if (ranges.empty())
            {
                return values;
            }

            hpx::id_type const destination_locality = hpx::find_here();

            auto append = [&values](values_type part) {
                values.insert(values.end(),
                    std::make_move_iterator(part.begin()),
                    std::make_move_iterator(part.end()));
            };

            if constexpr (is_seq::value)
            {
                for (auto& range : ranges)
                {
                    append(get_partition_values<Value>(destination_locality,
                        range.partition_id, HPX_MOVE(range.first),
                        HPX_MOVE(range.last))
                            .get());
                }
            }
            else
            {
                std::vector<hpx::future<values_type>> partition_futures;

                partition_futures.reserve(ranges.size());

                for (auto& range : ranges)
                {
                    partition_futures.push_back(get_partition_values<Value>(
                        destination_locality, range.partition_id,
                        HPX_MOVE(range.first), HPX_MOVE(range.last)));
                }

                hpx::wait_all(partition_futures);

                for (auto& future : partition_futures)
                {
                    append(future.get());
                }
            }

            return values;
        }
    };

    template <typename Value, typename RangeList, typename OutIterator,
        typename IsSeq>
    struct copy_receiver
    {
        using rangelist_type = std::decay_t<RangeList>;
        using output_iterator = std::decay_t<OutIterator>;
        using is_seq = std::decay_t<IsSeq>;

        static output_iterator copy_from_range(
            rangelist_type ranges, output_iterator dest)
        {
            using output_local_traits =
                hpx::traits::segmented_local_iterator_traits<output_iterator>;

            auto raw_dest = output_local_traits::local(HPX_MOVE(dest));

            if (ranges.empty())
            {
                return output_local_traits::remote(HPX_MOVE(raw_dest));
            }

            if (ranges.size() == 1)
            {
                auto& range = ranges.front();

                hpx::id_type const source_locality = hpx::get_colocation_id(
                    hpx::launch::sync, range.partition_id);

                if (source_locality == hpx::find_here())
                {
                    using input_local_iterator =
                        std::decay_t<decltype(range.first)>;

                    using input_local_traits =
                        hpx::traits::segmented_local_iterator_traits<
                            input_local_iterator>;

                    auto raw_first =
                        input_local_traits::local(HPX_MOVE(range.first));
                    auto raw_last =
                        input_local_traits::local(HPX_MOVE(range.last));

                    auto raw_result = std::copy(raw_first, raw_last, raw_dest);

                    return output_local_traits::remote(HPX_MOVE(raw_result));
                }
            }

            auto values =
                range_collector<is_seq>::template collect_range<Value>(
                    HPX_MOVE(ranges));

            auto raw_result = std::copy(values.begin(), values.end(), raw_dest);

            return output_local_traits::remote(HPX_MOVE(raw_result));
        }
    };

    enum class collected_input : std::uint8_t
    {
        first,
        second,
        all
    };

    template <typename Value1, typename Value2, collected_input CollectedInput,
        typename RangeList1, typename RangeList2, typename OutIterator,
        typename Algo, typename ExPolicy, typename IsSeq, typename... Args>
    struct receiver
    {
        using values_type1 = std::vector<Value1>;
        using values_type2 = std::vector<Value2>;
        using buffer_iterator1 = typename values_type1::iterator;
        using buffer_iterator2 = typename values_type2::iterator;
        using collected_value_type =
            std::conditional_t<CollectedInput == collected_input::second,
                Value2, Value1>;
        using collected_values_type =
            std::conditional_t<CollectedInput == collected_input::second,
                values_type2, values_type1>;
        using output_iterator = std::decay_t<OutIterator>;
        using result_type = parallel::util::detail::algorithm_result_t<ExPolicy,
            output_iterator>;
        using is_seq = std::decay_t<IsSeq>;

        template <typename Dispatcher, typename... CallArgs>
        HPX_FORCEINLINE static result_type invoke_dispatcher(
            Algo const& algo, ExPolicy policy, CallArgs&&... args)
        {
            if constexpr (is_seq::value)
            {
                auto result = Dispatcher::sequential(
                    algo, HPX_MOVE(policy), HPX_FORWARD(CallArgs, args)...);

                if constexpr (hpx::is_async_execution_policy_v<
                                  std::decay_t<ExPolicy>>)
                {
                    return hpx::make_future<output_iterator>(HPX_MOVE(result),
                        [](auto&& complete_result) -> output_iterator {
                            return HPX_MOVE(complete_result.out);
                        });
                }
                else
                {
                    return HPX_MOVE(result.out);
                }
            }
            else
            {
                auto result = Dispatcher::parallel(
                    algo, HPX_MOVE(policy), HPX_FORWARD(CallArgs, args)...);

                if constexpr (hpx::is_async_execution_policy_v<
                                  std::decay_t<ExPolicy>>)
                {
                    return hpx::make_future<output_iterator>(HPX_MOVE(result),
                        [](auto&& complete_result) -> output_iterator {
                            return HPX_MOVE(complete_result.out);
                        });
                }
                else
                {
                    return HPX_MOVE(result.out);
                }
            }
        }

        template <typename LocIterator, typename... CallArgs>
        HPX_FORCEINLINE static result_type invoke_one(Algo const& algo,
            ExPolicy policy, collected_values_type& values,
            LocIterator local_first, LocIterator final_last, OutIterator dest,
            CallArgs&&... args)
        {
            static_assert(CollectedInput != collected_input::all,
                "invoke_one cannot be used when both inputs are collected");

            using dispatcher_type =
                std::conditional_t<CollectedInput == collected_input::second,
                    dispatcher<std::decay_t<Algo>, ExPolicy, LocIterator,
                        LocIterator, buffer_iterator2, buffer_iterator2,
                        OutIterator, std::decay_t<CallArgs>...>,
                    dispatcher<std::decay_t<Algo>, ExPolicy, buffer_iterator1,
                        buffer_iterator1, LocIterator, LocIterator, OutIterator,
                        std::decay_t<CallArgs>...>>;

            if constexpr (CollectedInput == collected_input::second)
            {
                return invoke_dispatcher<dispatcher_type>(algo,
                    HPX_MOVE(policy), local_first, final_last, values.begin(),
                    values.end(), dest, HPX_FORWARD(CallArgs, args)...);
            }
            else
            {
                return invoke_dispatcher<dispatcher_type>(algo,
                    HPX_MOVE(policy), values.begin(), values.end(), local_first,
                    final_last, dest, HPX_FORWARD(CallArgs, args)...);
            }
        }

        HPX_FORCEINLINE static result_type getfrom_one(Algo const& algo,
            ExPolicy policy, RangeList1 ranges, RangeList2 local_first,
            RangeList2 final_last, OutIterator dest, Args... args)
        {
            collected_values_type values =
                range_collector<is_seq>::template collect_range<
                    collected_value_type>(HPX_MOVE(ranges));

            if constexpr (hpx::is_async_execution_policy_v<
                              std::decay_t<ExPolicy>>)
            {
                auto shared_values =
                    std::make_shared<collected_values_type>(HPX_MOVE(values));
                auto f = invoke_one(algo, HPX_MOVE(policy), *shared_values,
                    HPX_MOVE(local_first), HPX_MOVE(final_last), HPX_MOVE(dest),
                    HPX_MOVE(args)...);
                return f.then([shared_values](
                                  auto ready) mutable { return ready.get(); });
            }
            else
            {
                return invoke_one(algo, HPX_MOVE(policy), values,
                    HPX_MOVE(local_first), HPX_MOVE(final_last), HPX_MOVE(dest),
                    HPX_MOVE(args)...);
            }
        }

        HPX_FORCEINLINE static result_type getfrom_two(Algo const& algo,
            ExPolicy policy, RangeList1 ranges1, RangeList2 ranges2,
            OutIterator dest, Args... args)
        {
            using dispatcher_type = dispatcher<std::decay_t<Algo>, ExPolicy,
                buffer_iterator1, buffer_iterator1, buffer_iterator2,
                buffer_iterator2, OutIterator, Args...>;

            static constexpr bool is_task_policy =
                hpx::is_async_execution_policy_v<std::decay_t<ExPolicy>>;

            values_type1 values1;
            values_type2 values2;

            if constexpr (is_seq::value)
            {
                values1 =
                    range_collector<is_seq>::template collect_range<Value1>(
                        HPX_MOVE(ranges1));

                values2 =
                    range_collector<is_seq>::template collect_range<Value2>(
                        HPX_MOVE(ranges2));
            }
            else
            {
                auto values1_f = hpx::async(
                    [ranges1 = HPX_MOVE(ranges1)]() mutable -> values_type1 {
                        return range_collector<is_seq>::template collect_range<
                            Value1>(HPX_MOVE(ranges1));
                    });

                auto values2_f = hpx::async(
                    [ranges2 = HPX_MOVE(ranges2)]() mutable -> values_type2 {
                        return range_collector<is_seq>::template collect_range<
                            Value2>(HPX_MOVE(ranges2));
                    });

                if constexpr (is_task_policy)
                {
                    using algo_type = std::decay_t<Algo>;

                    return hpx::dataflow(
                        [algorithm = algo_type(algo), policy = HPX_MOVE(policy),
                            dest = HPX_MOVE(dest),
                            ... stored_args = HPX_MOVE(args)](
                            hpx::future<values_type1> ready_values1,
                            hpx::future<values_type2> ready_values2) mutable
                            -> result_type {
                            auto shared_values1 =
                                std::make_shared<values_type1>(
                                    ready_values1.get());

                            auto shared_values2 =
                                std::make_shared<values_type2>(
                                    ready_values2.get());

                            auto operation = invoke_dispatcher<dispatcher_type>(
                                algorithm, HPX_MOVE(policy),
                                shared_values1->begin(), shared_values1->end(),
                                shared_values2->begin(), shared_values2->end(),
                                HPX_MOVE(dest), HPX_MOVE(stored_args)...);

                            return operation.then(
                                [shared_values1, shared_values2](
                                    auto ready) mutable -> output_iterator {
                                    return ready.get();
                                });
                        },
                        HPX_MOVE(values1_f), HPX_MOVE(values2_f));
                }
                else
                {
                    values1 = values1_f.get();
                    values2 = values2_f.get();
                }
            }

            if constexpr (is_task_policy)
            {
                auto shared_values1 =
                    std::make_shared<values_type1>(HPX_MOVE(values1));

                auto shared_values2 =
                    std::make_shared<values_type2>(HPX_MOVE(values2));

                auto operation = invoke_dispatcher<dispatcher_type>(algo,
                    HPX_MOVE(policy), shared_values1->begin(),
                    shared_values1->end(), shared_values2->begin(),
                    shared_values2->end(), HPX_MOVE(dest), HPX_MOVE(args)...);

                return operation.then(
                    [shared_values1, shared_values2](
                        auto ready) mutable -> output_iterator {
                        return ready.get();
                    });
            }
            else
            {
                return invoke_dispatcher<dispatcher_type>(algo,
                    HPX_MOVE(policy), values1.begin(), values1.end(),
                    values2.begin(), values2.end(), HPX_MOVE(dest),
                    HPX_MOVE(args)...);
            }
        }
    };

    // input ranges receiver
    template <typename Value1, typename Value2, collected_input CollectedInput,
        typename RangeList, typename LocIterator, typename OutIterator,
        typename Algo, typename R, typename ExPolicy, typename IsSeq,
        typename... Args>
    struct get_values_from_range_action
      : hpx::actions::make_action<R (*)(Algo const&, ExPolicy, RangeList,
                                      LocIterator, LocIterator, OutIterator,
                                      Args...),
            &receiver<Value1, Value2, CollectedInput, RangeList, LocIterator,
                OutIterator, Algo, ExPolicy, IsSeq, Args...>::getfrom_one,
            get_values_from_range_action<Value1, Value2, CollectedInput,
                RangeList, LocIterator, OutIterator, Algo, R, ExPolicy, IsSeq,
                Args...>>::type
    {
    };

    template <typename Value1, typename Value2, typename RangeList1,
        typename RangeList2, typename OutIterator, typename Algo, typename R,
        typename ExPolicy, typename IsSeq, typename... Args>
    struct get_values_from_ranges_action
      : hpx::actions::make_action<R (*)(Algo const&, ExPolicy, RangeList1,
                                      RangeList2, OutIterator, Args...),
            &receiver<Value1, Value2, collected_input::all, RangeList1,
                RangeList2, OutIterator, Algo, ExPolicy, IsSeq,
                Args...>::getfrom_two,
            get_values_from_ranges_action<Value1, Value2, RangeList1,
                RangeList2, OutIterator, Algo, R, ExPolicy, IsSeq,
                Args...>>::type
    {
    };

    template <typename Value, typename RangeList, typename OutIterator,
        typename IsSeq>
    struct copy_values_from_range_action
      : hpx::actions::make_action<std::decay_t<OutIterator> (*)(
                                      std::decay_t<RangeList>,
                                      std::decay_t<OutIterator>),
            &copy_receiver<Value, std::decay_t<RangeList>,
                std::decay_t<OutIterator>,
                std::decay_t<IsSeq>>::copy_from_range,
            copy_values_from_range_action<Value, std::decay_t<RangeList>,
                std::decay_t<OutIterator>, std::decay_t<IsSeq>>>::type
    {
    };

    HPX_CXX_EXPORT template <typename Value1, typename Value2,
        collected_input CollectedInput, typename RangeList,
        typename LocIterator, typename OutIterator, typename Algo,
        typename ExPolicy, typename IsSeq, typename... Args>
    HPX_FORCEINLINE hpx::future<std::decay_t<OutIterator>>
    capture_dispatch_async_impl(hpx::id_type const& id_recv, Algo&& algo,
        ExPolicy policy, RangeList ranges, LocIterator local_first,
        LocIterator final_last, OutIterator dest, IsSeq, Args&&... args)
    {
        using algo_type = std::decay_t<Algo>;
        using result_type = parallel::util::detail::algorithm_result_t<ExPolicy,
            std::decay_t<OutIterator>>;
        using local_iterator_type = std::decay_t<LocIterator>;
        using rangelist_type = std::decay_t<RangeList>;
        using output_iterator = std::decay_t<OutIterator>;

        get_values_from_range_action<Value1, Value2, CollectedInput,
            rangelist_type, local_iterator_type, output_iterator, algo_type,
            result_type, ExPolicy, std::decay_t<IsSeq>,
            hpx::util::decay_unwrap_t<Args>...>
            act;

        auto operation = hpx::async(act, hpx::colocated(id_recv),
            HPX_FORWARD(Algo, algo), HPX_MOVE(policy), HPX_MOVE(ranges),
            HPX_MOVE(local_first), HPX_MOVE(final_last), HPX_MOVE(dest),
            HPX_FORWARD(Args, args)...);

        return handle_capture_exceptions<ExPolicy>(HPX_MOVE(operation));
    }

    HPX_CXX_EXPORT template <typename Value1, typename Value2,
        typename RangeList1, typename RangeList2, typename OutIterator,
        typename Algo, typename ExPolicy, typename IsSeq, typename... Args>
    HPX_FORCEINLINE hpx::future<std::decay_t<OutIterator>>
    capture_dispatch_async_impl(hpx::id_type const& id_recv, Algo&& algo,
        ExPolicy policy, RangeList1 ranges1, RangeList2 ranges2,
        OutIterator dest, IsSeq, Args&&... args)
    {
        using algo_type = std::decay_t<Algo>;
        using result_type = parallel::util::detail::algorithm_result_t<ExPolicy,
            std::decay_t<OutIterator>>;
        using rangelist1_type = std::decay_t<RangeList1>;
        using rangelist2_type = std::decay_t<RangeList2>;
        using output_iterator = std::decay_t<OutIterator>;

        get_values_from_ranges_action<Value1, Value2, rangelist1_type,
            rangelist2_type, output_iterator, algo_type, result_type, ExPolicy,
            std::decay_t<IsSeq>, hpx::util::decay_unwrap_t<Args>...>
            act;

        auto operation = hpx::async(act, hpx::colocated(id_recv),
            HPX_FORWARD(Algo, algo), HPX_MOVE(policy), HPX_MOVE(ranges1),
            HPX_MOVE(ranges2), HPX_MOVE(dest), HPX_FORWARD(Args, args)...);

        return handle_capture_exceptions<ExPolicy>(HPX_MOVE(operation));
    }

    template <typename TraitsDest, typename SegIteratorOut, typename Iterator1,
        typename Iterator2, typename OutIterator, typename Algo,
        typename ExPolicy, typename IsSeq, typename... Args>
    HPX_FORCEINLINE hpx::future<std::decay_t<OutIterator>>
    capture_dispatch_async(TraitsDest, Algo&& algo, ExPolicy policy, IsSeq,
        SegIteratorOut&& sdest, Iterator1 first1, Iterator1 last1,
        Iterator2 first2, Iterator2 last2, OutIterator dest, Args&&... args)
    {
        using Value1 =
            typename std::iterator_traits<std::decay_t<Iterator1>>::value_type;
        using Value2 =
            typename std::iterator_traits<std::decay_t<Iterator2>>::value_type;
        using is_seq = std::decay_t<IsSeq>;

        auto ranges1 = make_partition_ranges(first1, last1);
        auto ranges2 = make_partition_ranges(first2, last2);

        return capture_dispatch_async_impl<Value1, Value2>(
            TraitsDest::get_id(sdest), HPX_FORWARD(Algo, algo),
            HPX_MOVE(policy), HPX_MOVE(ranges1), HPX_MOVE(ranges2),
            HPX_MOVE(dest), is_seq{}, HPX_FORWARD(Args, args)...);
    }

    HPX_CXX_EXPORT template <typename traits_dest, typename SegIterator_out,
        typename Iterator1, typename Iterator2, typename OutIterator,
        typename Algo, typename ExPolicy, typename IsSeq, typename... Args>
    HPX_FORCEINLINE std::decay_t<OutIterator> capture_dispatch(traits_dest,
        Algo&& algo, ExPolicy policy, IsSeq, SegIterator_out&& sdest,
        Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2,
        OutIterator dest, Args&&... args)
    {
        using is_seq = std::decay_t<IsSeq>;
        using Value1 = typename std::iterator_traits<Iterator1>::value_type;
        using Value2 = typename std::iterator_traits<Iterator2>::value_type;

        using traits_in1 =
            hpx::traits::segmented_iterator_traits<std::decay_t<Iterator1>>;
        using traits_in2 =
            hpx::traits::segmented_iterator_traits<std::decay_t<Iterator2>>;

        using output_iterator = std::decay_t<OutIterator>;
        using capture_result_type = typename traits_dest::local_iterator;

        using algo_result_type = typename std::decay_t<Algo>::result_type;

        static_assert(
            std::is_same_v<output_iterator, std::decay_t<capture_result_type>>,
            "OutIterator must be the destination segment-local iterator");

        using algo_output_iterator =
            std::decay_t<decltype(std::declval<algo_result_type>().out)>;

        static_assert(std::is_same_v<output_iterator, algo_output_iterator>,
            "Algo::result_type::out must match OutIterator");

        auto seg_first1 = traits_in1::segment(first1);
        auto seg_last1 = (first1 == last1) ?
            traits_in1::segment(first1) :
            (traits_in1::segment(std::prev(last1)));
        auto seg_first2 = traits_in2::segment(first2);
        auto seg_last2 = (first2 == last2) ?
            traits_in2::segment(first2) :
            (traits_in2::segment(std::prev(last2)));

        HPX_ASSERT(first1 != last1);
        HPX_ASSERT(first2 != last2);

        auto local_first1 = traits_in1::local(first1);
        auto final_last1 = traits_in1::local(std::prev(last1));
        ++final_last1;

        auto local_first2 = traits_in2::local(first2);
        auto final_last2 = traits_in2::local(std::prev(last2));
        ++final_last2;

        auto dest_id = hpx::get_colocation_id(
            hpx::launch::sync, traits_dest::get_id(sdest));
        auto sit1_id = hpx::get_colocation_id(
            hpx::launch::sync, traits_in1::get_id(seg_first1));
        auto sit2_id = hpx::get_colocation_id(
            hpx::launch::sync, traits_in2::get_id(seg_first2));

        bool in1_local = dest_id == sit1_id && seg_first1 == seg_last1;
        bool in2_local = dest_id == sit2_id && seg_first2 == seg_last2;

        if (in1_local && in2_local)
        {
            // both dest and input ranges are on the same segments
            auto local_result = dispatch(traits_dest::get_id(sdest),
                HPX_FORWARD(Algo, algo), HPX_MOVE(policy), is_seq{},
                local_first1, final_last1, local_first2, final_last2,
                HPX_MOVE(dest), HPX_FORWARD(Args, args)...);
            return HPX_MOVE(local_result.out);
        }
        else if (in1_local)
        {
            // input1 range and dest range are on the same segment
            auto ranges2 = make_partition_ranges(first2, last2);

            return capture_dispatch_async_impl<Value1, Value2,
                collected_input::second>(traits_dest::get_id(sdest),
                HPX_FORWARD(Algo, algo), HPX_MOVE(policy), HPX_MOVE(ranges2),
                HPX_MOVE(local_first1), HPX_MOVE(final_last1), HPX_MOVE(dest),
                is_seq{}, HPX_FORWARD(Args, args)...)
                .get();
        }
        else if (in2_local)
        {
            // input2 range and dest range are on the same segment
            auto ranges1 = make_partition_ranges(first1, last1);

            return capture_dispatch_async_impl<Value1, Value2,
                collected_input::first>(traits_dest::get_id(sdest),
                HPX_FORWARD(Algo, algo), HPX_MOVE(policy), HPX_MOVE(ranges1),
                HPX_MOVE(local_first2), HPX_MOVE(final_last2), HPX_MOVE(dest),
                is_seq{}, HPX_FORWARD(Args, args)...)
                .get();
        }
        else
        {
            // dest and input ranges are not on the same segments
            auto ranges1 = make_partition_ranges(first1, last1);
            auto ranges2 = make_partition_ranges(first2, last2);

            return capture_dispatch_async_impl<Value1, Value2>(
                traits_dest::get_id(sdest), HPX_FORWARD(Algo, algo),
                HPX_MOVE(policy), HPX_MOVE(ranges1), HPX_MOVE(ranges2),
                HPX_MOVE(dest), is_seq{}, HPX_FORWARD(Args, args)...)
                .get();
        }
    }

    template <typename Value, typename TraitsDest, typename SegIteratorOut,
        typename Iterator, typename OutIterator, typename ExPolicy,
        typename IsSeq>
    HPX_FORCEINLINE hpx::future<std::decay_t<OutIterator>> capture_copy_async(
        TraitsDest, ExPolicy const&, IsSeq, SegIteratorOut const& seg_dest,
        Iterator first, Iterator last, OutIterator dest)
    {
        using output_iterator = std::decay_t<OutIterator>;
        using is_seq = std::decay_t<IsSeq>;

        if (first == last)
        {
            return hpx::make_ready_future(output_iterator(HPX_MOVE(dest)));
        }

        auto ranges = make_partition_ranges(HPX_MOVE(first), HPX_MOVE(last));

        using rangelist_type = std::decay_t<decltype(ranges)>;

        hpx::id_type const dest_id = TraitsDest::get_id(seg_dest);

        copy_values_from_range_action<Value, rangelist_type, output_iterator,
            is_seq>
            act;

        auto operation = hpx::async(
            act, hpx::colocated(dest_id), HPX_MOVE(ranges), HPX_MOVE(dest));

        return handle_capture_exceptions<ExPolicy>(HPX_MOVE(operation));
    }

    HPX_CXX_EXPORT template <typename Value, typename TraitsDest,
        typename SegIteratorOut, typename Iterator, typename OutIterator,
        typename ExPolicy, typename IsSeq>
    HPX_FORCEINLINE std::decay_t<OutIterator> capture_copy(TraitsDest,
        ExPolicy const& policy, IsSeq, SegIteratorOut const& seg_dest,
        Iterator first, Iterator last, OutIterator dest)
    {
        using output_iterator = std::decay_t<OutIterator>;

        if (first == last)
        {
            return output_iterator(HPX_MOVE(dest));
        }

        return capture_copy_async<Value>(TraitsDest{}, policy, IsSeq{},
            HPX_MOVE(seg_dest), HPX_MOVE(first), HPX_MOVE(last), HPX_MOVE(dest))
            .get();
    }

}    // namespace hpx::parallel::detail