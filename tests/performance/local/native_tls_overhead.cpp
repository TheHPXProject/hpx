//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/timing.hpp>

#include <cstdint>
#include <functional>
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

///////////////////////////////////////////////////////////////////////////////
// thread local globals
static thread_local double* global_scratch;

///////////////////////////////////////////////////////////////////////////////
inline void worker(hpx::util::barrier& b, std::uint64_t updates)
{
    b.wait();

    for (double i = 0.; i < static_cast<double>(updates); ++i)
    {
        global_scratch = new double;

        *global_scratch += 1. / (2. * i * (*global_scratch) + 1.);

        delete global_scratch;
    }
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
    ///////////////////////////////////////////////////////////////////////////
    // parse command line
    variables_map vm;

    options_description cmdline("Usage: " HPX_APPLICATION_STRING " [options]");

    std::uint32_t threads, updates;

    cmdline.add_options()("help,h", "print out program usage (this message)")

        ("threads,t", value<std::uint32_t>(&threads)->default_value(1),
            "number of OS-threads")

            ("updates,u",
                value<std::uint32_t>(&updates)->default_value(1 << 22),
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

    hpx::util::barrier b(threads);

    high_resolution_timer t;

    for (std::uint32_t i = 0; i != threads; ++i)
        workers.push_back(std::thread(worker, std::ref(b), updates));

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
