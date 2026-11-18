//  Copyright (c) 2023 Johan511
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  This work is based on https://github.com/Johan511/ByteLock

#pragma once

#include <hpx/synchronization/detail/range_mutex_impl.hpp>
#include <hpx/synchronization/spinlock.hpp>

#include <cstddef>
#include <functional>
#include <mutex>
#include <system_error>
#include <utility>

namespace hpx::synchronization {
    using range_mutex = hpx::synchronization::detail::range_mutex<hpx::spinlock,
        std::lock_guard>;

    // Lock guards for range_mutex

    template <typename RangeMutex>
    class range_guard
    {
        std::size_t lock_id;
        std::reference_wrapper<RangeMutex> mutex_ref;

    public:
        range_guard(RangeMutex& mtx, std::size_t begin, std::size_t end)
          : lock_id(mtx.lock(begin, end))
          , mutex_ref(mtx)
        {
        }
        ~range_guard()
        {
            mutex_ref.get().unlock(lock_id);
        }

        range_guard(range_guard const&) = delete;
        range_guard& operator=(range_guard const&) = delete;

        range_guard(range_guard&& rhs_lock)
          : lock_id(rhs_lock.lock_id)
          , mutex_ref(rhs_lock.mutex_ref)
        {
            rhs_lock.lock_id = 0;
        }

        range_guard& operator=(range_guard&& rhs_lock)
        {
            mutex_ref.get().unlock(lock_id);
            mutex_ref = rhs_lock.mutex_ref;
            lock_id = rhs_lock.lock_id;
            rhs_lock.lock_id = 0;    // invalidating rhs_lock
            return *this;
        }
    };

    template <typename RangeMutex>
    class range_unique_lock
    {
        std::size_t lock_id;
        std::reference_wrapper<RangeMutex> mutex_ref;

    public:
        range_unique_lock(RangeMutex& mtx, std::size_t begin, std::size_t end)
          : lock_id(mtx.lock(begin, end))
          , mutex_ref(mtx)
        {
        }
        ~range_unique_lock()
        {
            mutex_ref.get().unlock(lock_id);
        }

        range_unique_lock(range_unique_lock const&) = delete;
        range_unique_lock& operator=(range_unique_lock const&) = delete;

        range_unique_lock(range_unique_lock&& rhs_lock)
          : mutex_ref(rhs_lock.mutex_ref)
          , lock_id(rhs_lock.lock_id)
        {
            rhs_lock.lock_id = 0;
        }

        range_unique_lock& operator=(range_unique_lock&& rhs_lock)
        {
            mutex_ref.get().unlock(lock_id);
            mutex_ref = rhs_lock.mutex_ref;
            lock_id = rhs_lock.lock_id;
            rhs_lock.lock_id = 0;    // invalidating rhs_lock
            return *this;
        }

        void lock(std::size_t begin, std::size_t end)
        {
            if (lock_id != 0)
            {
                std::error_code ec = std::make_error_code(
                    std::errc::resource_deadlock_would_occur);
                throw std::system_error(
                    ec, "range_unique_lock::lock: already locked");
            }
            lock_id = mutex_ref.get().lock(begin, end);
        }

        void try_lock(std::size_t begin, std::size_t end)
        {
            if (lock_id != 0)
            {
                std::error_code ec = std::make_error_code(
                    std::errc::resource_deadlock_would_occur);
                throw std::system_error(
                    ec, "range_unique_lock::lock: already locked");
            }
            lock_id = mutex_ref.get().try_lock(begin, end);
        }

        void unlock()
        {
            mutex_ref.get().unlock(lock_id);
            lock_id = 0;
        }

        void swap(range_unique_lock& uLock)
        {
            std::swap(mutex_ref, uLock.mutex_ref);
            std::swap(lock_id, uLock.lock_id);
        }

        RangeMutex* release()
        {
            RangeMutex* mtx = mutex_ref.get();
            lock_id = 0;
            return mtx;
        }

        operator bool() const
        {
            return lock_id != 0;
        }

        bool owns_lock() const
        {
            return lock_id != 0;
        }

        RangeMutex* mutex() const
        {
            return mutex_ref.get();
        }
    };
}    // namespace hpx::synchronization
