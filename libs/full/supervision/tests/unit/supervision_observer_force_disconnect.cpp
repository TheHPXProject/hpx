//  Copyright (c) 2026 Rohan Pattanayak
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// An observer whose locality was force-disconnected is dropped by the
// supervision manager tracking its target, instead of the failed delivery
// being recorded as the target having failed (#7834). Two worker localities
// are launched. The first one registers an observer for a target tracked by
// the second one and is then force-disconnected. The next event published
// for the target is applied and leaves it running, an observer living on the
// console still receives it, and the dropped observer's handle is no longer
// known to the second worker.
//
// The workers are separate processes (see
// supervision_observer_force_disconnect_worker.cpp) launched with
// process::launch_connecting_locality(), since hpx::force_disconnect can only
// remove a locality that connected late.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/process.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/filesystem.hpp>
#include <hpx/modules/prefix.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/supervision.hpp>

#include "supervision_test_helpers.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace process = hpx::components::process;
namespace fs = hpx::filesystem;

// The actions invoked on the workers, see
// supervision_observer_force_disconnect_worker.cpp.
bool worker_is_running()
{
    return hpx::is_running();
}
HPX_PLAIN_ACTION(worker_is_running, worker_is_running_action)

hpx::id_type register_observer_here(
    hpx::id_type const& manager_locality, hpx::id_type const& target)
{
    return hpx::supervision::register_observer(hpx::launch::sync,
        manager_locality, target,
        [](hpx::supervision::lifecycle_event_notification const&) {
            return true;
        });
}
HPX_PLAIN_ACTION(register_observer_here, register_observer_here_action)

void wait_for_worker_startup(hpx::id_type const& id)
{
    auto const deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(10);
    do
    {
        hpx::future<bool> ready = hpx::async<worker_is_running_action>(id);
        if (ready.wait_until(deadline) != hpx::future_status::ready)
        {
            break;
        }
        if (ready.get())
        {
            return;
        }
        hpx::this_thread::sleep_for(std::chrono::milliseconds(1));
    } while (std::chrono::steady_clock::now() < deadline);

    HPX_THROW_EXCEPTION(hpx::error::invalid_status, "wait_for_worker_startup",
        "worker did not finish runtime startup");
}

// Launches a worker process and identifies the locality it registered as by
// diffing the set of known localities before and after the launch.
std::pair<process::child, hpx::id_type> launch_worker(
    fs::path const& exe, std::uint64_t const spawn_id)
{
    std::unordered_set<hpx::id_type> before;
    for (hpx::id_type const& id : hpx::find_all_localities())
    {
        before.insert(id);
    }

    process::child worker = process::launch_connecting_locality(
        fs::to_string(exe), {"--hpx:threads=1"}, {}, spawn_id);
    worker.wait();
    HPX_TEST(worker);

    hpx::id_type new_locality;
    for (hpx::id_type const& id : hpx::find_all_localities())
    {
        if (!before.contains(id))
        {
            new_locality = id;
            break;
        }
    }
    HPX_TEST(new_locality);
    if (new_locality)
    {
        wait_for_worker_startup(new_locality);
    }

    return std::make_pair(HPX_MOVE(worker), new_locality);
}

void test_force_disconnected_observer_is_dropped(
    hpx::id_type const& observer_locality,
    hpx::id_type const& tracking_locality)
{
    if (!observer_locality || !tracking_locality)
    {
        return;
    }

    hpx::id_type const target = make_test_target();
    hpx::supervision::publish_event(hpx::launch::sync, tracking_locality,
        target, hpx::supervision::event::started);

    // an observer living on the console, which stays connected
    auto const running_seen = std::make_shared<std::atomic<std::size_t>>(0);
    hpx::id_type const healthy_handle = hpx::supervision::register_observer(
        hpx::launch::sync, tracking_locality, target,
        [running_seen](
            hpx::supervision::lifecycle_event_notification const& n) {
            if (n.event == hpx::supervision::event::running)
            {
                running_seen->fetch_add(1, std::memory_order_relaxed);
            }
            return true;
        });

    // an observer living on the locality that is disconnected below
    constexpr register_observer_here_action register_here;
    hpx::id_type const dropped_handle =
        register_here(observer_locality, tracking_locality, target);

    hpx::error_code ec(hpx::throwmode::lightweight);
    HPX_TEST_EQ(hpx::force_disconnect(observer_locality, ec), 0);
    HPX_TEST(!ec);

    auto const result = hpx::supervision::publish_event(hpx::launch::sync,
        tracking_locality, target, hpx::supervision::event::running);
    HPX_TEST(result == hpx::supervision::publish_result::applied);

    // the target was not marked failed
    auto const state = hpx::supervision::query_state(
        hpx::launch::sync, tracking_locality, target);
    HPX_TEST(state.last_event == hpx::supervision::event::running);
    HPX_TEST(!state.ec);

    HPX_TEST_EQ(running_seen->load(), static_cast<std::size_t>(1));

    // the dropped observer's handle is no longer known
    hpx::error_code dropped_ec(hpx::throwmode::lightweight);
    hpx::supervision::unregister_observer(
        hpx::launch::sync, tracking_locality, dropped_handle, dropped_ec);
    HPX_TEST_EQ(
        dropped_ec.value(), static_cast<int>(hpx::error::bad_parameter));

    hpx::error_code healthy_ec(hpx::throwmode::lightweight);
    hpx::supervision::unregister_observer(
        hpx::launch::sync, tracking_locality, healthy_handle, healthy_ec);
    HPX_TEST(!healthy_ec);
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    // enable fault tolerance
    hpx::get_config().tolerate_node_faults(true);

    // find where the HPX core libraries are located
    fs::path base_dir = hpx::util::find_prefix();
    base_dir /= "bin";

    fs::path exe = base_dir /
        "supervision_observer_force_disconnect_worker" HPX_EXECUTABLE_EXTENSION;
    if (vm.contains("launch"))
    {
        exe = vm["launch"].as<std::string>();
    }

    std::cout << "Launching " << exe << "\n";

    auto [observer_worker, observer_locality] = launch_worker(exe, 1);
    auto [tracking_worker, tracking_locality] = launch_worker(exe, 2);

    HPX_SUPERVISION_TEST_RUN(test_force_disconnected_observer_is_dropped,
        observer_locality, tracking_locality);

    HPX_TEST_EQ(observer_worker.wait_for_exit(hpx::launch::sync), 0);

    if (tracking_locality)
    {
        hpx::error_code ec(hpx::throwmode::lightweight);
        HPX_TEST_EQ(hpx::force_disconnect(tracking_locality, ec), 0);
        HPX_TEST(!ec);
    }
    HPX_TEST_EQ(tracking_worker.wait_for_exit(hpx::launch::sync), 0);

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    // clang-format off
    desc_commandline.add_options()
        ("launch,l", value<std::string>(),
         "the process that will be launched and which connects back");
    // clang-format on

    std::vector<std::string> const cfg = {
        "hpx.expect_connecting_localities!=1"};

    hpx::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ(hpx::init(argc, argv, init_args), 0);
    return hpx::util::report_errors();
}

#else

int main(int, char*[])
{
    return 0;
}

#endif
