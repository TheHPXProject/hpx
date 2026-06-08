////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Lelbach
//  Copyright (c) 2011-2026 Hartmut Kaiser
//  Copyright (c) 2014 Thomas Heller
//  Copyright (c) 2008 Peter Dimov
//  Copyright (c) 2018 Patrick Diehl
//  Copyright (c) 2021 Nikunj Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

/// \file spinlock.hpp
/// \page hpx::spinlock
/// \headerfile hpx/mutex.hpp

#pragma once

#include <hpx/config.hpp>

#include <hpx/lock_registration/detail/register_locks.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/tracing.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    namespace detail {

        // std::mutex-compatible spinlock class (Backoff == true)
        // boost::mutex-compatible spinlock class (Backoff == false)
        HPX_CXX_CORE_EXPORT template <bool Backoff = true>
        struct spinlock
        {
        public:
            spinlock(spinlock const&) = delete;
            spinlock& operator=(spinlock const&) = delete;
            spinlock(spinlock&&) = delete;
            spinlock& operator=(spinlock&&) = delete;

        private:
            std::atomic<bool> v_;
            HPX_NO_UNIQUE_ADDRESS hpx::tracing::lock_context context_;

        public:
#if defined(HPX_HAVE_TRACING)
            spinlock() noexcept
              : v_(false)
              , context_("hpx::spinlock", this)
            {
            }

            explicit spinlock(char const* const desc) noexcept
              : v_(false)
              , context_("hpx::spinlock#", desc, this)
            {
            }

            ~spinlock() = default;
#else
            constexpr spinlock() noexcept
              : v_(false)
            {
            }

            explicit constexpr spinlock(char const* const) noexcept
              : v_(false)
            {
            }

            ~spinlock() = default;
#endif

            void lock()
            {
                bool const run_after = context_.before_lock();

                // Checking for the value in is_locked() ensures that
                // acquire_lock is only called when is_locked computes to false.
                // This way we spin only on a load operation which minimizes
                // false sharing that comes with an exchange operation. Consider
                // the following cases:
                //
                // 1. Only one thread wants access critical section:
                //      is_locked() -> false; computes acquire_lock()
                //      acquire_lock() -> false (new value set to true) Thread
                //      acquires the lock and moves to critical section.
                // 2. Two threads simultaneously access critical section:
                //      Thread 1: is_locked() || acquire_lock() -> false Thread
                //      1 acquires the lock and moves to critical section.
                //      Thread 2: is_locked() -> true; execution enters inside
                //      while without computing acquire_lock(). Thread 2 yields
                //      while is_locked() computes to false. Then it retries
                //      doing is_locked() -> false followed by an acquire_lock()
                //      operation. The above order can be changed arbitrarily
                //      but the nature of execution will still remain the same.
                if (!acquire_lock())
                {
                    auto pred = [this]() noexcept { return is_locked(); };
                    do
                    {
                        util::yield_while<Backoff>(pred, "hpx::spinlock::lock");
                    } while (!acquire_lock_plain());
                }
                if (run_after)
                    context_.after_lock();
                util::register_lock(this);
            }

            bool try_lock() noexcept(
                noexcept(util::register_lock(std::declval<spinlock*>())))
            {
                bool const run_after = context_.before_lock();

                if (acquire_lock())
                {
                    if (run_after)
                        context_.after_try_lock(true);
                    util::register_lock(this);
                    return true;
                }

                if (run_after)
                    context_.after_try_lock(false);
                return false;
            }

            void unlock() noexcept(
                noexcept(util::unregister_lock(std::declval<spinlock*>())))
            {
                context_.before_unlock();

                relinquish_lock();

                context_.after_unlock();
                util::unregister_lock(this);
            }

        private:
            // returns whether the mutex has been acquired
            HPX_FORCEINLINE bool acquire_lock() noexcept
            {
                // First do a relaxed load to check if lock is free in order to
                // prevent unnecessary cache misses.
                return !v_.load(std::memory_order_relaxed) &&
                    !v_.exchange(true, std::memory_order_acquire);
            }

            HPX_FORCEINLINE bool acquire_lock_plain() noexcept
            {
                return !v_.exchange(true, std::memory_order_acquire);
            }

            // relinquish lock
            HPX_FORCEINLINE void relinquish_lock() noexcept
            {
                v_.store(false, std::memory_order_release);
            }

            HPX_FORCEINLINE bool is_locked() const noexcept
            {
                return v_.load(std::memory_order_relaxed);
            }
        };
    }    // namespace detail

    /// \brief \c spinlock is a type of lock that causes a thread attempting to
    ///        obtain it to check for its availability while waiting in a loop
    ///        continuously.
    HPX_CXX_CORE_EXPORT using spinlock = detail::spinlock<true>;
    HPX_CXX_CORE_EXPORT using spinlock_no_backoff = detail::spinlock<false>;
}    // namespace hpx
