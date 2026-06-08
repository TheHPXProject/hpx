//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2013-2015 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/assert.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/lock_registration.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/synchronization/condition_variable.hpp>
#include <hpx/synchronization/mutex.hpp>
#include <hpx/synchronization/spinlock.hpp>

#include <mutex>
#include <string>
#include <utility>

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_TRACING)
    mutex::mutex(char const* const description)
      : owner_id_(threads::invalid_thread_id)
      , context_("hpx::mutex#", description, this)
    {
    }
#endif

#if defined(HPX_HAVE_TRACING)
    mutex::~mutex() = default;
#else
    mutex::~mutex() = default;
#endif

    void mutex::lock(char const* description, error_code& ec)
    {
        HPX_ASSERT(threads::get_self_ptr() != nullptr);

        bool const run_after = context_.before_lock();

        {
            std::unique_lock<mutex_type> l(mtx_);

            threads::thread_id_type const self_id = threads::get_self_id();
            if (owner_id_ == self_id)
            {
                l.unlock();
                if (run_after)
                    context_.after_try_lock(false);
                HPX_THROWS_IF(ec, hpx::error::deadlock, description,
                    "The calling thread already owns the mutex");
                return;
            }

            while (owner_id_ != threads::invalid_thread_id)
            {
                cond_.wait(l, ec);
                if (ec)
                {
                    if (run_after)
                        context_.after_try_lock(false);
                    return;
                }
            }

            util::register_lock(this);
            owner_id_ = self_id;
        }

        if (run_after)
            context_.after_lock();
    }

    bool mutex::try_lock(char const* /* description */, error_code& /* ec */)
    {
        HPX_ASSERT(threads::get_self_ptr() != nullptr);

        bool const run_after = context_.before_lock();

        {
            std::unique_lock<mutex_type> l(mtx_);

            if (owner_id_ != threads::invalid_thread_id)
            {
                if (run_after)
                    context_.after_try_lock(false);
                return false;
            }

            util::register_lock(this);
            owner_id_ = threads::get_self_id();
        }

        if (run_after)
            context_.after_try_lock(true);

        return true;
    }

    void mutex::unlock(error_code& ec)
    {
        HPX_ASSERT(threads::get_self_ptr() != nullptr);

        context_.before_unlock();
        // Unregister lock early as the lock guard below may suspend.
        util::unregister_lock(this);
        std::unique_lock<mutex_type> l(mtx_);

        threads::thread_id_type const self_id = threads::get_self_id();
        if (HPX_UNLIKELY(owner_id_ != self_id))
        {
            l.unlock();
            HPX_THROWS_IF(ec, hpx::error::lock_error, "mutex::unlock",
                "The calling thread does not own the mutex");
            return;
        }

        owner_id_ = threads::invalid_thread_id;

        context_.after_unlock();

        {
            [[maybe_unused]] util::ignore_while_checking il(&l);

            // Failing to release lock 'this->mtx' in function
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26115)
#endif

            cond_.notify_one(HPX_MOVE(l), threads::thread_priority::boost, ec);
            il.reset_owns_registration();

#if defined(HPX_MSVC)
#pragma warning(pop)
#endif
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    timed_mutex::timed_mutex(char const* const description)
      : mutex(description)
    {
    }

    timed_mutex::~timed_mutex() = default;

    bool timed_mutex::try_lock_until(
        hpx::chrono::steady_time_point const& abs_time,
        char const* /* description */, error_code& ec)
    {
        HPX_ASSERT(threads::get_self_ptr() != nullptr);

        bool const run_after = context_.before_lock();
        std::unique_lock<mutex_type> l(mtx_);

        threads::thread_id_type const self_id = threads::get_self_id();
        if (owner_id_ != threads::invalid_thread_id)
        {
            threads::thread_restart_state const reason =
                cond_.wait_until(l, abs_time, ec);
            if (ec)
            {
                if (run_after)
                    context_.after_try_lock(false);
                return false;
            }

            if (reason == threads::thread_restart_state::timeout)    //-V110
            {
                if (run_after)
                    context_.after_try_lock(false);
                return false;
            }

            if (owner_id_ != threads::invalid_thread_id)    //-V110
            {
                if (run_after)
                    context_.after_try_lock(false);
                return false;
            }
        }

        util::register_lock(this);
        if (run_after)
            context_.after_try_lock(true);
        owner_id_ = self_id;
        return true;
    }
}    // namespace hpx
