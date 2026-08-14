//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/timing.hpp>

#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

using hpx::program_options::command_line_parser;
using hpx::program_options::notify;
using hpx::program_options::options_description;
using hpx::program_options::store;
using hpx::program_options::value;
using hpx::program_options::variables_map;

using hpx::chrono::high_resolution_timer;
using hpx::util::thread_specific_ptr;

///////////////////////////////////////////////////////////////////////////////
// thread local globals
struct tag
{
};
static thread_specific_ptr<double, tag> global_scratch;

///////////////////////////////////////////////////////////////////////////////
inline void worker(std::uint64_t updates)
{
    global_scratch.reset(new double);

    for (double i = 0.; i < static_cast<double>(updates); ++i)
        *global_scratch += 1. / (2. * i + 1.);

    global_scratch.reset();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
    ///////////////////////////////////////////////////////////////////////////
    // parse command line
    variables_map vm;

    options_description cmdline("Usage: " HPX_APPLICATION_STRING " [options]");

    std::uint64_t threads, updates;

    cmdline.add_options()("help,h", "print out program usage (this message)")

        ("threads,t", value<std::uint64_t>(&threads)->default_value(1),
            "number of OS-threads")

            ("updates,u",
                value<std::uint64_t>(&updates)->default_value(1 << 22),
                "updates made to the TLS variable per OS-thread")

                ("csv",
                    "output results as csv (format: "
                    "updates,OS-threads,duration)");
    ;

    store(command_line_parser(argc, argv).options(cmdline).run(), vm);

    notify(vm);

    ///////////////////////////////////////////////////////////////////////////
    // print help screen
    if (vm.count("help"))
    {
        std::cout << cmdline;
        return 0;
    }

    ///////////////////////////////////////////////////////////////////////////
    // run the test
    std::vector<std::thread> workers;

    high_resolution_timer t;

    for (std::uint64_t i = 0; i != threads; ++i)
        workers.push_back(std::thread(worker, updates));

    for (std::thread& thread : workers)
    {
        if (thread.joinable())
            thread.join();
    }

    double const duration = t.elapsed();

    ///////////////////////////////////////////////////////////////////////////
    // output results
    if (vm.count("csv"))
        hpx::util::format_to(
            std::cout, "{1},{2},{3}\n", updates, threads, duration);
    else
        hpx::util::format_to(std::cout,
            "ran {1} updates per OS-thread on {2} "
            "OS-threads in {3} seconds\n",
            updates, threads, duration);
}
