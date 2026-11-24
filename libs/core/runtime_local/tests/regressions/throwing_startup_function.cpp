//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// An exception thrown by a startup function used to hang the runtime: it was
// finalized before it was up, and runtime::start kept waiting for it to come
// up. The error must reach the caller of hpx::local::init instead, and
// hpx_main must not run.

#include <hpx/init.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>

std::atomic<bool> main_ran{false};

int hpx_main()
{
    main_ran = true;
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    hpx::register_pre_startup_function([]() {
        HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
            "throwing_startup_function", "thrown by a startup function");
    });

    bool reported = false;
    try
    {
        reported = hpx::local::init(hpx_main, argc, argv) != 0;
    }
    catch (hpx::exception const& e)
    {
        reported = e.get_error() == hpx::error::bad_parameter;
    }

    HPX_TEST(reported);
    HPX_TEST(!main_ran);
    return hpx::util::report_errors();
}
