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
            // Sort via .base() so Apple libc++'s operator[] path does not
            // hit iterator_facade's brackets proxy. Fold proj into the
            // comparator: hpx::sort's CPO does not take a projection.
            hpx::sort(hpx::execution::experimental::to_non_task(policy),
                first.base(), last_iter.base(),
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
        runs.reserve(
            static_cast<std::size_t>(std::distance(sit, send)) + 1);

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
            auto last_iter = advance_to_sentinel(first, last);
            std::vector<value_type> result(static_cast<std::size_t>(
                std::distance(first, last_iter)));
            hpx::copy(hpx::execution::experimental::to_non_task(policy),
                first, last_iter, result.begin());
            return result;
        }

        template <typename ExPolicy, typename InIter, typename Sent>
        static std::vector<value_type> parallel(
            ExPolicy&& policy, InIter first, Sent last)
        {
            return sequential(
                HPX_FORWARD(ExPolicy, policy), first, last);
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
        static InIter sequential(ExPolicy&& policy, InIter first, Sent,
            std::vector<T> const& values)
        {
            return hpx::copy(
                hpx::execution::experimental::to_non_task(policy),
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

    template <typename LocalIter>
    std::size_t segmented_sort_host_index(
        std::vector<segmented_sort_run<LocalIter>> const& runs)
    {
        auto const it = std::max_element(runs.begin(), runs.end(),
            [](auto const& a, auto const& b) {
                return (a.last - a.first) < (b.last - b.first);
            });
        return static_cast<std::size_t>(std::distance(runs.begin(), it));
    }

    // Pairwise-merge sorted runs with hpx::merge. Peak temporary storage
    // is one merged buffer of size N.
    template <typename ExPolicy, typename T, typename Pred>
    std::vector<T> segmented_sort_merge_parts(ExPolicy&& policy,
        std::vector<std::vector<T>>& parts, Pred pred)
    {
        std::vector<std::vector<T>> nonempty;
        nonempty.reserve(parts.size());
        for (auto& part : parts)
        {
            if (!part.empty())
            {
                nonempty.push_back(HPX_MOVE(part));
            }
        }
        parts.clear();

        if (nonempty.empty())
        {
            return {};
        }
        if (nonempty.size() == 1)
        {
            return HPX_MOVE(nonempty[0]);
        }

        std::vector<T> merged = HPX_MOVE(nonempty[0]);
        auto const sync_policy =
            hpx::execution::experimental::to_non_task(policy);
        for (std::size_t i = 1; i != nonempty.size(); ++i)
        {
            std::vector<T> out(merged.size() + nonempty[i].size());
            hpx::merge(sync_policy, merged.begin(), merged.end(),
                nonempty[i].begin(), nonempty[i].end(), out.begin(), pred);
            merged = HPX_MOVE(out);
        }
        return merged;
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
        std::vector<std::size_t> counts(n);
        for (std::size_t i = 0; i != n; ++i)
        {
            counts[i] =
                static_cast<std::size_t>(runs[i].last - runs[i].first);
        }

        for (std::size_t i = 0; i != n; ++i)
        {
            if (i == host)
            {
                parts[i] = std::vector<value_type>(
                    static_cast<std::size_t>(std::distance(first, last_iter)));
                hpx::copy(hpx::execution::experimental::to_non_task(policy),
                    first, last_iter, parts[i].begin());
            }
            else
            {
                parts[i] = dispatch(runs[i].id,
                    segmented_fetch_values<LocalIter>(), policy,
                    std::true_type(), runs[i].first, runs[i].last);
            }
        }

        util::compare_projected<Comp&, Proj&> pred(comp, proj);
        std::vector<value_type> merged =
            segmented_sort_merge_parts(policy, parts, pred);

        std::size_t offset = 0;
        for (std::size_t dest = 0; dest != n; ++dest)
        {
            auto const piece_first = merged.begin() +
                static_cast<std::ptrdiff_t>(offset);
            auto const piece_last = piece_first +
                static_cast<std::ptrdiff_t>(counts[dest]);

            if (dest == host)
            {
                hpx::copy(hpx::execution::experimental::to_non_task(policy),
                    piece_first, piece_last, first);
            }
            else
            {
                std::vector<value_type> piece(piece_first, piece_last);
                dispatch(runs[dest].id, segmented_store_values<LocalIter>(),
                    policy, std::true_type(), runs[dest].first,
                    runs[dest].last, piece);
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
        std::vector<std::size_t> counts(n);
        for (std::size_t i = 0; i != n; ++i)
        {
            counts[i] =
                static_cast<std::size_t>(runs[i].last - runs[i].first);
        }

        std::vector<hpx::future<std::vector<value_type>>> fetches;
        fetches.reserve(n);
        std::vector<std::size_t> remote;
        remote.reserve(n);

        parts[host] = std::vector<value_type>(
            static_cast<std::size_t>(std::distance(first, last_iter)));
        hpx::copy(hpx::execution::experimental::to_non_task(policy), first,
            last_iter, parts[host].begin());

        for (std::size_t i = 0; i != n; ++i)
        {
            if (i == host)
            {
                continue;
            }
            remote.push_back(i);
            fetches.push_back(dispatch_async(runs[i].id,
                segmented_fetch_values<LocalIter>(), policy, std::false_type(),
                runs[i].first, runs[i].last));
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

        std::size_t offset = 0;
        for (std::size_t dest = 0; dest != n; ++dest)
        {
            auto const piece_first = merged.begin() +
                static_cast<std::ptrdiff_t>(offset);
            auto const piece_last = piece_first +
                static_cast<std::ptrdiff_t>(counts[dest]);

            if (dest == host)
            {
                hpx::copy(hpx::execution::experimental::to_non_task(policy),
                    piece_first, piece_last, first);
            }
            else
            {
                std::vector<value_type> piece(piece_first, piece_last);
                stores.push_back(dispatch_async(runs[dest].id,
                    segmented_store_values<LocalIter>(), policy,
                    std::false_type(), runs[dest].first, runs[dest].last,
                    HPX_MOVE(piece)));
            }
            offset += counts[dest];
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
