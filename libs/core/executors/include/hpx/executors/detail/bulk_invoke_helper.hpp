//  Copyright (c) 2019-2020 ETH Zurich
//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2019 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <type_traits>

namespace hpx::execution::experimental::detail {

    template <std::size_t... Is, typename F, typename T, typename Args>
    HPX_FORCEINLINE constexpr decltype(auto) bulk_invoke_helper(
        hpx::util::index_pack<Is...>, F&& f, T&& t, Args&& args)
    {
        // NOLINTBEGIN(bugprone-use-after-move)
        return HPX_INVOKE(HPX_FORWARD(F, f), HPX_FORWARD(T, t),
            hpx::get<Is>(HPX_FORWARD(Args, args))...);
        // NOLINTEND(bugprone-use-after-move)
    }

    template <std::size_t... Is, typename F, typename... Ts, typename Args>
    HPX_FORCEINLINE constexpr decltype(auto) bulk_invoke_helper(
        hpx::util::index_pack<Is...>, F&& f, hpx::tuple<Ts...>& t, Args&& args)
    {
        using embedded_index_pack_type =
            hpx::util::make_index_pack<sizeof...(Ts)>;

        // NOLINTBEGIN(bugprone-use-after-move)
        if constexpr (std::is_invocable_v<F, embedded_index_pack_type,
                          hpx::tuple<Ts...>&,
                          decltype(hpx::get<Is>(HPX_FORWARD(Args, args)))...>)
        {
            return HPX_INVOKE(HPX_FORWARD(F, f), embedded_index_pack_type{}, t,
                hpx::get<Is>(HPX_FORWARD(Args, args))...);
        }
        else
        {
            return HPX_INVOKE(
                HPX_FORWARD(F, f), t, hpx::get<Is>(HPX_FORWARD(Args, args))...);
        }
        // NOLINTEND(bugprone-use-after-move)
    }

    template <std::size_t... Is, typename F, typename... Ts, typename Args>
    HPX_FORCEINLINE constexpr decltype(auto) bulk_invoke_helper(
        hpx::util::index_pack<Is...>, F&& f, hpx::tuple<Ts...> const& t,
        Args&& args)
    {
        using embedded_index_pack_type =
            hpx::util::make_index_pack<sizeof...(Ts)>;

        // NOLINTBEGIN(bugprone-use-after-move)
        if constexpr (std::is_invocable_v<F, embedded_index_pack_type,
                          hpx::tuple<Ts...> const&,
                          decltype(hpx::get<Is>(HPX_FORWARD(Args, args)))...>)
        {
            return HPX_INVOKE(HPX_FORWARD(F, f), embedded_index_pack_type{}, t,
                hpx::get<Is>(HPX_FORWARD(Args, args))...);
        }
        else
        {
            return HPX_INVOKE(
                HPX_FORWARD(F, f), t, hpx::get<Is>(HPX_FORWARD(Args, args))...);
        }
        // NOLINTEND(bugprone-use-after-move)
    }

    template <std::size_t... Is, typename F, typename... Ts, typename Args>
    HPX_FORCEINLINE constexpr decltype(auto) bulk_invoke_helper(
        hpx::util::index_pack<Is...>, F&& f, hpx::tuple<Ts...>&& t, Args&& args)
    {
        using embedded_index_pack_type =
            hpx::util::make_index_pack<sizeof...(Ts)>;

        // NOLINTBEGIN(bugprone-use-after-move)
        if constexpr (std::is_invocable_v<F, embedded_index_pack_type,
                          hpx::tuple<Ts...>&&,
                          decltype(hpx::get<Is>(HPX_FORWARD(Args, args)))...>)
        {
            return HPX_INVOKE(HPX_FORWARD(F, f), embedded_index_pack_type{},
                HPX_MOVE(t), hpx::get<Is>(HPX_FORWARD(Args, args))...);
        }
        else
        {
            return HPX_INVOKE(HPX_FORWARD(F, f), HPX_MOVE(t),
                hpx::get<Is>(HPX_FORWARD(Args, args))...);
        }
        // NOLINTEND(bugprone-use-after-move)
    }
}    // namespace hpx::execution::experimental::detail
