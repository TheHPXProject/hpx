//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <type_traits>

namespace hpx::traits {

    // Customization point: whether objects of type T hold data in GPU memory
    // (e.g. thrust::device_vector). Only the results of remote calls whose
    // type has device data are received on a chosen GPU, the GPU of the
    // default target (see hpx::detail::result_device).
    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct has_device_data : std::false_type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool has_device_data_v =
        has_device_data<std::decay_t<T>>::value;
}    // namespace hpx::traits
