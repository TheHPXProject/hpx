//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::util::cache::entries {

    ///////////////////////////////////////////////////////////////////////////
    /// \class entry entry.hpp hpx/cache/entries/entry.hpp
    ///
    /// \tparam Value     The data type to be stored in a cache. It has to be
    ///                   default constructible, copy constructible and
    ///                   less_than_comparable.
    ///
    HPX_CXX_CORE_EXPORT template <typename Value>
    class entry
    {
    public:
        /// The type of value stored in this cache entry.
        using value_type = Value;

        // NOLINTBEGIN(bugprone-crtp-constructor-accessibility)

        /// \brief Any cache entry has to be default constructible
        entry() = default;

        /// \brief Construct a new instance of a cache entry holding the given
        ///        value.
        explicit entry(value_type const& val) noexcept(
            std::is_nothrow_copy_constructible_v<value_type>)
          : value_(val)
        {
        }

        /// \brief Construct a new instance of a cache entry holding the given
        ///        value.
        explicit entry(value_type&& val) noexcept
          : value_(HPX_MOVE(val))
        {
        }
        // NOLINTEND(bugprone-crtp-constructor-accessibility)

        /// \brief    The function \a touch is called by a cache holding this
        ///           instance whenever it has been requested (touched).
        ///
        /// \note     It is possible to change the entry in a way influencing
        ///           the sort criteria mandated by the UpdatePolicy. In this
        ///           case the function should return \a true to indicate this
        ///           to the cache, forcing to reorder the cache entries.
        /// \note     This function is part of the CacheEntry concept
        ///
        /// \return   This function should return true if the cache needs to
        ///           update its internal heap. Usually this is needed if the
        ///           entry has been changed by touch() in a way influencing
        ///           the sort order as mandated by the cache's UpdatePolicy
        static constexpr bool touch() noexcept
        {
            return false;
        }

        /// \brief    The function \a insert is called by a cache whenever it
        ///           is about to be inserted into the cache.
        ///
        /// \note     This function is part of the CacheEntry concept
        ///
        /// \returns  This function should return \a true if the entry should
        ///           be added to the cache, otherwise it should return
        ///           \a false.
        static constexpr bool insert() noexcept
        {
            return true;
        }

        /// \brief    The function \a remove is called by a cache holding this
        ///           instance whenever it is about to be removed from the
        ///           cache.
        ///
        /// \note     This function is part of the CacheEntry concept
        ///
        /// \returns  The return value can be used to avoid removing this
        ///           instance from the cache. If the value is \a true it is
        ///           ok to remove the entry, otherwise it will stay in the
        ///           cache.
        static constexpr bool remove() noexcept
        {
            return true;
        }

        /// \brief    Return the 'size' of this entry. By default, the size of
        ///           each entry is just one (1), which is sensible if the
        ///           cache has a limit (capacity) measured in number of
        ///           entries.
        static constexpr std::size_t get_size() noexcept
        {
            return 1;
        }

        /// \brief    Forwarding operator<=> allowing to compare entries instead
        ///           of the values.
        friend auto operator<=>(entry const&, entry const&) = default;

        /// \brief Get a reference to the stored data value
        ///
        /// \note This function is part of the CacheEntry concept
        [[nodiscard]] value_type& get() noexcept
        {
            return value_;
        }

        /// \brief Returns a const reference to the stored value.
        ///
        /// \note This function is part of the CacheEntry concept.
        [[nodiscard]] constexpr value_type const& get() const noexcept
        {
            return value_;
        }

    private:
        value_type value_;
    };
}    // namespace hpx::util::cache::entries
