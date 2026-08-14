//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/threading_base/create_work.hpp>
#include <hpx/threading_base/register_thread.hpp>
#include <hpx/threading_base/scheduler_base.hpp>
#include <hpx/threading_base/set_thread_state.hpp>
#include <hpx/threading_base/thread_data.hpp>
#include <hpx/threading_base/threading_base_fwd.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace hpx::threads::detail {

    ///////////////////////////////////////////////////////////////////////////
    namespace {

        thread_result_type set_active_state(thread_id_ref_type const& thrd,
            thread_schedule_state const newstate,
            thread_restart_state const newstate_ex,
            thread_priority const priority, thread_state const previous_state,
            thread_schedule_hint const schedulehint)
        {
            if (HPX_UNLIKELY(!thrd))
            {
                HPX_THROW_EXCEPTION(hpx::error::null_thread_id,
                    "threads::detail::set_active_state",
                    "null thread id encountered");
            }

            // make sure that the thread has not been suspended and set active
            // again in the meantime
            thread_state const current_state =
                get_thread_id_data(thrd)->get_state();

            if (current_state.state() == previous_state.state() &&
                current_state != previous_state)
            {
                LTM_(warning).format(
                    "set_active_state: thread is still active, however it was "
                    "non-active since the original set_state request was "
                    "issued, aborting state change, thread({}), "
                    "description({}), new state({})",
                    thrd, get_thread_id_data(thrd)->get_description(),
                    get_thread_state_name(newstate));
                return {thread_schedule_state::terminated, invalid_thread_id};
            }

            // Just retry, set_state will create new thread if target is still
            // active.
            error_code ec(throwmode::lightweight);    // do not throw
            detail::set_thread_state(thrd.noref(), newstate, newstate_ex,
                priority, schedulehint, true, ec);

            return {thread_schedule_state::terminated, invalid_thread_id};
        }

        thread_state reschedule_active(thread_id_type const& thrd,
            thread_schedule_state new_state, thread_restart_state new_state_ex,
            thread_priority priority, thread_state previous_state,
            thread_schedule_hint schedulehint, error_code& ec)
        {
            // schedule a new thread to set the state
            LTM_(warning).format(
                "set_thread_state: thread is currently active, "
                "scheduling new thread, thread({}), description({}), "
                "new state({})",
                thrd, get_thread_id_data(thrd)->get_description(),
                get_thread_state_name(new_state));

            thread_init_data data(
                hpx::bind(&set_active_state, thread_id_ref_type(thrd),
                    new_state, new_state_ex, priority, previous_state,
                    schedulehint),
                "set state for active thread", priority, schedulehint);

            create_work(
                get_thread_id_data(thrd)->get_scheduler_base(), data, ec);

            if (&ec != &throws)
                ec = make_success_code();

            return previous_state;    // done
        }
    }    // namespace

    ///////////////////////////////////////////////////////////////////////////
    thread_state set_thread_state(thread_id_type const& thrd,
        thread_schedule_state const new_state,
        thread_restart_state const new_state_ex, thread_priority const priority,
        thread_schedule_hint schedulehint, bool retry_on_active, error_code& ec)
    {
        if (HPX_UNLIKELY(!thrd))
        {
            HPX_THROWS_IF(ec, hpx::error::null_thread_id,
                "threads::detail::set_thread_state",
                "null thread id encountered");
            return thread_state{
                thread_schedule_state::unknown, thread_restart_state::unknown};
        }

        // set_state can't be used to force a thread into active state
        if (new_state == thread_schedule_state::active)
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "threads::detail::set_thread_state", "invalid new state: {}",
                new_state);
            return thread_state{
                thread_schedule_state::unknown, thread_restart_state::unknown};
        }

        thread_state previous_state;
        std::size_t k = 0;
        do
        {
            // action depends on the current state
            previous_state = get_thread_id_data(thrd)->get_state();
            thread_schedule_state previous_state_val = previous_state.state();

            // nothing to do here if the state doesn't change
            if (new_state == previous_state_val)
            {
                LTM_(warning).format(
                    "set_thread_state: old thread state is the same as new "
                    "thread state, aborting state change, thread({}), "
                    "description({}), new state({})",
                    thrd, get_thread_id_data(thrd)->get_description(),
                    get_thread_state_name(new_state));

                if (&ec != &throws)
                    ec = make_success_code();

                return thread_state{new_state, previous_state.state_ex()};
            }

            // the thread to set the state for is currently running, so we
            // schedule another thread to execute the pending set_state
            switch (previous_state_val)
            {
            case thread_schedule_state::active:
            {
                if (retry_on_active)
                {
                    return reschedule_active(thrd, new_state, new_state_ex,
                        priority, previous_state, schedulehint, ec);
                }

                if (hpx::execution_base::this_thread::yield_k(
                        k, "hpx::threads::detail::set_thread_state"))
                {
                    retry_on_active = true;    // don't wait too long
                }
                ++k;

                LTM_(warning).format(
                    "set_thread_state: thread is currently active, but not "
                    "scheduling new thread because retry_on_active = "
                    "false, thread({}), description({}), new state({})",
                    thrd, get_thread_id_data(thrd)->get_description(),
                    get_thread_state_name(new_state));

                continue;
            }
            case thread_schedule_state::deleted:
                [[fallthrough]];
            case thread_schedule_state::terminated:
            {
                LTM_(warning).format(
                    "set_thread_state: thread is terminated, aborting state "
                    "change, thread({}), description({}), new state({})",
                    thrd, get_thread_id_data(thrd)->get_description(),
                    get_thread_state_name(new_state));

                if (&ec != &throws)
                    ec = make_success_code();

                // If the thread has been terminated while this set_state was
                // pending nothing has to be done anymore.
                return previous_state;
            }

            case thread_schedule_state::pending:
                [[fallthrough]];
            case thread_schedule_state::pending_boost:
                if (thread_schedule_state::suspended == new_state)
                {
                    // we do not allow explicit resetting of a state to suspended
                    // without the thread being executed.
                    std::string str = hpx::util::format(
                        "set_thread_state: invalid new state, can't demote a "
                        "pending thread, thread({}), description({}), new "
                        "state({})",
                        thrd, get_thread_id_data(thrd)->get_description(),
                        new_state);

                    LTM_(fatal) << str;

                    HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                        "threads::detail::set_thread_state", str);
                    return thread_state{thread_schedule_state::unknown,
                        thread_restart_state::unknown};
                }
                break;

            case thread_schedule_state::suspended:
                break;    // fine, just set the new state

            case thread_schedule_state::unknown:
            case thread_schedule_state::depleted:
            case thread_schedule_state::staged:
            case thread_schedule_state::pending_do_not_schedule:
                [[fallthrough]];
            default:
            {
                HPX_ASSERT_MSG(false,
                    hpx::util::format("set_thread_state: previous state was {}",
                        previous_state_val));    // should not happen
                break;
            }
            }

            // If the previous state was pending we are supposed to remove the
            // thread from the queue. But in order to avoid linearly looking
            // through the queue we defer this to the thread function, which at
            // some point will ignore this thread by simply skipping it (if it's
            // not pending anymore).

            LTM_(info).format("set_thread_state: thread({}), description({}), "
                              "new state({}), old state({})",
                thrd, get_thread_id_data(thrd)->get_description(),
                get_thread_state_name(new_state),
                get_thread_state_name(previous_state_val));

            // So all what we do here is to set the new state.
            if (get_thread_id_data(thrd)->restore_state(
                    new_state, new_state_ex, previous_state))
            {
                break;
            }

            // state has changed since we fetched it from the thread, retry
            LTM_(warning).format(
                "set_thread_state: state has been changed since it was "
                "fetched, retrying, thread({}), description({}), new "
                "state({}), old state({})",
                get_thread_id_data(thrd),
                get_thread_id_data(thrd)->get_description(),
                get_thread_state_name(new_state),
                get_thread_state_name(previous_state_val));
        } while (true);

        thread_schedule_state const previous_state_val = previous_state.state();

        if (!(previous_state_val == thread_schedule_state::pending ||
                previous_state_val == thread_schedule_state::pending_boost) &&
            (new_state == thread_schedule_state::pending ||
                new_state == thread_schedule_state::pending_boost))
        {
            auto* thrd_data = get_thread_id_data(thrd);
            auto* scheduler = thrd_data->get_scheduler_base();
            auto const current_priority = thrd_data->get_priority();
            if (priority == thread_priority::bound)
            {
                if (schedulehint.mode == thread_schedule_hint_mode::thread &&
                    schedulehint.hint != -1)
                {
                    thrd_data->set_last_worker_thread_num(
                        static_cast<std::uint16_t>(schedulehint.hint));
                }
                thrd_data->set_priority(thread_priority::bound);
                scheduler->schedule_thread(
                    thrd, schedulehint, false, thread_priority::bound);
            }
            else if (current_priority == thread_priority::bound)
            {
                schedulehint.mode = thread_schedule_hint_mode::thread;
                schedulehint.hint = static_cast<std::int16_t>(
                    thrd_data->get_last_worker_thread_num());
                scheduler->schedule_thread(
                    thrd, schedulehint, false, thread_priority::bound);
            }
            else
            {
                threads::thread_priority p =
                    (priority == thread_priority::default_ ||
                        priority == thread_priority::unknown) ?
                    current_priority :
                    priority;

                scheduler->schedule_thread(thrd, schedulehint, false, p);
            }

            scheduler->do_some_work(schedulehint.mode ==
                        hpx::threads::thread_schedule_hint_mode::numa ?
                    static_cast<std::size_t>(-1) :
                    schedulehint.hint);
        }

        if (&ec != &throws)
            ec = make_success_code();

        return previous_state;
    }
}    // namespace hpx::threads::detail
