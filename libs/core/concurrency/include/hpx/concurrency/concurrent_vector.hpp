//  Copyright (c) 2026 Pratyksh Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/concurrency/detail/concurrent_accessor.hpp>
#include <hpx/concurrency/spinlock.hpp>
#include <hpx/errors/exception.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/type_support/assert_owns_lock.hpp>

#include <concepts>
#include <cstddef>
#include <iterator>
#include <mutex>

#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::concurrent {

    HPX_CXX_CORE_EXPORT template <typename T,
        typename Allocator = std::allocator<T>>
    class concurrent_vector
    {
    private:
        mutable hpx::util::spinlock mutex_;
        std::vector<T, Allocator> data_;

    public:
        using value_type = T;
        using allocator_type = Allocator;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        // Accessor classes to ensure thread-safe element access
        using accessor = detail::concurrent_accessor<T>;
        using const_accessor = detail::concurrent_accessor<T const>;

        using reference = accessor;
        using const_reference = const_accessor;

        concurrent_vector() = default;

        explicit concurrent_vector(Allocator const& alloc)
          : data_(alloc)
        {
        }

        explicit concurrent_vector(size_type count, T const& value = T(),
            Allocator const& alloc = Allocator())
          : data_(count, value, alloc)
        {
        }

        concurrent_vector(concurrent_vector const& other)
        {
            std::lock_guard<hpx::util::spinlock> lock(other.mutex_);
            data_ = other.data_;
        }

        concurrent_vector(concurrent_vector&& other) noexcept
        {
            std::lock_guard<hpx::util::spinlock> lock(other.mutex_);
            data_ = HPX_MOVE(other.data_);
        }

        concurrent_vector& operator=(concurrent_vector const& other)
        {
            if (this != &other)
            {
                std::scoped_lock lock(mutex_, other.mutex_);
                data_ = other.data_;
            }
            return *this;
        }

        concurrent_vector& operator=(concurrent_vector&& other) noexcept
        {
            if (this != &other)
            {
                std::lock_guard<hpx::util::spinlock> lock(mutex_);
                data_ = HPX_MOVE(other.data_);
            }
            return *this;
        }

        // Element access
        accessor operator[](size_type pos)
        {
            std::unique_lock<hpx::util::spinlock> lock(mutex_);
            return accessor(HPX_MOVE(lock), data_[pos]);
        }

        const_accessor operator[](size_type pos) const
        {
            std::unique_lock<hpx::util::spinlock> lock(mutex_);
            return const_accessor(HPX_MOVE(lock), data_[pos]);
        }

        accessor at(size_type pos)
        {
            std::unique_lock<hpx::util::spinlock> lock(mutex_);
            return accessor(HPX_MOVE(lock), data_.at(pos));
        }

        const_accessor at(size_type pos) const
        {
            std::unique_lock<hpx::util::spinlock> lock(mutex_);
            return const_accessor(HPX_MOVE(lock), data_.at(pos));
        }

        accessor front()
        {
            std::unique_lock<hpx::util::spinlock> lock(mutex_);
            return accessor(HPX_MOVE(lock), data_.front());
        }

        const_accessor front() const
        {
            std::unique_lock<hpx::util::spinlock> lock(mutex_);
            return const_accessor(HPX_MOVE(lock), data_.front());
        }

        accessor back()
        {
            std::unique_lock<hpx::util::spinlock> lock(mutex_);
            return accessor(HPX_MOVE(lock), data_.back());
        }

        const_accessor back() const
        {
            std::unique_lock<hpx::util::spinlock> lock(mutex_);
            return const_accessor(HPX_MOVE(lock), data_.back());
        }

        // Thread-safe iteration
        template <typename F>
        void for_each(F&& f)
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            for (auto& item : data_)
            {
                if constexpr (std::is_void_v<
                                  std::invoke_result_t<F, decltype(item)>>)
                {
                    f(item);
                }
                else
                {
                    if (!f(item))
                        break;
                }
            }
        }

        template <typename F>
        void for_each(F&& f) const
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            for (auto const& item : data_)
            {
                if constexpr (std::is_void_v<
                                  std::invoke_result_t<F, decltype(item)>>)
                {
                    f(item);
                }
                else
                {
                    if (!f(item))
                        break;
                }
            }
        }

        // Capacity
        bool empty() const noexcept
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return data_.empty();
        }

        size_type size() const noexcept
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return data_.size();
        }

        size_type max_size() const noexcept
        {
            // Consistent with map
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return data_.max_size();
        }

        void reserve(size_type new_cap)
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            data_.reserve(new_cap);
        }

        size_type capacity() const noexcept
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return data_.capacity();
        }

        void shrink_to_fit()
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            data_.shrink_to_fit();
        }

        // Modifiers
        void clear() noexcept
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            data_.clear();
        }

        void push_back(T const& value)
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            data_.push_back(value);
        }

        void push_back(T&& value)
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            data_.push_back(HPX_MOVE(value));
        }

        size_type grow_by(size_type n)
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            auto old_size = data_.size();
            data_.resize(old_size + n);
            return old_size;
        }

        size_type grow_by(size_type n, T const& value)
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            auto old_size = data_.size();
            data_.resize(old_size + n, value);
            return old_size;
        }
    };
}    // namespace hpx::concurrent
