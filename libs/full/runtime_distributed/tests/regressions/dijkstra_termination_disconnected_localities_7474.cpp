//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/process.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

namespace {

    std::atomic<bool> consulted(false);
}    // namespace

void busy_work()
{
    consulted.store(true);
    hpx::this_thread::suspend(std::chrono::milliseconds(200));
}
HPX_PLAIN_ACTION(busy_work, busy_work_action)

int hpx_main(hpx::program_options::variables_map& vm)
{
    namespace process = hpx::components::process;

    std::vector<hpx::id_type> workers;
    std::vector<process::child> children;

    for (std::size_t i = 0; i != 4; ++i)
    {
        std::vector<hpx::id_type> const localities_before =
            hpx::find_remote_localities();

        process::child child = process::launch_connecting_locality(
            vm["launch"].as<std::string>(), {"--hpx:threads=1"}, {}, i);
        child.wait();
        HPX_TEST(child);

        std::vector<hpx::id_type> const localities_after =
            hpx::find_remote_localities();
        auto const new_locality = std::ranges::find_if(localities_after,
            [&localities_before](hpx::id_type const& candidate) {
                return std::ranges::find(localities_before, candidate) ==
                    localities_before.end();
            });

        HPX_TEST(new_locality != localities_after.end());
        if (new_locality != localities_after.end())
        {
            workers.push_back(*new_locality);
        }
        children.push_back(HPX_MOVE(child));
    }

    HPX_TEST_EQ(workers.size(), static_cast<std::size_t>(4));
    if (workers.size() != 4)
    {
        return hpx::finalize();
    }

    std::vector<hpx::future<void>> work;
    work.reserve(workers.size());
    for (hpx::id_type const& worker : workers)
    {
        work.push_back(hpx::async<busy_work_action>(worker));
    }
    hpx::wait_all(work);

    for (hpx::id_type const& worker : workers)
    {
        HPX_TEST_EQ(hpx::force_disconnect(worker), 0);
    }

    // Regression test: termination detection must finish even when every
    // late-connected locality has disconnected.
    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    using hpx::program_options::options_description;
    using hpx::program_options::value;

    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    // clang-format off
    desc_commandline.add_options()
        ("launch", value<std::string>()->required(),
        "worker executable to launch as a connecting locality")
    ;
    // clang-format on

    std::vector<std::string> const cfg = {
        "hpx.expect_connecting_localities!=1"};

    hpx::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ(hpx::init(argc, argv, init_args), 0);

    return hpx::util::report_errors();
}
