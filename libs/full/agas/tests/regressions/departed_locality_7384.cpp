//  Copyright (c) 2026 Bita Yekrang
//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This test verifies that resolving (or dispatching an action to) a locality
// that has already disconnected gracefully fails through the intended AGAS
// error path (hpx::error::bad_parameter) instead of letting a
// std::system_error raised by unique_lock::unlock escape (see #7384).

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/include/process.hpp>
#include <hpx/modules/filesystem.hpp>
#include <hpx/modules/testing.hpp>

#include <hpx/agas/addressing_service.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iterator>
#include <string>
#include <system_error>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
inline int get_arraylen(char** arr)
{
    int count = 0;
    if (nullptr != arr)
    {
        while (nullptr != arr[count])
            ++count;    // simply count the strings
    }
    return count;
}

std::vector<std::string> get_environment()
{
    std::vector<std::string> env;
#if defined(HPX_WINDOWS)
    int len = get_arraylen(_environ);
    std::copy(&_environ[0], &_environ[len], std::back_inserter(env));
#elif defined(linux) || defined(__linux) || defined(__linux__) ||              \
    defined(__AIX__) || defined(__APPLE__) || defined(__FreeBSD__)
    int len = get_arraylen(environ);
    std::copy(&environ[0], &environ[len], std::back_inserter(env));
#else
#error "Don't know, how to access the execution environment on this platform"
#endif
    return env;
}

///////////////////////////////////////////////////////////////////////////////
void ping() {}
HPX_PLAIN_ACTION(ping, departed_locality_ping_action)

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    namespace process = hpx::components::process;
    namespace fs = hpx::filesystem;

    // find where the HPX core libraries are located
    fs::path base_dir = hpx::util::find_prefix();
    base_dir /= "bin";

    fs::path exe =
        base_dir / "departed_locality_7384_child" HPX_EXECUTABLE_EXTENSION;

    if (vm.count("launch"))
        exe = vm["launch"].as<std::string>();

    // set up command line for the launched executable
    std::vector<std::string> args;
    args.push_back(hpx::filesystem::to_string(exe));
    args.push_back("--hpx:ignore-batch-env");
    args.push_back("--hpx:threads=1");
    // Force use of the TCP parcelport
    args.push_back("--hpx:ini=hpx.parcel.tcp.priority=1000");
    args.push_back("--hpx:ini=hpx.parcel.bootstrap=tcp");

    // set up environment for the launched executable
    std::vector<std::string> env = get_environment();    // current environment

    // the launched locality talks to this locality's AGAS server and runs its
    // own parcelport on the same host
    std::string const address =
        hpx::get_config_entry("hpx.agas.address", HPX_INITIAL_IP_ADDRESS);

    // pass along the console parcelport address
    env.push_back("HPX_AGAS_SERVER_ADDRESS=" + address);
    env.push_back("HPX_AGAS_SERVER_PORT=" +
        hpx::get_config_entry(
            "hpx.agas.port", std::to_string(HPX_INITIAL_IP_PORT)));

    // The launched executable will run on the same host as this test. Each
    // launched HPX locality needs to be assigned a unique port (the
    // launch_process test uses 42).
    int const port = 43;

    env.push_back("HPX_PARCEL_SERVER_ADDRESS=" + address);
    env.push_back("HPX_PARCEL_SERVER_PORT=" +
        std::to_string(HPX_CONNECTING_IP_PORT - port));

    // instruct new locality to connect back on startup using the given name
    env.push_back("HPX_ON_STARTUP_WAIT_ON_LATCH=departed_locality_7384");

    // The launched locality waits on this latch before disconnecting so that
    // its locality id can be captured here while it is still connected.
    hpx::distributed::latch sync(2);
    sync.register_as("departed_locality_7384_sync");

    // launch the executable that will connect as a new locality
    process::child c = process::execute(hpx::find_here(),
        process::run_exe(hpx::filesystem::to_string(exe)),
        process::set_args(args), process::set_env(env),
        process::start_in_dir(hpx::filesystem::to_string(base_dir)),
        process::throw_on_error(),
        process::wait_on_latch("departed_locality_7384"));    // same as above!

    // wait for the new locality to be up and running
    c.wait();
    HPX_TEST(c);

    // capture the id of the connected locality while it is still alive
    hpx::id_type const here = hpx::find_here();
    hpx::id_type departed;

    std::vector<hpx::id_type> localities = hpx::find_all_localities();
    HPX_TEST_EQ(localities.size(), std::size_t(2));
    for (hpx::id_type const& id : localities)
    {
        if (id != here)
            departed = id;
    }
    HPX_TEST(departed != hpx::invalid_id);

    hpx::naming::gid_type const departed_gid = departed.get_gid();

    // While the locality is connected resolving it succeeds. This also
    // populates the resolver cache that has to be invalidated once the
    // locality departs.
    {
        hpx::error_code ec;
        auto const& endpoints =
            hpx::naming::get_agas_client().resolve_locality(departed_gid, ec);
        HPX_TEST(!ec);
        HPX_TEST(!endpoints.empty());
    }

    // let the launched locality proceed to its graceful hpx::disconnect and
    // wait for the process to exit cleanly
    sync.arrive_and_wait();

    int const exit_code = c.wait_for_exit(hpx::launch::sync);
    HPX_TEST_EQ(exit_code, 0);

    // the departed locality may not be a member anymore (bounded wait, the
    // gate itself is membership-based)
    for (std::size_t i = 0; i != 300; ++i)
    {
        if (hpx::find_all_localities().size() == 1)
            break;
        hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    HPX_TEST_EQ(hpx::find_all_localities().size(), std::size_t(1));

    // Resolving the departed locality now has to fail through the intended
    // AGAS error path. Before the fix for #7384 a std::system_error raised by
    // unique_lock::unlock escaped instead, even for the non-throwing
    // error_code overload. The resolver cache entry may be invalidated
    // slightly after the membership change, so poll (bounded) until the
    // resolution fails.
    bool caught_system_error = false;
    bool resolution_failed = false;

    hpx::error_code ec;
    for (std::size_t i = 0; i != 300; ++i)
    {
        ec = hpx::error_code();
        try
        {
            auto const& endpoints =
                hpx::naming::get_agas_client().resolve_locality(
                    departed_gid, ec);
            if (ec || endpoints.empty())
            {
                resolution_failed = true;
                break;
            }
        }
        catch (std::system_error const&)
        {
            caught_system_error = true;
            break;
        }

        hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // the non-throwing overload reports the intended error instead of
    // letting the internal lock error escape
    HPX_TEST(!caught_system_error);
    HPX_TEST(resolution_failed);
    HPX_TEST_EQ(ec.value(), hpx::error::bad_parameter);

    // Dispatching an action to the departed locality fails with the intended
    // hpx::error::bad_parameter as well. Note that hpx::exception is derived
    // from std::system_error, catch it first.
    bool caught_bad_parameter = false;
    bool no_error = false;
    caught_system_error = false;
    try
    {
        hpx::future<void> f =
            hpx::async(departed_locality_ping_action(), departed);
        f.get();
        no_error = true;
    }
    catch (hpx::exception const& e)
    {
        caught_bad_parameter = e.get_error() == hpx::error::bad_parameter;
    }
    catch (std::system_error const&)
    {
        caught_system_error = true;
    }

    HPX_TEST(!no_error);
    HPX_TEST(!caught_system_error);
    HPX_TEST(caught_bad_parameter);

    return hpx::finalize();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("launch,l", value<std::string>(),
        "the process that will be launched and which connects back");

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
