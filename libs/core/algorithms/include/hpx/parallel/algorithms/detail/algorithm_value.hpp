//  Copyright (c) 2026 the-ivii
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/execution.hpp>

#include <functional>
#include <ranges>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {
    /// \cond NOINTERNAL

    // Asynchronous policy algorithms must own their values or borrow from an
    // lvalue whose lifetime the caller keeps through the asynchronous work.
    // Synchronous calls may also borrow temporaries because they do not return
    // until the work has completed.
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename T>
    concept algorithm_value_argument =
        !hpx::is_async_execution_policy_v<ExPolicy> ||
        std::is_lvalue_reference_v<T> ||
        std::is_copy_constructible_v<std::remove_cvref_t<T>>;

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename T>
    class algorithm_value
    {
        static constexpr bool owns_value =
            hpx::is_async_execution_policy_v<ExPolicy> &&
            std::is_copy_constructible_v<T>;

        using storage_type =
            std::conditional_t<owns_value, T, std::reference_wrapper<T const>>;
        storage_type value_;

    public:
        template <typename U>
            requires(algorithm_value_argument<ExPolicy, U> &&
                std::is_same_v<std::remove_cvref_t<U>, T>)
        HPX_HOST_DEVICE constexpr explicit algorithm_value(U&& value)
          : value_(std::as_const(value))
        {
        }

        HPX_HOST_DEVICE constexpr T const& get() const noexcept
        {
            if constexpr (owns_value)
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
        return std::as_const(value);
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename T>
    HPX_HOST_DEVICE constexpr T const& unwrap_algorithm_value(
        algorithm_value<ExPolicy, T>& value) noexcept
    {
        return value.get();
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename T>
    HPX_HOST_DEVICE constexpr T const& unwrap_algorithm_value(
        algorithm_value<ExPolicy, T> const& value) noexcept
    {
        return value.get();
    }

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename T>
        requires(algorithm_value_argument<ExPolicy, T>)
    constexpr auto equal_to_value(T&& value)
    {
        using policy_type = std::decay_t<ExPolicy>;
        using value_type = std::remove_cvref_t<T>;

        return [value = algorithm_value<policy_type, value_type>(
                    HPX_FORWARD(T, value))](auto&& projected) {
            return std::ranges::equal_to{}(
                HPX_FORWARD(decltype(projected), projected), value.get());
        };
    }

    /// \endcond
}    // namespace hpx::parallel::detail
