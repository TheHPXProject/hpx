//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/algorithms.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/runtime_distributed/find_here.hpp>

#include <hpx/parallel/algorithms/detail/advance_and_get_distance.hpp>
#include <hpx/parallel/segmented_algorithms/detail/dispatch.hpp>
#include <hpx/parallel/util/detail/handle_remote_exceptions.hpp>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iterator>
#include <list>
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
                util::compare_projected<Comp&, Proj&>(comp, proj));
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

    template <typename LocalIter>
    struct segmented_sort_run
    {
        hpx::id_type id{};
        LocalIter first{};
        LocalIter last{};
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

        if (sit == send)
        {
            local_iterator beg = traits::local(first);
            local_iterator end = traits::local(last);
            if (beg != end)
            {
                runs.emplace_back(traits::get_id(sit), beg, end);
            }
            return runs;
        }

        local_iterator beg = traits::local(first);
        local_iterator end = traits::end(sit);
        if (beg != end)
        {
            runs.emplace_back(traits::get_id(sit), beg, end);
        }

        for (++sit; sit != send; ++sit)
        {
            beg = traits::begin(sit);
            end = traits::end(sit);
            if (beg != end)
            {
                runs.emplace_back(traits::get_id(sit), beg, end);
            }
        }

        beg = traits::begin(sit);
        end = traits::local(last);
        if (beg != end)
        {
            runs.emplace_back(traits::get_id(sit), beg, end);
        }
        return runs;
    }

    template <typename Iter>
    struct segmented_fetch_values
      : algorithm<segmented_fetch_values<Iter>,
            std::vector<typename std::iterator_traits<Iter>::value_type>>
    {
        using value_type = std::iterator_traits<Iter>::value_type;

        constexpr segmented_fetch_values() noexcept
          : algorithm<segmented_fetch_values, std::vector<value_type>>(
                "segmented_fetch_values")
        {
        }

        template <typename ExPolicy, typename InIter, typename Sent>
        static std::vector<value_type> sequential(
            ExPolicy&& policy, InIter first, Sent last)
        {
            auto last_iter = first;
            auto const size = advance_and_get_distance(last_iter, last);
            std::vector<value_type> result(static_cast<std::size_t>(size));
            hpx::copy(hpx::execution::experimental::to_non_task(
                          HPX_FORWARD(ExPolicy, policy)),
                first, last_iter, result.begin());
            return result;
        }

        template <typename ExPolicy, typename InIter, typename Sent>
        static std::vector<value_type> parallel(
            ExPolicy&& policy, InIter first, Sent last)
        {
            return sequential(HPX_FORWARD(ExPolicy, policy), first, last);
        }
    };

    template <typename Iter>
    struct segmented_store_values
      : algorithm<segmented_store_values<Iter>, Iter>
    {
        constexpr segmented_store_values() noexcept
          : algorithm<segmented_store_values, Iter>("segmented_store_values")
        {
        }

        template <typename ExPolicy, typename InIter, typename Sent, typename T>
        static InIter sequential(
            ExPolicy&& policy, InIter first, Sent, std::vector<T> const& values)
        {
            return hpx::copy(hpx::execution::experimental::to_non_task(
                                 HPX_FORWARD(ExPolicy, policy)),
                values.begin(), values.end(), first);
        }

        template <typename ExPolicy, typename InIter, typename Sent, typename T>
        static InIter parallel(ExPolicy&& policy, InIter first, Sent last,
            std::vector<T> const& values)
        {
            return sequential(
                HPX_FORWARD(ExPolicy, policy), first, last, values);
        }
    };

    template <typename ExPolicy, typename T>
    void segmented_sort_wait_all(std::vector<hpx::future<T>>& fs)
    {
        if (fs.empty())
        {
            return;
        }

        if (hpx::wait_all_nothrow(fs))
        {
            std::list<std::exception_ptr> errors;
            parallel::util::detail::handle_remote_exceptions<ExPolicy>::call(
                fs, errors);
        }
    }

    // Select the largest run by scanning the runs vector directly.
    template <typename LocalIter>
    std::size_t segmented_sort_host_index(
        std::vector<segmented_sort_run<LocalIter>> const& runs)
    {
        auto const it = std::max_element(
            runs.begin(), runs.end(), [](auto const& a, auto const& b) {
                return (a.last - a.first) < (b.last - b.first);
            });
        return static_cast<std::size_t>(std::distance(runs.begin(), it));
    }

    // Tree-merge sorted runs with hpx::merge. Reuse level/next and a single
    // merge buffer so allocations stay bounded; pair-reduce so the merge
    // depth is O(log P) when P > 3.
    template <typename ExPolicy, typename T, typename Pred>
    std::vector<T> segmented_sort_merge_parts(
        ExPolicy const& policy, std::vector<std::vector<T>>& parts, Pred pred)
    {
        std::vector<std::vector<T>> level;
        level.reserve(parts.size());
        std::size_t total = 0;
        for (auto& part : parts)
        {
            if (!part.empty())
            {
                total += part.size();
                level.push_back(HPX_MOVE(part));
            }
        }
        parts.clear();

        if (level.empty())
        {
            return {};
        }
        if (level.size() == 1)
        {
            return HPX_MOVE(level[0]);
        }

        std::vector<T> out;
        out.reserve(total);

        // Linear reduce for two or three runs; tree reduce otherwise.
        if (level.size() <= 3)
        {
            std::vector<T> merged = HPX_MOVE(level[0]);
            for (std::size_t i = 1; i != level.size(); ++i)
            {
                out.resize(merged.size() + level[i].size());
                hpx::merge(policy, merged.begin(), merged.end(),
                    level[i].begin(), level[i].end(), out.begin(), pred);
                merged.swap(out);
            }
            return merged;
        }

        std::size_t max_pair = 0;
        for (std::size_t i = 0; i + 1 < level.size(); i += 2)
        {
            max_pair =
                (std::max) (max_pair, level[i].size() + level[i + 1].size());
        }
        out.resize(max_pair);

        std::vector<std::vector<T>> next;
        next.reserve((level.size() + 1) / 2);

        while (level.size() > 1)
        {
            std::size_t const needed = (level.size() + 1) / 2;
            if (next.capacity() < needed)
            {
                next.reserve(needed);
            }
            next.clear();

            std::size_t max_next_pair = 0;
            std::size_t i = 0;
            for (; i + 1 < level.size(); i += 2)
            {
                auto const pair_size = level[i].size() + level[i + 1].size();
                max_next_pair = (std::max) (max_next_pair, pair_size);
                out.resize(pair_size);
                hpx::merge(policy, level[i].begin(), level[i].end(),
                    level[i + 1].begin(), level[i + 1].end(), out.begin(),
                    pred);
                next.emplace_back(out.begin(), out.end());
            }
            if (i < level.size())
            {
                next.push_back(HPX_MOVE(level[i]));
            }
            level.swap(next);
            if (max_next_pair > out.capacity())
            {
                out.reserve(max_next_pair);
            }
        }
        return HPX_MOVE(level[0]);
    }

    template <typename ExPolicy, typename InIter, typename LocalIter,
        typename Comp, typename Proj>
    InIter segmented_sort_merge_on_host_seq(ExPolicy const& policy,
        InIter first, InIter last_iter,
        std::vector<segmented_sort_run<LocalIter>> const& runs,
        std::size_t host, Comp&& comp, Proj&& proj)
    {
        using value_type = std::iterator_traits<InIter>::value_type;

        std::size_t const n = runs.size();
        std::vector<std::vector<value_type>> parts(n);
        std::vector<std::ptrdiff_t> counts(n);
        hpx::id_type const here = hpx::find_here();
        using local_traits =
            hpx::traits::segmented_local_iterator_traits<LocalIter>;

        for (std::size_t i = 0; i != n; ++i)
        {
            counts[i] = runs[i].last - runs[i].first;

            if (i == host)
            {
                auto local_last = first;
                auto const size =
                    advance_and_get_distance(local_last, last_iter);
                parts[i] =
                    std::vector<value_type>(static_cast<std::size_t>(size));
                hpx::copy(policy, first, local_last, parts[i].begin());
            }
            else if (runs[i].id == here)
            {
                parts[i] = std::vector<value_type>(
                    static_cast<std::size_t>(counts[i]));
                hpx::copy(policy, local_traits::local(runs[i].first),
                    local_traits::local(runs[i].last), parts[i].begin());
            }
            else
            {
                parts[i] =
                    dispatch(runs[i].id, segmented_fetch_values<LocalIter>(),
                        policy, std::true_type(), runs[i].first, runs[i].last);
            }
        }

        util::compare_projected<Comp&, Proj&> pred(comp, proj);
        std::vector<value_type> merged =
            segmented_sort_merge_parts(policy, parts, pred);

        std::ptrdiff_t offset = 0;
        for (std::size_t dest = 0; dest != n; ++dest)
        {
            auto const piece_first = merged.begin() + offset;
            auto const piece_last = piece_first + counts[dest];

            if (dest == host)
            {
                hpx::copy(policy, piece_first, piece_last, first);
            }
            else if (runs[dest].id == here)
            {
                hpx::copy(policy, piece_first, piece_last,
                    local_traits::local(runs[dest].first));
            }
            else
            {
                std::vector<value_type> piece(piece_first, piece_last);
                dispatch(runs[dest].id, segmented_store_values<LocalIter>(),
                    policy, std::true_type(), runs[dest].first, runs[dest].last,
                    piece);
            }
            offset += counts[dest];
        }

        return last_iter;
    }

    template <typename ExPolicy, typename InIter, typename LocalIter,
        typename Comp, typename Proj>
    InIter segmented_sort_merge_on_host_par(ExPolicy const& policy,
        InIter first, InIter last_iter,
        std::vector<segmented_sort_run<LocalIter>> const& runs,
        std::size_t host, Comp&& comp, Proj&& proj)
    {
        using value_type = std::iterator_traits<InIter>::value_type;

        std::size_t const n = runs.size();
        std::vector<std::vector<value_type>> parts(n);
        std::vector<std::ptrdiff_t> counts(n);
        hpx::id_type const here = hpx::find_here();
        using local_traits =
            hpx::traits::segmented_local_iterator_traits<LocalIter>;

        std::vector<hpx::future<std::vector<value_type>>> fetches;
        fetches.reserve(n);
        std::vector<std::size_t> remote;
        remote.reserve(n);
        std::vector<std::size_t> local_parts;
        local_parts.reserve(n);

        // Launch remote fetches first; co-located and host copies run after
        // so they overlap outstanding RPCs.
        for (std::size_t i = 0; i != n; ++i)
        {
            counts[i] = runs[i].last - runs[i].first;
            if (i == host)
            {
                continue;
            }
            if (runs[i].id == here)
            {
                local_parts.push_back(i);
                continue;
            }
            remote.push_back(i);
            fetches.push_back(
                dispatch_async(runs[i].id, segmented_fetch_values<LocalIter>(),
                    policy, std::false_type(), runs[i].first, runs[i].last));
        }

        {
            auto local_last = first;
            auto const size = advance_and_get_distance(local_last, last_iter);
            parts[host] =
                std::vector<value_type>(static_cast<std::size_t>(size));
            hpx::copy(policy, first, local_last, parts[host].begin());
        }

        for (std::size_t const i : local_parts)
        {
            parts[i] =
                std::vector<value_type>(static_cast<std::size_t>(counts[i]));
            hpx::copy(policy, local_traits::local(runs[i].first),
                local_traits::local(runs[i].last), parts[i].begin());
        }

        segmented_sort_wait_all<ExPolicy>(fetches);
        for (std::size_t j = 0; j != remote.size(); ++j)
        {
            parts[remote[j]] = fetches[j].get();
        }

        util::compare_projected<Comp&, Proj&> pred(comp, proj);
        std::vector<value_type> merged =
            segmented_sort_merge_parts(policy, parts, pred);

        std::vector<hpx::future<LocalIter>> stores;
        stores.reserve(remote.size());

        struct local_piece
        {
            std::size_t dest;
            std::ptrdiff_t offset;
            std::ptrdiff_t count;
        };
        std::vector<local_piece> local_stores;
        local_stores.reserve(local_parts.size());

        std::ptrdiff_t offset = 0;
        std::ptrdiff_t host_offset = 0;
        std::ptrdiff_t host_count = 0;
        for (std::size_t dest = 0; dest != n; ++dest)
        {
            if (dest == host)
            {
                host_offset = offset;
                host_count = counts[dest];
            }
            else if (runs[dest].id == here)
            {
                local_stores.push_back(local_piece{dest, offset, counts[dest]});
            }
            else
            {
                auto const piece_first = merged.begin() + offset;
                auto const piece_last = piece_first + counts[dest];
                std::vector<value_type> piece(piece_first, piece_last);
                stores.push_back(dispatch_async(runs[dest].id,
                    segmented_store_values<LocalIter>(), policy,
                    std::false_type(), runs[dest].first, runs[dest].last,
                    HPX_MOVE(piece)));
            }
            offset += counts[dest];
        }

        // Host and co-located writes overlap remote stores.
        hpx::copy(policy, merged.begin() + host_offset,
            merged.begin() + host_offset + host_count, first);
        for (auto const& lp : local_stores)
        {
            hpx::copy(policy, merged.begin() + lp.offset,
                merged.begin() + lp.offset + lp.count,
                local_traits::local(runs[lp.dest].first));
        }

        segmented_sort_wait_all<ExPolicy>(stores);
        return last_iter;
    }

    template <typename Iter>
    struct segmented_sort_kway_merge
      : algorithm<segmented_sort_kway_merge<Iter>, Iter>
    {
        constexpr segmented_sort_kway_merge() noexcept
          : algorithm<segmented_sort_kway_merge, Iter>(
                "segmented_sort_kway_merge")
        {
        }

        template <typename ExPolicy, typename InIter, typename Sent,
            typename LocalIter, typename Comp, typename Proj>
        static InIter sequential(ExPolicy&& policy, InIter first, Sent last,
            std::vector<segmented_sort_run<LocalIter>> const& runs,
            std::size_t host, Comp&& comp, Proj&& proj)
        {
            auto last_iter = advance_to_sentinel(first, last);
            return segmented_sort_merge_on_host_seq(
                HPX_FORWARD(ExPolicy, policy), first, last_iter, runs, host,
                HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
        }

        template <typename ExPolicy, typename InIter, typename Sent,
            typename LocalIter, typename Comp, typename Proj>
        static InIter parallel(ExPolicy&& policy, InIter first, Sent last,
            std::vector<segmented_sort_run<LocalIter>> const& runs,
            std::size_t host, Comp&& comp, Proj&& proj)
        {
            auto last_iter = advance_to_sentinel(first, last);
            return segmented_sort_merge_on_host_par(
                HPX_FORWARD(ExPolicy, policy), first, last_iter, runs, host,
                HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
        }
    };

    template <typename ExPolicy, typename LocalIter, typename Comp,
        typename Proj>
    void segmented_sort_local_runs(ExPolicy const& policy,
        std::vector<segmented_sort_run<LocalIter>>& runs, Comp&& comp,
        Proj&& proj, std::true_type)
    {
        using local_traits =
            hpx::traits::segmented_local_iterator_traits<LocalIter>;
        hpx::id_type const here = hpx::find_here();

        for (auto const& run : runs)
        {
            if (run.id == here)
            {
                hpx::sort(policy, local_traits::local(run.first),
                    local_traits::local(run.last),
                    util::compare_projected<Comp&, Proj&>(comp, proj));
            }
            else
            {
                dispatch(run.id, segmented_local_sort<LocalIter>(), policy,
                    std::true_type(), run.first, run.last, comp, proj);
            }
        }
    }

    template <typename ExPolicy, typename LocalIter, typename Comp,
        typename Proj>
    void segmented_sort_local_runs(ExPolicy const& policy,
        std::vector<segmented_sort_run<LocalIter>>& runs, Comp&& comp,
        Proj&& proj, std::false_type)
    {
        using forced_seq = std::false_type;
        using local_traits =
            hpx::traits::segmented_local_iterator_traits<LocalIter>;
        hpx::id_type const here = hpx::find_here();

        std::vector<hpx::future<LocalIter>> segments;
        segments.reserve(runs.size());
        std::vector<segmented_sort_run<LocalIter> const*> local_runs;
        local_runs.reserve(runs.size());

        for (auto const& run : runs)
        {
            if (run.id == here)
            {
                local_runs.push_back(&run);
            }
            else
            {
                segments.push_back(
                    dispatch_async(run.id, segmented_local_sort<LocalIter>(),
                        policy, forced_seq(), run.first, run.last, comp, proj));
            }
        }

        // Local sorts run synchronously while remotes are in flight.
        for (auto const* run : local_runs)
        {
            hpx::sort(policy, local_traits::local(run->first),
                local_traits::local(run->last),
                util::compare_projected<Comp&, Proj&>(comp, proj));
        }

        segmented_sort_wait_all<ExPolicy>(segments);
    }

    // Fetch each remote sorted run once onto the locality that already
    // holds the largest run, merge there with hpx::merge, and write only
    // the remote pieces back. That keeps traffic O(N) and avoids a full
    // gather to the caller.
    template <typename ExPolicy, typename LocalIter, typename Comp,
        typename Proj, typename IsSeq>
    void segmented_sort_merge_runs(ExPolicy const& policy,
        std::vector<segmented_sort_run<LocalIter>>& runs, Comp&& comp,
        Proj&& proj, IsSeq is_seq)
    {
        if (runs.size() <= 1)
        {
            return;
        }

        std::size_t const host = segmented_sort_host_index(runs);
        using local_traits =
            hpx::traits::segmented_local_iterator_traits<LocalIter>;
        hpx::id_type const here = hpx::find_here();

        if (runs[host].id == here)
        {
            if constexpr (IsSeq::value)
            {
                segmented_sort_merge_on_host_seq(policy,
                    local_traits::local(runs[host].first),
                    local_traits::local(runs[host].last), runs, host,
                    HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
            }
            else
            {
                segmented_sort_merge_on_host_par(policy,
                    local_traits::local(runs[host].first),
                    local_traits::local(runs[host].last), runs, host,
                    HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
            }
        }
        else
        {
            dispatch(runs[host].id, segmented_sort_kway_merge<LocalIter>(),
                policy, is_seq, runs[host].first, runs[host].last, runs, host,
                HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
        }
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

        auto runs = segmented_sort_runs(first, last);
        if (runs.empty())
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
            return result::get(hpx::async([=, runs = HPX_MOVE(runs)]() mutable {
                segmented_sort_local_runs(sync_policy, runs, cmp, prj, is_seq);
                segmented_sort_merge_runs(sync_policy, runs, cmp, prj, is_seq);
            }));
        }
        else
        {
            segmented_sort_local_runs(sync_policy, runs, cmp, prj, is_seq);
            segmented_sort_merge_runs(sync_policy, runs, cmp, prj, is_seq);
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
