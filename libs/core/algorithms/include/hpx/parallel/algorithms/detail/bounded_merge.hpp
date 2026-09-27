//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/util/detail/clear_container.hpp>
#include <hpx/parallel/util/partitioner.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <algorithm>
#include <bit>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {
    /// \cond NOINTERNAL

    HPX_CXX_CORE_EXPORT template <typename I1, typename I2, typename O>
    struct bounded_merge final
      : algorithm<bounded_merge<I1, I2, O>, util::in_in_out_result<I1, I2, O>>
    {
        using result_type = util::in_in_out_result<I1, I2, O>;
        using difference_type = std::common_type_t<std::iter_difference_t<I1>,
            std::iter_difference_t<I2>, std::iter_difference_t<O>>;

        bounded_merge()
          : algorithm<bounded_merge, result_type>("merge")
        {
        }

        // Locate the input positions after rank elements in the stable merge.
        // Equal elements in the first range precede those in the second range.
        template <typename Comp, typename Proj1, typename Proj2>
        static std::pair<I1, I2> split(I1 first1, I1 last1, I2 first2, I2 last2,
            difference_type rank, Comp& comp, Proj1& proj1, Proj2& proj2)
        {
            auto const size1 = difference_type(last1 - first1);
            auto const size2 = difference_type(last2 - first2);
            auto low = (std::max) (difference_type(0), rank - size2);
            auto high = (std::min) (rank, size1);
            while (low < high)
            {
                auto const middle = low + (high - low) / 2;
                auto const other = rank - middle;
                if (other != 0 && middle != size1 &&
                    !HPX_INVOKE(comp, HPX_INVOKE(proj2, *(first2 + other - 1)),
                        HPX_INVOKE(proj1, *(first1 + middle))))
                {
                    low = middle + 1;
                }
                else
                {
                    high = middle;
                }
            }
            return {first1 + low, first2 + (rank - low)};
        }

        static difference_type size(
            I1 first1, I1 last1, I2 first2, I2 last2, O dest, O dest_last)
        {
            auto const capacity = difference_type(dest_last - dest);
            auto const from_first =
                (std::min) (difference_type(last1 - first1), capacity);
            return from_first +
                (std::min) (difference_type(last2 - first2),
                    capacity - from_first);
        }

        template <typename ExPolicy, typename Comp, typename Proj1,
            typename Proj2>
        static result_type sequential(ExPolicy, I1 first1, I1 last1, I2 first2,
            I2 last2, O dest, O dest_last, Comp comp, Proj1 proj1, Proj2 proj2)
        {
            auto const count =
                size(first1, last1, first2, last2, dest, dest_last);
            auto end =
                split(first1, last1, first2, last2, count, comp, proj1, proj2);
            auto result =
                std::ranges::merge(first1, end.first, first2, end.second, dest,
                    HPX_MOVE(comp), HPX_MOVE(proj1), HPX_MOVE(proj2));
            return {result.in1, result.in2, result.out};
        }

        template <typename ExPolicy, typename Comp, typename Proj1,
            typename Proj2>
        static decltype(auto) parallel(ExPolicy&& policy, I1 first1, I1 last1,
            I2 first2, I2 last2, O dest, O dest_last, Comp comp, Proj1 proj1,
            Proj2 proj2)
        {
            auto const count =
                size(first1, last1, first2, last2, dest, dest_last);
            // Each tile contains at least log2(N) elements. This bounds the
            // total binary-search work by O(N), even with one tile per task.
            auto const tile_size = (std::max) (difference_type(1),
                difference_type(
                    std::bit_width(static_cast<std::size_t>(count))));
            auto const tiles = count / tile_size + (count % tile_size != 0);
            using index_iterator =
                hpx::util::counting_iterator<difference_type>;
            auto merge_partition = [=](index_iterator index,
                                       std::size_t length) mutable {
                auto const offset = *index * tile_size;
                auto const end_tile = *index + difference_type(length);
                auto const end_offset =
                    end_tile == tiles ? count : end_tile * tile_size;
                auto begin = split(
                    first1, last1, first2, last2, offset, comp, proj1, proj2);
                auto end = split(first1, last1, first2, last2, end_offset, comp,
                    proj1, proj2);
                std::ranges::merge(begin.first, end.first, begin.second,
                    end.second, dest + offset, comp, proj1, proj2);
            };
            auto finish = [=](auto&&... work) mutable -> result_type {
                util::detail::clear_container(work...);
                auto end = split(
                    first1, last1, first2, last2, count, comp, proj1, proj2);
                return {end.first, end.second, dest + count};
            };
            return util::partitioner<ExPolicy, result_type, void>::call(
                HPX_FORWARD(ExPolicy, policy), index_iterator(0), tiles,
                HPX_MOVE(merge_partition), HPX_MOVE(finish));
        }
    };
    /// \endcond
}    // namespace hpx::parallel::detail
