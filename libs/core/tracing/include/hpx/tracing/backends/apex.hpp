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
    HPX_CXX_CORE_EXPORT using annotation_handle = char const*;

    HPX_CXX_CORE_EXPORT constexpr annotation_handle create_annotation_handle(
        char const* name) noexcept
    {
        return name;
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
    HPX_CXX_CORE_EXPORT struct task_timer_data
    {
        std::shared_ptr<util::external_timer::task_wrapper> data;

        task_timer_data() noexcept = default;

        explicit task_timer_data(
            std::shared_ptr<util::external_timer::task_wrapper> data) noexcept
          : data(HPX_MOVE(data))
        {
        }

        operator std::shared_ptr<util::external_timer::task_wrapper>()
            const noexcept
        {
            return data;
        }

        constexpr bool valid() const noexcept
        {
            return static_cast<bool>(data);
        }
    };

    namespace detail {

        HPX_CXX_CORE_EXPORT struct task_timer_data_access
        {
            static std::shared_ptr<util::external_timer::task_wrapper> get(
                task_timer_data const& timer_data) noexcept
            {
                return timer_data.data;
            }
        };

    }    // namespace detail

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT task_timer_data create_task_timer(
        threads::thread_description const& description,
        std::uint32_t parent_locality_id,
        threads::thread_id const& parent_task);

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void update_task_timer(
        task_timer_data& timer, char const* new_name);

    HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT scoped_task_timer
    {
        explicit scoped_task_timer(task_timer_data data) noexcept;
        ~scoped_task_timer();

        scoped_task_timer(scoped_task_timer const&) = delete;
        scoped_task_timer& operator=(scoped_task_timer const&) = delete;

        void stop() noexcept;
        void yield() noexcept;

        template <typename T, typename State>
        void handle_post_execution(T* thrdptr, State s) noexcept
        {
            if (s == State::terminated || s == State::deleted)
            {
                stop();
                thrdptr->set_timer_data({});
            }
            else
            {
                yield();
            }
        }

    private:
        bool stopped_;
        task_timer_data data_;
    };

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void tracing_init(char const* name,
        int argc, char** argv, std::uint32_t rank = 0, std::uint32_t size = 1);

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void tracing_finalize();

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void register_thread(char const* name);

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void create_counter(
        std::string const& full_name, std::string const& short_name) noexcept;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void sample_counter(
        std::string const& name, std::string const& short_name,
        double value) noexcept;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void send_parcel(std::uint64_t tag,
        std::uint64_t size, std::uint64_t target_locality_id) noexcept;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void recv_parcel(std::uint64_t tag,
        std::uint64_t size, std::uint64_t source_locality_id,
        std::uint64_t source_thread_id) noexcept;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void set_enable_parent_task_handler(
        enable_parent_task_handler_type f);

}    // namespace hpx::tracing
