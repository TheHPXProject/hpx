//  Copyright (c) 2017-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/execution/detail/sync_launch_policy_dispatch.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/threading_base.hpp>

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace hpx::detail {

    ////////////////////////////////////////////////////////////////////////////
    // Run tid, a thread created as pending_do_not_schedule by a fork, next.
    // The work has been handed off at this point, so a failure to yield is
    // not reported to the caller; tid gets scheduled instead.
    inline void run_forked_thread(threads::thread_id_ref_type const& tid,
        threads::thread_description const& desc)
    {
        if (threads::thread_id_type const tid_self = threads::get_self_id())
        {
            try
            {
                // yield_to(tid), suspend dispatches tid to its own scheduler
                // if that differs from ours
                hpx::this_thread::suspend(
                    threads::thread_schedule_state::pending, tid.noref(), desc);
                return;
            }
            catch (hpx::thread_interrupted const&)
            {
                // re-arm, the interruption fires at the next interruption
                // point instead; ignore if interrupts were disabled
                // meanwhile (interrupt() throws thread_not_interruptable)
                try
                {
                    get_thread_id_data(tid_self)->interrupt();
                }
                catch (hpx::exception const& e)
                {
                    // thread_not_interruptable: interruption was disabled
                    // on this thread in the meantime
                    (void) e;
                }
            }
            catch (hpx::exception const& e)
            {
                // yield_aborted
                (void) e;
            }
        }

        // Not on an HPX thread, or suspend failed (possibly after yielding).
        // A duplicate queue entry is harmless: the scheduling loop only runs
        // a thread that is still pending. Preserve the registered priority
        // and worker placement, and wake an idling worker: set_thread_state
        // would do both, going through schedule_thread directly does not.
        auto* const thrd_data = get_thread_id_data(tid);
        auto* const scheduler = thrd_data->get_scheduler_base();

        threads::thread_schedule_hint const schedulehint(
            threads::thread_schedule_hint_mode::thread,
            static_cast<std::int16_t>(thrd_data->get_last_worker_thread_num()));
        scheduler->schedule_thread(
            tid, schedulehint, false, thrd_data->get_priority());
        scheduler->do_some_work(static_cast<std::size_t>(-1));
    }

    ////////////////////////////////////////////////////////////////////////////
    // forward declaration
    HPX_CXX_CORE_EXPORT template <typename Policy>
    struct post_policy_dispatch;

    template <>
    struct post_policy_dispatch<launch::async_policy>
    {
        template <typename Policy, typename F, typename... Ts>
        static void call(Policy policy,
            hpx::threads::thread_description const& desc,
            threads::thread_pool_base* pool, F&& f, Ts&&... ts)
        {
            // run_as_child doesn't make sense if we _post_ a task
            auto hint = policy.hint();
            hint.runs_as_child_mode(hpx::threads::thread_execution_hint::none);

            threads::thread_init_data data(
                threads::make_thread_function_nullary(
                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...),
                desc, policy.priority(), hint, policy.stacksize(),
                threads::thread_schedule_state::pending);

            threads::register_work(data, pool);
        }

        template <typename Policy, typename F, typename... Ts>
        static void call(Policy&& policy,
            hpx::threads::thread_description const& desc, F&& f, Ts&&... ts)
        {
            call(HPX_FORWARD(Policy, policy), desc,
                threads::detail::get_self_or_default_pool(), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }
    };

    template <>
    struct post_policy_dispatch<launch::fork_policy>
    {
        template <typename Policy, typename F, typename... Ts>
        static void call(Policy policy,
            hpx::threads::thread_description const& desc,
            threads::thread_pool_base* pool, F&& f, Ts&&... ts)
        {
            auto hint = policy.hint();
            threads::thread_init_data data(
                threads::make_thread_function_nullary(
                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...),
                desc, policy.priority(),
                threads::thread_schedule_hint(
                    threads::thread_schedule_hint_mode::thread,
                    static_cast<std::int16_t>(get_worker_thread_num()),
                    hint.placement_mode(),
                    hpx::threads::thread_execution_hint::none,
                    hint.sharing_mode()),
                policy.stacksize(),
                threads::thread_schedule_state::pending_do_not_schedule, true);

            threads::thread_id_ref_type const tid =
                threads::register_thread(data, pool);

            // make sure this thread is executed last
            if (tid)
            {
                run_forked_thread(tid, "post_policy_dispatch(suspend)");
            }
        }

        template <typename Policy, typename F, typename... Ts>
        static void call(Policy&& policy,
            hpx::threads::thread_description const& desc, F&& f, Ts&&... ts)
        {
            call(HPX_FORWARD(Policy, policy), desc,
                threads::detail::get_self_or_default_pool(), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }
    };

    template <>
    struct post_policy_dispatch<launch::sync_policy>
    {
        template <typename Policy, typename F, typename... Ts>
        static void call(Policy&& policy,
            hpx::threads::thread_description const&, threads::thread_pool_base*,
            F&& f, Ts&&... ts)
        {
            hpx::detail::sync_launch_policy_dispatch<launch::sync_policy>::call(
                HPX_FORWARD(Policy, policy), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }

        template <typename Policy, typename F, typename... Ts>
        static void call(Policy&& policy,
            hpx::threads::thread_description const&, F&& f, Ts&&... ts)
        {
            hpx::detail::sync_launch_policy_dispatch<launch::sync_policy>::call(
                HPX_FORWARD(Policy, policy), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }
    };

    template <>
    struct post_policy_dispatch<launch::deferred_policy>
    {
        template <typename Policy, typename F, typename... Ts>
        static void call(Policy&& policy,
            hpx::threads::thread_description const&, threads::thread_pool_base*,
            F&& f, Ts&&... ts)
        {
            hpx::detail::sync_launch_policy_dispatch<
                launch::deferred_policy>::call(HPX_FORWARD(Policy, policy),
                HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename Policy, typename F, typename... Ts>
        static void call(Policy&& policy,
            hpx::threads::thread_description const&, F&& f, Ts&&... ts)
        {
            hpx::detail::sync_launch_policy_dispatch<
                launch::deferred_policy>::call(HPX_FORWARD(Policy, policy),
                HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }
    };

    HPX_CXX_CORE_EXPORT template <typename Policy>
    struct post_policy_dispatch
    {
        template <typename F, typename... Ts>
        static void call(Policy policy,
            hpx::threads::thread_description const& desc,
            threads::thread_pool_base* pool, F&& f, Ts&&... ts)
        {
            HPX_ASSERT(pool != nullptr);

            if (policy == launch::async)
            {
                post_policy_dispatch<launch::async_policy>::call(
                    HPX_MOVE(policy), desc, pool, HPX_FORWARD(F, f),
                    HPX_FORWARD(Ts, ts)...);
            }
            else if (policy == launch::sync)
            {
                post_policy_dispatch<launch::sync_policy>::call(
                    HPX_MOVE(policy), desc, pool, HPX_FORWARD(F, f),
                    HPX_FORWARD(Ts, ts)...);
            }
            else if (policy == launch::deferred)
            {
                post_policy_dispatch<launch::deferred_policy>::call(
                    HPX_MOVE(policy), desc, pool, HPX_FORWARD(F, f),
                    HPX_FORWARD(Ts, ts)...);
            }
            else if (policy == launch::fork)
            {
                post_policy_dispatch<launch::fork_policy>::call(
                    HPX_MOVE(policy), desc, pool, HPX_FORWARD(F, f),
                    HPX_FORWARD(Ts, ts)...);
            }
            else
            {
                post_policy_dispatch<launch::async_policy>::call(
                    HPX_MOVE(policy), desc, pool, HPX_FORWARD(F, f),
                    HPX_FORWARD(Ts, ts)...);
            }
        }

        template <typename F, typename... Ts>
        static void call(Policy&& policy,
            hpx::threads::thread_description const& desc, F&& f, Ts&&... ts)
        {
            call(HPX_FORWARD(Policy, policy), desc,
                threads::detail::get_self_or_default_pool(), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }
    };
}    // namespace hpx::detail
