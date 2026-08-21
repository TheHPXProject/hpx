//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// non-HPX thread, or trying to disconnect the console locality itself, both
// fail with hpx::error::invalid_status, both through the error_code overload
// and by throwing), as well as the happy path, where disconnecting a valid
// remote locality succeeds and actually removes it from AGAS/the connection
// caches while leaving the remaining localities unaffected. Calling
// force_disconnect a second time on an already-disconnected locality does not
// hang. Once a locality has been removed, its gid no longer reports
// is_connecting() == true, so the eligibility check in force_disconnect fails
// fast with hpx::error::bad_parameter rather than attempting a second removal
// or blocking.
//
// Beyond that baseline, this test also covers: disconnecting a locality while
// an action is still in flight to it; repeated connect/disconnect cycles across
// distinct localities; two concurrent force_disconnect calls racing on the same
// target; and disconnecting a locality whose process has already been killed
// outright (rather than one that is cooperatively still running).
//
// The remote localities are spawned as separate worker processes (see
// force_disconnect_worker.cpp) via process::launch_connecting_locality(), since
// this test itself is always run as a single, console-only locality.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/include/process.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/agas.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/filesystem.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/prefix.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/testing.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <system_error>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

namespace process = hpx::components::process;
namespace fs = hpx::filesystem;

// Calling hpx::force_disconnect from a thread that is not an HPX-thread should
// fail with hpx::error::invalid_status, regardless of the given locality, since
// this check is performed before any locality validation.
void test_non_hpx_thread_guard()
{
    hpx::id_type const target = hpx::find_here();

    // ec-based overload
    {
        int retval = 0;
        hpx::error_code ec(hpx::throwmode::lightweight);

        std::thread t([&]() { retval = hpx::force_disconnect(target, ec); });
        t.join();

        HPX_TEST_EQ(retval, -1);
        HPX_TEST(ec);
        HPX_TEST_EQ(ec.value(), static_cast<int>(hpx::error::invalid_status));
    }

    // throwing overload
    {
        bool caught_exception = false;
        hpx::error thrown_error = hpx::error::success;

        std::thread t([&]() {
            try
            {
                hpx::force_disconnect(target);
                HPX_TEST(false);
            }
            catch (hpx::exception const& e)
            {
                caught_exception = true;
                thrown_error = e.get_error();
            }
        });
        t.join();

        HPX_TEST(caught_exception);
        HPX_TEST_EQ(thrown_error, hpx::error::invalid_status);
    }
}

// The console locality is not allowed to disconnect itself; calling
// hpx::force_disconnect(hpx::find_here(), ...) from console must fail.
void test_console_cannot_disconnect_itself(std::uint32_t const locality_id)
{
    if (locality_id != 0)
    {
        return;
    }

    hpx::id_type const here = hpx::find_here();

    // error_code overload
    {
        hpx::error_code ec(hpx::throwmode::lightweight);
        int const result = hpx::force_disconnect(here, ec);
        HPX_TEST_EQ(result, -1);
        HPX_TEST(ec);
        HPX_TEST_EQ(ec.value(), static_cast<int>(hpx::error::bad_parameter));
        std::string const msg = ec.get_message();
        HPX_TEST(msg.find("HPX(bad_parameter)") != std::string::npos ||
            msg.find("cannot be used to disconnect the ") != std::string::npos);
    }

    // throwing overload
    {
        bool caught_exception = false;
        try
        {
            hpx::force_disconnect(here);
            HPX_TEST(false);
        }
        catch (hpx::exception const& e)
        {
            caught_exception = true;
            HPX_TEST(e.get_error() == hpx::error::bad_parameter);
            std::string const msg = e.what();
            HPX_TEST(msg.find("HPX(bad_parameter)") != std::string::npos ||
                msg.find("cannot be used to disconnect the ") !=
                    std::string::npos);
        }
        HPX_TEST(caught_exception);
    }
}

// A trivial action used to probe whether a given locality is still reachable,
// i.e. still registered with AGAS and present in the connection caches.
std::uint32_t ping_locality()
{
    return hpx::get_locality_id();
}
HPX_PLAIN_ACTION(ping_locality, ping_locality_action)

// A long-running action used to simulate a parcel that is still in flight while
// the target locality is being force-disconnected.
std::uint32_t sleep_locality(int const ms)
{
    hpx::this_thread::sleep_for(std::chrono::milliseconds(ms));
    return hpx::get_locality_id();
}
HPX_PLAIN_ACTION(sleep_locality, sleep_locality_action)

// Launches a new connecting-locality worker process and identifies the
// hpx::id_type it registered as, by diffing the set of known localities before
// and after the launch. Returns both the child handle (so the caller can, e.g.,
// terminate it abruptly) and the newly registered locality id.
std::pair<process::child, hpx::id_type> launch_worker(
    fs::path const& exe, std::uint64_t spawn_id)
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

    return std::make_pair(HPX_MOVE(worker), new_locality);
}

// Calling hpx::force_disconnect on a valid, non-console, non-self locality must
// succeed and actually remove that locality from AGAS/the connection caches (so
// that further action dispatches to it fail), while leaving all remaining
// localities, including console itself, fully operational.
//
// Returns the id of the disconnected (now removed) locality, together with the
// id of the still-reachable control locality, so that a follow-up
// double-disconnect test (see below) can reuse the same setup instead of
// re-deriving it.
std::pair<hpx::id_type, hpx::id_type> test_force_disconnect_removes_locality()
{
    std::vector<hpx::id_type> const localities = hpx::find_all_localities();
    HPX_TEST(localities.size() >= 3);
    if (localities.size() < 3)
    {
        return {};
    }

    hpx::id_type const here = hpx::find_here();
    hpx::id_type target;
    hpx::id_type control;
    for (hpx::id_type const& id : localities)
    {
        if (id == here)
        {
            continue;
        }
        if (!target)
        {
            target = id;
        }
        else if (!control)
        {
            control = id;
        }
    }
    HPX_TEST(target);
    HPX_TEST(control);

    // The launched locality must be reported to have been connecting late.
    HPX_TEST(hpx::agas::is_connecting(target.get_gid()));

    // Disconnect the target locality; this must succeed.
    hpx::error_code ec(hpx::throwmode::lightweight);
    int const result = hpx::force_disconnect(target, ec);
    HPX_TEST_EQ(result, 0);
    HPX_TEST(!ec);

    // Dispatching an action to the disconnected locality must now fail, proving
    // that it was actually removed from AGAS/the connection caches.
    {
        bool caught_exception = false;
        try
        {
            constexpr ping_locality_action act;
            [[maybe_unused]] auto const r = act(target);
            HPX_TEST(false);
        }
        catch (hpx::exception const&)
        {
            caught_exception = true;
        }
        HPX_TEST(caught_exception);
    }

    // Confirm removal directly at the AGAS level, not just inferred from a
    // failed action dispatch (which could also be explained by a stale
    // connection-cache entry rather than actual deregistration).
    {
        hpx::error_code ec2(hpx::throwmode::lightweight);
        hpx::naming::address const addr =
            hpx::agas::resolve(hpx::launch::sync, target, ec2);
        HPX_TEST(ec2 || !addr);
    }

    // A third, uninvolved locality must still respond normally.
    {
        constexpr ping_locality_action act;
        HPX_TEST_EQ(
            act(control), hpx::naming::get_locality_id_from_id(control));
    }

    // The console locality itself must remain fully operational.
    HPX_TEST(hpx::agas::is_console());

    return std::make_pair(target, control);
}

// Calling force_disconnect a second time on an already-disconnected
// locality does not hang. Once a locality has been removed, its gid no
// longer reports is_connecting() == true, so the eligibility check in
// force_disconnect fails fast with hpx::error::bad_parameter rather than
// attempting a second removal or blocking.
void test_double_disconnect_should_fail(hpx::id_type const& target)
{
    if (!target)
    {
        return;
    }

    // error_code overload
    {
        hpx::error_code ec(hpx::throwmode::lightweight);
        int const result = hpx::force_disconnect(target, ec);
        HPX_TEST_EQ(result, -1);
        HPX_TEST(ec);
        HPX_TEST_EQ(ec.value(), static_cast<int>(hpx::error::bad_parameter));
    }

    // throwing overload
    {
        bool caught_exception = false;
        try
        {
            hpx::force_disconnect(target);
        }
        catch (hpx::exception const& e)
        {
            caught_exception = true;
            HPX_TEST(e.get_error() == hpx::error::bad_parameter);
        }
        HPX_TEST(caught_exception);
    }
}

// Disconnecting more than one locality in the same run must work: the second
// disconnect is independent of the first and must not be affected by state
// left over from having already removed a different locality (e.g. stale
// cache entries or partition-table bookkeeping for the earlier target).
void test_sequential_disconnects(hpx::id_type const& second_target)
{
    if (!second_target)
    {
        return;
    }

    // The launched locality must be reported to have been connecting late.
    HPX_TEST(hpx::agas::is_connecting(second_target.get_gid()));

    hpx::error_code ec(hpx::throwmode::lightweight);
    int const result = hpx::force_disconnect(second_target, ec);
    HPX_TEST_EQ(result, 0);
    HPX_TEST(!ec);

    // Confirm this second locality is also actually gone, not just the first.
    bool caught_exception = false;
    try
    {
        constexpr ping_locality_action act;
        [[maybe_unused]] auto const r = act(second_target);
        HPX_TEST(false);
    }
    catch (hpx::exception const&)
    {
        caught_exception = true;
    }
    HPX_TEST(caught_exception);
}

// Disconnecting a locality while an action is still in flight to it must not
// hang: the in-flight call either completes normally (it was already
// dispatched before the disconnect took effect) or fails with
// hpx::error::locality_was_disconnected (the dispatch guard or the
// connection teardown caught it); any other outcome is a failure.
void test_disconnect_with_inflight_parcel(hpx::id_type const& target)
{
    if (!target)
    {
        return;
    }

    constexpr sleep_locality_action act;
    hpx::future<std::uint32_t> f = hpx::async(act, target, 2000);

    hpx::error_code ec(hpx::throwmode::lightweight);
    int const result = hpx::force_disconnect(target, ec);
    HPX_TEST_EQ(result, 0);
    HPX_TEST(!ec);

    bool outcome_is_acceptable = false;
    try
    {
        [[maybe_unused]] std::uint32_t const r = f.get();
        outcome_is_acceptable = true;
    }
    catch (hpx::exception const& e)
    {
        outcome_is_acceptable =
            (e.get_error() == hpx::error::locality_was_disconnected);
    }
    catch (...)
    {
        HPX_TEST(false);
    }
    HPX_TEST(outcome_is_acceptable);
}

// Repeatedly launching and force-disconnecting distinct worker localities
// must not leak state or cross-contaminate: each cycle must independently
// register with, and then be fully removed from, AGAS.
void test_repeated_connect_disconnect_cycles(fs::path const& exe,
    std::size_t const iterations, std::uint64_t const first_spawn_id)
{
    for (std::size_t i = 0; i != iterations; ++i)
    {
        auto [worker, id] = launch_worker(exe, first_spawn_id + i);

        HPX_TEST(hpx::agas::is_connecting(id.get_gid()));

        hpx::error_code ec(hpx::throwmode::lightweight);
        int const result = hpx::force_disconnect(id, ec);
        HPX_TEST_EQ(result, 0);
        HPX_TEST(!ec);

        hpx::error_code resolve_ec(hpx::throwmode::lightweight);
        hpx::naming::address const addr =
            hpx::agas::resolve(hpx::launch::sync, id, resolve_ec);
        HPX_TEST(resolve_ec || !addr);

        int const exit_code = worker.wait_for_exit(hpx::launch::sync);
        HPX_TEST_EQ(exit_code, 0);
    }
}

// Two concurrent hpx::force_disconnect calls targeting the same locality
// must not corrupt state or hang: at most one may succeed, and the losing
// call must fail cleanly instead of duplicating the removal.
void test_concurrent_double_disconnect_race(hpx::id_type const& target)
{
    if (!target)
    {
        return;
    }

    hpx::error_code ec1(hpx::throwmode::lightweight);
    hpx::error_code ec2(hpx::throwmode::lightweight);

    hpx::future<int> f1 = hpx::async(
        [&target, &ec1]() { return hpx::force_disconnect(target, ec1); });
    hpx::future<int> f2 = hpx::async(
        [&target, &ec2]() { return hpx::force_disconnect(target, ec2); });

    int const r1 = f1.get();
    int const r2 = f2.get();

    bool const succeeded1 = (r1 == 0) && !ec1;
    bool const succeeded2 = (r2 == 0) && !ec2;
    HPX_TEST(succeeded1 != succeeded2);
}

// Force-disconnecting a locality whose process has already been killed (rather
// than one that is cooperatively still running) must complete within the
// best-effort notify timeout instead of hanging, and must still succeed in
// cleaning up the local AGAS/connection-cache state.
void test_disconnect_unreachable_locality(
    process::child& worker, hpx::id_type const& target)
{
    if (!target)
    {
        return;
    }

    worker.terminate(hpx::launch::sync);

    auto const start = std::chrono::steady_clock::now();

    hpx::error_code ec(hpx::throwmode::lightweight);
    int const result = hpx::force_disconnect(target, ec);

    auto const elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start);

    // Bounded well above the ~5s best-effort notify window used internally for
    // an unreachable target, but far below the ctest timeout for this test.
    HPX_TEST_LT(elapsed_ms.count(), 10000);

    HPX_TEST_EQ(result, 0);
    HPX_TEST(!ec);
}

// hpx::finalize() is asynchronous: it merely flags all localities to stop
// scheduling new work and returns almost immediately, regardless of
// whether shutdown actually completes promptly (or hangs, e.g. because of
// a just-disconnected locality). The real end-to-end shutdown duration is
// the time between the point where finalize() is invoked and hpx::init()
// actually returning in main(); since hpx_main() and main() do not share a
// live call stack across the shutdown boundary, the starting timestamp is
// stashed in a variable with static storage duration.
std::chrono::steady_clock::time_point finalize_time;

int hpx_main(hpx::program_options::variables_map& vm)
{
    // enable fault tolerance
    hpx::get_config().tolerate_node_faults(true);

    std::uint32_t const locality_id = hpx::get_locality_id();
    std::cout << "locality " << locality_id << " reached hpx_main\n";

    test_non_hpx_thread_guard();
    test_console_cannot_disconnect_itself(locality_id);

    // find where the HPX core libraries are located
    fs::path base_dir = hpx::util::find_prefix();
    base_dir /= "bin";

    fs::path exe =
        base_dir / "force_disconnect_worker" HPX_EXECUTABLE_EXTENSION;
    if (vm.contains("launch"))
    {
        exe = vm["launch"].as<std::string>();
    }

    std::cout << "Launching " << exe << "\n";

    process::child w1 = process::launch_connecting_locality(
        fs::to_string(exe), {"--hpx:threads=1"}, {}, 1);
    w1.wait();
    HPX_TEST(w1);

    process::child w2 = process::launch_connecting_locality(
        fs::to_string(exe), {"--hpx:threads=1"}, {}, 2);
    w2.wait();
    HPX_TEST(w2);

    HPX_TEST_EQ(hpx::find_all_localities().size(), static_cast<std::size_t>(3));

    auto const [first, second] = test_force_disconnect_removes_locality();
    test_double_disconnect_should_fail(first);
    test_sequential_disconnects(second);

    int const exit_code1 = w1.wait_for_exit(hpx::launch::sync);
    HPX_TEST_EQ(exit_code1, 0);
    int const exit_code2 = w2.wait_for_exit(hpx::launch::sync);
    HPX_TEST_EQ(exit_code2, 0);

    std::cout
        << "Disconnecting a locality with an action still in flight to it.\n";
    {
        auto [w3, id3] = launch_worker(exe, 3);
        test_disconnect_with_inflight_parcel(id3);
        int const exit_code3 = w3.wait_for_exit(hpx::launch::sync);
        HPX_TEST_EQ(exit_code3, 0);
    }

    std::cout
        << "Repeated connect/disconnect cycles across distinct localities.\n";
    test_repeated_connect_disconnect_cycles(exe, 2, 4);

    std::cout << "Concurrent double force_disconnect on the same locality.\n";
    {
        auto [w6, id6] = launch_worker(exe, 6);
        test_concurrent_double_disconnect_race(id6);
        int const exit_code6 = w6.wait_for_exit(hpx::launch::sync);
        HPX_TEST_EQ(exit_code6, 0);
    }

    std::cout << "Disconnecting a locality whose process is already gone.\n";
    {
        auto [w7, id7] = launch_worker(exe, 7);
        test_disconnect_unreachable_locality(w7, id7);
    }

    finalize_time = std::chrono::steady_clock::now();
    return hpx::finalize();
}

int main(int const argc, char* argv[])
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

    HPX_TEST_EQ_MSG(hpx::init(argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    // Measure the actual end-to-end shutdown duration, i.e. the time it took
    // hpx::init() to return after hpx::finalize() was called above. This is a
    // generous but still meaningful bound (far above normal shutdown time, but
    // far below the ctest timeout for this test), meant to catch shutdown
    // hanging or taking excessively long because of the locality that was
    // disconnected above; the ctest timeout remains the ultimate backstop
    // against an actual hang.
    auto const duration = std::chrono::steady_clock::now() - finalize_time;
    auto const shutdown_milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(duration);

    std::cout << "shutdown after force_disconnect took "
              << shutdown_milliseconds.count() << " milliseconds\n";
    HPX_TEST_LT(shutdown_milliseconds.count(), 30000);

    return hpx::util::report_errors();
}

#endif
