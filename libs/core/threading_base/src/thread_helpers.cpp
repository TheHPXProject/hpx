//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/assert.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/threading_base/scheduler_base.hpp>
#include <hpx/threading_base/scheduler_state.hpp>
#include <hpx/threading_base/set_thread_state.hpp>
#include <hpx/threading_base/set_thread_state_timed.hpp>
#include <hpx/threading_base/thread_data.hpp>
#include <hpx/threading_base/thread_description.hpp>
#include <hpx/threading_base/thread_helpers.hpp>
#include <hpx/threading_base/thread_pool_base.hpp>

#ifdef HPX_HAVE_VERIFY_LOCKS
#include <hpx/modules/lock_registration.hpp>
#endif
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

#include <atomic>
#include <cstddef>
#include <limits>
#include <memory>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::threads {

    ///////////////////////////////////////////////////////////////////////////
    thread_state set_thread_state(thread_id_type const& id,
        thread_schedule_state state, thread_restart_state stateex,
        thread_priority priority, bool retry_on_active, error_code& ec)
    {
        if (&ec != &throws)
            ec = make_success_code();

        return detail::set_thread_state(id, state, stateex, priority,
            thread_schedule_hint(), retry_on_active, ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    thread_id_ref_type set_thread_state(thread_id_type const& id,
        hpx::chrono::steady_time_point const& abs_time,
        std::atomic<bool>* timer_started, thread_schedule_state state,
        thread_restart_state stateex, thread_priority priority,
        bool retry_on_active, error_code& ec)
    {
        return detail::set_thread_state_timed(
            get_thread_id_data(id)->get_scheduler_base(), abs_time, id, state,
            stateex, priority, thread_schedule_hint(), timer_started,
            retry_on_active, ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    thread_state get_thread_state(
        thread_id_type const& id, error_code& /* ec */) noexcept
    {
        return id ? get_thread_id_data(id)->get_state() :
                    thread_state(thread_schedule_state::terminated,
                        thread_restart_state::unknown);
    }

    ///////////////////////////////////////////////////////////////////////////
    std::size_t get_thread_phase(
        thread_id_type const& id, error_code& /* ec */) noexcept
    {
        return id ? get_thread_id_data(id)->get_thread_phase() :
                    static_cast<std::size_t>(~0);
    }

    ///////////////////////////////////////////////////////////////////////////
    threads::thread_priority get_thread_priority(
        thread_id_type const& id, error_code& /* ec */) noexcept
    {
        return id ? get_thread_id_data(id)->get_priority() :
                    thread_priority::unknown;
    }

    std::ptrdiff_t get_stack_size(
        thread_id_type const& id, error_code& /* ec */) noexcept
    {
        return id ? get_thread_id_data(id)->get_stack_size() :
                    static_cast<std::ptrdiff_t>(thread_stacksize::unknown);
    }

    void interrupt_thread(thread_id_type const& id, bool flag, error_code& ec)
    {
        // Copy id as the original `id` may get reset by the caller
        // asynchronously.
        auto const keep_alive = id;
        if (HPX_UNLIKELY(!id))
        {
            HPX_THROWS_IF(ec, hpx::error::null_thread_id,
                "hpx::threads::interrupt_thread", "null thread id encountered");
            return;
        }

        if (&ec != &throws)
            ec = make_success_code();

        get_thread_id_data(id)->interrupt(flag);    // notify thread

        // Set thread state to pending. If the thread is currently active we do
        // not retry. The thread will either exit or hit an interruption_point.
        set_thread_state(keep_alive, thread_schedule_state::pending,
            thread_restart_state::abort, thread_priority::normal, false, ec);
    }

    void interruption_point(thread_id_type const& id, error_code& ec)
    {
        if (HPX_UNLIKELY(!id))
        {
            HPX_THROWS_IF(ec, hpx::error::null_thread_id,
                "hpx::threads::interruption_point",
                "null thread id encountered");
            return;
        }

        if (&ec != &throws)
            ec = make_success_code();

        get_thread_id_data(id)->interruption_point();    // notify thread
    }

    ///////////////////////////////////////////////////////////////////////////
    bool get_thread_interruption_enabled(
        thread_id_type const& id, error_code& ec)
    {
        if (HPX_UNLIKELY(!id))
        {
            HPX_THROW_EXCEPTION(hpx::error::null_thread_id,
                "hpx::threads::get_thread_interruption_enabled",
                "null thread id encountered");
        }

        if (&ec != &throws)
            ec = make_success_code();

        return get_thread_id_data(id)->interruption_enabled();
    }

    bool set_thread_interruption_enabled(
        thread_id_type const& id, bool enable, error_code& ec)
    {
        if (HPX_UNLIKELY(!id))
        {
            HPX_THROW_EXCEPTION(hpx::error::null_thread_id,
                "hpx::threads::get_thread_interruption_enabled",
                "null thread id encountered");
        }

        if (&ec != &throws)
            ec = make_success_code();

        return get_thread_id_data(id)->set_interruption_enabled(enable);
    }

    bool get_thread_interruption_requested(
        thread_id_type const& id, error_code& ec)
    {
        if (HPX_UNLIKELY(!id))
        {
            HPX_THROWS_IF(ec, hpx::error::null_thread_id,
                "hpx::threads::get_thread_interruption_requested",
                "null thread id encountered");
            return false;
        }

        if (&ec != &throws)
            ec = make_success_code();

        return get_thread_id_data(id)->interruption_requested();
    }

    ///////////////////////////////////////////////////////////////////////////
    std::size_t get_thread_data(thread_id_type const& id, error_code& ec)
    {
        if (HPX_UNLIKELY(!id))
        {
            HPX_THROWS_IF(ec, hpx::error::null_thread_id,
                "hpx::threads::get_thread_data", "null thread id encountered");
            return 0;
        }

        return get_thread_id_data(id)->get_thread_data();
    }

    std::size_t set_thread_data(
        thread_id_type const& id, std::size_t data, error_code& ec)
    {
        if (HPX_UNLIKELY(!id))
        {
            HPX_THROWS_IF(ec, hpx::error::null_thread_id,
                "hpx::threads::set_thread_data", "null thread id encountered");
            return 0;
        }

        return get_thread_id_data(id)->set_thread_data(data);
    }

#if defined(HPX_HAVE_LIBCDS)
    std::size_t get_libcds_data(thread_id_type const& id, error_code& ec)
    {
        if (HPX_UNLIKELY(!id))
        {
            HPX_THROWS_IF(ec, hpx::error::null_thread_id,
                "hpx::threads::get_libcds_data", "null thread id encountered");
            return 0;
        }

        return get_thread_id_data(id)->get_libcds_data();
    }

    std::size_t set_libcds_data(
        thread_id_type const& id, std::size_t data, error_code& ec)
    {
        if (HPX_UNLIKELY(!id))
        {
            HPX_THROWS_IF(ec, hpx::error::null_thread_id,
                "hpx::threads::set_libcds_data", "null thread id encountered");
            return 0;
        }

        return get_thread_id_data(id)->set_libcds_data(data);
    }

    std::size_t get_libcds_hazard_pointer_data(
        thread_id_type const& id, error_code& ec)
    {
        if (HPX_UNLIKELY(!id))
        {
            HPX_THROWS_IF(ec, hpx::error::null_thread_id,
                "hpx::threads::get_libcds_hazard_pointer_data",
                "null thread id encountered");
            return 0;
        }

        return get_thread_id_data(id)->get_libcds_hazard_pointer_data();
    }

    std::size_t set_libcds_hazard_pointer_data(
        thread_id_type const& id, std::size_t data, error_code& ec)
    {
        if (HPX_UNLIKELY(!id))
        {
            HPX_THROWS_IF(ec, hpx::error::null_thread_id,
                "hpx::threads::set_libcds_hazard_pointer_data",
                "null thread id encountered");
            return 0;
        }

        return get_thread_id_data(id)->set_libcds_hazard_pointer_data(data);
    }

    std::size_t get_libcds_dynamic_hazard_pointer_data(
        thread_id_type const& id, error_code& ec)
    {
        if (HPX_UNLIKELY(!id))
        {
            HPX_THROWS_IF(ec, hpx::error::null_thread_id,
                "hpx::threads::get_libcds_dynamic_hazard_pointer_data",
                "null thread id encountered");
            return 0;
        }

        return get_thread_id_data(id)->get_libcds_dynamic_hazard_pointer_data();
    }

    std::size_t set_libcds_dynamic_hazard_pointer_data(
        thread_id_type const& id, std::size_t data, error_code& ec)
    {
        if (HPX_UNLIKELY(!id))
        {
            HPX_THROWS_IF(ec, hpx::error::null_thread_id,
                "hpx::threads::set_libcds_dynamic_hazard_pointer_data",
                "null thread id encountered");
            return 0;
        }

        return get_thread_id_data(id)->set_libcds_dynamic_hazard_pointer_data(
            data);
    }
#endif

    ////////////////////////////////////////////////////////////////////////////
    namespace {

        thread_local std::size_t continuation_recursion_count(0);
    }

    std::size_t& get_continuation_recursion_count() noexcept
    {
        if (thread_self* self_ptr = get_self_ptr())
        {
            return self_ptr->get_continuation_recursion_count();
        }
        return continuation_recursion_count;
    }

    void reset_continuation_recursion_count() noexcept
    {
        continuation_recursion_count = 0;
    }

    ///////////////////////////////////////////////////////////////////////////
    void run_thread_exit_callbacks(thread_id_type const& id, error_code& ec)
    {
        if (HPX_UNLIKELY(!id))
        {
            HPX_THROWS_IF(ec, hpx::error::null_thread_id,
                "hpx::threads::run_thread_exit_callbacks",
                "null thread id encountered");
            return;
        }

        if (&ec != &throws)
            ec = make_success_code();

        get_thread_id_data(id)->run_thread_exit_callbacks();
    }

    bool add_thread_exit_callback(thread_id_type const& id,
        hpx::function<void()> const& f, error_code& ec)
    {
        if (HPX_UNLIKELY(!id))
        {
            HPX_THROWS_IF(ec, hpx::error::null_thread_id,
                "hpx::threads::add_thread_exit_callback",
                "null thread id encountered");
            return false;
        }

        if (&ec != &throws)
            ec = make_success_code();

        return get_thread_id_data(id)->add_thread_exit_callback(f);
    }

    void free_thread_exit_callbacks(thread_id_type const& id, error_code& ec)
    {
        if (HPX_UNLIKELY(!id))
        {
            HPX_THROWS_IF(ec, hpx::error::null_thread_id,
                "hpx::threads::add_thread_exit_callback",
                "null thread id encountered");
            return;
        }

        if (&ec != &throws)
            ec = make_success_code();

        get_thread_id_data(id)->free_thread_exit_callbacks();
    }

    ///////////////////////////////////////////////////////////////////////////
#ifdef HPX_HAVE_THREAD_FULLBACKTRACE_ON_SUSPENSION
    char const* get_thread_backtrace(thread_id_type const& id, error_code& ec)
#else
    util::backtrace const* get_thread_backtrace(
        thread_id_type const& id, error_code& ec)
#endif
    {
        if (HPX_UNLIKELY(!id))
        {
            HPX_THROWS_IF(ec, hpx::error::null_thread_id,
                "hpx::threads::get_thread_backtrace",
                "null thread id encountered");
            return nullptr;
        }

        if (&ec != &throws)
            ec = make_success_code();

        return get_thread_id_data(id)->get_backtrace();
    }

#ifdef HPX_HAVE_THREAD_FULLBACKTRACE_ON_SUSPENSION
    char const* set_thread_backtrace(
        thread_id_type const& id, char const* bt, error_code& ec)
#else
    util::backtrace const* set_thread_backtrace(
        thread_id_type const& id, util::backtrace const* bt, error_code& ec)
#endif
    {
        if (HPX_UNLIKELY(!id))
        {
            HPX_THROWS_IF(ec, hpx::error::null_thread_id,
                "hpx::threads::set_thread_backtrace",
                "null thread id encountered");
            return nullptr;
        }

        if (&ec != &throws)
            ec = make_success_code();

        return get_thread_id_data(id)->set_backtrace(bt);
    }

    threads::thread_pool_base* get_pool(
        thread_id_type const& id, error_code& ec)
    {
        if (HPX_UNLIKELY(!id))
        {
            HPX_THROWS_IF(ec, hpx::error::null_thread_id,
                "hpx::threads::get_pool", "null thread id encountered");
            return nullptr;
        }

        if (&ec != &throws)
            ec = make_success_code();

        return get_thread_id_data(id)->get_scheduler_base()->get_parent_pool();
    }
}    // namespace hpx::threads

namespace hpx::this_thread {

    namespace {
        char const* get_suspend_reason(
            threads::thread_description const& description) noexcept
        {
            return threads::thread_data::get_safe_description(
                description, "this_thread::suspend");
        }
    }    // namespace

    // The function 'suspend' will return control to the thread manager
    // (suspends the current thread). It sets the new state of this thread to
    // the thread state passed as the parameter.
    //
    // If the suspension was aborted, this function will throw a
    // \a yield_aborted exception.
    threads::thread_restart_state suspend(threads::thread_schedule_state state,
        threads::thread_id_type nextid,
        [[maybe_unused]] threads::thread_description const& description,
        error_code& ec)
    {
        // let the thread manager do other things while waiting
        threads::thread_self& self = threads::get_self();

        // keep alive
        threads::thread_id_ref_type id = self.get_outer_thread_id();

        // handle interruption, if needed
        threads::interruption_point(id.noref(), ec);
        if (ec)
            return threads::thread_restart_state::unknown;

        threads::thread_restart_state statex;

        {
#ifdef HPX_HAVE_VERIFY_LOCKS
            // verify that there are no more registered locks for this OS-thread
            util::verify_no_locks();

            [[maybe_unused]] auto held_locks = hpx::experimental::scope_exit(
                [data = hpx::util::get_held_locks_data()]() mutable {
                    hpx::util::set_held_locks_data(HPX_MOVE(data));
                });
#endif
#ifdef HPX_HAVE_THREAD_DESCRIPTION
            threads::detail::reset_lco_description desc(
                id.noref(), description, ec);
#endif
#ifdef HPX_HAVE_THREAD_BACKTRACE_ON_SUSPENSION
            threads::detail::reset_backtrace bt(id, ec);
#endif
#ifdef HPX_HAVE_MODULE_LIKWID
            hpx::likwid::suspend_region region;
#endif
            hpx::tracing::fiber_suspend_region tracy_suspend(
                get_suspend_reason(description));
            // We might need to dispatch 'nextid' to it's correct scheduler only
            // if our current scheduler is the same, we should yield to the id
            if (nextid &&
                get_thread_id_data(nextid)->get_scheduler_base() !=
                    get_thread_id_data(id)->get_scheduler_base())
            {
                auto* scheduler =
                    get_thread_id_data(nextid)->get_scheduler_base();
                scheduler->schedule_thread(
                    HPX_MOVE(nextid), threads::thread_schedule_hint());

                statex = self.yield(threads::thread_result_type(
                    state, threads::invalid_thread_id));
            }
            else
            {
                statex = self.yield(
                    threads::thread_result_type(state, HPX_MOVE(nextid)));
            }
        }

        // handle interruption, if needed
        threads::interruption_point(id.noref(), ec);
        if (ec)
            return threads::thread_restart_state::unknown;

        // handle interrupt and abort
        if (statex == threads::thread_restart_state::abort)
        {
            HPX_THROWS_IF(ec, hpx::error::yield_aborted, "suspend",
                "thread({}, {}) aborted (yield returned wait_abort)",
                id.noref(), threads::get_thread_description(id.noref()));
        }

        if (&ec != &throws)
            ec = make_success_code();

        return statex;
    }

    threads::thread_restart_state suspend(
        hpx::chrono::steady_time_point const& abs_time,
        threads::thread_id_type nextid,
        [[maybe_unused]] threads::thread_description const& description,
        error_code& ec)
    {
        // schedule a thread waking us up at_time
        threads::thread_self& self = threads::get_self();

        // keep alive
        threads::thread_id_ref_type id = self.get_outer_thread_id();

        // handle interruption, if needed
        threads::interruption_point(id.noref(), ec);
        if (ec)
            return threads::thread_restart_state::unknown;

        // let the thread manager do other things while waiting
        threads::thread_restart_state statex;

        {
#ifdef HPX_HAVE_VERIFY_LOCKS
            // verify that there are no more registered locks for this OS-thread
            util::verify_no_locks();

            [[maybe_unused]] auto held_locks = hpx::experimental::scope_exit(
                [data = hpx::util::get_held_locks_data()]() mutable {
                    hpx::util::set_held_locks_data(HPX_MOVE(data));
                });
#endif
#ifdef HPX_HAVE_THREAD_DESCRIPTION
            threads::detail::reset_lco_description desc(
                id.noref(), description, ec);
#endif
#ifdef HPX_HAVE_THREAD_BACKTRACE_ON_SUSPENSION
            threads::detail::reset_backtrace bt(id, ec);
#endif
#ifdef HPX_HAVE_MODULE_LIKWID
            hpx::likwid::suspend_region region;
#endif
            hpx::tracing::fiber_suspend_region tracy_suspend(
                get_suspend_reason(description));
            std::atomic<bool> timer_started(false);
            threads::thread_id_ref_type const timer_id =
                threads::set_thread_state(id.noref(), abs_time, &timer_started,
                    threads::thread_schedule_state::pending,
                    threads::thread_restart_state::timeout,
                    threads::thread_priority::boost, true, ec);
            if (ec)
                return threads::thread_restart_state::unknown;

            // We might need to dispatch 'nextid' to it's correct scheduler only
            // if our current scheduler is the same, we should yield to the id
            if (nextid &&
                get_thread_id_data(nextid)->get_scheduler_base() !=
                    get_thread_id_data(id)->get_scheduler_base())
            {
                auto* scheduler =
                    get_thread_id_data(nextid)->get_scheduler_base();
                scheduler->schedule_thread(
                    HPX_MOVE(nextid), threads::thread_schedule_hint());

                statex = self.yield(threads::thread_result_type(
                    threads::thread_schedule_state::suspended,
                    threads::invalid_thread_id));
            }
            else
            {
                statex = self.yield(threads::thread_result_type(
                    threads::thread_schedule_state::suspended,
                    HPX_MOVE(nextid)));
            }

            if (statex != threads::thread_restart_state::timeout)
            {
                HPX_ASSERT(statex == threads::thread_restart_state::abort ||
                    statex == threads::thread_restart_state::signaled);

                error_code ec1(throwmode::lightweight);    // do not throw
                hpx::util::yield_while<true>(
                    [&timer_started]() { return !timer_started.load(); },
                    "set_thread_state_timed");
                threads::set_thread_state(timer_id.noref(),
                    threads::thread_schedule_state::pending,
                    threads::thread_restart_state::abort,
                    threads::thread_priority::boost, true, ec1);
            }
        }

        // handle interruption, if needed
        threads::interruption_point(id.noref(), ec);
        if (ec)
            return threads::thread_restart_state::unknown;

        // handle interrupt and abort
        if (statex == threads::thread_restart_state::abort)
        {
            HPX_THROWS_IF(ec, hpx::error::yield_aborted, "suspend_at",
                "thread({}, {}) aborted (yield returned wait_abort)",
                id.noref(), threads::get_thread_description(id.noref()));
        }

        if (&ec != &throws)
            ec = make_success_code();

        return statex;
    }

    ///////////////////////////////////////////////////////////////////////////
    threads::thread_pool_base* get_pool(error_code& ec)
    {
        return threads::get_pool(threads::get_self_id(), ec);
    }

    std::ptrdiff_t get_available_stack_space() noexcept
    {
        if (threads::thread_self const* self = threads::get_self_ptr())
        {
            return self->get_available_stack_space();
        }
        return (std::numeric_limits<std::ptrdiff_t>::max)();
    }

    bool has_sufficient_stack_space(
        [[maybe_unused]] std::size_t space_needed) noexcept
    {
        if (nullptr == hpx::threads::get_self_ptr())
            return false;

#if defined(HPX_HAVE_THREADS_GET_STACK_POINTER)
        std::ptrdiff_t const remaining_stack = get_available_stack_space();
        if (remaining_stack < 0)
        {
            HPX_THROW_BAD_ALLOC("has_sufficient_stack_space");
        }

        bool const sufficient_stack_space =
            static_cast<std::size_t>(remaining_stack) >= space_needed;

        return sufficient_stack_space;
#else
        return true;
#endif
    }
}    // namespace hpx::this_thread
