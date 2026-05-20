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
        restricted_policy_executor(
            restricted_policy_executor const& other)
            noexcept(std::is_nothrow_copy_constructible_v<embedded_executor>)
          : first_thread_(other.first_thread_)
          , os_thread_(other.os_thread_.load())
          , exec_(other.exec_)
        {
        }

        restricted_policy_executor&
        operator=(restricted_policy_executor const& rhs)
            noexcept(std::is_nothrow_copy_assignable_v<embedded_executor>)
        {
            first_thread_ = rhs.first_thread_;
            os_thread_ = rhs.os_thread_.load();
            exec_ = rhs.exec_;
            return *this;
        }
        // clang-format on

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

    private:
        // property implementations

        // support all properties exposed by the embedded executor
        template <typename Tag, typename Property>
            requires(
                hpx::execution::experimental::is_scheduling_property_v<Tag> &&
                hpx::functional::is_tag_invocable_v<Tag, embedded_executor,
                    Property>)
        friend restricted_policy_executor tag_invoke(
            Tag tag, restricted_policy_executor const& exec, Property&& prop)
        {
            auto exec_with_prop = exec;
            exec_with_prop.exec_ = hpx::functional::tag_invoke(tag,
                exec.generate_executor(exec.get_current_thread_num()),
                HPX_FORWARD(Property, prop));
            return exec_with_prop;
        }

        template <scheduling_property Tag>
            requires(
                hpx::functional::is_tag_invocable_v<Tag, embedded_executor>)
        friend decltype(auto) tag_invoke(
            Tag tag, restricted_policy_executor const& exec)
        {
            return tag(exec.generate_executor(exec.get_current_thread_num()));
        }

        template <executor_parameters Parameters>
        friend constexpr std::size_t tag_invoke(
            hpx::execution::experimental::processing_units_count_t tag,
            Parameters&& params, restricted_policy_executor const& exec,
            hpx::chrono::steady_duration const& duration =
                hpx::chrono::null_duration,
            std::size_t num_tasks = 0)
        {
            return tag(HPX_FORWARD(Parameters, params),
                exec.generate_executor(exec.get_current_thread_num()), duration,
                num_tasks);
        }

        // executor API
        template <typename F, typename... Ts>
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::sync_execute_t,
            restricted_policy_executor const& exec, F&& f, Ts&&... ts)
        {
            return hpx::parallel::execution::sync_execute(
                exec.generate_executor(exec.get_next_thread_num()),
                HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename... Ts>
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::async_execute_t,
            restricted_policy_executor const& exec, F&& f, Ts&&... ts)
        {
            return hpx::parallel::execution::async_execute(
                exec.generate_executor(exec.get_next_thread_num()),
                HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename Future, typename... Ts>
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::then_execute_t,
            restricted_policy_executor const& exec, F&& f, Future&& predecessor,
            Ts&&... ts)
        {
            return hpx::parallel::execution::then_execute(
                exec.generate_executor(exec.get_next_thread_num()),
                HPX_FORWARD(F, f), HPX_FORWARD(Future, predecessor),
                HPX_FORWARD(Ts, ts)...);
        }

        // NonBlockingOneWayExecutor (adapted) interface
        template <typename F, typename... Ts>
        friend decltype(auto) tag_invoke(hpx::parallel::execution::post_t,
            restricted_policy_executor const& exec, F&& f, Ts&&... ts)
        {
            return hpx::parallel::execution::post(
                exec.generate_executor(exec.get_next_thread_num()),
                HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename S, typename... Ts>
            requires(!std::is_integral_v<S>)
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::bulk_async_execute_t,
            restricted_policy_executor const& exec, F&& f, S const& shape,
            Ts&&... ts)
        {
            return hpx::parallel::execution::bulk_async_execute(
                exec.generate_executor(exec.first_thread_), HPX_FORWARD(F, f),
                shape, HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename S, typename Future, typename... Ts>
            requires(!std::is_integral_v<S>)
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::bulk_then_execute_t,
            restricted_policy_executor const& exec, F&& f, S const& shape,
            Future&& predecessor, Ts&&... ts)
        {
            return hpx::parallel::execution::bulk_then_execute(
                exec.generate_executor(exec.first_thread_), HPX_FORWARD(F, f),
                shape, HPX_FORWARD(Future, predecessor),
                HPX_FORWARD(Ts, ts)...);
        }
        /// \endcond

    public:
        // clang-format off
        constexpr hpx::execution::experimental::executor_scheduler<
            restricted_policy_executor>
        query(hpx::execution::experimental::get_scheduler_t) const
            noexcept(std::is_nothrow_copy_constructible_v<
                restricted_policy_executor>)
        {
            return hpx::execution::experimental::executor_scheduler<
                restricted_policy_executor>(*this);
        }

        constexpr hpx::execution::experimental::detail::executor_sender<
            restricted_policy_executor>
        schedule() const
            noexcept(std::is_nothrow_copy_constructible_v<
                restricted_policy_executor>)
        {
            return hpx::execution::experimental::executor_scheduler<
                restricted_policy_executor>(*this)
                .schedule();
        }
        // clang-format on

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
