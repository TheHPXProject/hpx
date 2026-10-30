//  Copyright (c) 2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/synchronization/mutex.hpp>
#include <hpx/synchronization/stop_token.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>

namespace hpx::detail {

    ///////////////////////////////////////////////////////////////////////////
    void intrusive_ptr_add_ref(stop_state* p) noexcept
    {
        p->state_.fetch_add(
            stop_state::token_ref_increment, std::memory_order_relaxed);
    }

    void intrusive_ptr_release(stop_state* p) noexcept
    {
        std::uint64_t const old_state = p->state_.fetch_sub(
            stop_state::token_ref_increment, std::memory_order_release);

        if ((old_state & stop_state::token_ref_mask) ==
            stop_state::token_ref_increment)
        {
            // The thread that decrements the reference count to zero must
            // perform an acquire to ensure that it doesn't start destructing
            // the object until all previous writes have drained.
            std::atomic_thread_fence(std::memory_order_acquire);

            delete p;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    void stop_callback_base::add_this_callback(
        stop_callback_base*& callbacks) noexcept
    {
        next_ = callbacks;
        if (next_ != nullptr)
        {
            next_->prev_ = &next_;
        }
        prev_ = &callbacks;
        callbacks = this;
    }

    // returns true if the callback was successfully removed
    bool stop_callback_base::remove_this_callback() const noexcept
    {
        if (prev_ != nullptr)
        {
            // Still registered, not yet executed: just remove from the list.
            *prev_ = next_;
            if (next_ != nullptr)
            {
                next_->prev_ = prev_;
            }
            return true;
        }
        return false;
    }

    ///////////////////////////////////////////////////////////////////////////
    void stop_state::lock() noexcept
    {
        auto old_state = state_.load(std::memory_order_relaxed);
        do
        {
            for (std::size_t k = 0; is_locked(old_state); ++k)
            {
                hpx::execution_base::this_thread::yield_k(
                    k, "stop_state::lock");
                old_state = state_.load(std::memory_order_relaxed);
            }
        } while (!state_.compare_exchange_weak(old_state,
            old_state | stop_state::locked_flag, std::memory_order_acquire,
            std::memory_order_relaxed));
    }

    ///////////////////////////////////////////////////////////////////////////
    bool stop_state::lock_and_request_stop() noexcept
    {
        std::uint64_t old_state = state_.load(std::memory_order_acquire);

        if (stop_requested(old_state))
            return false;

        do
        {
            for (std::size_t k = 0; is_locked(old_state); ++k)
            {
                hpx::execution_base::this_thread::yield_k(
                    k, "stop_state::lock_and_request_stop");
                old_state = state_.load(std::memory_order_acquire);

                if (stop_requested(old_state))
                    return false;
            }
        } while (!state_.compare_exchange_weak(old_state,
            old_state | stop_state::stop_requested_flag |
                stop_state::locked_flag,
            std::memory_order_acquire, std::memory_order_relaxed));
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    bool stop_state::lock_if_not_stopped(stop_callback_base* cb) noexcept
    {
        std::uint64_t old_state = state_.load(std::memory_order_acquire);

        if (stop_requested(old_state))
        {
            cb->execute();
            cb->callback_finished_executing_.store(
                true, std::memory_order_release);
            return false;
        }
        else if (!stop_possible(old_state))
        {
            return false;
        }

        do
        {
            for (std::size_t k = 0; is_locked(old_state); ++k)
            {
                hpx::execution_base::this_thread::yield_k(
                    k, "stop_state::add_callback");
                old_state = state_.load(std::memory_order_acquire);

                if (stop_requested(old_state))
                {
                    cb->execute();
                    cb->callback_finished_executing_.store(
                        true, std::memory_order_release);
                    return false;
                }
                else if (!stop_possible(old_state))
                {
                    return false;
                }
            }
        } while (!state_.compare_exchange_weak(old_state,
            old_state | stop_state::locked_flag, std::memory_order_acquire,
            std::memory_order_relaxed));

        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    struct scoped_lock_if_not_stopped
    {
        scoped_lock_if_not_stopped(
            stop_state& state, stop_callback_base* cb) noexcept
          : state_(state)
          , has_lock_(state_.lock_if_not_stopped(cb))
        {
        }
        ~scoped_lock_if_not_stopped()
        {
            if (has_lock_)
                state_.unlock();
        }

        explicit operator bool() const noexcept
        {
            return has_lock_;
        }

        stop_state& state_;
        bool has_lock_;
    };

    bool stop_state::add_callback(stop_callback_base* cb) noexcept
    {
        scoped_lock_if_not_stopped const l(*this, cb);
        if (!l)
            return false;

        // Push callback onto callback list
        cb->add_this_callback(callbacks_);
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    void stop_state::remove_callback(stop_callback_base const* cb) noexcept
    {
        {
            std::lock_guard<stop_state> l(*this);
            if (cb->remove_this_callback())
                return;
        }

        // Callback has either already executed or is executing concurrently
        // on another thread.
        if (signalling_thread_ == hpx::threads::get_self_id())
        {
            // Callback executed on this thread or is still currently executing
            // and is unregistering itself from within the callback.
            if (cb->is_removed_ != nullptr)
            {
                // Currently inside the callback, let the request_stop() method
                // know the object is about to be destructed and that it should
                // not try to access the object when the callback returns.
                *cb->is_removed_ = true;
            }
        }
        else
        {
            // Callback is currently executing on another thread,
            // block until it finishes executing.
            for (std::size_t k = 0; !cb->callback_finished_executing_.load(
                     std::memory_order_acquire);
                ++k)
            {
                hpx::execution_base::this_thread::yield_k(
                    k, "stop_state::remove_callback");
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    struct scoped_lock_and_request_stop
    {
        explicit scoped_lock_and_request_stop(stop_state& state) noexcept
          : state_(state)
          , has_lock_(state_.lock_and_request_stop())
        {
        }
        ~scoped_lock_and_request_stop()
        {
            if (has_lock_)
                state_.unlock();
        }

        scoped_lock_and_request_stop(
            scoped_lock_and_request_stop const&) = delete;
        scoped_lock_and_request_stop(scoped_lock_and_request_stop&&) = delete;
        scoped_lock_and_request_stop& operator=(
            scoped_lock_and_request_stop const&) = delete;
        scoped_lock_and_request_stop& operator=(
            scoped_lock_and_request_stop&&) = delete;

        explicit operator bool() const noexcept
        {
            return has_lock_;
        }

        void unlock() const noexcept
        {
            state_.unlock();
        }

    private:
        stop_state& state_;
        bool has_lock_;
    };

    bool stop_state::request_stop() noexcept
    {
        // Set the 'stop_requested' signal and acquired the lock.
        scoped_lock_and_request_stop const l(*this);
        if (!l)
            return false;    // stop has already been requested.

        HPX_ASSERT_LOCKED(
            l, stop_requested(state_.load(std::memory_order_acquire)));

        signalling_thread_ = hpx::threads::get_self_id();

        // invoke registered callbacks
        while (callbacks_ != nullptr)
        {
            // Dequeue the head of the queue
            auto* cb = callbacks_;
            callbacks_ = cb->next_;

            if (callbacks_ != nullptr)
                callbacks_->prev_ = &callbacks_;

            // Mark this item as removed from the list.
            cb->prev_ = nullptr;

            bool is_removed = false;
            cb->is_removed_ = &is_removed;    //-V506

            {
                // Don't hold lock while executing callback so we don't
                // block other threads from unregistering callbacks.
                unlock_guard ul(*this);
                cb->execute();
            }

            if (!is_removed)
            {
                cb->is_removed_ = nullptr;
                cb->callback_finished_executing_.store(
                    true, std::memory_order_release);
            }
        }

        return true;
    }
}    // namespace hpx::detail
