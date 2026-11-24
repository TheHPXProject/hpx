//  Copyright (c) 2026 Arpit Khandelwal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/executors.hpp>

#include <hpx/parallel/segmented_algorithms/detail/capture_dispatch.hpp>
#include <hpx/parallel/segmented_algorithms/detail/dispatch.hpp>
#include <hpx/parallel/segmented_algorithms/detail/transfer.hpp>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iterator>
#include <list>
#include <numeric>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    // segmented_copy

    template <typename InputLocal, typename OutputLocal>
    struct captured_copy
      : algorithm<captured_copy<InputLocal, OutputLocal>, std::size_t>
    {
        using output_traits =
            hpx::traits::segmented_local_iterator_traits<OutputLocal>;
        using raw_output_iterator = typename output_traits::local_raw_iterator;
        using value_type =
            typename std::iterator_traits<InputLocal>::value_type;

        captured_copy()
          : algorithm<captured_copy<InputLocal, OutputLocal>, std::size_t>(
                "captured_copy")
        {
        }

        template <typename ExPolicy>
        static std::size_t sequential(ExPolicy policy,
            std::vector<value_type> values, raw_output_iterator dest)
        {
            return invoke(HPX_MOVE(policy), HPX_MOVE(values), HPX_MOVE(dest),
                std::true_type{});
        }

        template <typename ExPolicy>
        static std::size_t parallel(ExPolicy policy,
            std::vector<value_type> values, raw_output_iterator dest)
        {
            return invoke(HPX_MOVE(policy), HPX_MOVE(values), HPX_MOVE(dest),
                std::false_type{});
        }

    private:
        template <typename ExPolicy, typename IsSeq>
        static std::size_t invoke(ExPolicy policy,
            std::vector<value_type> values, raw_output_iterator dest, IsSeq)
        {
            using raw_input_iterator =
                typename hpx::traits::segmented_local_iterator_traits<
                    InputLocal>::local_raw_iterator;
            constexpr bool const_input =
                std::is_const_v<std::remove_reference_t<typename std::
                        iterator_traits<raw_input_iterator>::reference>>;
            auto input = [&]() {
                if constexpr (const_input)
                    return values.cbegin();
                else
                    return values.begin();
            }();
            auto end = [&]() {
                if constexpr (const_input)
                    return values.cend();
                else
                    return values.end();
            }();
            using local_result =
                util::in_out_result<decltype(input), raw_output_iterator>;
            copy<local_result>{}.call2(policy(hpx::execution::non_task),
                IsSeq{}, input, end, HPX_MOVE(dest));
            return values.size();
        }
    };

    /// \cond NOINTERNAL

    // sequential remote implementation
    template <typename Algo, typename ExPolicy, typename SegIter,
        typename OutIter>
    util::detail::algorithm_result_t<ExPolicy,
        util::in_out_result<SegIter, OutIter>>
    segmented_copy(Algo&& algo, ExPolicy const& policy, SegIter first,
        SegIter last, OutIter dest, std::true_type)
    {
        using traits1 = hpx::traits::segmented_iterator_traits<SegIter>;
        using traits2 = hpx::traits::segmented_iterator_traits<OutIter>;
        using segment_iterator1 = typename traits1::segment_iterator;
        using local_iterator_type1 = typename traits1::local_iterator;
        using segment_iterator2 = typename traits2::segment_iterator;
        using local_iterator_type2 = typename traits2::local_iterator;

        using result = util::detail::algorithm_result<ExPolicy,
            util::in_out_result<SegIter, OutIter>>;

        auto remaining = std::distance(first, last);
        segment_iterator1 sit = traits1::segment(first);
        segment_iterator1 send = traits1::segment(last);
        segment_iterator2 sdest = traits2::segment(dest);
        local_iterator_type1 input = traits1::local(first);
        local_iterator_type2 output = traits2::local(dest);

        while (remaining != 0)
        {
            local_iterator_type1 input_end =
                sit == send ? traits1::local(last) : traits1::end(sit);
            while (input == input_end && sit != send)
            {
                ++sit;
                input = traits1::begin(sit);
                input_end =
                    sit == send ? traits1::local(last) : traits1::end(sit);
            }

            local_iterator_type2 output_end = traits2::end(sdest);
            while (output == output_end)
            {
                ++sdest;
                output = traits2::begin(sdest);
                output_end = traits2::end(sdest);
            }

            auto count =
                (std::min) ((std::min) (std::distance(input, input_end),
                                std::distance(output, output_end)),
                    remaining);
            HPX_ASSERT(count > 0);
            hpx::id_type const source_id = traits1::get_id(sit);
            hpx::id_type const destination_id = traits2::get_id(sdest);
            bool const same_locality = get_partition_locality(source_id) ==
                get_partition_locality(destination_id);
            if (!same_locality)
            {
                using value_type = typename std::iterator_traits<
                    local_iterator_type1>::value_type;
                count = (std::min) (count,
                    static_cast<decltype(count)>(
                        max_capture_batch_elements<value_type, value_type>()));
            }

            local_iterator_type1 input_chunk_end = input;
            std::advance(input_chunk_end, count);
            if (same_locality)
            {
                dispatch(destination_id, algo, policy, std::true_type(), input,
                    input_chunk_end, output);
            }
            else
            {
                using value_type = typename std::iterator_traits<
                    local_iterator_type1>::value_type;
                std::vector<indexed_partition_range<local_iterator_type1>>
                    ranges{{0, {source_id, input, input_chunk_end}}};
                auto captured = get_capture_result<ExPolicy>(
                    capture_async<value_type, local_iterator_type1>(
                        source_id, HPX_MOVE(ranges)));
                dispatch(destination_id,
                    captured_copy<local_iterator_type1, local_iterator_type2>{},
                    policy, std::true_type(), HPX_MOVE(captured.values),
                    output);
            }

            input = input_chunk_end;
            std::advance(output, count);
            std::advance(dest, count);
            remaining -= count;
        }
        return result::get(util::in_out_result<SegIter, OutIter>{
            HPX_MOVE(last), HPX_MOVE(dest)});
    }

    // parallel remote implementation
    template <typename Algo, typename ExPolicy, typename SegIter,
        typename OutIter>
    util::detail::algorithm_result_t<ExPolicy,
        util::in_out_result<SegIter, OutIter>>
    segmented_copy(Algo&& algo, ExPolicy const& policy, SegIter first,
        SegIter last, OutIter dest, std::false_type)
    {
        using traits1 = hpx::traits::segmented_iterator_traits<SegIter>;
        using traits2 = hpx::traits::segmented_iterator_traits<OutIter>;
        using segment_iterator1 = typename traits1::segment_iterator;
        using local_iterator_type1 = typename traits1::local_iterator;
        using segment_iterator2 = typename traits2::segment_iterator;
        using local_iterator_type2 = typename traits2::local_iterator;

        using result = util::detail::algorithm_result<ExPolicy,
            util::in_out_result<SegIter, OutIter>>;

        using forced_seq = std::integral_constant<bool,
            !hpx::traits::is_forward_iterator_v<SegIter>>;

        auto remaining = std::distance(first, last);
        segment_iterator1 sit = traits1::segment(first);
        segment_iterator1 send = traits1::segment(last);
        segment_iterator2 sdest = traits2::segment(dest);
        local_iterator_type1 input = traits1::local(first);
        local_iterator_type2 output = traits2::local(dest);

        std::vector<hpx::future<void>> segments;
        hpx::future<void> captures = hpx::make_ready_future();

        while (remaining != 0)
        {
            local_iterator_type1 input_end =
                sit == send ? traits1::local(last) : traits1::end(sit);
            while (input == input_end && sit != send)
            {
                ++sit;
                input = traits1::begin(sit);
                input_end =
                    sit == send ? traits1::local(last) : traits1::end(sit);
            }

            local_iterator_type2 output_end = traits2::end(sdest);
            while (output == output_end)
            {
                ++sdest;
                output = traits2::begin(sdest);
                output_end = traits2::end(sdest);
            }

            auto count =
                (std::min) ((std::min) (std::distance(input, input_end),
                                std::distance(output, output_end)),
                    remaining);
            HPX_ASSERT(count > 0);
            hpx::id_type const source_id = traits1::get_id(sit);
            hpx::id_type const destination_id = traits2::get_id(sdest);
            bool const same_locality = get_partition_locality(source_id) ==
                get_partition_locality(destination_id);
            if (!same_locality)
            {
                using value_type = typename std::iterator_traits<
                    local_iterator_type1>::value_type;
                count = (std::min) (count,
                    static_cast<decltype(count)>(
                        max_capture_batch_elements<value_type, value_type>()));
            }

            local_iterator_type1 input_chunk_end = input;
            std::advance(input_chunk_end, count);
            if (same_locality)
            {
                segments.push_back(dispatch_async(destination_id, algo, policy,
                    forced_seq(), input, input_chunk_end, output)
                        .then([](auto ready) { ready.get(); }));
            }
            else
            {
                captures = HPX_MOVE(captures).then([=](hpx::future<void>
                                                           previous) mutable
                                                       -> hpx::future<void> {
                    previous.get();
                    using value_type = typename std::iterator_traits<
                        local_iterator_type1>::value_type;
                    std::vector<indexed_partition_range<local_iterator_type1>>
                        ranges{{0, {source_id, input, input_chunk_end}}};
                    return capture_async<value_type, local_iterator_type1>(
                        source_id, HPX_MOVE(ranges))
                        .then([=](auto ready) mutable {
                            auto captured =
                                get_capture_result<ExPolicy>(HPX_MOVE(ready));
                            return dispatch_async(destination_id,
                                captured_copy<local_iterator_type1,
                                    local_iterator_type2>{},
                                policy, forced_seq(), HPX_MOVE(captured.values),
                                output)
                                .then([](auto done) { done.get(); });
                        });
                });
            }

            input = input_chunk_end;
            std::advance(output, count);
            std::advance(dest, count);
            remaining -= count;
        }

        segments.push_back(HPX_MOVE(captures));
        return result::get(dataflow(
            [=](std::vector<hpx::future<void>>&& operations) mutable
                -> util::in_out_result<SegIter, OutIter> {
                (void) hpx::wait_all_nothrow(operations);
                std::list<std::exception_ptr> errors;
                parallel::util::detail::handle_remote_exceptions<
                    ExPolicy>::call(operations, errors);
                return {HPX_MOVE(last), HPX_MOVE(dest)};
            },
            HPX_MOVE(segments)));
    }
    /// \endcond
}    // namespace hpx::parallel::detail

// The segmented iterators we support all live in namespace hpx::segmented
namespace hpx::segmented {

    ///////////////////////////////////////////////////////////////////////////
    // segmented copy - hpx_invoke overloads

    // no-policy (sequential) overload
    HPX_CXX_EXPORT template <typename SegIter, typename OutIter>
        requires(hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter> &&
            hpx::traits::is_iterator_v<OutIter> &&
            hpx::traits::is_segmented_iterator_v<OutIter>)
    OutIter hpx_invoke(hpx::copy_t, SegIter first, SegIter last, OutIter dest)
    {
        // the segmented iterator is read more than once (its segment and its
        // local position), it can't be a single-pass iterator
        static_assert(hpx::traits::is_forward_iterator_v<SegIter>,
            "Requires at least forward iterator.");

        if (first == last)
        {
            return HPX_MOVE(dest);
        }

        using iterator_traits1 =
            hpx::traits::segmented_iterator_traits<SegIter>;
        using iterator_traits2 =
            hpx::traits::segmented_iterator_traits<OutIter>;

        auto result = hpx::parallel::detail::segmented_copy(
            hpx::parallel::detail::copy<hpx::parallel::util::in_out_result<
                typename iterator_traits1::local_iterator,
                typename iterator_traits2::local_iterator>>(),
            hpx::execution::seq, first, last, dest, std::true_type{});

        return HPX_MOVE(result.out);
    }

    // execution-policy overload
    HPX_CXX_EXPORT template <typename ExPolicy, typename SegIter,
        typename OutIter>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter> &&
            hpx::traits::is_iterator_v<OutIter> &&
            hpx::traits::is_segmented_iterator_v<OutIter>)
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy, OutIter>
    hpx_invoke(hpx::copy_t, ExPolicy&& policy, SegIter first, SegIter last,
        OutIter dest)
    {
        static_assert(hpx::traits::is_forward_iterator_v<SegIter>,
            "Requires at least forward iterator.");

        using is_seq = typename hpx::is_sequenced_execution_policy<
            std::decay_t<ExPolicy>>::type;

        if (first == last)
        {
            using result =
                hpx::parallel::util::detail::algorithm_result<ExPolicy,
                    OutIter>;
            return result::get(HPX_MOVE(dest));
        }

        using iterator_traits1 =
            hpx::traits::segmented_iterator_traits<SegIter>;
        using iterator_traits2 =
            hpx::traits::segmented_iterator_traits<OutIter>;

        return hpx::parallel::util::get_second_element(
            hpx::parallel::detail::segmented_copy(
                hpx::parallel::detail::copy<hpx::parallel::util::in_out_result<
                    typename iterator_traits1::local_iterator,
                    typename iterator_traits2::local_iterator>>(),
                HPX_FORWARD(ExPolicy, policy), first, last, dest, is_seq{}));
    }
}    // namespace hpx::segmented
