//  Copyright (c) 2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/assert.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/type_support.hpp>

#include <array>
#include <cstddef>
#include <functional>
#include <type_traits>

namespace hpx::detail {

    ///////////////////////////////////////////////////////////////////////////
    template <typename R, typename Tuple, typename F, std::size_t... Is>
    struct access_table
    {
        using tuple_type = Tuple;
        using return_type = R;

        template <std::size_t N>
        [[nodiscard]] static constexpr return_type access_tuple(
            tuple_type& t, F& f) noexcept
        {
            return HPX_INVOKE(f, hpx::get<N>(t));
        }

        using accessor_fun_ptr = return_type (*)(tuple_type&, F&) noexcept;
        static constexpr std::size_t table_size = sizeof...(Is);

        static constexpr std::array<accessor_fun_ptr, table_size> lookup_table =
            {{&access_tuple<Is>...}};
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename R, typename Tuple, typename F, std::size_t... Is>
    [[nodiscard]] constexpr decltype(auto) call_access_function(
        Tuple& t, std::size_t i, F&& f, hpx::util::index_pack<Is...>) noexcept
    {
        HPX_ASSERT_MSG(
            i < sizeof...(Is), "index must be smaller than tuple size");

        constexpr auto& table = access_table<R, Tuple, F, Is...>::lookup_table;
        return table[i](t, f);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Tuple>
    using first_tuple_element_t =
        hpx::tuple_element_t<0, std::remove_reference_t<Tuple>>;

    HPX_CXX_CORE_EXPORT template <typename Tuple>
    [[nodiscard]] constexpr decltype(auto) homogenous_runtime_get(
        Tuple& t, std::size_t i) noexcept
    {
        return call_access_function<
            first_tuple_element_t<std::decay_t<Tuple>&>>(t, i, hpx::identity_v,
            hpx::util::make_index_pack<
                hpx::tuple_size_v<std::decay_t<Tuple>>>{});
    }

    ///////////////////////////////////////////////////////////////////////////
    // Generate variant that uniquely holds all the tuple types
    template <typename Tuple>
    struct variant_from_tuple;

    template <typename... Ts>
    struct variant_from_tuple<hpx::tuple<Ts...>>
    {
        using type =
            hpx::meta::invoke<hpx::meta::unique<hpx::meta::func<hpx::variant>>,
                std::reference_wrapper<std::decay_t<Ts>>...>;
    };

    template <typename Tuple>
    using variant_from_tuple_t = typename variant_from_tuple<Tuple>::type;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Tuple>
    [[nodiscard]] constexpr decltype(auto) runtime_get(
        Tuple& t, std::size_t i) noexcept
    {
        return call_access_function<variant_from_tuple_t<std::decay_t<Tuple>>>(
            t, i, [](auto& element) { return std::ref(element); },
            hpx::util::make_index_pack<
                hpx::tuple_size_v<std::decay_t<Tuple>>>{});
    }
}    // namespace hpx::detail
