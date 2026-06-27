///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <hpx/config.hpp>

#include <type_traits>

namespace hpx::compute::detail {

    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct get_proxy_type_impl
    {
        using type = T;
    };

    template <typename T>
    struct get_proxy_type_impl<T,
        std::void_t<typename std::decay_t<T>::proxy_type>>
    {
        using proxy_type = typename std::decay_t<T>::proxy_type;
    };

    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct get_proxy_type : get_proxy_type_impl<T>
    {
    };
}    // namespace hpx::compute::detail
