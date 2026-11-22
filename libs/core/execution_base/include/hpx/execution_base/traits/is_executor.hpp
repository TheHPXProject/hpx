//  Copyright (c) 2017-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/preprocessor.hpp>
#include <hpx/modules/type_support.hpp>

#include <type_traits>

namespace hpx::execution::experimental {

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename T>
        struct is_one_way_executor : std::false_type
        {
        };

        HPX_CXX_CORE_EXPORT template <typename T>
        struct is_never_blocking_one_way_executor : std::false_type
        {
        };

        HPX_CXX_CORE_EXPORT template <typename T>
        struct is_bulk_one_way_executor : std::false_type
        {
        };

        HPX_CXX_CORE_EXPORT template <typename T>
        struct is_two_way_executor : std::false_type
        {
        };

        HPX_CXX_CORE_EXPORT template <typename T>
        struct is_bulk_two_way_executor : std::false_type
        {
        };

        HPX_CXX_CORE_EXPORT template <typename T>
        struct is_scheduler_executor : std::false_type
        {
        };
    }    // namespace detail

    // Executor type traits:

    // Condition: T meets the syntactic requirements for OneWayExecutor
    //            (supports sync_execute)
    // Precondition: T is a complete type
    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct is_one_way_executor : detail::is_one_way_executor<std::decay_t<T>>
    {
    };

    // Condition: T meets the syntactic requirements for NonBlockingOneWayExecutor
    //            (supports post_execute)
    // Precondition: T is a complete type
    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct is_never_blocking_one_way_executor
      : detail::is_never_blocking_one_way_executor<std::decay_t<T>>
    {
    };

    // Condition: T meets the syntactic requirements for BulkOneWayExecutor
    //            (supports bulk_sync_execute)
    // Precondition: T is a complete type
    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct is_bulk_one_way_executor
      : detail::is_bulk_one_way_executor<std::decay_t<T>>
    {
    };

    // Condition: T meets the syntactic requirements for TwoWayExecutor
    //            (supports async_execute)
    // Precondition: T is a complete type
    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct is_two_way_executor : detail::is_two_way_executor<std::decay_t<T>>
    {
    };

    // Condition: T meets the syntactic requirements for BulkTwoWayExecutor
    //            (supports bulk_async_execute)
    // Precondition: T is a complete type
    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct is_bulk_two_way_executor
      : detail::is_bulk_two_way_executor<std::decay_t<T>>
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    // is_scheduler_executor evaluates to true for executors that return senders
    // from their scheduling functions
    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct is_scheduler_executor
      : detail::is_scheduler_executor<std::decay_t<T>>
    {
    };
}    // namespace hpx::execution::experimental

namespace hpx::traits {

    // Concurrency TS V2: executor framework
    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct is_one_way_executor
      : execution::experimental::is_one_way_executor<std::decay_t<T>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_one_way_executor_v = is_one_way_executor<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct is_never_blocking_one_way_executor
      : execution::experimental::is_never_blocking_one_way_executor<
            std::decay_t<T>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_never_blocking_one_way_executor_v =
        is_never_blocking_one_way_executor<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct is_bulk_one_way_executor
      : execution::experimental::is_bulk_one_way_executor<std::decay_t<T>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_bulk_one_way_executor_v =
        is_bulk_one_way_executor<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct is_two_way_executor
      : execution::experimental::is_two_way_executor<std::decay_t<T>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_two_way_executor_v = is_two_way_executor<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct is_bulk_two_way_executor
      : execution::experimental::is_bulk_two_way_executor<std::decay_t<T>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_bulk_two_way_executor_v =
        is_bulk_two_way_executor<T>::value;

    // trait testing for any of the above
    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct is_executor_any
      : util::any_of<is_one_way_executor<T>,
            is_never_blocking_one_way_executor<T>, is_bulk_one_way_executor<T>,
            is_two_way_executor<T>, is_bulk_two_way_executor<T>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_executor_any_v = is_executor_any<T>::value;

    ///////////////////////////////////////////////////////////////////////////
    // is_scheduler_executor evaluates to true for executors that return senders
    // from their scheduling functions
    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct is_scheduler_executor
      : execution::experimental::is_scheduler_executor<T>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_scheduler_executor_v =
        is_scheduler_executor<T>::value;
}    // namespace hpx::traits

namespace hpx {

    HPX_CXX_CORE_EXPORT template <typename Executor>
    concept one_way_executor = hpx::traits::is_one_way_executor_v<Executor>;

    HPX_CXX_CORE_EXPORT template <typename Executor>
    concept never_blocking_one_way_executor =
        hpx::traits::is_never_blocking_one_way_executor_v<Executor>;

    HPX_CXX_CORE_EXPORT template <typename Executor>
    concept bulk_one_way_executor =
        hpx::traits::is_bulk_one_way_executor_v<Executor>;

    HPX_CXX_CORE_EXPORT template <typename Executor>
    concept two_way_executor = hpx::traits::is_two_way_executor_v<Executor>;

    HPX_CXX_CORE_EXPORT template <typename Executor>
    concept bulk_two_way_executor =
        hpx::traits::is_bulk_two_way_executor_v<Executor>;

    HPX_CXX_CORE_EXPORT template <typename Executor>
    concept executor_any = hpx::traits::is_executor_any_v<Executor>;

    HPX_CXX_CORE_EXPORT template <typename T>
    concept scheduler_executor = hpx::traits::is_scheduler_executor_v<T>;
}    // namespace hpx

// backwards compatibility layer
namespace hpx::parallel::execution {

#define HPX_IS_EXECUTOR_DEPRECATED(name)                                       \
    template <typename T, typename Enable = void>                              \
    using name HPX_DEPRECATED_V(1, 11,                                         \
        "hpx::parallel::execution::" HPX_PP_STRINGIZE(                         \
            name) " is deprecated, use "                                       \
                  "hpx::execution::experimental::" HPX_PP_STRINGIZE(           \
                      name) " instead") =                                      \
        ::hpx::execution::experimental::name<T, Enable> /**/

    HPX_IS_EXECUTOR_DEPRECATED(is_one_way_executor);
    HPX_IS_EXECUTOR_DEPRECATED(is_never_blocking_one_way_executor);
    HPX_IS_EXECUTOR_DEPRECATED(is_bulk_one_way_executor);
    HPX_IS_EXECUTOR_DEPRECATED(is_two_way_executor);
    HPX_IS_EXECUTOR_DEPRECATED(is_bulk_two_way_executor);
    HPX_IS_EXECUTOR_DEPRECATED(is_scheduler_executor);
}    // namespace hpx::parallel::execution
