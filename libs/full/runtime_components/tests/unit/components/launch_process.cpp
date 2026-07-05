//  Copyright (c) 2016 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/process.hpp>
#include <hpx/modules/filesystem.hpp>
#include <hpx/modules/testing.hpp>

#include "components/launch_process_test_server.hpp"

#include <cstddef>
#include <iostream>
#include <string>
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

// ----------------------------------------------------------------------------
int hpx_main(hpx::program_options::variables_map& vm)
{
    namespace process = hpx::components::process;
    namespace fs = hpx::filesystem;

    // find where the HPX core libraries are located
    fs::path base_dir = hpx::util::find_prefix();
    base_dir /= "bin";

    fs::path exe = base_dir / "launched_process_test" HPX_EXECUTABLE_EXTENSION;

    std::string launch_target;
    if (vm.count("launch"))
    {
        launch_target = vm["launch"].as<std::string>();
        std::cout << "using launch: " << launch_target << std::endl;
        exe = launch_target;
    }
    else
    {
        std::cout << "using launch (default): " << exe << std::endl;
    }

    // set up command line for launched executable
    std::vector<std::string> args;
    args.push_back(hpx::filesystem::to_string(exe));
    args.push_back("--exit_code=42");
    args.push_back("--component=test_server");
    args.push_back("--set_message=accessed");
    args.push_back("--hpx:ignore-batch-env");
    args.push_back("--hpx:threads=1");
    // Force use of the TCP parcelport
    args.push_back("--hpx:ini=hpx.parcel.tcp.priority=1000");
    args.push_back("--hpx:ini=hpx.parcel.bootstrap=tcp");

    // set up environment for launched executable
    std::vector<std::string> env = get_environment();    // current environment

    // Pass along the console parcelport address
    env.push_back("HPX_AGAS_SERVER_ADDRESS=" +
        hpx::get_config_entry("hpx.agas.address", HPX_INITIAL_IP_ADDRESS));
    env.push_back("HPX_AGAS_SERVER_PORT=" +
        hpx::get_config_entry(
            "hpx.agas.port", std::to_string(HPX_INITIAL_IP_PORT)));

    // Pass along the parcelport address which should be used by the launched
    // executable

    // The launched executable will run on the same host as this test
    int port = 42;    // each launched HPX locality needs to be assigned a
                      // unique port

    env.push_back("HPX_PARCEL_SERVER_ADDRESS=" +
        hpx::get_config_entry("hpx.agas.address", HPX_INITIAL_IP_ADDRESS));
    env.push_back("HPX_PARCEL_SERVER_PORT=" +
        std::to_string(HPX_CONNECTING_IP_PORT - port));

    // Instruct new locality to connect back on startup using the given name.
    env.push_back("HPX_ON_STARTUP_WAIT_ON_LATCH=launch_process");

    // launch test executable
    process::child c = process::execute(
        hpx::find_here(), process::run_exe(hpx::filesystem::to_string(exe)),
        process::set_args(args), process::set_env(env),
        process::start_in_dir(hpx::filesystem::to_string(base_dir)),
        process::throw_on_error(),
        process::wait_on_latch("launch_process")    // same as above!
    );

    {
        // now create an instance of the test_server component
        hpx::components::client<launch_process::test_server> t =
            hpx::new_<launch_process::test_server>(hpx::find_here());

        hpx::future<std::string> f =
            hpx::async(launch_process_get_message_action(), t);
        HPX_TEST_EQ(f.get(), std::string("initialized"));

        // register the component instance with AGAS
        t.register_as("test_server");    // same as --component=<> above

        // wait for the HPX locality to be up and running
        c.wait();
        HPX_TEST(c);

        // the launched executable should have connected back as a new locality
        HPX_TEST_EQ(hpx::find_all_localities().size(), std::size_t(2));

        // wait for it to exit, we know it returns 42 (see --exit_code=<> above)
        int exit_code = c.wait_for_exit(hpx::launch::sync);
        HPX_TEST_EQ(exit_code, 42);

        // make sure the launched process has set the message in the component
        // this should be the same as --set_message=<> above
        f = hpx::async(launch_process_get_message_action(), t);
        HPX_TEST_EQ(f.get(), std::string("accessed"));

    }    // release the component

    // the new locality should have disconnected now
    HPX_TEST_EQ(hpx::find_all_localities().size(), std::size_t(1));

    return hpx::finalize();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    // add command line option which controls the random number generator seed
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("launch,l", value<std::string>(),
        "the process that will be launched and which connects back");

    // This explicitly enables the component we depend on (it is disabled by
    // default to avoid being loaded outside of this test).
    std::vector<std::string> const cfg = {"hpx.expect_connecting_localities!=1",
        "hpx.components.launch_process_test_server.enabled!=1"};

    hpx::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::init(argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
#endif
