//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// hpx.fast_idle_cap_time: in fast idle mode, with the cap set, idle workers
// poll for a limited time before each backoff step. With a cap small enough
// to cut every run of rounds short, the workers still have to pick up every
// burst of tasks after an idle gap, in fast idle mode and in normal mode, and
// the setting has to reach the configuration.

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

namespace {

    constexpr std::size_t bursts = 200;
    constexpr std::size_t tasks_per_burst = 16;

    // bursts of short tasks separated by idle gaps long enough for the idle
    // workers to run out of rounds and take backoff steps
    void run_bursts()
    {
        std::atomic<std::size_t> done{0};
        for (std::size_t b = 0; b != bursts; ++b)
        {
            std::vector<hpx::future<void>> burst;
            burst.reserve(tasks_per_burst);
            for (std::size_t t = 0; t != tasks_per_burst; ++t)
            {
                burst.push_back(hpx::async([&done] { ++done; }));
            }
            hpx::wait_all(burst);
            hpx::this_thread::sleep_for(std::chrono::microseconds(500));
        }
        HPX_TEST_EQ(done.load(), bursts * tasks_per_burst);
    }

    int hpx_main()
    {
        HPX_TEST_EQ(hpx::get_config_entry("hpx.fast_idle_cap_time", ""),
            std::string("2"));

        hpx::threads::add_scheduler_mode(
            hpx::threads::policies::scheduler_mode::fast_idle_mode);
        run_bursts();
        hpx::threads::remove_scheduler_mode(
            hpx::threads::policies::scheduler_mode::fast_idle_mode);

        // and in normal mode, where the cap doesn't apply
        run_bursts();

        return hpx::local::finalize();
    }

    int run_with_cap(char* argv0, std::string const& cap)
    {
        std::vector<char*> argv = {argv0, nullptr};
        hpx::local::init_params init_args;
        init_args.cfg = {"hpx.os_threads=4", "hpx.fast_idle_cap_time=" + cap};
        return hpx::local::init(hpx_main, 1, argv.data(), init_args);
    }
}    // namespace

int main(int, char* argv[])
{
    // a cap of 2 us cuts every run of rounds to its minimum
    HPX_TEST_EQ(run_with_cap(argv[0], "2"), 0);
    return hpx::util::report_errors();
}
