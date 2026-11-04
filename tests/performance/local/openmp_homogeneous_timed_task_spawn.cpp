//  Copyright (c) 2011-2012 Bryce Adelstein-Lelbach
//  Copyright (c) 2007-2012 Hartmut Kaiser
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
#include <hpx/assert.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/timing.hpp>

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

#include <omp.h>

#include "worker_timed.hpp"

using hpx::program_options::command_line_parser;
using hpx::program_options::notify;
using hpx::program_options::options_description;
using hpx::program_options::store;
using hpx::program_options::value;
using hpx::program_options::variables_map;

using hpx::chrono::high_resolution_timer;

///////////////////////////////////////////////////////////////////////////////
// Command-line variables.
std::uint64_t tasks = 500000;
std::uint64_t delay = 0;
bool header = true;

///////////////////////////////////////////////////////////////////////////////
void print_results(int cores, double walltime)
{
    if (header)
        std::cout << "OS-threads,Tasks,Delay (iterations),"
                     "Total Walltime (seconds),Walltime per Task (seconds)\n";

    std::string const cores_str = hpx::util::format("{},", cores);
    std::string const tasks_str = hpx::util::format("{},", tasks);
    std::string const delay_str = hpx::util::format("{},", delay);

    hpx::util::format_to(std::cout, "{:-21} {:-21} {:-21} {:10.12}, {:10.12}\n",
        cores_str, tasks_str, delay_str, walltime, walltime / tasks);
}

///////////////////////////////////////////////////////////////////////////////
int omp_main(variables_map&)
{
    // Validate command line.
    if (0 == tasks)
        throw std::invalid_argument("count of 0 tasks specified\n");

    // Start the clock.
    high_resolution_timer t;

#pragma omp parallel
#pragma omp single
    {
        for (std::uint64_t i = 0; i < tasks; ++i)
#if _OPENMP >= 200805
#pragma omp task untied
#endif
            worker_timed(delay * 1000);

// Yield until all work is done.
#if _OPENMP >= 200805
#pragma omp taskwait
#endif

        print_results(omp_get_num_threads(), t.elapsed());
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
    // Parse command line.
    variables_map vm;

    options_description cmdline("Usage: " HPX_APPLICATION_STRING " [options]");

    // clang-format off
    cmdline.add_options()
        ( "help,h"
        , "print out program usage (this message)")

        ( "threads,t"
        , value<int>()->default_value(1),
         "number of OS-threads to use")

        ( "tasks"
        , value<std::uint64_t>(&tasks)->default_value(500000)
        , "number of tasks to invoke")

        ( "delay"
        , value<std::uint64_t>(&delay)->default_value(0)
        , "number of iterations in the delay loop")

        ( "no-header"
        , "do not print out the csv header row")
        ;
     ;
     //clang-format on

    store(command_line_parser(argc, argv)
              .allow_unregistered()
              .options(cmdline)
              .run(),
        vm);

    notify(vm);

    // Print help screen.
    if (vm.count("help"))
    {
        std::cout << cmdline;
        return 0;
    }

    if (vm.count("no-header"))
        header = false;

    // Setup the OMP environment.
    omp_set_num_threads(vm["threads"].as<int>());

    return omp_main(vm);
}

