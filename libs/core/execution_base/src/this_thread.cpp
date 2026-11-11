//  Copyright (c) 2013-2019 Thomas Heller
//  Copyright (c) 2008 Peter Dimov
//  Copyright (c) 2018-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/execution_base/agent_base.hpp>
#include <hpx/execution_base/context_base.hpp>
#include <hpx/execution_base/this_thread.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/timing.hpp>

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

#if !defined(HPX_WINDOWS)
#ifndef _AIX
#include <sched.h>
#else
// AIX's sched.h defines ::var which sometimes conflicts with Lambda's var
extern "C" int sched_yield(void);
#endif
#include <time.h>
#endif

namespace {
    // Upper bound on how long any single sleep_for/sleep_until increment blocks
    // before re-checking wait_cond(). Keeps the fallback agent reasonably
    // responsive without spinning.
    constexpr std::chrono::milliseconds default_agent_poll_interval{20};
}    // namespace

namespace hpx::execution_base {

    namespace {

        ///////////////////////////////////////////////////////////////////////
        struct default_context final : execution_base::context_base
        {
            [[nodiscard]] resource_base const& resource()
                const noexcept override
            {
                return resource_;
            }

            resource_base resource_;
        };

        struct default_agent : execution_base::agent_base
        {
            default_agent();

            [[nodiscard]] std::string description() const override
            {
                return hpx::util::format("{}", id_);
            }

            [[nodiscard]] default_context const& context()
                const noexcept override
            {
                return context_;
            }

            void yield(char const* desc) override;
            bool yield_k(std::size_t k, char const* desc) override;
            void suspend(char const* desc) override;
            void resume(hpx::threads::thread_priority priority,
                char const* desc) override;
            void abort(char const* desc) override;
            threads::thread_restart_state sleep_for(
                hpx::chrono::steady_duration const& sleep_duration,
                hpx::move_only_function<bool()>&& wait_cond,
                char const* desc) override;
            threads::thread_restart_state sleep_until(
                hpx::chrono::steady_time_point const& sleep_time,
                hpx::move_only_function<bool()>&& wait_cond,
                char const* desc) override;

        private:
            bool running_;
            bool aborted_;
            // Set by resume() to record a wakeup. resume() never blocks:
            // a wakeup that arrives before the agent has parked is
            // remembered here and consumed by the next suspend() or
            // sleep_until() chunk, so it is neither lost nor able to
            // deadlock a caller that holds a user-level lock.
            bool wakeup_pending_;
            std::thread::id id_;
            std::mutex mtx_;
            std::condition_variable suspend_cv_;
            default_context context_;
        };

        default_agent::default_agent()
          : running_(true)
          , aborted_(false)
          , wakeup_pending_(false)
          , id_(std::this_thread::get_id())
        {
        }

        void default_agent::yield(char const* /* desc */)
        {
#if defined(HPX_WINDOWS)
            hpx::util::win_nanosleep(hpx::util::wait_0ns);
#else
            sched_yield();
#endif
        }

        bool default_agent::yield_k(std::size_t const k, char const* /* desc */)
        {
            return hpx::util::detail::yield_k_backoff(static_cast<unsigned>(k));
        }

        void default_agent::suspend(char const* /* desc */)
        {
            std::unique_lock<std::mutex> l(mtx_);
            HPX_ASSERT(running_);

            running_ = false;
            while (!wakeup_pending_ && !aborted_)
            {
                suspend_cv_.wait(l);    //-V1089
            }
            wakeup_pending_ = false;    // consume exactly one wakeup
            running_ = true;

            if (aborted_)
            {
                aborted_ = false;    // report each abort exactly once
                HPX_THROW_EXCEPTION(hpx::error::yield_aborted, "suspend",
                    "std::thread({}) aborted (yield returned wait_abort)", id_);
            }
        }

        void default_agent::resume(hpx::threads::thread_priority, char const*)
        {
            {
                std::lock_guard<std::mutex> l(mtx_);
                // Record the wakeup and return. The agent consumes it in
                // suspend() or at the next sleep_until() chunk, whether it
                // is already parked or still on its way into the wait.
                wakeup_pending_ = true;
                suspend_cv_.notify_one();
            }
        }

        void default_agent::abort(char const* /* desc */)
        {
            {
                std::lock_guard<std::mutex> l(mtx_);
                aborted_ = true;
                suspend_cv_.notify_one();
            }
        }

        threads::thread_restart_state default_agent::sleep_for(
            hpx::chrono::steady_duration const& sleep_duration,
            hpx::move_only_function<bool()>&& wait_cond, char const* desc)
        {
            return sleep_until(
                hpx::chrono::steady_time_point(
                    std::chrono::steady_clock::now() + sleep_duration.value()),
                HPX_MOVE(wait_cond), desc);
        }

        threads::thread_restart_state default_agent::sleep_until(
            hpx::chrono::steady_time_point const& sleep_time,
            hpx::move_only_function<bool()>&& wait_cond, char const* desc)
        {
            auto const cond = HPX_MOVE(wait_cond);
            auto const deadline = sleep_time.value();
            auto check_abort = [this, desc]() {
                std::lock_guard<std::mutex> l(mtx_);
                if (aborted_)
                {
                    aborted_ = false;    // report each abort exactly once
                    HPX_THROW_EXCEPTION(hpx::error::yield_aborted, desc,
                        "std::thread({}) aborted (sleep_until returned "
                        "wait_abort)",
                        id_);
                }
            };

            // A previous abort is latched; report it before waiting again
            // (suspend() does the same after waking).
            {
                std::lock_guard<std::mutex> l(mtx_);
                if (aborted_)
                {
                    aborted_ = false;    // report each abort exactly once
                    HPX_THROW_EXCEPTION(hpx::error::yield_aborted, desc,
                        "std::thread({}) aborted (sleep_until returned "
                        "wait_abort)",
                        id_);
                }
            }

            for (;;)
            {
                if (cond && cond())
                {
                    check_abort();
                    return threads::thread_restart_state::signaled;
                }

                auto const now = std::chrono::steady_clock::now();
                if (now >= deadline)
                {
                    break;
                }

                auto const remaining = deadline - now;
                auto const chunk = (std::min) (remaining,
                    std::chrono::duration_cast<
                        std::remove_const_t<decltype(remaining)>>(
                        default_agent_poll_interval));

                // Sleep one chunk while staying resumable. A wakeup
                // recorded by resume() before this chunk (or during it)
                // ends the chunk early so cond() is re-evaluated; it is
                // consumed here, exactly once.
                bool aborted = false;
                {
                    std::unique_lock<std::mutex> l(mtx_);
                    HPX_ASSERT(running_);
                    if (!wakeup_pending_ && !aborted_)
                    {
                        running_ = false;
                        suspend_cv_.wait_for(l, chunk,
                            [&] { return wakeup_pending_ || aborted_; });
                        running_ = true;
                    }
                    wakeup_pending_ = false;
                    aborted = aborted_;
                    aborted_ = false;    // reported via the snapshot below
                }

                if (aborted)
                {
                    HPX_THROW_EXCEPTION(hpx::error::yield_aborted, desc,
                        "std::thread({}) aborted (sleep_until returned "
                        "wait_abort)",
                        id_);
                }
            }

            // final check right at/after the deadline, in case wait_cond
            // became true during the last sleep increment
            if (cond && cond())
            {
                check_abort();
                return threads::thread_restart_state::signaled;
            }

            // A latched abort must not be masked by the deadline: cond() can
            // block across the deadline while abort() sets aborted_, in which
            // case the loop above breaks without reaching its in-loop check.
            check_abort();

            return threads::thread_restart_state::timeout;
        }
    }    // namespace

    namespace detail {

        [[nodiscard]] agent_base& get_default_agent()
        {
            static thread_local default_agent agent;
            return agent;
        }
    }    // namespace detail

    namespace this_thread {

        namespace detail {

            struct agent_storage
            {
                agent_storage()
                  : impl_(&hpx::execution_base::detail::get_default_agent())
                {
                }

                agent_base* set(agent_base* context) noexcept
                {
                    std::swap(context, impl_);
                    return context;
                }

                agent_base* impl_;
            };

            [[nodiscard]] agent_storage* get_agent_storage()
            {
                static thread_local agent_storage storage;
                return &storage;
            }
        }    // namespace detail

        reset_agent::reset_agent(
            detail::agent_storage* storage, agent_base& impl)
          : storage_(storage)
          , old_(storage_->set(&impl))
        {
        }

        reset_agent::reset_agent(agent_base& impl)
          : reset_agent(detail::get_agent_storage(), impl)
        {
        }

        reset_agent::~reset_agent()
        {
            storage_->set(old_);
        }

        [[nodiscard]] agent_ref agent()
        {
            return agent_ref(detail::get_agent_storage()->impl_);
        }

        void yield(char const* desc)
        {
            agent().yield(desc);
        }

        bool yield_k(std::size_t k, char const* desc)
        {
            return agent().yield_k(k, desc);
        }

        void suspend(char const* desc)
        {
            agent().suspend(desc);
        }
    }    // namespace this_thread
}    // namespace hpx::execution_base
