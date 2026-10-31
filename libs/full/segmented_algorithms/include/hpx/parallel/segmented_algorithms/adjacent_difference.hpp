//  Copyright (c) 2017 Ajai V George
//  Copyright (c) 2021 Karame M.Shokooh
//  Copyright (c) 2024-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>

#include <hpx/parallel/segmented_algorithms/detail/capture_dispatch.hpp>
#include <hpx/parallel/segmented_algorithms/detail/dispatch.hpp>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iterator>
#include <list>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parallel::detail {

    template <typename Algo, typename LocalIterator>
    struct captured_adjacent_difference
      : algorithm<captured_adjacent_difference<Algo, LocalIterator>,
            typename Algo::result_type>
    {
        using input_traits =
            hpx::traits::segmented_local_iterator_traits<LocalIterator>;
        using output_traits = hpx::traits::segmented_local_iterator_traits<
            typename Algo::result_type>;
        using raw_input_iterator = typename input_traits::local_raw_iterator;
        using raw_output_iterator = typename output_traits::local_raw_iterator;
        using result_type = typename Algo::result_type;
        using value_type =
            typename std::iterator_traits<LocalIterator>::value_type;

        captured_adjacent_difference()
          : algorithm<captured_adjacent_difference<Algo, LocalIterator>,
                result_type>("captured_adjacent_difference")
        {
        }

        template <typename ExPolicy, typename OutIterator, typename Op>
        static raw_output_iterator sequential(ExPolicy policy,
            hpx::id_type source_id, raw_input_iterator first,
            raw_input_iterator last,
            OutIterator dest, Op op, value_type previous, bool has_previous)
        {
            return invoke(HPX_MOVE(policy), source_id,
                HPX_MOVE(first), HPX_MOVE(last), HPX_MOVE(dest),
                HPX_MOVE(op), HPX_MOVE(previous), has_previous,
                std::true_type{});
        }

        template <typename ExPolicy, typename OutIterator, typename Op>
        static raw_output_iterator parallel(ExPolicy policy,
            hpx::id_type source_id, raw_input_iterator first,
            raw_input_iterator last,
            OutIterator dest, Op op, value_type previous, bool has_previous)
        {
            return invoke(HPX_MOVE(policy), source_id,
                HPX_MOVE(first), HPX_MOVE(last), HPX_MOVE(dest),
                HPX_MOVE(op), HPX_MOVE(previous), has_previous,
                std::false_type{});
        }

    private:
        template <typename ExPolicy, typename OutIterator, typename Op,
            typename IsSeq>
        static raw_output_iterator invoke(ExPolicy policy,
            hpx::id_type const& source_id, raw_input_iterator first,
            raw_input_iterator last, OutIterator dest, Op op,
            value_type previous, bool has_previous, IsSeq)
        {
            using range_type = partition_range<LocalIterator>;
            using indexed_range_type =
                indexed_partition_range<LocalIterator>;

            std::vector<indexed_range_type> ranges;
            ranges.push_back(
                indexed_range_type{0,
                    range_type{source_id, input_traits::remote(HPX_MOVE(first)),
                        input_traits::remote(HPX_MOVE(last))}});

            auto captured = get_capture_result<ExPolicy>(
                capture_async<value_type, LocalIterator>(
                    source_id, HPX_MOVE(ranges)));

            OutIterator const first_dest = dest;
            raw_output_iterator out = Algo{}.call2(
                policy(hpx::execution::non_task), IsSeq{},
                captured.values.begin(),
                captured.values.end(), HPX_MOVE(dest), op);

            if (has_previous)
            {
                *first_dest = HPX_INVOKE(op, captured.values.front(), previous);
            }
            return out;
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // segmented_adjacent_difference

    /// \cond NOINTERNAL

    // sequential remote implementation
    template <typename Algo, typename ExPolicy, typename FwdIter1,
        typename FwdIter2, typename Op>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2>
    segmented_adjacent_difference(Algo&& algo, ExPolicy const& policy,
        FwdIter1 first, FwdIter1 last, FwdIter2 dest, Op&& op, std::true_type)
    {
        using traits1 = hpx::traits::segmented_iterator_traits<FwdIter1>;
        using traits2 = hpx::traits::segmented_iterator_traits<FwdIter2>;
        using segment_iterator1 = typename traits1::segment_iterator;
        using local_iterator_type1 = typename traits1::local_iterator;
        using segment_iterator2 = typename traits2::segment_iterator;
        using local_iterator_type2 = typename traits2::local_iterator;

        using result =
            hpx::parallel::util::detail::algorithm_result<ExPolicy, FwdIter2>;
        using value_type1 =
            typename std::iterator_traits<FwdIter1>::value_type;

        if (first == last)
        {
            return result::get(HPX_MOVE(dest));
        }

        FwdIter2 end_dest = dest;
        auto const total_size = std::distance(first, last);
        std::advance(end_dest, total_size);

        FwdIter1 input_last_element = first;
        std::advance(input_last_element, total_size - 1);
        FwdIter2 output_last_element = dest;
        std::advance(output_last_element, total_size - 1);

        segment_iterator1 sit = traits1::segment(first);
        segment_iterator1 send = traits1::segment(input_last_element);
        segment_iterator2 sdest = traits2::segment(dest);
        segment_iterator2 send_dest =
            traits2::segment(output_last_element);

        local_iterator_type1 input = traits1::local(first);
        local_iterator_type2 output = traits2::local(dest);
        local_iterator_type1 input_last =
            traits1::local(input_last_element);
        ++input_last;
        local_iterator_type2 output_last =
            traits2::local(output_last_element);
        ++output_last;

        FwdIter1 input_position = first;
        FwdIter2 output_position = dest;
        std::size_t remaining = static_cast<std::size_t>(total_size);
        std::size_t chunk_index = 0;
        value_type1 previous = *first;
        std::size_t const max_chunk_size =
            max_capture_batch_elements<value_type1, value_type1>();

        while (remaining != 0)
        {
            local_iterator_type1 input_end =
                sit == send ? input_last : traits1::end(sit);
            while (input == input_end && sit != send)
            {
                ++sit;
                input = traits1::begin(sit);
                input_end = sit == send ? input_last : traits1::end(sit);
            }

            local_iterator_type2 output_end = sdest == send_dest ?
                output_last : traits2::end(sdest);
            while (output == output_end && sdest != send_dest)
            {
                ++sdest;
                output = traits2::begin(sdest);
                output_end = sdest == send_dest ?
                    output_last : traits2::end(sdest);
            }

            std::size_t const chunk_size = (std::min) (
                (std::min) (static_cast<std::size_t>(
                                std::distance(input, input_end)),
                    static_cast<std::size_t>(
                        std::distance(output, output_end))),
                (std::min) (remaining, max_chunk_size));

            HPX_ASSERT(chunk_size != 0);

            local_iterator_type1 input_chunk_end = input;
            std::advance(input_chunk_end,
                static_cast<typename std::iterator_traits<
                    local_iterator_type1>::difference_type>(chunk_size));

            FwdIter1 last_position = input_position;
            std::advance(last_position,
                static_cast<typename std::iterator_traits<
                    FwdIter1>::difference_type>(chunk_size - 1));
            value_type1 last_value = *last_position;

            hpx::id_type const source_id = traits1::get_id(sit);
            hpx::id_type const destination_id = traits2::get_id(sdest);
            bool const has_previous = chunk_index != 0;

            if (source_id == destination_id)
            {
                if (has_previous)
                {
                    dispatch(destination_id, algo, policy, std::true_type(),
                        input, input_chunk_end, output, op, previous);
                }
                else
                {
                    dispatch(destination_id, algo, policy, std::true_type(),
                        input, input_chunk_end, output, op);
                }
            }
            else
            {
                using captured_algo =
                    captured_adjacent_difference<Algo, local_iterator_type1>;

                dispatch(destination_id, captured_algo{}, policy,
                    std::true_type(), source_id, input, input_chunk_end,
                    output, op, previous, has_previous);
            }

            previous = HPX_MOVE(last_value);
            remaining -= chunk_size;
            chunk_index++;
            std::advance(input_position,
                static_cast<typename std::iterator_traits<
                    FwdIter1>::difference_type>(chunk_size));
            std::advance(output_position,
                static_cast<typename std::iterator_traits<
                    FwdIter2>::difference_type>(chunk_size));
            std::advance(input,
                static_cast<typename std::iterator_traits<
                    local_iterator_type1>::difference_type>(chunk_size));
            std::advance(output,
                static_cast<typename std::iterator_traits<
                    local_iterator_type2>::difference_type>(chunk_size));

            if (remaining != 0 && input == input_end)
            {
                ++sit;
                input = traits1::begin(sit);
            }
            if (remaining != 0 && output == output_end)
            {
                ++sdest;
                output = traits2::begin(sdest);
            }
        }

        return result::get(HPX_MOVE(end_dest));
    }

    // parallel remote implementation
    template <typename Algo, typename ExPolicy, typename FwdIter1,
        typename FwdIter2, typename Op>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2>
    segmented_adjacent_difference(Algo&& algo, ExPolicy const& policy,
        FwdIter1 first, FwdIter1 last, FwdIter2 dest, Op&& op, std::false_type)
    {
        using traits1 = hpx::traits::segmented_iterator_traits<FwdIter1>;
        using traits2 = hpx::traits::segmented_iterator_traits<FwdIter2>;
        using segment_iterator1 = typename traits1::segment_iterator;
        using local_iterator_type1 = typename traits1::local_iterator;
        using segment_iterator2 = typename traits2::segment_iterator;
        using local_iterator_type2 = typename traits2::local_iterator;

        using result =
            hpx::parallel::util::detail::algorithm_result<ExPolicy, FwdIter2>;

        using forced_seq =
            std::integral_constant<bool, !std::forward_iterator<FwdIter1>>;

        using value_type1 =
            typename std::iterator_traits<FwdIter1>::value_type;

        if (first == last)
        {
            return result::get(HPX_MOVE(dest));
        }

        if constexpr (requires { first == dest; })
        {
            if (first == dest)
            {
                return segmented_adjacent_difference(HPX_FORWARD(Algo, algo),
                    policy, first, last, dest, HPX_FORWARD(Op, op),
                    std::true_type{});
            }
        }

        FwdIter2 end_dest = dest;
        auto const total_size = std::distance(first, last);
        std::advance(end_dest, total_size);

        FwdIter1 input_last_element = first;
        std::advance(input_last_element, total_size - 1);
        FwdIter2 output_last_element = dest;
        std::advance(output_last_element, total_size - 1);

        segment_iterator1 sit = traits1::segment(first);
        segment_iterator1 send = traits1::segment(input_last_element);
        segment_iterator2 sdest = traits2::segment(dest);
        segment_iterator2 send_dest =
            traits2::segment(output_last_element);

        local_iterator_type1 input = traits1::local(first);
        local_iterator_type2 output = traits2::local(dest);
        local_iterator_type1 input_last =
            traits1::local(input_last_element);
        ++input_last;
        local_iterator_type2 output_last =
            traits2::local(output_last_element);
        ++output_last;

        using segment_type = std::vector<future<local_iterator_type2>>;
        segment_type segments;

        auto const size = std::distance(sit, send) + 1;
        segments.reserve(static_cast<std::size_t>(size));

        struct boundary_values
        {
            FwdIter2 dest;
            value_type1 current;
            value_type1 previous;
        };

        std::vector<boundary_values> boundaries;
        boundaries.reserve(size);

        FwdIter1 input_position = first;
        std::size_t remaining = static_cast<std::size_t>(total_size);
        std::size_t chunk_index = 0;
        value_type1 previous = *first;
        std::size_t const max_chunk_size =
            max_capture_batch_elements<value_type1, value_type1>();

        while (remaining != 0)
        {
            local_iterator_type1 input_end =
                sit == send ? input_last : traits1::end(sit);
            while (input == input_end && sit != send)
            {
                ++sit;
                input = traits1::begin(sit);
                input_end = sit == send ? input_last : traits1::end(sit);
            }

            local_iterator_type2 output_end = sdest == send_dest ?
                output_last : traits2::end(sdest);
            while (output == output_end && sdest != send_dest)
            {
                ++sdest;
                output = traits2::begin(sdest);
                output_end = sdest == send_dest ?
                    output_last : traits2::end(sdest);
            }

            auto const input_available = static_cast<std::size_t>(
                std::distance(input, input_end));
            auto const output_available = static_cast<std::size_t>(
                std::distance(output, output_end));
            std::size_t const chunk_size = (std::min) (
                (std::min) (input_available, output_available),
                (std::min) (remaining, max_chunk_size));

            HPX_ASSERT(chunk_size != 0);

            local_iterator_type1 input_chunk_end = input;
            std::advance(input_chunk_end,
                static_cast<typename std::iterator_traits<
                    local_iterator_type1>::difference_type>(chunk_size));
            bool const has_previous = chunk_index != 0;
            FwdIter1 last_position = input_position;
            std::advance(last_position,
                static_cast<typename std::iterator_traits<
                    FwdIter1>::difference_type>(chunk_size - 1));
            value_type1 last_value = *last_position;

            hpx::id_type const source_id = traits1::get_id(sit);
            hpx::id_type const destination_id = traits2::get_id(sdest);

            if (source_id == destination_id)
            {
                if (has_previous)
                {
                    FwdIter2 boundary_dest = dest;
                    std::advance(boundary_dest,
                        std::distance(first, input_position));
                    boundaries.push_back(boundary_values{
                        HPX_MOVE(boundary_dest), value_type1(*input_position),
                        previous});
                }

                segments.push_back(dispatch_async(destination_id, algo, policy,
                    forced_seq(), input, input_chunk_end, output, op));
            }
            else
            {
                using captured_algo =
                    captured_adjacent_difference<Algo, local_iterator_type1>;

                segments.push_back(dispatch_async(destination_id,
                    captured_algo{}, policy, forced_seq(), source_id, input,
                    input_chunk_end, output, op, previous, has_previous));
            }

            previous = HPX_MOVE(last_value);
            remaining -= chunk_size;
            chunk_index++;
            std::advance(input_position,
                static_cast<typename std::iterator_traits<
                    FwdIter1>::difference_type>(chunk_size));
            std::advance(input,
                static_cast<typename std::iterator_traits<
                    local_iterator_type1>::difference_type>(chunk_size));
            std::advance(output,
                static_cast<typename std::iterator_traits<
                    local_iterator_type2>::difference_type>(chunk_size));

            if (remaining != 0 && input == input_end)
            {
                ++sit;
                input = traits1::begin(sit);
            }
            if (remaining != 0 && output == output_end)
            {
                ++sdest;
                output = traits2::begin(sdest);
            }
        }

        return result::get(dataflow(
            [=](segment_type&& r) mutable -> FwdIter2 {
                // handle any remote exceptions, will throw on error
                std::list<std::exception_ptr> errors;
                parallel::util::detail::handle_remote_exceptions<
                    ExPolicy>::call(r, errors);

                for (auto& segment : r)
                {
                    segment.get();
                }
                for (auto& boundary : boundaries)
                {
                    *boundary.dest = HPX_INVOKE(
                        op, boundary.current, boundary.previous);
                }
                return end_dest;
            },
            HPX_MOVE(segments)));
    }
    /// \endcond
}    // namespace hpx::parallel::detail

// The segmented iterators we support all live in namespace hpx::segmented
namespace hpx::segmented {

    HPX_CXX_EXPORT template <typename ExPolicy, typename FwdIter1,
        typename FwdIter2, typename Op>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<FwdIter1> &&
            hpx::traits::is_segmented_iterator_v<FwdIter1> &&
            hpx::traits::is_iterator_v<FwdIter2> &&
            hpx::traits::is_segmented_iterator_v<FwdIter2>)
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, FwdIter2>
    hpx_invoke(hpx::adjacent_difference_t, ExPolicy&& policy, FwdIter1 first,
        FwdIter1 last, FwdIter2 dest, Op&& op)
    {
        static_assert(std::forward_iterator<FwdIter1>,
            "Requires at least forward iterator.");
        static_assert(std::forward_iterator<FwdIter2>,
            "Requires at least forward iterator.");

        if (first == last)
        {
            using result =
                hpx::parallel::util::detail::algorithm_result<ExPolicy,
                    FwdIter2>;
            return result::get(HPX_MOVE(dest));
        }

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;
        using traits = hpx::traits::segmented_iterator_traits<FwdIter2>;

        return hpx::parallel::detail::segmented_adjacent_difference(
            hpx::parallel::detail::adjacent_difference<
                typename traits::local_iterator>(),
            HPX_FORWARD(ExPolicy, policy), first, last, dest,
            HPX_FORWARD(Op, op), is_seq());
    }

    HPX_CXX_EXPORT template <typename InIter1, typename InIter2, typename Op>
        requires(hpx::traits::is_iterator_v<InIter1> &&
            hpx::traits::is_segmented_iterator_v<InIter1> &&
            hpx::traits::is_iterator_v<InIter2> &&
            hpx::traits::is_segmented_iterator_v<InIter2>)
    InIter2 hpx_invoke(hpx::adjacent_difference_t, InIter1 first, InIter1 last,
        InIter2 dest, Op&& op)
    {
        static_assert(
            std::input_iterator<InIter1>, "Requires at least input iterator.");
        static_assert(
            std::input_iterator<InIter2>, "Requires at least input iterator.");

        if (first == last)
        {
            return dest;
        }

        using traits = hpx::traits::segmented_iterator_traits<InIter2>;

        return hpx::parallel::detail::segmented_adjacent_difference(
            hpx::parallel::detail::adjacent_difference<
                typename traits::local_iterator>(),
            hpx::execution::seq, first, last, dest, HPX_FORWARD(Op, op),
            std::true_type{});
    }
}    // namespace hpx::segmented
