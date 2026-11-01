//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// (C) Copyright 2008 CodeRage, LLC (turkanis at coderage dot com)
// (C) Copyright 2003-2007 Jonathan Turkanis

// See http://www.boost.org/libs/iostreams for documentation.

#pragma once

#include <hpx/config.hpp>
#include <hpx/iostream/constants.hpp>
#include <hpx/iostream/detail/dispatch.hpp>
#include <hpx/iostream/operations_fwd.hpp>
#include <hpx/iostream/traits.hpp>

#include <hpx/config/warnings_prefix.hpp>

#include <ios>
#include <type_traits>

namespace hpx::iostream {

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename T>
        struct optimal_buffer_size_impl;

        HPX_CXX_CORE_EXPORT template <typename T>
        struct optimal_buffer_size_impl
          : std::conditional_t<is_custom_v<T>, operations<T>,
                optimal_buffer_size_impl<dispatch_t<T, optimally_buffered_tag,
                    device_tag, filter_tag>>>
        {
        };

        template <>
        struct optimal_buffer_size_impl<optimally_buffered_tag>
        {
            template <typename T>
            static constexpr std::streamsize optimal_buffer_size(
                T const& t) noexcept
            {
                return t.optimal_buffer_size();
            }
        };

        template <>
        struct optimal_buffer_size_impl<device_tag>
        {
            template <typename T>
            static constexpr std::streamsize optimal_buffer_size(
                T const&) noexcept
            {
                return default_device_buffer_size;
            }
        };

        template <>
        struct optimal_buffer_size_impl<filter_tag>
        {
            template <typename T>
            static constexpr std::streamsize optimal_buffer_size(
                T const&) noexcept
            {
                return default_filter_buffer_size;
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename T>
    constexpr std::streamsize optimal_buffer_size(T const& t) noexcept
    {
        return detail::optimal_buffer_size_impl<T>::optimal_buffer_size(
            util::unwrap_ref(t));
    }
}    // namespace hpx::iostream

#include <hpx/config/warnings_suffix.hpp>
