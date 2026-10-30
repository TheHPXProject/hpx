//  Copyright (c) 2026 The STE||AR Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Regression test for issue #7608.
//
// backtrace.cpp's Windows path calls SymInitialize(..., TRUE) exactly
// once, the first time a symbol is resolved. That call snapshots the set
// of currently loaded modules; DbgHelp never learns about a module
// loaded afterwards unless something explicitly asks it to
// (SymRefreshModuleList). Before the fix, a backtrace frame that lands
// in a DLL loaded after that first resolution would resolve to "???"
// forever, even though the module is perfectly loaded and its symbols
// are perfectly available.
//
// This test:
//   1. Forces DbgHelp initialisation *before* the helper DLL is loaded,
//      exactly reproducing the stale-snapshot scenario.
//   2. Loads the helper DLL and calls into it so that a real captured
//      backtrace contains a return address inside it.
//   3. Confirms that address resolves to the helper's exported symbol
//      name rather than "???".
//   4. Unloads the helper DLL and confirms a subsequent, unrelated
//      backtrace still works without crashing (the module-list/ cache
//      churn on unload must not leave stale state behind).
//
// On non-MSVC platforms (and on MSVC configurations where
// HPX_HAVE_STACKTRACES is off) this test is a no-op: the DbgHelp symbol
// resolution path in backtrace.cpp is only compiled for HPX_MSVC (see
// the #elif defined(HPX_MSVC) branches there), not for every HPX_WINDOWS
// toolchain, so there is nothing to check without it.

#include <hpx/config.hpp>

#if defined(HPX_MSVC) && defined(HPX_HAVE_STACKTRACES)

#include <hpx/debugging/detail/dbghelp_lock.hpp>
#include <hpx/debugging/detail/dbghelp_symbol_cache.hpp>
#include <hpx/modules/debugging.hpp>
#include <hpx/modules/testing.hpp>

#include <windows.h>

#include <cstddef>
#include <optional>
#include <string>

namespace {

    using call_into_helper_t = void (*)(void (*)());

    // Populated by helper_callback() from inside the helper DLL's stack
    // frame; checked back on the test's own thread once
    // call_into_helper() returns.
    std::optional<hpx::util::backtrace> g_captured;

    void helper_callback()
    {
        // Captured while call_into_helper()'s frame (inside the helper
        // DLL) is still on the stack, so it shows up in frames_.
        g_captured.emplace();
    }

    [[nodiscard]] bool any_frame_names(
        hpx::util::backtrace const& bt, std::string const& needle)
    {
        for (std::size_t i = 0; i != bt.stack_size(); ++i)
        {
            if (bt.trace_line(i).find(needle) != std::string::npos)
            {
                return true;
            }
        }
        return false;
    }

    // Returns the return address of the first frame whose resolved line
    // contains the given needle, or nullptr if there is none.
    [[nodiscard]] void* find_frame_address(
        hpx::util::backtrace const& bt, std::string const& needle)
    {
        for (std::size_t i = 0; i != bt.stack_size(); ++i)
        {
            if (bt.trace_line(i).find(needle) != std::string::npos)
            {
                return bt.return_address(i);
            }
        }
        return nullptr;
    }

}    // namespace

int main()
{
    // Step 1: force DbgHelp's one-time SymInitialize snapshot to happen
    // now, while the helper DLL is not yet loaded. Discard the result;
    // this call exists only to establish the stale-snapshot precondition
    // #7608 describes.
    (void) hpx::util::trace();

    void* helper_frame = nullptr;

    // Step 2: load a module DbgHelp has never heard of, and capture a
    // real backtrace with one of its frames inside that module.
    HMODULE const helper =
        LoadLibraryW(L"dbghelp_refresh_module_list_7608_helper.dll");
    HPX_TEST(helper != nullptr);
    if (helper == nullptr)
    {
        // Nothing further to check without the helper module; report
        // what we have and stop here rather than dereferencing a null
        // handle below.
        return hpx::util::report_errors();
    }

    auto* const call_into_helper = reinterpret_cast<call_into_helper_t>(
        GetProcAddress(helper, "call_into_helper"));
    HPX_TEST(call_into_helper != nullptr);

    if (call_into_helper != nullptr)
    {
        call_into_helper(&helper_callback);

        HPX_TEST(g_captured.has_value());
        if (g_captured.has_value())
        {
            // Step 3: at least one captured frame must be the
            // call_into_helper() return address, inside a module that
            // was loaded after the very first SymInitialize. Before the
            // #7608 fix this resolves to "???" because DbgHelp's module
            // table was never refreshed.
            HPX_TEST(any_frame_names(*g_captured, "call_into_helper"));
            helper_frame = find_frame_address(*g_captured, "call_into_helper");
        }
    }

    // Step 4: unload the module DbgHelp just learned about, and make
    // sure a later, unrelated backtrace still behaves: no crash, no use
    // of any now-dangling module state. We deliberately do not assert
    // anything about symbol names here (an address that used to belong
    // to the unloaded module is not guaranteed to resolve to anything
    // in particular, and address-space reuse after FreeLibrary is exactly
    // the scenario the module-list-changed cache clear exists for), only
    // that resolving continues to work at all.
    FreeLibrary(helper);
    g_captured.reset();

    std::string const post_unload_trace = hpx::util::trace();
    HPX_TEST(!post_unload_trace.empty());

    // Step 5: load the helper again. It may or may not land at the same
    // address as before; either way a stale cache entry from the first
    // load must not produce a wrong or missing symbol.
    //
    // Plant a sentinel for the first load's frame address. If the reload
    // reuses that address, the cached entry's allocation base no longer
    // matches the live module, so it must be dropped rather than served.
    if (helper_frame != nullptr)
    {
        hpx::util::detail::dbghelp_scoped_lock const l;
        auto& cache = hpx::util::detail::get_dbghelp_symbol_cache();
        cache.clear();

        hpx::util::detail::resolved_symbol_info sentinel;
        sentinel.name = "stale_cache_sentinel";
        sentinel.allocation_base = 0;
        cache.insert(reinterpret_cast<DWORD64>(helper_frame), sentinel);
    }

    // A refresh may have just been throttled (500ms in backtrace.cpp); wait
    // it out so a relocated reload can still trigger a module list refresh.
    Sleep(600);

    HMODULE const reloaded =
        LoadLibraryW(L"dbghelp_refresh_module_list_7608_helper.dll");
    HPX_TEST(reloaded != nullptr);
    if (reloaded != nullptr)
    {
        auto* const reloaded_call = reinterpret_cast<call_into_helper_t>(
            GetProcAddress(reloaded, "call_into_helper"));
        HPX_TEST(reloaded_call != nullptr);
        if (reloaded_call != nullptr)
        {
            reloaded_call(&helper_callback);
            HPX_TEST(g_captured.has_value());
            if (g_captured.has_value())
            {
                HPX_TEST(any_frame_names(*g_captured, "call_into_helper"));
                HPX_TEST(!any_frame_names(*g_captured, "stale_cache_sentinel"));
            }
        }
        g_captured.reset();
        FreeLibrary(reloaded);
    }

    return hpx::util::report_errors();
}

#else

int main()
{
    return 0;
}

#endif
