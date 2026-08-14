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

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void task_staged(
        char const* description, void const* parent_task_id = nullptr) noexcept;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void task_created(
        char const* description, void const* task_id,
        void const* parent_task_id = nullptr) noexcept;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void task_executing(void const* task_id,
        char const* description, std::size_t worker_thread) noexcept;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void task_yielded(
        void const* task_id, char const* description) noexcept;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void task_suspended(void const* task_id,
        char const* description, char const* reason = nullptr) noexcept;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void task_resumed(void const* task_id,
        char const* description, char const* wake_reason = nullptr) noexcept;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void task_completed(
        void const* task_id, char const* description) noexcept;

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void task_deleted(
        void const* task_id) noexcept;

    ////////////////////////////////////////////////////////////////////////////
    // Causal tracing: future fulfillment signals.
    //
    // These functions emit a producer-side Tracy message at the exact point a
    // future shared state transitions to ready. The `future_id` parameter is
    // the address of the `future_data` shared state and acts as a stable
    // correlation key: a consumer thread's subsequent `task_resumed` event
    // (emitted by thread_helpers.cpp) can be visually linked to the preceding
    // `future_fulfilled` / `future_exception_set` message in the Tracy log.
    //
    // Placement contract: both functions must be called after the atomic state
    // CAS succeeds (so `future_id` is already observable as ready by any
    // newly-woken consumer) and before `cond_.notify_one()` fires (so the
    // producer message precedes the consumer wake-up in wall-clock order).

    /// \brief Producer-side signal: a future shared state was set to a value.
    ///
    /// \param future_id  Pointer to the `future_data` shared state. Used as a
    ///                   stable identifier to correlate producer and consumer
    ///                   events in the Tracy message log.
    /// \param desc       Optional description of the producing context
    ///                   (e.g. the name of the promise or task).
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void future_fulfilled(
        void const* future_id, char const* desc = nullptr) noexcept;

    /// \brief Producer-side signal: a future shared state was set with an
    ///        exception.
    ///
    /// \param future_id  Pointer to the `future_data` shared state.
    /// \param desc       Optional description of the producing context.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void future_exception_set(
        void const* future_id, char const* desc = nullptr) noexcept;

    /// \brief Consumer-side signal: a continuation has started executing,
    ///        correlated with the source future state.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void continuation_run(
        void const* task_id = nullptr) noexcept;

    /// \brief Consumer-side signal: a continuation has finished executing.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void continuation_finished(
        void const* task_id = nullptr) noexcept;

    /// \brief Consumer-side signal: handle_on_completed fired, dispatching
    ///        registered continuations.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void handle_on_completed_fired(
        void const* task_id = nullptr) noexcept;

    /// \brief Emit a frame/stage boundary marker in Tracy timeline.
    ///
    /// \param name  Optional name for the frame/stage.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void frame_mark(
        char const* name = nullptr) noexcept;

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
