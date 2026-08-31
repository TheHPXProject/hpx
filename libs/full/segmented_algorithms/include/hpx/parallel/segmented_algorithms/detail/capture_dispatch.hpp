//  Copyright (c) 2026 Bharath Kollanur
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <hpx/assert.hpp>
#include <hpx/async_distributed/async.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_colocated.hpp>
#include <hpx/modules/distribution_policies.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/segmented_algorithms/detail/dispatch.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/serialization.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////

    template <typename Value, typename Iterator>
    struct transmitter
    {
        using iterator_type = std::decay_t<Iterator>;
        using result_type = std::vector<Value>;

        static result_type send_values(
            iterator_type first, iterator_type last)
        {
            using iterator_traits =
                hpx::traits::segmented_local_iterator_traits<
                    iterator_type>;

            auto raw_first =
                iterator_traits::local(HPX_MOVE(first));

            auto raw_last =
                iterator_traits::local(HPX_MOVE(last));

            result_type values;

            values.insert(values.end(), raw_first, raw_last);

            return values;
        }
    };

    template <typename Value, typename Iterator>
    struct send_values_action
    : hpx::actions::make_action<
            std::vector<Value> (*)(
                std::decay_t<Iterator>,
                std::decay_t<Iterator>),
            &transmitter<Value,
                std::decay_t<Iterator>>::send_values,
            send_values_action<Value,
                std::decay_t<Iterator>>>::type
    {
    };

    template <typename Value, typename Iterator>
    hpx::future<std::vector<Value>> capture_async(
        hpx::id_type const& partition,
        Iterator first,
        Iterator last)
    {
        send_values_action<Value,
            std::decay_t<Iterator>> act;

        return hpx::async(act,
            hpx::colocated(partition),
            HPX_MOVE(first),
            HPX_MOVE(last));
    }

    enum class collected_input
    {
        first,
        second,
        all
    };

    struct capture_copy_tag
    {
    };

    template <typename Value1, typename Value2, collected_input CollectedInput, 
        typename traits1, typename traits2,
        typename LocIterator1, typename SegIterator1, 
        typename LocIterator2, typename SegIterator2, 
        typename OutIterator, typename Algo, 
        typename ExPolicy, typename is_seq,
         typename... Args>
    struct receiver
    {
        using values_type1 = std::vector<Value1>;
        using values_type2 = std::vector<Value2>;
        using buffer_iterator1 = typename values_type1::iterator;
        using buffer_iterator2 = typename values_type2::iterator;
        using collected_value_type = std::conditional_t<
            CollectedInput == collected_input::second, Value2, Value1>;
        using collected_values_type = std::conditional_t<
            CollectedInput == collected_input::second, values_type2, values_type1>;
        using output_iterator = std::decay_t<OutIterator>;
        using result_type = 
            parallel::util::detail::algorithm_result_t<ExPolicy,
            output_iterator>;
        

        template <typename Dispatcher, typename... CallArgs>
        HPX_FORCEINLINE static result_type invoke_dispatcher(
            Algo const& algo, ExPolicy policy, CallArgs&&... args)
        {
            if constexpr (is_seq::value)
            {
                auto result = Dispatcher::sequential(
                    algo, HPX_MOVE(policy),
                    HPX_FORWARD(CallArgs, args)...);
                
                if constexpr (hpx::is_async_execution_policy_v<
                                std::decay_t<ExPolicy>>)
                {
                    return hpx::make_future<output_iterator>(
                        HPX_MOVE(result), 
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
                    algo, HPX_MOVE(policy),
                    HPX_FORWARD(CallArgs, args)...);  

                if constexpr (hpx::is_async_execution_policy_v<
                                std::decay_t<ExPolicy>>)
                {
                    return hpx::make_future<output_iterator>(
                        HPX_MOVE(result), 
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
        HPX_FORCEINLINE static result_type invoke_one(
            Algo const& algo, ExPolicy policy, collected_values_type& values,
            LocIterator local_first, LocIterator final_last, 
            OutIterator dest, CallArgs&&... args)
        {
            static_assert(CollectedInput != collected_input::all,
                "invoke_one cannot be used when both inputs are collected");
            
            using dispatcher_type =
                std::conditional_t<CollectedInput == collected_input::second,
                dispatcher<std::decay_t<Algo>, ExPolicy,
                    LocIterator, LocIterator, 
                    buffer_iterator2, buffer_iterator2, 
                    OutIterator, std::decay_t<CallArgs>...>,
                dispatcher<std::decay_t<Algo>, ExPolicy, 
                    buffer_iterator1, buffer_iterator1, 
                    LocIterator, LocIterator, 
                    OutIterator, std::decay_t<CallArgs>...>>;
                    
            if constexpr (CollectedInput == collected_input::second)
            {
                return invoke_dispatcher<dispatcher_type>(
                    algo, HPX_MOVE(policy),
                    local_first, final_last, 
                    values.begin(), values.end(), 
                    dest, HPX_FORWARD(CallArgs, args)...);
            }
            else
            {
                return invoke_dispatcher<dispatcher_type>(
                    algo, HPX_MOVE(policy),
                    values.begin(), values.end(), 
                    local_first, final_last, 
                    dest, HPX_FORWARD(CallArgs, args)...);
            }
        }

        template <typename traits, typename Value, typename LocIterator,  
            typename SegIterator>
        HPX_FORCEINLINE static hpx::future<std::vector<Value>> get_partition_values(
            hpx::id_type const& dest_locality, 
            SegIterator const& seg, LocIterator local_first, LocIterator local_last)
        {
            using values_type = std::vector<Value>;
            
            hpx::id_type const partition = traits::get_id(seg);
            hpx::id_type const partition_locality = 
                hpx::get_colocation_id(partition).get();

            if (partition_locality == dest_locality)
            {
                if constexpr (is_seq::value)
                {
                    using local_traits =
                        hpx::traits::segmented_local_iterator_traits<
                            std::decay_t<LocIterator>>;

                    auto raw_first = local_traits::local(HPX_MOVE(local_first));
                    auto raw_last = local_traits::local(HPX_MOVE(local_last));

                    values_type values;
                    values.insert(values.end(), raw_first, raw_last);

                    return hpx::make_ready_future(HPX_MOVE(values));
                }
                else
                {
                    return hpx::async(
                        [local_first = HPX_MOVE(local_first),
                            local_last = HPX_MOVE(local_last)]() mutable 
                            -> std::vector<Value> {
                                using local_traits =
                                    hpx::traits::segmented_local_iterator_traits<
                                        std::decay_t<LocIterator>>;

                                auto raw_first = local_traits::local(HPX_MOVE(local_first));
                                auto raw_last = local_traits::local(HPX_MOVE(local_last));

                                values_type values;
                                values.insert(values.end(), raw_first, raw_last);

                                return values;
                            });
                }
            }
            else
            {
                return capture_async<Value>(partition, HPX_MOVE(local_first), 
                    HPX_MOVE(local_last));
            }

        }

        template <typename traits, typename Value, typename LocIterator,  
            typename SegIterator>
        HPX_FORCEINLINE static std::vector<Value> collect_range(LocIterator first, 
            LocIterator final_last, SegIterator seg_first, 
            SegIterator seg_last)
        {
            hpx::id_type const dest_locality = hpx::find_here();

            using values_type = std::vector<Value>;
            values_type values;
            std::vector<hpx::future<values_type>> partition_futures;

            auto process_partition = 
                [&](SegIterator const& segment,
                    LocIterator local_first,
                    LocIterator local_last) {
                        auto values_future = 
                            get_partition_values<traits, Value>(
                                dest_locality, segment,
                                HPX_MOVE(local_first),
                                HPX_MOVE(local_last));
                        
                        if constexpr (is_seq::value)
                        {
                            auto temp = values_future.get();
                            values.insert(values.end(), 
                                std::make_move_iterator(temp.begin()), 
                                std::make_move_iterator(temp.end()));
                        }
                        else
                        {
                            partition_futures.push_back(
                                HPX_MOVE(values_future));
                        }
                    };

            if (seg_first == seg_last)
            {
                process_partition(
                    seg_first,
                    HPX_MOVE(first), 
                    HPX_MOVE(final_last));
            }
            else
            {
                auto local_end = traits::end(seg_first);
                if (first != local_end)
                {
                    process_partition(
                        seg_first,
                        HPX_MOVE(first), 
                        HPX_MOVE(local_end));
                }

                SegIterator seg = seg_first;
                for(++seg; seg != seg_last; ++seg)
                {
                    auto part_first = traits::begin(seg);
                    auto part_last = traits::end(seg);
                    process_partition(
                        seg, 
                        HPX_MOVE(part_first), 
                        HPX_MOVE(part_last));
                }

                auto final_first = traits::begin(seg);

                if (final_first != final_last)
                {
                    process_partition(
                        seg, 
                        HPX_MOVE(final_first), 
                        HPX_MOVE(final_last));
                }
            }

            if constexpr (!is_seq::value)
            {

                hpx::wait_all(partition_futures);

                for (auto& future : partition_futures)
                {
                    values_type part = future.get();

                    values.insert(values.end(),
                        std::make_move_iterator(part.begin()),
                        std::make_move_iterator(part.end()));
                }
            }

            return values;
        }

        HPX_FORCEINLINE static result_type getfrom_one(
            Algo const& algo, ExPolicy policy, LocIterator1 local_first1, 
            LocIterator1 final_last1, SegIterator1 seg_first, 
            SegIterator1 seg_last, LocIterator2 local_first2, 
            LocIterator2 final_last2, OutIterator dest, Args... args)
        {
            collected_values_type values = collect_range<traits1, collected_value_type>(
                HPX_MOVE(local_first1), 
                HPX_MOVE(final_last1), 
                HPX_MOVE(seg_first), 
                HPX_MOVE(seg_last));

            if constexpr (hpx::is_async_execution_policy_v<
                std::decay_t<ExPolicy>>)
            {
                auto shared_values =
                    std::make_shared<collected_values_type>(HPX_MOVE(values));
                auto f = invoke_one(algo, HPX_MOVE(policy), 
                        *shared_values, HPX_MOVE(local_first2),  
                        HPX_MOVE(final_last2), HPX_MOVE(dest), 
                        HPX_MOVE(args)...);
                    return f.then(
                        [shared_values](auto ready) mutable {
                            return ready.get();
                        });
                }
                else
                {
                    return invoke_one(algo, HPX_MOVE(policy), 
                        values, HPX_MOVE(local_first2), 
                        HPX_MOVE(final_last2), 
                        HPX_MOVE(dest), HPX_MOVE(args)...);
                }
        }

        HPX_FORCEINLINE static result_type getfrom_two(
            Algo const& algo, ExPolicy policy,
            LocIterator1 local_first1,
            LocIterator1 final_last1,
            SegIterator1 seg_first1,
            SegIterator1 seg_last1,
            LocIterator2 local_first2,
            LocIterator2 final_last2,
            SegIterator2 seg_first2,
            SegIterator2 seg_last2,
            OutIterator dest,
            Args... args)
        {
            using dispatcher_type =
                dispatcher<std::decay_t<Algo>, ExPolicy,
                    buffer_iterator1, buffer_iterator1,
                    buffer_iterator2, buffer_iterator2,
                    OutIterator, Args...>;

            static constexpr bool is_task_policy =
                hpx::is_async_execution_policy_v<
                    std::decay_t<ExPolicy>>;

            values_type1 values1;
            values_type2 values2;

            if constexpr (is_seq::value)
            {
                values1 = collect_range<traits1, Value1>(
                    HPX_MOVE(local_first1),
                    HPX_MOVE(final_last1),
                    HPX_MOVE(seg_first1),
                    HPX_MOVE(seg_last1));

                values2 = collect_range<traits2, Value2>(
                    HPX_MOVE(local_first2),
                    HPX_MOVE(final_last2),
                    HPX_MOVE(seg_first2),
                    HPX_MOVE(seg_last2));
            }
            else
            {.
                auto values1_f = hpx::async(
                    [local_first1 = HPX_MOVE(local_first1),
                        final_last1 = HPX_MOVE(final_last1),
                        seg_first1 = HPX_MOVE(seg_first1),
                        seg_last1 = HPX_MOVE(seg_last1)]() mutable
                        -> values_type1 {
                        return collect_range<traits1, Value1>(
                            HPX_MOVE(local_first1),
                            HPX_MOVE(final_last1),
                            HPX_MOVE(seg_first1),
                            HPX_MOVE(seg_last1));
                    });

                auto values2_f = hpx::async(
                    [local_first2 = HPX_MOVE(local_first2),
                        final_last2 = HPX_MOVE(final_last2),
                        seg_first2 = HPX_MOVE(seg_first2),
                        seg_last2 = HPX_MOVE(seg_last2)]() mutable
                        -> values_type2 {
                        return collect_range<traits2, Value2>(
                            HPX_MOVE(local_first2),
                            HPX_MOVE(final_last2),
                            HPX_MOVE(seg_first2),
                            HPX_MOVE(seg_last2));
                    });

                if constexpr (is_task_policy)
                {
                    using algo_type = std::decay_t<Algo>;

                    return hpx::dataflow(
                        [algorithm = algo_type(algo),
                            policy = HPX_MOVE(policy),
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

                            auto operation =
                                invoke_dispatcher<dispatcher_type>(
                                    algorithm,
                                    HPX_MOVE(policy),
                                    shared_values1->begin(),
                                    shared_values1->end(),
                                    shared_values2->begin(),
                                    shared_values2->end(),
                                    HPX_MOVE(dest),
                                    HPX_MOVE(stored_args)...);

                            return operation.then(
                                [shared_values1, shared_values2](
                                    auto ready) mutable -> output_iterator {
                                    return ready.get();
                                });
                        },
                        HPX_MOVE(values1_f),
                        HPX_MOVE(values2_f));
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

                auto operation = invoke_dispatcher<dispatcher_type>(
                    algo,
                    HPX_MOVE(policy),
                    shared_values1->begin(),
                    shared_values1->end(),
                    shared_values2->begin(),
                    shared_values2->end(),
                    HPX_MOVE(dest),
                    HPX_MOVE(args)...);

                return operation.then(
                    [shared_values1, shared_values2](
                        auto ready) mutable -> output_iterator {
                        return ready.get();
                    });
            }
            else
            {
                return invoke_dispatcher<dispatcher_type>(
                    algo,
                    HPX_MOVE(policy),
                    values1.begin(),
                    values1.end(),
                    values2.begin(),
                    values2.end(),
                    HPX_MOVE(dest),
                    HPX_MOVE(args)...);
            }
        }

        HPX_FORCEINLINE static output_iterator copy_from_range(
            LocIterator1 local_first, LocIterator1 final_last, 
            SegIterator1 seg_first, SegIterator1 seg_last, 
            OutIterator dest)
        {
            using input_local_traits = 
                hpx::traits::segmented_local_iterator_traits<std::decay_t<LocIterator1>>;
            using output_local_traits = 
                hpx::traits::segmented_local_iterator_traits<output_iterator>;

            auto raw_dest = 
                output_local_traits::local(HPX_MOVE(dest));

            hpx::id_type const source_locality = 
                hpx::get_colocation_id(traits1::get_id(seg_first)).get();
            
            if (seg_first == seg_last && source_locality == hpx::find_here())
            {
                auto raw_first = 
                    input_local_traits::local(HPX_MOVE(local_first));
                auto raw_last =
                    input_local_traits::local(HPX_MOVE(final_last));
                auto raw_out = 
                    std::copy(raw_first, raw_last, raw_dest);

                return output_local_traits::remote(HPX_MOVE(raw_out));
            }

            values_type1 values = 
                collect_range<traits1, Value1>(HPX_MOVE(local_first), HPX_MOVE(final_last), 
                                        HPX_MOVE(seg_first), HPX_MOVE(seg_last));
                                    
            auto raw_out = 
                std::copy(values.begin(), values.end(), raw_dest);

            return output_local_traits::remote(HPX_MOVE(raw_out));
        }
        
    };

    // input ranges receiver
    template <typename Value1, typename Value2, collected_input CollectedInput, typename LocIterator1,   
        typename SegIterator, typename LocIterator2, typename OutIterator, 
        typename Algo, typename R, typename ExPolicy, typename traits, 
        typename is_seq, typename... Args>
    struct get_values_from_range_action
      : hpx::actions::make_action<R (*)(Algo const&, ExPolicy,  
            LocIterator1, LocIterator1, SegIterator, SegIterator, 
            LocIterator2, LocIterator2, OutIterator, Args...),
            &receiver<Value1, Value2, CollectedInput, traits, 
            traits, LocIterator1,  
            SegIterator, LocIterator2, SegIterator, OutIterator, 
            Algo, ExPolicy, is_seq, Args...>::getfrom_one,
            get_values_from_range_action<Value1, Value2,  CollectedInput, 
            LocIterator1, SegIterator, LocIterator2, OutIterator, 
            Algo, R, ExPolicy, traits, is_seq, Args...>>::type
    {
    };

    template <typename Value1, typename Value2, typename LocIterator1, 
        typename SegIterator1, typename LocIterator2, 
        typename SegIterator2, typename OutIterator, typename Algo, 
        typename R, typename ExPolicy, typename traits1, 
        typename traits2, typename is_seq, typename... Args>
    struct get_values_from_ranges_action
      : hpx::actions::make_action<R (*)(Algo const&, ExPolicy,  
            LocIterator1, LocIterator1, SegIterator1, SegIterator1, LocIterator2, 
            LocIterator2, SegIterator2, SegIterator2, OutIterator, Args...),
            &receiver<Value1, Value2, collected_input::all, 
            traits1, traits2, LocIterator1,  
            SegIterator1, LocIterator2, SegIterator2, OutIterator, 
            Algo, ExPolicy, is_seq, Args...>::getfrom_two,
            get_values_from_ranges_action<Value1, Value2, LocIterator1, SegIterator1, 
            LocIterator2, SegIterator2, OutIterator, Algo, R, ExPolicy, 
            traits1, traits2, is_seq, Args...>>::type
    {
    };

    template <typename Value, typename LocIterator, typename SegIterator, 
        typename OutIterator, typename ExPolicy, typename Traits, typename IsSeq>
    struct copy_values_from_range_action
        : hpx::actions::make_action< OutIterator (*)(LocIterator, LocIterator, 
            SegIterator, SegIterator, OutIterator),
            &receiver<Value, Value, collected_input::first, Traits, Traits, 
            LocIterator, SegIterator, LocIterator, SegIterator, OutIterator, 
            capture_copy_tag, ExPolicy, IsSeq>::copy_from_range,
            copy_values_from_range_action<Value, LocIterator, SegIterator, 
            OutIterator, ExPolicy, Traits, IsSeq>>::type
    {
    };

    HPX_CXX_EXPORT template <typename Value1, typename Value2, collected_input CollectedInput, typename LocIterator1, 
        typename SegIterator, typename LocIterator2, typename OutIterator, typename Algo, typename ExPolicy, 
        typename traits, typename is_seq, typename... Args>
    HPX_FORCEINLINE hpx::future<std::decay_t<OutIterator>> capture_dispatch_async_impl(
        traits, hpx::id_type const& id_recv, Algo&& algo, ExPolicy policy, LocIterator1 local_first1, 
        LocIterator1 final_last1, SegIterator seg_first, SegIterator seg_last, LocIterator2 local_first2, 
        LocIterator2 final_last2, OutIterator dest, is_seq, Args&&... args)
    {
        using algo_type = std::decay_t<Algo>;
        using result_type = parallel::util::detail::algorithm_result_t<ExPolicy,
            std::decay_t<OutIterator>>;
        using SegIterator_type = std::decay_t<SegIterator>;
        using LocIterator_type1 = std::decay_t<LocIterator1>;
        using LocIterator_type2 = std::decay_t<LocIterator2>;
        
        get_values_from_range_action<Value1, Value2, CollectedInput, LocIterator_type1, SegIterator_type, LocIterator_type2, 
            OutIterator, algo_type, result_type, ExPolicy, 
            traits, is_seq, hpx::util::decay_unwrap_t<Args>...> act;

        return hpx::async(act, hpx::colocated(id_recv), HPX_FORWARD(Algo, algo), 
            HPX_MOVE(policy), HPX_MOVE(local_first1), HPX_MOVE(final_last1),
            HPX_MOVE(seg_first), HPX_MOVE(seg_last), HPX_MOVE(local_first2), 
            HPX_MOVE(final_last2), HPX_MOVE(dest), HPX_FORWARD(Args, args)...);
    }

    HPX_CXX_EXPORT template <typename Value1, typename Value2, typename LocIterator1, typename SegIterator1, 
        typename LocIterator2, typename SegIterator2, typename OutIterator, typename Algo, typename ExPolicy, 
        typename traits1, typename traits2, typename is_seq, typename... Args>
    HPX_FORCEINLINE hpx::future<std::decay_t<OutIterator>> capture_dispatch_async_impl(
        traits1, traits2, hpx::id_type const& id_recv, Algo&& algo, ExPolicy policy, LocIterator1 local_first1, LocIterator1 final_last1,  
        SegIterator1 seg_first1, SegIterator1 seg_last1, LocIterator2 local_first2, LocIterator2 final_last2, SegIterator2 seg_first2, 
        SegIterator2 seg_last2, OutIterator dest, is_seq, Args&&... args)
    {
        using algo_type = std::decay_t<Algo>;
        using result_type = parallel::util::detail::algorithm_result_t<ExPolicy,
            std::decay_t<OutIterator>>;
        using SegIterator1_type = std::decay_t<SegIterator1>;
        using SegIterator2_type = std::decay_t<SegIterator2>;
        using LocIterator1_type = std::decay_t<LocIterator1>;
        using LocIterator2_type = std::decay_t<LocIterator2>;

        get_values_from_ranges_action<Value1, Value2, LocIterator1_type, SegIterator1_type, 
            LocIterator2_type, SegIterator2_type, OutIterator, algo_type, 
            result_type, ExPolicy, traits1, traits2, is_seq, 
            hpx::util::decay_unwrap_t<Args>...> act;
            
        return hpx::async(act, hpx::colocated(id_recv), HPX_FORWARD(Algo, algo), 
            HPX_MOVE(policy), HPX_MOVE(local_first1), HPX_MOVE(final_last1), HPX_MOVE(seg_first1),  
            HPX_MOVE(seg_last1), HPX_MOVE(local_first2), HPX_MOVE(final_last2), HPX_MOVE(seg_first2), 
            HPX_MOVE(seg_last2), HPX_MOVE(dest), HPX_FORWARD(Args, args)...);
    }

    template <typename TraitsDest, typename SegIteratorOut, typename Iterator1, typename Iterator2,
    typename OutIterator, typename Algo, typename ExPolicy, typename IsSeq, typename... Args>
    HPX_FORCEINLINE hpx::future<std::decay_t<OutIterator>> capture_dispatch_async(
        TraitsDest, Algo&& algo, ExPolicy policy, IsSeq, SegIteratorOut&& sdest,
        Iterator1 first1, Iterator1 last1, Iterator2 first2, Iterator2 last2,
        OutIterator dest, Args&&... args)
    {
        using traits_in1 =
            hpx::traits::segmented_iterator_traits<std::decay_t<Iterator1>>;
        using traits_in2 =
            hpx::traits::segmented_iterator_traits<std::decay_t<Iterator2>>;
        using Value1 =
            typename std::iterator_traits<std::decay_t<Iterator1>>::value_type;
        using Value2 =
            typename std::iterator_traits<std::decay_t<Iterator2>>::value_type;
        using is_seq = std::decay_t<IsSeq>;

        auto seg_first1 = traits_in1::segment(first1);
        auto seg_last1 = traits_in1::segment(last1);
        auto seg_first2 = traits_in2::segment(first2);
        auto seg_last2 = traits_in2::segment(last2);

        auto local_first1 = traits_in1::local(first1);
        auto final_last1 = traits_in1::local(last1);
        auto local_first2 = traits_in2::local(first2);
        auto final_last2 = traits_in2::local(last2);

        return capture_dispatch_async_impl<Value1, Value2>(
            traits_in1{}, traits_in2{}, TraitsDest::get_id(sdest),
            HPX_FORWARD(Algo, algo), HPX_MOVE(policy),
            HPX_MOVE(local_first1), HPX_MOVE(final_last1),
            HPX_MOVE(seg_first1), HPX_MOVE(seg_last1),
            HPX_MOVE(local_first2), HPX_MOVE(final_last2),
            HPX_MOVE(seg_first2), HPX_MOVE(seg_last2),
            HPX_MOVE(dest), is_seq{}, HPX_FORWARD(Args, args)...);
    }
    
    HPX_CXX_EXPORT template <typename traits_dest, typename SegIterator_out, typename Iterator1, 
        typename Iterator2, typename OutIterator, typename Algo, typename ExPolicy, typename IsSeq, typename... Args>
    HPX_FORCEINLINE std::decay_t<OutIterator> capture_dispatch(
        traits_dest, Algo&& algo, ExPolicy policy, IsSeq, 
        SegIterator_out&& sdest, Iterator1 first1, Iterator1 last1,
        Iterator2 first2, Iterator2 last2, OutIterator dest, Args&&... args)
    {
        using is_seq = std::decay_t<IsSeq>;
        using Value1 = 
            typename std::iterator_traits<Iterator1>::value_type;
        using Value2 = 
            typename std::iterator_traits<Iterator2>::value_type;

        using traits_in1  = 
            hpx::traits::segmented_iterator_traits<std::decay_t<Iterator1>>;
        using traits_in2  = 
            hpx::traits::segmented_iterator_traits<std::decay_t<Iterator2>>;

        using output_iterator = std::decay_t<OutIterator>;
        using capture_result_type =
            typename traits_dest::local_iterator;

        using algo_result_type =
            typename std::decay_t<Algo>::result_type;

        static_assert(std::is_same_v<
            output_iterator, std::decay_t<capture_result_type>>,
            "OutIterator must be the destination segment-local iterator");

        using algo_output_iterator = std::decay_t<
            decltype(std::declval<algo_result_type>().out)>;

        static_assert(std::is_same_v<
            output_iterator, algo_output_iterator>,
            "Algo::result_type::out must match OutIterator");
        
        auto seg_first1 = traits_in1::segment(first1);
        auto seg_last1 = traits_in1::segment(last1);
        auto seg_first2 = traits_in2::segment(first2);
        auto seg_last2 = traits_in2::segment(last2);

        auto local_first1 = traits_in1::local(first1);
        auto final_last1 = traits_in1::local(last1);
        auto local_first2 = traits_in2::local(first2);
        auto final_last2 = traits_in2::local(last2);

        auto dest_id_f = hpx::get_colocation_id(traits_dest::get_id(sdest));
        auto sit1_id_f = hpx::get_colocation_id(traits_in1::get_id(seg_first1));
        auto sit2_id_f = hpx::get_colocation_id(traits_in2::get_id(seg_first2));

        auto dest_id = dest_id_f.get();
        auto sit1_id = sit1_id_f.get();
        auto sit2_id = sit2_id_f.get();

        bool in1_local = 
            dest_id == sit1_id && seg_first1 == seg_last1;
        bool in2_local =
            dest_id == sit2_id && seg_first2 == seg_last2;
                
        if (in1_local && in2_local)
        {
            // both dest and input ranges are on the same segments
            auto local_result = dispatch(traits_dest::get_id(sdest), HPX_FORWARD(Algo, algo), HPX_MOVE(policy), 
                is_seq{}, local_first1, final_last1, local_first2, final_last2, HPX_MOVE(dest), HPX_FORWARD(Args, args)...);
            return HPX_MOVE(local_result.out);
        }
        else if (in1_local)
        {
            // input1 range and dest range are on the same segment
            return capture_dispatch_async_impl<Value1, Value2, collected_input::second>(traits_in2{}, traits_dest::get_id(sdest),
                HPX_FORWARD(Algo, algo), HPX_MOVE(policy), local_first2, final_last2, 
                seg_first2, seg_last2, local_first1, final_last1, HPX_MOVE(dest), is_seq{}, 
                HPX_FORWARD(Args, args)...).get();
        }
        else if (in2_local)
        {
            // input2 range and dest range are on the same segment
            return capture_dispatch_async_impl<Value1, Value2, collected_input::first>(traits_in1{}, traits_dest::get_id(sdest),
                HPX_FORWARD(Algo, algo), HPX_MOVE(policy), local_first1, final_last1, 
                seg_first1, seg_last1, local_first2, final_last2, HPX_MOVE(dest), is_seq{},
                HPX_FORWARD(Args, args)...).get();
        }
        else
        {
            // dest and input ranges are not on the same segments
            return capture_dispatch_async_impl<Value1, Value2>(traits_in1{}, traits_in2{},
                traits_dest::get_id(sdest), HPX_FORWARD(Algo, algo), HPX_MOVE(policy), local_first1, final_last1,
                seg_first1, seg_last1, local_first2, final_last2, seg_first2, seg_last2, HPX_MOVE(dest), is_seq{}, HPX_FORWARD(Args, args)...).get();
        }
    }

    template <typename Value, typename TraitsDest, typename SegIteratorOut, typename Iterator,
        typename OutIterator, typename ExPolicy, typename IsSeq>
    HPX_FORCEINLINE hpx::future<std::decay_t<OutIterator>> capture_copy_async(
        TraitsDest, ExPolicy const&, IsSeq, SegIteratorOut const& seg_dest,
        Iterator first, Iterator last, OutIterator dest)
    {
        using iterator_type = std::decay_t<Iterator>;
        using traits_in =
            hpx::traits::segmented_iterator_traits<iterator_type>;
        using output_iterator =
            std::decay_t<OutIterator>;
        using input_local_iterator = typename traits_in::local_iterator;
        using input_seg_iterator = typename traits_in::segment_iterator;
        using policy_type = std::decay_t<ExPolicy>;
        using is_seq = std::decay_t<IsSeq>;

        if (first == last)
        {
            return hpx::make_ready_future(
                output_iterator(HPX_MOVE(dest)));
        }

        input_seg_iterator seg_first = traits_in::segment(first);
        input_seg_iterator seg_last = traits_in::segment(last);
        input_local_iterator local_first = traits_in::local(first);
        input_local_iterator final_last = traits_in::local(last);

        hpx::id_type const dest_id =
            TraitsDest::get_id(seg_dest);

        copy_values_from_range_action<Value, input_local_iterator, input_seg_iterator, output_iterator, 
            policy_type, traits_in, is_seq> act;

        return hpx::async(act, hpx::colocated(dest_id), HPX_MOVE(local_first), HPX_MOVE(final_last), 
            HPX_MOVE(seg_first), HPX_MOVE(seg_last), HPX_MOVE(dest));
    }

    HPX_CXX_EXPORT template <typename Value, typename TraitsDest, typename SegIteratorOut, typename Iterator, 
        typename OutIterator, typename ExPolicy, typename IsSeq>
    HPX_FORCEINLINE std::decay_t<OutIterator> capture_copy(TraitsDest, ExPolicy const& policy, 
        IsSeq, SegIteratorOut const& seg_dest, Iterator first, Iterator last, OutIterator dest)
    {   
        if (first == last)
        {
            return dest;
        }

        return capture_copy_async<Value>(TraitsDest{}, policy, IsSeq{}, 
            HPX_MOVE(seg_dest), HPX_MOVE(first), HPX_MOVE(last),
            HPX_MOVE(dest)).get();
    }

} // namespace hpx::parallel::detail