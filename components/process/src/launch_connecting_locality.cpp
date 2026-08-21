//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/filesystem.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/runtime_local.hpp>

#include <hpx/components/process/child.hpp>
#include <hpx/components/process/process.hpp>
#include <hpx/components/process/util/initializers.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace {

    ///////////////////////////////////////////////////////////////////////////////
    int get_arraylen(char** arr)
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
        int const len = get_arraylen(_environ);
        std::copy(&_environ[0], &_environ[len], std::back_inserter(env));
#elif defined(linux) || defined(__linux) || defined(__linux__) ||              \
    defined(__AIX__) || defined(__APPLE__) || defined(__FreeBSD__)
        int const len = get_arraylen(environ);
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

        env.erase(std::ranges::remove_if(env,
                      [&prefix](std::string const& entry) {
                          return entry.starts_with(prefix);
                      })
                      .begin(),
            env.end());

        env.push_back(prefix + value);
    }

}    // namespace

namespace hpx::components::process {

    // Launch the given executable as a connecting HPX locality
    child launch_connecting_locality(std::string const& to_launch,
        std::vector<std::string> const& outer_args,
        std::vector<std::string> const& outer_env,
        std::optional<std::uint64_t> spawn_id)
    {
        hpx::filesystem::path const exe(to_launch);

        std::vector<std::string> args;
        args.emplace_back(hpx::filesystem::to_string(exe));
        args.emplace_back("--hpx:ignore-batch-env");
        args.emplace_back("--hpx:run-hpx-main");

        // Force use of the TCP parcelport, which is the one that binds a port.
        args.emplace_back("--hpx:ini=hpx.parcel.tcp.priority=1000");
        args.emplace_back("--hpx:ini=hpx.parcel.bootstrap=tcp");

        // optionally pass additional arguments
        args.insert(args.end(), outer_args.begin(), outer_args.end());

        std::string const address =
            hpx::get_config_entry("hpx.agas.address", HPX_INITIAL_IP_ADDRESS);

        std::vector<std::string> env = get_environment();

        // optionally pass additional environment entries first
        std::ranges::for_each(outer_env, [&](std::string const& e) {
            if (std::size_t const pos = e.find('='); pos != std::string::npos)
            {
                set_env_var(env, e.substr(0, pos), e.substr(pos + 1));
            }
            else
            {
                set_env_var(env, e, "");
            }
        });

        // Apply launcher-managed values last.
        set_env_var(env, "HPX_AGAS_SERVER_ADDRESS", address);
        set_env_var(env, "HPX_AGAS_SERVER_PORT",
            hpx::get_config_entry(
                "hpx.agas.port", std::to_string(HPX_INITIAL_IP_PORT)));

        // Hand the launched locality port 0 and let the operating system pick.
        // it registered.
        set_env_var(env, "HPX_PARCEL_SERVER_ADDRESS", address);
        set_env_var(env, "HPX_PARCEL_SERVER_PORT", "0");

        std::string stem(hpx::filesystem::to_string(exe.stem()));
        if (spawn_id.has_value())
        {
            stem += "/" + std::to_string(*spawn_id);
        }
        else
        {
            // Ensure every launch without an explicit spawn_id still gets a
            // unique latch name, avoiding collisions between concurrent
            // launches of the same executable.
            static std::atomic<std::uint64_t> next_launch_id{0};
            stem += "/" + std::to_string(next_launch_id++);
        }
        set_env_var(env, "HPX_ON_STARTUP_WAIT_ON_LATCH", stem);

        // clang-format off
        process::child c = process::execute(hpx::find_here(),
            process::run_exe(hpx::filesystem::to_string(to_launch)),
            process::set_args(args),
            process::set_env(env),
            process::start_in_dir(
                hpx::filesystem::to_string(exe.parent_path())),
            process::throw_on_error(),
            process::wait_on_latch(stem));
        // clang-format on

        return c;
    }
}    // namespace hpx::components::process
