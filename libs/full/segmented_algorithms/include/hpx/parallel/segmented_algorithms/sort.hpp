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

    template <typename LocalIter>
    std::vector<typename std::iterator_traits<LocalIter>::value_type>
    segmented_sort_fetch_span(
        std::vector<segmented_sort_run<LocalIter>> const& runs,
        std::size_t first, std::size_t last)
    {
        using value_type = typename std::iterator_traits<LocalIter>::value_type;

        std::vector<value_type> values;
        for (std::size_t i = first; i != last; ++i)
        {
            auto part = dispatch(runs[i].id,
                segmented_fetch_values<LocalIter>(), hpx::execution::seq,
                std::true_type(), runs[i].first, runs[i].last);
            values.insert(values.end(), part.begin(), part.end());
        }
        return values;
    }

    template <typename LocalIter>
    void segmented_sort_store_span(
        std::vector<segmented_sort_run<LocalIter>> const& runs,
        std::size_t first, std::size_t last,
        std::vector<typename std::iterator_traits<LocalIter>::value_type> const&
            values)
    {
        using value_type = typename std::iterator_traits<LocalIter>::value_type;

        std::size_t offset = 0;
        for (std::size_t i = first; i != last; ++i)
        {
            auto const n =
                static_cast<std::size_t>(runs[i].last - runs[i].first);
            std::vector<value_type> piece(
                values.begin() + static_cast<std::ptrdiff_t>(offset),
                values.begin() + static_cast<std::ptrdiff_t>(offset + n));
            dispatch(runs[i].id, segmented_store_values<LocalIter>(),
                hpx::execution::seq, std::true_type(), runs[i].first,
                runs[i].last, piece);
            offset += n;
        }
    }

    // Merge two already-sorted adjacent groups of partitions
    // [first, mid) and [mid, last) and write the merged order back.
    template <typename LocalIter, typename Comp, typename Proj>
    void segmented_sort_merge_span(
        std::vector<segmented_sort_run<LocalIter>> const& runs,
        std::size_t first, std::size_t mid, std::size_t last, Comp& comp,
        Proj& proj)
    {
        using value_type = typename std::iterator_traits<LocalIter>::value_type;

        auto left = segmented_sort_fetch_span(runs, first, mid);
        auto right = segmented_sort_fetch_span(runs, mid, last);

        std::vector<value_type> merged;
        merged.resize(left.size() + right.size());
        util::compare_projected<Comp&, Proj&> pred(comp, proj);
        std::merge(left.begin(), left.end(), right.begin(), right.end(),
            merged.begin(), pred);

        segmented_sort_store_span(runs, first, last, merged);
    }

    template <typename ExPolicy, typename LocalIter, typename Comp,
        typename Proj>
    void segmented_sort_local_runs(ExPolicy const& policy,
        std::vector<segmented_sort_run<LocalIter>>& runs, Comp& comp,
        Proj& proj, std::true_type)
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
        std::vector<segmented_sort_run<LocalIter>>& runs, Comp& comp,
        Proj& proj, std::false_type)
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

        if (!segments.empty())
        {
            hpx::wait_all(segments);
            std::list<std::exception_ptr> errors;
            parallel::util::detail::handle_remote_exceptions<ExPolicy>::call(
                segments, errors);
        }
    }

    template <typename LocalIter, typename Comp, typename Proj>
    void segmented_sort_merge_tree(
        std::vector<segmented_sort_run<LocalIter>>& runs, Comp& comp,
        Proj& proj, std::true_type)
    {
        std::size_t const n = runs.size();
        for (std::size_t stride = 1; stride < n; stride *= 2)
        {
            for (std::size_t i = 0; i + stride < n; i += 2 * stride)
            {
                std::size_t const last = (std::min) (i + 2 * stride, n);
                segmented_sort_merge_span(
                    runs, i, i + stride, last, comp, proj);
            }
        }
    }

    template <typename LocalIter, typename Comp, typename Proj>
    void segmented_sort_merge_tree(
        std::vector<segmented_sort_run<LocalIter>>& runs, Comp& comp,
        Proj& proj, std::false_type)
    {
        std::size_t const n = runs.size();
        for (std::size_t stride = 1; stride < n; stride *= 2)
        {
            std::vector<hpx::future<void>> merges;
            for (std::size_t i = 0; i + stride < n; i += 2 * stride)
            {
                std::size_t const last = (std::min) (i + 2 * stride, n);
                std::size_t const mid = i + stride;
                merges.push_back(hpx::async([&runs, i, mid, last, &comp,
                                                &proj]() {
                    segmented_sort_merge_span(runs, i, mid, last, comp, proj);
                }));
            }

            if (!merges.empty())
            {
                hpx::wait_all(merges);
                for (auto& f : merges)
                {
                    f.get();
                }
            }
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

        if constexpr (hpx::is_async_execution_policy_v<ExPolicy>)
        {
            return result::get(hpx::async([=, runs = HPX_MOVE(runs)]() mutable {
                segmented_sort_local_runs(policy, runs, cmp, prj, is_seq);
                segmented_sort_merge_tree(runs, cmp, prj, is_seq);
            }));
        }
        else
        {
            segmented_sort_local_runs(policy, runs, cmp, prj, is_seq);
            segmented_sort_merge_tree(runs, cmp, prj, is_seq);
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
