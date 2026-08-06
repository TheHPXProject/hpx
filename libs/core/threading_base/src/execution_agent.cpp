//  Copyright (c) 2019 Thomas Heller
//  Copyright (c) 2020-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/lock_registration.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/threading_base/execution_agent.hpp>
#include <hpx/threading_base/scheduler_base.hpp>
#include <hpx/threading_base/set_thread_state.hpp>
#include <hpx/threading_base/thread_data.hpp>
#include <hpx/threading_base/thread_description.hpp>
#include <hpx/threading_base/thread_helpers.hpp>
#include <hpx/threading_base/thread_num_tss.hpp>

#ifdef HPX_HAVE_THREAD_DESCRIPTION
#include <hpx/threading_base/detail/reset_lco_description.hpp>
#endif
#ifdef HPX_HAVE_THREAD_BACKTRACE_ON_SUSPENSION
#include <hpx/modules/debugging.hpp>
#include <hpx/threading_base/detail/reset_backtrace.hpp>
#endif
#ifdef HPX_HAVE_MODULE_LIKWID
#include <hpx/modules/likwid.hpp>
#endif

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace {

    // Below this remaining duration, the multi-hop suspend()/at_timer/
    // wake_timer machinery (two extra HPX threads plus an ASIO timer
    // round-trip) costs more than just spinning/yielding a few times and
    // rechecking cond(). Tune independently of yield_k's iteration thresholds
    // since this is a wall-clock cutoff, not a counter.
    constexpr std::chrono::microseconds sleep_until_spin_threshold{10};
}    // namespace

namespace hpx::threads {

    execution_agent::execution_agent(
        coroutines::detail::coroutine_impl* coroutine) noexcept
      : self_(coroutine)
    {
    }

    std::string execution_agent::description() const
    {
        thread_id_type const id = self_.get_thread_id();
        if (HPX_UNLIKELY(!id))
        {
            HPX_THROW_EXCEPTION(hpx::error::null_thread_id,
                "execution_agent::description",
                "null thread id encountered (is this executed on a "
                "HPX-thread?)");
        }

        return hpx::util::format(
            "{}: {}", id, get_thread_id_data(id)->get_description());
    }

    void execution_agent::yield(char const* desc)
    {
        do_yield(desc, hpx::threads::thread_schedule_state::pending);
    }

    bool execution_agent::yield_k(std::size_t const k, char const* desc)
    {
        if (k < 4)    //-V112
        {
            return false;
        }
#if defined(HPX_SMT_PAUSE)
        else if (k < 16)
        {
            HPX_SMT_PAUSE;
            return false;
        }
#endif
        else if (k < 32 || k & 1)    //-V112
        {
            do_yield(desc, hpx::threads::thread_schedule_state::pending_boost);
            return true;
        }
        else
        {
            do_yield(desc, hpx::threads::thread_schedule_state::pending);
            return true;
        }
    }

    void execution_agent::resume(
        hpx::threads::thread_priority const priority, char const* desc)
    {
        do_resume(priority, desc, threads::thread_restart_state::signaled);
    }

    void execution_agent::abort(char const* desc)
    {
        do_resume(hpx::threads::thread_priority::default_, desc,
            threads::thread_restart_state::abort);
    }

    void execution_agent::suspend(char const* desc)
    {
        do_yield(desc, threads::thread_schedule_state::suspended);
    }

    threads::thread_restart_state execution_agent::sleep_for(
        hpx::chrono::steady_duration const& sleep_duration,
        hpx::move_only_function<bool()>&& wait_cond, char const* desc)
    {
        return sleep_until(
            sleep_duration.from_now(), HPX_MOVE(wait_cond), desc);
    }

    threads::thread_restart_state execution_agent::sleep_until(
        hpx::chrono::steady_time_point const& sleep_time,
        hpx::move_only_function<bool()>&& wait_cond, char const* desc)
    {
        auto const cond = HPX_MOVE(wait_cond);

        // fast path: already satisfied, no need to suspend or spin at all
        if (cond && cond())
        {
            return threads::thread_restart_state::signaled;
        }

        std::size_t k = 0;
        for (;;)
        {
            auto const now = std::chrono::steady_clock::now();
            if (now >= sleep_time.value())
            {
                if (cond && cond())
                {
                    return threads::thread_restart_state::signaled;
                }
                return threads::thread_restart_state::timeout;
            }

            if (sleep_time.value() - now < sleep_until_spin_threshold)
            {
                // sub-threshold tail: spin/yield via the existing backoff
                // ladder instead of arming a scheduler timer for a remainder
                // too short to make the round trip worthwhile
                yield_k(k++, desc);

                if (cond && cond())
                {
                    return threads::thread_restart_state::signaled;
                }
                continue;
            }

            hpx::error_code ec(hpx::throwmode::lightweight);
            threads::thread_restart_state const statex =
                hpx::this_thread::suspend(sleep_time, desc, ec);
            if (ec)
            {
                return threads::thread_restart_state::abort;
            }

            if (cond && cond())
            {
                return threads::thread_restart_state::signaled;
            }

            if (statex == threads::thread_restart_state::timeout)
            {
                return threads::thread_restart_state::timeout;
            }

            // woken early (e.g. condition_variable::notify_one's direct
            // ctx.resume() -> set_thread_state), condition still false,
            // deadline not yet reached - loop; next iteration re-evaluates
            // remaining time and picks suspend() or the spin tail again
        }
    }

    hpx::threads::thread_restart_state execution_agent::do_yield(
        char const* desc, threads::thread_schedule_state state)
    {
        thread_id_type id = self_.get_outer_thread_id();
        if (HPX_UNLIKELY(!id))
        {
            HPX_THROW_EXCEPTION(hpx::error::null_thread_id,
                "execution_agent::do_yield",
                "null thread id encountered (is this executed on a "
                "HPX-thread?)");
        }

        // handle interruption, if needed
        thread_data* thrd_data = get_thread_id_data(id);
        if (HPX_UNLIKELY(thrd_data == nullptr))
        {
            HPX_THROW_EXCEPTION(hpx::error::null_thread_id,
                "execution_agent::do_yield",
                "null thread id encountered (is this executed on a "
                "HPX-thread?)");
        }

        thrd_data->interruption_point();

        // keep the thread alive if it's not a background thread (background
        // threads are being kept alive by the scheduler)
        hpx::threads::keep_alive_thread_id kept_alive(
            id, !thrd_data->is_background());

        if (thrd_data->get_priority() == thread_priority::bound)
        {
            auto const num_thread = hpx::get_local_worker_thread_num();
            thrd_data->set_last_worker_thread_num(
                static_cast<std::uint16_t>(num_thread));
        }

        threads::thread_restart_state statex;

        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            [[maybe_unused]] threads::detail::reset_lco_description reset_desc(
                id, threads::thread_description(desc));
#endif
#if defined(HPX_HAVE_THREAD_BACKTRACE_ON_SUSPENSION)
            [[maybe_unused]] threads::detail::reset_backtrace reset_bt(id);
#endif
#if defined(HPX_HAVE_VERIFY_LOCKS)
            [[maybe_unused]] auto held_locks = hpx::experimental::scope_exit(
                [data = hpx::util::get_held_locks_data()]() mutable {
                    hpx::util::set_held_locks_data(HPX_MOVE(data));
                });
#endif
#ifdef HPX_HAVE_MODULE_LIKWID
            hpx::likwid::suspend_region region;
#endif
            // fiber_suspend_region operates only on the fiber zone stack
            // (current_fiber_zone). It does NOT touch the OS-thread zone
            // (current_region / stop_region), so there is no zone-stack
            // conflict. The sequence is:
            //   constructor: close running zone -> open grey "suspended" zone
            //   self_.yield(): fiber parks, scheduler picks next task
            //   destructor:  close "suspended" zone -> reopen running zone
            hpx::tracing::fiber_suspend_region tracy_suspend(desc);

            HPX_ASSERT(thrd_data != nullptr &&
                thrd_data->get_state().state() ==
                    thread_schedule_state::active);
            HPX_ASSERT(state != thread_schedule_state::active);

            if (state == thread_schedule_state::pending ||
                state == thread_schedule_state::pending_boost)
            {
                hpx::tracing::task_yielded(id);
            }
            else if (state == thread_schedule_state::suspended)
            {
                hpx::tracing::task_suspended(id, desc);
            }

            // actual yield operation
            statex = self_.yield(
                threads::thread_result_type(state, threads::invalid_thread_id));

            HPX_ASSERT(thrd_data != nullptr &&
                thrd_data->get_state().state() ==
                    thread_schedule_state::active);

            if (state == thread_schedule_state::suspended)
            {
                hpx::tracing::task_resumed(id, statex);
            }
        }

        // handle interruption, if needed
        thrd_data->interruption_point();

        // handle interrupt and abort
        if (statex == threads::thread_restart_state::abort)
        {
            HPX_THROW_EXCEPTION(hpx::error::yield_aborted, desc,
                "thread({}) aborted (yield returned wait_abort)",
                description());
        }

        return statex;
    }

    void execution_agent::do_resume(
        hpx::threads::thread_priority const priority, char const* /* desc */,
        hpx::threads::thread_restart_state const statex) const
    {
        threads::detail::set_thread_state(self_.get_thread_id(),
            thread_schedule_state::pending, statex, priority,
            thread_schedule_hint{}, false);
    }
}    // namespace hpx::threads
