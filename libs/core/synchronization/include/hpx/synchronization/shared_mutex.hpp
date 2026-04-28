//  (C) Copyright 2006-2008 Anthony Williams
//  (C) Copyright      2011 Bryce Lelbach
//  (C) Copyright 2022-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file shared_mutex.hpp
/// \page hpx::shared_mutex
/// \headerfile hpx/shared_mutex.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/lock_registration.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/synchronization/detail/condition_variable.hpp>
#include <hpx/synchronization/spinlock.hpp>

#include <atomic>
#include <cstdint>
#include <mutex>

namespace hpx::detail {

    ///////////////////////////////////////////////////////////////////////////
    template <typename Mutex>
    struct shared_mutex_data
    {
        using mutex_type = Mutex;

        HPX_HOST_DEVICE_CONSTEXPR shared_mutex_data() noexcept
          : count_(1)
        {
        }

        struct state_data
        {
            std::uint32_t shared_count;
            std::uint8_t tag;    // ABA protection
            bool exclusive;
            bool upgrade;
            bool exclusive_waiting_blocked;
        };

        struct shared_state
        {
            union
            {
                std::uint64_t value = 0;
                state_data data;
            };

            shared_state() = default;
        };

        util::cache_aligned_data_derived<std::atomic<shared_state>> state;

        using condition_variable = lcos::local::detail::condition_variable;

        util::cache_aligned_data_derived<mutex_type> state_change;
        util::cache_aligned_data_derived<condition_variable> shared_cond;
        util::cache_aligned_data_derived<condition_variable> exclusive_cond;
        util::cache_aligned_data_derived<condition_variable> upgrade_cond;

        void release_waiters(std::unique_lock<mutex_type>& lk)
        {
            [[maybe_unused]] util::ignore_while_checking il(&lk);
            exclusive_cond.notify_one_no_unlock(lk);
            shared_cond.notify_all(HPX_MOVE(lk));
            il.reset_owns_registration();
        }

        bool set_state(shared_state& s1, shared_state& s) noexcept
        {
            ++s.data.tag;
            return state.compare_exchange_strong(
                s1, s, std::memory_order_release, std::memory_order_relaxed);
        }

        bool set_state(shared_state& s1, shared_state& s,
            std::unique_lock<mutex_type>& lk) noexcept
        {
            ++s.data.tag;

            lk = std::unique_lock<mutex_type>(state_change);
            if (state.compare_exchange_strong(
                    s1, s, std::memory_order_release, std::memory_order_relaxed))
                return true;

            lk.unlock();
            return false;
        }

        void lock_shared()
        {
            while (true)
            {
                std::unique_lock<mutex_type> lk(state_change);
                auto s = state.load(std::memory_order_acquire);
                while (s.data.exclusive || s.data.exclusive_waiting_blocked)
                {
                    shared_cond.wait(lk);
                    s = state.load(std::memory_order_acquire);
                }

                auto s1 = s;

                ++s.data.shared_count;
                if (set_state(s1, s))
                {
                    break;
                }
            }
        }

        bool try_lock_shared()
        {
            auto s = state.load(std::memory_order_acquire);
            while (true)
            {
                if (s.data.exclusive || s.data.exclusive_waiting_blocked)
                {
                    return false;
                }

                auto s1 = s;

                ++s.data.shared_count;
                if (set_state(s1, s))
                {
                    break;
                }
                s = s1;
            }
            return true;
        }

        bool try_unlock_shared_fast()
        {
            auto s = state.load(std::memory_order_acquire);
            while (true)
            {
                if (s.data.exclusive || s.data.exclusive_waiting_blocked ||
                    s.data.upgrade || s.data.shared_count <= 1)
                {
                    return false;
                }

                auto s1 = s;
                --s.data.shared_count;
                if (set_state(s1, s))
                {
                    return true;
                }
                s = s1;
            }
        }

        void unlock_shared()
        {
            auto s = state.load(std::memory_order_acquire);
            while (true)
            {
                auto s1 = s;

                if (--s.data.shared_count == 0)
                {
                    if (s.data.upgrade)
                    {
                        s.data.upgrade = false;
                        s.data.exclusive = true;

                        std::unique_lock<mutex_type> lk;
                        if (set_state(s1, s, lk))
                        {
                            HPX_ASSERT_OWNS_LOCK(lk);
                            {
                                [[maybe_unused]] hpx::util::
                                    ignore_while_checking il(&lk);
                                upgrade_cond.notify_one_no_unlock(lk);
                            }
                            release_waiters(lk);
                            break;
                        }
                    }
                    else
                    {
                        s.data.exclusive_waiting_blocked = false;

                        std::unique_lock<mutex_type> lk;
                        if (set_state(s1, s, lk))
                        {
                            HPX_ASSERT_OWNS_LOCK(lk);
                            release_waiters(lk);
                            break;
                        }
                    }
                }
                else if (set_state(s1, s))
                {
                    break;
                }
                s = s1;
            }
        }

        void lock()
        {
            auto s = state.load(std::memory_order_acquire);
            while (true)
            {
                while (s.data.shared_count != 0 || s.data.exclusive)
                {
                    auto s1 = s;

                    s.data.exclusive_waiting_blocked = true;
                    std::unique_lock<mutex_type> lk;
                    if (set_state(s1, s, lk))
                    {
                        HPX_ASSERT_OWNS_LOCK(lk);
                        exclusive_cond.wait(lk);
                    }

                    s = state.load(std::memory_order_acquire);
                }

                auto s1 = s;

                s.data.exclusive = true;
                if (set_state(s1, s))
                {
                    break;
                }
                s = s1;
            }
        }

        bool try_lock()
        {
            auto s = state.load(std::memory_order_acquire);
            while (true)
            {
                if (s.data.shared_count || s.data.exclusive)
                {
                    return false;
                }

                auto s1 = s;

                s.data.exclusive = true;
                if (set_state(s1, s))
                {
                    break;
                }
                s = s1;
            }
            return true;
        }

        void unlock()
        {
            auto s = state.load(std::memory_order_acquire);
            while (true)
            {
                auto s1 = s;

                s.data.exclusive = false;
                s.data.exclusive_waiting_blocked = false;

                std::unique_lock<mutex_type> lk;
                if (set_state(s1, s, lk))
                {
                    HPX_ASSERT_OWNS_LOCK(lk);
                    release_waiters(lk);
                    break;
                }
                s = s1;
            }
        }

        void lock_upgrade()
        {
            while (true)
            {
                std::unique_lock<mutex_type> lk(state_change);
                auto s = state.load(std::memory_order_acquire);
                while (s.data.exclusive || s.data.exclusive_waiting_blocked ||
                    s.data.upgrade)
                {
                    {
                        shared_cond.wait(lk);
                    }

                    s = state.load(std::memory_order_acquire);
                }

                auto s1 = s;

                ++s.data.shared_count;
                s.data.upgrade = true;
                if (set_state(s1, s))
                {
                    break;
                }
            }
        }

        bool try_lock_upgrade()
        {
            auto s = state.load(std::memory_order_acquire);
            while (true)
            {
                if (s.data.exclusive || s.data.exclusive_waiting_blocked ||
                    s.data.upgrade)
                {
                    return false;
                }

                auto s1 = s;

                ++s.data.shared_count;
                s.data.upgrade = true;
                if (set_state(s1, s))
                {
                    break;
                }
                s = s1;
            }
            return true;
        }

        void unlock_upgrade()
        {
            auto s = state.load(std::memory_order_acquire);
            while (true)
            {
                auto s1 = s;

                bool release = false;
                s.data.upgrade = false;
                if (--s.data.shared_count == 0)
                {
                    s.data.exclusive_waiting_blocked = false;
                    release = true;
                }

                if (release)
                {
                    std::unique_lock<mutex_type> lk;
                    if (set_state(s1, s, lk))
                    {
                        HPX_ASSERT_OWNS_LOCK(lk);
                        release_waiters(lk);
                        break;
                    }
                }
                else if (set_state(s1, s))
                {
                    break;
                }
                s = s1;
            }
        }

        void unlock_upgrade_and_lock()
        {
            // Flag to ensure the thread releases shared lock exactly once
            // even if the upgrade to exclusive fails and it retries the loop
            bool shared_released = false;

            while (true)
            {
                std::unique_lock<mutex_type> lk(state_change);
                auto s = state.load(std::memory_order_acquire);
                if (!shared_released)
                {
                    auto s1 = s;

                    --s.data.shared_count;
                    if (!set_state(s1, s))
                    {
                        continue;
                    }
                    shared_released = true;
                }

                s = state.load(std::memory_order_acquire);
                while (s.data.shared_count != 0)
                {
                    {
                        upgrade_cond.wait(lk);
                    }
                    s = state.load(std::memory_order_acquire);
                }

                auto s1 = s;

                s.data.upgrade = false;
                s.data.exclusive = true;
                if (set_state(s1, s))
                {
                    break;
                }
            }
        }

        void unlock_and_lock_upgrade()
        {
            auto s = state.load(std::memory_order_acquire);
            while (true)
            {
                auto s1 = s;

                s.data.exclusive = false;
                s.data.exclusive_waiting_blocked = false;
                s.data.upgrade = true;
                ++s.data.shared_count;

                std::unique_lock<mutex_type> lk;
                if (set_state(s1, s, lk))
                {
                    HPX_ASSERT_OWNS_LOCK(lk);
                    release_waiters(lk);
                    break;
                }
                s = s1;
            }
        }

        void unlock_and_lock_shared()
        {
            auto s = state.load(std::memory_order_acquire);
            while (true)
            {
                auto s1 = s;

                s.data.exclusive = false;
                s.data.exclusive_waiting_blocked = false;
                ++s.data.shared_count;

                std::unique_lock<mutex_type> lk;
                if (set_state(s1, s, lk))
                {
                    HPX_ASSERT_OWNS_LOCK(lk);
                    release_waiters(lk);
                    break;
                }
                s = s1;
            }
        }

        bool try_unlock_shared_and_lock()
        {
            auto s = state.load(std::memory_order_acquire);
            while (true)
            {
                if (s.data.exclusive || s.data.exclusive_waiting_blocked ||
                    s.data.upgrade || s.data.shared_count != 1)
                {
                    return false;
                }

                auto s1 = s;

                s.data.shared_count = 0;
                s.data.exclusive = true;
                if (set_state(s1, s))
                {
                    break;
                }
                s = s1;
            }
            return true;
        }

        void unlock_upgrade_and_lock_shared()
        {
            auto s = state.load(std::memory_order_acquire);
            while (true)
            {
                auto s1 = s;

                s.data.exclusive_waiting_blocked = false;
                s.data.upgrade = false;

                std::unique_lock<mutex_type> lk;
                if (set_state(s1, s, lk))
                {
                    HPX_ASSERT_OWNS_LOCK(lk);
                    release_waiters(lk);
                    break;
                }
                s = s1;
            }
        }

    private:
        friend void intrusive_ptr_add_ref(shared_mutex_data* p) noexcept
        {
            p->count_.increment();
        }

        friend void intrusive_ptr_release(shared_mutex_data* p) noexcept
        {
            if (0 == p->count_.decrement())
            {
                // The thread that decrements the reference count to zero must
                // perform an acquire to ensure that it doesn't start destructing
                // the object until all previous writes have drained.
                std::atomic_thread_fence(std::memory_order_acquire);

                delete p;
            }
        }

        hpx::util::atomic_count count_;
    };

    HPX_CXX_CORE_EXPORT template <typename Mutex = hpx::spinlock>
    class shared_mutex
    {
    private:
        using mutex_type = Mutex;

        using data_type = hpx::intrusive_ptr<shared_mutex_data<Mutex>>;
        hpx::util::cache_aligned_data_derived<data_type> data_;

        using shared_state = typename shared_mutex_data<Mutex>::shared_state;

    public:
        shared_mutex()
          : data_(new shared_mutex_data<Mutex>, false)
        {
        }

        void lock_shared()
        {
            auto data = data_;
            if (data->try_lock_shared())
                return;
            data->lock_shared();
        }

        bool try_lock_shared()
        {
            auto data = data_;
            return data->try_lock_shared();
        }

        void unlock_shared()
        {
            auto data = data_;
            if (data->try_unlock_shared_fast())
                return;
            data->unlock_shared();
        }

        void lock()
        {
            auto data = data_;
            data->lock();
        }

        bool try_lock()
        {
            auto data = data_;
            return data->try_lock();
        }

        void unlock()
        {
            auto data = data_;
            data->unlock();
        }

        void lock_upgrade()
        {
            auto data = data_;
            data->lock_upgrade();
        }

        bool try_lock_upgrade()
        {
            auto data = data_;
            return data->try_lock_upgrade();
        }

        void unlock_upgrade()
        {
            auto data = data_;
            data->unlock_upgrade();
        }

        void unlock_upgrade_and_lock()
        {
            auto data = data_;
            data->unlock_upgrade_and_lock();
        }

        void unlock_and_lock_upgrade()
        {
            auto data = data_;
            data->unlock_and_lock_upgrade();
        }

        void unlock_and_lock_shared()
        {
            auto data = data_;
            data->unlock_and_lock_shared();
        }

        bool try_unlock_shared_and_lock()
        {
            auto data = data_;
            return data->try_unlock_shared_and_lock();
        }

        void unlock_upgrade_and_lock_shared()
        {
            auto data = data_;
            data->unlock_upgrade_and_lock_shared();
        }
    };
}    // namespace hpx::detail

namespace hpx {

    /// The \a shared_mutex class is a synchronization primitive that can be
    /// used to protect shared data from being simultaneously accessed by
    /// multiple threads. In contrast to other mutex types which facilitate
    /// exclusive access, a \a shared_mutex has two levels of access:
    ///   - \a shared - several threads can share ownership of the same
    ///            mutex.
    ///   - \a exclusive - only one thread can own the mutex.
    ///
    /// \details If one thread has acquired the exclusive lock (through \a lock,
    ///          \a try_lock), no other threads can acquire the lock (including
    ///          the shared). If one thread has acquired the shared lock
    ///          (through \a lock_shared, \a try_lock_shared), no other thread
    ///          can acquire the exclusive lock, but can acquire the shared
    ///          lock. Only when the exclusive lock has not been acquired by any
    ///          thread, the shared lock can be acquired by multiple threads.
    ///          Within one thread, only one lock (shared or exclusive) can be
    ///          acquired at the same time. Shared mutexes are especially useful
    ///          when shared data can be safely read by any number of threads
    ///          simultaneously, but a thread may only write the same data when
    ///          no other thread is reading or writing at the same time. The \a
    ///          shared_mutex class satisfies all requirements of \a SharedMutex
    ///          and \a StandardLayoutType.
    HPX_CXX_CORE_EXPORT using shared_mutex = detail::shared_mutex<>;
}    // namespace hpx
