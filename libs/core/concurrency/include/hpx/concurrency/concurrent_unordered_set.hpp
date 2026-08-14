//  Copyright (c) 2026 Pratyksh Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/concurrency/detail/concurrent_accessor.hpp>
#include <hpx/concurrency/spinlock.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/type_support.hpp>

#include <functional>
#include <mutex>

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <unordered_set>
#include <utility>

namespace hpx::concurrent {

    HPX_CXX_CORE_EXPORT template <typename Key, typename Hash = std::hash<Key>,
        typename KeyEqual = std::equal_to<Key>,
        typename Allocator = std::allocator<Key>>
    class concurrent_unordered_set
    {
    private:
        mutable hpx::util::spinlock mutex_;
        std::unordered_set<Key, Hash, KeyEqual, Allocator> set_;

    public:
        using key_type = Key;
        using value_type = Key;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using hasher = Hash;
        using key_equal = KeyEqual;
        using allocator_type = Allocator;

        using const_accessor = detail::concurrent_accessor<Key const>;
        using reference = const_accessor;
        using const_reference = const_accessor;

        concurrent_unordered_set() = default;

        explicit concurrent_unordered_set(size_type bucket_count,
            Hash const& hash = Hash(), KeyEqual const& equal = KeyEqual(),
            Allocator const& alloc = Allocator())
          : set_(bucket_count, hash, equal, alloc)
        {
        }

        concurrent_unordered_set(size_type bucket_count, Allocator const& alloc)
          : set_(bucket_count, alloc)
        {
        }

        concurrent_unordered_set(
            size_type bucket_count, Hash const& hash, Allocator const& alloc)
          : set_(bucket_count, hash, alloc)
        {
        }

        explicit concurrent_unordered_set(Allocator const& alloc)
          : set_(alloc)
        {
        }

        concurrent_unordered_set(concurrent_unordered_set const& other)
        {
            std::lock_guard<hpx::util::spinlock> lock(other.mutex_);
            set_ = other.set_;
        }

        concurrent_unordered_set(concurrent_unordered_set&& other) noexcept
        {
            std::lock_guard<hpx::util::spinlock> lock(other.mutex_);
            set_ = HPX_MOVE(other.set_);
        }

        concurrent_unordered_set& operator=(
            concurrent_unordered_set const& other)
        {
            if (this != &other)
            {
                std::scoped_lock lock(mutex_, other.mutex_);
                set_ = other.set_;
            }
            return *this;
        }

        concurrent_unordered_set& operator=(
            concurrent_unordered_set&& other) noexcept
        {
            if (this != &other)
            {
                std::lock_guard<hpx::util::spinlock> lock(mutex_);
                set_ = HPX_MOVE(other.set_);
            }
            return *this;
        }

        // Capacity
        bool empty() const noexcept
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return set_.empty();
        }

        size_type size() const noexcept
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return set_.size();
        }

        size_type max_size() const noexcept
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return set_.max_size();
        }

        // Modifiers
        void clear() noexcept
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            set_.clear();
        }

        bool insert(value_type const& value)
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return set_.insert(value).second;
        }

        bool insert(value_type&& value)
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return set_.insert(HPX_MOVE(value)).second;
        }

        size_type erase(Key const& key)
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return set_.erase(key);
        }

#if defined(HPX_HAVE_CXX23_STD_UNORDERED_TRANSPARENT_ERASE)
        template <typename K>
        size_type erase(K&& key)
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return set_.erase(HPX_FORWARD(K, key));
        }
#endif

        void swap(concurrent_unordered_set& other) noexcept
        {
            if (this != &other)
            {
                std::lock(mutex_, other.mutex_);
                std::lock_guard<hpx::util::spinlock> lock(
                    mutex_, std::adopt_lock);
                std::lock_guard<hpx::util::spinlock> other_lock(
                    other.mutex_, std::adopt_lock);
                set_.swap(other.set_);
            }
        }

        // Lookup
        size_type count(Key const& key) const
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return set_.count(key);
        }

        template <typename K>
        size_type count(K const& key) const
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return set_.count(key);
        }

        const_accessor find(Key const& key) const
        {
            std::unique_lock<hpx::util::spinlock> lock(mutex_);
            auto it = set_.find(key);
            if (it != set_.end())
            {
                return const_accessor(HPX_MOVE(lock), *it);
            }
            return const_accessor();
        }

        template <typename K>
        const_accessor find(K const& key) const
        {
            std::unique_lock<hpx::util::spinlock> lock(mutex_);
            auto it = set_.find(key);
            if (it != set_.end())
            {
                return const_accessor(HPX_MOVE(lock), *it);
            }
            return const_accessor();
        }

        bool contains(Key const& key) const
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return set_.find(key) != set_.end();
        }

        template <typename K>
        bool contains(K const& key) const
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return set_.find(key) != set_.end();
        }

        // Thread-safe iteration
        template <typename F>
        void for_each(F&& f) const
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            for (auto const& elem : set_)
            {
                if constexpr (std::is_void_v<
                                  std::invoke_result_t<F, decltype(elem)>>)
                {
                    f(elem);
                }
                else
                {
                    if (!f(elem))
                        break;
                }
            }
        }

        // Bucket interface
        size_type bucket_count() const noexcept
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return set_.bucket_count();
        }

        size_type max_bucket_count() const noexcept
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return set_.max_bucket_count();
        }

        size_type bucket_size(size_type n) const
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return set_.bucket_size(n);
        }

        size_type bucket(Key const& key) const
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return set_.bucket(key);
        }

        // Hash policy
        float load_factor() const noexcept
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return set_.load_factor();
        }

        float max_load_factor() const noexcept
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return set_.max_load_factor();
        }

        void max_load_factor(float ml)
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            set_.max_load_factor(ml);
        }

        void rehash(size_type count)
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            set_.rehash(count);
        }

        void reserve(size_type count)
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            set_.reserve(count);
        }
    };
}    // namespace hpx::concurrent
