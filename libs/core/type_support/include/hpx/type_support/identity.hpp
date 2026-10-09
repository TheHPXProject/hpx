//  Copyright (c) 2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file identity.hpp
/// \page hpx::identity
/// \headerfile hpx/modules/type_support.hpp

#pragma once

#include <hpx/config.hpp>

#include <type_traits>

namespace hpx {

    HPX_CXX_CORE_EXPORT struct identity
    {
        using is_transparent = std::true_type;

        template <typename T>
        HPX_HOST_DEVICE constexpr T&& operator()(T&& t) const noexcept
        {
            return HPX_FORWARD(T, t);
        }
    };

    HPX_CXX_CORE_EXPORT inline constexpr identity identity_v = identity();

    HPX_CXX_CORE_EXPORT template <typename T>
    struct type_identity
    {
        using type = T;
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    using type_identity_t = typename type_identity<T>::type;

}    // namespace hpx

namespace hpx::util {

    template <typename T>
    using identity HPX_DEPRECATED_V(1, 9,
        "hpx::util::identity is deprecated, use hpx::type_identity instead") =
        hpx::type_identity<T>;
}    // namespace hpx::util
