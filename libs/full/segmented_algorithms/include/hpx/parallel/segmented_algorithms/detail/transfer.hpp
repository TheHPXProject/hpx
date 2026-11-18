//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/executors.hpp>

#include <hpx/parallel/segmented_algorithms/detail/capture_dispatch.hpp>
#include <hpx/parallel/segmented_algorithms/detail/dispatch.hpp>

#include <algorithm>
#include <exception>
#include <iterator>
#include <list>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    // segmented transfer

    /// \cond NOINTERNAL

    template <typename InputLocal, typename OutputLocal>
    struct captured_move
      : algorithm<captured_move<InputLocal, OutputLocal>, std::size_t>
    {
        using output_traits =
            hpx::traits::segmented_local_iterator_traits<OutputLocal>;
        using raw_output_iterator = typename output_traits::local_raw_iterator;
        using value_type =
            typename std::iterator_traits<InputLocal>::value_type;

        captured_move()
          : algorithm<captured_move<InputLocal, OutputLocal>, std::size_t>(
                "captured_move")
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
            using local_result =
                util::in_out_result<typename std::vector<value_type>::iterator,
                    raw_output_iterator>;
            move_pair<local_result>{}.call2(policy(hpx::execution::non_task),
                IsSeq{}, values.begin(), values.end(), HPX_MOVE(dest));
            return values.size();
        }
    };

    // sequential remote implementation
    template <typename Algo, typename ExPolicy, typename SegIter,
        typename SegOutIter>
    static util::detail::algorithm_result_t<ExPolicy,
        util::in_out_result<SegIter, SegOutIter>>
    segmented_transfer(Algo&& algo, ExPolicy const& policy, std::true_type,
        SegIter first, SegIter last, SegOutIter dest)
    {
        using traits = hpx::traits::segmented_iterator_traits<SegIter>;
        using segment_iterator = typename traits::segment_iterator;
        using local_iterator_type = typename traits::local_iterator;

        using output_traits =
            hpx::traits::segmented_iterator_traits<SegOutIter>;
        using segment_output_iterator =
            typename output_traits::segment_iterator;
        using local_output_iterator_type =
            typename output_traits::local_iterator;

        segment_iterator sit = traits::segment(first);
        segment_iterator send = traits::segment(last);
        segment_output_iterator sdest = output_traits::segment(dest);
        std::size_t remaining =
            static_cast<std::size_t>(std::distance(first, last));
        local_iterator_type input = traits::local(first);
        local_output_iterator_type output = output_traits::local(dest);

        while (remaining != 0)
        {
            local_iterator_type input_end =
                sit == send ? traits::local(last) : traits::end(sit);
            while (input == input_end && sit != send)
            {
                ++sit;
                input = traits::begin(sit);
                input_end =
                    sit == send ? traits::local(last) : traits::end(sit);
            }

            local_output_iterator_type output_end = output_traits::end(sdest);
            while (output == output_end)
            {
                ++sdest;
                output = output_traits::begin(sdest);
                output_end = output_traits::end(sdest);
            }

            auto count =
                (std::min) ((std::min) (std::distance(input, input_end),
                                std::distance(output, output_end)),
                    static_cast<decltype(std::distance(input, input_end))>(
                        remaining));
            HPX_ASSERT(count > 0);

            hpx::id_type const source_id = traits::get_id(sit);
            hpx::id_type const destination_id = output_traits::get_id(sdest);
            bool const same_locality = get_partition_locality(source_id) ==
                get_partition_locality(destination_id);
            if (!same_locality)
            {
                using value_type = typename std::iterator_traits<
                    local_iterator_type>::value_type;
                count = (std::min) (count,
                    static_cast<decltype(count)>(
                        max_capture_batch_elements<value_type, value_type>()));
            }
            local_iterator_type input_chunk_end = input;
            std::advance(input_chunk_end, count);

            if (same_locality)
            {
                dispatch(destination_id, algo, policy, std::true_type(), input,
                    input_chunk_end, output);
            }
            else
            {
                using value_type = typename std::iterator_traits<
                    local_iterator_type>::value_type;
                std::vector<indexed_partition_range<local_iterator_type>>
                    ranges{{0, {source_id, input, input_chunk_end}}};
                auto captured = get_capture_result<ExPolicy>(
                    capture_move_async<value_type, local_iterator_type>(
                        source_id, HPX_MOVE(ranges)));
                dispatch(destination_id,
                    captured_move<local_iterator_type,
                        local_output_iterator_type>{},
                    policy, std::true_type(), HPX_MOVE(captured.values),
                    output);
            }

            input = input_chunk_end;
            std::advance(output, count);
            std::advance(dest, count);
            remaining -= static_cast<std::size_t>(count);
        }

        using result_type = util::in_out_result<SegIter, SegOutIter>;

        return util::detail::algorithm_result<ExPolicy, result_type>::get(
            result_type{last, dest});
    }

    // parallel remote implementation
    template <typename Algo, typename ExPolicy, typename SegIter,
        typename SegOutIter>
    static util::detail::algorithm_result_t<ExPolicy,
        util::in_out_result<SegIter, SegOutIter>>
    segmented_transfer(Algo&& algo, ExPolicy const& policy, std::false_type,
        SegIter first, SegIter last, SegOutIter dest)
    {
        using traits = hpx::traits::segmented_iterator_traits<SegIter>;
        using segment_iterator = typename traits::segment_iterator;
        using local_iterator_type = typename traits::local_iterator;

        using output_traits =
            hpx::traits::segmented_iterator_traits<SegOutIter>;
        using segment_output_iterator =
            typename output_traits::segment_iterator;
        using local_output_iterator_type =
            typename output_traits::local_iterator;

        using forced_seq =
            std::integral_constant<bool, !std::forward_iterator<SegIter>>;

        segment_iterator sit = traits::segment(first);
        segment_iterator send = traits::segment(last);
        segment_output_iterator sdest = output_traits::segment(dest);

        std::size_t remaining =
            static_cast<std::size_t>(std::distance(first, last));
        local_iterator_type input = traits::local(first);
        local_output_iterator_type output = output_traits::local(dest);

        std::vector<hpx::future<void>> segments;
        segments.reserve(std::distance(sit, send) + 1);
        hpx::future<void> captures = hpx::make_ready_future();

        while (remaining != 0)
        {
            local_iterator_type input_end =
                sit == send ? traits::local(last) : traits::end(sit);
            while (input == input_end && sit != send)
            {
                ++sit;
                input = traits::begin(sit);
                input_end =
                    sit == send ? traits::local(last) : traits::end(sit);
            }

            local_output_iterator_type output_end = output_traits::end(sdest);
            while (output == output_end)
            {
                ++sdest;
                output = output_traits::begin(sdest);
                output_end = output_traits::end(sdest);
            }

            auto count =
                (std::min) ((std::min) (std::distance(input, input_end),
                                std::distance(output, output_end)),
                    static_cast<decltype(std::distance(input, input_end))>(
                        remaining));
            HPX_ASSERT(count > 0);

            hpx::id_type const source_id = traits::get_id(sit);
            hpx::id_type const destination_id = output_traits::get_id(sdest);
            bool const same_locality = get_partition_locality(source_id) ==
                get_partition_locality(destination_id);
            if (!same_locality)
            {
                using value_type = typename std::iterator_traits<
                    local_iterator_type>::value_type;
                count = (std::min) (count,
                    static_cast<decltype(count)>(
                        max_capture_batch_elements<value_type, value_type>()));
            }
            local_iterator_type input_chunk_end = input;
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
                        local_iterator_type>::value_type;
                    std::vector<indexed_partition_range<local_iterator_type>>
                        ranges{{0, {source_id, input, input_chunk_end}}};
                    return capture_move_async<value_type, local_iterator_type>(
                        source_id, HPX_MOVE(ranges))
                        .then([=](auto ready) mutable {
                            auto captured =
                                get_capture_result<ExPolicy>(HPX_MOVE(ready));
                            return dispatch_async(destination_id,
                                captured_move<local_iterator_type,
                                    local_output_iterator_type>{},
                                policy, forced_seq(), HPX_MOVE(captured.values),
                                output)
                                .then([](auto done) { done.get(); });
                        });
                });
            }

            input = input_chunk_end;
            std::advance(output, count);
            std::advance(dest, count);
            remaining -= static_cast<std::size_t>(count);
        }

        segments.push_back(HPX_MOVE(captures));
        return util::detail::algorithm_result<ExPolicy,
            util::in_out_result<SegIter, SegOutIter>>::
            get(hpx::dataflow(
                [=](std::vector<hpx::future<void>>&& operations)
                    -> util::in_out_result<SegIter, SegOutIter> {
                    (void) hpx::wait_all_nothrow(operations);
                    std::list<std::exception_ptr> errors;
                    parallel::util::detail::handle_remote_exceptions<
                        ExPolicy>::call(operations, errors);
                    return {HPX_MOVE(last), HPX_MOVE(dest)};
                },
                HPX_MOVE(segments)));
    }

    ///////////////////////////////////////////////////////////////////////
    // segmented implementation
    HPX_CXX_EXPORT template <typename Algo, typename ExPolicy,
        typename FwdIter1, typename Sent1, typename FwdIter2>
    util::detail::algorithm_result_t<ExPolicy,
        util::in_out_result<FwdIter1, FwdIter2>>
    transfer_(ExPolicy&& policy, FwdIter1 first, Sent1 last, FwdIter2 dest,
        std::true_type)
    {
        if (first == last)
        {
            using result_type = util::in_out_result<FwdIter1, FwdIter2>;

            return util::detail::algorithm_result<ExPolicy, result_type>::get(
                result_type{last, dest});
        }

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;
        return segmented_transfer(
            Algo(), HPX_FORWARD(ExPolicy, policy), is_seq(), first, last, dest);
    }

    // forward declare the non-segmented version of this algorithm
    template <typename Algo, typename ExPolicy, typename FwdIter1,
        typename Sent1, typename FwdIter2>
    util::detail::algorithm_result_t<ExPolicy,
        util::in_out_result<FwdIter1, FwdIter2>>
    transfer_(ExPolicy&& policy, FwdIter1 first, Sent1 last, FwdIter2 dest,
        std::false_type);

    /// \endcond
}    // namespace hpx::parallel::detail
