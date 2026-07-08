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

#include <hpx/modules/itt_notify.hpp>

namespace hpx::tracing {

    HPX_CXX_CORE_EXPORT using enable_parent_task_handler_type = bool (*)();

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT using annotation_handle = util::itt::string_handle;

    HPX_CXX_CORE_EXPORT inline annotation_handle create_annotation_handle(
        char const* name) noexcept
    {
        return util::itt::string_handle(name);
    }

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct region_init_data
    {
        char const* name = nullptr;
        std::size_t thread_phase = 0;
        void const* thread_ptr = nullptr;
        bool is_stackless = false;
        std::size_t address = 0;
        bool is_address_type = false;
        annotation_handle handle;
    };

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT loop_context
    {
        explicit loop_context() noexcept;
        ~loop_context();

        loop_context(loop_context const&) = delete;
        loop_context& operator=(loop_context const&) = delete;

        util::itt::stack_context stack_ctx;
        util::itt::thread_domain thread_domain;
        util::itt::string_handle task_id;
        util::itt::string_handle task_phase;
    };

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT region
    {
        explicit region(
            loop_context& ctx, region_init_data const& data, std::size_t);
        ~region();

        region(region const&) = delete;
        region& operator=(region const&) = delete;

    private:
        static util::itt::task make_task(
            loop_context& ctx, region_init_data const& data);

        util::itt::caller_context cctx;
        util::itt::task task;
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
        explicit lock_context(
            char const* name = nullptr, void const* addr = nullptr) noexcept
          : addr_(addr)
        {
            if (addr_)
                HPX_ITT_SYNC_CREATE(
                    const_cast<void*>(addr_), name ? name : "sync", "");
        }
        explicit lock_context(char const* prefix, char const* suffix,
            void const* addr = nullptr) noexcept
          : addr_(addr)
        {
            if (addr_)
                HPX_ITT_SYNC_CREATE(const_cast<void*>(addr_),
                    prefix ? prefix : "sync", suffix ? suffix : "");
        }

        ~lock_context()
        {
            if (addr_)
                HPX_ITT_SYNC_DESTROY(const_cast<void*>(addr_));
        }

        bool before_lock() const noexcept
        {
            if (addr_)
                HPX_ITT_SYNC_PREPARE(const_cast<void*>(addr_));
            return addr_ != nullptr;
        }

        void after_lock() const noexcept
        {
            if (addr_)
                HPX_ITT_SYNC_ACQUIRED(const_cast<void*>(addr_));
        }

        void after_try_lock(bool success) const noexcept
        {
            if (addr_)
            {
                if (success)
                    HPX_ITT_SYNC_ACQUIRED(const_cast<void*>(addr_));
                else
                    HPX_ITT_SYNC_CANCEL(const_cast<void*>(addr_));
            }
        }

        void before_unlock() const noexcept
        {
            if (addr_)
                HPX_ITT_SYNC_RELEASING(const_cast<void*>(addr_));
        }

        void after_unlock() const noexcept
        {
            if (addr_)
                HPX_ITT_SYNC_RELEASED(const_cast<void*>(addr_));
        }

    private:
        void const* addr_ = nullptr;
    };

    ////////////////////////////////////////////////////////////////////////////
    namespace detail {
        HPX_CXX_CORE_EXPORT inline void sync_prepare(void const* addr) noexcept
        {
            if (addr)
                HPX_ITT_SYNC_PREPARE(const_cast<void*>(addr));
        }
        HPX_CXX_CORE_EXPORT inline void sync_acquired(void const* addr) noexcept
        {
            if (addr)
                HPX_ITT_SYNC_ACQUIRED(const_cast<void*>(addr));
        }
        HPX_CXX_CORE_EXPORT inline void sync_cancel(void const* addr) noexcept
        {
            if (addr)
                HPX_ITT_SYNC_CANCEL(const_cast<void*>(addr));
        }
        HPX_CXX_CORE_EXPORT inline void sync_releasing(
            void const* addr) noexcept
        {
            if (addr)
                HPX_ITT_SYNC_RELEASING(const_cast<void*>(addr));
        }
        HPX_CXX_CORE_EXPORT inline void sync_released(void const* addr) noexcept
        {
            if (addr)
                HPX_ITT_SYNC_RELEASED(const_cast<void*>(addr));
        }
    }    // namespace detail

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT inline void set_thread_name(char const* name) noexcept
    {
        HPX_ITT_THREAD_SET_NAME(name);
    }

    ////////////////////////////////////////////////////////////////////////////
    // ITT has no rename_region equivalent
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
