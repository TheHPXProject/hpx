
//  Copyright (c) 2011-2012 Bryce Adelstein-Lelbach
//  Copyright (c) 2007-2012 Hartmut Kaiser
//  Copyright (c) 2012-2013 Patricia Grubel
//
//  SPDX-License-Identifier: BSL-1.0 AND BSD-3-Clause
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Copyright (c) 2007, Sandia Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of the Sandia Corporation nor the names of its
//       contributors may be used to endorse or promote products derived from
//       this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
// EVENT SHALL SANDIA CORPORATION BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
// OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <hpx/config.hpp>

#include <hpx/modules/format.hpp>
#include <hpx/modules/timing.hpp>

#include <hpx/modules/program_options.hpp>

#include <atomic>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

#include <qthread/qthread.h>

using hpx::program_options::command_line_parser;
using hpx::program_options::notify;
using hpx::program_options::options_description;
using hpx::program_options::store;
using hpx::program_options::value;
using hpx::program_options::variables_map;

using hpx::chrono::high_resolution_timer;

///////////////////////////////////////////////////////////////////////////////
// Applications globals.
std::atomic<std::uint64_t> donecount(0);

// Command-line variables.
std::uint64_t tasks = 500000;
std::uint64_t delay = 0;
bool header = true;

// delay in seconds
double delay_sec = 0;
///////////////////////////////////////////////////////////////////////////////
void print_results(std::uint64_t cores, double walltime)
{
    if (header)
        std::cout << "OS-threads,Tasks,Delay (micro-seconds),"
                     "Total Walltime (seconds),Walltime per Task (seconds)\n";

    std::string const cores_str = hpx::util::format("{},", cores);
    std::string const tasks_str = hpx::util::format("{},", tasks);
    std::string const delay_str = hpx::util::format("{},", delay);

    hpx::util::format_to(std::cout, "{:-21} {:-21} {:-21} {:10.12}, {:10.12}\n",
        cores_str, tasks_str, delay_str, walltime, walltime / tasks);
}

///////////////////////////////////////////////////////////////////////////////
extern "C" aligned_t worker(void*)
{
    int volatile i = 0;

    //start timer
    high_resolution_timer td;

    while (true)
    {
        if (td.elapsed() > delay_sec)
            break;
        else
            ++i;
    }

    ++donecount;

    return aligned_t();
}

///////////////////////////////////////////////////////////////////////////////
int qthreads_main(variables_map& vm)
{
    if (vm.count("no-header"))
        header = false;

    //time in seconds
    delay_sec = (delay) * 1.0E-6;

    {
        // Validate command line.
        if (0 == tasks)
            throw std::invalid_argument("count of 0 tasks specified\n");

        // Start the clock.
        high_resolution_timer t;

        for (std::uint64_t i = 0; i < tasks; ++i)
            qthread_fork(&worker, nullptr, nullptr);

        // Yield until all our null qthreads are done.
        do
        {
            qthread_yield();
        } while (donecount != tasks);

        print_results(qthread_num_workers(), t.elapsed());
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
    // Parse command line.
    variables_map vm;

    options_description cmdline("Usage: " HPX_APPLICATION_STRING " [options]");

    cmdline.add_options()("help,h", "print out program usage (this message)")

        ("shepherds,s", value<std::uint64_t>()->default_value(1),
            "number of shepherds to use")

            ("workers-per-shepherd,w", value<std::uint64_t>()->default_value(1),
                "number of worker OS-threads per shepherd")

                ("tasks", value<std::uint64_t>(&tasks)->default_value(500000),
                    "number of tasks (e.g. qthreads) to invoke")

                    ("delay", value<std::uint64_t>(&delay)->default_value(0),
                        "delay in micro-seconds for the loop")

                        ("no-header", "do not print out the csv header row");
    ;

    store(command_line_parser(argc, argv).options(cmdline).run(), vm);

    notify(vm);

    // Print help screen.
    if (vm.count("help"))
    {
        std::cout << cmdline;
        return 0;
    }

    // Set qthreads environment variables.
    std::string const shepherds =
        std::to_string(vm["shepherds"].as<std::uint64_t>());
    std::string const workers =
        std::to_string(vm["workers-per-shepherd"].as<std::uint64_t>());

    setenv("QT_NUM_SHEPHERDS", shepherds.c_str(), 1);
    setenv("QT_NUM_WORKERS_PER_SHEPHERD", workers.c_str(), 1);

    // Setup the qthreads environment.
    if (qthread_initialize() != 0)
        throw std::runtime_error("qthreads failed to initialize\n");

    return qthreads_main(vm);
}
