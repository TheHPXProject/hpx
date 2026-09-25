//  Copyright (c) 2026 The STE||AR Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

// Windows-only. Caches the result of SymFromAddr so that repeated
// backtraces over the same return addresses (e.g. one per lock
// acquisition under HPX_WITH_VERIFY_LOCKS_BACKTRACE) do not re-enter
// DbgHelp for an address already resolved. The pimpl below keeps the
// cache module (hpx/modules/cache.hpp) out of this public header; only
// dbghelp_symbol_cache.cpp needs it.
#if defined(HPX_WINDOWS)

#include <cstddef>
#include <memory>
#include <string>

#include <windows.h>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::util::detail {

    /// \brief The result of a single SymFromAddr lookup, as needed by
    ///        backtrace formatting.
    struct resolved_symbol_info
    {
        std::string name;
        DWORD64 displacement = 0;

        /// The AllocationBase (as reported by VirtualQuery) of the memory
        /// region containing the address when it was resolved. Used to
        /// detect a cached entry whose module was unloaded and whose
        /// address range was reused by another module (see #7608).
        DWORD64 allocation_base = 0;
    };

    /// \brief A process-wide cache mapping addresses already resolved by
    ///        SymFromAddr to their symbol name and displacement.
    ///
    /// \note All access to an instance of this class must happen while
    ///       the caller holds \a dbghelp_scoped_lock, including calls
    ///       into \a try_get and \a insert. This class performs no
    ///       locking of its own; it only stores previously resolved
    ///       symbols so that DbgHelp is not called again for an address
    ///       already seen.
    class HPX_CORE_EXPORT dbghelp_symbol_cache
    {
    public:
        explicit dbghelp_symbol_cache(std::size_t max_entries = 1024);
        ~dbghelp_symbol_cache();

        dbghelp_symbol_cache(dbghelp_symbol_cache const&) = delete;
        dbghelp_symbol_cache& operator=(dbghelp_symbol_cache const&) = delete;

        /// \brief Look up a previously resolved address.
        ///
        /// \returns \a true if \a address was found, with \a out filled
        ///          in and marked most-recently-used; \a false otherwise.
        [[nodiscard]] bool try_get(DWORD64 address, resolved_symbol_info& out);

        /// \brief Record the result of resolving \a address, evicting
        ///        the least recently used entry first if the cache is
        ///        already at capacity.
        void insert(DWORD64 address, resolved_symbol_info const& value);

        /// \brief Drop every cached resolution.
        ///
        /// \note Used after a DbgHelp module-list refresh (see #7608):
        ///       once the set of loaded modules has changed, an address
        ///       already in the cache may now belong to a different
        ///       module (or one that no longer exists), so the safe
        ///       choice is to forget everything rather than reason
        ///       about which entries are still valid.
        void clear();

    private:
        struct impl;
        std::unique_ptr<impl> impl_;
    };

    /// \brief Access the single, process-wide symbol cache used by
    ///        \a get_symbol().
    HPX_CORE_EXPORT dbghelp_symbol_cache& get_dbghelp_symbol_cache();
}    // namespace hpx::util::detail

#include <hpx/config/warnings_suffix.hpp>

#endif    // HPX_WINDOWS
