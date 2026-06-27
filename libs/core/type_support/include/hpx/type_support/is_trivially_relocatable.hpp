//  Copyright (c) 2023 Isidoros Tsaousis-Seiras
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <type_traits>

namespace hpx::experimental {

// P2786R13 specifies a single feature-test macro __cpp_trivial_relocatability
// that signals availability of both the language facilities and the library
// trait std::is_trivially_relocatable, see

// Note: while Clang V22.0.0 defines __cpp_trivial_relocatability pre C++26, it
// does not have std::is_trivially_relocatable.

// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2786r13.html#language-feature-test-macros
#if __cplusplus >= 202611 && defined(__cpp_trivial_relocatability)
    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_trivially_relocatable : std::is_trivially_relocatable<T>
    {
    };
#else
    // All trivially copyable types are trivially relocatable
    // Other types should default to false.
    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_trivially_relocatable : std::is_trivially_copyable<T>
    {
    };

    // References are not trivially relocatable
    template <typename T>
    struct is_trivially_relocatable<T&> : std::false_type
    {
    };

    // Temporary objects are not trivially relocatable
    template <typename T>
    struct is_trivially_relocatable<T&&> : std::false_type
    {
    };

    // Constness, Volatility, Arrays are ignored
    template <typename T>
    struct is_trivially_relocatable<T const> : is_trivially_relocatable<T>
    {
    };

    template <typename T>
    struct is_trivially_relocatable<T volatile> : is_trivially_relocatable<T>
    {
    };

    template <typename T>
    struct is_trivially_relocatable<T const volatile>
      : is_trivially_relocatable<T>
    {
    };

    template <typename T>
    struct is_trivially_relocatable<T[]> : is_trivially_relocatable<T>
    {
    };

    template <typename T, int N>
    struct is_trivially_relocatable<T[N]> : is_trivially_relocatable<T>
    {
    };

    template <typename T, int N>
    struct is_trivially_relocatable<T const[N]> : is_trivially_relocatable<T>
    {
    };

    template <typename T, int N>
    struct is_trivially_relocatable<T volatile[N]> : is_trivially_relocatable<T>
    {
    };

    template <typename T, int N>
    struct is_trivially_relocatable<T const volatile[N]>
      : is_trivially_relocatable<T>
    {
    };
#endif

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_trivially_relocatable_v =
        is_trivially_relocatable<T>::value;
}    // namespace hpx::experimental
