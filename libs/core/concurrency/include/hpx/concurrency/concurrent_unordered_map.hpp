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
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace hpx::concurrent {

    HPX_CXX_CORE_EXPORT template <typename Key, typename T,
        typename Hash = std::hash<Key>, typename KeyEqual = std::equal_to<Key>,
        typename Allocator = std::allocator<std::pair<Key const, T>>>
    class concurrent_unordered_map
    {
    private:
        mutable hpx::util::spinlock mutex_;
        std::unordered_map<Key, T, Hash, KeyEqual, Allocator> map_;

    public:
        using key_type = Key;
        using mapped_type = T;
        using value_type = std::pair<Key const, T>;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using hasher = Hash;
        using key_equal = KeyEqual;
        using allocator_type = Allocator;

        using accessor = detail::concurrent_accessor<T>;
        using const_accessor = detail::concurrent_accessor<T const>;

        using reference = accessor;
        using const_reference = const_accessor;

        concurrent_unordered_map() = default;

        explicit concurrent_unordered_map(size_type bucket_count,
            Hash const& hash = Hash(), KeyEqual const& equal = KeyEqual(),
            Allocator const& alloc = Allocator())
          : map_(bucket_count, hash, equal, alloc)
        {
        }

        concurrent_unordered_map(size_type bucket_count, Allocator const& alloc)
          : map_(bucket_count, alloc)
        {
        }

        concurrent_unordered_map(
            size_type bucket_count, Hash const& hash, Allocator const& alloc)
          : map_(bucket_count, hash, alloc)
        {
        }

        explicit concurrent_unordered_map(Allocator const& alloc)
          : map_(alloc)
        {
        }

        concurrent_unordered_map(concurrent_unordered_map const& other)
        {
            std::lock_guard<hpx::util::spinlock> lock(other.mutex_);
            map_ = other.map_;
        }

        concurrent_unordered_map(concurrent_unordered_map&& other) noexcept
        {
            std::lock_guard<hpx::util::spinlock> lock(other.mutex_);
            map_ = HPX_MOVE(other.map_);
        }

        concurrent_unordered_map& operator=(
            concurrent_unordered_map const& other)
        {
            if (this != &other)
            {
                std::lock(mutex_, other.mutex_);
                std::lock_guard<hpx::util::spinlock> lock(
                    mutex_, std::adopt_lock);
                std::lock_guard<hpx::util::spinlock> other_lock(
                    other.mutex_, std::adopt_lock);
                map_ = other.map_;
            }
            return *this;
        }

        concurrent_unordered_map& operator=(
            concurrent_unordered_map&& other) noexcept
        {
            if (this != &other)
            {
                std::lock_guard<hpx::util::spinlock> lock(mutex_);
                map_ = HPX_MOVE(other.map_);
            }
            return *this;
        }

        // Capacity
        bool empty() const noexcept
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return map_.empty();
        }

        size_type size() const noexcept
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return map_.size();
        }

        size_type max_size() const noexcept
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return map_.max_size();
        }

        // Modifiers
        void clear() noexcept
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            map_.clear();
        }

        bool insert(value_type const& value)
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return map_.insert(value).second;
        }

        bool insert(value_type&& value)
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return map_.insert(HPX_MOVE(value)).second;
        }

        size_type erase(Key const& key)
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return map_.erase(key);
        }

#if defined(HPX_HAVE_CXX23_STD_UNORDERED_TRANSPARENT_ERASE)
        template <typename K>
        size_type erase(K&& key)
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return map_.erase(HPX_FORWARD(K, key));
        }
#endif

        void swap(concurrent_unordered_map& other) noexcept
        {
            if (this != &other)
            {
                std::lock(mutex_, other.mutex_);
                std::lock_guard<hpx::util::spinlock> lock(
                    mutex_, std::adopt_lock);
                std::lock_guard<hpx::util::spinlock> other_lock(
                    other.mutex_, std::adopt_lock);
                map_.swap(other.map_);
            }
        }

        // Lookup
        accessor operator[](Key const& key)
        {
            std::unique_lock<hpx::util::spinlock> lock(mutex_);
            return accessor(HPX_MOVE(lock), map_[key]);
        }

        accessor operator[](Key&& key)
        {
            std::unique_lock<hpx::util::spinlock> lock(mutex_);
            return accessor(HPX_MOVE(lock), map_[HPX_MOVE(key)]);
        }

#if defined(HPX_HAVE_CXX26_STD_UNORDERED_TRANSPARENT_LOOKUP)
        template <typename K>
        accessor operator[](K&& key)
        {
            std::unique_lock<hpx::util::spinlock> lock(mutex_);
            return accessor(HPX_MOVE(lock), map_[HPX_FORWARD(K, key)]);
        }
#endif

        accessor at(Key const& key)
        {
            std::unique_lock<hpx::util::spinlock> lock(mutex_);
            return accessor(HPX_MOVE(lock), map_.at(key));
        }

        const_accessor at(Key const& key) const
        {
            std::unique_lock<hpx::util::spinlock> lock(mutex_);
            return const_accessor(HPX_MOVE(lock), map_.at(key));
        }

#if defined(HPX_HAVE_CXX26_STD_UNORDERED_TRANSPARENT_LOOKUP)
        template <typename K>
        accessor at(K const& key)
        {
            std::unique_lock<hpx::util::spinlock> lock(mutex_);
            return accessor(HPX_MOVE(lock), map_.at(key));
        }

        template <typename K>
        const_accessor at(K const& key) const
        {
            std::unique_lock<hpx::util::spinlock> lock(mutex_);
            return const_accessor(HPX_MOVE(lock), map_.at(key));
        }
#endif

        size_type count(Key const& key) const
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return map_.count(key);
        }

        template <typename K>
        size_type count(K const& key) const
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return map_.count(key);
        }

        accessor find(Key const& key)
        {
            std::unique_lock<hpx::util::spinlock> lock(mutex_);
            auto it = map_.find(key);
            if (it != map_.end())
            {
                return accessor(HPX_MOVE(lock), it->second);
            }
            return accessor();
        }

        const_accessor find(Key const& key) const
        {
            std::unique_lock<hpx::util::spinlock> lock(mutex_);
            auto it = map_.find(key);
            if (it != map_.end())
            {
                return const_accessor(HPX_MOVE(lock), it->second);
            }
            return const_accessor();
        }

        template <typename K>
        accessor find(K const& key)
        {
            std::unique_lock<hpx::util::spinlock> lock(mutex_);
            auto it = map_.find(key);
            if (it != map_.end())
            {
                return accessor(HPX_MOVE(lock), it->second);
            }
            return accessor();
        }

        template <typename K>
        const_accessor find(K const& key) const
        {
            std::unique_lock<hpx::util::spinlock> lock(mutex_);
            auto it = map_.find(key);
            if (it != map_.end())
            {
                return const_accessor(HPX_MOVE(lock), it->second);
            }
            return const_accessor();
        }

        bool contains(Key const& key) const
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return map_.find(key) != map_.end();
        }

        template <typename K>
        bool contains(K const& key) const
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return map_.find(key) != map_.end();
        }

        // Thread-safe iteration
        template <typename F>
        void for_each(F&& f)
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            for (auto& kv : map_)
            {
                if constexpr (std::is_void_v<
                                  std::invoke_result_t<F, decltype(kv)>>)
                {
                    f(kv);
                }
                else
                {
                    if (!f(kv))
                        break;
                }
            }
        }

        template <typename F>
        void for_each(F&& f) const
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            for (auto const& kv : map_)
            {
                if constexpr (std::is_void_v<
                                  std::invoke_result_t<F, decltype(kv)>>)
                {
                    f(kv);
                }
                else
                {
                    if (!f(kv))
                        break;
                }
            }
        }

        // Bucket interface
        size_type bucket_count() const noexcept
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return map_.bucket_count();
        }

        size_type max_bucket_count() const noexcept
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return map_.max_bucket_count();
        }

        size_type bucket_size(size_type n) const
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return map_.bucket_size(n);
        }

        size_type bucket(Key const& key) const
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return map_.bucket(key);
        }

        // Hash policy
        float load_factor() const noexcept
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return map_.load_factor();
        }

        float max_load_factor() const noexcept
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            return map_.max_load_factor();
        }

        void max_load_factor(float ml)
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            map_.max_load_factor(ml);
        }

        void rehash(size_type count)
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            map_.rehash(count);
        }

        void reserve(size_type count)
        {
            std::lock_guard<hpx::util::spinlock> lock(mutex_);
            map_.reserve(count);
        }
    };
}    // namespace hpx::concurrent
