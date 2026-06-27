//  Copyright (c) 2019-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/futures/detail/execute_thread.hpp>
#include <hpx/futures/future.hpp>
#include <hpx/futures/futures_factory.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/tracing.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>

namespace hpx::threads::detail {

#if !defined(HPX_HAVE_THREADS_GET_STACK_POINTER)
    ///////////////////////////////////////////////////////////////////////////
    // reuse the continuation recursion count here as well
    struct execute_thread_recursion_count
    {
        execute_thread_recursion_count() noexcept
          : count_(threads::get_continuation_recursion_count())
        {
            ++count_;
        }
        ~execute_thread_recursion_count() noexcept
        {
            --count_;
        }

        std::size_t& count_;
    };
#endif

    // make sure thread invocation does not recurse deeper than allowed
    HPX_FORCEINLINE coroutine_type::result_type handle_execute_thread(
        thread_id_type const& thrd)
    {
        auto* thrdptr = get_thread_id_data(thrd);

        // We need to run the completion on a new thread
        HPX_ASSERT(nullptr != hpx::threads::get_self_ptr());

#if defined(HPX_HAVE_THREADS_GET_STACK_POINTER)
        bool recurse_asynchronously =
            !this_thread::has_sufficient_stack_space();
#else
        execute_thread_recursion_count cnt;
        bool recurse_asynchronously =
            cnt.count_ > HPX_CONTINUATION_MAX_RECURSION_DEPTH;
#endif
        if (!recurse_asynchronously)
        {
            // directly execute continuation on this thread
            return thrdptr->invoke_directly();
        }

        LTM_(error).format(
            "handle_execute_thread: couldn't directly execute thread({}), "
            "description({})",
            thrdptr, thrdptr->get_description());

        return {thread_schedule_state::pending, invalid_thread_id};
    }

    bool execute_thread(thread_id_ref_type thrd)
    {
        auto* thrdptr = get_thread_id_data(thrd);
        thread_state state = thrdptr->get_state();
        thread_schedule_state state_val = state.state();

        // the given thread can be executed inline if its state is 'pending'
        // (i.e. not running and not finished running)
        if (state_val != thread_schedule_state::pending)
        {
            return false;
        }

        bool reschedule = false;

        // don't directly run any threads that have started running 'normally'
        // and were suspended afterward
        if (thrdptr->runs_as_child())
        {
            LTM_(error).format(
                "execute_thread: attempting to directly execute thread({}), "
                "description({}), runs_as_child({})",
                thrdptr, thrdptr->get_description(),
                thrdptr->runs_as_child(std::memory_order_relaxed));

            // tries to set state to active (only if state is still the same as
            // 'state')
            switch_status thrd_stat(thrd, state);
            if (HPX_UNLIKELY(!thrd_stat.is_valid()))
            {
                // state change failed
                LTM_(error).format(
                    "execute_thread: couldn't directly execute "
                    "thread({}), description({}), state change failed",
                    thrdptr, thrdptr->get_description());

                // switch_status will not reset thread state
                return false;
            }

            HPX_ASSERT(
                thrdptr->get_state().state() == thread_schedule_state::active);

            if (HPX_UNLIKELY(
                    thrd_stat.get_previous() != thread_schedule_state::pending))
            {
                // thread was not pending
                LTM_(error).format(
                    "execute_thread: couldn't directly execute "
                    "thread({}), description({}), thread not pending",
                    thrdptr, thrdptr->get_description());

                // switch_status will reset state to what it was before
                return false;
            }

            // check again, making sure the state has not changed in the
            // meantime
            if (thrdptr->runs_as_child())
            {
                // get the tracing data, in case we are resuming the thread
                // and have to restore any leaf timers from direct actions, etc.
                hpx::tracing::scoped_task_timer profiler(
                    thrdptr->get_timer_data());

                thrd_stat = handle_execute_thread(thrd.noref());

                profiler.handle_post_execution(
                    thrdptr, thrd_stat.get_previous());
            }
            else
            {
                // reschedule thread as it could have been dropped on the floor
                // by the scheduler while the status was set to active here
                reschedule = true;
            }

            // store and retrieve the new state in the thread
            if (HPX_LIKELY(thrd_stat.store_state(state)))
            {
                // direct execution doesn't support specifying the next
                // thread to execute
                HPX_ASSERT(thrd_stat.get_next_thread() == nullptr);

                state_val = state.state();
                if (state_val == thread_schedule_state::pending)
                {
                    // explicitly reschedule thread as it was not executed
                    // directly
                    reschedule = true;
                }
            }

            LTM_(error).format("execute_thread: directly executed thread({}), "
                               "description({}), returned state({})",
                thrdptr, thrdptr->get_description(), state_val);

            // any exception thrown from the thread will reset its state at this
            // point
        }

        if (reschedule)
        {
            LTM_(error).format(
                "execute_thread: rescheduling thread after failing to directly "
                "execute thread({}), description({})",
                thrdptr, thrdptr->get_description());

            set_thread_state(thrd.noref(), thread_schedule_state::pending,
                thread_restart_state::signaled);
            auto* scheduler = thrdptr->get_scheduler_base();

            auto const hint = thread_schedule_hint(static_cast<std::int16_t>(
                thrdptr->get_last_worker_thread_num()));
            scheduler->schedule_thread_last(HPX_MOVE(thrd), hint);
            scheduler->do_some_work(
                hint.mode == hpx::threads::thread_schedule_hint_mode::numa ?
                    static_cast<std::size_t>(-1) :
                    hint.hint);
        }

        HPX_ASSERT(state_val != thread_schedule_state::terminated);
        return state_val == thread_schedule_state::deleted;
    }
}    // namespace hpx::threads::detail
