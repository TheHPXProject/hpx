//  Copyright (c) 2026 Mohammad Izaan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)
#include <hpx/execution/traits/is_execution_policy.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/algorithms/detail/distance.hpp>
#include <hpx/parallel/algorithms/detail/is_heap.hpp>
#include <hpx/parallel/datapar/iterator_helpers.hpp>
#include <hpx/parallel/util/cancellation_token.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    HPX_CXX_CORE_EXPORT template <typename Comp, typename Proj, typename Vector>
    inline constexpr bool is_heap_datapar_compatible_v = requires(
        std::decay_t<Comp>& comp, std::decay_t<Proj>& proj, Vector& v) {
        requires(!std::is_convertible_v<
            std::decay_t<decltype(HPX_INVOKE(
                comp, HPX_INVOKE(proj, v), HPX_INVOKE(proj, v)))>,
            bool>);
        hpx::parallel::traits::find_first_of(
            HPX_INVOKE(comp, HPX_INVOKE(proj, v), HPX_INVOKE(proj, v)));
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct datapar_is_heap_until
    {
        template <typename Iter, typename Sent, typename Comp, typename Proj>
        static Iter call(Iter first, Sent last, Comp&& comp, Proj&& proj)
        {
            using policy_t = std::decay_t<ExPolicy>;
            constexpr std::size_t lanes =
                hpx::execution::policy_traits<policy_t>::num_lanes;
            using value_type = typename std::iterator_traits<Iter>::value_type;

            constexpr bool can_vectorize_values =
                is_vectorizable_value_type_v<value_type>;

            if constexpr (!can_vectorize_values)
            {
                using base_policy_type =
                    decltype(hpx::execution::experimental::to_non_simd(
                        std::declval<ExPolicy>()));
                return sequential_is_heap_until<base_policy_type>(first, last,
                    HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
            }
            else
            {
                using vector_type =
                    traits::vector_pack_type_t<value_type, lanes>;
                constexpr std::size_t num_lanes =
                    traits::vector_pack_size_v<vector_type>;

                constexpr bool can_vectorize = (num_lanes > 1) &&
                    is_heap_datapar_compatible_v<Comp, Proj, vector_type>;

                if constexpr (!can_vectorize)
                {
                    using base_policy_type =
                        decltype(hpx::execution::experimental::to_non_simd(
                            std::declval<ExPolicy>()));
                    return sequential_is_heap_until<base_policy_type>(first,
                        last, HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
                }
                else
                {
                    constexpr bool has_subtraction_distance =
                        (std::is_same_v<Iter, Sent> &&
                            std::random_access_iterator<Iter>) ||
                        std::sized_sentinel_for<Sent, Iter>;

                    if constexpr (!has_subtraction_distance)
                    {
                        using base_policy_type =
                            decltype(hpx::execution::experimental::to_non_simd(
                                std::declval<ExPolicy>()));
                        return sequential_is_heap_until<base_policy_type>(first,
                            last, HPX_FORWARD(Comp, comp),
                            HPX_FORWARD(Proj, proj));
                    }
                    else
                    {
                        auto count = detail::distance(first, last);
                        if (count <= 1)
                        {
                            std::advance(first, count);
                            return first;
                        }

                        Iter second = first + 1;
                        --count;
                        std::size_t base_idx = 0;
                        Iter it = second;

                        while (count >= static_cast<decltype(count)>(num_lanes))
                        {
                            vector_type parent_vec{};
                            for (std::size_t e = 0; e != num_lanes; ++e)
                            {
                                value_type const parent_val =
                                    *(first + (base_idx + e) / 2);
                                traits::set(parent_vec, e, parent_val);
                            }

                            auto child_vec =
                                traits::vector_pack_load<vector_type,
                                    value_type>::unaligned(it);

                            int offset = -1;
                            if constexpr (std::is_same_v<hpx::identity,
                                              std::decay_t<Proj>>)
                            {
                                auto mask =
                                    HPX_INVOKE(comp, parent_vec, child_vec);
                                offset = traits::find_first_of(mask);
                            }
                            else
                            {
                                auto mask = HPX_INVOKE(comp,
                                    HPX_INVOKE(proj, parent_vec),
                                    HPX_INVOKE(proj, child_vec));
                                offset = traits::find_first_of(mask);
                            }

                            if (offset != -1)
                            {
                                std::advance(it, offset);
                                return it;
                            }

                            std::advance(it, num_lanes);
                            base_idx += num_lanes;
                            count -= num_lanes;
                        }

                        while (count != 0)
                        {
                            if constexpr (std::is_same_v<hpx::identity,
                                              std::decay_t<Proj>>)
                            {
                                if (HPX_INVOKE(
                                        comp, *(first + base_idx / 2), *it))
                                {
                                    return it;
                                }
                            }
                            else
                            {
                                if (HPX_INVOKE(comp,
                                        HPX_INVOKE(
                                            proj, *(first + base_idx / 2)),
                                        HPX_INVOKE(proj, *it)))
                                {
                                    return it;
                                }
                            }

                            ++it;
                            ++base_idx;
                            --count;
                        }

                        return it;
                    }
                }
            }
        }

        template <typename Iter, typename Token, typename Comp, typename Proj>
        static void call(Iter first, std::size_t base_idx, Iter it,
            std::size_t part_size, Token& tok, Comp&& comp, Proj&& proj)
        {
            using policy_t = std::decay_t<ExPolicy>;
            constexpr std::size_t lanes =
                hpx::execution::policy_traits<policy_t>::num_lanes;
            using value_type = typename std::iterator_traits<Iter>::value_type;

            constexpr bool can_vectorize_values =
                is_vectorizable_value_type_v<value_type>;

            if constexpr (!can_vectorize_values)
            {
                using base_policy_type =
                    decltype(hpx::execution::experimental::to_non_simd(
                        std::declval<ExPolicy>()));
                sequential_is_heap_until<base_policy_type>(first, base_idx, it,
                    part_size, tok, HPX_FORWARD(Comp, comp),
                    HPX_FORWARD(Proj, proj));
            }
            else
            {
                using vector_type =
                    traits::vector_pack_type_t<value_type, lanes>;
                constexpr std::size_t num_lanes =
                    traits::vector_pack_size_v<vector_type>;

                constexpr bool can_vectorize = (num_lanes > 1) &&
                    is_heap_datapar_compatible_v<Comp, Proj, vector_type>;

                if constexpr (!can_vectorize)
                {
                    using base_policy_type =
                        decltype(hpx::execution::experimental::to_non_simd(
                            std::declval<ExPolicy>()));
                    sequential_is_heap_until<base_policy_type>(first, base_idx,
                        it, part_size, tok, HPX_FORWARD(Comp, comp),
                        HPX_FORWARD(Proj, proj));
                }
                else
                {
                    while (part_size >= num_lanes)
                    {
                        if (tok.was_cancelled(base_idx))
                        {
                            return;
                        }

                        vector_type parent_vec{};
                        for (std::size_t e = 0; e != num_lanes; ++e)
                        {
                            value_type const parent_val =
                                *(first + (base_idx + e) / 2);
                            traits::set(parent_vec, e, parent_val);
                        }

                        auto child_vec = traits::vector_pack_load<vector_type,
                            value_type>::unaligned(it);

                        int offset = -1;
                        if constexpr (std::is_same_v<hpx::identity,
                                          std::decay_t<Proj>>)
                        {
                            auto mask = HPX_INVOKE(comp, parent_vec, child_vec);
                            offset = traits::find_first_of(mask);
                        }
                        else
                        {
                            auto mask =
                                HPX_INVOKE(comp, HPX_INVOKE(proj, parent_vec),
                                    HPX_INVOKE(proj, child_vec));
                            offset = traits::find_first_of(mask);
                        }

                        if (offset != -1)
                        {
                            tok.cancel(
                                base_idx + static_cast<std::size_t>(offset));
                            return;
                        }

                        std::advance(it, num_lanes);
                        base_idx += num_lanes;
                        part_size -= num_lanes;
                    }

                    while (part_size != 0)
                    {
                        if (tok.was_cancelled(base_idx))
                        {
                            return;
                        }

                        if constexpr (std::is_same_v<hpx::identity,
                                          std::decay_t<Proj>>)
                        {
                            if (HPX_INVOKE(comp, *(first + base_idx / 2), *it))
                            {
                                tok.cancel(base_idx);
                                return;
                            }
                        }
                        else
                        {
                            if (HPX_INVOKE(comp,
                                    HPX_INVOKE(proj, *(first + base_idx / 2)),
                                    HPX_INVOKE(proj, *it)))
                            {
                                tok.cancel(base_idx);
                                return;
                            }
                        }

                        ++it;
                        ++base_idx;
                        --part_size;
                    }
                }
            }
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy>
    struct datapar_is_heap
    {
        template <typename Iter, typename Sent, typename Comp, typename Proj>
        static bool call(Iter first, Sent last, Comp&& comp, Proj&& proj)
        {
            return datapar_is_heap_until<ExPolicy>::call(first, last,
                       HPX_FORWARD(Comp, comp),
                       HPX_FORWARD(Proj, proj)) == last;
        }

        template <typename Iter, typename Token, typename Comp, typename Proj>
        static void call(Iter first, std::size_t base_idx, Iter it,
            std::size_t part_size, Token& tok, Comp&& comp, Proj&& proj)
        {
            datapar_is_heap_until<ExPolicy>::call(first, base_idx, it,
                part_size, tok, HPX_FORWARD(Comp, comp),
                HPX_FORWARD(Proj, proj));
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename Sent, typename Comp, typename Proj = hpx::identity>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    constexpr Iter hpx_invoke(sequential_is_heap_until_t<ExPolicy>, Iter first,
        Sent last, Comp&& comp, Proj&& proj = Proj())
    {
        constexpr bool is_contiguous =
            std::contiguous_iterator<std::decay_t<Iter>> ||
            hpx::traits::is_contiguous_iterator_v<std::decay_t<Iter>>;

        constexpr bool is_datapar_compatible =
            hpx::parallel::util::detail::iterator_datapar_compatible_v<Iter>;

        if constexpr (is_contiguous && is_datapar_compatible)
        {
            return datapar_is_heap_until<ExPolicy>::call(
                first, last, HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
        }
        else
        {
            using base_policy_type =
                decltype(hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>()));
            return sequential_is_heap_until<base_policy_type>(
                first, last, HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename Token, typename Comp, typename Proj = hpx::identity>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    constexpr void hpx_invoke(sequential_is_heap_until_t<ExPolicy>, Iter first,
        std::size_t base_idx, Iter it, std::size_t part_size, Token& tok,
        Comp&& comp, Proj&& proj = Proj())
    {
        constexpr bool is_contiguous =
            std::contiguous_iterator<std::decay_t<Iter>> ||
            hpx::traits::is_contiguous_iterator_v<std::decay_t<Iter>>;

        constexpr bool is_datapar_compatible =
            hpx::parallel::util::detail::iterator_datapar_compatible_v<Iter>;

        if constexpr (is_contiguous && is_datapar_compatible)
        {
            datapar_is_heap_until<ExPolicy>::call(first, base_idx, it,
                part_size, tok, HPX_FORWARD(Comp, comp),
                HPX_FORWARD(Proj, proj));
        }
        else
        {
            using base_policy_type =
                decltype(hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>()));
            sequential_is_heap_until<base_policy_type>(first, base_idx, it,
                part_size, tok, HPX_FORWARD(Comp, comp),
                HPX_FORWARD(Proj, proj));
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename Sent, typename Comp, typename Proj = hpx::identity>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    constexpr bool hpx_invoke(sequential_is_heap_t<ExPolicy>, Iter first,
        Sent last, Comp&& comp, Proj&& proj = Proj())
    {
        constexpr bool is_contiguous =
            std::contiguous_iterator<std::decay_t<Iter>> ||
            hpx::traits::is_contiguous_iterator_v<std::decay_t<Iter>>;

        constexpr bool is_datapar_compatible =
            hpx::parallel::util::detail::iterator_datapar_compatible_v<Iter>;

        if constexpr (is_contiguous && is_datapar_compatible)
        {
            return datapar_is_heap<ExPolicy>::call(
                first, last, HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
        }
        else
        {
            using base_policy_type =
                decltype(hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>()));
            return sequential_is_heap<base_policy_type>(
                first, last, HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename Token, typename Comp, typename Proj = hpx::identity>
        requires(hpx::is_vectorpack_execution_policy_v<ExPolicy>)
    constexpr void hpx_invoke(sequential_is_heap_t<ExPolicy>, Iter first,
        std::size_t base_idx, Iter it, std::size_t part_size, Token& tok,
        Comp&& comp, Proj&& proj = Proj())
    {
        constexpr bool is_contiguous =
            std::contiguous_iterator<std::decay_t<Iter>> ||
            hpx::traits::is_contiguous_iterator_v<std::decay_t<Iter>>;

        constexpr bool is_datapar_compatible =
            hpx::parallel::util::detail::iterator_datapar_compatible_v<Iter>;

        if constexpr (is_contiguous && is_datapar_compatible)
        {
            datapar_is_heap<ExPolicy>::call(first, base_idx, it, part_size, tok,
                HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
        }
        else
        {
            using base_policy_type =
                decltype(hpx::execution::experimental::to_non_simd(
                    std::declval<ExPolicy>()));
            sequential_is_heap<base_policy_type>(first, base_idx, it, part_size,
                tok, HPX_FORWARD(Comp, comp), HPX_FORWARD(Proj, proj));
        }
    }

}    // namespace hpx::parallel::detail

#endif
