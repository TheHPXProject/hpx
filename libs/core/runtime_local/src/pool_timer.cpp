//  Copyright (c) 2016 Bibek Wagle
//  Copyright (c) 2024-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/io_service.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/runtime_local/pool_timer.hpp>
#include <hpx/runtime_local/runtime_local.hpp>
#include <hpx/runtime_local/shutdown_function.hpp>

#include <asio/basic_waitable_timer.hpp>

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <system_error>

namespace hpx::util::detail {

    ///////////////////////////////////////////////////////////////////////////
    class pool_timer : public std::enable_shared_from_this<pool_timer>
    {
        friend class util::pool_timer;

        using mutex_type = hpx::spinlock;

    public:
        pool_timer();

        pool_timer(hpx::function<bool()> const& f,
            hpx::function<void()> const& on_term, std::string description,
            bool pre_shutdown);

        pool_timer(pool_timer const&) = delete;
        pool_timer(pool_timer&&) = delete;
        pool_timer& operator=(pool_timer const&) = delete;
        pool_timer& operator=(pool_timer&&) = delete;

        ~pool_timer();

        bool start(
            hpx::chrono::steady_duration const& time_duration, bool evaluate);
        bool stop();

        [[nodiscard]] constexpr bool is_started() const noexcept
        {
            return is_started_;
        }
        [[nodiscard]] constexpr bool is_terminated() const noexcept
        {
            return is_terminated_;
        }
        void timer_handler(std::error_code const&);

        void terminate();    // handle system shutdown
        bool stop_locked();

    private:
        using deadline_timer =
            ::asio::basic_waitable_timer<std::chrono::steady_clock>;

        mutable mutex_type mtx_;
        hpx::function<bool()> f_;          ///< function to call
        hpx::function<void()> on_term_;    ///< function to call on termination
        std::string description_;    ///< description of this interval timer

        bool pre_shutdown_ =
            false;    ///< execute termination during pre-shutdown
        bool is_started_ = false;    ///< timer has been started (is running)
        bool first_start_ =
            true;    ///< flag to distinguish first start invocation
        bool is_terminated_ = false;    ///< The timer has been terminated
        bool is_stopped_ = false;
        deadline_timer* timer_ = nullptr;
    };

    ///////////////////////////////////////////////////////////////////////////
    pool_timer::pool_timer()
      : timer_(new deadline_timer(
            hpx::get_runtime().get_thread_pool("timer_pool")->get_io_service()))
    {
    }

    pool_timer::pool_timer(hpx::function<bool()> const& f,
        hpx::function<void()> const& on_term, std::string description,
        bool pre_shutdown)
      : f_(f)
      , on_term_(on_term)
      , description_(HPX_MOVE(description))
      , pre_shutdown_(pre_shutdown)
      , timer_(new deadline_timer(
            hpx::get_runtime().get_thread_pool("timer_pool")->get_io_service()))
    {
    }

    void pool_timer::timer_handler(std::error_code const& err)
    {
        if (!is_stopped_ || !is_terminated_)
        {
            is_started_ = false;
            if (!err)
            {
                [[maybe_unused]] auto _ = f_();
            }
        }
    }

    bool pool_timer::start(
        hpx::chrono::steady_duration const& time_duration, bool /* evaluate_ */)
    {
        std::unique_lock<mutex_type> l(mtx_);
        if (is_terminated_)
            return false;

        if (!is_started_)
        {
            is_stopped_ = false;
            is_started_ = true;

            if (first_start_)
            {
                first_start_ = false;

                hpx::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                if (pre_shutdown_)
                {
                    register_pre_shutdown_function(util::deferred_call(
                        &pool_timer::terminate, this->shared_from_this()));
                }
                else
                {
                    register_shutdown_function(util::deferred_call(
                        &pool_timer::terminate, this->shared_from_this()));
                }
            }

            HPX_ASSERT(timer_ != nullptr);
            timer_->expires_at(time_duration.from_now());
            timer_->async_wait(hpx::bind_front(    //-V779
                &pool_timer::timer_handler, this->shared_from_this()));

            return true;
        }
        return false;
    }

    bool pool_timer::stop()
    {
        std::lock_guard<mutex_type> l(mtx_);
        return stop_locked();
    }

    bool pool_timer::stop_locked()
    {
        if (!is_terminated_ && is_started_ && !is_stopped_)
        {
            is_started_ = false;
            is_stopped_ = true;

            HPX_ASSERT(timer_ != nullptr);
            timer_->cancel();
            return true;
        }
        return false;
    }

    void pool_timer::terminate()
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
        delete timer_;
        timer_ = nullptr;
    }

    pool_timer::~pool_timer()
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
}    // namespace hpx::util::detail

namespace hpx::util {

    pool_timer::pool_timer() = default;

    pool_timer::pool_timer(hpx::function<bool()> const& f,
        hpx::function<void()> const& on_term, std::string const& description,
        bool pre_shutdown)
      : timer_(std::make_shared<detail::pool_timer>(
            f, on_term, description, pre_shutdown))
    {
    }

    pool_timer::~pool_timer()
    {
        timer_->terminate();
    }

    bool pool_timer::start(
        hpx::chrono::steady_duration const& time_duration, bool evaluate) const
    {
        return timer_->start(time_duration, evaluate);
    }

    bool pool_timer::stop() const
    {
        return timer_->stop();
    }

    bool pool_timer::is_started() const
    {
        return timer_->is_started();
    }

    bool pool_timer::is_terminated() const
    {
        return timer_->is_terminated();
    }
}    // namespace hpx::util
