//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
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
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <optional>
#include <set>
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
            return sequential(HPX_FORWARD(ExPolicy, policy), first, last,
                HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
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
                runs.emplace_back(HPX_MOVE(id), hpx::id_type{}, beg, end);
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

        // Resolve independent namespace entries concurrently, with a bounded
        // number of outstanding requests. Task policies collect runs inside
        // their asynchronous operation, including this discovery phase.
        for (std::size_t first_run = 0; first_run < runs.size();)
        {
            std::vector<hpx::future<hpx::id_type>> pending;
            auto const count = (std::min) (segmented_sort_transfer_limit,
                runs.size() - first_run);
            pending.reserve(count);
            std::exception_ptr error;
            try
            {
                for (std::size_t i = 0; i < count; ++i)
                {
                    pending.push_back(
                        hpx::get_colocation_id(runs[first_run + i].id));
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
                runs[first_run + i].locality = pending[i].get();
            }
            first_run += count;
        }
        return runs;
    }

    // Logical blocks have equal sizes except for the final block. They may
    // span physical partitions, including clipped subrange endpoints.
    inline constexpr std::size_t segmented_sort_max_block_size = 65536;

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

    // Multiple physical partitions on one locality should not increase the
    // network depth. The cap bounds buffers when N / localities is large.
    template <typename LocalIter>
    std::size_t segmented_sort_block_capacity(
        segmented_sort_block<LocalIter> const& runs)
    {
        auto const total = segmented_sort_block_size(runs);
        // A single bounded block avoids network stages for small ranges.
        if (total <= segmented_sort_max_block_size)
        {
            return total;
        }
        std::set<hpx::id_type> localities;
        for (auto const& run : runs)
        {
            localities.insert(run.locality);
        }
        auto const count = localities.size();
        return (std::min) (segmented_sort_max_block_size,
            total / count + (total % count != 0));
    }

    template <typename LocalIter>
    auto segmented_sort_blocks(
        segmented_sort_block<LocalIter> const& runs, std::size_t block_size)
    {
        std::vector<segmented_sort_block<LocalIter>> blocks;
        std::size_t remaining = 0;
        for (auto const& run : runs)
        {
            auto first = run.first;
            while (first != run.last)
            {
                if (remaining == 0)
                {
                    blocks.emplace_back();
                    remaining = block_size;
                }
                auto const count = (std::min) (remaining,
                    static_cast<std::size_t>(run.last - first));
                auto last = first + static_cast<std::ptrdiff_t>(count);
                blocks.back().emplace_back(run.id, run.locality, first, last);
                first = last;
                remaining -= count;
            }
        }
        return blocks;
    }

    // Choose the locality holding the most input for this operation, summing
    // all participating pieces rather than selecting a single partition.
    // Balance equally good hosts across the current stage.
    template <typename LocalIter>
    hpx::id_type segmented_sort_host(
        segmented_sort_block<LocalIter> const& left,
        segmented_sort_block<LocalIter> const& right,
        std::map<hpx::id_type, std::size_t> const& assignments)
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
        auto assigned = [&](hpx::id_type const& locality) {
            auto const it = assignments.find(locality);
            return it == assignments.end() ? std::size_t(0) : it->second;
        };
        auto host = sizes.begin();
        for (auto it = std::next(host); it != sizes.end(); ++it)
        {
            if (it->second > host->second ||
                (it->second == host->second &&
                    assigned(it->first) < assigned(host->first)))
            {
                host = it;
            }
        }
        return host->first;
    }

    template <typename LocalIter>
    hpx::id_type segmented_sort_host(
        segmented_sort_block<LocalIter> const& left,
        segmented_sort_block<LocalIter> const& right)
    {
        return segmented_sort_host(
            left, right, std::map<hpx::id_type, std::size_t>{});
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
                values.insert(values.end(), traits::local(run.first),
                    traits::local(run.last));
            }
            return values;
        }

        template <typename ExPolicy>
        static std::vector<value_type> parallel(
            ExPolicy const& policy, segmented_sort_block<LocalIter> const& runs)
        {
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
        constexpr auto limit = hpx::is_sequenced_execution_policy_v<ExPolicy> ?
            1 :
            segmented_sort_transfer_limit;
        auto it = block.begin();
        while (it != block.end())
        {
            std::vector<hpx::future<std::vector<value_type>>> pending;
            pending.reserve(limit);
            std::vector<std::optional<std::vector<value_type>>> parts(limit);
            std::vector<std::size_t> indices;
            indices.reserve(limit);
            std::exception_ptr error;
            try
            {
                for (std::size_t i = 0; i < limit && it != block.end(); ++i)
                {
                    auto end =
                        std::find_if(it, block.end(), [&](auto const& run) {
                            return run.locality != it->locality;
                        });
                    segmented_sort_block<LocalIter> pieces(it, end);
                    if (it->locality == here)
                    {
                        pending.push_back(hpx::make_ready_future(
                            segmented_fetch_values<LocalIter>::sequential(
                                policy, pieces)));
                    }
                    else
                    {
                        pending.push_back(dispatch_async(it->locality,
                            segmented_fetch_values<LocalIter>(), policy,
                            std::true_type(), HPX_MOVE(pieces)));
                    }
                    indices.push_back(indices.size());
                    it = end;
                }
            }
            catch (...)
            {
                error = std::current_exception();
            }
            if (error)
            {
                segmented_sort_wait<ExPolicy>(pending, error);
            }

            // Consume completed transfers eagerly. Out-of-order results wait
            // only for their preceding logical pieces. Exact reservation of
            // values prevents repeated reallocation while pieces are appended.
            auto const part_count = pending.size();
            std::size_t next_part = 0;
            while (!pending.empty())
            {
                if (hpx::wait_any_nothrow(pending))
                {
                    segmented_sort_wait<ExPolicy>(pending);
                }
                try
                {
                    for (std::size_t i = pending.size(); i-- != 0;)
                    {
                        if (pending[i].is_ready())
                        {
                            parts[indices[i]].emplace(pending[i].get());
                            pending.erase(pending.begin() +
                                static_cast<std::ptrdiff_t>(i));
                            indices.erase(indices.begin() +
                                static_cast<std::ptrdiff_t>(i));
                        }
                    }
                    while (next_part != part_count && parts[next_part])
                    {
                        auto& part = *parts[next_part];
                        values.insert(values.end(),
                            std::make_move_iterator(part.begin()),
                            std::make_move_iterator(part.end()));
                        ++next_part;
                    }
                }
                catch (...)
                {
                    segmented_sort_wait<ExPolicy>(
                        pending, std::current_exception());
                }
            }
        }
        return values;
    }

    template <typename ExPolicy, typename LocalIter>
    auto segmented_sort_fetch_pair(ExPolicy const& policy,
        segmented_sort_block<LocalIter> const& left,
        segmented_sort_block<LocalIter> const& right)
    {
        using value_type = std::iterator_traits<LocalIter>::value_type;
        std::vector<hpx::future<std::vector<value_type>>> pending;
        pending.reserve(2);
        std::exception_ptr error;
        try
        {
            pending.push_back(hpx::async([policy, left]() {
                return segmented_sort_fetch_block(policy, left);
            }));
            pending.push_back(hpx::async([policy, right]() {
                return segmented_sort_fetch_block(policy, right);
            }));
        }
        catch (...)
        {
            error = std::current_exception();
        }
        segmented_sort_wait<ExPolicy>(pending, error);
        return std::pair{pending[0].get(), pending[1].get()};
    }

    template <typename ExPolicy, typename LocalIter, typename Iter>
    Iter segmented_sort_store_block(ExPolicy const& policy,
        segmented_sort_block<LocalIter> const& block, Iter first)
    {
        using value_type = std::iterator_traits<LocalIter>::value_type;
        auto const here = hpx::find_here();
        constexpr auto limit = hpx::is_sequenced_execution_policy_v<ExPolicy> ?
            1 :
            segmented_sort_transfer_limit;
        auto it = block.begin();
        while (it != block.end())
        {
            std::vector<hpx::future<bool>> pending;
            pending.reserve(limit);
            std::exception_ptr error;
            try
            {
                for (std::size_t i = 0; i < limit && it != block.end(); ++i)
                {
                    auto end =
                        std::find_if(it, block.end(), [&](auto const& run) {
                            return run.locality != it->locality;
                        });
                    segmented_sort_block<LocalIter> pieces(it, end);
                    auto last = first +
                        static_cast<std::ptrdiff_t>(
                            segmented_sort_block_size(pieces));
                    if (it->locality == here)
                    {
                        using traits =
                            hpx::traits::segmented_local_iterator_traits<
                                LocalIter>;
                        auto current = first;
                        for (auto const& piece : pieces)
                        {
                            auto next = current + piece.size();
                            hpx::move(policy, current, next,
                                traits::local(piece.first));
                            current = next;
                        }
                    }
                    else
                    {
                        std::vector<value_type> values(
                            std::make_move_iterator(first),
                            std::make_move_iterator(last));
                        pending.push_back(dispatch_async(it->locality,
                            segmented_store_values<LocalIter>(), policy,
                            std::true_type(), HPX_MOVE(pieces),
                            HPX_MOVE(values)));
                    }
                    first = last;
                    it = end;
                }
            }
            catch (...)
            {
                error = std::current_exception();
            }
            segmented_sort_wait<ExPolicy>(pending, error);
        }
        return first;
    }

    // Merge directly into the destination. There is no serial copy of the
    // merged output into a second vector.
    template <typename ExPolicy, typename T, typename Pred>
    std::vector<T> segmented_sort_merge_pair(ExPolicy const& policy,
        std::vector<T> const& left, std::vector<T> const& right, Pred pred)
    {
        std::vector<T> merged(left.size() + right.size());
        hpx::merge(policy, left.begin(), left.end(), right.begin(), right.end(),
            merged.begin(), pred);
        return merged;
    }

    template <typename LocalIter>
    struct segmented_sort_compare_split
      : algorithm<segmented_sort_compare_split<LocalIter>, bool>
    {
        constexpr segmented_sort_compare_split() noexcept
          : algorithm<segmented_sort_compare_split, bool>(
                "segmented_sort_compare_split")
        {
        }

        template <typename ExPolicy, typename Comp, typename Proj,
            typename IsParallel>
        static bool execute(ExPolicy const& policy,
            segmented_sort_block<LocalIter> const& left,
            segmented_sort_block<LocalIter> const& right, Comp comp, Proj proj,
            IsParallel)
        {
            // Sorting one local piece needs no communication or copy buffer.
            if (right.empty() && left.size() == 1)
            {
                segmented_local_sort<LocalIter>::sequential(
                    policy, left[0].first, left[0].last, comp, proj);
                return true;
            }

            using value_type = std::iterator_traits<LocalIter>::value_type;
            util::compare_projected<Comp&, Proj&> pred(comp, proj);
            if (!right.empty())
            {
                auto left_end = left.back();
                left_end.first = left_end.last - 1;
                auto right_begin = right.front();
                right_begin.last = right_begin.first + 1;
                auto endpoints = segmented_sort_fetch_block(policy,
                    segmented_sort_block<LocalIter>{left_end, right_begin});
                if (!pred(endpoints[1], endpoints[0]))
                {
                    return true;
                }
            }
            std::vector<value_type> output;
            {
                if (right.empty())
                {
                    auto left_values = segmented_sort_fetch_block(policy, left);
                    hpx::sort(policy, left_values.begin(), left_values.end(),
                        std::ref(pred));
                    output = HPX_MOVE(left_values);
                }
                else
                {
                    if constexpr (IsParallel::value)
                    {
                        auto values =
                            segmented_sort_fetch_pair(policy, left, right);
                        output = segmented_sort_merge_pair(policy, values.first,
                            values.second, std::ref(pred));
                    }
                    else
                    {
                        auto left_values =
                            segmented_sort_fetch_block(policy, left);
                        auto right_values =
                            segmented_sort_fetch_block(policy, right);
                        output = segmented_sort_merge_pair(
                            policy, left_values, right_values, std::ref(pred));
                    }
                }
            }

            // Both inputs have been read completely before either is changed.
            // Release the input buffers before allocating outbound pieces.
            auto middle =
                segmented_sort_store_block(policy, left, output.begin());
            segmented_sort_store_block(policy, right, middle);
            return true;
        }

        template <typename ExPolicy, typename Comp, typename Proj>
        static bool sequential(ExPolicy const& policy,
            segmented_sort_block<LocalIter> const& left,
            segmented_sort_block<LocalIter> const& right, Comp comp, Proj proj)
        {
            return execute(policy, left, right, HPX_MOVE(comp), HPX_MOVE(proj),
                std::false_type{});
        }

        template <typename ExPolicy, typename Comp, typename Proj>
        static bool parallel(ExPolicy const& policy,
            segmented_sort_block<LocalIter> const& left,
            segmented_sort_block<LocalIter> const& right, Comp comp, Proj proj)
        {
            return execute(policy, left, right, HPX_MOVE(comp), HPX_MOVE(proj),
                std::true_type{});
        }
    };

    using segmented_sort_pair = std::pair<std::size_t, std::size_t>;

    struct segmented_sort_worker_count
      : algorithm<segmented_sort_worker_count, std::size_t>
    {
        constexpr segmented_sort_worker_count() noexcept
          : algorithm<segmented_sort_worker_count, std::size_t>(
                "segmented_sort_worker_count")
        {
        }

        template <typename ExPolicy>
        static std::size_t sequential(ExPolicy const&)
        {
            return (std::max) (std::size_t(1),
                static_cast<std::size_t>(hpx::get_os_thread_count()));
        }

        template <typename ExPolicy>
        static std::size_t parallel(ExPolicy const& policy)
        {
            return sequential(policy);
        }
    };

    // Batcher odd-even merge stages. With K blocks, O(log^2 K) stages trade
    // additional communication rounds for bounded merge buffers. All
    // comparisons are ascending, so a short final block and absent blocks
    // behave as trailing infinities without constructing sentinel values or
    // requiring a power-of-two size.
    template <typename F>
    void segmented_sort_merge_stages(std::size_t count, F stage)
    {
        std::vector<segmented_sort_pair> pairs;
        pairs.reserve(count / 2);
        for (std::size_t width = 1; width < count; width *= 2)
        {
            for (std::size_t stride = width; stride != 0; stride /= 2)
            {
                pairs.clear();
                for (std::size_t start = stride % width; start + stride < count;
                    start += 2 * stride)
                {
                    for (std::size_t i = 0;
                        i < stride && start + i + stride < count; ++i)
                    {
                        auto const left = start + i;
                        auto const right = left + stride;
                        if (left / (2 * width) == right / (2 * width))
                        {
                            pairs.emplace_back(left, right);
                        }
                    }
                }
                stage(pairs);
            }
        }
    }

    template <typename ExPolicy, typename LocalIter, typename Comp,
        typename Proj, typename IsSeq>
    void segmented_sort_stage(ExPolicy const& policy,
        std::vector<segmented_sort_block<LocalIter>> const& blocks,
        std::vector<segmented_sort_pair> const& pairs, Comp const& comp,
        Proj const& proj, IsSeq is_seq)
    {
        using operation = segmented_sort_compare_split<LocalIter>;
        segmented_sort_block<LocalIter> const empty;
        auto const here = hpx::find_here();
        auto right_block = [&](segmented_sort_pair pair) -> auto const& {
            return pair.first == pair.second ? empty : blocks[pair.second];
        };

        if constexpr (IsSeq::value)
        {
            for (auto pair : pairs)
            {
                auto const& left = blocks[pair.first];
                auto const& right = right_block(pair);
                auto const host = segmented_sort_host(left, right);
                if (host == here)
                {
                    operation().call2(policy, is_seq, left, right, comp, proj);
                }
                else
                {
                    dispatch(host, operation(), policy, is_seq, left, right,
                        comp, proj);
                }
            }
        }
        else
        {
            std::map<hpx::id_type, std::vector<segmented_sort_pair>> jobs;
            std::map<hpx::id_type, std::size_t> assignments;
            for (auto pair : pairs)
            {
                auto host = segmented_sort_host(
                    blocks[pair.first], right_block(pair), assignments);
                jobs[host].push_back(pair);
                ++assignments[host];
            }

            std::map<hpx::id_type, std::size_t> worker_counts;
            bool const initial_stage = std::all_of(pairs.begin(), pairs.end(),
                [](auto pair) { return pair.first == pair.second; });
            if (initial_stage)
            {
                std::vector<hpx::id_type> remote_hosts;
                std::vector<hpx::future<std::size_t>> pending;
                std::exception_ptr error;
                try
                {
                    for (auto const& job : jobs)
                    {
                        auto const& host = job.first;
                        if (host == here)
                        {
                            worker_counts[host] =
                                segmented_sort_worker_count::sequential(policy);
                        }
                        else
                        {
                            remote_hosts.push_back(host);
                            pending.push_back(dispatch_async(host,
                                segmented_sort_worker_count(), policy,
                                std::true_type()));
                        }
                    }
                }
                catch (...)
                {
                    error = std::current_exception();
                }
                segmented_sort_wait<ExPolicy>(pending, error);
                for (std::size_t i = 0; i != pending.size(); ++i)
                {
                    worker_counts[remote_hosts[i]] = pending[i].get();
                }
            }

            // Initial sorts can share a locality: buffered sorts charge two
            // blocks each, and in-place sorts need no value buffers. Bound
            // scratch storage by the same four-block budget as one merge.
            // Destination schedulers control worker use. Merge stages still
            // run at most one compare/split per locality.
            for (;;)
            {
                std::vector<hpx::future<bool>> pending;
                pending.reserve(pairs.size());
                std::exception_ptr error;
                try
                {
                    for (auto& [host, queue] : jobs)
                    {
                        std::size_t used = 0;
                        auto const workers = initial_stage ?
                            worker_counts[host] :
                            std::size_t(1);
                        for (std::size_t active = 0;
                            active != workers && !queue.empty(); ++active)
                        {
                            auto const pair = queue.back();
                            auto const& left = blocks[pair.first];
                            auto const& right = right_block(pair);
                            auto const cost = right.empty() ?
                                (left.size() == 1 ?
                                        0 :
                                        2 * segmented_sort_block_size(left)) :
                                4 * segmented_sort_max_block_size;
                            if (used + cost > 4 * segmented_sort_max_block_size)
                            {
                                break;
                            }
                            used += cost;
                            queue.pop_back();
                            if (host == here)
                            {
                                pending.push_back(
                                    hpx::async([policy, left, right, comp, proj,
                                                   is_seq]() {
                                        return operation().call2(policy, is_seq,
                                            left, right, comp, proj);
                                    }));
                            }
                            else
                            {
                                pending.push_back(
                                    dispatch_async(host, operation(), policy,
                                        is_seq, left, right, comp, proj));
                            }
                            if (!right.empty())
                            {
                                break;
                            }
                        }
                    }
                }
                catch (...)
                {
                    error = std::current_exception();
                }
                segmented_sort_wait<ExPolicy>(pending, error);
                if (pending.empty())
                {
                    break;
                }
            }
        }
    }

    template <typename ExPolicy, typename LocalIter, typename Comp,
        typename Proj, typename IsSeq>
    void segmented_sort_distributed(ExPolicy const& policy,
        segmented_sort_block<LocalIter> const& runs, Comp const& comp,
        Proj const& proj, IsSeq is_seq)
    {
        // Keep the single-partition path in place, regardless of its size.
        if (runs.size() == 1)
        {
            segmented_sort_stage(policy,
                std::vector<segmented_sort_block<LocalIter>>{runs}, {{0, 0}},
                comp, proj, is_seq);
            return;
        }
        auto const block_size = segmented_sort_block_capacity(runs);
        auto blocks = segmented_sort_blocks(runs, block_size);
        std::vector<segmented_sort_pair> initial;
        initial.reserve(blocks.size());
        for (std::size_t i = 0; i != blocks.size(); ++i)
        {
            initial.emplace_back(i, i);
        }
        segmented_sort_stage(policy, blocks, initial, comp, proj, is_seq);
        segmented_sort_merge_stages(blocks.size(), [&](auto const& pairs) {
            segmented_sort_stage(policy, blocks, pairs, comp, proj, is_seq);
        });
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
