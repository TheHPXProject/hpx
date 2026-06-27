//  Copyright (c) 2017-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <functional>

namespace hpx::util {

    HPX_CXX_CORE_EXPORT template <typename T>
    struct unwrap_reference
    {
        using type = T;
    };

    template <typename T>
    struct unwrap_reference<::std::reference_wrapper<T>>
    {
        using type = T;
    };

    template <typename T>
    struct unwrap_reference<::std::reference_wrapper<T> const>
    {
        using type = T;
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    using unwrap_reference_t = typename unwrap_reference<T>::type;

    HPX_CXX_CORE_EXPORT template <typename T>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr unwrap_reference_t<T>& unwrap_ref(
        T& t) noexcept
    {
        return t;    // NOLINT(bugprone-return-const-ref-from-parameter)
    }
}    // namespace hpx::util
