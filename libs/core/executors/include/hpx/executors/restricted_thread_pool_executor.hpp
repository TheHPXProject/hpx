//  Copyright (c)      2020 Mikael Simberg
//  Copyright (c) 2007-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/executors/restricted_thread_pool_executors.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/executors/executor_scheduler.hpp>
#include <hpx/executors/parallel_executor.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/execution.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::execution::experimental {

    HPX_CXX_CORE_EXPORT template <typename Policy>
    class restricted_policy_executor
    {
    private:
        static constexpr std::size_t hierarchical_threshold_default_ = 6;

        using embedded_executor =
            hpx::execution::parallel_policy_executor<Policy>;

    public:
        /// Associate the parallel_execution_tag executor tag type as a default
        /// with this executor.
        using execution_category =
            typename embedded_executor::execution_category;

        using executor_parameters_type =
            typename embedded_executor::executor_parameters_type;

        /// Create a new parallel executor
        explicit restricted_policy_executor(std::size_t first_thread = 0,
            std::size_t num_threads = 1,
            threads::thread_priority priority =
                threads::thread_priority::default_,
            threads::thread_stacksize stacksize =
                threads::thread_stacksize::default_,
            threads::thread_schedule_hint schedulehint = {},
            [[maybe_unused]] std::size_t hierarchical_threshold =
                hierarchical_threshold_default_)
          : first_thread_(static_cast<std::uint16_t>(first_thread))
          , os_thread_(0)
          , exec_(priority, stacksize, schedulehint,
                parallel::execution::detail::get_default_policy<Policy>::call())
        {
            // set initial number of cores
            exec_ = hpx::execution::experimental::with_processing_units_count(
                exec_, num_threads);
        }

        // clang-format off
        restricted_policy_executor(restricted_policy_executor const& other)
            noexcept(std::is_nothrow_copy_constructible_v<embedded_executor>)
          // clang-format on
          : first_thread_(other.first_thread_)
          , os_thread_(other.os_thread_.load())
          , exec_(other.exec_)
        {
        }

        restricted_policy_executor(restricted_policy_executor&& other) noexcept
          : first_thread_(other.first_thread_)
          , os_thread_(other.os_thread_.load())
          , exec_(HPX_MOVE(other.exec_))
        {
        }

        // clang-format off
        restricted_policy_executor& operator=(
            restricted_policy_executor const& rhs)
            noexcept(std::is_nothrow_copy_assignable_v<embedded_executor>)
        // clang-format on
        {
            first_thread_ = rhs.first_thread_;
            os_thread_ = rhs.os_thread_.load();
            exec_ = rhs.exec_;
            return *this;
        }

        restricted_policy_executor& operator=(
            restricted_policy_executor&& rhs) noexcept
        {
            first_thread_ = rhs.first_thread_;
            os_thread_ = rhs.os_thread_.load();
            exec_ = HPX_MOVE(rhs.exec_);
            return *this;
        }

        /// \cond NOINTERNAL
        bool operator==(restricted_policy_executor const& rhs) const noexcept
        {
            return exec_ == rhs.exec_ && first_thread_ == rhs.first_thread_;
        }

        bool operator!=(restricted_policy_executor const& rhs) const noexcept
        {
            return !(*this == rhs);
        }

        restricted_policy_executor const& context() const noexcept
        {
            return *this;
        }

    private:
        // this function is conceptually const (os_threads_ is mutable)
        std::int16_t get_next_thread_num() const
        {
            return static_cast<std::int16_t>(first_thread_ +
                (os_thread_++ %
                    hpx::execution::experimental::processing_units_count(
                        exec_)));
        }

        std::int16_t get_current_thread_num() const
        {
            return static_cast<std::int16_t>(first_thread_ + os_thread_++);
        }

        embedded_executor generate_executor(std::uint16_t thread_num) const
        {
            return hpx::execution::experimental::with_hint(exec_,
                threads::thread_schedule_hint(
                    static_cast<std::int16_t>(thread_num)));
        }

    public:
        // property implementations

        // support all properties exposed by the embedded executor
        template <typename Tag, typename Property>
            requires(
                hpx::execution::experimental::is_scheduling_property_v<Tag>)
        restricted_policy_executor query(Tag tag, Property&& prop) const
        {
            auto exec_with_prop = *this;
            exec_with_prop.exec_ =
                tag(generate_executor(get_current_thread_num()),
                    HPX_FORWARD(Property, prop));
            return exec_with_prop;
        }

        template <scheduling_property Tag>
        decltype(auto) query(Tag tag) const
        {
            return tag(generate_executor(get_current_thread_num()));
        }

        template <executor_parameters Parameters>
        constexpr std::size_t query(
            hpx::execution::experimental::processing_units_count_t tag,
            Parameters&& params,
            hpx::chrono::steady_duration const& duration =
                hpx::chrono::null_duration,
            std::size_t num_tasks = 0) const
        {
            return tag(HPX_FORWARD(Parameters, params),
                generate_executor(get_current_thread_num()), duration,
                num_tasks);
        }

        // executor API
        template <typename F, typename... Ts>
        decltype(auto) sync_execute(F&& f, Ts&&... ts) const
        {
            return hpx::parallel::execution::sync_execute(
                generate_executor(get_next_thread_num()), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename... Ts>
        decltype(auto) async_execute(F&& f, Ts&&... ts) const
        {
            return hpx::parallel::execution::async_execute(
                generate_executor(get_next_thread_num()), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename Future, typename... Ts>
        decltype(auto) then_execute(
            F&& f, Future&& predecessor, Ts&&... ts) const
        {
            return hpx::parallel::execution::then_execute(
                generate_executor(get_next_thread_num()), HPX_FORWARD(F, f),
                HPX_FORWARD(Future, predecessor), HPX_FORWARD(Ts, ts)...);
        }

        // NonBlockingOneWayExecutor (adapted) interface
        template <typename F, typename... Ts>
        decltype(auto) post(F&& f, Ts&&... ts) const
        {
            return hpx::parallel::execution::post(
                generate_executor(get_next_thread_num()), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename S, typename... Ts>
            requires(!std::is_integral_v<S>)
        decltype(auto) bulk_async_execute(
            F&& f, S const& shape, Ts&&... ts) const
        {
            return hpx::parallel::execution::bulk_async_execute(
                generate_executor(first_thread_), HPX_FORWARD(F, f), shape,
                HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename S, typename Future, typename... Ts>
            requires(!std::is_integral_v<S>)
        decltype(auto) bulk_then_execute(
            F&& f, S const& shape, Future&& predecessor, Ts&&... ts) const
        {
            return hpx::parallel::execution::bulk_then_execute(
                generate_executor(first_thread_), HPX_FORWARD(F, f), shape,
                HPX_FORWARD(Future, predecessor), HPX_FORWARD(Ts, ts)...);
        }
        /// \endcond

    public:
        // clang-format off
        constexpr hpx::execution::experimental::executor_scheduler<
            restricted_policy_executor>
        query(hpx::execution::experimental::get_scheduler_t) const
            noexcept(std::is_nothrow_copy_constructible_v<embedded_executor>)
        // clang-format on
        {
            return hpx::execution::experimental::executor_scheduler<
                restricted_policy_executor>(*this);
        }

    private:
        std::uint16_t first_thread_;
        mutable std::atomic<std::size_t> os_thread_;

        embedded_executor exec_;
    };

    HPX_CXX_CORE_EXPORT using restricted_thread_pool_executor =
        restricted_policy_executor<hpx::launch>;
}    // namespace hpx::execution::experimental

namespace hpx::execution::experimental {

    ///////////////////////////////////////////////////////////////////////////
    /// \cond NOINTERNAL
    template <typename Policy>
    struct is_never_blocking_one_way_executor<
        restricted_policy_executor<Policy>>
      : is_never_blocking_one_way_executor<
            hpx::execution::parallel_policy_executor<Policy>>
    {
    };

    template <typename Policy>
    struct is_two_way_executor<restricted_policy_executor<Policy>>
      : is_two_way_executor<hpx::execution::parallel_policy_executor<Policy>>
    {
    };

    template <typename Policy>
    struct is_bulk_two_way_executor<restricted_policy_executor<Policy>>
      : is_bulk_two_way_executor<
            hpx::execution::parallel_policy_executor<Policy>>
    {
    };

    template <typename Policy>
    struct is_scheduler_executor<restricted_policy_executor<Policy>>
      : is_scheduler_executor<hpx::execution::parallel_policy_executor<Policy>>
    {
    };
    /// \endcond
}    // namespace hpx::execution::experimental
