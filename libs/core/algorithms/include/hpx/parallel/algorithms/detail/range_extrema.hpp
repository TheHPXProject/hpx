//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/util/partitioner.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace hpx::parallel::detail {
    /// \cond NOINTERNAL

    HPX_CXX_CORE_EXPORT enum class range_extrema_kind {
        minimum,
        maximum,
        minimum_maximum
    };

    HPX_CXX_CORE_EXPORT template <typename I, range_extrema_kind Kind>
    struct range_extrema final
      : algorithm<range_extrema<I, Kind>,
            std::conditional_t<Kind == range_extrema_kind::minimum_maximum,
                util::min_max_result<I>, I>>
    {
        using result_type =
            std::conditional_t<Kind == range_extrema_kind::minimum_maximum,
                util::min_max_result<I>, I>;

        range_extrema()
          : algorithm<range_extrema, result_type>("range_extrema")
        {
        }

        template <typename ExPolicy, typename Comp, typename Proj>
        static result_type sequential(
            ExPolicy, I first, I last, Comp comp, Proj proj)
        {
            if constexpr (Kind == range_extrema_kind::minimum)
                return std::ranges::min_element(first, last, comp, proj);
            else if constexpr (Kind == range_extrema_kind::maximum)
                return std::ranges::max_element(first, last, comp, proj);
            else
            {
                auto result =
                    std::ranges::minmax_element(first, last, comp, proj);
                return {result.min, result.max};
            }
        }

        template <typename ExPolicy, typename Comp, typename Proj>
        static decltype(auto) parallel(
            ExPolicy&& policy, I first, I last, Comp comp, Proj proj)
        {
            auto const count = static_cast<std::size_t>(last - first);
            // Pair boundaries keep the minmax comparison count within
            // floor(3 * (N - 1) / 2), including the final reduction.
            constexpr std::size_t stride =
                Kind == range_extrema_kind::minimum_maximum ? 2 : 1;
            auto const groups = count / stride + (count % stride != 0);
            using index_iterator = hpx::util::counting_iterator<std::size_t>;
            auto scan = [=](index_iterator index, std::size_t size) mutable {
                auto const offset = *index * stride;
                auto const length = (std::min) (size * stride, count - offset);
                return sequential(hpx::execution::seq, first + offset,
                    first + offset + length, comp, proj);
            };
            auto combine = [=](auto&& parts) mutable -> result_type {
                if (parts.empty())
                {
                    if constexpr (Kind == range_extrema_kind::minimum_maximum)
                        return {last, last};
                    else
                        return last;
                }
                auto result = parts.front();
                std::for_each(std::next(parts.begin()), parts.end(),
                    [&](result_type const& current) {
                        if constexpr (Kind ==
                            range_extrema_kind::minimum_maximum)
                        {
                            if (HPX_INVOKE(comp, HPX_INVOKE(proj, *current.min),
                                    HPX_INVOKE(proj, *result.min)))
                                result.min = current.min;
                            if (!HPX_INVOKE(comp,
                                    HPX_INVOKE(proj, *current.max),
                                    HPX_INVOKE(proj, *result.max)))
                                result.max = current.max;
                        }
                        else if constexpr (Kind == range_extrema_kind::minimum)
                        {
                            if (HPX_INVOKE(comp, HPX_INVOKE(proj, *current),
                                    HPX_INVOKE(proj, *result)))
                                result = current;
                        }
                        else
                        {
                            if (HPX_INVOKE(comp, HPX_INVOKE(proj, *result),
                                    HPX_INVOKE(proj, *current)))
                                result = current;
                        }
                    });
                return result;
            };
            return util::partitioner<ExPolicy, result_type, result_type>::call(
                HPX_FORWARD(ExPolicy, policy), index_iterator(0), groups,
                HPX_MOVE(scan), hpx::unwrapping(HPX_MOVE(combine)));
        }
    };
    /// \endcond
}    // namespace hpx::parallel::detail
