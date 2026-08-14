//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// A locality that is given parcelport port 0 asks the operating system to
// choose one for it. This test launches such a locality and requires it to
// become reachable, which it can only be if the port it actually bound is
// what reaches AGAS.
//
// Before the fix for #7406 the parcelport published the port from its
// configuration rather than the port its acceptor bound. With port 0 that
// published a locality listening on an ephemeral port while advertising port
// zero, so the launched locality connected outwards but nothing could reach
// it: this test hung until it was killed rather than failing.

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
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <sstream>
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

// Replace any inherited entry for the given name. The environment is handed to
// execve verbatim and getenv returns the first match, so appending alone would
// leave an inherited entry shadowing the value set here.
void set_env_var(std::vector<std::string>& env, std::string const& name,
    std::string const& value)
{
    std::string const prefix = name + "=";

    env.erase(std::remove_if(env.begin(), env.end(),
                  [&prefix](std::string const& entry) {
                      return entry.starts_with(prefix);
                  }),
        env.end());

    env.push_back(prefix + value);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    namespace process = hpx::components::process;
    namespace fs = hpx::filesystem;

    fs::path base_dir = hpx::util::find_prefix();
    base_dir /= "bin";

    fs::path exe = base_dir /
        "os_assigned_parcelport_port_7406_child" HPX_EXECUTABLE_EXTENSION;

    if (vm.count("launch"))
        exe = vm["launch"].as<std::string>();

    std::vector<std::string> args;
    args.push_back(hpx::filesystem::to_string(exe));
    args.push_back("--hpx:ignore-batch-env");
    args.push_back("--hpx:threads=1");
    // Force use of the TCP parcelport, which is the one that binds a port.
    args.push_back("--hpx:ini=hpx.parcel.tcp.priority=1000");
    args.push_back("--hpx:ini=hpx.parcel.bootstrap=tcp");

    std::vector<std::string> env = get_environment();

    std::string const address =
        hpx::get_config_entry("hpx.agas.address", HPX_INITIAL_IP_ADDRESS);

    set_env_var(env, "HPX_AGAS_SERVER_ADDRESS", address);
    set_env_var(env, "HPX_AGAS_SERVER_PORT",
        hpx::get_config_entry(
            "hpx.agas.port", std::to_string(HPX_INITIAL_IP_PORT)));

    // This is the point of the test: hand the launched locality port 0 and
    // let the operating system pick. Nothing here knows which port that will
    // be, so the only way the locality below becomes reachable is if the port
    // it bound is the port it registered.
    set_env_var(env, "HPX_PARCEL_SERVER_ADDRESS", address);
    set_env_var(env, "HPX_PARCEL_SERVER_PORT", "0");

    set_env_var(env, "HPX_ON_STARTUP_WAIT_ON_LATCH",
        "os_assigned_parcelport_port_7406");

    hpx::distributed::latch sync(2);
    sync.register_as("os_assigned_parcelport_port_7406_sync");

    process::child c = process::execute(hpx::find_here(),
        process::run_exe(hpx::filesystem::to_string(exe)),
        process::set_args(args), process::set_env(env),
        process::start_in_dir(hpx::filesystem::to_string(base_dir)),
        process::throw_on_error(),
        process::wait_on_latch("os_assigned_parcelport_port_7406"));

    c.wait();
    HPX_TEST(c);

    // the launched locality has joined
    hpx::id_type const here = hpx::find_here();
    std::vector<hpx::id_type> const localities = hpx::find_all_localities();
    HPX_TEST_EQ(localities.size(), std::size_t(2));

    hpx::id_type launched;
    for (hpx::id_type const& id : localities)
    {
        if (id != here)
            launched = id;
    }
    HPX_TEST(launched != hpx::invalid_id);

    // AGAS has to hold the endpoint the locality actually bound, not the one
    // it was configured with. Advertising the configured port would register
    // this locality on port zero.
    {
        hpx::error_code ec;
        auto const& endpoints = hpx::naming::get_agas_client().resolve_locality(
            launched.get_gid(), ec);
        HPX_TEST(!ec);
        HPX_TEST(!endpoints.empty());

        for (auto const& ep : endpoints)
        {
            std::ostringstream os;
            os << ep.second;

            // the locality prints as "address:port"
            HPX_TEST(!os.str().ends_with(":0"));
        }
    }

    // Releasing the latch is what proves the locality is reachable: the
    // launched locality is blocked in arrive_and_wait, and only a parcel from
    // here can free it. That parcel has to arrive at the port the operating
    // system chose, which can only happen if that is the port the locality
    // registered. A locality that published its configured port instead is
    // never woken and this test hangs.
    sync.arrive_and_wait();

    int const exit_code = c.wait_for_exit(hpx::launch::sync);
    HPX_TEST_EQ(exit_code, 0);

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;
    options_description desc_commandline(
        "usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("launch", value<std::string>(),
        "the process to launch as an additional locality");

    hpx::init_params init_args;
    init_args.desc_cmdline = desc_commandline;

    HPX_TEST_EQ(hpx::init(argc, argv, init_args), 0);
    return hpx::util::report_errors();
}
#endif
