//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This test verifies that hpx::components::process::launch_connecting_locality
// can be used to launch a new locality that connects back to this (bootstrap)
// locality, and that it is properly removed from AGAS again once it
// disconnects.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/process.hpp>
#include <hpx/modules/filesystem.hpp>
#include <hpx/modules/testing.hpp>

#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

int hpx_main(hpx::program_options::variables_map& vm)
{
    namespace process = hpx::components::process;
    namespace fs = hpx::filesystem;

    // find where the HPX core libraries are located
    fs::path base_dir = hpx::util::find_prefix();
    base_dir /= "bin";

    fs::path exe =
        base_dir / "worker_connecting_locality" HPX_EXECUTABLE_EXTENSION;

    if (vm.count("launch"))
        exe = vm["launch"].as<std::string>();

    std::size_t const before = hpx::find_all_localities().size();

    // Launch the worker as a connecting locality; this blocks (via an
    // internally-managed startup latch) until the worker has connected back.
    process::child c =
        process::launch_connecting_locality(hpx::filesystem::to_string(exe));

    c.wait();
    HPX_TEST(c);

    // the launched executable should have connected back as a new locality
    HPX_TEST(hpx::find_all_localities().size() > before);

    // wait for the worker to exit, it disconnects and returns 0
    int const exit_code = c.wait_for_exit(hpx::launch::sync);
    HPX_TEST_EQ(exit_code, 0);

    // disconnect() on the worker side deregisters it from AGAS, but that is
    // not synchronously reflected here, so poll (bounded) until it is.
    for (std::size_t i = 0; i != 100; ++i)
    {
        if (hpx::find_all_localities().size() == before)
            break;
        hpx::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    HPX_TEST_EQ(hpx::find_all_localities().size(), before);

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("launch,l", value<std::string>(),
        "the worker executable that will be launched and connect back");

    std::vector<std::string> const cfg = {
        "hpx.expect_connecting_localities!=1"};

    hpx::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::init(argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}

#endif
