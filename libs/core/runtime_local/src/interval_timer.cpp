//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/runtime_local/interval_timer.hpp>
#include <hpx/runtime_local/shutdown_function.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

namespace hpx::util::detail {

    ///////////////////////////////////////////////////////////////////////////
    interval_timer::interval_timer() = default;

    interval_timer::interval_timer(hpx::function<bool()> const& f,
        std::int64_t microsecs, std::string const& description,
        bool pre_shutdown)
      : f_(f)
      , microsecs_(microsecs)
      , description_(description)
      , pre_shutdown_(pre_shutdown)
    {
    }

    interval_timer::interval_timer(hpx::function<bool()> const& f,
        hpx::function<void()> const& on_term, std::int64_t microsecs,
        std::string const& description, bool pre_shutdown)
      : f_(f)
      , on_term_(on_term)
      , microsecs_(microsecs)
      , description_(description)
      , pre_shutdown_(pre_shutdown)
    {
    }

    bool interval_timer::start(bool evaluate_)
    {
        std::unique_lock<mutex_type> l(mtx_);
        if (is_terminated_)
            return false;

        if (!is_started_)
        {
            if (first_start_)
            {
                first_start_ = false;

                hpx::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                if (pre_shutdown_)
                {
                    register_pre_shutdown_function(util::deferred_call(
                        &interval_timer::terminate, this->shared_from_this()));
                }
                else
                {
                    register_shutdown_function(util::deferred_call(
                        &interval_timer::terminate, this->shared_from_this()));
                }
            }

            is_stopped_ = false;

            if (evaluate_)
            {
                l.unlock();
                evaluate(threads::thread_restart_state::signaled);
            }
            else
            {
                schedule_thread(l);
            }

            return true;
        }
        return false;
    }

    bool interval_timer::restart(bool evaluate_)
    {
        if (!is_started_)
            return start(evaluate_);

        std::unique_lock<mutex_type> l(mtx_);

        if (is_terminated_)
            return false;

        // interrupt timer thread, if needed
        stop_locked();

        // reschedule evaluation thread
        if (evaluate_)
        {
            l.unlock();
            evaluate(threads::thread_restart_state::signaled);
        }
        else
        {
            schedule_thread(l);
        }
        return true;
    }

    bool interval_timer::stop(bool terminate_timer)
    {
        if (terminate_timer)
        {
            terminate();
            return true;
        }

        std::lock_guard<mutex_type> l(mtx_);
        is_stopped_ = true;
        return stop_locked();
    }

    bool interval_timer::stop_locked()
    {
        if (is_started_)
        {
            is_started_ = false;

            // a timeout that fires anyway finds the thread no longer in the
            // suspension it was set for (it is aborted below)
            timeout_.cancel();

            if (id_)
            {
                error_code ec(
                    throwmode::lightweight);    // avoid throwing on error
                threads::set_thread_state(id_.noref(),
                    threads::thread_schedule_state::pending,
                    threads::thread_restart_state::abort,
                    threads::thread_priority::boost, true, ec);
                id_.reset();
            }
            return true;
        }

        HPX_ASSERT(id_ == nullptr);
        return false;
    }

    void interval_timer::terminate()
    {
        std::unique_lock<mutex_type> l(mtx_);
        if (!is_terminated_)
        {
            is_terminated_ = true;
            stop_locked();

            if (on_term_)
            {
                l.unlock();
                on_term_();
            }
        }
    }

    interval_timer::~interval_timer()
    {
        try
        {
            terminate();
        }
        // NOLINTNEXTLINE(bugprone-empty-catch)
        catch (...)
        {
        }
    }

    std::int64_t interval_timer::get_interval() const
    {
        std::lock_guard<mutex_type> l(mtx_);
        return microsecs_;
    }

    void interval_timer::change_interval(std::int64_t new_interval)
    {
        HPX_ASSERT(new_interval > 0);

        std::lock_guard<mutex_type> l(mtx_);
        microsecs_ = new_interval;
    }

    threads::thread_result_type interval_timer::evaluate(
        threads::thread_restart_state statex)
    {
        try
        {
            std::unique_lock<mutex_type> l(mtx_);

            if (is_stopped_ || is_terminated_ ||
                statex == threads::thread_restart_state::abort ||
                0 == microsecs_)
            {
                // object has been finalized, exit
                return {threads::thread_schedule_state::terminated,
                    threads::invalid_thread_id};
            }

            if (id_ != nullptr && id_ != threads::get_self_id())
            {
                // obsolete timer thread
                return {threads::thread_schedule_state::terminated,
                    threads::invalid_thread_id};
            }

            id_.reset();
            timeout_ = threads::detail::suspension_timeout();
            is_started_ = false;

            bool result;

            {
                hpx::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                result = f_();    // invoke the supplied function
            }

            // some other thread might already have started the timer
            if (nullptr == id_ && result)
            {
                HPX_ASSERT(!is_started_);
                schedule_thread(l);    // wait and repeat
            }

            if (!result)
                is_terminated_ = true;
        }
        catch (hpx::exception const& e)
        {
            // the lock above might throw yield_aborted
            if (e.get_error() != hpx::error::yield_aborted)
                throw;
        }

        // do not re-schedule this thread
        return {threads::thread_schedule_state::terminated,
            threads::invalid_thread_id};
    }

    // schedule a high priority task after a given time interval
    void interval_timer::schedule_thread(
        [[maybe_unused]] std::unique_lock<mutex_type>& l)
    {
        HPX_ASSERT_OWNS_LOCK(l);

        using namespace hpx::threads;

        error_code ec;

        // create a new suspended thread
        threads::thread_id_ref_type id;
        {
            // FIXME: registering threads might lead to thread suspension since
            // the allocators use hpx::spinlock. Unlocking the lock here would
            // be the right thing but leads to crashes and hangs at shutdown.
            // unlock_guard<std::unique_lock<mutex_type> > ul(l);
            hpx::threads::thread_init_data data(
                hpx::threads::make_thread_function(hpx::bind_front(
                    &interval_timer::evaluate, this->shared_from_this())),
                description_.c_str(), threads::thread_priority::boost,
                threads::thread_schedule_hint(),
                threads::thread_stacksize::default_,
                threads::thread_schedule_state::suspended, true);
            id = hpx::threads::register_thread(data, ec);
        }

        if (ec)
        {
            is_terminated_ = true;
            is_started_ = false;
            return;
        }

        // schedule this thread to be run after the given amount of
        // microseconds: it stays in the suspension it was created in until
        // then, and the timeout of that suspension needs no thread of its own
        std::int64_t const suspension_tag =
            threads::get_thread_id_data(id)->get_state().tag();
        threads::detail::suspension_timeout timeout =
            threads::detail::suspension_timeout::set(
                hpx::chrono::steady_duration(
                    std::chrono::microseconds(microsecs_))
                    .from_now(),
                id.noref(), suspension_tag, threads::thread_priority::boost,
                ec);

        if (ec)
        {
            is_terminated_ = true;
            is_started_ = false;

            // abort the newly created thread
            threads::set_thread_state(id.noref(),
                threads::thread_schedule_state::pending,
                threads::thread_restart_state::abort,
                threads::thread_priority::boost, true, ec);

            return;
        }

        id_ = id;
        timeout_ = HPX_MOVE(timeout);
        is_started_ = true;
    }
}    // namespace hpx::util::detail

namespace hpx::util {

    interval_timer::interval_timer() = default;

    interval_timer::interval_timer(    // -V730
        hpx::function<bool()> const& f, std::int64_t microsecs,
        std::string const& description, bool pre_shutdown)
      : timer_(std::make_shared<detail::interval_timer>(
            f, microsecs, description, pre_shutdown))
    {
    }

    interval_timer::interval_timer(    // -V730
        hpx::function<bool()> const& f, hpx::function<void()> const& on_term,
        std::int64_t microsecs, std::string const& description,
        bool pre_shutdown)
      : timer_(std::make_shared<detail::interval_timer>(
            f, on_term, microsecs, description, pre_shutdown))
    {
    }

    interval_timer::interval_timer(    // -V730
        hpx::function<bool()> const& f,
        hpx::chrono::steady_duration const& rel_time, char const* description,
        bool pre_shutdown)
      : timer_(std::make_shared<detail::interval_timer>(
            f, rel_time.value().count() / 1000, description, pre_shutdown))
    {
    }

    interval_timer::interval_timer(    // -V730
        hpx::function<bool()> const& f, hpx::function<void()> const& on_term,
        hpx::chrono::steady_duration const& rel_time, char const* description,
        bool pre_shutdown)
      : timer_(std::make_shared<detail::interval_timer>(f, on_term,
            rel_time.value().count() / 1000, description, pre_shutdown))
    {
    }

    interval_timer::~interval_timer()
    {
        timer_->terminate();
    }

    std::int64_t interval_timer::get_interval() const
    {
        return timer_->get_interval();
    }

    void interval_timer::change_interval(std::int64_t new_interval) const
    {
        return timer_->change_interval(new_interval);
    }

    void interval_timer::change_interval(
        hpx::chrono::steady_duration const& new_interval) const
    {
        return timer_->change_interval(new_interval.value().count() / 1000);
    }
}    // namespace hpx::util
