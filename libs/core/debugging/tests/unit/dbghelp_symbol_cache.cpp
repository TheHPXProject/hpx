//  Copyright (c) 2026 The STE||AR Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Unit test for hpx::util::detail::dbghelp_symbol_cache, in particular the
// clear() member added for #7608: after a DbgHelp module-list refresh, any
// previously cached address may now belong to a different module (or none
// at all), so backtrace.cpp drops the whole cache rather than reasoning
// about individual entries. This test exercises the cache directly, with
// synthetic addresses, so it needs no live symbol resolution and no
// particular set of loaded modules.

#include <hpx/config.hpp>

#if defined(HPX_WINDOWS)
#include <hpx/debugging/detail/dbghelp_symbol_cache.hpp>
#include <hpx/modules/testing.hpp>

#include <windows.h>

int main()
{
    using hpx::util::detail::dbghelp_symbol_cache;
    using hpx::util::detail::resolved_symbol_info;

    {
        // Basic insert/try_get round trip.
        dbghelp_symbol_cache cache;

        resolved_symbol_info out;
        HPX_TEST(!cache.try_get(0x1000, out));

        resolved_symbol_info in;
        in.name = "some_function";
        in.displacement = 0x10;
        cache.insert(0x1000, in);

        HPX_TEST(cache.try_get(0x1000, out));
        HPX_TEST_EQ(out.name, in.name);
        HPX_TEST_EQ(out.displacement, in.displacement);

        // A different address was never inserted.
        HPX_TEST(!cache.try_get(0x2000, out));
    }

    {
        // clear() drops every entry, not just the most recent one.
        dbghelp_symbol_cache cache;

        resolved_symbol_info a;
        a.name = "func_a";
        a.displacement = 1;
        cache.insert(0x1000, a);

        resolved_symbol_info b;
        b.name = "func_b";
        b.displacement = 2;
        cache.insert(0x2000, b);

        resolved_symbol_info out;
        HPX_TEST(cache.try_get(0x1000, out));
        HPX_TEST(cache.try_get(0x2000, out));

        cache.clear();

        HPX_TEST(!cache.try_get(0x1000, out));
        HPX_TEST(!cache.try_get(0x2000, out));
    }

    {
        // The cache is usable again after clear(): a fresh insert for an
        // address that existed before clear() is not accidentally
        // treated as still present.
        dbghelp_symbol_cache cache;

        resolved_symbol_info stale;
        stale.name = "old_module_func";
        stale.displacement = 0x40;
        cache.insert(0x3000, stale);

        cache.clear();

        resolved_symbol_info fresh;
        fresh.name = "new_module_func";
        fresh.displacement = 0x80;
        cache.insert(0x3000, fresh);

        resolved_symbol_info out;
        HPX_TEST(cache.try_get(0x3000, out));
        HPX_TEST_EQ(out.name, fresh.name);
        HPX_TEST_EQ(out.displacement, fresh.displacement);
    }

    {
        // The allocation base recorded for validating a cache hit (see
        // #7608) survives the round trip unchanged.
        dbghelp_symbol_cache cache;

        resolved_symbol_info in;
        in.name = "some_function";
        in.displacement = 0x10;
        in.allocation_base = 0x400000;
        cache.insert(0x1000, in);

        resolved_symbol_info out;
        HPX_TEST(cache.try_get(0x1000, out));
        HPX_TEST_EQ(out.allocation_base, in.allocation_base);
    }

    return hpx::util::report_errors();
}
#else
int main()
{
    return 0;
}
#endif
