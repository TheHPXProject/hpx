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

#include <hpx/modules/tracy.hpp>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::tracing {

    HPX_CXX_CORE_EXPORT using enable_parent_task_handler_type = bool (*)();

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT using annotation_handle = char const*;

    HPX_CXX_CORE_EXPORT constexpr annotation_handle create_annotation_handle(
        char const* name) noexcept
    {
        return name;
    }

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct region_init_data
    {
        char const* name = nullptr;
        std::size_t thread_phase = 0;
        bool is_stackless = false;
    };

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT [[maybe_unused]] loop_context
    {
        constexpr explicit loop_context() noexcept {}

        ~loop_context() = default;

        loop_context(loop_context const&) = delete;
        loop_context& operator=(loop_context const&) = delete;
    };

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT region
    {
        explicit region(loop_context&, region_init_data const& init_data,
            std::size_t num_thread) noexcept;

        ~region();

    private:
        static hpx::tracy::region create_tracy_region(
            region_init_data const& data, std::size_t num_thread) noexcept;

        hpx::tracy::region impl;
    };

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT mark_event
    {
        explicit mark_event(char const* name) noexcept;
        ~mark_event();

    private:
        hpx::tracy::mark_event impl;
    };

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct fiber_region_init_data
    {
        char const* name = nullptr;
        char const* fiber_name = nullptr;
        bool is_stackless = false;
    };

    HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT fiber_region
    {
        explicit fiber_region(fiber_region_init_data const& data,
            std::size_t num_thread) noexcept;

        ~fiber_region();

    private:
        static hpx::tracy::fiber_region create_tracy_fiber_region(
            fiber_region_init_data const& data,
            std::size_t num_thread) noexcept;

        hpx::tracy::fiber_region impl;
    };

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT fiber_suspend_region
    {
        explicit fiber_suspend_region(char const* desc) noexcept;
        ~fiber_suspend_region();

    private:
        hpx::tracy::fiber_suspend_region impl;
    };

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT [[maybe_unused]] lock_context
    {
        explicit lock_context(
            char const* name = nullptr, void const* addr = nullptr) noexcept;
        explicit lock_context(char const* prefix, char const* suffix,
            void const* addr = nullptr) noexcept;

        ~lock_context();

        lock_context(lock_context const&) = delete;
        lock_context& operator=(lock_context const&) = delete;

        bool before_lock() const noexcept;
        void after_lock() const noexcept;
        void after_try_lock(bool acquired) const noexcept;
        void before_unlock() const noexcept;
        void after_unlock() const noexcept;

    private:
        hpx::tracy::lock_data impl;
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
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void set_thread_name(
        char const* name) noexcept;

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT char const* rename_region(
        char const* name) noexcept;

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

    HPX_CXX_CORE_EXPORT constexpr void tracing_init(
        char const*, int, char**, std::uint32_t = 0, std::uint32_t = 1) noexcept
    {
    }

    HPX_CXX_CORE_EXPORT constexpr void tracing_finalize() noexcept {}

    HPX_CXX_CORE_EXPORT constexpr void register_thread(char const*) noexcept {}

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void create_counter(
        std::string const& full_name, std::string const& short_name) noexcept;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void sample_counter(
        std::string const& name, std::string const& short_name,
        double value) noexcept;

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

#include <hpx/config/warnings_suffix.hpp>
