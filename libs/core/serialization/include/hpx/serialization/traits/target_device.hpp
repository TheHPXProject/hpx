//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <type_traits>
#include <utility>

namespace hpx::traits {

    namespace detail {

        template <typename Target, typename Enable = void>
        struct has_integral_device : std::false_type
        {
        };

        template <typename Target>
        struct has_integral_device<Target,
            std::enable_if_t<std::is_integral_v<
                std::decay_t<decltype(std::declval<Target const&>()
                        .native_handle()
                        .get_device())>>>> : std::true_type
        {
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // Customization point: the GPU a target (for instance
    // hpx::cuda::experimental::target) stands for, -1 if it stands for none
    // (for instance hpx::compute::host::target). Device data received for a
    // target is placed on that GPU. By default, the device of a target whose
    // native_handle().get_device() is an integer is that integer, other
    // targets stand for no GPU.
    HPX_CXX_CORE_EXPORT template <typename Target, typename Enable = void>
    struct target_device
    {
        static int call([[maybe_unused]] Target const& target) noexcept
        {
            if constexpr (detail::has_integral_device<Target>::value)
            {
                return static_cast<int>(target.native_handle().get_device());
            }
            else
            {
                return -1;
            }
        }
    };
}    // namespace hpx::traits
