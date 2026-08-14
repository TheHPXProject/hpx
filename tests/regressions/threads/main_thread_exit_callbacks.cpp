//  Copyright (c) 2023 Panos Syskakis
//  Copyright (c) 2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/runtime.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <cstddef>

bool callback_called(false);

int hpx_main()
{
    hpx::threads::thread_id_type id = hpx::threads::get_self_id();
    hpx::threads::add_thread_exit_callback(id, [id]() {
        hpx::threads::thread_id_type const id1 = hpx::threads::get_self_id();
        HPX_TEST_EQ(id1, id);

        callback_called = true;
    });

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    // Test local runtime
    {
        hpx::init_params iparams;
        iparams.mode = hpx::runtime_mode::local;
        callback_called = false;
        HPX_TEST_EQ_MSG(hpx::init(argc, argv, iparams), 0,
            "HPX main exited with non-zero status");
        HPX_TEST(callback_called);
    }

#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
    // Test distributed runtime
    {
        hpx::init_params iparams;
        iparams.mode = hpx::runtime_mode::console;
        callback_called = false;
        HPX_TEST_EQ_MSG(hpx::init(argc, argv, iparams), 0,
            "HPX main exited with non-zero status");

        HPX_TEST(callback_called);
    }
#endif
    return hpx::util::report_errors();
}
