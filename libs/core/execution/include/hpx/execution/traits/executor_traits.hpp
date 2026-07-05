//  Copyright (c) 2017-2024 Hartmut Kaiser
//  Copyright (c) 2026 Sai Charan Arvapally
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/execution/traits/is_execution_policy.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx::execution {

    namespace experimental {

        HPX_CXX_CORE_EXPORT struct adaptive_static_chunk_size;
        HPX_CXX_CORE_EXPORT struct auto_chunk_size;
        HPX_CXX_CORE_EXPORT struct default_parameters;
        HPX_CXX_CORE_EXPORT struct dynamic_chunk_size;
        HPX_CXX_CORE_EXPORT struct guided_chunk_size;
        HPX_CXX_CORE_EXPORT struct persistent_auto_chunk_size;
        HPX_CXX_CORE_EXPORT struct static_chunk_size;
        HPX_CXX_CORE_EXPORT struct num_cores;
    }    // namespace experimental

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct sequenced_execution_tag;
    HPX_CXX_CORE_EXPORT struct parallel_execution_tag;
    HPX_CXX_CORE_EXPORT struct unsequenced_execution_tag;
}    // namespace hpx::execution

namespace hpx::execution::experimental {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_HAS_MEMBER_XXX_TRAIT_DEF(post)
        HPX_HAS_MEMBER_XXX_TRAIT_DEF(sync_execute)
        HPX_HAS_MEMBER_XXX_TRAIT_DEF(async_execute)
        HPX_HAS_MEMBER_XXX_TRAIT_DEF(then_execute)
        HPX_HAS_MEMBER_XXX_TRAIT_DEF(bulk_sync_execute)
        HPX_HAS_MEMBER_XXX_TRAIT_DEF(bulk_async_execute)
        HPX_HAS_MEMBER_XXX_TRAIT_DEF(bulk_then_execute)
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct has_post_member : detail::has_post<std::decay_t<T>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct has_sync_execute_member : detail::has_sync_execute<std::decay_t<T>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct has_async_execute_member : detail::has_async_execute<std::decay_t<T>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct has_then_execute_member : detail::has_then_execute<std::decay_t<T>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct has_bulk_sync_execute_member
      : detail::has_bulk_sync_execute<std::decay_t<T>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct has_bulk_async_execute_member
      : detail::has_bulk_async_execute<std::decay_t<T>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct has_bulk_then_execute_member
      : detail::has_bulk_then_execute<std::decay_t<T>>
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Executor>
    struct executor_context
    {
        using type =
            std::decay_t<decltype(std::declval<Executor const&>().context())>;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Components that create groups of execution agents may use execution
    // categories to communicate the forward progress and ordering guarantees of
    // these execution agents with respect to other agents within the same
    // group.

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Executor>
    struct executor_execution_category
    {
    private:
        template <typename T>
        using execution_category = typename T::execution_category;

    public:
        using type =
            hpx::util::detected_or_t<hpx::execution::unsequenced_execution_tag,
                execution_category, Executor>;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Executor>
    struct executor_shape
    {
    private:
        template <typename T>
        using shape_type = typename T::shape_type;

    public:
        using type =
            hpx::util::detected_or_t<std::size_t, shape_type, Executor>;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Executor>
    struct executor_index
    {
    private:
        // exposition only
        template <typename T>
        using index_type = typename T::index_type;

    public:
        using type =
            hpx::util::detected_or_t<typename executor_shape<Executor>::type,
                index_type, Executor>;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Executor>
    struct executor_parameters_type
    {
    private:
        template <typename T>
        using parameters_type = typename T::parameters_type;

    public:
        using type = hpx::util::detected_or_t<
            hpx::execution::experimental::default_parameters, parameters_type,
            Executor>;
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename Executor, typename T,
            typename Ts, typename Enable = void>
        struct executor_future;

        HPX_CXX_CORE_EXPORT template <typename Executor, typename T,
            typename Enable = void>
        struct exposes_future_type : std::false_type
        {
        };

        template <typename Executor, typename T>
        struct exposes_future_type<Executor, T,
            std::void_t<typename Executor::template future_type<T>>>
          : std::true_type
        {
        };

        template <typename Executor, typename T, typename... Ts>
        struct executor_future<Executor, T, hpx::util::pack<Ts...>,
            std::enable_if_t<hpx::traits::is_two_way_executor_v<Executor> &&
                exposes_future_type<Executor, T>::value>>
        {
            using type = typename Executor::template future_type<T>;
        };

        template <typename Executor, typename T, typename... Ts>
        struct executor_future<Executor, T, hpx::util::pack<Ts...>,
            std::enable_if_t<hpx::traits::is_two_way_executor_v<Executor> &&
                has_async_execute_member<Executor>::value &&
                !exposes_future_type<Executor, T>::value>>
        {
            using type = decltype(std::declval<Executor&&>().async_execute(
                std::declval<T (*)(Ts...)>(), std::declval<Ts>()...));
        };

        // Fallback for two-way executors that don't have an async_execute
        // member and don't expose future_type. With tag_invoke removed,
        // executors should provide async_execute as a public member function
        // (the specialization above will deduce the return type). This
        // fallback assumes hpx::future<T> for executors that haven't been
        // migrated yet.
        template <typename Executor, typename T, typename... Ts>
        struct executor_future<Executor, T, hpx::util::pack<Ts...>,
            std::enable_if_t<hpx::traits::is_two_way_executor_v<Executor> &&
                !has_async_execute_member<Executor>::value &&
                !exposes_future_type<Executor, T>::value>>
        {
            using type = hpx::future<T>;
        };

        template <typename Executor, typename T, typename Ts>
        struct executor_future<Executor, T, Ts,
            std::enable_if_t<!hpx::traits::is_two_way_executor_v<Executor>>>
        {
            using type = hpx::future<T>;
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename Executor, typename T, typename... Ts>
    struct executor_future
      : detail::executor_future<std::decay_t<Executor>, T,
            hpx::util::pack<std::decay_t<Ts>...>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Executor, typename T, typename... Ts>
    using executor_future_t =
        typename executor_future<Executor, T, Ts...>::type;
}    // namespace hpx::execution::experimental

namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct has_post_member
      : hpx::execution::experimental::has_post_member<std::decay_t<T>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct has_sync_execute_member
      : hpx::execution::experimental::has_sync_execute_member<std::decay_t<T>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct has_async_execute_member
      : hpx::execution::experimental::has_async_execute_member<std::decay_t<T>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct has_then_execute_member
      : hpx::execution::experimental::has_then_execute_member<std::decay_t<T>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct has_bulk_sync_execute_member
      : hpx::execution::experimental::has_bulk_sync_execute_member<
            std::decay_t<T>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct has_bulk_async_execute_member
      : hpx::execution::experimental::has_bulk_async_execute_member<
            std::decay_t<T>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct has_bulk_then_execute_member
      : hpx::execution::experimental::has_bulk_then_execute_member<
            std::decay_t<T>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool has_post_member_v = has_post_member<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool has_sync_execute_member_v =
        has_sync_execute_member<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool has_async_execute_member_v =
        has_async_execute_member<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool has_then_execute_member_v =
        has_then_execute_member<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool has_bulk_sync_execute_member_v =
        has_bulk_sync_execute_member<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool has_bulk_async_execute_member_v =
        has_bulk_async_execute_member<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool has_bulk_then_execute_member_v =
        has_bulk_then_execute_member<T>::value;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Executor, typename Enable = void>
    struct executor_context
      : hpx::execution::experimental::executor_context<std::decay_t<Executor>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Executor>
    using executor_context_t = typename executor_context<Executor>::type;

    HPX_CXX_CORE_EXPORT template <typename Executor, typename Enable = void>
    struct executor_execution_category
      : hpx::execution::experimental::executor_execution_category<
            std::decay_t<Executor>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Executor>
    using executor_execution_category_t =
        typename executor_execution_category<Executor>::type;

    HPX_CXX_CORE_EXPORT template <typename Executor, typename Enable = void>
    struct executor_shape
      : hpx::execution::experimental::executor_shape<std::decay_t<Executor>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Executor>
    using executor_shape_t = typename executor_shape<Executor>::type;

    HPX_CXX_CORE_EXPORT template <typename Executor, typename Enable = void>
    struct executor_index
      : hpx::execution::experimental::executor_index<std::decay_t<Executor>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Executor>
    using executor_index_t = typename executor_index<Executor>::type;

    HPX_CXX_CORE_EXPORT template <typename Executor, typename T, typename... Ts>
    struct executor_future
      : hpx::execution::experimental::executor_future<std::decay_t<Executor>, T,
            std::decay_t<Ts>...>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Executor, typename T, typename... Ts>
    using executor_future_t =
        typename executor_future<Executor, T, Ts...>::type;

    ///////////////////////////////////////////////////////////////////////////
    // extension
    HPX_CXX_CORE_EXPORT template <typename Executor, typename Enable = void>
    struct executor_parameters_type
      : hpx::execution::experimental::executor_parameters_type<
            std::decay_t<Executor>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Executor>
    using executor_parameters_type_t =
        typename executor_parameters_type<Executor>::type;
}    // namespace hpx::traits
