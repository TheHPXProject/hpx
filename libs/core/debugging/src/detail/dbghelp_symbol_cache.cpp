//  Copyright (c) 2026 The STE||AR Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_WINDOWS)

#include <hpx/debugging/detail/dbghelp_symbol_cache.hpp>

// Consumed from outside the cache module, so this must go through the
// generated umbrella header rather than an internal hpx/cache/... header
// (see AGENTS.md).
#include <hpx/modules/cache.hpp>

#include <cstddef>
#include <memory>

namespace hpx::util::detail {

    struct dbghelp_symbol_cache::impl
    {
        explicit impl(std::size_t max_entries)
          : cache(max_entries)
        {
        }

        hpx::util::cache::lru_cache<DWORD64,
            hpx::util::cache::entries::lru_entry<resolved_symbol_info>>
            cache;
    };

    dbghelp_symbol_cache::dbghelp_symbol_cache(std::size_t max_entries)
      : impl_(std::make_unique<impl>(max_entries))
    {
    }

    dbghelp_symbol_cache::~dbghelp_symbol_cache() = default;

    bool dbghelp_symbol_cache::try_get(
        DWORD64 address, resolved_symbol_info& out)
    {
        DWORD64 realkey = 0;
        hpx::util::cache::entries::lru_entry<resolved_symbol_info> entry;
        if (!impl_->cache.get_entry(address, realkey, entry))
        {
            return false;
        }

        out = entry.get();
        return true;
    }

    void dbghelp_symbol_cache::insert(
        DWORD64 address, resolved_symbol_info const& value)
    {
        impl_->cache.insert(address,
            hpx::util::cache::entries::lru_entry<resolved_symbol_info>(value));
    }

    dbghelp_symbol_cache& get_dbghelp_symbol_cache()
    {
        // Function-local static: constructed on first use, same
        // rationale as dbghelp_mutex() in dbghelp_lock.cpp.
        static dbghelp_symbol_cache cache;
        return cache;
    }
}    // namespace hpx::util::detail

#endif    // HPX_WINDOWS
