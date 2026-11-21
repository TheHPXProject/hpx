//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>

#include <cstddef>
#include <type_traits>

namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename T, typename Enable = void>
    struct is_client : std::false_type
    {
    };

    HPX_CXX_EXPORT template <typename T>
    inline constexpr bool is_client_v = is_client<T>::value;

    // A client refers to a component, so hpx::async<&f>(client, ...) is
    // remote: a member function runs there, and a free function doesn't
    // compile, even if it takes a client.
    template <typename T>
        requires(is_client_v<T>)
    struct is_remote_target<T> : std::true_type
    {
    };

    HPX_CXX_EXPORT template <typename T, typename Enable = void>
    struct is_client_or_client_array : is_client<T>
    {
    };

    template <typename T>
    struct is_client_or_client_array<T[]> : is_client<T>
    {
    };

    template <typename T, std::size_t N>
    struct is_client_or_client_array<T[N]> : is_client<T>
    {
    };

    HPX_CXX_EXPORT template <typename T>
    inline constexpr bool is_client_or_client_array_v =
        is_client_or_client_array<T>::value;
}    // namespace hpx::traits
