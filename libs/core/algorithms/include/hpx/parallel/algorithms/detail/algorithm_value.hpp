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

    /// \brief Checks whether an algorithm value can safely be captured for the
    ///        given execution policy.
    ///
    /// Synchronous algorithms borrow their values because they finish before
    /// returning. Asynchronous algorithms copy values when possible and borrow
    /// only non-copyable lvalues whose lifetime the caller must preserve.
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename T>
    concept algorithm_value_argument =
        !hpx::is_async_execution_policy_v<ExPolicy> ||
        std::is_lvalue_reference_v<T> ||
        std::is_copy_constructible_v<std::remove_cvref_t<T>>;

    /// \brief Stores or borrows an algorithm value according to the execution
    ///        policy and the value's copyability.
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
        /// \brief Constructs storage for \a value.
        template <typename U>
            requires(algorithm_value_argument<ExPolicy, U> &&
                std::is_same_v<std::remove_cvref_t<U>, T>)
        HPX_HOST_DEVICE constexpr explicit algorithm_value(U&& value)
          : value_(std::as_const(value))
        {
        }

        /// \brief Returns the stored or borrowed value.
        HPX_HOST_DEVICE constexpr T const& get() const noexcept
        {
            if constexpr (owns_value)
                return value_;
            else
                return value_.get();
        }
    };

    /// \brief Creates an equality predicate with policy-aware value storage.
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
