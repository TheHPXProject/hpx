//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The definition of scheduler_base::idle_callback_with_rules, included only by
// the source files that instantiate it (scheduler_base.cpp for
// fixed_scheduling_rules, scheduler_base_tuned.cpp for tuned_scheduling_rules).

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/threading_base/scheduler_base.hpp>
#include <hpx/threading_base/scheduler_mode.hpp>
#include <hpx/threading_base/scheduling_policy.hpp>

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>

namespace hpx::threads::policies {

#if defined(HPX_HAVE_THREAD_MANAGER_IDLE_BACKOFF)
    namespace detail {

        // one round of the spin of idle_callback, without its look at the
        // queues
        HPX_FORCEINLINE void spin_round() noexcept
        {
            for (std::uint32_t i = 0;
                i != scheduling_policy::calibration_pauses_per_round; ++i)
            {
                HPX_SMT_PAUSE;
            }
        }

        // A round of another number of pauses, for a scheduler that tunes it
        // (hpx.idle_backoff_spin_round_pauses). The rate is measured with
        // rounds of spin_round, so the number of rounds is scaled by the
        // number of pauses (see scheduling_policy::idle_step_for). Out of
        // line, so that the spin with the default rounds stays as it is.
        HPX_NOINLINE inline void spin_round(std::uint32_t const pauses) noexcept
        {
            for (std::uint32_t i = 0; i != pauses; ++i)
            {
                HPX_SMT_PAUSE;
            }
        }

        // Spin for the rounds of step, or until there is work: the own
        // queues are looked at every round, the queues to steal from every
        // step.steal_check_rounds rounds. Inlined into idle_callback, as it
        // was while it was local to scheduler_base.cpp: a call here would be
        // made every time an idle worker starts to spin.
        template <typename Round, typename HasOwnWork, typename HasVisibleWork>
        HPX_FORCEINLINE void spin(idle_step const& step, Round&& round,
            HasOwnWork&& has_own_work, HasVisibleWork&& has_visible_work)
        {
            for (std::uint64_t i = 1; i <= step.spin_rounds; ++i)
            {
                round();
                if (has_own_work() ||
                    (i % step.steal_check_rounds == 0 && has_visible_work()))
                {
                    break;
                }
            }
        }
    }    // namespace detail
#endif

    template <typename Rules>
    bool scheduler_base::idle_callback_with_rules(
        [[maybe_unused]] std::size_t num_thread,
        [[maybe_unused]] Rules const& rules)
    {
#if defined(HPX_HAVE_THREAD_MANAGER_IDLE_BACKOFF)
        if (modes_[num_thread].data_.load(std::memory_order_relaxed) &
            policies::scheduler_mode::enable_idle_backoff)
        {
            HPX_TRACING_MARK_EVENT("idle_callback");
            // Put this thread to sleep for some time, additionally it gets
            // woken up on new work.

            auto& data = wait_count_data_[num_thread].data_;

            // going to sleep again ends a free pass
            data.free_pass = false;

            std::unique_lock<pu_mutex_type> l(data.wait_mtx, std::try_to_lock);
            if (!l)
            {
                return false;
            }

            // the backoff period and whether to spin or sleep through it
            // (see idle_step_for)
            std::uint32_t const count =
                data.wait_count.fetch_add(1, std::memory_order_relaxed);
            idle_step const step = [&] {
                if constexpr (Rules::tunes_idle)
                {
                    return rules.idle_step_for(
                        num_thread, count, spin_rounds_per_us_);
                }
                else
                {
                    return idle_step_for(num_thread, count);
                }
            }();

            // A spinning worker is awake: it is not announced in sleep_mask_,
            // and it leaves as soon as it sees work. do_some_work may still
            // reset its count and notify it, which is harmless.
            if (step.spin_rounds != 0)
            {
                l.unlock();
                spinning_workers_.data_.fetch_add(1, std::memory_order_relaxed);
                auto const has_own_work = [&] {
                    return get_queue_length(num_thread) != 0;
                };
                auto const has_work_to_steal = [&] {
                    return has_visible_work(num_thread);
                };
                if (step.pauses_per_round ==
                    scheduling_policy::calibration_pauses_per_round)
                {
                    detail::spin(
                        step, [] { detail::spin_round(); }, has_own_work,
                        has_work_to_steal);
                }
                else
                {
                    detail::spin(
                        step,
                        [pauses = step.pauses_per_round] {
                            detail::spin_round(pauses);
                        },
                        has_own_work, has_work_to_steal);
                }

                // No fence after leaving the spin. A seq_cst fence here,
                // paired with one in has_spinning_worker, would make sure
                // that either a worker that queues small work for itself sees
                // this one leave (and wakes up a sleeper) or this one sees the
                // work when it looks for work next. The fence on the other
                // side would run for each such thread, so both are gone (see
                // wake_one). Without them, the worker that queued the thread
                // may still count this one as spinning while this one misses
                // the thread. The thread is not lost: its caller runs it once
                // it gets to it, and this worker finds it in its next look
                // for work or after its next backoff period (at most
                // max_idle_backoff_time).
                spinning_workers_.data_.fetch_sub(1, std::memory_order_relaxed);
                return true;
            }

            // Announce, then check for work once more. There is no fence
            // between the two: it would only help together with a fence in
            // wake_one and wake_if_sleeping, on the path of every new nano and
            // small thread (see wake_one for why that one is gone). Without
            // them, the announcement may become visible only after the check
            // has read the queues, so a thread queued at the same time can be
            // missed by both sides: the worker that queued it does not see
            // this worker in the sleep mask, and this worker does not see the
            // thread. This worker then sleeps for the period it has computed
            // (at most max_idle_backoff_time) and finds the thread when it
            // wakes up, unless another worker has run it by then. The check
            // still finds work that was queued before this worker got here,
            // so the window is only as long as the announcement takes to
            // become visible. do_some_work accepts the same race.
            std::uint64_t const bit = std::uint64_t(1) << (num_thread % 64);
            auto& word = sleep_mask_[num_thread / 64].data_;
            word.fetch_or(bit, std::memory_order_relaxed);

            if (!has_visible_work(num_thread) &&
                data.wait_cond.wait_for(l, step.period) ==    //-V1089
                    std::cv_status::no_timeout)
            {
                // reset counter if thread was woken up
                data.wait_count.store(0, std::memory_order_relaxed);
                if constexpr (Rules::tunes_idle)
                {
                    data.free_pass = rules.free_pass_after_wake();
                }
                else
                {
                    data.free_pass = free_pass_after_wake();
                }
            }

            // withdraw the announcement if nobody claimed it
            word.fetch_and(~bit, std::memory_order_relaxed);
            return true;
        }
#endif
        return false;
    }
}    // namespace hpx::threads::policies
