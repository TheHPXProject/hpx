//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <cstdint>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::util::cache::statistics {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT enum class method {
        get_entry = 0,
        insert_entry = 1,
        update_entry = 2,
        erase_entry = 3
    };

    ///////////////////////////////////////////////////////////////////////////
    /// Provides a cache-statistics policy that records no statistics.
    HPX_CXX_CORE_EXPORT class no_statistics
    {
    public:
        /// \brief  The function \a got_hit will be called by a cache instance
        ///         whenever an entry got touched.
        static constexpr void got_hit() noexcept {}

        /// \brief  The function \a got_miss will be called by a cache instance
        ///         whenever a requested entry has not been found in the cache.
        static constexpr void got_miss() noexcept {}

        /// \brief  The function \a got_insertion will be called by a cache
        ///         instance whenever a new entry has been inserted.
        static constexpr void got_insertion() noexcept {}

        /// \brief  The function \a got_eviction will be called by a cache
        ///         instance whenever an entry has been removed from the cache
        ///         because a new inserted entry let the cache grow beyond its
        ///         capacity.
        static constexpr void got_eviction() noexcept {}

        /// \brief Reset all statistics
        static constexpr void clear() noexcept {}

        /// Helper class to update timings and counts on function exit
        struct update_on_exit
        {
            constexpr update_on_exit(no_statistics const&, method) noexcept {}
            ~update_on_exit() = default;

            update_on_exit(update_on_exit const&) = delete;
            update_on_exit(update_on_exit&&) = delete;
            update_on_exit& operator=(update_on_exit const&) = delete;
            update_on_exit& operator=(update_on_exit&&) = delete;
        };

        /// The function \a get_get_entry_count returns the number of
        /// invocations of the get_entry() API function of the cache.
        [[nodiscard]] static constexpr std::int64_t get_get_entry_count(
            bool) noexcept
        {
            return 0;
        }

        /// The function \a get_insert_entry_count returns the number of
        /// invocations of the insert_entry() API function of the cache.
        [[nodiscard]] static constexpr std::int64_t get_insert_entry_count(
            bool) noexcept
        {
            return 0;
        }

        /// The function \a get_update_entry_count returns the number of
        /// invocations of the update_entry() API function of the cache.
        [[nodiscard]] static constexpr std::int64_t get_update_entry_count(
            bool) noexcept
        {
            return 0;
        }

        /// The function \a get_erase_entry_count returns the number of
        /// invocations of the erase() API function of the cache.
        [[nodiscard]] static constexpr std::int64_t get_erase_entry_count(
            bool) noexcept
        {
            return 0;
        }

        /// The function \a get_get_entry_time returns the overall time spent
        /// executing of the get_entry() API function of the cache.
        [[nodiscard]] static constexpr std::int64_t get_get_entry_time(
            bool) noexcept
        {
            return 0;
        }

        /// The function \a get_insert_entry_time returns the overall time
        /// spent executing of the insert_entry() API function of the cache.
        [[nodiscard]] static constexpr std::int64_t get_insert_entry_time(
            bool) noexcept
        {
            return 0;
        }

        /// The function \a get_update_entry_time returns the overall time
        /// spent executing of the update_entry() API function of the cache.
        [[nodiscard]] static constexpr std::int64_t get_update_entry_time(
            bool) noexcept
        {
            return 0;
        }

        /// The function \a get_erase_entry_time returns the overall time spent
        /// executing of the erase() API function of the cache.
        [[nodiscard]] static constexpr std::int64_t get_erase_entry_time(
            bool) noexcept
        {
            return 0;
        }
    };
}    // namespace hpx::util::cache::statistics
