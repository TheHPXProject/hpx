//  Copyright (c) 2007-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/timing.hpp>

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::util {

    HPX_CXX_CORE_EXPORT class interval_timer;
}    // namespace hpx::util

namespace hpx::util::detail {

    ///////////////////////////////////////////////////////////////////////////
    class HPX_CORE_EXPORT interval_timer
      : public std::enable_shared_from_this<interval_timer>
    {
        friend class util::interval_timer;

        using mutex_type = hpx::spinlock;

    public:
        interval_timer();

        interval_timer(hpx::function<bool()> const& f, std::int64_t microsecs,
            std::string const& description, bool pre_shutdown);
        interval_timer(hpx::function<bool()> const& f,
            hpx::function<void()> const& on_term, std::int64_t microsecs,
            std::string const& description, bool pre_shutdown);

        ~interval_timer();

        bool start(bool evaluate);
        bool stop(bool terminate = false);

        bool restart(bool evaluate);

        [[nodiscard]] constexpr bool is_started() const noexcept
        {
            return is_started_;
        }
        [[nodiscard]] constexpr bool is_terminated() const noexcept
        {
            return is_terminated_;
        }

        [[nodiscard]] std::int64_t get_interval() const;

        void change_interval(std::int64_t new_interval);

    protected:
        // schedule a high priority task after a given time interval
        void schedule_thread(std::unique_lock<mutex_type>& l);

        threads::thread_result_type evaluate(
            threads::thread_restart_state statex);

        void terminate();    // handle system shutdown
        bool stop_locked();

    private:
        mutable mutex_type mtx_;
        /// function to call
        hpx::function<bool()> f_;
        /// function to call on termination
        hpx::function<void()> on_term_;
        /// time interval
        std::int64_t microsecs_ = 0;
        /// id of currently scheduled thread
        threads::thread_id_ref_type id_;
        /// the timeout that starts the currently scheduled thread
        threads::detail::suspension_timeout timeout_;
        /// description of this interval timer
        std::string description_;

        /// execute termination during pre-shutdown
        bool pre_shutdown_ = false;
        /// timer has been started (is running)
        bool is_started_ = false;
        /// flag to distinguish first invocation of start()
        bool first_start_ = true;
        /// The timer has been terminated
        bool is_terminated_ = false;
        bool is_stopped_ = false;
    };
}    // namespace hpx::util::detail

namespace hpx::util {

    HPX_CXX_CORE_EXPORT class HPX_CORE_EXPORT interval_timer
    {
    public:
        interval_timer();

        interval_timer(interval_timer const&) = delete;
        interval_timer(interval_timer&&) = delete;
        interval_timer& operator=(interval_timer const&) = delete;
        interval_timer& operator=(interval_timer&&) = delete;

        interval_timer(hpx::function<bool()> const& f, std::int64_t microsecs,
            std::string const& description = "", bool pre_shutdown = false);
        interval_timer(hpx::function<bool()> const& f,
            hpx::function<void()> const& on_term, std::int64_t microsecs,
            std::string const& description = "", bool pre_shutdown = false);

        interval_timer(hpx::function<bool()> const& f,
            hpx::chrono::steady_duration const& rel_time,
            char const* description = "", bool pre_shutdown = false);
        interval_timer(hpx::function<bool()> const& f,
            hpx::function<void()> const& on_term,
            hpx::chrono::steady_duration const& rel_time,
            char const* description = "", bool pre_shutdown = false);

        ~interval_timer();

        bool start(bool evaluate = true) const
        {
            return timer_->start(evaluate);
        }
        bool stop(bool terminate = false) const
        {
            return timer_->stop(terminate);
        }

        bool restart(bool evaluate = true) const
        {
            return timer_->restart(evaluate);
        }

        [[nodiscard]] bool is_started() const
        {
            return timer_->is_started();
        }
        [[nodiscard]] bool is_terminated() const
        {
            return timer_->is_terminated();
        }

        [[nodiscard]] std::int64_t get_interval() const;

        void change_interval(std::int64_t new_interval) const;
        void change_interval(
            hpx::chrono::steady_duration const& new_interval) const;

    private:
        std::shared_ptr<detail::interval_timer> timer_;
    };
}    // namespace hpx::util

#include <hpx/config/warnings_suffix.hpp>
