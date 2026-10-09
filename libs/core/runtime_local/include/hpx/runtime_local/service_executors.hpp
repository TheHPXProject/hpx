//  Copyright (c) 2007-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file service_executors.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/executors.hpp>

#include <cstdint>
#include <type_traits>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::execution::experimental {

    HPX_CXX_CORE_EXPORT enum class service_executor_type : std::uint8_t {
        io_thread_pool,        ///< Selects creating a service executor using
                               ///< the I/O pool of threads
        parcel_thread_pool,    ///< Selects creating a service executor using
                               ///< the parcel pool of threads
        timer_thread_pool,     ///< Selects creating a service executor using
                               ///< the timer pool of threads
        main_thread            ///< Selects creating a service executor using
                               ///< the main thread
    };

    namespace detail {

        HPX_CORE_EXPORT hpx::util::io_service_pool* get_service_pool(
            service_executor_type t, char const* name_suffix = "");
    }    // namespace detail

    HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT service_executor
      : public parallel::execution::detail::service_executor
    {
        explicit service_executor(
            service_executor_type t, char const* name_suffix = "");
    };

    HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT io_pool_executor
      : service_executor
    {
        io_pool_executor();
    };

    HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT parcel_pool_executor
      : service_executor
    {
        explicit parcel_pool_executor(char const* name_suffix = "-tcp");
    };

    HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT timer_pool_executor
      : service_executor
    {
        timer_pool_executor();
    };

    HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT main_pool_executor
      : service_executor
    {
        main_pool_executor();
    };

    ///  \cond NOINTERNAL
    template <>
    struct is_one_way_executor<parallel::execution::detail::service_executor>
      : std::true_type
    {
    };

    template <>
    struct is_two_way_executor<parallel::execution::detail::service_executor>
      : std::true_type
    {
    };

    template <>
    struct is_bulk_two_way_executor<
        parallel::execution::detail::service_executor> : std::true_type
    {
    };

    template <>
    struct is_one_way_executor<service_executor> : std::true_type
    {
    };

    template <>
    struct is_two_way_executor<service_executor> : std::true_type
    {
    };

    template <>
    struct is_bulk_two_way_executor<service_executor> : std::true_type
    {
    };

    template <>
    struct is_one_way_executor<io_pool_executor> : std::true_type
    {
    };

    template <>
    struct is_two_way_executor<io_pool_executor> : std::true_type
    {
    };

    template <>
    struct is_bulk_two_way_executor<io_pool_executor> : std::true_type
    {
    };

    template <>
    struct is_one_way_executor<parcel_pool_executor> : std::true_type
    {
    };

    template <>
    struct is_two_way_executor<parcel_pool_executor> : std::true_type
    {
    };

    template <>
    struct is_bulk_two_way_executor<parcel_pool_executor> : std::true_type
    {
    };

    template <>
    struct is_one_way_executor<timer_pool_executor> : std::true_type
    {
    };

    template <>
    struct is_two_way_executor<timer_pool_executor> : std::true_type
    {
    };

    template <>
    struct is_bulk_two_way_executor<timer_pool_executor> : std::true_type
    {
    };

    template <>
    struct is_one_way_executor<main_pool_executor> : std::true_type
    {
    };

    template <>
    struct is_two_way_executor<main_pool_executor> : std::true_type
    {
    };

    template <>
    struct is_bulk_two_way_executor<main_pool_executor> : std::true_type
    {
    };
    /// \endcond
}    // namespace hpx::execution::experimental

#include <hpx/config/warnings_suffix.hpp>
