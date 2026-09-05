//  Copyright (c) 2026 Hartmut Kaiser
//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include <hpx/modules/tracy.hpp>

#include <tracy/TracyC.h>

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

    // Inline connection-gate. When no Tracy client is attached, the per-task
    // hot-path entries below short-circuit at the call site to a single
    // atomic load.
    namespace detail {
        HPX_CXX_CORE_EXPORT inline bool is_profiler_connected() noexcept
        {
            return ___tracy_connected() != 0;
        }
    }    // namespace detail

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
    // RAII correctness: instances constructed while disconnected must not
    // emit on destruction, even if a client connects mid-lifetime. The
    // inner `hpx::tracy::*` impl captures the connected state at ctor and
    // honours it in its own dtor; the outer wrapper defers to that byte.
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
        // Private delegating ctor so `is_profiler_connected()` is queried
        // exactly once, then the snapshotted `active` flag is passed twice
        // into `impl`'s ctor without a second atomic load that could race
        // with a mid-lifetime client connect.
        explicit fiber_suspend_region(char const* desc, bool active) noexcept;

        hpx::tracy::fiber_suspend_region impl;
    };

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct HPX_CORE_EXPORT background_work_region
    {
        explicit background_work_region(std::size_t num_thread) noexcept;
        ~background_work_region();

        background_work_region(background_work_region const&) = delete;
        background_work_region& operator=(
            background_work_region const&) = delete;

    private:
        hpx::tracy::background_work_region impl;
    };

    ////////////////////////////////////////////////////////////////////////////
    // lock_context: TracyCLockAnnounce / TracyCLockTerminate in ctor/dtor
    // are safe whether Tracy is connected or not, so we gate only the four
    // take/release member functions. The ctor snapshots the connected state
    // so that a lock announced while disconnected keeps skipping its
    // per-cycle events even if a client connects mid-lifetime -- announce
    // and per-cycle events stay consistent.
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
        bool active_;
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
    /// \brief Set the OS thread name in the profiler timeline.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void set_thread_name(
        char const* name) noexcept;

    ////////////////////////////////////////////////////////////////////////////
    namespace detail {
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT char const* rename_region_impl(
            char const* name) noexcept;
    }    // namespace detail

    /// \brief Rename the currently active region on this thread; returns
    ///        the previous name, or the caller's input if no rename occurred.
    // Disconnected path returns the caller's input, mirroring the underlying
    // `hpx::tracy::detail::rename_region`'s in-fiber no-op. Returning nullptr
    // would risk a strlen(nullptr) if a caller later restored a saved name.
    HPX_CXX_CORE_EXPORT inline char const* rename_region(
        char const* name) noexcept
    {
        if (!detail::is_profiler_connected())
            return name;
        return detail::rename_region_impl(name);
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

    namespace detail {
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void task_staged_impl(
            char const* description,
            void const* parent_task_id = nullptr) noexcept;

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void task_created_impl(
            char const* description, void const* task_id,
            void const* parent_task_id = nullptr) noexcept;

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void task_executing_impl(
            void const* task_id, char const* description,
            std::size_t worker_thread) noexcept;

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void task_yielded_impl(
            void const* task_id, char const* description) noexcept;

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void task_suspended_impl(
            void const* task_id, char const* description,
            char const* reason = nullptr) noexcept;

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void task_resumed_impl(
            void const* task_id, char const* description,
            char const* wake_reason = nullptr) noexcept;

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void task_completed_impl(
            void const* task_id, char const* description) noexcept;

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void task_deleted_impl(
            void const* task_id) noexcept;
    }    // namespace detail

    // Inline gate wrappers. When the profiler is not connected, the whole
    // per-task snprintf/message/log-string chain is short-circuited at the
    // call site; only a single atomic load runs.

    /// \brief Task-lifecycle signal: task description registered before the
    ///        thread object exists.
    ///
    /// \param parent_task_id  Optional pointer to the spawning task, for
    ///                        parent-child correlation.
    HPX_CXX_CORE_EXPORT inline void task_staged(
        char const* description, void const* parent_task_id = nullptr) noexcept
    {
        if (!detail::is_profiler_connected())
            return;
        detail::task_staged_impl(description, parent_task_id);
    }

    /// \brief Task-lifecycle signal: thread object fully constructed.
    ///
    /// \param parent_task_id  Optional pointer to the spawning task, for
    ///                        parent-child correlation.
    HPX_CXX_CORE_EXPORT inline void task_created(char const* description,
        void const* task_id, void const* parent_task_id = nullptr) noexcept
    {
        if (!detail::is_profiler_connected())
            return;
        detail::task_created_impl(description, task_id, parent_task_id);
    }

    /// \brief Task-lifecycle signal: task begins executing on a worker thread.
    ///
    /// \param worker_thread  Index of the worker executing this task.
    HPX_CXX_CORE_EXPORT inline void task_executing(void const* task_id,
        char const* description, std::size_t worker_thread) noexcept
    {
        if (!detail::is_profiler_connected())
            return;
        detail::task_executing_impl(task_id, description, worker_thread);
    }

    /// \brief Task-lifecycle signal: task voluntarily yielded to the scheduler.
    HPX_CXX_CORE_EXPORT inline void task_yielded(
        void const* task_id, char const* description) noexcept
    {
        if (!detail::is_profiler_connected())
            return;
        detail::task_yielded_impl(task_id, description);
    }

    /// \brief Task-lifecycle signal: task blocked on an external resource.
    ///
    /// \param reason  Optional description of what the task is waiting on.
    HPX_CXX_CORE_EXPORT inline void task_suspended(void const* task_id,
        char const* description, char const* reason = nullptr) noexcept
    {
        if (!detail::is_profiler_connected())
            return;
        detail::task_suspended_impl(task_id, description, reason);
    }

    /// \brief Task-lifecycle signal: task unblocked and returned to a
    ///        pending state.
    ///
    /// \param wake_reason  Optional description of what unblocked the task.
    HPX_CXX_CORE_EXPORT inline void task_resumed(void const* task_id,
        char const* description, char const* wake_reason = nullptr) noexcept
    {
        if (!detail::is_profiler_connected())
            return;
        detail::task_resumed_impl(task_id, description, wake_reason);
    }

    /// \brief Task-lifecycle signal: task finished its execution loop.
    HPX_CXX_CORE_EXPORT inline void task_completed(
        void const* task_id, char const* description) noexcept
    {
        if (!detail::is_profiler_connected())
            return;
        detail::task_completed_impl(task_id, description);
    }

    /// \brief Task-lifecycle signal: task identity removed from scheduler maps.
    HPX_CXX_CORE_EXPORT inline void task_deleted(void const* task_id) noexcept
    {
        if (!detail::is_profiler_connected())
            return;
        detail::task_deleted_impl(task_id);
    }

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

    namespace detail {
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void future_fulfilled_impl(
            void const* future_id, char const* desc = nullptr) noexcept;

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void future_exception_set_impl(
            void const* future_id, char const* desc = nullptr) noexcept;

        // Active-continuations counter. Updated unconditionally by the
        // continuation_run / continuation_finished inline wrappers below,
        // even when no profiler is attached, so the tally stays consistent
        // across connect/disconnect transitions. Only the Tracy emission
        // (message + sample_value + fiber text) is gated. If both sides
        // were gated instead, a run that started disconnected and finished
        // connected would decrement without a matching increment (and the
        // reverse would leak a permanent positive drift).
        //
        // The plot value is loaded inside each _emit body rather than
        // snapshotted at the fetch_add/fetch_sub call site, so that Tracy
        // records the counter's value at emit time rather than at update
        // time. This avoids the reordering artefact where two concurrent
        // continuations could publish stale snapshots to the plot.
        HPX_CORE_EXPORT extern std::atomic<std::int64_t> g_active_continuations;

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void continuation_run_emit(
            void const* task_id) noexcept;

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void
        continuation_finished_emit() noexcept;

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void handle_on_completed_fired_impl(
            void const* task_id = nullptr) noexcept;

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void work_stolen_impl(
            std::size_t thief_id, std::size_t victim_id, void const* task_id,
            char const* desc = nullptr) noexcept;

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void frame_mark_impl(
            char const* name = nullptr) noexcept;

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void os_thread_sleep_impl(
            std::size_t num_thread) noexcept;

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void send_parcel_impl(
            std::uint64_t tag_msb, std::uint64_t tag_lsb, std::uint64_t size,
            std::uint64_t target_locality_id) noexcept;

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void recv_parcel_impl(
            std::uint64_t tag_msb, std::uint64_t tag_lsb,
            std::uint64_t source_locality_id) noexcept;

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void parcel_scheduled_impl(
            std::uint64_t tag_msb, std::uint64_t tag_lsb,
            std::uint64_t source_locality_id,
            std::uint64_t source_thread_id) noexcept;
    }    // namespace detail

    /// \brief Producer-side signal: a future shared state was set to a value.
    ///
    /// \param future_id  Pointer to the `future_data` shared state. Used
    ///                   as a stable identifier to correlate producer and
    ///                   consumer events in the Tracy message log.
    /// \param desc       Optional description of the producing context
    ///                   (e.g. the name of the promise or task).
    HPX_CXX_CORE_EXPORT inline void future_fulfilled(
        void const* future_id, char const* desc = nullptr) noexcept
    {
        if (!detail::is_profiler_connected())
            return;
        detail::future_fulfilled_impl(future_id, desc);
    }

    /// \brief Producer-side signal: a future shared state was set with an
    ///        exception.
    ///
    /// \param future_id  Pointer to the `future_data` shared state.
    /// \param desc       Optional description of the producing context.
    HPX_CXX_CORE_EXPORT inline void future_exception_set(
        void const* future_id, char const* desc = nullptr) noexcept
    {
        if (!detail::is_profiler_connected())
            return;
        detail::future_exception_set_impl(future_id, desc);
    }

    /// \brief Consumer-side signal: a continuation has started
    ///        executing, correlated with the source future state.
    HPX_CXX_CORE_EXPORT inline void continuation_run(
        void const* task_id = nullptr) noexcept
    {
        // Counter update runs unconditionally so the tally stays consistent
        // across connect/disconnect transitions -- see g_active_continuations.
        detail::g_active_continuations.fetch_add(1, std::memory_order_relaxed);
        if (!detail::is_profiler_connected())
            return;
        detail::continuation_run_emit(task_id);
    }

    /// \brief Consumer-side signal: a continuation has finished
    ///        executing.
    HPX_CXX_CORE_EXPORT inline void continuation_finished(
        void const* /* task_id */ = nullptr) noexcept
    {
        detail::g_active_continuations.fetch_sub(1, std::memory_order_relaxed);
        if (!detail::is_profiler_connected())
            return;
        detail::continuation_finished_emit();
    }

    /// \brief Consumer-side signal: handle_on_completed fired,
    ///        dispatching registered continuations.
    HPX_CXX_CORE_EXPORT inline void handle_on_completed_fired(
        void const* task_id = nullptr) noexcept
    {
        if (!detail::is_profiler_connected())
            return;
        detail::handle_on_completed_fired_impl(task_id);
    }

    /// \brief Signal emitted when a worker thread steals a task from
    ///        another worker.
    HPX_CXX_CORE_EXPORT inline void work_stolen(std::size_t thief_id,
        std::size_t victim_id, void const* task_id,
        char const* desc = nullptr) noexcept
    {
        if (!detail::is_profiler_connected())
            return;
        detail::work_stolen_impl(thief_id, victim_id, task_id, desc);
    }

    /// \brief Emit a frame/stage boundary marker in Tracy timeline.
    ///
    /// \param name  Optional name for the frame/stage.
    HPX_CXX_CORE_EXPORT inline void frame_mark(
        char const* name = nullptr) noexcept
    {
        if (!detail::is_profiler_connected())
            return;
        detail::frame_mark_impl(name);
    }

    /// \brief Point message emitted just before an OS worker thread suspends
    ///        on its condition variable.
    ///
    /// \param num_thread  Index of the worker thread entering sleep.
    HPX_CXX_CORE_EXPORT inline void os_thread_sleep(
        std::size_t num_thread) noexcept
    {
        if (!detail::is_profiler_connected())
            return;
        detail::os_thread_sleep_impl(num_thread);
    }

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

    /// \brief Parcel-send signal: a parcel has been handed to the
    ///        parcelport for transmission.
    ///
    /// \param tag_msb              High 64 bits of the parcel id.
    /// \param tag_lsb              Low 64 bits of the parcel id. Together
    ///                             with tag_msb they form the full 128-bit
    ///                             id used to pair events across localities;
    ///                             lsb alone restarts per locality and
    ///                             would produce false matches.
    /// \param size                 Wire size of the parcel in bytes.
    /// \param target_locality_id   Destination locality id, or
    ///                             `naming::invalid_locality_id` if unknown.
    HPX_CXX_CORE_EXPORT inline void send_parcel(std::uint64_t tag_msb,
        std::uint64_t tag_lsb, std::uint64_t size,
        std::uint64_t target_locality_id) noexcept
    {
        if (!detail::is_profiler_connected())
            return;
        detail::send_parcel_impl(tag_msb, tag_lsb, size, target_locality_id);
    }

    /// \brief Parcel-receive signal: a parcel has arrived and its header
    ///        has been deserialized.
    ///
    /// \param tag_msb              High 64 bits of the parcel id.
    /// \param tag_lsb              Low 64 bits of the parcel id.
    /// \param source_locality_id   Sending locality id, or
    ///                             `naming::invalid_locality_id` when the
    ///                             parcel has no source id yet (pre-AGAS
    ///                             handshake parcels).
    HPX_CXX_CORE_EXPORT inline void recv_parcel(std::uint64_t tag_msb,
        std::uint64_t tag_lsb, std::uint64_t source_locality_id) noexcept
    {
        if (!detail::is_profiler_connected())
            return;
        detail::recv_parcel_impl(tag_msb, tag_lsb, source_locality_id);
    }

    /// \brief Parcel-scheduled signal: an action carried by a parcel has
    ///        been placed onto the local scheduler. Fires for deferred
    ///        receives (zero-copy or multi-parcel message), for
    ///        AGAS-routed parcels, and for parcels the AGAS re-route
    ///        chain delivered after a migration detection. Does not fire
    ///        for the non-deferred path where load_schedule schedules
    ///        inline.
    ///
    /// \param tag_msb              High 64 bits of the parcel id.
    /// \param tag_lsb              Low 64 bits of the parcel id.
    /// \param source_locality_id   Sending locality id (see recv_parcel).
    /// \param source_thread_id     Parent HPX thread id captured on the
    ///                             sender; may be 0 when unavailable.
    HPX_CXX_CORE_EXPORT inline void parcel_scheduled(std::uint64_t tag_msb,
        std::uint64_t tag_lsb, std::uint64_t source_locality_id,
        std::uint64_t source_thread_id) noexcept
    {
        if (!detail::is_profiler_connected())
            return;
        detail::parcel_scheduled_impl(
            tag_msb, tag_lsb, source_locality_id, source_thread_id);
    }

    HPX_CXX_CORE_EXPORT constexpr void set_enable_parent_task_handler(
        enable_parent_task_handler_type) noexcept
    {
    }

}    // namespace hpx::tracing

#include <hpx/config/warnings_suffix.hpp>
