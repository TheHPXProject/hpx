//  Copyright (c) 2023-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/thread_pools/detail/background_thread.hpp>
#include <hpx/thread_pools/detail/scheduling_callbacks.hpp>
#include <hpx/thread_pools/detail/scoped_background_timer.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>

namespace hpx::threads::detail {

    ///////////////////////////////////////////////////////////////////////////
    thread_id_ref_type create_background_thread(
        threads::policies::scheduler_base& scheduler_base,
        std::size_t const num_thread, scheduling_callbacks const& callbacks,
        std::shared_ptr<bool>& background_running,
        std::int64_t& idle_loop_count)
    {
        threads::thread_schedule_hint const schedulehint(
            static_cast<std::int16_t>(num_thread));

        thread_id_ref_type background_thread;
        background_running = std::make_shared<bool>(true);

        thread_init_data background_init(
            [&, background_running](
                thread_restart_state) -> thread_result_type {
                while (*background_running)
                {
                    if (callbacks.background_())
                    {
                        // we only update the idle_loop_count if
                        // background_running is true. If it was false, this
                        // task was given back to the scheduler.
                        if (*background_running)
                        {
                            idle_loop_count = 0;
                        }
                    }

                    // Force yield...
                    hpx::execution_base::this_thread::yield("background_work");
                }

                return {thread_schedule_state::terminated, invalid_thread_id};
            },
            hpx::threads::thread_description("background_work"),
            thread_priority::high_recursive, schedulehint,
            thread_stacksize::large,
            // Create in suspended to prevent the thread from being scheduled
            // directly...
            thread_schedule_state::suspended, true, &scheduler_base);

        scheduler_base.create_thread(
            background_init, &background_thread, hpx::throws);
        HPX_ASSERT(background_thread);

        auto* thread_ptr = get_thread_id_data(background_thread);
        thread_ptr->set_is_background();

        scheduler_base.increment_background_thread_count();

        LTM_(debug).format("create_background_thread: pool({}), "
                           "scheduler({}), worker_thread({}), thread({})",
            scheduler_base.get_parent_pool(), scheduler_base, num_thread,
            thread_ptr);

        // We can now set the state to pending
        [[maybe_unused]] auto old_state =
            thread_ptr->set_state(thread_schedule_state::pending);
        return background_thread;
    }

    ///////////////////////////////////////////////////////////////////////////
    class switch_status_background
    {
    public:
        switch_status_background(
            thread_id_ref_type const& t, thread_state const prev_state) noexcept
          : thread_(get_thread_id_data(t))
          , prev_state_(prev_state)
          , next_thread_id_(nullptr)
          , need_restore_state_(
                thread_->set_state_tagged(thread_schedule_state::active,
                    prev_state_, orig_state_, std::memory_order_relaxed))
        {
        }

        switch_status_background(switch_status_background const&) = delete;
        switch_status_background(switch_status_background&&) = delete;

        switch_status_background& operator=(
            switch_status_background const&) = delete;
        switch_status_background& operator=(
            switch_status_background&&) = delete;

        ~switch_status_background()
        {
            if (need_restore_state_)
            {
                store_state(prev_state_);
            }
        }

        [[nodiscard]] constexpr bool is_valid() const noexcept
        {
            return need_restore_state_;
        }

        // allow to change the state the thread will be switched to after
        // execution
        switch_status_background& operator=(
            thread_result_type&& new_state) noexcept
        {
            prev_state_ = thread_state(
                new_state.first, prev_state_.state_ex(), prev_state_.tag() + 1);
            if (new_state.second != nullptr)
            {
                next_thread_id_ = HPX_MOVE(new_state.second);
            }
            return *this;
        }

        // Get the state this thread was in before execution (usually pending),
        // this helps to make sure no other worker-thread is started to execute
        // this HPX-thread in the meantime.
        [[nodiscard]] thread_schedule_state get_previous() const noexcept
        {
            return prev_state_.state();
        }

        // This restores the previous state, while making sure that the original
        // state has not been changed since we started executing this thread.
        // The function returns true if the state has been set, false otherwise.
        bool store_state(thread_state& newstate) noexcept
        {
            disable_restore();
            if (thread_->restore_state(prev_state_, orig_state_))
            {
                newstate = prev_state_;
                return true;
            }
            return false;
        }

        // disable default handling in destructor
        void disable_restore() noexcept
        {
            need_restore_state_ = false;
        }

        [[nodiscard]] constexpr thread_id_ref_type const& get_next_thread()
            const noexcept
        {
            return next_thread_id_;
        }

        thread_id_ref_type move_next_thread() noexcept
        {
            return HPX_MOVE(next_thread_id_);
        }

    private:
        thread_data* thread_;
        thread_state prev_state_;
        thread_state orig_state_;
        thread_id_ref_type next_thread_id_;
        bool need_restore_state_;
    };

    // This function tries to invoke the background work thread. It returns
    // false when we need to give the background thread back to scheduler and
    // create a new one that is supposed to be executed inside the
    // scheduling_loop, true otherwise
    bool call_background_thread(thread_id_ref_type& background_thread,
        thread_id_ref_type& next_thrd,
        threads::policies::scheduler_base& scheduler_base,
        std::size_t const num_thread,
        [[maybe_unused]] background_work_exec_time& exec_time,
        hpx::execution_base::this_thread::detail::agent_storage*
            context_storage)
    {
        LTM_(debug).format("call_background_thread: pool({}), "
                           "scheduler({}), worker_thread({}), thread({})",
            scheduler_base.get_parent_pool(), scheduler_base, num_thread,
            get_thread_id_data(background_thread));

        if (HPX_LIKELY(background_thread))
        {
            auto* thrdptr = get_thread_id_data(background_thread);
            thread_state state = thrdptr->get_state();
            thread_schedule_state state_val = state.state();

            // we should only deal with pending here.
            HPX_ASSERT(thread_schedule_state::pending == state_val);

            // tries to set state to active (only if state is still
            // the same as 'state')
            detail::switch_status_background thrd_stat(
                background_thread, state);

            if (HPX_LIKELY(thrd_stat.is_valid() &&
                    thrd_stat.get_previous() == thread_schedule_state::pending))
            {
#if defined(HPX_HAVE_BACKGROUND_THREAD_COUNTERS) &&                            \
    defined(HPX_HAVE_THREAD_IDLE_RATES)
                // measure background work duration
                background_work_duration_counter bg_work_duration(
                    exec_time.timer);
                background_exec_time_wrapper bg_exec_time(bg_work_duration);
#endif    // HPX_HAVE_BACKGROUND_THREAD_COUNTERS

                // invoke background thread
                thrd_stat = (*thrdptr)(context_storage);

                if (thread_id_ref_type next = thrd_stat.move_next_thread();
                    next && next != background_thread)
                {
                    if (!next_thrd)
                    {
                        next_thrd = HPX_MOVE(next);
                    }
                    else
                    {
                        auto* scheduler =
                            get_thread_id_data(next)->get_scheduler_base();
                        scheduler->schedule_thread(HPX_MOVE(next),
                            threads::thread_schedule_hint(
                                static_cast<std::int16_t>(num_thread)),
                            true);
                        scheduler->do_some_work(num_thread);
                    }
                }
            }
            thrd_stat.store_state(state);
            state_val = state.state();

            if (HPX_LIKELY(state_val == thread_schedule_state::pending_boost))
            {
                [[maybe_unused]] auto old_state =
                    thrdptr->set_state(thread_schedule_state::pending);
            }
            else if (thread_schedule_state::terminated == state_val)
            {
                LTM_(debug).format(
                    "call_background_thread terminated: pool({}), "
                    "scheduler({}), worker_thread({}), thread({})",
                    scheduler_base.get_parent_pool(), scheduler_base,
                    num_thread, get_thread_id_data(background_thread));

                scheduler_base.decrement_background_thread_count();
                background_thread = thread_id_type();
            }
            else if (thread_schedule_state::suspended == state_val)
            {
                LTM_(debug).format(
                    "call_background_thread suspended: pool({}), "
                    "scheduler({}), worker_thread({}), thread({})",
                    scheduler_base.get_parent_pool(), scheduler_base,
                    num_thread, get_thread_id_data(background_thread));

                return false;
            }
        }
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    bool call_and_create_background_thread(
        thread_id_ref_type& background_thread, thread_id_ref_type& next_thrd,
        threads::policies::scheduler_base& scheduler_base,
        std::size_t const num_thread, background_work_exec_time& exec_time,
        hpx::execution_base::this_thread::detail::agent_storage*
            context_storage,
        scheduling_callbacks const& callbacks, std::shared_ptr<bool>& running,
        std::int64_t& idle_loop_count)
    {
        if (!call_background_thread(background_thread, next_thrd,
                scheduler_base, num_thread, exec_time, context_storage))
        {
            // Let the current background thread terminate as soon as possible.
            // No need to reschedule, as another thread will set it to pending
            // and schedule it back eventually
            HPX_ASSERT(background_thread);
            HPX_ASSERT(running);

            *running = false;
            scheduler_base.decrement_background_thread_count();

            // Create a new one that will replace the current such we avoid
            // deadlock situations, if all background threads are blocked.
            background_thread = create_background_thread(scheduler_base,
                num_thread, callbacks, running, idle_loop_count);

            return true;
        }
        return false;
    }
}    // namespace hpx::threads::detail
