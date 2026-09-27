//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <functional>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {
    /// \cond NOINTERNAL

    // Preserve the existing ownership of copyable arguments in task policies.
    // Noncopyable values are referenced and must outlive asynchronous work.
    HPX_CXX_CORE_EXPORT template <typename T>
    class algorithm_value
    {
        using storage_type = std::conditional_t<std::is_copy_constructible_v<T>,
            T, std::reference_wrapper<T const>>;
        storage_type value_;

    public:
        HPX_HOST_DEVICE constexpr explicit algorithm_value(T const& value)
          : value_(value)
        {
        }

        HPX_HOST_DEVICE constexpr T const& get() const noexcept
        {
            if constexpr (std::is_copy_constructible_v<T>)
                return value_;
            else
                return value_.get();
        }
    };

    // Unwrapping borrows from a named value or its storage. Reject temporaries
    // so the returned reference cannot outlive them.
    HPX_CXX_CORE_EXPORT template <typename T>
    HPX_HOST_DEVICE constexpr T const& unwrap_algorithm_value(
        T const&& value) noexcept = delete;

    HPX_CXX_CORE_EXPORT template <typename T>
    HPX_HOST_DEVICE constexpr T const& unwrap_algorithm_value(T& value) noexcept
    {
        return value;
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    HPX_HOST_DEVICE constexpr T const& unwrap_algorithm_value(
        algorithm_value<T>& value) noexcept
    {
        return value.get();
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    HPX_HOST_DEVICE constexpr T const& unwrap_algorithm_value(
        algorithm_value<T> const& value) noexcept
    {
        return value.get();
    }

    HPX_CXX_CORE_EXPORT template <typename T>
    constexpr auto equal_to_value(T const& value)
    {
        return [value = algorithm_value<T>(value)](auto&& projected) {
            return std::ranges::equal_to{}(
                HPX_FORWARD(decltype(projected), projected), value.get());
        };
    }

    /// \endcond
}    // namespace hpx::parallel::detail
