//  Copyright (c) 2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This example demonstrates how the Process component can be used to launch
// arbitrary commands on any of the participating localities.

#include <hpx/hpx_main.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/process.hpp>
#include <hpx/include/runtime.hpp>

#include <iostream>
#include <vector>

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

int main()
{
    namespace process = hpx::components::process;

    // use hpx::find_all_localities(); if you want to include the current
    // locality as well
    std::vector<hpx::id_type> localities = hpx::find_remote_localities();
    std::vector<process::child> children;
    children.reserve(localities.size());

    for (auto const& locality : localities)
    {
#if defined(HPX_WINDOWS)
        std::string exe = "cmd.exe";
#else
        std::string exe = "ls";
#endif

        // set up command line for launched executable
        std::vector<std::string> args;
        args.push_back(exe);
#if defined(HPX_WINDOWS)
        args.push_back("/c");
        args.push_back("dir");
#endif

        // set up environment for launched executable (propagate current env)
        std::vector<std::string> env = get_environment();

        // launch test executable
        process::child c = process::execute(locality, process::run_exe(exe),
            process::set_args(args), process::set_env(env),
            process::start_in_dir("."), process::throw_on_error());

        children.push_back(c);
    }

    // wait for the processes to start executing
    hpx::wait_all(children);

    // wait for the processes to terminate
    for (auto& c : children)
    {
        int retval = c.wait_for_exit(hpx::launch::sync);
        std::cout << retval << '\n';
    }

    return 0;
}
