//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2022 Hartmut Kaiser
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

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Variants>
    struct single_result
    {
        static_assert(sizeof(Variants) == 0,
            "expected a single variant with a single type in value_types_of_t");
    };

    template <>
    struct single_result<meta::pack<>>
    {
        using type = void;
    };

    template <>
    struct single_result<meta::pack<meta::pack<>>>
    {
        using type = void;
    };

    template <typename T>
    struct single_result<meta::pack<meta::pack<T>>>
    {
        using type = T;
    };

    template <typename T, typename U, typename... Ts>
    struct single_result<meta::pack<meta::pack<T, U, Ts...>>>
    {
        static_assert(sizeof(T) == 0,
            "expected a single variant with a single type in value_types_of_t "
            "(single variant with two or more types given)");
    };

    template <typename T, typename U, typename... Ts>
    struct single_result<meta::pack<T, U, Ts...>>
    {
        static_assert(sizeof(T) == 0,
            "expected a single variant with a single type in value_types_of_t "
            "(two or more variants)");
    };

    HPX_CXX_CORE_EXPORT template <typename Variants>
    using single_result_t = meta::type<single_result<Variants>>;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Variants>
    struct single_result_non_void
    {
        using type = single_result_t<Variants>;
        static_assert(
            !std::is_void_v<type>, "expected a non-void type in single_result");
    };

    HPX_CXX_CORE_EXPORT template <typename Variants>
    using single_result_non_void_t =
        meta::type<single_result_non_void<Variants>>;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Variants>
    struct single_variant
    {
        static_assert(sizeof(Variants) == 0,
            "expected a single variant completion_signatures<>::value_types");
    };

    template <typename T>
    struct single_variant<util::pack<T>>
    {
        using type = T;
    };

    template <typename T>
    struct single_variant<meta::pack<T>>
    {
        using type = T;
    };

    template <typename T>
    struct single_variant<hpx::variant<T>>
    {
        using type = T;
    };

    HPX_CXX_CORE_EXPORT template <typename Variants>
    using single_variant_t = meta::type<single_variant<Variants>>;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Variants>
    struct single_variant_tuple_size
    {
        static_assert(sizeof(Variants) == 0,
            "expected a single variant completion_signatures<>::value_types");
    };

    template <template <typename...> typename Variants>
    struct single_variant_tuple_size<Variants<>>
    {
        static constexpr std::size_t size = 0;
    };

    template <template <typename...> typename Variants,
        template <typename...> typename Tuple, typename... Ts>
    struct single_variant_tuple_size<Variants<Tuple<Ts...>>>
    {
        static constexpr std::size_t size = sizeof...(Ts);
    };

    HPX_CXX_CORE_EXPORT template <typename Variants>
    inline constexpr std::size_t single_variant_tuple_size_v =
        single_variant_tuple_size<Variants>::size;
}    // namespace hpx::execution::experimental::detail
