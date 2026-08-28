//  Copyright (c) 2026 Hartmut Kaiser
//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace hpx::tracing {

    HPX_CXX_CORE_EXPORT using enable_parent_task_handler_type = bool (*)();

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct [[maybe_unused]] annotation_handle
    {
    };

    HPX_CXX_CORE_EXPORT constexpr annotation_handle create_annotation_handle(
        char const*) noexcept
    {
        return {};
    }

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct region_init_data
    {
    };

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct [[maybe_unused]] loop_context
    {
        constexpr explicit loop_context() noexcept {}

        ~loop_context() = default;

        loop_context(loop_context const&) = delete;
        loop_context& operator=(loop_context const&) = delete;
    };

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct [[maybe_unused]] region
    {
        constexpr explicit region(
            loop_context&, region_init_data const&, std::size_t) noexcept
        {
        }
    };

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct [[maybe_unused]] mark_event
    {
        constexpr explicit mark_event(char const*) noexcept {}
    };

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct fiber_region_init_data
    {
    };

    HPX_CXX_CORE_EXPORT struct [[maybe_unused]] fiber_region
    {
        constexpr explicit fiber_region(
            fiber_region_init_data const&, std::size_t) noexcept
        {
        }
    };

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct [[maybe_unused]] fiber_suspend_region
    {
        constexpr explicit fiber_suspend_region(char const*) noexcept {}
    };

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct [[maybe_unused]] background_work_region
    {
        constexpr explicit background_work_region(std::size_t = 0) noexcept {}
    };

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct [[maybe_unused]] lock_context
    {
        constexpr explicit lock_context(
            char const* = nullptr, void const* = nullptr) noexcept
        {
        }
        constexpr explicit lock_context(
            char const*, char const*, void const* = nullptr) noexcept
        {
        }

        constexpr bool before_lock() const noexcept
        {
            return false;
        }

        constexpr void after_lock() const noexcept {}

        constexpr void after_try_lock(bool) const noexcept {}

        constexpr void before_unlock() const noexcept {}

        constexpr void after_unlock() const noexcept {}
    };

    ////////////////////////////////////////////////////////////////////////////
    namespace detail {
        HPX_CXX_CORE_EXPORT constexpr void sync_prepare(void const*) noexcept {}
        HPX_CXX_CORE_EXPORT constexpr void sync_acquired(void const*) noexcept
        {
        }
        HPX_CXX_CORE_EXPORT constexpr void sync_cancel(void const*) noexcept {}
        HPX_CXX_CORE_EXPORT constexpr void sync_releasing(void const*) noexcept
        {
        }
        HPX_CXX_CORE_EXPORT constexpr void sync_released(void const*) noexcept
        {
        }
    }    // namespace detail

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT constexpr void set_thread_name(char const*) noexcept {}

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT constexpr char const* rename_region(
        char const*) noexcept
    {
        return nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct [[maybe_unused]] task_timer_data
    {
        constexpr static bool valid() noexcept
        {
            return false;
        }
    };

    HPX_CXX_CORE_EXPORT constexpr task_timer_data create_task_timer(
        threads::thread_description const&, std::uint32_t,
        threads::thread_id const&) noexcept
    {
        return {};
    }

    HPX_CXX_CORE_EXPORT constexpr void update_task_timer(
        task_timer_data&, char const*) noexcept
    {
    }

    HPX_CXX_CORE_EXPORT struct [[maybe_unused]] scoped_task_timer
    {
        constexpr explicit scoped_task_timer(task_timer_data) noexcept {}

        constexpr void stop() noexcept {}
        constexpr void yield() noexcept {}

        template <typename T, typename State>
        constexpr void handle_post_execution(T* thrdptr, State s) noexcept
        {
            if (s == State::terminated || s == State::deleted)
            {
                thrdptr->set_timer_data({});
            }
        }
    };

    HPX_CXX_CORE_EXPORT constexpr void task_staged(
        char const*, void const* = nullptr) noexcept
    {
    }

    HPX_CXX_CORE_EXPORT constexpr void task_created(
        char const*, void const*, void const* = nullptr) noexcept
    {
    }

    HPX_CXX_CORE_EXPORT constexpr void task_executing(
        void const*, char const*, std::size_t) noexcept
    {
    }

    HPX_CXX_CORE_EXPORT constexpr void task_yielded(
        void const*, char const*) noexcept
    {
    }

    HPX_CXX_CORE_EXPORT constexpr void task_suspended(
        void const*, char const*, char const* = nullptr) noexcept
    {
    }

    HPX_CXX_CORE_EXPORT constexpr void task_resumed(
        void const*, char const*, char const* = nullptr) noexcept
    {
    }

    HPX_CXX_CORE_EXPORT constexpr void task_completed(
        void const*, char const*) noexcept
    {
    }

    HPX_CXX_CORE_EXPORT constexpr void task_deleted(void const*) noexcept {}

    /// \brief Producer-side signal: future state fulfilled (no-op stub).
    HPX_CXX_CORE_EXPORT constexpr void future_fulfilled(
        void const*, char const* = nullptr) noexcept
    {
    }

    /// \brief Producer-side signal: future exception set (no-op stub).
    HPX_CXX_CORE_EXPORT constexpr void future_exception_set(
        void const*, char const* = nullptr) noexcept
    {
    }

    /// \brief Consumer-side signal: continuation started (no-op stub).
    HPX_CXX_CORE_EXPORT constexpr void continuation_run(
        void const* = nullptr) noexcept
    {
    }

    /// \brief Consumer-side signal: continuation finished (no-op stub).
    HPX_CXX_CORE_EXPORT constexpr void continuation_finished(
        void const* = nullptr) noexcept
    {
    }

    /// \brief Consumer-side signal: handle_on_completed fired (no-op stub).
    HPX_CXX_CORE_EXPORT constexpr void handle_on_completed_fired(
        void const* = nullptr) noexcept
    {
    }

    /// \brief Signal emitted when a worker thread steals a task from another worker.
    HPX_CXX_CORE_EXPORT constexpr void work_stolen(
        std::size_t, std::size_t, void const*, char const* = nullptr) noexcept
    {
    }

    /// \brief Frame boundary marker (no-op stub).
    HPX_CXX_CORE_EXPORT constexpr void frame_mark(
        char const* = nullptr) noexcept
    {
    }

    HPX_CXX_CORE_EXPORT constexpr void os_thread_sleep(std::size_t) noexcept {}

    HPX_CXX_CORE_EXPORT constexpr void tracing_init(
        char const*, int, char**, std::uint32_t = 0, std::uint32_t = 1) noexcept
    {
    }

    HPX_CXX_CORE_EXPORT constexpr void tracing_finalize() noexcept {}

    HPX_CXX_CORE_EXPORT constexpr void register_thread(char const*) noexcept {}

    HPX_CXX_CORE_EXPORT constexpr void create_counter(
        std::string const&, std::string const&) noexcept
    {
    }

    HPX_CXX_CORE_EXPORT constexpr void sample_counter(
        std::string const&, std::string const&, double) noexcept
    {
    }

    HPX_CXX_CORE_EXPORT constexpr void send_parcel(
        std::uint64_t, std::uint64_t, std::uint64_t) noexcept
    {
    }

    HPX_CXX_CORE_EXPORT constexpr void recv_parcel(
        std::uint64_t, std::uint64_t, std::uint64_t, std::uint64_t) noexcept
    {
    }

    HPX_CXX_CORE_EXPORT constexpr void set_enable_parent_task_handler(
        enable_parent_task_handler_type) noexcept
    {
    }

}    // namespace hpx::tracing
