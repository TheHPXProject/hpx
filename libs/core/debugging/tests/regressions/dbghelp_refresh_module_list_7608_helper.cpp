//  Copyright (c) 2026 The STE||AR Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Companion DLL for the #7608 regression test. It is intentionally *not*
// linked into the test executable: it is loaded at runtime with
// LoadLibraryW, after the test process (and therefore DbgHelp's
// SymInitialize snapshot) has already started. That is the exact scenario
// #7608 describes: a module that did not exist when HPX's DbgHelp
// module table was first populated.
//
// call_into_helper() exists purely so that, while it is on the call
// stack, a captured backtrace contains a return address inside *this*
// module rather than inside the test executable or the CRT. It does not
// need to do anything interesting; it only needs an exported name and a
// non-trivial address for DbgHelp to resolve.

#if defined(_WIN32)
#include <windows.h>

extern "C" __declspec(dllexport) void call_into_helper(void (*callback)())
{
    // Do not tail-call/inline this away: the caller relies on this
    // function's own return address showing up as a stack frame. A
    // volatile write after callback() returns keeps the call from being
    // the last operation on this path, so it cannot be tail-call
    // optimized into replacing this frame.
    int volatile guard = 1;
    if (guard)
    {
        callback();
        guard = 0;
    }
}

BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID)
{
    return TRUE;
}
#endif
