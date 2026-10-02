//  Copyright (c) 2026 Fabian C.
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/include/components.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <chrono>
#include <string>
#include <vector>

std::atomic<bool> component_destroyed{false};

struct test_component : hpx::components::component_base<test_component>
{
    ~test_component()
    {
        component_destroyed.store(true, std::memory_order_release);
    }
};

HPX_REGISTER_COMPONENT(
    hpx::components::component<test_component>, test_component_type)

int hpx_main()
{
    {
        hpx::id_type id = hpx::new_<test_component>(hpx::find_here()).get();

        HPX_TEST(!component_destroyed.load(std::memory_order_acquire));

        (void) id;
    }

    // Do not explicitly invoke AGAS garbage collection here. The component
    // should be reclaimed by the runtime's scheduler background work.
    auto const deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(5);

    while (!component_destroyed.load(std::memory_order_acquire) &&
        std::chrono::steady_clock::now() < deadline)
    {
        hpx::this_thread::yield();
    }

    HPX_TEST(component_destroyed.load(std::memory_order_acquire));

    return hpx::finalize();
}

int main(int argc, char** argv)
{
    // Keep threshold-triggered flushing out of the test and explicitly request
    // background work. Disabling loopback networking used to suppress it.
    std::vector<std::string> const cfg = {
        // Make threshold-triggered flushing effectively unreachable here.
        "hpx.agas.max_pending_refcnt_requests=1000000000",

        // Exercise a nonzero limit that the old networking gate reset to zero.
        "hpx.max_background_threads=1",

        // Make scheduler background work run promptly and deterministically.
        "hpx.max_busy_loop_count=0",

        // Exercise the single-locality no-networking path.
        "hpx.loopback_network=0",
    };

    hpx::init_params init_args;
    init_args.cfg = cfg;

    HPX_TEST_EQ(hpx::init(argc, argv, init_args), 0);
    return hpx::util::report_errors();
}

#endif
