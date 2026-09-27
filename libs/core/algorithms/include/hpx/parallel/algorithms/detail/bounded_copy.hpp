//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/parallel/algorithms/detail/dispatch.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/clear_container.hpp>
#include <hpx/parallel/util/loop.hpp>
#include <hpx/parallel/util/result_types.hpp>
#include <hpx/parallel/util/scan_partitioner.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

namespace hpx::parallel::detail {
    /// \cond NOINTERNAL

    // A stable bounded compaction. Selection takes an input iterator, allowing
    // both unary filters and adjacent-element comparisons to share the scan.
    // Each input is selected at most once, and each partition writes only its
    // portion of the destination. The result points to the first selected
    // element that did not fit, consuming intervening unselected elements.
    HPX_CXX_CORE_EXPORT template <typename I, typename O>
    struct bounded_copy_selected final
      : algorithm<bounded_copy_selected<I, O>, util::in_out_result<I, O>>
    {
        using result_type = util::in_out_result<I, O>;

        bounded_copy_selected()
          : algorithm<bounded_copy_selected, result_type>("bounded_copy")
        {
        }

        template <typename ExPolicy, typename Select>
        static result_type sequential(
            ExPolicy, I first, I last, O dest, O dest_last, Select select)
        {
            for (; first != last; ++first)
            {
                if (select(first))
                {
                    if (dest == dest_last)
                        break;
                    *dest++ = *first;
                }
            }
            return {first, dest};
        }

        template <typename ExPolicy, typename Select>
        static decltype(auto) parallel(ExPolicy&& policy, I first, I last,
            O dest, O dest_last, Select select)
        {
            using result =
                util::detail::algorithm_result<ExPolicy, result_type>;
            if (first == last)
                return result::get(result_type{first, dest});

            auto const count = static_cast<std::size_t>(last - first);
            auto const capacity = static_cast<std::size_t>(dest_last - dest);
            auto flags = std::make_shared<std::vector<unsigned char>>(count);
            auto select_partition = [first, flags, select = HPX_MOVE(select)](
                                        I it, std::size_t size) mutable {
                std::size_t selected = 0;
                util::loop_n<ExPolicy>(it, size, [&](I current) {
                    bool const keep = select(current);
                    (*flags)[current - first] = keep;
                    selected += keep;
                });
                return selected;
            };
            auto copy_partition = [first, dest, flags, capacity](I it,
                                      std::size_t size, std::size_t offset) {
                if (offset >= capacity)
                    return;
                auto out = dest + offset;
                auto remaining = capacity - offset;
                util::loop_n<ExPolicy>(it, size, [&](I current) {
                    if (remaining != 0 && (*flags)[current - first])
                    {
                        *out++ = *current;
                        --remaining;
                    }
                });
            };
            auto finish =
                [first, dest, flags, capacity](
                    std::vector<std::size_t>&& counts,
                    std::vector<hpx::future<void>>&& work) -> result_type {
                auto const copied = (std::min) (counts.back(), capacity);
                std::size_t selected = 0;
                auto resume = std::find_if(
                    flags->begin(), flags->end(), [&](unsigned char keep) {
                        return keep && selected++ == capacity;
                    });
                util::detail::clear_container(work);
                return {first + (resume - flags->begin()), dest + copied};
            };
            return util::scan_partitioner<ExPolicy, result_type,
                std::size_t>::call(HPX_FORWARD(ExPolicy, policy), first, count,
                std::size_t(0), HPX_MOVE(select_partition),
                std::plus<std::size_t>{}, HPX_MOVE(copy_partition),
                HPX_MOVE(finish));
        }
    };

    HPX_CXX_CORE_EXPORT template <typename I, typename O1, typename O2>
    struct bounded_partition_copy final
      : algorithm<bounded_partition_copy<I, O1, O2>,
            util::in_out_out_result<I, O1, O2>>
    {
        using result_type = util::in_out_out_result<I, O1, O2>;

        bounded_partition_copy()
          : algorithm<bounded_partition_copy, result_type>("partition_copy")
        {
        }

        template <typename ExPolicy, typename Pred, typename Proj>
        static result_type sequential(ExPolicy, I first, I last, O1 yes,
            O1 yes_last, O2 no, O2 no_last, Pred pred, Proj proj)
        {
            for (; first != last; ++first)
            {
                if (HPX_INVOKE(pred, HPX_INVOKE(proj, *first)))
                {
                    if (yes == yes_last)
                        break;
                    *yes++ = *first;
                }
                else
                {
                    if (no == no_last)
                        break;
                    *no++ = *first;
                }
            }
            return {first, yes, no};
        }

        template <typename ExPolicy, typename Pred, typename Proj>
        static decltype(auto) parallel(ExPolicy&& policy, I first, I last,
            O1 yes, O1 yes_last, O2 no, O2 no_last, Pred pred, Proj proj)
        {
            using result =
                util::detail::algorithm_result<ExPolicy, result_type>;
            if (first == last)
                return result::get(result_type{first, yes, no});

            auto const count = static_cast<std::size_t>(last - first);
            auto const yes_capacity = static_cast<std::size_t>(yes_last - yes);
            auto const no_capacity = static_cast<std::size_t>(no_last - no);
            auto flags = std::make_shared<std::vector<unsigned char>>(count);
            auto select_partition = [first, flags, pred = HPX_MOVE(pred),
                                        proj = HPX_MOVE(proj)](
                                        I it, std::size_t size) mutable {
                std::size_t selected = 0;
                util::loop_n<ExPolicy>(it, size, [&](I current) {
                    bool const keep =
                        HPX_INVOKE(pred, HPX_INVOKE(proj, *current));
                    (*flags)[current - first] = keep;
                    selected += keep;
                });
                return selected;
            };
            auto copy_partition = [=](I it, std::size_t size,
                                      std::size_t yes_offset) {
                auto no_offset =
                    static_cast<std::size_t>(it - first) - yes_offset;
                if (yes_offset > yes_capacity || no_offset > no_capacity)
                    return;
                bool stopped = false;
                util::loop_n<ExPolicy>(it, size, [&](I current) {
                    if (stopped)
                        return;
                    if ((*flags)[current - first])
                    {
                        if (yes_offset == yes_capacity)
                            stopped = true;
                        else
                            *(yes + yes_offset++) = *current;
                    }
                    else
                    {
                        if (no_offset == no_capacity)
                            stopped = true;
                        else
                            *(no + no_offset++) = *current;
                    }
                });
            };
            auto finish =
                [=](std::vector<std::size_t>&&,
                    std::vector<hpx::future<void>>&& work) -> result_type {
                std::size_t yes_count = 0, no_count = 0;
                auto resume = std::find_if(
                    flags->begin(), flags->end(), [&](unsigned char keep) {
                        if (keep)
                        {
                            if (yes_count == yes_capacity)
                                return true;
                            ++yes_count;
                        }
                        else
                        {
                            if (no_count == no_capacity)
                                return true;
                            ++no_count;
                        }
                        return false;
                    });
                util::detail::clear_container(work);
                return {first + (resume - flags->begin()), yes + yes_count,
                    no + no_count};
            };
            return util::scan_partitioner<ExPolicy, result_type,
                std::size_t>::call(HPX_FORWARD(ExPolicy, policy), first, count,
                std::size_t(0), HPX_MOVE(select_partition),
                std::plus<std::size_t>{}, HPX_MOVE(copy_partition),
                HPX_MOVE(finish));
        }
    };
    /// \endcond
}    // namespace hpx::parallel::detail
