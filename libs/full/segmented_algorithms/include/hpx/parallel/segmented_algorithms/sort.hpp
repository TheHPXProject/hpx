//  Copyright (c) 2026 the-ivii
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
#include <queue>
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

    // Choose the locality holding the most participating data so the k-way
    // merge transfers as little input as possible.
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

    // Merge the sorted input runs without constructing a second full-size
    // output buffer. Values are selected once into one destination buffer at
    // a time. Stores are incremental and therefore cannot be rolled back if a
    // later comparison or destination operation throws.
    template <typename T, typename Pred, typename Emit>
    void segmented_sort_kway_for_each(std::vector<T>& values,
        std::vector<std::size_t> const& offsets,
        std::vector<std::size_t> const& counts, Pred& pred, Emit&& emit)
    {
        struct cursor
        {
            std::size_t pos;
            std::size_t end;
        };

        auto order = [&](cursor const& lhs, cursor const& rhs) {
            return pred(values[rhs.pos], values[lhs.pos]);
        };
        std::priority_queue<cursor, std::vector<cursor>, decltype(order)> heap(
            order);

        for (std::size_t i = 0; i != counts.size(); ++i)
        {
            if (counts[i] != 0)
            {
                heap.push(cursor{offsets[i], offsets[i] + counts[i]});
            }
        }

        while (!heap.empty())
        {
            auto current = heap.top();
            heap.pop();
            emit(HPX_MOVE(values[current.pos]));
            if (++current.pos != current.end)
            {
                heap.push(current);
            }
        }
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
    struct segmented_sort_kway_merge_on_host
      : algorithm<segmented_sort_kway_merge_on_host<LocalIter>, bool>
    {
        constexpr segmented_sort_kway_merge_on_host() noexcept
          : algorithm<segmented_sort_kway_merge_on_host, bool>(
                "segmented_sort_kway_merge_on_host")
        {
        }

        template <typename ExPolicy, typename Comp, typename Proj>
        static bool execute(ExPolicy const& policy,
            segmented_sort_block<LocalIter> const& runs, Comp comp, Proj proj)
        {
            using value_type = std::iterator_traits<LocalIter>::value_type;

            std::vector<std::size_t> counts;
            std::vector<std::size_t> offsets;
            counts.reserve(runs.size());
            offsets.reserve(runs.size());
            std::size_t total = 0;
            for (auto const& run : runs)
            {
                offsets.push_back(total);
                auto const count = static_cast<std::size_t>(run.size());
                counts.push_back(count);
                total += count;
            }

            auto values = segmented_sort_fetch_block(policy, runs);
            HPX_ASSERT(values.size() == total);

            util::compare_projected<Comp&, Proj&> pred(comp, proj);
            std::size_t destination = 0;
            std::vector<value_type> output;
            output.reserve(counts.front());
            auto emit = [&](value_type&& value) {
                output.push_back(HPX_MOVE(value));
                if (output.size() != counts[destination])
                {
                    return;
                }

                segmented_sort_block<LocalIter> block{runs[destination]};
                [[maybe_unused]] auto const last =
                    segmented_sort_store_block(policy, block, output.begin());
                HPX_ASSERT(last == output.end());
                output.clear();
                if (++destination != runs.size())
                {
                    output.reserve(counts[destination]);
                }
            };

            segmented_sort_kway_for_each(values, offsets, counts, pred, emit);
            HPX_ASSERT(destination == runs.size());
            return true;
        }

        template <typename ExPolicy, typename Comp, typename Proj>
        static bool sequential(ExPolicy const& policy,
            segmented_sort_block<LocalIter> const& runs, Comp comp, Proj proj)
        {
            return execute(policy, runs, HPX_MOVE(comp), HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename Comp, typename Proj>
        static bool parallel(ExPolicy const& policy,
            segmented_sort_block<LocalIter> const& runs, Comp comp, Proj proj)
        {
            return execute(policy, runs, HPX_MOVE(comp), HPX_MOVE(proj));
        }
    };

    // Fetch each sorted run once onto the locality already holding the most
    // participating data, perform one serial k-way merge there, and write each
    // final run once. Network traffic and coordinator storage are linear in the
    // input size.
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

        using operation = segmented_sort_kway_merge_on_host<LocalIter>;
        segmented_sort_block<LocalIter> const empty;
        auto const host = segmented_sort_host(runs, empty);
        std::vector<hpx::future<bool>> pending;
        std::exception_ptr error;
        try
        {
            if (host == hpx::find_here())
            {
                operation().call2(policy, is_seq, runs, comp, proj);
            }
            else
            {
                pending.push_back(dispatch_async(
                    host, operation(), policy, is_seq, runs, comp, proj));
            }
        }
        catch (...)
        {
            error = std::current_exception();
        }
        segmented_sort_wait<ExPolicy>(pending, error);
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
    /// The final merge runs serially on one participating locality and uses
    /// storage proportional to the input size. Destination runs are committed
    /// incrementally; an exception during this phase can leave a partially
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
    /// locality operations overlap. The final merge runs serially on one
    /// participating locality and uses storage proportional to the input size.
    /// Destination runs are committed incrementally; an exception during this
    /// phase can leave a partially reordered range.
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
