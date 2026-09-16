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

#include <hpx/parallel/segmented_algorithms/detail/dispatch.hpp>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iterator>
#include <list>
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
        static constexpr InIter sequential(
            ExPolicy, InIter first, Sent last, Comp&& comp, Proj&& proj)
        {
            auto last_iter = advance_to_sentinel(first, last);
            std::sort(first, last_iter,
                util::compare_projected<Comp&, Proj&>(comp, proj));
            return last_iter;
        }

        template <typename ExPolicy, typename InIter, typename Sent,
            typename Comp, typename Proj>
        static InIter parallel(
            ExPolicy&&, InIter first, Sent last, Comp&& comp, Proj&& proj)
        {
            return sequential(hpx::execution::seq, first, last,
                HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
        }
    };

    template <typename LocalIter>
    struct segmented_sort_run
    {
        hpx::id_type id;
        LocalIter first;
        LocalIter last;

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            // clang-format off
            ar & id & first & last;
            // clang-format on
        }
    };

    template <typename SegIter>
    auto segmented_sort_runs(SegIter first, SegIter last)
    {
        using traits = hpx::traits::segmented_iterator_traits<SegIter>;
        using segment_iterator = typename traits::segment_iterator;
        using local_iterator = typename traits::local_iterator;
        using run_type = segmented_sort_run<local_iterator>;

        std::vector<run_type> runs;
        segment_iterator sit = traits::segment(first);
        segment_iterator send = traits::segment(last);

        if (sit == send)
        {
            local_iterator beg = traits::local(first);
            local_iterator end = traits::local(last);
            if (beg != end)
            {
                runs.push_back(run_type{traits::get_id(sit), beg, end});
            }
            return runs;
        }

        local_iterator beg = traits::local(first);
        local_iterator end = traits::end(sit);
        if (beg != end)
        {
            runs.push_back(run_type{traits::get_id(sit), beg, end});
        }

        for (++sit; sit != send; ++sit)
        {
            beg = traits::begin(sit);
            end = traits::end(sit);
            if (beg != end)
            {
                runs.push_back(run_type{traits::get_id(sit), beg, end});
            }
        }

        beg = traits::begin(sit);
        end = traits::local(last);
        if (beg != end)
        {
            runs.push_back(run_type{traits::get_id(sit), beg, end});
        }
        return runs;
    }

    template <typename Iter>
    struct segmented_fetch_values
      : algorithm<segmented_fetch_values<Iter>,
            std::vector<typename std::iterator_traits<Iter>::value_type>>
    {
        using value_type = typename std::iterator_traits<Iter>::value_type;

        constexpr segmented_fetch_values() noexcept
          : algorithm<segmented_fetch_values, std::vector<value_type>>(
                "segmented_fetch_values")
        {
        }

        template <typename ExPolicy, typename InIter, typename Sent>
        static std::vector<value_type> sequential(
            ExPolicy, InIter first, Sent last)
        {
            return std::vector<value_type>(first, last);
        }

        template <typename ExPolicy, typename InIter, typename Sent>
        static std::vector<value_type> parallel(
            ExPolicy&&, InIter first, Sent last)
        {
            return sequential(hpx::execution::seq, first, last);
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
            ExPolicy, InIter first, Sent, std::vector<T> const& values)
        {
            return std::copy(values.begin(), values.end(), first);
        }

        template <typename ExPolicy, typename InIter, typename Sent, typename T>
        static InIter parallel(
            ExPolicy&&, InIter first, Sent last, std::vector<T> const& values)
        {
            return sequential(hpx::execution::seq, first, last, values);
        }
    };

    template <typename ExPolicy, typename T>
    void segmented_sort_wait_all(std::vector<hpx::future<T>>& fs)
    {
        if (fs.empty())
        {
            return;
        }

        hpx::wait_all_nothrow(fs);
        std::list<std::exception_ptr> errors;
        parallel::util::detail::handle_remote_exceptions<ExPolicy>::call(
            fs, errors);
    }

    template <typename LocalIter>
    std::size_t segmented_sort_host_index(
        std::vector<segmented_sort_run<LocalIter>> const& runs)
    {
        std::size_t host = 0;
        auto host_n = runs[0].last - runs[0].first;
        for (std::size_t i = 1; i != runs.size(); ++i)
        {
            auto const n = runs[i].last - runs[i].first;
            if (n > host_n)
            {
                host = i;
                host_n = n;
            }
        }
        return host;
    }

    template <typename T, typename Pred>
    std::vector<T> segmented_sort_kway_heap(
        std::vector<std::vector<T>> const& parts, Pred pred)
    {
        struct cursor
        {
            std::size_t part;
            std::size_t pos;
        };

        auto order = [&](cursor const& a, cursor const& b) {
            return pred(parts[b.part][b.pos], parts[a.part][a.pos]);
        };

        std::priority_queue<cursor, std::vector<cursor>, decltype(order)> heap(
            order);

        std::size_t total = 0;
        for (std::size_t i = 0; i != parts.size(); ++i)
        {
            total += parts[i].size();
            if (!parts[i].empty())
            {
                heap.push(cursor{i, 0});
            }
        }

        std::vector<T> merged;
        merged.reserve(total);
        while (!heap.empty())
        {
            cursor const c = heap.top();
            heap.pop();
            merged.push_back(parts[c.part][c.pos]);
            std::size_t const next = c.pos + 1;
            if (next != parts[c.part].size())
            {
                heap.push(cursor{c.part, next});
            }
        }
        return merged;
    }

    template <typename T, typename Pred>
    std::vector<T> segmented_sort_merge_parts(
        std::true_type, std::vector<std::vector<T>>& parts, Pred pred)
    {
        if (parts.size() == 2)
        {
            std::size_t const total = parts[0].size() + parts[1].size();
            std::vector<T> merged;
            merged.reserve(total);
            std::merge(parts[0].begin(), parts[0].end(), parts[1].begin(),
                parts[1].end(), std::back_inserter(merged), pred);
            return merged;
        }
        return segmented_sort_kway_heap(parts, pred);
    }

    template <typename T, typename Pred>
    std::vector<T> segmented_sort_merge_parts(
        std::false_type, std::vector<std::vector<T>>& parts, Pred pred)
    {
        if (parts.size() == 2)
        {
            std::size_t const total = parts[0].size() + parts[1].size();
            if (total == 0)
            {
                return {};
            }

            T const sample =
                !parts[0].empty() ? parts[0].front() : parts[1].front();
            std::vector<T> merged(total, sample);
            hpx::merge(hpx::execution::par, parts[0].begin(), parts[0].end(),
                parts[1].begin(), parts[1].end(), merged.begin(), pred);
            return merged;
        }
        return segmented_sort_kway_heap(parts, pred);
    }

    template <typename ExPolicy, typename InIter, typename Sent,
        typename LocalIter, typename Comp, typename Proj, typename IsSeq>
    InIter segmented_sort_merge_on_host(ExPolicy const& policy, InIter first,
        Sent last, std::vector<segmented_sort_run<LocalIter>> const& runs,
        std::size_t host, Comp&& comp, Proj&& proj, IsSeq is_seq)
    {
        using value_type = typename std::iterator_traits<InIter>::value_type;

        auto last_iter = advance_to_sentinel(first, last);
        std::size_t const n = runs.size();
        std::vector<std::vector<value_type>> parts(n);

        if constexpr (IsSeq::value)
        {
            for (std::size_t i = 0; i != n; ++i)
            {
                if (i == host)
                {
                    parts[i] = std::vector<value_type>(first, last_iter);
                }
                else
                {
                    parts[i] = dispatch(runs[i].id,
                        segmented_fetch_values<LocalIter>(),
                        hpx::execution::seq, std::true_type(), runs[i].first,
                        runs[i].last);
                }
            }
        }
        else
        {
            std::vector<hpx::future<std::vector<value_type>>> fetches;
            fetches.reserve(n);
            std::vector<std::size_t> remote;
            remote.reserve(n);

            parts[host] = std::vector<value_type>(first, last_iter);
            for (std::size_t i = 0; i != n; ++i)
            {
                if (i == host)
                {
                    continue;
                }
                remote.push_back(i);
                fetches.push_back(dispatch_async(runs[i].id,
                    segmented_fetch_values<LocalIter>(), hpx::execution::seq,
                    std::true_type(), runs[i].first, runs[i].last));
            }

            segmented_sort_wait_all<ExPolicy>(fetches);
            for (std::size_t j = 0; j != remote.size(); ++j)
            {
                parts[remote[j]] = fetches[j].get();
            }
        }

        util::compare_projected<Comp&, Proj&> pred(comp, proj);
        std::vector<value_type> merged =
            segmented_sort_merge_parts(is_seq, parts, pred);
        parts.clear();

        std::size_t offset = 0;
        if constexpr (IsSeq::value)
        {
            for (std::size_t i = 0; i != n; ++i)
            {
                auto const count =
                    static_cast<std::size_t>(runs[i].last - runs[i].first);
                if (i == host)
                {
                    std::copy(
                        merged.begin() + static_cast<std::ptrdiff_t>(offset),
                        merged.begin() +
                            static_cast<std::ptrdiff_t>(offset + count),
                        first);
                }
                else
                {
                    std::vector<value_type> piece(
                        merged.begin() + static_cast<std::ptrdiff_t>(offset),
                        merged.begin() +
                            static_cast<std::ptrdiff_t>(offset + count));
                    dispatch(runs[i].id, segmented_store_values<LocalIter>(),
                        hpx::execution::seq, std::true_type(), runs[i].first,
                        runs[i].last, piece);
                }
                offset += count;
            }
        }
        else
        {
            std::vector<hpx::future<LocalIter>> stores;
            stores.reserve(n);
            for (std::size_t i = 0; i != n; ++i)
            {
                auto const count =
                    static_cast<std::size_t>(runs[i].last - runs[i].first);
                if (i == host)
                {
                    std::copy(
                        merged.begin() + static_cast<std::ptrdiff_t>(offset),
                        merged.begin() +
                            static_cast<std::ptrdiff_t>(offset + count),
                        first);
                }
                else
                {
                    std::vector<value_type> piece(
                        merged.begin() + static_cast<std::ptrdiff_t>(offset),
                        merged.begin() +
                            static_cast<std::ptrdiff_t>(offset + count));
                    stores.push_back(dispatch_async(runs[i].id,
                        segmented_store_values<LocalIter>(),
                        hpx::execution::seq, std::true_type(), runs[i].first,
                        runs[i].last, HPX_MOVE(piece)));
                }
                offset += count;
            }
            segmented_sort_wait_all<ExPolicy>(stores);
        }

        HPX_UNUSED(policy);
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
            return segmented_sort_merge_on_host(HPX_FORWARD(ExPolicy, policy),
                first, last, runs, host, HPX_FORWARD(Comp, comp),
                HPX_FORWARD(Proj, proj), std::true_type{});
        }

        template <typename ExPolicy, typename InIter, typename Sent,
            typename LocalIter, typename Comp, typename Proj>
        static InIter parallel(ExPolicy&& policy, InIter first, Sent last,
            std::vector<segmented_sort_run<LocalIter>> const& runs,
            std::size_t host, Comp&& comp, Proj&& proj)
        {
            return segmented_sort_merge_on_host(HPX_FORWARD(ExPolicy, policy),
                first, last, runs, host, HPX_FORWARD(Comp, comp),
                HPX_FORWARD(Proj, proj), std::false_type{});
        }
    };

    template <typename ExPolicy, typename LocalIter, typename Comp,
        typename Proj>
    void segmented_sort_local_runs(ExPolicy const& policy,
        std::vector<segmented_sort_run<LocalIter>>& runs, Comp&& comp,
        Proj&& proj, std::true_type)
    {
        for (auto const& run : runs)
        {
            dispatch(run.id, segmented_local_sort<LocalIter>(), policy,
                std::true_type(), run.first, run.last, comp, proj);
        }
    }

    template <typename ExPolicy, typename LocalIter, typename Comp,
        typename Proj>
    void segmented_sort_local_runs(ExPolicy const& policy,
        std::vector<segmented_sort_run<LocalIter>>& runs, Comp&& comp,
        Proj&& proj, std::false_type)
    {
        using forced_seq = std::false_type;

        std::vector<hpx::future<LocalIter>> segments;
        segments.reserve(runs.size());

        for (auto const& run : runs)
        {
            segments.push_back(
                dispatch_async(run.id, segmented_local_sort<LocalIter>(),
                    policy, forced_seq(), run.first, run.last, comp, proj));
        }

        segmented_sort_wait_all<ExPolicy>(segments);
    }

    // Fetch each remote sorted run once onto the locality that already
    // holds the largest run, k-way merge there, and write only the remote
    // pieces back. That keeps traffic O(N) and avoids a full gather to the
    // caller.
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
        dispatch(runs[host].id, segmented_sort_kway_merge<LocalIter>(), policy,
            is_seq, runs[host].first, runs[host].last, runs, host,
            HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
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

        if constexpr (hpx::is_async_execution_policy_v<ExPolicy>)
        {
            return result::get(hpx::async([=, runs = HPX_MOVE(runs)]() mutable {
                segmented_sort_local_runs(policy, runs, cmp, prj, is_seq);
                segmented_sort_merge_runs(policy, runs, cmp, prj, is_seq);
            }));
        }
        else
        {
            segmented_sort_local_runs(policy, runs, cmp, prj, is_seq);
            segmented_sort_merge_runs(policy, runs, cmp, prj, is_seq);
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
