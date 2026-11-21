//  Copyright (c) 2017 Taeguk Kwon
//  Copyright (c) 2020-2026 Hartmut Kaiser
//  Copyright (c) 2026 Mohammad Izaan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/execution/traits/is_execution_policy.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/parallel/algorithms/detail/tag_dispatch.hpp>
#include <hpx/parallel/util/loop.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct sequential_is_heap_until_t final
      : hpx::detail::tag_dispatch<sequential_is_heap_until_t<ExPolicy>,
            hpx::detail::no_base>
    {
        template <typename Iter, typename Sent, typename Comp, typename Proj>
        static constexpr Iter invoke_default(
            Iter first, Sent last, Comp&& comp, Proj&& proj)
        {
            Iter child = first;
            if (child == last)
            {
                return child;
            }

            while (true)
            {
                ++child;
                if (child == last ||
                    HPX_INVOKE(comp, HPX_INVOKE(proj, *first),
                        HPX_INVOKE(proj, *child)))
                {
                    break;
                }

                ++child;
                if (child == last ||
                    HPX_INVOKE(comp, HPX_INVOKE(proj, *first),
                        HPX_INVOKE(proj, *child)))
                {
                    break;
                }

                ++first;
            }

            return child;
        }

        template <typename Iter, typename Sent, typename Comp>
        static constexpr Iter invoke_default(
            Iter first, Sent last, Comp&& comp, hpx::identity)
        {
            Iter child = first;
            if (child == last)
            {
                return child;
            }

            while (true)
            {
                ++child;
                if (child == last || HPX_INVOKE(comp, *first, *child))
                {
                    break;
                }

                ++child;
                if (child == last || HPX_INVOKE(comp, *first, *child))
                {
                    break;
                }

                ++first;
            }

            return child;
        }

        template <typename Iter, typename Token, typename Comp, typename Proj>
        static constexpr void invoke_default(Iter first, std::size_t base_idx,
            Iter it, std::size_t part_size, Token& tok, Comp&& comp,
            Proj&& proj)
        {
            using type = typename std::iterator_traits<Iter>::value_type;
            std::size_t cancelled = static_cast<std::size_t>(-1);

            util::loop_idx_n<ExPolicy>(base_idx, it, part_size, tok,
                [&cancelled, first, &comp, &proj](
                    type const& v, std::size_t const i) -> void {
                    if constexpr (std::is_same_v<hpx::identity,
                                      std::decay_t<Proj>>)
                    {
                        HPX_UNUSED(proj);
                        if (cancelled == static_cast<std::size_t>(-1))
                        {
                            if (HPX_INVOKE(comp, *(first + i / 2), v))
                            {
                                cancelled = i;
                            }
                        }
                    }
                    else
                    {
                        if (cancelled == static_cast<std::size_t>(-1))
                        {
                            if (HPX_INVOKE(comp,
                                    HPX_INVOKE(proj, *(first + i / 2)),
                                    HPX_INVOKE(proj, v)))
                            {
                                cancelled = i;
                            }
                        }
                    }
                });

            if (cancelled != static_cast<std::size_t>(-1))
            {
                tok.cancel(cancelled);
            }
        }
    };

    HPX_CXX_CORE_EXPORT template <
        typename ExPolicy = hpx::execution::sequenced_policy, typename Iter,
        typename Sent, typename Comp, typename Proj = hpx::identity>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr Iter sequential_is_heap_until(
        Iter first, Sent last, Comp&& comp, Proj&& proj = Proj())
    {
        return sequential_is_heap_until_t<ExPolicy>{}(
            first, last, HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
    }

    HPX_CXX_CORE_EXPORT template <
        typename ExPolicy = hpx::execution::sequenced_policy, typename Iter,
        typename Token, typename Comp, typename Proj = hpx::identity>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr void sequential_is_heap_until(
        Iter first, std::size_t base_idx, Iter it, std::size_t part_size,
        Token& tok, Comp&& comp, Proj&& proj = Proj())
    {
        sequential_is_heap_until_t<ExPolicy>{}(first, base_idx, it, part_size,
            tok, HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct sequential_is_heap_t final
      : hpx::detail::tag_dispatch<sequential_is_heap_t<ExPolicy>,
            hpx::detail::no_base>
    {
        template <typename Iter, typename Sent, typename Comp, typename Proj>
        static constexpr bool invoke_default(
            Iter first, Sent last, Comp&& comp, Proj&& proj)
        {
            return sequential_is_heap_until<ExPolicy>(first, last,
                       HPX_FORWARD(Comp, comp),
                       HPX_FORWARD(Proj, proj)) == last;
        }

        template <typename Iter, typename Token, typename Comp, typename Proj>
        static constexpr void invoke_default(Iter first, std::size_t base_idx,
            Iter it, std::size_t part_size, Token& tok, Comp&& comp,
            Proj&& proj)
        {
            sequential_is_heap_until<ExPolicy>(first, base_idx, it, part_size,
                tok, HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
        }
    };

    HPX_CXX_CORE_EXPORT template <
        typename ExPolicy = hpx::execution::sequenced_policy, typename Iter,
        typename Sent, typename Comp, typename Proj = hpx::identity>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr bool sequential_is_heap(
        Iter first, Sent last, Comp&& comp, Proj&& proj = Proj())
    {
        return sequential_is_heap_t<ExPolicy>{}(
            first, last, HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
    }

    HPX_CXX_CORE_EXPORT template <
        typename ExPolicy = hpx::execution::sequenced_policy, typename Iter,
        typename Token, typename Comp, typename Proj = hpx::identity>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr void sequential_is_heap(
        Iter first, std::size_t base_idx, Iter it, std::size_t part_size,
        Token& tok, Comp&& comp, Proj&& proj = Proj())
    {
        sequential_is_heap_t<ExPolicy>{}(first, base_idx, it, part_size, tok,
            HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
    }

}    // namespace hpx::parallel::detail
