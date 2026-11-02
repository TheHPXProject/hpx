//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/cache/entries/entry.hpp>

#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::util::cache::entries {

    ///////////////////////////////////////////////////////////////////////////
    /// \class lfu_entry lfu_entry.hpp hpx/cache/entries/lfu_entry.hpp
    ///
    /// The \a lfu_entry type can be used to store arbitrary values in a cache.
    /// Using this type as the cache's entry type makes sure that the least
    /// frequently used entries are discarded from the cache first.
    ///
    /// \note The \a lfu_entry conforms to the CacheEntry concept.
    /// \note This type can be used to model a 'most frequently used' cache
    ///       policy if it is used with a std::greater as the caches'
    ///       UpdatePolicy (instead of the default std::less).
    ///
    /// \tparam Value     The data type to be stored in a cache. It has to be
    ///                   default constructible, copy constructible and
    ///                   less_than_comparable.
    ///
    HPX_CXX_CORE_EXPORT template <typename Value>
    class lfu_entry : public entry<Value>
    {
    private:
        using base_type = entry<Value>;

    public:
        /// \brief Any cache entry has to be default constructible
        lfu_entry() = default;

        /// \brief Construct a new instance of a cache entry holding the given
        ///        value.
        explicit lfu_entry(Value const& val) noexcept(
            std::is_nothrow_constructible_v<base_type, Value const&>)
          : base_type(val)
        {
        }

        /// \brief Construct a new instance of a cache entry holding the given
        ///        value.
        explicit lfu_entry(Value&& val) noexcept
          : base_type(HPX_MOVE(val))
        {
        }

        /// \brief    The function \a touch is called by a cache holding this
        ///           instance whenever it has been requested (touched).
        ///
        /// In the case of the LFU entry we store the reference count tracking
        /// the number of times this entry has been requested. This which will
        /// be used to compare the age of an entry during the invocation of the
        /// operator<().
        ///
        /// \returns  This function should return true if the cache needs to
        ///           update its internal heap. Usually this is needed if the
        ///           entry has been changed by touch() in a way influencing
        ///           the sort order as mandated by the cache's UpdatePolicy
        bool touch() noexcept
        {
            ++ref_count_;
            return true;
        }

        [[nodiscard]] constexpr unsigned long const& get_access_count()
            const noexcept
        {
            return ref_count_;
        }

        /// \brief Compare the 'age' of two entries. An entry is 'older' than
        ///        another entry if it has been accessed less frequently (LFU).
        friend auto operator<(lfu_entry const& lhs, lfu_entry const& rhs)
        {
            return rhs.get_access_count() < lhs.get_access_count();
        }
        friend auto operator>(lfu_entry const& lhs, lfu_entry const& rhs)
        {
            return rhs < lhs;
        }
        friend auto operator<=(lfu_entry const& lhs, lfu_entry const& rhs)
        {
            return !(rhs < lhs);
        }
        friend auto operator>=(lfu_entry const& lhs, lfu_entry const& rhs)
        {
            return !(lhs < rhs);
        }
        friend auto operator==(lfu_entry const& lhs, lfu_entry const& rhs)
        {
            return lhs.get_access_count() == rhs.get_access_count();
        }
        friend auto operator!=(lfu_entry const& lhs, lfu_entry const& rhs)
        {
            return !(lhs == rhs);
        }

    private:
        unsigned long ref_count_ = 0;
    };
}    // namespace hpx::util::cache::entries
