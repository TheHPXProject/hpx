//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)
#include <hpx/modules/execution.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/advance_and_get_distance.hpp>
#include <hpx/parallel/algorithms/merge.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <cstddef>
#include <functional>
#include <iterator>
#include <type_traits>
#include <utility>

// The kernel needs vector_pack_min/max/generate, which these backends provide.
// The emulation runs each pack operation as a loop over the lanes, which
// makes the kernel slower than the scalar merge; it is enabled there anyway,
// like the other datapar algorithms, so that it is tested with MSVC.
#if defined(HPX_HAVE_DATAPAR_STD_EXPERIMENTAL_SIMD) ||                         \
    defined(HPX_HAVE_DATAPAR_EMULATION) || defined(HPX_HAVE_DATAPAR_EVE)
#define HPX_DATAPAR_MERGE_KERNEL 1
#endif

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    // The order a comparator gives on packs: comparators known to compare
    // lane values with < (ascending) or > (descending); others are not
    // vectorized (datapar_merge_order_v is 0).
    HPX_CXX_CORE_EXPORT template <typename Comp>
    struct datapar_merge_order : std::integral_constant<int, 0>
    {
    };

    template <typename T>
    struct datapar_merge_order<std::less<T>> : std::integral_constant<int, 1>
    {
    };

    template <typename T>
    struct datapar_merge_order<std::greater<T>> : std::integral_constant<int, 2>
    {
    };

    template <>
    struct datapar_merge_order<std::ranges::less>
      : std::integral_constant<int, 1>
    {
    };

    template <>
    struct datapar_merge_order<std::ranges::greater>
      : std::integral_constant<int, 2>
    {
    };

    template <>
    struct datapar_merge_order<hpx::parallel::detail::less>
      : std::integral_constant<int, 1>
    {
    };

    template <>
    struct datapar_merge_order<hpx::parallel::detail::greater>
      : std::integral_constant<int, 2>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Comp>
    inline constexpr int datapar_merge_order_v =
        datapar_merge_order<std::decay_t<Comp>>::value;

    HPX_CXX_CORE_EXPORT template <typename Proj>
    inline constexpr bool datapar_merge_identity_v =
        std::is_same_v<std::decay_t<Proj>, hpx::identity> ||
        std::is_same_v<std::decay_t<Proj>, std::identity>;

    ///////////////////////////////////////////////////////////////////////////
    // The pack type a policy merges T in: the native pack for simd and
    // par_simd, fixed_size_simd<T, N> for (par_)fixed_size_simd<N>.
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename T>
    using datapar_merge_pack_t = traits::vector_pack_type_t<T,
        hpx::execution::policy_traits<std::decay_t<ExPolicy>>::num_lanes>;

    // Whether the vector kernel merges these ranges. It needs contiguous
    // ranges of the same integral type (equal elements are then
    // indistinguishable, so the kernel's order of equal elements from both
    // inputs, which differs from std::merge's, can't be observed; floating
    // point (-0.0 and +0.0) and projections could observe it), a comparator
    // known to work on packs and a pack of more than one lane.
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter1,
        typename Iter2, typename OutIter, typename Comp, typename Proj1,
        typename Proj2>
    constexpr bool datapar_merge_applies() noexcept
    {
#if defined(HPX_DATAPAR_MERGE_KERNEL)
        if constexpr (hpx::traits::is_contiguous_iterator_v<Iter1> &&
            hpx::traits::is_contiguous_iterator_v<Iter2> &&
            hpx::traits::is_contiguous_iterator_v<OutIter>)
        {
            using T = std::iter_value_t<Iter1>;
            if constexpr (std::is_same_v<T, std::iter_value_t<Iter2>> &&
                std::is_same_v<T, std::iter_value_t<OutIter>> &&
                std::is_integral_v<T> && !std::is_same_v<T, bool> &&
                datapar_merge_order_v<Comp> != 0 &&
                datapar_merge_identity_v<Proj1> &&
                datapar_merge_identity_v<Proj2>)
            {
                return traits::is_vector_pack_v<
                    datapar_merge_pack_t<ExPolicy, T>>;
            }
        }
#endif
        return false;
    }

#if defined(HPX_DATAPAR_MERGE_KERNEL)
    ///////////////////////////////////////////////////////////////////////////
    // Merge of two sorted ranges of integers with packs V: one pack is loaded
    // from each input and the two are merged with a bitonic network (lane-wise
    // min and max, then sorting each half); the lower pack is written, the
    // upper one kept, and the next pack is loaded from the input whose next
    // element comes first. The elements left when one input has less than a
    // pack go through the scalar merge.
    HPX_CXX_CORE_EXPORT template <typename V, bool Descending>
    struct datapar_merge_kernel
    {
        using value_type = typename V::value_type;
        static constexpr std::size_t size = traits::vector_pack_size_v<V>;

        // lane-wise first and second in the merge order
        static V first_of(V const& lhs, V const& rhs) noexcept
        {
            if constexpr (Descending)
                return traits::vector_pack_max(lhs, rhs);
            else
                return traits::vector_pack_min(lhs, rhs);
        }

        static V second_of(V const& lhs, V const& rhs) noexcept
        {
            if constexpr (Descending)
                return traits::vector_pack_min(lhs, rhs);
            else
                return traits::vector_pack_max(lhs, rhs);
        }

        // sort a bitonic pack: the stages of distance Distance, Distance / 2,
        // ..., 1 compare lane i with lane i ^ distance
        template <std::size_t Distance>
        static V sort_bitonic(V v) noexcept
        {
            if constexpr (Distance == 0)
            {
                return v;
            }
            else
            {
                V partner = traits::vector_pack_generate<V>(
                    [&](auto i) { return traits::get(v, i ^ Distance); });
                V lower = first_of(v, partner);
                V upper = second_of(v, partner);
                V merged = traits::vector_pack_generate<V>([&](auto i) {
                    if constexpr ((decltype(i)::value & Distance) != 0)
                        return traits::get(upper, i);
                    else
                        return traits::get(lower, i);
                });
                return sort_bitonic<Distance / 2>(merged);
            }
        }

        template <typename Comp>
        static value_type* call(value_type const* first1,
            value_type const* last1, value_type const* first2,
            value_type const* last2, value_type* dest, Comp& comp)
        {
            if (last1 - first1 < static_cast<std::ptrdiff_t>(size) ||
                last2 - first2 < static_cast<std::ptrdiff_t>(size))
            {
                return sequential_merge(first1, last1, first2, last2, dest,
                    comp, hpx::identity_v, hpx::identity_v)
                    .out;
            }

            V keep = traits::vector_pack_load<V, value_type>::unaligned(first1);
            V next = traits::vector_pack_load<V, value_type>::unaligned(first2);
            first1 += size;
            first2 += size;
            while (true)
            {
                // keep is sorted, next is sorted: reversed, next makes the
                // two a bitonic sequence
                V reversed = traits::vector_pack_generate<V>(
                    [&](auto i) { return traits::get(next, size - 1 - i); });
                V lower = sort_bitonic<size / 2>(first_of(keep, reversed));
                keep = sort_bitonic<size / 2>(second_of(keep, reversed));
                traits::vector_pack_store<V, value_type>::unaligned(
                    lower, dest);
                dest += size;

                if (last1 - first1 < static_cast<std::ptrdiff_t>(size) ||
                    last2 - first2 < static_cast<std::ptrdiff_t>(size))
                {
                    break;
                }
                bool const take2 =
                    static_cast<bool>(HPX_INVOKE(comp, *first2, *first1));
                value_type const* source = take2 ? first2 : first1;
                next =
                    traits::vector_pack_load<V, value_type>::unaligned(source);
                (take2 ? first2 : first1) += size;
            }

            // merge the kept pack with the rest of the input that has less
            // than a pack left, then the result with the rest of the other
            value_type kept[size];
            value_type* kept_ptr = kept;
            traits::vector_pack_store<V, value_type>::unaligned(keep, kept_ptr);

            value_type buffer[2 * size];
            if (last1 - first1 < static_cast<std::ptrdiff_t>(size))
            {
                value_type* end = sequential_merge(kept, kept + size, first1,
                    last1, buffer, comp, hpx::identity_v, hpx::identity_v)
                                      .out;
                return sequential_merge(static_cast<value_type const*>(buffer),
                    static_cast<value_type const*>(end), first2, last2, dest,
                    comp, hpx::identity_v, hpx::identity_v)
                    .out;
            }
            value_type* end = sequential_merge(kept, kept + size, first2, last2,
                buffer, comp, hpx::identity_v, hpx::identity_v)
                                  .out;
            return sequential_merge(first1, last1,
                static_cast<value_type const*>(buffer),
                static_cast<value_type const*>(end), dest, comp,
                hpx::identity_v, hpx::identity_v)
                .out;
        }
    };
#endif

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter1,
        typename Sent1, typename Iter2, typename Sent2, typename OutIter,
        typename Comp, typename Proj1, typename Proj2>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    constexpr util::in_in_out_result<Iter1, Iter2, OutIter> hpx_invoke(
        sequential_merge_t<ExPolicy>, Iter1 first1, Sent1 last1, Iter2 first2,
        Sent2 last2, OutIter dest, Comp&& comp, Proj1&& proj1, Proj2&& proj2)
    {
#if defined(HPX_DATAPAR_MERGE_KERNEL)
        if constexpr (datapar_merge_applies<ExPolicy, Iter1, Iter2, OutIter,
                          Comp, Proj1, Proj2>())
        {
            using T = std::iter_value_t<Iter1>;
            using kernel =
                datapar_merge_kernel<datapar_merge_pack_t<ExPolicy, T>,
                    datapar_merge_order_v<Comp> == 2>;

            auto end1 = first1;
            auto const len1 = detail::advance_and_get_distance(end1, last1);
            auto end2 = first2;
            auto const len2 = detail::advance_and_get_distance(end2, last2);

            T const* const begin1 = hpx::util::get_unwrapped(first1);
            T const* const begin2 = hpx::util::get_unwrapped(first2);
            kernel::call(begin1, begin1 + len1, begin2, begin2 + len2,
                hpx::util::get_unwrapped(dest), comp);

            return {end1, end2, std::next(dest, len1 + len2)};
        }
        else
#endif
        {
            return sequential_merge(first1, last1, first2, last2, dest,
                HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj1, proj1),
                HPX_FORWARD(Proj2, proj2));
        }
    }
}    // namespace hpx::parallel::detail

#undef HPX_DATAPAR_MERGE_KERNEL

#endif
