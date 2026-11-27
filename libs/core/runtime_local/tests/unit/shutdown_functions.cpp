//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The local runtime runs the pre-shutdown functions and then the shutdown
// functions on an HPX thread during hpx::local::finalize.

#include <hpx/init.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>

#include <string>

std::string calls;
bool on_hpx_thread = true;

void record(char const* name)
{
    calls += name;
    calls += ' ';
    on_hpx_thread = on_hpx_thread && hpx::threads::get_self_ptr() != nullptr;
}

int hpx_main()
{
    hpx::register_shutdown_function([] { record("shutdown"); });
    hpx::register_pre_shutdown_function([] {
        record("pre_shutdown");

        // a pre-shutdown function may still register a shutdown function
        hpx::register_shutdown_function([] { record("late_shutdown"); });
    });

    HPX_TEST(calls.empty());
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // registered before the runtime exists
    hpx::register_pre_shutdown_function([] { record("early_pre_shutdown"); });

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    HPX_TEST_EQ(calls,
        std::string("early_pre_shutdown pre_shutdown shutdown late_shutdown "));
    HPX_TEST(on_hpx_thread);

    return hpx::util::report_errors();
}
