//  Copyright (c) 2026 Arivoli Ramamoorthy
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/algorithms/radix_sort.hpp
/// \page hpx::radix_sort
/// \headerfile hpx/algorithm.hpp

#pragma once

#if defined(DOXYGEN)

namespace hpx {
    // clang-format off

    ///////////////////////////////////////////////////////////////////////////
    /// Sorts the elements in the range [first, last) in ascending order of
    /// their integral keys, using an in-place MSD radix sort (IPS2Ra, see
    /// Axtmann et al., "Engineering In-place (Shared-memory) Sorting
    /// Algorithms", ACM TOPC 2022). The key of an element is the result of
    /// applying \a proj to it; it must be of an integral type other than
    /// bool. The order of elements with equal keys is not guaranteed to be
    /// preserved.
    ///
    /// \note   Complexity: O(N * W / 8) element moves and key extractions,
    ///         where N = std::distance(first, last) and W is the number of
    ///         bits in the key type, plus O(N log N) comparisons for the
    ///         small partitions sorted by comparison. Digits that are equal
    ///         in all keys are skipped. Auxiliary memory is independent of N.
    ///
    /// \tparam RandomIt    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     random access iterator.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity.
    ///
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param proj         Specifies the function (or function object) which
    ///                     yields the integral key of an element.
    ///
    /// The assignments in the parallel \a radix_sort algorithm invoked
    /// without an execution policy object execute in sequential order in the
    /// calling thread.
    ///
    template <typename RandomIt, typename Proj = hpx::identity>
    void radix_sort(RandomIt first, RandomIt last, Proj proj = Proj());

    ///////////////////////////////////////////////////////////////////////////
    /// Sorts the elements in the range [first, last) in ascending order of
    /// their integral keys, using an in-place MSD radix sort (IPS2Ra, see
    /// Axtmann et al., "Engineering In-place (Shared-memory) Sorting
    /// Algorithms", ACM TOPC 2022). The key of an element is the result of
    /// applying \a proj to it; it must be of an integral type other than
    /// bool. The order of elements with equal keys is not guaranteed to be
    /// preserved.
    ///
    /// \note   Complexity: O(N * W / 8) element moves and key extractions,
    ///         where N = std::distance(first, last) and W is the number of
    ///         bits in the key type, plus O(N log N) comparisons for the
    ///         small partitions sorted by comparison. Digits that are equal
    ///         in all keys are skipped. Auxiliary memory is O(P * B * 256)
    ///         elements, where P is the number of threads used and B the
    ///         block size (2 KiB worth of elements).
    ///
    /// \tparam ExPolicy    The type of the execution policy to use (deduced).
    ///                     It describes the manner in which the execution
    ///                     of the algorithm may be parallelized and the manner
    ///                     in which it executes the assignments.
    /// \tparam RandomIt    The type of the source iterators used (deduced).
    ///                     This iterator type must meet the requirements of a
    ///                     random access iterator.
    /// \tparam Proj        The type of an optional projection function. This
    ///                     defaults to \a hpx::identity.
    ///
    /// \param policy       The execution policy to use for the scheduling of
    ///                     the iterations.
    /// \param first        Refers to the beginning of the sequence of elements
    ///                     the algorithm will be applied to.
    /// \param last         Refers to the end of the sequence of elements the
    ///                     algorithm will be applied to.
    /// \param proj         Specifies the function (or function object) which
    ///                     yields the integral key of an element.
    ///
    /// The application of function objects in parallel algorithm
    /// invoked with an execution policy object of type
    /// \a sequenced_policy execute in sequential order in the
    /// calling thread.
    ///
    /// The application of function objects in parallel algorithm
    /// invoked with an execution policy object of type
    /// \a parallel_policy or \a parallel_task_policy are
    /// permitted to execute in an unordered fashion in unspecified
    /// threads, and indeterminately sequenced within each thread.
    ///
    /// \returns  The \a radix_sort algorithm returns a
    ///           \a hpx::future<void> if the execution policy is of
    ///           type
    ///           \a sequenced_task_policy or
    ///           \a parallel_task_policy and returns \a void
    ///           otherwise.
    ///
    template <typename ExPolicy, typename RandomIt,
        typename Proj = hpx::identity>
    hpx::parallel::util::detail::algorithm_result_t<ExPolicy> radix_sort(
        ExPolicy&& policy, RandomIt first, RandomIt last, Proj proj = Proj());

    // clang-format on
}    // namespace hpx

#else

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/projected.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/advance_to_sentinel.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/algorithms/detail/ips_distribution.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/sender_util.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <exception>
#include <iterator>
#include <limits>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

// Implementation overview: an MSD radix sort with 8-bit digits, structured
// along the IPS^2Ra pipeline. Every level classifies elements into digit
// buckets, computes bucket boundaries via prefix sums, permutes in place and
// recurses into the buckets. Large partitions are distributed with the
// blockwise engine from detail/ips_distribution.hpp (in parallel for parallel
// execution policies), smaller ones with an element-wise in-place
// permutation, and the smallest ones are handed to std::sort.

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    // Key extraction.
    //
    // We sort by an unsigned "lifted" key derived from the projected value.
    // Signed integers are mapped by flipping the sign bit so that ascending
    // unsigned order matches ascending signed order.

    HPX_CXX_CORE_EXPORT template <typename T>
    struct radix_key_traits
    {
        static_assert(std::is_integral_v<T> && !std::is_same_v<T, bool>,
            "hpx::radix_sort currently supports integral projected keys "
            "only.");

        using unsigned_t = std::make_unsigned_t<T>;
        static constexpr int bits = std::numeric_limits<unsigned_t>::digits;

        static constexpr unsigned_t lift(T v) noexcept
        {
            if constexpr (std::is_signed_v<T>)
            {
                constexpr unsigned_t sign_bit = unsigned_t{1} << (bits - 1);
                return static_cast<unsigned_t>(v) ^ sign_bit;
            }
            else
            {
                return v;
            }
        }
    };

    // Whether projecting the elements of Iter with Proj yields a key that
    // radix_sort can sort by: integral, but not bool.
    HPX_CXX_CORE_EXPORT template <typename Iter, typename Proj>
    using radix_sort_key_t = std::decay_t<decltype(HPX_INVOKE(
        std::declval<Proj&>(), *std::declval<Iter>()))>;

    HPX_CXX_CORE_EXPORT template <typename Iter, typename Proj>
    inline constexpr bool is_radix_sort_key_v =
        std::is_integral_v<radix_sort_key_t<Iter, Proj>> &&
        !std::is_same_v<radix_sort_key_t<Iter, Proj>, bool>;

    ///////////////////////////////////////////////////////////////////////////
    // Pipeline parameters.
    //
    // Radix width is fixed at 8 bits per pass (256 buckets) for the serial
    // implementation. The proposal calls out adaptive radix width as a
    // follow-up exploration.

    HPX_CXX_CORE_EXPORT inline constexpr int radix_sort_bits_per_pass = 8;
    HPX_CXX_CORE_EXPORT inline constexpr std::size_t radix_sort_buckets =
        std::size_t{1} << radix_sort_bits_per_pass;
    HPX_CXX_CORE_EXPORT inline constexpr std::size_t radix_sort_bucket_mask =
        radix_sort_buckets - 1;

    // Below this many elements, fall back to std::sort. Tuned on x86-64:
    // smaller values make the radix passes dominate (16 is ~2x slower for
    // 2^20 keys), larger ones pay for std::sort on buckets that another pass
    // would have split (512 is ~1.6x slower for 2^16 keys).
    HPX_CXX_CORE_EXPORT inline constexpr std::size_t radix_sort_base_case = 64;

    // At or above this many elements, use the blockwise distribution engine
    // instead of element-wise permutation. Below it the engine's buffers
    // (one block per bucket) would be mostly empty. Sequential timings were
    // insensitive to this value between 1/32 and 2 times K * B.
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr std::size_t radix_sort_blockwise_threshold =
        radix_sort_buckets * ips_default_block_size<T>;

    HPX_CXX_CORE_EXPORT template <typename UInt>
    constexpr std::size_t radix_sort_digit(UInt key, int shift) noexcept
    {
        return static_cast<std::size_t>(key >> shift) & radix_sort_bucket_mask;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Stage 1+2: build a per-bucket histogram, then turn it into prefix-sum
    // boundaries. `bucket_begin[k]` ends up holding the offset of bucket k's
    // start; `bucket_end[k]` holds the offset of its end (== begin of k+1).

    HPX_CXX_CORE_EXPORT template <typename RandomIt, typename Proj>
    void radix_sort_classify(RandomIt first, RandomIt last, int shift,
        Proj& proj, std::array<std::size_t, radix_sort_buckets>& bucket_begin,
        std::array<std::size_t, radix_sort_buckets>& bucket_end)
    {
        bucket_begin.fill(0);

        using value_type = typename std::iterator_traits<RandomIt>::value_type;
        using key_t = std::decay_t<decltype(HPX_INVOKE(
            proj, std::declval<value_type const&>()))>;
        using traits = radix_key_traits<key_t>;

        for (RandomIt it = first; it != last; ++it)
        {
            auto const k = traits::lift(HPX_INVOKE(proj, *it));
            ++bucket_begin[radix_sort_digit(k, shift)];
        }

        std::size_t sum = 0;
        for (std::size_t b = 0; b < radix_sort_buckets; ++b)
        {
            std::size_t const cnt = bucket_begin[b];
            bucket_begin[b] = sum;
            sum += cnt;
            bucket_end[b] = sum;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Stage 3 (small partitions): in-place permutation by cycle walking.
    //
    // For each bucket, walk a cursor through its segment. Any element whose
    // current digit places it in a different bucket gets swapped into that
    // bucket's write head; we keep swapping until the slot under the cursor
    // is correct, then advance. This is O(n) swaps per pass and uses only
    // the 256 head/tail pointers as auxiliary state.
    //
    // Every swap touches a random bucket, so for large partitions this is
    // dominated by cache misses; those go through ips_distribute instead.

    HPX_CXX_CORE_EXPORT template <typename RandomIt, typename Proj>
    void radix_sort_permute(RandomIt first, int shift, Proj& proj,
        std::array<std::size_t, radix_sort_buckets> const& bucket_begin,
        std::array<std::size_t, radix_sort_buckets> const& bucket_end)
    {
        using value_type = typename std::iterator_traits<RandomIt>::value_type;
        using key_t = std::decay_t<decltype(HPX_INVOKE(
            proj, std::declval<value_type const&>()))>;
        using traits = radix_key_traits<key_t>;

        std::array<std::size_t, radix_sort_buckets> head = bucket_begin;

        for (std::size_t b = 0; b < radix_sort_buckets; ++b)
        {
            while (head[b] < bucket_end[b])
            {
                value_type& slot = first[static_cast<std::ptrdiff_t>(head[b])];
                std::size_t const target = radix_sort_digit(
                    traits::lift(HPX_INVOKE(proj, slot)), shift);

                if (target == b)
                {
                    ++head[b];
                }
                else
                {
                    using std::swap;
                    swap(
                        slot, first[static_cast<std::ptrdiff_t>(head[target])]);
                    ++head[target];
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Base case: comparison sort on the lifted key.

    HPX_CXX_CORE_EXPORT template <typename RandomIt, typename Proj>
    void radix_sort_base_case_sort(RandomIt first, RandomIt last, Proj& proj)
    {
        using value_type = typename std::iterator_traits<RandomIt>::value_type;
        using key_t = std::decay_t<decltype(HPX_INVOKE(
            proj, std::declval<value_type const&>()))>;
        using traits = radix_key_traits<key_t>;

        std::sort(
            first, last, [&proj](value_type const& a, value_type const& b) {
                return traits::lift(HPX_INVOKE(proj, a)) <
                    traits::lift(HPX_INVOKE(proj, b));
            });
    }

    HPX_CXX_CORE_EXPORT using radix_sort_bounds =
        std::array<std::size_t, radix_sort_buckets + 1>;

    ///////////////////////////////////////////////////////////////////////////
    // One sequential distribution step on the digit at `shift`: reorder
    // [first, first + n) by digit and store the bucket boundaries in `bounds`.
    //
    // `buf` holds the blockwise engine's buffers. It is allocated on first
    // use and can be reused by later calls, since the buffers are empty again
    // once ips_distribute returns.

    HPX_CXX_CORE_EXPORT template <typename RandomIt, typename Proj>
    void radix_sort_partition(RandomIt first, std::size_t n, int shift,
        Proj& proj,
        std::unique_ptr<ips_distribution_buffers<
            typename std::iterator_traits<RandomIt>::value_type>>& buf,
        radix_sort_bounds& bounds)
    {
        using value_type = typename std::iterator_traits<RandomIt>::value_type;
        using key_t = std::decay_t<decltype(HPX_INVOKE(
            proj, std::declval<value_type const&>()))>;
        using traits = radix_key_traits<key_t>;

        if (n >= radix_sort_blockwise_threshold<value_type>)
        {
            if (!buf)
            {
                buf = std::make_unique<ips_distribution_buffers<value_type>>(
                    radix_sort_buckets, ips_default_block_size<value_type>);
            }

            ips_distribute(
                first, n,
                [&proj, shift](value_type const& v) {
                    return radix_sort_digit(
                        traits::lift(HPX_INVOKE(proj, v)), shift);
                },
                *buf);

            std::copy(buf->bucket_start.begin(), buf->bucket_start.end(),
                bounds.begin());
            return;
        }

        std::array<std::size_t, radix_sort_buckets> bucket_begin{};
        std::array<std::size_t, radix_sort_buckets> bucket_end{};
        radix_sort_classify(first, first + static_cast<std::ptrdiff_t>(n),
            shift, proj, bucket_begin, bucket_end);

        // Lazy pass skipping (proposal section 5): if every element fell in
        // the same bucket this pass would do no useful separation.
        bool single_bucket = false;
        for (std::size_t b = 0; b < radix_sort_buckets; ++b)
        {
            if (bucket_end[b] - bucket_begin[b] == n)
            {
                single_bucket = true;
                break;
            }
        }

        if (!single_bucket)
            radix_sort_permute(first, shift, proj, bucket_begin, bucket_end);

        std::copy(bucket_begin.begin(), bucket_begin.end(), bounds.begin());
        bounds[radix_sort_buckets] = n;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Sequential MSD recursion: distribute on the current digit, then recurse
    // on each non-trivial bucket at the next-lower digit. Buckets of size <=
    // radix_sort_base_case go through std::sort.

    HPX_CXX_CORE_EXPORT template <typename RandomIt, typename Proj>
    void radix_sort_serial(RandomIt first, RandomIt last, int shift, Proj& proj,
        std::unique_ptr<ips_distribution_buffers<
            typename std::iterator_traits<RandomIt>::value_type>>& buf)
    {
        std::size_t const n = static_cast<std::size_t>(last - first);
        if (n < 2)
            return;

        if (shift < 0 || n <= radix_sort_base_case)
        {
            radix_sort_base_case_sort(first, last, proj);
            return;
        }

        radix_sort_bounds bounds;
        radix_sort_partition(first, n, shift, proj, buf, bounds);

        int const next_shift = shift - radix_sort_bits_per_pass;
        for (std::size_t b = 0; b < radix_sort_buckets; ++b)
        {
            if (bounds[b + 1] - bounds[b] < 2)
                continue;
            radix_sort_serial(first + static_cast<std::ptrdiff_t>(bounds[b]),
                first + static_cast<std::ptrdiff_t>(bounds[b + 1]), next_shift,
                proj, buf);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Leading-digit skipping and sortedness check.
    //
    // Before sorting, one read-only pass over the keys collects their minimum
    // and maximum and whether they are already in order. The highest bit in
    // which any two keys differ is the highest set bit of min ^ max, so the
    // sort can start at the digit containing it: all digits above are shared
    // by every key and would each cost a full pass that puts everything into
    // one bucket. If the keys are already sorted (in particular, if they are
    // all equal) there is nothing to do.

    HPX_CXX_CORE_EXPORT template <typename UInt>
    struct radix_sort_key_summary
    {
        UInt min_key;
        UInt max_key;
        UInt first_key;
        UInt last_key;
        bool sorted;

        // Summary of the concatenation of the ranges described by *this and
        // `next`, in that order.
        constexpr void append(radix_sort_key_summary const& next) noexcept
        {
            min_key = (std::min) (min_key, next.min_key);
            max_key = (std::max) (max_key, next.max_key);
            sorted = sorted && next.sorted && last_key <= next.first_key;
            last_key = next.last_key;
        }

        // The shift of the first digit to sort by, or -1 if the keys are
        // already sorted.
        constexpr int start_shift() const noexcept
        {
            if (sorted)
                return -1;
            UInt const diff = min_key ^ max_key;
            HPX_ASSERT(diff != 0);
            int const top_bit = static_cast<int>(std::bit_width(diff)) - 1;
            return top_bit / radix_sort_bits_per_pass *
                radix_sort_bits_per_pass;
        }
    };

    // Key summary of [first, first + n), n > 0.
    HPX_CXX_CORE_EXPORT template <typename RandomIt, typename Proj>
    auto radix_sort_summarize_keys(RandomIt first, std::size_t n, Proj& proj)
    {
        using value_type = typename std::iterator_traits<RandomIt>::value_type;
        using key_t = std::decay_t<decltype(HPX_INVOKE(
            proj, std::declval<value_type const&>()))>;
        using traits = radix_key_traits<key_t>;
        using unsigned_t = typename traits::unsigned_t;

        HPX_ASSERT(n != 0);
        unsigned_t const k0 = traits::lift(HPX_INVOKE(proj, *first));
        radix_sort_key_summary<unsigned_t> summary{k0, k0, k0, k0, true};
        unsigned_t prev = k0;
        for (std::size_t i = 1; i != n; ++i)
        {
            unsigned_t const k = traits::lift(
                HPX_INVOKE(proj, *(first + static_cast<std::ptrdiff_t>(i))));
            summary.min_key = (std::min) (summary.min_key, k);
            summary.max_key = (std::max) (summary.max_key, k);
            summary.sorted = summary.sorted && prev <= k;
            prev = k;
        }
        summary.last_key = prev;
        return summary;
    }

    // Parallel version of the above over `chunks` slices; returns the start
    // shift (or -1 if the keys are already sorted).
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename RandomIt,
        typename Proj>
    int radix_sort_parallel_start_shift(ExPolicy& policy, RandomIt first,
        std::size_t n, Proj& proj, std::size_t chunks)
    {
        HPX_ASSERT(n != 0 && chunks != 0 && chunks <= n);
        using summary_t = decltype(radix_sort_summarize_keys(first, n, proj));
        std::vector<summary_t> summaries(chunks);

        execution::bulk_sync_execute(
            policy.executor(),
            [&](std::size_t i) {
                std::size_t const b = n * i / chunks;
                std::size_t const e = n * (i + 1) / chunks;
                summaries[i] = radix_sort_summarize_keys(
                    first + static_cast<std::ptrdiff_t>(b), e - b, proj);
            },
            hpx::util::iterator_range(
                hpx::util::counting_iterator<std::size_t>(0),
                hpx::util::counting_iterator<std::size_t>(chunks)));

        summary_t summary = summaries[0];
        for (std::size_t i = 1; i != chunks; ++i)
            summary.append(summaries[i]);
        return summary.start_shift();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Parallel recursion.
    //
    // A range of n elements with a budget of `threads` workers is distributed
    // by ips_distribute_parallel when it is large enough to give every worker
    // a reasonable stripe, and sequentially otherwise. The buckets are then
    // sorted as tasks: a bucket holding at least 1/threads of the elements
    // (skewed inputs) recurses in parallel with a proportional share of the
    // workers; the remaining buckets are grouped into tasks of roughly n / (4
    // * threads) elements that are each sorted sequentially.
    //
    // With `skip_leading_digits`, the key range is computed first so that
    // digits shared by all keys are skipped. The top level always does this;
    // in the recursion it is done for the large buckets only, as those
    // indicate skewed keys, which tend to share further digits.

    // Smallest amount of work, in elements, given to a task. Ranges smaller
    // than two tasks are sorted sequentially. With 16 threads, 2^14 sorts
    // 2^16 keys ~2x faster than 2^16 did; 2^17 and up were slower throughout.
    HPX_CXX_CORE_EXPORT inline constexpr std::size_t radix_sort_min_task_size =
        std::size_t(1) << 14;

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename RandomIt,
        typename Proj>
    void radix_sort_parallel(ExPolicy& policy, RandomIt first, std::size_t n,
        int shift, Proj& proj, std::size_t threads, bool skip_leading_digits)
    {
        using value_type = typename std::iterator_traits<RandomIt>::value_type;
        using key_t = std::decay_t<decltype(HPX_INVOKE(
            proj, std::declval<value_type const&>()))>;
        using traits = radix_key_traits<key_t>;
        using serial_buffers =
            std::unique_ptr<ips_distribution_buffers<value_type>>;

        if (threads <= 1 || shift < 0 || n < 2 * radix_sort_min_task_size)
        {
            serial_buffers buf;
            radix_sort_serial(first, first + static_cast<std::ptrdiff_t>(n),
                shift, proj, buf);
            return;
        }

        if (skip_leading_digits)
        {
            std::size_t const chunks = (std::min) (threads,
                (std::max) (std::size_t(1), n / radix_sort_min_task_size));
            shift =
                radix_sort_parallel_start_shift(policy, first, n, proj, chunks);
            if (shift < 0)
                return;
        }

        radix_sort_bounds bounds;

        // Every worker should classify at least one block per bucket on
        // average, otherwise the per-worker buffers dominate.
        std::size_t const workers = (std::min) (threads,
            n / radix_sort_blockwise_threshold<value_type>);
        if (workers >= 2)
        {
            ips_parallel_distribution_buffers<value_type> buf(workers,
                radix_sort_buckets, ips_default_block_size<value_type>);

            auto run = [&policy](std::size_t count, auto&& f) {
                execution::bulk_sync_execute(policy.executor(), f,
                    hpx::util::iterator_range(
                        hpx::util::counting_iterator<std::size_t>(0),
                        hpx::util::counting_iterator<std::size_t>(count)));
            };

            ips_distribute_parallel(
                first, n,
                [&proj, shift](value_type const& v) {
                    return radix_sort_digit(
                        traits::lift(HPX_INVOKE(proj, v)), shift);
                },
                buf, run);

            std::copy(buf.bucket_start.begin(), buf.bucket_start.end(),
                bounds.begin());
        }
        else
        {
            serial_buffers buf;
            radix_sort_partition(first, n, shift, proj, buf, bounds);
        }

        int const next_shift = shift - radix_sort_bits_per_pass;
        std::size_t const grain =
            (std::max) (n / (4 * threads), radix_sort_min_task_size);

        // All tasks are joined before returning, so they can capture locals
        // by reference.
        std::vector<hpx::future<void>> tasks;

        std::size_t group_first = 0;
        std::size_t group_size = 0;
        auto flush_group = [&](std::size_t group_last) {
            if (group_size != 0)
            {
                tasks.push_back(execution::async_execute(
                    policy.executor(), [&, group_first, group_last]() {
                        serial_buffers buf;
                        for (std::size_t b = group_first; b != group_last; ++b)
                        {
                            if (bounds[b + 1] - bounds[b] < 2)
                                continue;
                            radix_sort_serial(
                                first + static_cast<std::ptrdiff_t>(bounds[b]),
                                first +
                                    static_cast<std::ptrdiff_t>(bounds[b + 1]),
                                next_shift, proj, buf);
                        }
                    }));
            }
            group_first = group_last;
            group_size = 0;
        };

        for (std::size_t b = 0; b != radix_sort_buckets; ++b)
        {
            std::size_t const size = bounds[b + 1] - bounds[b];
            std::size_t const share = size * threads / n;
            if (share >= 2 && size >= 2 * radix_sort_min_task_size)
            {
                flush_group(b);
                tasks.push_back(execution::async_execute(
                    policy.executor(), [&, b, size, share]() {
                        radix_sort_parallel(policy,
                            first + static_cast<std::ptrdiff_t>(bounds[b]),
                            size, next_shift, proj, share, true);
                    }));
                group_first = b + 1;
                continue;
            }

            group_size += size;
            if (group_size >= grain)
                flush_group(b + 1);
        }
        flush_group(radix_sort_buckets);

        hpx::wait_all(tasks);
        for (auto& task : tasks)
            task.get();    // rethrow the first exception, if any
    }

    ///////////////////////////////////////////////////////////////////////////
    // Algorithm entry struct.

    HPX_CXX_CORE_EXPORT template <typename RandomIt>
    struct radix_sort_algo
      : public algorithm<radix_sort_algo<RandomIt>, RandomIt>
    {
        constexpr radix_sort_algo() noexcept
          : algorithm<radix_sort_algo, RandomIt>("radix_sort")
        {
        }

        template <typename Proj>
        static constexpr int start_shift() noexcept
        {
            using value_type =
                typename std::iterator_traits<RandomIt>::value_type;
            using key_t = std::decay_t<decltype(HPX_INVOKE(
                std::declval<Proj&>(), std::declval<value_type const&>()))>;
            return radix_key_traits<key_t>::bits - radix_sort_bits_per_pass;
        }

        template <typename ExPolicy, typename Sent, typename Proj>
        static RandomIt sequential(
            ExPolicy, RandomIt first, Sent last, Proj proj)
        {
            using value_type =
                typename std::iterator_traits<RandomIt>::value_type;

            RandomIt const last_iter = detail::advance_to_sentinel(first, last);
            std::size_t const n = static_cast<std::size_t>(last_iter - first);
            if (n < 2)
                return last_iter;

            // Skipping leading digits only pays off where a pass is
            // expensive; small ranges go straight to the base case.
            int shift = start_shift<Proj>();
            if (n > radix_sort_base_case)
            {
                shift = radix_sort_summarize_keys(first, n, proj).start_shift();
                if (shift < 0)
                    return last_iter;
            }

            std::unique_ptr<ips_distribution_buffers<value_type>> buf;
            radix_sort_serial(first, last_iter, shift, proj, buf);
            return last_iter;
        }

        template <typename ExPolicy, typename Sent, typename Proj>
        static util::detail::algorithm_result_t<ExPolicy, RandomIt> parallel(
            ExPolicy&& policy, RandomIt first, Sent last, Proj proj)
        {
            using algorithm_result =
                util::detail::algorithm_result<ExPolicy, RandomIt>;

            RandomIt const last_iter = detail::advance_to_sentinel(first, last);
            auto sort = [policy, first, last_iter,
                            proj = HPX_MOVE(proj)]() mutable -> RandomIt {
                std::size_t const n =
                    static_cast<std::size_t>(last_iter - first);
                if (n < 2)
                    return last_iter;

                std::size_t const cores =
                    hpx::execution::experimental::processing_units_count(
                        policy.parameters(), policy.executor(),
                        hpx::chrono::null_duration, n);

                // radix_sort_parallel skips the leading digits itself unless
                // the range is too small to be split at all.
                int shift = start_shift<Proj>();
                if (cores <= 1 || n < 2 * radix_sort_min_task_size)
                {
                    if (n > radix_sort_base_case)
                    {
                        shift = radix_sort_summarize_keys(first, n, proj)
                                    .start_shift();
                        if (shift < 0)
                            return last_iter;
                    }
                }

                radix_sort_parallel(policy, first, n, shift, proj, cores, true);
                return last_iter;
            };

            try
            {
                if constexpr (hpx::is_async_execution_policy_v<ExPolicy>)
                {
                    return algorithm_result::get(execution::async_execute(
                        policy.executor(), HPX_MOVE(sort)));
                }
                else
                {
                    return algorithm_result::get(sort());
                }
            }
            catch (...)
            {
                return algorithm_result::get(
                    detail::handle_exception<ExPolicy, RandomIt>::call(
                        std::current_exception()));
            }
        }
    };

}    // namespace hpx::parallel::detail

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // CPO for hpx::radix_sort.
    HPX_CXX_CORE_EXPORT inline constexpr struct radix_sort_t final
      : hpx::detail::tag_dispatch<radix_sort_t,
            hpx::detail::tag_parallel_algorithm<radix_sort_t>>
    {
        template <typename RandomIt, typename Proj = hpx::identity>
            requires(hpx::traits::is_iterator_v<RandomIt> &&
                hpx::parallel::detail::is_radix_sort_key_v<RandomIt, Proj>)
        static void invoke_default(
            RandomIt first, RandomIt last, Proj proj = Proj())
        {
            static_assert(std::random_access_iterator<RandomIt>,
                "hpx::radix_sort requires a random access iterator.");

            hpx::parallel::detail::radix_sort_algo<RandomIt>().call(
                hpx::execution::seq, first, last, HPX_MOVE(proj));
        }

        template <typename ExPolicy, typename RandomIt,
            typename Proj = hpx::identity>
            requires(hpx::is_execution_policy_v<ExPolicy> &&
                hpx::traits::is_iterator_v<RandomIt> &&
                hpx::parallel::detail::is_radix_sort_key_v<RandomIt, Proj>)
        static parallel::util::detail::algorithm_result_t<ExPolicy>
        invoke_default(ExPolicy&& policy, RandomIt first, RandomIt last,
            Proj proj = Proj())
        {
            static_assert(std::random_access_iterator<RandomIt>,
                "hpx::radix_sort requires a random access iterator.");

            using result_type =
                typename hpx::parallel::util::detail::algorithm_result<
                    ExPolicy>::type;

            return hpx::util::void_guard<result_type>(),
                   hpx::parallel::detail::radix_sort_algo<RandomIt>().call(
                       HPX_FORWARD(ExPolicy, policy), first, last,
                       HPX_MOVE(proj));
        }
    } radix_sort{};
}    // namespace hpx

#endif    // DOXYGEN
