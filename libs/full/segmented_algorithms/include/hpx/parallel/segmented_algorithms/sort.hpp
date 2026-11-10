//  Copyright (c) 2026 Pratyksh Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/async_colocated.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/runtime_local.hpp>

#include <hpx/parallel/segmented_algorithms/detail/dispatch.hpp>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iterator>
#include <list>
#include <map>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parallel::detail {

    /// \cond NOINTERNAL

    // sequential() is templated on the iterator actually passed after
    // segmented_local_iterator_traits unwraps to a raw iterator. The class
    // result type stays the local (serializable) iterator so dispatch can
    // return it.
    template <typename Iter>
    struct segmented_local_sort : algorithm<segmented_local_sort<Iter>, Iter>
    {
        constexpr segmented_local_sort() noexcept
          : algorithm<segmented_local_sort, Iter>("segmented_local_sort")
        {
        }

        template <typename ExPolicy, typename InIter, typename Sent,
            typename Comp, typename Proj>
        static InIter sequential(ExPolicy&& policy, InIter first, Sent last,
            Comp&& comp, Proj&& proj)
        {
            auto last_iter = advance_to_sentinel(first, last);
            // Unwrap to the local raw iterator (same path as dispatch). Fold
            // proj into the comparator: hpx::sort's CPO has no projection.
            using local_traits =
                hpx::traits::segmented_local_iterator_traits<InIter>;
            hpx::sort(hpx::execution::experimental::to_non_task(
                          HPX_FORWARD(ExPolicy, policy)),
                local_traits::local(first), local_traits::local(last_iter),
                util::compare_projected<Comp&&, Proj&&>(
                    HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj)));
            return last_iter;
        }

        template <typename ExPolicy, typename InIter, typename Sent,
            typename Comp, typename Proj>
        static InIter parallel(ExPolicy&& policy, InIter first, Sent last,
            Comp&& comp, Proj&& proj)
        {
            // The shared control path still passes the parallel policy to the
            // local hpx::sort invocation above.
            return sequential(HPX_FORWARD(ExPolicy, policy), first, last,
                HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
        }
    };

    /// Merge two adjacent sorted ranges stored in one physical partition.
    template <typename Iter>
    struct segmented_local_merge : algorithm<segmented_local_merge<Iter>, Iter>
    {
        constexpr segmented_local_merge() noexcept
          : algorithm<segmented_local_merge, Iter>("segmented_local_merge")
        {
        }

        /// Merge two local ranges using the supplied policy and projection.
        template <typename ExPolicy, typename InIter, typename Comp,
            typename Proj>
        static InIter sequential(ExPolicy&& policy, InIter first, InIter middle,
            InIter last, Comp&& comp, Proj&& proj)
        {
            using local_traits =
                hpx::traits::segmented_local_iterator_traits<InIter>;
            using predicate_type =
                util::compare_projected<std::decay_t<Comp>, std::decay_t<Proj>>;
            predicate_type pred(
                HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
            hpx::inplace_merge(hpx::execution::experimental::to_non_task(
                                   HPX_FORWARD(ExPolicy, policy)),
                local_traits::local(first), local_traits::local(middle),
                local_traits::local(last), pred);
            return last;
        }

        /// Parallel entry point for merging two local ranges.
        template <typename ExPolicy, typename InIter, typename Comp,
            typename Proj>
        static InIter parallel(ExPolicy&& policy, InIter first, InIter middle,
            InIter last, Comp&& comp, Proj&& proj)
        {
            return sequential(HPX_FORWARD(ExPolicy, policy), first, middle,
                last, HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
        }
    };

    // Bound outstanding transport and namespace requests independently of
    // the number of partitions.
    inline constexpr std::size_t segmented_sort_transfer_limit = 8;

    // Even a synchronous launch failure must not leave actions accessing the
    // input after the caller observes an exception.
    template <typename ExPolicy, typename T>
    void segmented_sort_wait(
        std::vector<hpx::future<T>>& pending, std::exception_ptr error = {})
    {
        bool const exceptional = hpx::wait_all_nothrow(pending);
        if (!exceptional && !error)
        {
            return;
        }

        std::list<std::exception_ptr> errors;
        using handler = util::detail::handle_remote_exceptions<ExPolicy>;
        if (error)
        {
            handler::call(error, errors);
        }
        handler::call(pending, errors);
    }

    template <typename LocalIter>
    struct segmented_sort_run
    {
        hpx::id_type id{};
        hpx::id_type locality{};
        LocalIter first{};
        LocalIter last{};

        std::ptrdiff_t size() const
        {
            return last - first;
        }
    };

    template <typename SegIter>
    auto segmented_sort_runs(SegIter first, SegIter last)
    {
        using traits = hpx::traits::segmented_iterator_traits<SegIter>;
        using segment_iterator = typename traits::segment_iterator;
        using local_iterator = typename traits::local_iterator;
        using run_type = segmented_sort_run<local_iterator>;

        segment_iterator sit = traits::segment(first);
        segment_iterator send = traits::segment(last);

        std::vector<run_type> runs;
        runs.reserve(static_cast<std::size_t>(std::distance(sit, send)) + 1);

        auto add_run = [&](segment_iterator const& segment,
                           local_iterator const& beg,
                           local_iterator const& end) {
            if (beg != end)
            {
                auto id = traits::get_id(segment);
                if constexpr (requires { traits::get_locality_id(segment); })
                {
                    hpx::id_type locality;
                    auto const locality_id = traits::get_locality_id(segment);
                    if (locality_id != hpx::naming::invalid_locality_id)
                    {
                        locality =
                            hpx::naming::get_id_from_locality_id(locality_id);
                    }
                    runs.emplace_back(
                        HPX_MOVE(id), HPX_MOVE(locality), beg, end);
                }
                else
                {
                    runs.emplace_back(HPX_MOVE(id), hpx::id_type{}, beg, end);
                }
            }
        };

        if (sit == send)
        {
            add_run(sit, traits::local(first), traits::local(last));
        }
        else
        {
            add_run(sit, traits::local(first), traits::end(sit));
            for (++sit; sit != send; ++sit)
            {
                add_run(sit, traits::begin(sit), traits::end(sit));
            }
            add_run(sit, traits::begin(sit), traits::local(last));
        }

        // Segmented containers without placement metadata still use bounded,
        // concurrent namespace lookups. Task policies collect runs inside
        // their asynchronous operation, including this fallback phase.
        std::vector<std::size_t> unresolved;
        for (std::size_t i = 0; i != runs.size(); ++i)
        {
            if (runs[i].locality == hpx::invalid_id)
            {
                unresolved.push_back(i);
            }
        }
        for (std::size_t first_run = 0; first_run < unresolved.size();)
        {
            std::vector<hpx::future<hpx::id_type>> pending;
            auto const count = (std::min) (segmented_sort_transfer_limit,
                unresolved.size() - first_run);
            pending.reserve(count);
            std::exception_ptr error;
            try
            {
                for (std::size_t i = 0; i < count; ++i)
                {
                    pending.push_back(hpx::get_colocation_id(
                        runs[unresolved[first_run + i]].id));
                }
            }
            catch (...)
            {
                error = std::current_exception();
            }
            segmented_sort_wait<hpx::execution::sequenced_policy>(
                pending, error);
            for (std::size_t i = 0; i < count; ++i)
            {
                runs[unresolved[first_run + i]].locality = pending[i].get();
            }
            first_run += count;
        }
        return runs;
    }

    // Bound the payload of each remote transfer independently of the physical
    // partition size. Physical partitions remain the sorting units.
    inline constexpr std::size_t segmented_sort_transfer_chunk_size = 65536;

    template <typename LocalIter>
    using segmented_sort_block = std::vector<segmented_sort_run<LocalIter>>;

    template <typename LocalIter>
    std::size_t segmented_sort_block_size(
        segmented_sort_block<LocalIter> const& block)
    {
        std::size_t size = 0;
        for (auto const& run : block)
        {
            size += static_cast<std::size_t>(run.size());
        }
        return size;
    }

    template <typename LocalIter>
    auto segmented_sort_transfer_chunks(
        segmented_sort_block<LocalIter> const& block)
    {
        std::vector<segmented_sort_block<LocalIter>> chunks;
        for (auto const& run : block)
        {
            auto first = run.first;
            while (first != run.last)
            {
                auto const count =
                    (std::min) (segmented_sort_transfer_chunk_size,
                        static_cast<std::size_t>(run.last - first));
                auto last = first + static_cast<std::ptrdiff_t>(count);
                chunks.push_back({{run.id, run.locality, first, last}});
                first = last;
            }
        }
        return chunks;
    }

    // Choose the locality holding the most participating data so a compare-
    // split transfers as little input as possible.
    template <typename LocalIter>
    hpx::id_type segmented_sort_host(
        segmented_sort_block<LocalIter> const& left,
        segmented_sort_block<LocalIter> const& right)
    {
        std::map<hpx::id_type, std::size_t> sizes;
        for (auto const& run : left)
        {
            sizes[run.locality] += static_cast<std::size_t>(run.size());
        }
        for (auto const& run : right)
        {
            sizes[run.locality] += static_cast<std::size_t>(run.size());
        }
        auto host = sizes.begin();
        for (auto it = std::next(host); it != sizes.end(); ++it)
        {
            if (it->second > host->second)
            {
                host = it;
            }
        }
        return host->first;
    }

    template <typename LocalIter>
    struct segmented_fetch_values
      : algorithm<segmented_fetch_values<LocalIter>,
            std::vector<typename std::iterator_traits<LocalIter>::value_type>>
    {
        using value_type = std::iterator_traits<LocalIter>::value_type;

        constexpr segmented_fetch_values() noexcept
          : algorithm<segmented_fetch_values, std::vector<value_type>>(
                "segmented_fetch_values")
        {
        }

        template <typename ExPolicy>
        static std::vector<value_type> sequential(
            ExPolicy const&, segmented_sort_block<LocalIter> const& runs)
        {
            using traits =
                hpx::traits::segmented_local_iterator_traits<LocalIter>;
            std::vector<value_type> values;
            values.reserve(segmented_sort_block_size(runs));
            for (auto const& run : runs)
            {
                if constexpr (std::is_copy_constructible_v<value_type>)
                {
                    values.insert(values.end(), traits::local(run.first),
                        traits::local(run.last));
                }
                else
                {
                    values.insert(values.end(),
                        std::make_move_iterator(traits::local(run.first)),
                        std::make_move_iterator(traits::local(run.last)));
                }
            }
            return values;
        }

        template <typename ExPolicy>
        static std::vector<value_type> parallel(
            ExPolicy const& policy, segmented_sort_block<LocalIter> const& runs)
        {
            // A single block is copied serially. Concurrency between remote
            // blocks is provided by segmented_sort_fetch_block.
            return sequential(policy, runs);
        }
    };

    template <typename LocalIter>
    struct segmented_store_values
      : algorithm<segmented_store_values<LocalIter>, bool>
    {
        using value_type = std::iterator_traits<LocalIter>::value_type;

        constexpr segmented_store_values() noexcept
          : algorithm<segmented_store_values, bool>("segmented_store_values")
        {
        }

        template <typename ExPolicy>
        static bool sequential(ExPolicy const& policy,
            segmented_sort_block<LocalIter> const& runs,
            std::vector<value_type> values)
        {
            using traits =
                hpx::traits::segmented_local_iterator_traits<LocalIter>;
            auto first = values.begin();
            for (auto const& run : runs)
            {
                auto last = first + run.size();
                hpx::move(policy, first, last, traits::local(run.first));
                first = last;
            }
            return true;
        }

        template <typename ExPolicy>
        static bool parallel(ExPolicy const& policy,
            segmented_sort_block<LocalIter> const& runs,
            std::vector<value_type> values)
        {
            // The shared control path still passes the parallel policy to
            // hpx::move for every destination run.
            return sequential(policy, runs, HPX_MOVE(values));
        }
    };

    template <typename ExPolicy, typename LocalIter>
    auto segmented_sort_fetch_block(
        ExPolicy const& policy, segmented_sort_block<LocalIter> const& block)
    {
        using value_type = std::iterator_traits<LocalIter>::value_type;
        auto const here = hpx::find_here();
        if (std::all_of(block.begin(), block.end(),
                [&](auto const& run) { return run.locality == here; }))
        {
            return segmented_fetch_values<LocalIter>::sequential(policy, block);
        }
        std::vector<value_type> values;
        values.reserve(segmented_sort_block_size(block));
        auto chunks = segmented_sort_transfer_chunks(block);
        constexpr auto limit = hpx::is_sequenced_execution_policy_v<ExPolicy> ?
            1 :
            segmented_sort_transfer_limit;
        auto it = chunks.begin();
        while (it != chunks.end())
        {
            auto batch_end = it;
            for (std::size_t i = 0; i < limit && batch_end != chunks.end();
                ++i, ++batch_end)
            {
            }

            auto const count =
                static_cast<std::size_t>(std::distance(it, batch_end));
            std::vector<std::vector<value_type>> parts(count);
            std::vector<hpx::future<void>> pending;
            pending.reserve(limit);
            std::exception_ptr error;
            try
            {
                auto current = it;
                for (std::size_t i = 0; current != batch_end; ++i, ++current)
                {
                    auto pieces = *current;
                    auto const host = pieces.front().locality;
                    if (host != here)
                    {
                        pending.push_back(dispatch_async(host,
                            segmented_fetch_values<LocalIter>(), policy,
                            std::true_type(), HPX_MOVE(pieces))
                                .then([&parts, i](
                                          hpx::future<std::vector<value_type>>
                                              ready) {
                                    parts[i] = ready.get();
                                }));
                    }
                }

                current = it;
                for (std::size_t i = 0; current != batch_end; ++i, ++current)
                {
                    auto const& pieces = *current;
                    if (pieces.front().locality == here)
                    {
                        parts[i] =
                            segmented_fetch_values<LocalIter>::sequential(
                                policy, pieces);
                    }
                }
            }
            catch (...)
            {
                error = std::current_exception();
            }
            segmented_sort_wait<ExPolicy>(pending, error);
            for (auto& part : parts)
            {
                values.insert(values.end(),
                    std::make_move_iterator(part.begin()),
                    std::make_move_iterator(part.end()));
            }
            it = batch_end;
        }
        return values;
    }

    template <typename ExPolicy, typename LocalIter, typename Iter>
    Iter segmented_sort_store_block(ExPolicy const& policy,
        segmented_sort_block<LocalIter> const& block, Iter first)
    {
        using value_type = std::iterator_traits<LocalIter>::value_type;
        auto const here = hpx::find_here();
        auto chunks = segmented_sort_transfer_chunks(block);
        constexpr auto limit = hpx::is_sequenced_execution_policy_v<ExPolicy> ?
            1 :
            segmented_sort_transfer_limit;
        auto it = chunks.begin();
        while (it != chunks.end())
        {
            auto batch_end = it;
            for (std::size_t i = 0; i < limit && batch_end != chunks.end();
                ++i, ++batch_end)
            {
            }

            std::vector<hpx::future<bool>> pending;
            pending.reserve(limit);
            std::exception_ptr error;
            try
            {
                auto current = first;
                for (auto chunk = it; chunk != batch_end; ++chunk)
                {
                    auto pieces = *chunk;
                    auto const host = pieces.front().locality;
                    auto last = current +
                        static_cast<std::ptrdiff_t>(
                            segmented_sort_block_size(pieces));
                    if (host != here)
                    {
                        std::vector<value_type> values(
                            std::make_move_iterator(current),
                            std::make_move_iterator(last));
                        pending.push_back(dispatch_async(host,
                            segmented_store_values<LocalIter>(), policy,
                            std::true_type(), HPX_MOVE(pieces),
                            HPX_MOVE(values)));
                    }
                    current = last;
                }

                current = first;
                for (auto chunk = it; chunk != batch_end; ++chunk)
                {
                    auto const& pieces = *chunk;
                    auto last = current +
                        static_cast<std::ptrdiff_t>(
                            segmented_sort_block_size(pieces));
                    if (pieces.front().locality == here)
                    {
                        using traits =
                            hpx::traits::segmented_local_iterator_traits<
                                LocalIter>;
                        auto source = current;
                        for (auto const& piece : pieces)
                        {
                            auto next = source + piece.size();
                            hpx::move(policy, source, next,
                                traits::local(piece.first));
                            source = next;
                        }
                    }
                    current = last;
                }
                first = current;
            }
            catch (...)
            {
                error = std::current_exception();
            }
            segmented_sort_wait<ExPolicy>(pending, error);
            it = batch_end;
        }
        return first;
    }

    template <typename LocalIter>
    struct segmented_sort_local_runs_on_host
      : algorithm<segmented_sort_local_runs_on_host<LocalIter>, bool>
    {
        constexpr segmented_sort_local_runs_on_host() noexcept
          : algorithm<segmented_sort_local_runs_on_host, bool>(
                "segmented_sort_local_runs_on_host")
        {
        }

        template <typename ExPolicy, typename Comp, typename Proj>
        static bool sequential(ExPolicy const& policy,
            segmented_sort_block<LocalIter> const& runs, Comp comp, Proj proj)
        {
            for (auto const& run : runs)
            {
                segmented_local_sort<LocalIter>::sequential(
                    policy, run.first, run.last, comp, proj);
            }
            return true;
        }

        template <typename ExPolicy, typename Comp, typename Proj>
        static bool parallel(ExPolicy const& policy,
            segmented_sort_block<LocalIter> const& runs, Comp comp, Proj proj)
        {
            // The run loop is orchestration; each local sort receives the
            // parallel policy, and localities are overlapped by the caller.
            return sequential(policy, runs, HPX_MOVE(comp), HPX_MOVE(proj));
        }
    };

    // Send one operation to each participating locality. A parallel policy
    // overlaps localities, while each operation applies the caller's policy to
    // every complete physical partition on that locality.
    template <typename ExPolicy, typename LocalIter, typename Comp,
        typename Proj, typename IsSeq>
    void segmented_sort_local_runs(ExPolicy const& policy,
        segmented_sort_block<LocalIter> const& runs, Comp const& comp,
        Proj const& proj, IsSeq is_seq)
    {
        using operation = segmented_sort_local_runs_on_host<LocalIter>;
        std::map<hpx::id_type, segmented_sort_block<LocalIter>> groups;
        for (auto const& run : runs)
        {
            groups[run.locality].push_back(run);
        }

        auto const here = hpx::find_here();
        if constexpr (IsSeq::value)
        {
            for (auto const& [host, local_runs] : groups)
            {
                if (host == here)
                {
                    operation::sequential(policy, local_runs, comp, proj);
                }
                else
                {
                    dispatch(host, operation(), policy, is_seq, local_runs,
                        comp, proj);
                }
            }
        }
        else
        {
            std::vector<hpx::future<bool>> pending;
            pending.reserve(groups.size());
            std::exception_ptr error;
            try
            {
                for (auto const& [host, local_runs] : groups)
                {
                    if (host == here)
                    {
                        pending.push_back(hpx::async(
                            [policy, local_runs, comp, proj]() mutable {
                                return operation::parallel(policy, local_runs,
                                    HPX_MOVE(comp), HPX_MOVE(proj));
                            }));
                    }
                    else
                    {
                        pending.push_back(dispatch_async(host, operation(),
                            policy, is_seq, local_runs, comp, proj));
                    }
                }
            }
            catch (...)
            {
                error = std::current_exception();
            }
            segmented_sort_wait<ExPolicy>(pending, error);
        }
    }

    template <typename LocalIter>
    struct segmented_sort_compare_split_on_host
      : algorithm<segmented_sort_compare_split_on_host<LocalIter>, bool>
    {
        constexpr segmented_sort_compare_split_on_host() noexcept
          : algorithm<segmented_sort_compare_split_on_host, bool>(
                "segmented_sort_compare_split_on_host")
        {
        }

        /// Return a subrange of a physical sorted run.
        static segmented_sort_run<LocalIter> subrun(
            segmented_sort_run<LocalIter> const& run, std::size_t offset,
            std::size_t count)
        {
            auto const first = run.first + static_cast<std::ptrdiff_t>(offset);
            return {run.id, run.locality, first,
                first + static_cast<std::ptrdiff_t>(count)};
        }

        /// Compare projected values at two positions without gathering either
        /// complete run on the coordinating locality.
        template <typename ExPolicy, typename Comp, typename Proj>
        static bool less_at(ExPolicy const& policy,
            segmented_sort_run<LocalIter> const& lhs, std::size_t lhs_offset,
            segmented_sort_run<LocalIter> const& rhs, std::size_t rhs_offset,
            Comp& comp, Proj& proj)
        {
            segmented_sort_block<LocalIter> const values_to_fetch{
                subrun(lhs, lhs_offset, 1), subrun(rhs, rhs_offset, 1)};
            auto values = segmented_sort_fetch_block(policy, values_to_fetch);
            HPX_ASSERT(values.size() == 2);
            util::compare_projected<Comp&, Proj&> pred(comp, proj);
            return pred(values[0], values[1]);
        }

        /// Find the stable merge partition that leaves left.size() values in
        /// the left run. The two returned offsets delimit equally sized
        /// crossing ranges: left[i, end) and right[begin, j).
        ///
        /// \tparam ExPolicy The execution policy type.
        /// \tparam Comp The comparison function type.
        /// \tparam Proj The projection function type.
        /// \param policy The execution policy instance.
        /// \param left The sorted left run.
        /// \param right The sorted right run.
        /// \param comp The comparison function.
        /// \param proj The projection function.
        /// \return The left and right offsets delimiting the crossing ranges.
        template <typename ExPolicy, typename Comp, typename Proj>
        static std::pair<std::size_t, std::size_t> merge_partition(
            ExPolicy const& policy, segmented_sort_run<LocalIter> const& left,
            segmented_sort_run<LocalIter> const& right, Comp& comp, Proj& proj)
        {
            auto const left_size = static_cast<std::size_t>(left.size());
            auto const right_size = static_cast<std::size_t>(right.size());
            std::size_t low =
                left_size > right_size ? left_size - right_size : 0;
            std::size_t high = left_size;

            while (low <= high)
            {
                auto const left_cut = low + (high - low) / 2;
                auto const right_cut = left_size - left_cut;

                if (left_cut != 0 && right_cut != right_size &&
                    less_at(policy, right, right_cut, left, left_cut - 1, comp,
                        proj))
                {
                    high = left_cut - 1;
                }
                else if (right_cut != 0 && left_cut != left_size &&
                    less_at(policy, left, left_cut, right, right_cut - 1, comp,
                        proj))
                {
                    low = left_cut + 1;
                }
                else
                {
                    return {left_cut, right_cut};
                }
            }

            HPX_ASSERT(false);
            return {left_size, 0};
        }

        /// Exchange the crossing suffix and prefix in bounded chunks. At most
        /// two transfer chunks are materialized on the coordinator at once.
        ///
        /// \tparam ExPolicy The execution policy type.
        /// \param policy The execution policy instance.
        /// \param left The run containing the suffix to exchange.
        /// \param right The run containing the prefix to exchange.
        /// \param left_cut The offset of the crossing suffix in \a left.
        /// \param crossing_size The number of values exchanged in each
        ///        direction.
        template <typename ExPolicy>
        static void exchange_crossing_ranges(ExPolicy const& policy,
            segmented_sort_run<LocalIter> const& left,
            segmented_sort_run<LocalIter> const& right, std::size_t left_cut,
            std::size_t crossing_size)
        {
            for (std::size_t offset = 0; offset != crossing_size;)
            {
                auto const count =
                    (std::min) (segmented_sort_transfer_chunk_size,
                        crossing_size - offset);
                auto const left_chunk = subrun(left, left_cut + offset, count);
                auto const right_chunk = subrun(right, offset, count);
                segmented_sort_block<LocalIter> const chunks{
                    left_chunk, right_chunk};
                auto values = segmented_sort_fetch_block(policy, chunks);
                HPX_ASSERT(values.size() == 2 * count);

                segmented_sort_block<LocalIter> const left_destination{
                    left_chunk};
                segmented_sort_block<LocalIter> const right_destination{
                    right_chunk};
                auto const middle = values.begin() +
                    static_cast<typename decltype(values)::difference_type>(
                        count);
                [[maybe_unused]] auto const left_last =
                    segmented_sort_store_block(
                        policy, left_destination, middle);
                [[maybe_unused]] auto const right_last =
                    segmented_sort_store_block(
                        policy, right_destination, values.begin());
                HPX_ASSERT(left_last == values.end());
                HPX_ASSERT(right_last == middle);
                offset += count;
            }
        }

        /// Merge the two exchanged runs independently on their owning
        /// localities. Parallel policies overlap the two local merges.
        ///
        /// \tparam ExPolicy The execution policy type.
        /// \tparam Comp The comparison function type.
        /// \tparam Proj The projection function type.
        /// \tparam IsSeq A compile-time flag selecting sequenced dispatch.
        /// \param policy The execution policy instance.
        /// \param left The left run after exchanging its crossing suffix.
        /// \param right The right run after exchanging its crossing prefix.
        /// \param left_cut The merge boundary in \a left.
        /// \param right_cut The merge boundary in \a right.
        /// \param comp The comparison function.
        /// \param proj The projection function.
        /// \param is_seq The sequenced-dispatch flag.
        template <typename ExPolicy, typename Comp, typename Proj,
            typename IsSeq>
        static void merge_exchanged_runs(ExPolicy const& policy,
            segmented_sort_run<LocalIter> const& left,
            segmented_sort_run<LocalIter> const& right, std::size_t left_cut,
            std::size_t right_cut, Comp const& comp, Proj const& proj,
            IsSeq is_seq)
        {
            using operation = segmented_local_merge<LocalIter>;
            auto const here = hpx::find_here();
            std::vector<hpx::future<LocalIter>> pending;
            std::exception_ptr error;
            auto merge_run = [&](segmented_sort_run<LocalIter> const& run,
                                 std::size_t middle_offset) {
                if (middle_offset == 0 ||
                    middle_offset == static_cast<std::size_t>(run.size()))
                {
                    return;
                }

                auto const middle =
                    run.first + static_cast<std::ptrdiff_t>(middle_offset);
                if constexpr (IsSeq::value)
                {
                    if (run.locality == here)
                    {
                        operation::sequential(
                            policy, run.first, middle, run.last, comp, proj);
                    }
                    else
                    {
                        dispatch(run.locality, operation(), policy, is_seq,
                            run.first, middle, run.last, comp, proj);
                    }
                }
                else if (run.locality == here)
                {
                    pending.push_back(
                        hpx::async([policy, first = run.first, middle,
                                       last = run.last, comp, proj]() mutable {
                            return operation::parallel(policy, first, middle,
                                last, HPX_MOVE(comp), HPX_MOVE(proj));
                        }));
                }
                else
                {
                    pending.push_back(
                        dispatch_async(run.locality, operation(), policy,
                            is_seq, run.first, middle, run.last, comp, proj));
                }
            };

            try
            {
                merge_run(left, left_cut);
                merge_run(right, right_cut);
            }
            catch (...)
            {
                error = std::current_exception();
            }
            segmented_sort_wait<ExPolicy>(pending, error);
        }

        /// Perform a bounded merge-split of two adjacent sorted runs.
        ///
        /// \tparam ExPolicy The execution policy used for transfers and local
        ///         merges.
        /// \tparam Comp The comparison function type.
        /// \tparam Proj The projection function type.
        /// \param policy The execution policy instance.
        /// \param left The sorted run that retains the lower values.
        /// \param right The sorted run that retains the higher values.
        /// \param comp The comparison function.
        /// \param proj The projection function.
        /// \return True if values crossed the run boundary; otherwise false.
        template <typename ExPolicy, typename Comp, typename Proj>
        static bool execute(ExPolicy const& policy,
            segmented_sort_run<LocalIter> const& left,
            segmented_sort_run<LocalIter> const& right, Comp comp, Proj proj)
        {
            auto const left_size = static_cast<std::size_t>(left.size());
            if (!less_at(policy, right, 0, left, left_size - 1, comp, proj))
            {
                return false;
            }

            auto const [left_cut, right_cut] =
                merge_partition(policy, left, right, comp, proj);
            auto const crossing_size = right_cut;
            HPX_ASSERT(crossing_size == left_size - left_cut);
            HPX_ASSERT(crossing_size != 0);

            exchange_crossing_ranges(
                policy, left, right, left_cut, crossing_size);
            merge_exchanged_runs(policy, left, right, left_cut, right_cut, comp,
                proj, hpx::is_sequenced_execution_policy<ExPolicy>{});
            return true;
        }

        /// Sequenced dispatch entry point for a compare-split.
        template <typename ExPolicy, typename Comp, typename Proj>
        static bool sequential(ExPolicy const& policy,
            segmented_sort_run<LocalIter> const& left,
            segmented_sort_run<LocalIter> const& right, Comp comp, Proj proj)
        {
            return execute(policy, left, right, HPX_MOVE(comp), HPX_MOVE(proj));
        }

        /// Parallel dispatch entry point for a compare-split.
        template <typename ExPolicy, typename Comp, typename Proj>
        static bool parallel(ExPolicy const& policy,
            segmented_sort_run<LocalIter> const& left,
            segmented_sort_run<LocalIter> const& right, Comp comp, Proj proj)
        {
            // Disjoint pairs are overlapped by segmented_sort_merge_runs; the
            // policy is also used by the local merge and stores.
            return execute(policy, left, right, HPX_MOVE(comp), HPX_MOVE(proj));
        }
    };

    /// Run odd-even compare-split phases while keeping data distributed. Each
    /// operation exchanges only the crossing suffix and prefix using bounded
    /// buffers, then merges both runs on their owning localities. Alternating
    /// phases continue until both parities report no changes. Parallel policies
    /// overlap the disjoint pairs in each phase, although values can move in
    /// multiple phases.
    ///
    /// \tparam ExPolicy The execution policy type.
    /// \tparam LocalIter The local iterator type of each run.
    /// \tparam Comp The comparison function type.
    /// \tparam Proj The projection function type.
    /// \tparam IsSeq A compile-time flag selecting sequenced dispatch.
    /// \param policy The execution policy instance.
    /// \param runs The sorted distributed runs to merge globally.
    /// \param comp The comparison function.
    /// \param proj The projection function.
    /// \param is_seq The sequenced-dispatch flag.
    template <typename ExPolicy, typename LocalIter, typename Comp,
        typename Proj, typename IsSeq>
    void segmented_sort_merge_runs(ExPolicy const& policy,
        segmented_sort_block<LocalIter> const& runs, Comp const& comp,
        Proj const& proj, IsSeq is_seq)
    {
        if (runs.size() <= 1)
        {
            return;
        }

        using operation = segmented_sort_compare_split_on_host<LocalIter>;
        auto const here = hpx::find_here();
        std::size_t phase = 0;
        std::size_t unchanged_phases = 0;
        while (unchanged_phases != 2)
        {
            bool phase_changed = false;
            std::vector<hpx::future<bool>> pending;
            pending.reserve(runs.size() / 2);
            std::exception_ptr error;
            try
            {
                for (std::size_t i = phase % 2; i + 1 < runs.size(); i += 2)
                {
                    segmented_sort_block<LocalIter> const left{runs[i]};
                    segmented_sort_block<LocalIter> const right{runs[i + 1]};
                    auto const host = segmented_sort_host(left, right);
                    if constexpr (IsSeq::value)
                    {
                        if (host == here)
                        {
                            phase_changed =
                                operation().call2(policy, is_seq, runs[i],
                                    runs[i + 1], comp, proj) ||
                                phase_changed;
                        }
                        else
                        {
                            phase_changed =
                                dispatch(host, operation(), policy, is_seq,
                                    runs[i], runs[i + 1], comp, proj) ||
                                phase_changed;
                        }
                    }
                    else if (host == here)
                    {
                        auto left_run = runs[i];
                        auto right_run = runs[i + 1];
                        pending.push_back(
                            hpx::async([policy, left_run, right_run, comp,
                                           proj]() mutable {
                                return operation::parallel(policy, left_run,
                                    right_run, HPX_MOVE(comp), HPX_MOVE(proj));
                            }));
                    }
                    else
                    {
                        pending.push_back(dispatch_async(host, operation(),
                            policy, is_seq, runs[i], runs[i + 1], comp, proj));
                    }
                }
            }
            catch (...)
            {
                error = std::current_exception();
            }
            segmented_sort_wait<ExPolicy>(pending, error);
            for (auto& result : pending)
            {
                phase_changed = result.get() || phase_changed;
            }
            unchanged_phases = phase_changed ? 0 : unchanged_phases + 1;
            ++phase;
        }
    }

    template <typename ExPolicy, typename LocalIter, typename Comp,
        typename Proj, typename IsSeq>
    void segmented_sort_distributed(ExPolicy const& policy,
        segmented_sort_block<LocalIter> const& runs, Comp const& comp,
        Proj const& proj, IsSeq is_seq)
    {
        segmented_sort_local_runs(policy, runs, comp, proj, is_seq);
        segmented_sort_merge_runs(policy, runs, comp, proj, is_seq);
    }

    template <typename ExPolicy, typename SegIter, typename Comp, typename Proj,
        typename IsSeq>
    util::detail::algorithm_result_t<ExPolicy> segmented_sort(
        ExPolicy const& policy, SegIter first, SegIter last, Comp&& comp,
        Proj&& proj, IsSeq is_seq)
    {
        using result = util::detail::algorithm_result<ExPolicy>;

        if (first == last)
        {
            return result::get();
        }

        std::decay_t<Comp> cmp(HPX_FORWARD(Comp, comp));
        std::decay_t<Proj> prj(HPX_FORWARD(Proj, proj));

        // Drop the task bit so nested hpx::sort/copy/merge stay synchronous
        // while still honouring the caller's parallel/sequenced choice.
        auto const sync_policy =
            hpx::execution::experimental::to_non_task(policy);

        if constexpr (hpx::is_async_execution_policy_v<ExPolicy>)
        {
            return result::get(hpx::async([=]() mutable {
                auto runs = segmented_sort_runs(first, last);
                if (!runs.empty())
                {
                    segmented_sort_distributed(
                        sync_policy, runs, cmp, prj, is_seq);
                }
            }));
        }
        else
        {
            auto runs = segmented_sort_runs(first, last);
            if (!runs.empty())
            {
                segmented_sort_distributed(sync_policy, runs, cmp, prj, is_seq);
            }
            return result::get();
        }
    }

    /// \endcond
}    // namespace hpx::parallel::detail

namespace hpx::segmented {

    /// \brief Segmented overload of \a hpx::sort for
    ///        \a partitioned_vector iterators.
    ///
    /// A sequenced policy processes participating localities one at a time.
    /// A parallel policy overlaps independent locality operations. For ranges
    /// spanning localities, the comparator must be copyable and serializable
    /// by HPX.
    ///
    /// The final merge uses alternating odd-even compare-split phases until
    /// both parities are unchanged. Each operation finds the merge partition,
    /// exchanges only the crossing suffix and prefix in bounded chunks, and
    /// merges both runs on their owning localities. No locality gathers the
    /// full input, although a value can be transferred in multiple phases. An
    /// exception after a compare-split stores data can leave a partially
    /// reordered range.
    HPX_CXX_EXPORT template <typename SegIter,
        typename Comp = hpx::parallel::detail::less>
        requires(hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter>)
    void hpx_invoke(
        hpx::sort_t, SegIter first, SegIter last, Comp&& comp = Comp())
    {
        static_assert(std::random_access_iterator<SegIter>,
            "Requires a random access iterator.");

        if (first == last)
        {
            return;
        }

        hpx::parallel::detail::segmented_sort(hpx::execution::seq, first, last,
            HPX_FORWARD(Comp, comp), hpx::identity_v, std::true_type{});
    }

    /// \brief Segmented overload of \a hpx::sort with an execution policy.
    ///
    /// For ranges spanning localities, the comparator must be copyable and
    /// serializable by HPX.
    ///
    /// The policy controls local partition sorting and whether independent
    /// compare-splits overlap. The final merge uses alternating odd-even phases
    /// until both parities are unchanged. Each operation finds the merge
    /// partition, exchanges only the crossing suffix and prefix in bounded
    /// chunks, and merges both runs on their owning localities. No locality
    /// gathers the full input, although a value can be transferred in multiple
    /// phases. An exception after a compare-split stores data can leave a
    /// partially reordered range.
    HPX_CXX_EXPORT template <typename ExPolicy, typename SegIter,
        typename Comp = hpx::parallel::detail::less>
        requires(hpx::is_execution_policy_v<ExPolicy> &&
            hpx::traits::is_iterator_v<SegIter> &&
            hpx::traits::is_segmented_iterator_v<SegIter>)
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy> hpx_invoke(
        hpx::sort_t, ExPolicy&& policy, SegIter first, SegIter last,
        Comp&& comp = Comp())
    {
        static_assert(std::random_access_iterator<SegIter>,
            "Requires a random access iterator.");

        using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;

        return hpx::parallel::detail::segmented_sort(
            HPX_FORWARD(ExPolicy, policy), first, last, HPX_FORWARD(Comp, comp),
            hpx::identity_v, is_seq());
    }
}    // namespace hpx::segmented
