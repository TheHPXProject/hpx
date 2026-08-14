//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  Part of this code has been adopted from code published under the BSL by:
//
//  (C) Copyright 2006-7 Anthony Williams

/// \page hpx::recursive_mutex
/// \headerfile hpx/mutex.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/lock_registration/detail/register_locks.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/synchronization/spinlock.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <type_traits>

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    namespace detail {

        /// An exclusive-ownership recursive mutex which implements
        /// Boost.Thread's TimedLockable concept.
        HPX_CXX_CORE_EXPORT template <typename Mutex = hpx::spinlock>
        struct recursive_mutex_impl
        {
        public:
            HPX_NON_COPYABLE(recursive_mutex_impl);

        private:
            std::atomic<std::uint64_t> recursion_count;
            std::atomic<hpx::execution_base::agent_ref> locking_context;
            Mutex mtx;

        public:
            // clang-format off
            explicit recursive_mutex_impl(char const* desc = "recursive_mutex_impl")
                noexcept(noexcept(
                    std::is_nothrow_constructible_v<Mutex, char const*>))
              : recursion_count(0)
              , mtx(desc)
            {
            }
            // clang-format on

            /// Attempts to acquire ownership of the \a recursive_mutex.
            /// Never blocks.
            ///
            /// \returns \a true if ownership was acquired; otherwise, \a false.
            ///
            /// \throws Never throws.
            bool try_lock()
            {
                auto const ctx = hpx::execution_base::this_thread::agent();
                HPX_ASSERT(ctx);

                return try_recursive_lock(ctx) || try_basic_lock(ctx);
            }

            /// Acquires ownership of the \a recursive_mutex. Suspends the
            /// current HPX-thread if ownership cannot be obtained immediately.
            ///
            /// \throws Throws \a hpx#error#bad_parameter if an error occurs
            ///         while suspending. Throws \a hpx#error#yield_aborted if
            ///         the mutex is destroyed while suspended. Throws \a
            ///         hpx#error#null_thread_id if called outside of a
            ///         HPX-thread.
            void lock()
            {
                auto const ctx = hpx::execution_base::this_thread::agent();
                HPX_ASSERT(ctx);

                if (!try_recursive_lock(ctx))
                {
                    mtx.lock();
                    locking_context.exchange(ctx);
                    util::ignore_lock(&mtx);
                    util::register_lock(this);
                    recursion_count.store(1);
                }
            }

            /// Release ownership of the \a recursive_mutex.
            ///
            /// \throws Throws \a hpx#error#bad_parameter if an error occurs
            ///         while releasing the mutex. Throws \a
            ///         hpx#error#null_thread_id if called outside of a
            ///         HPX-thread.
            void unlock()
            {
                if (0 == --recursion_count)
                {
                    locking_context.exchange(hpx::execution_base::agent_ref());
                    util::unregister_lock(this);
                    util::reset_ignored(&mtx);
                    mtx.unlock();
                }
            }

        private:
            bool try_recursive_lock(
                hpx::execution_base::agent_ref current_context)
            {
                if (locking_context.load(std::memory_order_acquire) ==
                    current_context)
                {
                    if (++recursion_count == 1)
                        util::register_lock(this);
                    return true;
                }
                return false;
            }

            bool try_basic_lock(hpx::execution_base::agent_ref current_context)
            {
                if (mtx.try_lock())
                {
                    locking_context.exchange(current_context);
                    util::ignore_lock(&mtx);
                    util::register_lock(this);
                    recursion_count.store(1);
                    return true;
                }
                return false;
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT using recursive_mutex = detail::recursive_mutex_impl<>;
}    // namespace hpx
