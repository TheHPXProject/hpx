//  Copyright (c) 2025 Isidoros Tsaousis-Seiras
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <cstddef>
#include <type_traits>

namespace hpx::experimental {

    // P2786R13 specifies a single feature-test macro __cpp_trivial_relocatability
    // that signals availability of both the language facilities and the library
    // trait std::is_replaceable, see
    // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2786r13.html#language-feature-test-macros

    // Note: while Clang V22.0.0 defines __cpp_trivial_relocatability pre C++26, it
    // does not have std::is_replaceable.

#if __cplusplus >= 202611 && defined(__cpp_trivial_relocatability)
    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_replaceable : std::is_replaceable<T>
    {
    };
#else
    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_replaceable
      : std::bool_constant<std::is_object_v<T> && !std::is_const_v<T> &&
            !std::is_volatile_v<T> &&
            (std::is_scalar_v<T> ||
                ((std::is_class_v<T> || std::is_union_v<T>) &&
                    std::is_trivially_move_constructible_v<T> &&
                    std::is_trivially_move_assignable_v<T> &&
                    std::is_trivially_destructible_v<T>) )>
    {
    };

    template <typename T>
    struct is_replaceable<T[]> : std::false_type
    {
    };

    template <typename T, std::size_t N>
    struct is_replaceable<T[N]> : std::false_type
    {
    };
#endif

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_replaceable_v = is_replaceable<T>::value;

}    // namespace hpx::experimental
