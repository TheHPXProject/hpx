// Copyright (c) 2026 The STE||AR Group
// Copyright (c) 2026 Vansh Dobhal
//
// SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This smoke test verifies that HPX correctly invokes ITT API functions
// when built with HPX_WITH_ITTNOTIFY=ON. It exercises HPX's ITT RAII
// wrappers (domain, task, string_handle, mark_context) and spawns async
// work so the scheduling loop also generates ITT instrumentation.
//
// When run with the ittapi ref-collector (INTEL_LIBITTNOTIFY64), all
// ITT calls are logged to a file that CI can inspect for verification.

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/itt_notify.hpp>

#include <cstddef>

int hpx_main()
{
    // Exercise the ITT RAII wrappers directly
    hpx::util::itt::domain domain("hpx.itt.smoke");
    hpx::util::itt::string_handle task_name("smoke_task");
    hpx::util::itt::mark_context mark("smoke_mark");

    {
        hpx::util::itt::task t(domain, task_name);
    }

    // Spawn async work so the scheduler exercises its ITT instrumentation
    constexpr std::size_t n = 8;
    hpx::future<int> futs[n];

    for (std::size_t i = 0; i < n; ++i)
    {
        futs[i] = hpx::async([i]() -> int { return static_cast<int>(i * i); });
    }

    for (auto& f : futs)
    {
        f.get();
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return hpx::local::init(hpx_main, argc, argv);
}
