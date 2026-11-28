//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Localities that leave while the UCX device conduit moves data to them: the
// test (the console, started alone) launches worker localities
// (locality_removed_worker) that connect to it, sends them device data, and
// removes them while transfers are in flight:
//
//  - with hpx::force_disconnect,
//  - by the worker leaving on its own (hpx::disconnect),
//  - by killing the worker, then hpx::force_disconnect.
//
// The conduit is told (device_conduit::locality_removed) and closes its
// endpoint to the locality. The workers run on the test's node: their
// endpoints have no peer error handling unless
// hpx.parcel.conduit.ucx.same_node_error_handling is 'peer' (the test runs
// both ways), and the conduit fails the sends to a removed worker itself. A
// failed device send to a removed locality must not stop the application,
// and the calls with device data in flight must not report wrong data (HPX
// doesn't guarantee that they finish). Large host data
// (hpx.parcel.conduit_host_chunks) goes through the conduit too.
// Afterwards, device data still reaches the other localities, a locality
// connecting later is reached through a new endpoint, and the test shuts
// down in time (the conduit doesn't wait for the localities that went away).

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/process.hpp>
#include <hpx/modules/filesystem.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/prefix.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/testing.hpp>

#include "locality_removed.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace process = hpx::components::process;
namespace fs = hpx::filesystem;

constexpr std::size_t small_size = std::size_t(1) << 20;
constexpr std::size_t large_size = std::size_t(16) << 20;
constexpr std::size_t transfers_in_flight = 16;
constexpr std::chrono::seconds outcome_timeout(10);

// the settings of the test the workers need as well
std::vector<std::string> worker_args;

void wait_for_worker_startup(hpx::id_type const& id)
{
    auto const deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(30);
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

// launches a worker (with 'extra_args' after the common ones, and the
// environment variables 'env') and returns it with the locality it
// registered as
std::pair<process::child, hpx::id_type> launch_worker(fs::path const& exe,
    std::uint64_t const spawn_id,
    std::vector<std::string> const& extra_args = {},
    std::vector<std::string> const& env = {})
{
    std::unordered_set<hpx::id_type> before;
    for (hpx::id_type const& id : hpx::find_all_localities())
    {
        before.insert(id);
    }

    std::vector<std::string> args = worker_args;
    args.insert(args.end(), extra_args.begin(), extra_args.end());
    process::child worker = process::launch_connecting_locality(
        fs::to_string(exe), args, env, spawn_id);
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

// host chunks go to 'id' through the conduit once it has checked that it
// reaches it, which it starts when a message has one (they go through the
// parcelport until then)
bool wait_for_host_chunks(hpx::id_type const& id)
{
    hpx::parcelset::device_conduit* conduit =
        hpx::parcelset::get_device_conduit();
    std::uint32_t const locality_id = hpx::naming::get_locality_id_from_id(id);
    auto const deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (!conduit->host_chunks_ready(locality_id))
    {
        if (std::chrono::steady_clock::now() >= deadline)
        {
            return false;
        }
        hpx::async(put_host_data_action(), id, pattern(small_size, 'w')).get();
        hpx::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}

// device data and large host data reach 'id' intact, through the conduit
void test_device_data(hpx::id_type const& id)
{
    hpx::parcelset::device_conduit const* conduit =
        hpx::parcelset::get_device_conduit();
    HPX_TEST(wait_for_host_chunks(id));
    std::uint64_t const before = conduit->statistics().chunks_sent;

    constexpr std::size_t count = 4;
    std::vector<hpx::future<bool>> results;
    for (std::size_t i = 0; i != count / 2; ++i)
    {
        char const first = static_cast<char>('a' + i);
        results.push_back(hpx::async(check_device_data_action(), id,
            device_like{pattern(small_size, first)}, first));
        results.push_back(hpx::async(
            check_host_data_action(), id, pattern(small_size, first), first));
    }
    for (hpx::future<bool>& f : results)
    {
        HPX_TEST(f.get());
    }

    // the conduit may complete its sends after the parcels have left
    auto const deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (conduit->statistics().chunks_sent < before + count &&
        std::chrono::steady_clock::now() < deadline)
    {
        hpx::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    HPX_TEST_LTE(before + count, conduit->statistics().chunks_sent);
}

// large host data reaches 'id' intact through the parcelport: the conduit
// doesn't reach it (also once it has had the time to check)
void test_host_data_without_conduit(hpx::id_type const& id)
{
    hpx::parcelset::device_conduit const* conduit =
        hpx::parcelset::get_device_conduit();
    std::uint64_t const before = conduit->statistics().chunks_sent;
    for (int round = 0; round != 2; ++round)
    {
        std::vector<hpx::future<bool>> results;
        for (std::size_t i = 0; i != 4; ++i)
        {
            char const first = static_cast<char>('a' + i);
            results.push_back(hpx::async(check_host_data_action(), id,
                pattern(small_size, first), first));
        }
        for (hpx::future<bool>& f : results)
        {
            HPX_TEST(f.get());
        }

        // the conduit's check (if it started one) has finished by now
        hpx::this_thread::sleep_for(std::chrono::seconds(2));
    }
    HPX_TEST_EQ(conduit->statistics().chunks_sent, before);
}

// device and host data to 'id' that is still being moved when it goes away
std::vector<hpx::future<bool>> start_transfers(hpx::id_type const& id)
{
    std::vector<hpx::future<bool>> results;
    for (std::size_t i = 0; i != transfers_in_flight / 2; ++i)
    {
        results.push_back(hpx::async(check_device_data_action(), id,
            device_like{pattern(large_size, 'x')}, 'x'));
        results.push_back(hpx::async(
            check_host_data_action(), id, pattern(large_size, 'x'), 'x'));
    }
    return results;
}

// The calls whose device data was in flight finish, fail because their
// destination went away, or never finish: HPX doesn't fail the futures of
// calls to a locality that was removed before it replied. None may report
// wrong data or another error, and waiting for them must not keep the test
// from going on.
void check_outcomes(std::vector<hpx::future<bool>>& results)
{
    auto const deadline = std::chrono::steady_clock::now() + outcome_timeout;
    std::size_t finished = 0;
    std::size_t failed = 0;
    std::size_t pending = 0;
    for (hpx::future<bool>& f : results)
    {
        if (f.wait_until(deadline) != hpx::future_status::ready)
        {
            ++pending;
            continue;
        }

        try
        {
            HPX_TEST(f.get());
            ++finished;
        }
        catch (hpx::exception const& e)
        {
            HPX_TEST_MSG(
                e.get_error() == hpx::error::locality_was_disconnected ||
                    e.get_error() == hpx::error::network_error,
                e.what());
            ++failed;
        }
    }
    std::cout << "  calls with device data in flight: " << finished
              << " finished, " << failed << " failed, " << pending
              << " pending\n";
}

std::chrono::steady_clock::time_point finalize_time;

int hpx_main(hpx::program_options::variables_map& vm)
{
    // errors of transfers to localities that have gone are tolerated
    hpx::get_config().tolerate_node_faults(true);

    HPX_TEST(hpx::parcelset::get_device_conduit() != nullptr);
    if (hpx::parcelset::get_device_conduit() == nullptr)
    {
        return hpx::finalize();
    }

    fs::path exe = hpx::util::find_prefix();
    exe /= "bin";
    exe /= "locality_removed_worker" HPX_EXECUTABLE_EXTENSION;
    if (vm.contains("launch"))
    {
        exe = vm["launch"].as<std::string>();
    }

    worker_args = {"--hpx:threads=2",
        "--hpx:ini=hpx.parcel.device_conduit=" +
            hpx::get_config_entry("hpx.parcel.device_conduit", "ucx"),
        "--hpx:ini=hpx.parcel.conduit.ucx.error_handling=" +
            hpx::get_config_entry(
                "hpx.parcel.conduit.ucx.error_handling", "peer"),
        "--hpx:ini=hpx.parcel.conduit.ucx.same_node_error_handling=" +
            hpx::get_config_entry(
                "hpx.parcel.conduit.ucx.same_node_error_handling", "none"),
        "--hpx:ini=hpx.parcel.conduit_host_chunks=1",
        "--hpx:ini=hpx.parcel.conduit_host_threshold=65536",
        "--hpx:ini=hpx.parcel.bootstrap=tcp",
        "--hpx:ini=hpx.parcel.tcp.priority=1000"};

    // a locality that stays until the end: device data still reaches it
    // after each of the others went away
    auto [control_worker, control] = launch_worker(exe, 1);
    test_device_data(control);

    std::cout << "Disconnecting a locality with device data in flight.\n";
    {
        auto [worker, id] = launch_worker(exe, 2);
        test_device_data(id);

        std::vector<hpx::future<bool>> results = start_transfers(id);
        hpx::error_code ec(hpx::throwmode::lightweight);
        HPX_TEST_EQ(hpx::force_disconnect(id, ec), 0);
        HPX_TEST(!ec);
        check_outcomes(results);

        HPX_TEST_EQ(worker.wait_for_exit(hpx::launch::sync), 0);
        test_device_data(control);
    }

    std::cout << "A locality leaving with device data in flight.\n";
    {
        auto [worker, id] = launch_worker(exe, 3);
        test_device_data(id);

        std::vector<hpx::future<bool>> results = start_transfers(id);
        hpx::post<leave_action>(id);
        HPX_TEST_EQ(worker.wait_for_exit(hpx::launch::sync), 0);
        check_outcomes(results);

        test_device_data(control);
    }

    std::cout << "A locality killed with device data in flight.\n";
    {
        auto [worker, id] = launch_worker(exe, 4);
        test_device_data(id);

        // the parcels aren't waited for: the process is gone, only the
        // conduit's sends are left
        for (std::size_t i = 0; i != transfers_in_flight / 2; ++i)
        {
            hpx::post<put_device_data_action>(
                id, device_like{pattern(large_size, 'k')});
            hpx::post<put_host_data_action>(id, pattern(large_size, 'k'));
        }
        worker.terminate(hpx::launch::sync);

        auto const start = std::chrono::steady_clock::now();
        hpx::error_code ec(hpx::throwmode::lightweight);
        HPX_TEST_EQ(hpx::force_disconnect(id, ec), 0);
        HPX_TEST(!ec);
        HPX_TEST_LT(std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - start)
                        .count(),
            10000);

        test_device_data(control);
    }

    std::cout << "A locality connecting after the others went away.\n";
    {
        auto [worker, id] = launch_worker(exe, 5);
        test_device_data(id);

        hpx::error_code ec(hpx::throwmode::lightweight);
        HPX_TEST_EQ(hpx::force_disconnect(id, ec), 0);
        HPX_TEST(!ec);
        HPX_TEST_EQ(worker.wait_for_exit(hpx::launch::sync), 0);
    }

    std::cout << "Localities the conduit doesn't move host chunks to.\n";
    {
        // one without a conduit (its endpoints say so)
        auto [plain_worker, plain] =
            launch_worker(exe, 6, {"--hpx:ini=hpx.parcel.device_conduit=none"});
        test_host_data_without_conduit(plain);

        // one with the UCX conduit that no other process reaches (UCX may
        // only use its loopback transport there)
        auto [isolated_worker, isolated] =
            launch_worker(exe, 7, {}, {"HPX_DEVICE_CONDUIT_UCX_TLS=self"});
        test_host_data_without_conduit(isolated);

        for (auto const& id : {plain, isolated})
        {
            hpx::error_code ec(hpx::throwmode::lightweight);
            HPX_TEST_EQ(hpx::force_disconnect(id, ec), 0);
            HPX_TEST(!ec);
        }
        HPX_TEST_EQ(plain_worker.wait_for_exit(hpx::launch::sync), 0);
        HPX_TEST_EQ(isolated_worker.wait_for_exit(hpx::launch::sync), 0);

        // the others are reached as before
        test_device_data(control);
    }

    {
        hpx::error_code ec(hpx::throwmode::lightweight);
        HPX_TEST_EQ(hpx::force_disconnect(control, ec), 0);
        HPX_TEST(!ec);
        HPX_TEST_EQ(control_worker.wait_for_exit(hpx::launch::sync), 0);
    }

    finalize_time = std::chrono::steady_clock::now();
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
         "the worker that is launched and connects to this test");
    // clang-format on

    hpx::init_params params;
    params.desc_cmdline = desc_commandline;
    params.cfg = {"hpx.expect_connecting_localities!=1",
        "hpx.parcel.device_conduit!=ucx", "hpx.parcel.conduit_host_chunks!=1",
        "hpx.parcel.conduit_host_threshold!=65536"};

    HPX_TEST_EQ(hpx::init(argc, argv, params), 0);

    // the conduit's endpoints to the localities that went away must not
    // delay the shutdown
    auto const shutdown = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - finalize_time);
    std::cout << "shutdown took " << shutdown.count() << " ms\n";
    HPX_TEST_LT(shutdown.count(), 5000);

    return hpx::util::report_errors();
}

#endif
