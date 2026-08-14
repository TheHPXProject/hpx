//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c) 2013-2015 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/synchronization/spinlock.hpp>

#include <cstddef>
#include <mutex>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::lcos::local::detail {

    HPX_CXX_CORE_EXPORT class condition_variable
    {
    private:
        using mutex_type = hpx::spinlock;

    private:
        // define data structures needed for intrusive slist container used for
        // the queues
        struct queue_entry;
        struct reset_queue_entry;

        using queue_type = hpx::detail::intrusive_list<queue_entry>;

    public:
        constexpr condition_variable() noexcept = default;

        // non-copyable
        condition_variable(condition_variable const&) = delete;
        condition_variable(condition_variable&&) = delete;
        condition_variable& operator=(condition_variable const&) = delete;
        condition_variable& operator=(condition_variable&&) = delete;

        HPX_CORE_EXPORT ~condition_variable();

        HPX_CORE_EXPORT bool empty(
            std::unique_lock<mutex_type>& lock) const noexcept;

        HPX_CORE_EXPORT std::size_t size(
            std::unique_lock<mutex_type>& lock) const noexcept;

        // Return false if no more threads are waiting (returns true if queue is
        // non-empty).
        HPX_CORE_EXPORT bool notify_one(std::unique_lock<mutex_type>& lock,
            threads::thread_priority priority, bool unlock,
            error_code& ec = throws);

        HPX_CORE_EXPORT void notify_all(std::unique_lock<mutex_type>& lock,
            threads::thread_priority priority, bool unlock,
            error_code& ec = throws);

        bool notify_one(std::unique_lock<mutex_type> lock,
            threads::thread_priority priority, error_code& ec = throws)
        {
            return notify_one(lock, priority, true, ec);
        }

        bool notify_one(
            std::unique_lock<mutex_type> lock, error_code& ec = throws)
        {
            return notify_one(
                lock, threads::thread_priority::default_, true, ec);
        }

        bool notify_one_no_unlock(
            std::unique_lock<mutex_type>& lock, error_code& ec = throws)
        {
            return notify_one(
                lock, threads::thread_priority::default_, false, ec);
        }

        void notify_all(
            std::unique_lock<mutex_type> lock, error_code& ec = throws)
        {
            return notify_all(
                lock, threads::thread_priority::default_, true, ec);
        }

        void notify_all_no_unlock(
            std::unique_lock<mutex_type>& lock, error_code& ec = throws)
        {
            return notify_all(
                lock, threads::thread_priority::default_, false, ec);
        }

        HPX_CORE_EXPORT void abort_all(std::unique_lock<mutex_type> lock);

        HPX_CORE_EXPORT threads::thread_restart_state wait(
            std::unique_lock<mutex_type>& lock, char const* description,
            error_code& ec = throws);

        threads::thread_restart_state wait(
            std::unique_lock<mutex_type>& lock, error_code& ec = throws)
        {
            return wait(lock, "condition_variable::wait", ec);
        }

        HPX_CORE_EXPORT threads::thread_restart_state wait_until(
            std::unique_lock<mutex_type>& lock,
            hpx::chrono::steady_time_point const& abs_time,
            char const* description, error_code& ec = throws);

        HPX_CORE_EXPORT threads::thread_restart_state wait_until(
            std::unique_lock<mutex_type>& lock,
            hpx::chrono::steady_time_point const& abs_time,
            hpx::move_only_function<bool()>&& wait_cond,
            char const* description, error_code& ec = throws);

        threads::thread_restart_state wait_until(
            std::unique_lock<mutex_type>& lock,
            hpx::chrono::steady_time_point const& abs_time,
            hpx::move_only_function<bool()>&& wait_cond,
            error_code& ec = throws)
        {
            return wait_until(lock, abs_time, HPX_MOVE(wait_cond),
                "condition_variable::wait_until", ec);
        }

        threads::thread_restart_state wait_until(
            std::unique_lock<mutex_type>& lock,
            hpx::chrono::steady_time_point const& abs_time,
            error_code& ec = throws)
        {
            return wait_until(
                lock, abs_time, "condition_variable::wait_until", ec);
        }

        threads::thread_restart_state wait_for(
            std::unique_lock<mutex_type>& lock,
            hpx::chrono::steady_duration const& rel_time,
            char const* description, error_code& ec = throws)
        {
            return wait_until(lock, rel_time.from_now(), description, ec);
        }

        threads::thread_restart_state wait_for(
            std::unique_lock<mutex_type>& lock,
            hpx::chrono::steady_duration const& rel_time,
            error_code& ec = throws)
        {
            return wait_until(
                lock, rel_time.from_now(), "condition_variable::wait_for", ec);
        }

    private:
        template <typename Mutex>
        void abort_all(std::unique_lock<Mutex> lock);

        // re-add the remaining items to the original queue
        HPX_CORE_EXPORT void prepend_entries(
            std::unique_lock<mutex_type>& lock, queue_type& queue) noexcept;

    private:
        queue_type queue_;
    };

    ///////////////////////////////////////////////////////////////////////////
    struct condition_variable_data;

    HPX_CORE_EXPORT void intrusive_ptr_add_ref(
        condition_variable_data* p) noexcept;
    HPX_CORE_EXPORT void intrusive_ptr_release(
        condition_variable_data* p) noexcept;

    struct condition_variable_data
    {
        using mutex_type = hpx::spinlock;

        HPX_HOST_DEVICE_CONSTEXPR condition_variable_data() noexcept
          : count_(1)
        {
        }

        util::cache_aligned_data_derived<mutex_type> mtx_;
        util::cache_aligned_data_derived<detail::condition_variable> cond_;

    private:
        friend HPX_CORE_EXPORT void intrusive_ptr_add_ref(
            condition_variable_data*) noexcept;
        friend HPX_CORE_EXPORT void intrusive_ptr_release(
            condition_variable_data*) noexcept;

        hpx::util::atomic_count count_;
    };
}    // namespace hpx::lcos::local::detail
