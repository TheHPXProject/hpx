//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Background work smoke test with realistic multi-subsystem polling.
//
// Simulates the three main background-work consumers present in a real HPX
// distributed run:
//
//   parcel::tcp   - parcelport TCP I/O completion polling (frequent, ~10 us)
//   agas::resolve - AGAS global-address resolution (moderate, ~5 us)
//   cuda::poll    - GPU/MPI completion polling (rare, ~2 us)
//
// Each subsystem emits a Tracy mark_event zone on the background-fiber row
// when it "finds" work, so the three event types are visible as distinct
// sub-zones within the background fiber timeline.
//
// The outer OS-thread zone "hpx::background_work [Worker #N]" (teal, from
// scheduling_loop.hpp) wraps every call to the combined poll function.
//
// TRACY USAGE:
//   1. Build with HPX_WITH_TRACY=ON.
//   2. Run:  TRACY_NO_EXIT=1 ./bin/background_work_smoke --iterations=20
//   3. Connect Tracy GUI.
//
//   OS-thread row:     teal "hpx::background_work" bars, zone info shows
//                      which worker thread drove the poll.
//   Background fiber:  blue sub-zones: "parcel::tcp::recv", "agas::resolve",
//                      "cuda::completion" appear when a subsystem finds work.
//   Zone Statistics (Ctrl+Z):  search "background_work" or the subsystem name.

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <vector>

namespace {

    // Parcel/TCP polling: fires every ~7 background calls, simulates ~10 us of
    // I/O completion processing.
    struct parcel_tcp_subsystem
    {
        static constexpr std::size_t poll_interval = 7;
        static constexpr std::chrono::microseconds work_time{10};

        std::atomic<std::size_t> polls{0};
        std::atomic<std::size_t> work_done{0};

        bool poll(std::size_t) noexcept
        {
            if (++polls % poll_interval != 0)
                return false;
            ++work_done;
            hpx::tracing::mark_event ev("parcel::tcp::recv");
            auto const end = std::chrono::steady_clock::now() + work_time;
            while (std::chrono::steady_clock::now() < end)
                ;
            return true;
        }
    } parcel_tcp_sim;

    // AGAS resolution: fires every ~23 background calls, simulates ~5 us of
    // address-resolution work.
    struct agas_subsystem
    {
        static constexpr std::size_t poll_interval = 23;
        static constexpr std::chrono::microseconds work_time{5};

        std::atomic<std::size_t> polls{0};
        std::atomic<std::size_t> work_done{0};

        bool poll(std::size_t) noexcept
        {
            if (++polls % poll_interval != 0)
                return false;
            ++work_done;
            hpx::tracing::mark_event ev("agas::resolve");
            auto const end = std::chrono::steady_clock::now() + work_time;
            while (std::chrono::steady_clock::now() < end)
                ;
            return true;
        }
    } agas_sim;

    // CUDA/MPI completion polling: fires every ~41 background calls, simulates
    // ~2 us of completion-queue draining.
    struct cuda_subsystem
    {
        static constexpr std::size_t poll_interval = 41;
        static constexpr std::chrono::microseconds work_time{2};

        std::atomic<std::size_t> polls{0};
        std::atomic<std::size_t> work_done{0};

        bool poll(std::size_t) noexcept
        {
            if (++polls % poll_interval != 0)
                return false;
            ++work_done;
            hpx::tracing::mark_event ev("cuda::completion");
            auto const end = std::chrono::steady_clock::now() + work_time;
            while (std::chrono::steady_clock::now() < end)
                ;
            return true;
        }
    } cuda_sim;

    bool background_poll(std::size_t num_thread) noexcept
    {
        bool did_work = false;
        did_work |= parcel_tcp_sim.poll(num_thread);
        did_work |= agas_sim.poll(num_thread);
        did_work |= cuda_sim.poll(num_thread);
        return did_work;
    }

}    // namespace

int hpx_main(hpx::program_options::variables_map& vm)
{
    std::size_t const iterations = vm["iterations"].as<std::size_t>();

    std::cout << "background_work_smoke: starting (" << iterations
              << " iterations)\n"
              << std::flush;

    for (std::size_t i = 0; i < iterations; ++i)
    {
        hpx::tracing::frame_mark("background_work_iteration");

        std::vector<hpx::future<void>> futures;
        futures.reserve(8);
        for (std::size_t j = 0; j < 8; ++j)
            futures.push_back(hpx::async([] {}));

        hpx::wait_all(futures);

        // 50 ms idle gap: background subsystems fire repeatedly, each
        // producing visible mark_event sub-zones on the fiber row.
        hpx::this_thread::sleep_for(std::chrono::milliseconds(50));

        if (iterations >= 10 && i % (iterations / 10) == 0)
        {
            std::cout << "  iteration " << i << "/" << iterations << " OK\n"
                      << std::flush;
        }
    }

    std::size_t const total_polls = parcel_tcp_sim.polls.load();

    std::cout << "  parcel::tcp  polls=" << parcel_tcp_sim.polls.load()
              << "  work=" << parcel_tcp_sim.work_done.load() << "\n"
              << "  agas         polls=" << agas_sim.polls.load()
              << "  work=" << agas_sim.work_done.load() << "\n"
              << "  cuda         polls=" << cuda_sim.polls.load()
              << "  work=" << cuda_sim.work_done.load() << "\n";

    if (total_polls == 0)
    {
        std::cerr << "FAIL: background_poll was never invoked\n";
        hpx::local::finalize();
        return -1;
    }

    std::cout << "OK: background_work_smoke - all " << iterations
              << " iterations completed\n";

    hpx::local::finalize();
    return 0;
}

int main(int argc, char* argv[])
{
    hpx::program_options::options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");
    desc_commandline.add_options()("iterations",
        hpx::program_options::value<std::size_t>()->default_value(5),
        "Number of test iterations to execute (default: 5 for fast CI)");

    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = {"hpx.os_threads=4"};
    init_args.rp_callback = [](hpx::resource::partitioner& rp,
                                hpx::program_options::variables_map const&) {
        rp.create_thread_pool("default",
            hpx::resource::scheduling_policy::local_priority_fifo,
            hpx::threads::policies::scheduler_mode::default_, &background_poll);
    };
    return hpx::local::init(hpx_main, argc, argv, init_args);
}
