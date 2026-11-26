//  Copyright (c) 2020 Mikael Simberg
//  Copyright (c) 2022-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/assert.hpp>
#include <hpx/concurrency/cache_line_data.hpp>
#include <hpx/modules/datastructures.hpp>

#include <atomic>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace hpx::concurrency::detail {

    /// \brief Identify one of the ends of the queue
    HPX_CXX_CORE_EXPORT enum class queue_end : std::uint8_t {
        left = 0,
        right = 1
    };

    HPX_CXX_CORE_EXPORT template <queue_end Which>
    struct opposite_end;

    template <>
    struct opposite_end<queue_end::left>
    {
        static constexpr queue_end value = queue_end::right;
    };

    template <>
    struct opposite_end<queue_end::right>
    {
        static constexpr queue_end value = queue_end::left;
    };

    HPX_CXX_CORE_EXPORT template <queue_end Which>
    inline constexpr queue_end opposite_end_v = opposite_end<Which>::value;

    /// \brief A concurrent queue which can only hold contiguous ranges of
    ///        integers.
    ///
    /// A concurrent queue which can be initialized with a range of integers.
    /// Items can be popped from both ends of the queue. Popping from the right
    /// decrements the next element that will be popped from the right, if there
    /// are items left. Popping from the left increments the next element that
    /// will be popped from the left, if there are items left.
    HPX_CXX_CORE_EXPORT template <typename T = std::uint32_t>
    class contiguous_index_queue
    {
        static_assert(sizeof(T) <= 4,    //-V112
            "contiguous_index_queue assumes at most 32 bit indices to fit two "
            "indices in an at most 64 bit struct");
        static_assert(std::is_integral_v<T>,
            "contiguous_index_queue only works with integral indices");

        struct range
        {
            T first = 0;
            T last = 0;

            range() = default;

            constexpr range(T first, T last) noexcept
              : first(first)
              , last(last)
            {
            }

            constexpr range increment_first() const noexcept
            {
                return range{first + 1, last};
            }

            constexpr range decrement_last() const noexcept
            {
                return range{first, last - 1};
            }

            constexpr bool empty() const noexcept
            {
                return first >= last;
            }

            // The number of items a chunked pop takes from a non-empty range:
            // the given fraction of the items, at least one.
            constexpr T chunk_size(T const divisor) const noexcept
            {
                T const count = static_cast<T>((last - first) / divisor);
                return count == 0 ? T(1) : count;
            }
        };

    public:
        /// \brief Reset the queue with the given range.
        ///
        /// Reset the queue with the given range. No additional synchronization
        /// is done to ensure that other threads are not accessing elements from
        /// the queue. It is the caller's responsibility to ensure that it is
        /// safe to reset the queue.
        ///
        /// \param first Beginning of the new range.
        /// \param last  End of the new range.
        constexpr void reset(T first, T last) noexcept
        {
            initial_range = range{first, last};
            current_range.data_ = range{first, last};
            HPX_ASSERT(first <= last);
        }

        /// \brief Construct a new contiguous_index_queue.
        ///
        /// Construct a new queue with an empty range.
        constexpr contiguous_index_queue() noexcept
          : initial_range{}
          , current_range{}
        {
        }

        /// \brief Construct a new contiguous_index_queue with the given range.
        ///
        /// Construct a new queue with the given range as the initial range.
        constexpr contiguous_index_queue(T first, T last) noexcept
          : initial_range{}
          , current_range{}
        {
            reset(first, last);
        }

        /// \brief Copy-construct a queue.
        ///
        /// No additional synchronization is done to ensure that other threads
        /// are not accessing elements from the queue being copied. It is the
        /// caller's responsibility to ensure that it is safe to copy the queue.
        contiguous_index_queue(contiguous_index_queue<T> const& other) noexcept
          : initial_range{other.initial_range}
          , current_range{}
        {
            current_range.data_ =
                other.current_range.data_.load(std::memory_order_relaxed);
        }

        /// \brief Copy-assign a queue.
        ///
        /// No additional synchronization is done to ensure that other threads
        /// are not accessing elements from the queue being copied. It is the
        /// caller's responsibility to ensure that it is safe to copy the queue.
        contiguous_index_queue& operator=(
            contiguous_index_queue const& other) noexcept
        {
            initial_range = other.initial_range;
            current_range =
                other.current_range.data_.load(std::memory_order_relaxed);
            return *this;
        }

        /// \brief Attempt to pop an item from the left of the queue.
        ///
        /// Attempt to pop an item from the left (beginning) of the queue. If
        /// no items are left hpx::nullopt is returned.
        hpx::optional<T> pop_left() noexcept
        {
            range desired_range{0, 0};
            T index = 0;

            range expected_range =
                current_range.data_.load(std::memory_order_relaxed);

            do
            {
                if (expected_range.empty())
                {
                    return hpx::optional<T>(hpx::nullopt);
                }

                // reduce pipeline pressure
                HPX_SMT_PAUSE;

                index = expected_range.first;
                desired_range = expected_range.increment_first();

            } while (!current_range.data_.compare_exchange_weak(
                expected_range, desired_range));

            return hpx::optional<T>(HPX_MOVE(index));
        }

        /// \brief Attempt to pop an item from the right of the queue.
        ///
        /// Attempt to pop an item from the right (end) of the queue. If no
        /// items are left hpx::nullopt is returned.
        hpx::optional<T> pop_right() noexcept
        {
            range desired_range{0, 0};
            T index = 0;

            range expected_range =
                current_range.data_.load(std::memory_order_relaxed);

            do
            {
                if (expected_range.empty())
                {
                    return hpx::optional<T>(hpx::nullopt);
                }

                // reduce pipeline pressure
                HPX_SMT_PAUSE;

                desired_range = expected_range.decrement_last();
                index = desired_range.last;

            } while (!current_range.data_.compare_exchange_weak(
                expected_range, desired_range));

            return hpx::optional<T>(HPX_MOVE(index));
        }

        /// \brief Attempt to pop a range of items from the left of the queue.
        ///
        /// Attempt to pop a range of items from the left (beginning) of the
        /// queue with a single atomic operation. The range holds n / divisor
        /// items, but at least one, where n is the number of items in the
        /// queue when the range is taken. If no items are left hpx::nullopt
        /// is returned.
        ///
        /// \param divisor The fraction of the items in the queue to pop, must
        ///        be larger than zero.
        /// \returns The popped range [first, last), or hpx::nullopt.
        hpx::optional<std::pair<T, T>> pop_left_chunk(T const divisor) noexcept
        {
            HPX_ASSERT(divisor > 0);

            range desired_range{0, 0};
            range expected_range =
                current_range.data_.load(std::memory_order_relaxed);

            while (true)
            {
                if (expected_range.empty())
                {
                    return hpx::optional<std::pair<T, T>>(hpx::nullopt);
                }

                desired_range = range{static_cast<T>(expected_range.first +
                                          expected_range.chunk_size(divisor)),
                    expected_range.last};

                if (current_range.data_.compare_exchange_weak(
                        expected_range, desired_range))
                {
                    break;
                }

                // reduce pipeline pressure before retrying
                HPX_SMT_PAUSE;
            }

            // expected_range holds the range before the pop
            return hpx::optional<std::pair<T, T>>(
                std::pair<T, T>(expected_range.first, desired_range.first));
        }

        /// \brief Attempt to pop a range of items from the right of the queue.
        ///
        /// Attempt to pop a range of items from the right (end) of the queue
        /// with a single atomic operation. The range holds n / divisor items,
        /// but at least one, where n is the number of items in the queue when
        /// the range is taken. If no items are left hpx::nullopt is returned.
        ///
        /// \param divisor The fraction of the items in the queue to pop, must
        ///        be larger than zero.
        /// \returns The popped range [first, last), or hpx::nullopt.
        hpx::optional<std::pair<T, T>> pop_right_chunk(T const divisor) noexcept
        {
            HPX_ASSERT(divisor > 0);

            range desired_range{0, 0};
            range expected_range =
                current_range.data_.load(std::memory_order_relaxed);

            while (true)
            {
                if (expected_range.empty())
                {
                    return hpx::optional<std::pair<T, T>>(hpx::nullopt);
                }

                desired_range = range{expected_range.first,
                    static_cast<T>(expected_range.last -
                        expected_range.chunk_size(divisor))};

                if (current_range.data_.compare_exchange_weak(
                        expected_range, desired_range))
                {
                    break;
                }

                // reduce pipeline pressure before retrying
                HPX_SMT_PAUSE;
            }

            // expected_range holds the range before the pop
            return hpx::optional<std::pair<T, T>>(
                std::pair<T, T>(desired_range.last, expected_range.last));
        }

        /// \brief Attempt to pop an item from the given end of the queue.
        ///
        /// Attempt to pop an item from the given end of the queue. If no items
        /// are left hpx::nullopt is returned.
        template <queue_end Which>
        hpx::optional<T> pop() noexcept
        {
            if constexpr (Which == queue_end::left)
            {
                return pop_left();
            }
            else
            {
                return pop_right();
            }
        }

        constexpr bool empty() const noexcept
        {
            return current_range.data_.load(std::memory_order_relaxed).empty();
        }

        std::pair<T, T> get_current_range() const noexcept
        {
            auto r = current_range.data_.load(std::memory_order_relaxed);
            return {r.first, r.last};
        }

    private:
        range initial_range;
        hpx::util::cache_line_data<std::atomic<range>> current_range;
    };
}    // namespace hpx::concurrency::detail
