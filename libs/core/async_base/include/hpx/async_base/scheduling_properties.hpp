//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2022-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/async_base/detail/query_first_fallback.hpp>
#include <hpx/async_base/query_dispatch.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/execution_base.hpp>

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Property, typename Enable = void>
    struct is_scheduling_property : std::false_type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Property>
    inline constexpr bool is_scheduling_property_v =
        is_scheduling_property<Property>::value;

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        // The given property (Tag) is not supported on the given type (first
        // type in Args). Ensure that you are including the correct headers if
        // the property is supported. Alternatively, implement support for the
        // property through a query() member on the target type.
        template <typename Tag, typename... Ts>
        struct property_not_supported;

        template <executor_any Executor>
        auto wrap_with_annotation(Executor&& exec, char const* annotation);

        template <executor_any Executor>
        auto wrap_with_annotation(Executor&& exec, std::string annotation);

        // NOLINTBEGIN(bugprone-crtp-constructor-accessibility)
        template <typename Tag>
        struct property_base
        {
            template <typename Target, typename... Args>
                requires(has_query_v<Target, Tag, Args...>)
            constexpr auto operator()(Target&& target, Args&&... args) const
            {
                return HPX_FORWARD(Target, target)
                    .query(Tag{}, HPX_FORWARD(Args, args)...);
            }

            template <typename Target, typename... Args>
                requires(!has_query_v<Target, Tag, Args...>)
            constexpr auto operator()(Target&&, Args&&...) const noexcept
                -> decltype(property_not_supported<Tag, Target, Args...>());
        };

        struct get_priority_fallback
        {
            template <typename Target, typename... Args>
            HPX_FORCEINLINE constexpr auto operator()(
                Target&&, Args&&...) const noexcept
            {
                return hpx::threads::thread_priority::default_;
            }
        };

        struct get_stacksize_fallback
        {
            template <typename Target, typename... Args>
            HPX_FORCEINLINE constexpr auto operator()(
                Target&&, Args&&...) const noexcept
            {
                return hpx::threads::thread_stacksize::default_;
            }
        };

        struct get_hint_fallback
        {
            template <typename Target, typename... Args>
            HPX_FORCEINLINE constexpr auto operator()(
                Target&&, Args&&...) const noexcept
            {
                return hpx::threads::thread_schedule_hint{};
            }
        };

        struct get_annotation_fallback
        {
            template <typename Target, typename... Args>
            HPX_FORCEINLINE constexpr auto operator()(
                Target&&, Args&&...) const noexcept
            {
                return static_cast<char const*>(nullptr);
            }
        };

        struct get_first_core_fallback
        {
            template <typename Target, typename... Args>
            HPX_FORCEINLINE constexpr std::size_t operator()(
                Target&&, Args&&...) const noexcept
            {
                return 0;
            }
        };

        // NOLINTEND(bugprone-crtp-constructor-accessibility)
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    template <typename Property>
    struct is_scheduling_property<Property,
        std::enable_if_t<
            std::is_base_of_v<detail::property_base<Property>, Property>>>
      : std::true_type
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct with_priority_t final
      : detail::property_base<with_priority_t>
    {
    } with_priority{};

    template <>
    struct is_scheduling_property<with_priority_t> : std::true_type
    {
    };

    HPX_CXX_CORE_EXPORT inline constexpr struct get_priority_t final
      : detail::query_first_tag_fallback<get_priority_t,
            detail::get_priority_fallback>
    {
        constexpr get_priority_t() = default;
    } get_priority{};

    template <>
    struct is_scheduling_property<get_priority_t> : std::true_type
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct with_stacksize_t final
      : detail::property_base<with_stacksize_t>
    {
    } with_stacksize{};

    template <>
    struct is_scheduling_property<with_stacksize_t> : std::true_type
    {
    };

    HPX_CXX_CORE_EXPORT inline constexpr struct get_stacksize_t final
      : detail::query_first_tag_fallback<get_stacksize_t,
            detail::get_stacksize_fallback>
    {
        constexpr get_stacksize_t() = default;
    } get_stacksize{};

    template <>
    struct is_scheduling_property<get_stacksize_t> : std::true_type
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct with_hint_t final
      : detail::property_base<with_hint_t>
    {
    } with_hint{};

    template <>
    struct is_scheduling_property<with_hint_t> : std::true_type
    {
    };

    HPX_CXX_CORE_EXPORT inline constexpr struct get_hint_t final
      : detail::query_first_tag_fallback<get_hint_t, detail::get_hint_fallback>
    {
        constexpr get_hint_t() = default;
    } get_hint{};

    template <>
    struct is_scheduling_property<get_hint_t> : std::true_type
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct with_annotation_t final
    {
        template <typename Target, typename... Args>
            requires(has_query_v<Target, with_annotation_t, Args...>)
        constexpr auto operator()(Target&& target, Args&&... args) const
        {
            return HPX_FORWARD(Target, target)
                .query(*this, HPX_FORWARD(Args, args)...);
        }

        template <executor_any Executor>
            requires(!has_query_v<Executor, with_annotation_t, char const*>)
        constexpr auto operator()(Executor&& exec, char const* annotation) const
        {
            return detail::wrap_with_annotation(
                HPX_FORWARD(Executor, exec), annotation);
        }

        template <executor_any Executor>
            requires(!has_query_v<Executor, with_annotation_t, std::string>)
        auto operator()(Executor&& exec, std::string annotation) const
        {
            return detail::wrap_with_annotation(
                HPX_FORWARD(Executor, exec), HPX_MOVE(annotation));
        }
    } with_annotation{};

    template <>
    struct is_scheduling_property<with_annotation_t> : std::true_type
    {
    };

    HPX_CXX_CORE_EXPORT inline constexpr struct get_annotation_t final
      : detail::query_first_tag_fallback<get_annotation_t,
            detail::get_annotation_fallback>
    {
        constexpr get_annotation_t() = default;
    } get_annotation{};

    template <>
    struct is_scheduling_property<get_annotation_t> : std::true_type
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline constexpr struct with_first_core_t final
      : detail::property_base<with_first_core_t>
    {
    } with_first_core{};

    HPX_CXX_CORE_EXPORT inline constexpr struct get_first_core_t final
      : detail::query_first_tag_fallback<get_first_core_t,
            detail::get_first_core_fallback>
    {
        constexpr get_first_core_t() = default;
    } get_first_core{};

    template <>
    struct is_scheduling_property<get_first_core_t> : std::true_type
    {
    };
}    // namespace hpx::execution::experimental

namespace hpx {

    template <typename Property>
    concept scheduling_property =
        hpx::execution::experimental::is_scheduling_property_v<Property>;
}    // namespace hpx
