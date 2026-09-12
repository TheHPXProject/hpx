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

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////////////////
    // segmented merge

    /// \cond NOINTERNAL

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

    template <bool Async, typename Traits3, typename Algo, typename ExPolicy,
        typename IsSeq, typename Iter1, typename Iter2, typename SegIteratorOut,
        typename OutLocIterator, typename Comp, typename Proj1, typename Proj2>
    HPX_FORCEINLINE auto process_chunk(Traits3, Algo&& algo, ExPolicy& policy,
        IsSeq, Iter1 const& first1, std::size_t len1, Iter2 const& first2,
        std::size_t len2, SegIteratorOut seg_out, OutLocIterator out_first,
        std::size_t k0, std::size_t k1, Comp& comparator, Proj1& projection1,
        Proj2& projection2) -> std::conditional_t<Async,
        hpx::future<OutLocIterator>, OutLocIterator>
    {
        using value_type1 = typename std::iterator_traits<Iter1>::value_type;
        using value_type2 = typename std::iterator_traits<Iter2>::value_type;

        auto [a0, b0] = segmented_diagonal_intersection(first1, len1, first2,
            len2, k0, comparator, projection1, projection2);
        auto [a1, b1] = segmented_diagonal_intersection(first1, len1, first2,
            len2, k1, comparator, projection1, projection2);

        Iter1 chunk_first1 = std::next(first1, a0);
        Iter1 chunk_last1 = std::next(first1, a1);
        Iter2 chunk_first2 = std::next(first2, b0);
        Iter2 chunk_last2 = std::next(first2, b1);

        if (chunk_first1 == chunk_last1)
        {
            if constexpr (Async)
            {
                return capture_copy_async<value_type2>(Traits3{}, policy,
                    IsSeq{}, seg_out, chunk_first2, chunk_last2,
                    HPX_MOVE(out_first));
            }
            else
            {
                return capture_copy<value_type2>(Traits3{}, policy, IsSeq{},
                    seg_out, chunk_first2, chunk_last2, HPX_MOVE(out_first));
            }
        }

        if (chunk_first2 == chunk_last2)
        {
            if constexpr (Async)
            {
                return capture_copy_async<value_type1>(Traits3{}, policy,
                    IsSeq{}, seg_out, chunk_first1, chunk_last1,
                    HPX_MOVE(out_first));
            }
            else
            {
                return capture_copy<value_type1>(Traits3{}, policy, IsSeq{},
                    seg_out, chunk_first1, chunk_last1, HPX_MOVE(out_first));
            }
        }

        if constexpr (Async)
        {
            return capture_dispatch_async(Traits3{}, algo, policy, IsSeq{},
                seg_out, chunk_first1, chunk_last1, chunk_first2, chunk_last2,
                HPX_MOVE(out_first), comparator, projection1, projection2);
        }
        else
        {
            return capture_dispatch(Traits3{}, algo, policy, IsSeq{}, seg_out,
                chunk_first1, chunk_last1, chunk_first2, chunk_last2,
                HPX_MOVE(out_first), comparator, projection1, projection2);
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Sequential remote implementation

    template <typename Algo, typename ExPolicy, typename Iter1, typename Iter2,
        typename Iter3, typename Comp, typename Proj1, typename Proj2>
    HPX_FORCEINLINE util::detail::algorithm_result_t<ExPolicy,
        util::in_in_out_result<Iter1, Iter2, Iter3>>
    segmented_merge_sequential(Algo&& algo, ExPolicy policy, Iter1 first1,
        Iter1 last1, Iter2 first2, Iter2 last2, Iter3 dest, Comp&& comp,
        Proj1&& proj1, Proj2&& proj2, std::size_t len1, std::size_t len2)
    {
        using traits3 = hpx::traits::segmented_iterator_traits<Iter3>;
        using segment_iterator_out = typename traits3::segment_iterator;
        using out_local_iterator_type = typename traits3::local_iterator;
        using policy_type = std::decay_t<ExPolicy>;
        using result_value_type = util::in_in_out_result<Iter1, Iter2, Iter3>;
        using result =
            util::detail::algorithm_result<ExPolicy, result_value_type>;

        static constexpr bool is_task_policy =
            hpx::is_async_execution_policy_v<policy_type>;
        std::size_t const total_size = len1 + len2;

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
                Iter1 first1;
                std::size_t len1;
                Iter2 first2;
                std::size_t len2;
            };

            auto state =
                std::make_shared<task_state>(task_state{HPX_FORWARD(Algo, algo),
                    HPX_MOVE(policy), HPX_FORWARD(Comp, comp),
                    HPX_FORWARD(Proj1, proj1), HPX_FORWARD(Proj2, proj2),
                    HPX_MOVE(first1), len1, HPX_MOVE(first2), len2});

            hpx::future<out_local_iterator_type> sequential_operation;
            bool has_sequential_operation = false;

            auto output_position =
                for_each_output_chunk(traits3{}, dest, total_size,
                    [&](segment_iterator_out seg_out,
                        out_local_iterator_type out_first, std::size_t k0,
                        std::size_t k1) {
                        auto chunk_operation = [state, seg_out, out_first, k0,
                                                   k1]() mutable {
                            return process_chunk<true>(traits3{},
                                state->algorithm, state->execution_policy,
                                std::true_type{}, state->first1, state->len1,
                                state->first2, state->len2, seg_out, out_first,
                                k0, k1, state->comparator, state->projection1,
                                state->projection2);
                        };

                        if (!has_sequential_operation)
                        {
                            sequential_operation = chunk_operation();
                            has_sequential_operation = true;
                        }
                        else
                        {
                            sequential_operation = sequential_operation.then(
                                [chunk_operation = HPX_MOVE(chunk_operation)](
                                    auto previous) mutable
                                    -> hpx::future<out_local_iterator_type> {
                                    previous.get();
                                    return chunk_operation();
                                });
                        }
                    });

            HPX_ASSERT(has_sequential_operation);

            auto end_dest_future = sequential_operation.then(
                [final_segment = HPX_MOVE(output_position.first)](
                    auto ready) mutable -> Iter3 {
                    auto final_local = ready.get();
                    return traits3::compose(
                        final_segment, HPX_MOVE(final_local));
                });

            return end_dest_future.then(
                [last1 = HPX_MOVE(last1), last2 = HPX_MOVE(last2)](
                    auto ready) mutable -> result_value_type {
                    return result_value_type{
                        HPX_MOVE(last1), HPX_MOVE(last2), ready.get()};
                });
        }
        else
        {
            auto output_position = for_each_output_chunk(traits3{}, dest,
                total_size,
                [&](segment_iterator_out seg_out,
                    out_local_iterator_type& out_first, std::size_t k0,
                    std::size_t k1) {
                    out_first = process_chunk<false>(traits3{}, algo, policy,
                        std::true_type{}, first1, len1, first2, len2, seg_out,
                        out_first, k0, k1, comp, proj1, proj2);
                });

            Iter3 end_dest = traits3::compose(
                output_position.first, HPX_MOVE(output_position.second));

            return result::get(result_value_type{
                HPX_MOVE(last1), HPX_MOVE(last2), HPX_MOVE(end_dest)});
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Parallel remote implementation

    template <typename Algo, typename ExPolicy, typename Iter1, typename Iter2,
        typename Iter3, typename Comp, typename Proj1, typename Proj2>
    HPX_FORCEINLINE util::detail::algorithm_result_t<ExPolicy,
        util::in_in_out_result<Iter1, Iter2, Iter3>>
    segmented_merge_parallel(Algo&& algo, ExPolicy policy, Iter1 first1,
        Iter1 last1, Iter2 first2, Iter2 last2, Iter3 dest, Comp&& comp,
        Proj1&& proj1, Proj2&& proj2, std::size_t len1, std::size_t len2)
    {
        using traits3 = hpx::traits::segmented_iterator_traits<Iter3>;
        using segment_iterator_out = typename traits3::segment_iterator;
        using out_local_iterator_type = typename traits3::local_iterator;
        using policy_type = std::decay_t<ExPolicy>;
        using result_value_type = util::in_in_out_result<Iter1, Iter2, Iter3>;

        static constexpr bool is_task_policy =
            hpx::is_async_execution_policy_v<policy_type>;

        std::size_t total_size = len1 + len2;

        std::vector<hpx::future<out_local_iterator_type>> parallel_operations;

        auto output_position = for_each_output_chunk(traits3{}, dest,
            total_size,
            [&](segment_iterator_out seg_out, out_local_iterator_type out_first,
                std::size_t k0, std::size_t k1) {
                parallel_operations.push_back(process_chunk<true>(traits3{},
                    algo, policy, std::false_type{}, first1, len1, first2, len2,
                    seg_out, out_first, k0, k1, comp, proj1, proj2));
            });

        HPX_ASSERT(!parallel_operations.empty());

        auto end_dest_future =
            hpx::when_all(HPX_MOVE(parallel_operations))
                .then([final_segment = HPX_MOVE(output_position.first)](
                          auto ready) mutable -> Iter3 {
                    auto operations = ready.get();

                    HPX_ASSERT(!operations.empty());

                    for (std::size_t i = 0; i + 1 != operations.size(); ++i)
                    {
                        operations[i].get();
                    }

                    auto final_local = operations.back().get();
                    return traits3::compose(
                        final_segment, HPX_MOVE(final_local));
                });

        auto operation = end_dest_future.then(
            [last1 = HPX_MOVE(last1), last2 = HPX_MOVE(last2)](
                auto ready) mutable -> result_value_type {
                return result_value_type{
                    HPX_MOVE(last1), HPX_MOVE(last2), ready.get()};
            });

        if constexpr (is_task_policy)
        {
            return operation;
        }
        else
        {
            return operation.get();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////

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

    HPX_CXX_EXPORT template <typename ExPolicy, typename Iter1, typename Iter2,
        typename Iter3, typename Comp = hpx::parallel::detail::less>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<Iter1> &&
            hpx::traits::is_segmented_iterator_v<Iter1> &&
            hpx::traits::is_iterator_v<Iter2> &&
            hpx::traits::is_segmented_iterator_v<Iter2> &&
            hpx::traits::is_iterator_v<Iter3> &&
            hpx::traits::is_segmented_iterator_v<Iter3>)
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, Iter3> hpx_invoke(
        hpx::merge_t, ExPolicy&& policy, Iter1 first1, Iter1 last1,
        Iter2 first2, Iter2 last2, Iter3 dest, Comp comp = Comp())
    {
        static_assert(std::random_access_iterator<Iter1>,
            "Requires a random-access iterator");
        static_assert(std::random_access_iterator<Iter2>,
            "Requires a random-access iterator");
        static_assert(std::random_access_iterator<Iter3>,
            "Requires a random-access iterator");

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
        requires(hpx::traits::is_iterator_v<Iter1> &&
            hpx::traits::is_segmented_iterator_v<Iter1> &&
            hpx::traits::is_iterator_v<Iter2> &&
            hpx::traits::is_segmented_iterator_v<Iter2> &&
            hpx::traits::is_iterator_v<Iter3> &&
            hpx::traits::is_segmented_iterator_v<Iter3>)
    Iter3 hpx_invoke(hpx::merge_t, Iter1 first1, Iter1 last1, Iter2 first2,
        Iter2 last2, Iter3 dest, Comp comp = Comp())
    {
        static_assert(std::random_access_iterator<Iter1>,
            "Requires a random-access iterator");
        static_assert(std::random_access_iterator<Iter2>,
            "Requires a random-access iterator");
        static_assert(std::random_access_iterator<Iter3>,
            "Requires a random-access iterator");

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
            hpx::traits::is_iterator_v<Iter1> &&
            hpx::traits::is_segmented_iterator_v<Iter1> &&
            hpx::traits::is_iterator_v<Iter2> &&
            hpx::traits::is_segmented_iterator_v<Iter2> &&
            hpx::traits::is_iterator_v<Iter3> &&
            hpx::traits::is_segmented_iterator_v<Iter3>)
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy,
        hpx::ranges::merge_result<Iter1, Iter2, Iter3>>
    hpx_invoke(hpx::ranges::merge_t, ExPolicy&& policy, Iter1 first1,
        Iter1 last1, Iter2 first2, Iter2 last2, Iter3 dest, Comp comp = Comp(),
        Proj1 proj1 = Proj1(), Proj2 proj2 = Proj2())
    {
        static_assert(std::random_access_iterator<Iter1>,
            "Requires a random-access iterator.");
        static_assert(std::random_access_iterator<Iter2>,
            "Requires a random-access iterator.");
        static_assert(std::random_access_iterator<Iter3>,
            "Requires a random-access iterator.");

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
        requires(hpx::traits::is_iterator_v<Iter1> &&
            hpx::traits::is_segmented_iterator_v<Iter1> &&
            hpx::traits::is_iterator_v<Iter2> &&
            hpx::traits::is_segmented_iterator_v<Iter2> &&
            hpx::traits::is_iterator_v<Iter3> &&
            hpx::traits::is_segmented_iterator_v<Iter3>)
    hpx::ranges::merge_result<Iter1, Iter2, Iter3> hpx_invoke(
        hpx::ranges::merge_t, Iter1 first1, Iter1 last1, Iter2 first2,
        Iter2 last2, Iter3 dest, Comp comp = Comp(), Proj1 proj1 = Proj1(),
        Proj2 proj2 = Proj2())
    {
        static_assert(std::random_access_iterator<Iter1>,
            "Requires a random-access iterator.");
        static_assert(std::random_access_iterator<Iter2>,
            "Requires a random-access iterator.");
        static_assert(std::random_access_iterator<Iter3>,
            "Requires a random-access iterator.");

        using traits3 = hpx::traits::segmented_iterator_traits<Iter3>;
        using output_local_iterator = typename traits3::local_iterator;

        return hpx::parallel::detail::segmented_merge(
            hpx::parallel::detail::captured_merge<output_local_iterator>{},
            hpx::execution::seq, first1, last1, first2, last2, dest,
            HPX_MOVE(comp), HPX_MOVE(proj1), HPX_MOVE(proj2));
    }

}    // namespace hpx::segmented