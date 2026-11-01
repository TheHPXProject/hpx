//  Copyright (c) 2007-2024 Hartmut Kaiser
//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/threading_base/create_thread.hpp>
#include <hpx/threading_base/detail/get_default_timer_service.hpp>
#include <hpx/threading_base/set_thread_state_timed.hpp>
#include <hpx/threading_base/threading_base_fwd.hpp>
#ifdef HPX_HAVE_MODULE_LIKWID
#include <hpx/modules/likwid.hpp>
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <winsock2.h>
#endif
#include <asio/basic_waitable_timer.hpp>
#include <asio/error.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <system_error>
#include <utility>

namespace hpx::threads::detail {

    // Who is responsible for resuming the wake_timer thread: the timer handler
    // or, if the timer was aborted first, the at_timer thread itself.
    enum class wake_state : std::uint8_t
    {
        unclaimed,
        claimed,
        scheduled
    };

    ///////////////////////////////////////////////////////////////////////////
    /// State shared between an at_timer thread, the wake_timer thread it
    /// creates, and the timer handler that resumes the wake_timer thread.
    struct timer_state
    {
        std::atomic<bool> triggered{false};
        std::atomic<wake_state> wake{wake_state::unclaimed};
    };

    ///////////////////////////////////////////////////////////////////////////
    /// This thread function is used by the at_timer thread below to trigger
    /// the required action.
    thread_result_type wake_timer_thread(thread_id_ref_type const& thrd,
        thread_schedule_state /*newstate*/,
        thread_restart_state /*newstate_ex*/, thread_priority /*priority*/,
        thread_id_type timer_id, std::shared_ptr<timer_state> const& state,
        bool retry_on_active, thread_restart_state my_statex)
    {
        if (HPX_UNLIKELY(!thrd))
        {
            HPX_THROW_EXCEPTION(hpx::error::null_thread_id,
                "threads::detail::wake_timer_thread",
                "null thread id encountered (id)");
        }

        if (HPX_UNLIKELY(!timer_id))
        {
            HPX_THROW_EXCEPTION(hpx::error::null_thread_id,
                "threads::detail::wake_timer_thread",
                "null thread id encountered (timer_id)");
        }

        HPX_ASSERT(my_statex == thread_restart_state::abort ||
            my_statex == thread_restart_state::timeout);

        if (!state->triggered.load())
        {
            error_code ec(throwmode::lightweight);    // do not throw
            set_thread_state(timer_id, thread_schedule_state::pending,
                my_statex, thread_priority::boost, thread_schedule_hint(),
                retry_on_active, ec);
        }

        return {thread_schedule_state::terminated, invalid_thread_id};
    }

    // This thread function initiates the required set_state action (on behalf
    // of one of the threads#detail#set_thread_state functions).
    thread_result_type at_timer(policies::scheduler_base* scheduler,
        std::chrono::steady_clock::time_point const& abs_time,
        thread_id_ref_type const& thrd, thread_schedule_state newstate,
        thread_restart_state newstate_ex, thread_priority priority,
        std::atomic<bool>* started, bool retry_on_active)
    {
        if (HPX_UNLIKELY(!thrd))
        {
            HPX_THROW_EXCEPTION(hpx::error::null_thread_id,
                "threads::detail::at_timer", "null thread id encountered");
        }

        // create a new thread in suspended state, which will execute the
        // requested set_state when timer fires and will re-awaken this thread,
        // allowing the deadline_timer to go out of scope gracefully
        thread_id_ref_type const self_id = get_self_id();    // keep alive

        std::shared_ptr<timer_state> state(std::make_shared<timer_state>());

        thread_init_data data(
            hpx::bind_front(&wake_timer_thread, thrd, newstate, newstate_ex,
                priority, self_id.noref(), state, retry_on_active),
            "wake_timer", priority, thread_schedule_hint(),
            thread_stacksize::small_, thread_schedule_state::suspended, true);

        // at_timer holds its own reference to the wake_timer thread: the
        // handler's copy below may already be destroyed when the abort path
        // resumes it.
        thread_id_ref_type wake_id = invalid_thread_id;
        create_thread(scheduler, data, wake_id);

        // create timer firing in correspondence with given time
        using deadline_timer =
            ::asio::basic_waitable_timer<std::chrono::steady_clock>;

        deadline_timer t(get_default_timer_service(), abs_time);

        // let the timer invoke the set_state on the new (suspended) thread
        t.async_wait([wake_id, state, priority, retry_on_active](
                         std::error_code const& ec) {
            // Cancellation is handled by at_timer, which makes wake_timer
            // pending itself before canceling the timer below.
            if (ec == ::asio::error::operation_aborted)
            {
                return;
            }

            wake_state expected = wake_state::unclaimed;
            if (!state->wake.compare_exchange_strong(
                    expected, wake_state::claimed))
            {
                return;
            }

            // Release at_timer even if scheduling below throws; wake_timer is
            // then left suspended, but at_timer does not wait for it forever.
            auto on_exit = hpx::experimental::scope_exit(
                [&state] { state->wake.store(wake_state::scheduled); });

            error_code ec2(throwmode::lightweight);    // do not throw
            set_thread_state(wake_id.noref(), thread_schedule_state::pending,
                thread_restart_state::timeout, priority, thread_schedule_hint(),
                retry_on_active, ec2);
        });

        if (started != nullptr)
        {
            started->store(true);
        }

        // this waits for the thread to be reactivated when the timer fired
        // if it returns signaled the timer has been canceled, otherwise
        // the timer fired and the wake_timer_thread above has been executed
        thread_restart_state statex;

        {
#ifdef HPX_HAVE_MODULE_LIKWID
            hpx::likwid::suspend_region region;
#endif
            hpx::tracing::fiber_suspend_region tracy_suspend("timer_wait");

            // Mirror the suspend/resume message pair from
            // execution_agent.cpp and thread_helpers.cpp so the profiler
            // sees the timer wait as a task suspension, not just a fiber
            // zone. The yield below always uses state=suspended, so no
            // task_yielded branch is needed here.
            hpx::tracing::task_suspended(self_id.noref(), "timer_wait");

            statex = get_self().yield(thread_result_type(
                thread_schedule_state::suspended, invalid_thread_id));

            hpx::tracing::task_resumed(self_id.noref(), statex);
        }

        HPX_ASSERT(statex == thread_restart_state::abort ||
            statex == thread_restart_state::timeout);

        if (thread_restart_state::timeout != statex)    //-V601
        {
            state->triggered.store(true);

            // Resume the wake_timer thread from here instead of leaving it to
            // the timer's cancel handler, which during shutdown may run after
            // all worker threads have exited. If the handler claimed it first,
            // wait until it has actually made the wake_timer thread pending.
            wake_state expected = wake_state::unclaimed;
            if (state->wake.compare_exchange_strong(
                    expected, wake_state::claimed))
            {
                error_code ec(throwmode::lightweight);    // do not throw
                set_thread_state(wake_id.noref(),
                    thread_schedule_state::pending, thread_restart_state::abort,
                    priority, thread_schedule_hint(), retry_on_active, ec);
                state->wake.store(wake_state::scheduled);
            }
            else
            {
                hpx::util::yield_while(
                    [&] { return state->wake.load() != wake_state::scheduled; },
                    "at_timer");
            }

            // wake_timer is pending by now, so the cancel handler is a no-op
            t.cancel();
        }
        else
        {
            detail::set_thread_state(
                thrd.noref(), newstate, newstate_ex, priority);
        }

        return {thread_schedule_state::terminated, invalid_thread_id};
    }

    // Set a timer to set the state of the given \a thread to the given new
    // value after it expired (at the given time)
    thread_id_ref_type set_thread_state_timed(
        policies::scheduler_base* scheduler,
        hpx::chrono::steady_time_point const& abs_time,
        thread_id_type const& thrd, thread_schedule_state newstate,
        thread_restart_state newstate_ex, thread_priority priority,
        thread_schedule_hint schedulehint, std::atomic<bool>* started,
        bool retry_on_active, error_code& ec)
    {
        if (HPX_UNLIKELY(!thrd))
        {
            HPX_THROWS_IF(ec, hpx::error::null_thread_id,
                "threads::detail::set_thread_state",
                "null thread id encountered");
            return invalid_thread_id;
        }

        // this creates a new thread that creates the timer and handles the
        // requested actions
        thread_init_data data(
            hpx::bind(&at_timer, scheduler, abs_time.value(),
                thread_id_ref_type(thrd), newstate, newstate_ex, priority,
                started, retry_on_active),
            "at_timer (expire at)", priority, schedulehint,
            thread_stacksize::small_, thread_schedule_state::pending, true);

        thread_id_ref_type newid = invalid_thread_id;
        create_thread(scheduler, data, newid, ec);    //-V601
        return newid;
    }
}    // namespace hpx::threads::detail
