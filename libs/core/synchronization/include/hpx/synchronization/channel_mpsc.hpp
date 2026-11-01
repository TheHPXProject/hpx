//  Copyright (c) 2019-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  This work is inspired by https://github.com/aprell/tasking-2.0

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/synchronization/channel_spsc.hpp>
#include <hpx/synchronization/spinlock.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <utility>

namespace hpx::lcos::local {

    ////////////////////////////////////////////////////////////////////////////
    // A simple but very high performance implementation of the channel concept.
    // This channel is bounded to a size given at construction time and supports
    // a multiple producers and a single consumer. The data is stored in a
    // ring-buffer.
    HPX_CXX_CORE_EXPORT template <typename T,
        typename Mutex = hpx::util::spinlock,
        channel_mode = channel_mode::normal>
    class base_channel_mpsc
    {
    private:
        using mutex_type = Mutex;

        [[nodiscard]] HPX_FORCEINLINE bool is_full(
            std::size_t tail) const noexcept
        {
            std::size_t next_tail = tail + 1;
            if (next_tail >= size_)
            {
                next_tail = 0;
            }
            return next_tail == head_.data_.load(std::memory_order_acquire);
        }

        [[nodiscard]] HPX_FORCEINLINE bool is_empty(
            std::size_t head) const noexcept
        {
            if (head == tail_cached_.data_) [[unlikely]]
            {
                tail_cached_.data_ =
                    tail_.data_.tail_.load(std::memory_order_acquire);
                if (head == tail_cached_.data_)
                {
                    return true;
                }
            }
            return false;
        }

    public:
        explicit base_channel_mpsc(std::size_t size)
          : size_(size + 1)
          , buffer_(new T[size + 1])
          , closed_(false)
        {
            HPX_ASSERT(size != 0);

            head_.data_.store(0, std::memory_order_relaxed);
            tail_.data_.tail_.store(0, std::memory_order_relaxed);
            tail_cached_.data_ = 0;
        }

        base_channel_mpsc(base_channel_mpsc const& rhs) = delete;
        base_channel_mpsc& operator=(base_channel_mpsc const& rhs) = delete;

        base_channel_mpsc(base_channel_mpsc&& rhs) noexcept
          : size_(rhs.size_)
          , buffer_(HPX_MOVE(rhs.buffer_))
        {
            head_.data_.store(rhs.head_.data_.load(std::memory_order_acquire),
                std::memory_order_relaxed);
            tail_.data_.tail_.store(
                rhs.tail_.data_.tail_.load(std::memory_order_acquire),
                std::memory_order_relaxed);

            tail_cached_.data_ = rhs.tail_cached_.data_;

            closed_.store(rhs.closed_.load(std::memory_order_acquire),
                std::memory_order_relaxed);
            rhs.closed_.store(true, std::memory_order_release);
        }

        base_channel_mpsc& operator=(base_channel_mpsc&& rhs) noexcept
        {
            head_.data_.store(rhs.head_.data_.load(std::memory_order_acquire),
                std::memory_order_relaxed);
            tail_.data_.tail_.store(
                rhs.tail_.data_.tail_.load(std::memory_order_acquire),
                std::memory_order_relaxed);

            tail_cached_.data_ = rhs.tail_cached_.data_;

            size_ = rhs.size_;
            buffer_ = HPX_MOVE(rhs.buffer_);

            closed_.store(rhs.closed_.load(std::memory_order_acquire),
                std::memory_order_relaxed);
            rhs.closed_.store(true, std::memory_order_release);

            return *this;
        }

        ~base_channel_mpsc()
        {
            if (!closed_.load(std::memory_order_relaxed))
            {
                close();
            }
        }

        [[nodiscard]] bool is_empty() const noexcept
        {
            if (closed_.load(std::memory_order_relaxed))
            {
                return true;
            }
            return is_empty(head_.data_.load(std::memory_order_relaxed));
        }

        bool get(T* val = nullptr) const noexcept
        {
            if (closed_.load(std::memory_order_relaxed))
            {
                return false;
            }

            std::size_t head = head_.data_.load(std::memory_order_relaxed);

            if (is_empty(head))
            {
                return false;
            }

            if (val == nullptr)
            {
                return true;
            }

            *val = HPX_MOVE(buffer_[head]);
            if (++head >= size_)
            {
                head = 0;
            }
            head_.data_.store(head, std::memory_order_release);

            return true;
        }

        // clang-format off
        bool set(T&& t) noexcept(
            noexcept(std::declval<std::unique_lock<mutex_type>&>().lock()) &&
            noexcept(std::declval<std::unique_lock<mutex_type>&>().unlock()))
        // clang-format on
        {
            if (closed_.load(std::memory_order_relaxed))
            {
                return false;
            }

            std::unique_lock<mutex_type> l(tail_.data_.mtx_);

            std::size_t tail =
                tail_.data_.tail_.load(std::memory_order_acquire);

            if (is_full(tail))
            {
                return false;
            }

            buffer_[tail] = HPX_MOVE(t);
            if (++tail >= size_)
            {
                tail = 0;
            }
            tail_.data_.tail_.store(tail, std::memory_order_release);

            return true;
        }

        std::size_t close()
        {
            bool expected = false;
            if (!closed_.compare_exchange_weak(expected, true))
            {
                HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                    "hpx::lcos::local::base_channel_mpsc::close",
                    "attempting to close an already closed channel");
            }
            return 0;
        }

        [[nodiscard]] constexpr std::size_t capacity() const noexcept
        {
            return size_ - 1;
        }

    private:
        struct tail_data
        {
            mutex_type mtx_;
            std::atomic<std::size_t> tail_;
        };

        // keep the head, tail cache, and the mutex with the tail pointer
        // each on separate cache lines
        mutable hpx::util::cache_aligned_data<std::atomic<std::size_t>> head_;
        mutable hpx::util::cache_aligned_data<std::size_t> tail_cached_;
        hpx::util::cache_aligned_data<tail_data> tail_;

        // a channel of size n can buffer n-1 items
        std::size_t size_;

        // channel buffer
        std::unique_ptr<T[]> buffer_;

        // this channel was closed, i.e. no further operations are possible
        std::atomic<bool> closed_;
    };

    template <typename T, typename Mutex>
    class base_channel_mpsc<T, Mutex, channel_mode::dont_support_close>
    {
        static_assert(std::is_nothrow_move_assignable_v<T>,
            "base_channel_mpsc in dont_support_close mode requires nothrow "
            "move-assignable types");

    private:
        struct cell_type
        {
            std::atomic<std::size_t> sequence_;
            T data_;
        };

        static constexpr std::size_t round_up_power_of_two(
            std::size_t v) noexcept
        {
            if (v == 0)
            {
                return 1;
            }
            --v;
            v |= v >> 1;
            v |= v >> 2;
            v |= v >> 4;
            v |= v >> 8;
            v |= v >> 16;
            v |= v >> 32;
            return ++v;
        }

    public:
        explicit base_channel_mpsc(std::size_t size)
        {
            HPX_ASSERT(size != 0);
            std::size_t const capacity =
                (std::max) (std::size_t(8), round_up_power_of_two(size + 1));
            buffer_mask_ = capacity - 1;
            buffer_ = std::make_unique<cell_type[]>(capacity);
            for (std::size_t i = 0; i != capacity; ++i)
            {
                buffer_[i].sequence_.store(i, std::memory_order_relaxed);
            }
            head_.data_.store(0, std::memory_order_relaxed);
            tail_.data_.store(0, std::memory_order_relaxed);
        }

        base_channel_mpsc(base_channel_mpsc const& rhs) = delete;
        base_channel_mpsc& operator=(base_channel_mpsc const& rhs) = delete;

        base_channel_mpsc(base_channel_mpsc&& rhs) noexcept
          : buffer_mask_(rhs.buffer_mask_)
          , buffer_(HPX_MOVE(rhs.buffer_))
        {
            head_.data_.store(rhs.head_.data_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
            tail_.data_.store(rhs.tail_.data_.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
        }

        base_channel_mpsc& operator=(base_channel_mpsc&& rhs) noexcept
        {
            if (this != &rhs)
            {
                buffer_mask_ = rhs.buffer_mask_;
                buffer_ = HPX_MOVE(rhs.buffer_);
                head_.data_.store(
                    rhs.head_.data_.load(std::memory_order_relaxed),
                    std::memory_order_relaxed);
                tail_.data_.store(
                    rhs.tail_.data_.load(std::memory_order_relaxed),
                    std::memory_order_relaxed);
            }
            return *this;
        }

        ~base_channel_mpsc() = default;

        [[nodiscard]] bool is_empty() const noexcept
        {
            std::size_t const pos = head_.data_.load(std::memory_order_relaxed);
            cell_type const& cell = buffer_[pos & buffer_mask_];
            std::size_t const seq =
                cell.sequence_.load(std::memory_order_acquire);
            return static_cast<std::intptr_t>(seq) -
                static_cast<std::intptr_t>(pos + 1) !=
                0;
        }

        bool get(T* val = nullptr) const noexcept
        {
            std::size_t const pos = head_.data_.load(std::memory_order_relaxed);
            cell_type& cell = buffer_[pos & buffer_mask_];
            std::size_t const seq =
                cell.sequence_.load(std::memory_order_acquire);
            std::intptr_t const dif = static_cast<std::intptr_t>(seq) -
                static_cast<std::intptr_t>(pos + 1);

            if (dif == 0)
            {
                if (val == nullptr)
                {
                    return true;
                }

                head_.data_.store(pos + 1, std::memory_order_relaxed);
                *val = HPX_MOVE(cell.data_);
                cell.sequence_.store(
                    pos + buffer_mask_ + 1, std::memory_order_release);
                return true;
            }
            return false;
        }

        bool set(T&& t) noexcept
        {
            std::size_t pos = tail_.data_.load(std::memory_order_relaxed);
            for (;;)
            {
                cell_type& cell = buffer_[pos & buffer_mask_];
                std::size_t const seq =
                    cell.sequence_.load(std::memory_order_acquire);
                std::intptr_t const dif = static_cast<std::intptr_t>(seq) -
                    static_cast<std::intptr_t>(pos);

                if (dif == 0)
                {
                    if (tail_.data_.compare_exchange_weak(
                            pos, pos + 1, std::memory_order_relaxed))
                    {
                        cell.data_ = HPX_MOVE(t);
                        cell.sequence_.store(
                            pos + 1, std::memory_order_release);
                        return true;
                    }
                }
                else if (dif < 0)
                {
                    return false;
                }
                else
                {
                    pos = tail_.data_.load(std::memory_order_relaxed);
                }
            }
        }

        [[nodiscard]] constexpr std::size_t capacity() const noexcept
        {
            return buffer_mask_ + 1;
        }

    private:
        mutable hpx::util::cache_aligned_data<std::atomic<std::size_t>> head_;
        mutable hpx::util::cache_aligned_data<std::atomic<std::size_t>> tail_;
        std::size_t buffer_mask_ = 0;
        std::unique_ptr<cell_type[]> buffer_;
    };

    ////////////////////////////////////////////////////////////////////////////
    // Using hpx::util::spinlock as the means of synchronization enables the use
    // of this channel with non-HPX threads.
    HPX_CXX_CORE_EXPORT template <typename T,
        channel_mode Mode = channel_mode::normal>
    using channel_mpsc = base_channel_mpsc<T, hpx::spinlock, Mode>;
}    // namespace hpx::lcos::local
