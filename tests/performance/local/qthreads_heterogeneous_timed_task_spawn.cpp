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

#include <hpx/assert.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/timing.hpp>

#include <hpx/modules/program_options.hpp>

#include <atomic>
#include <cstdint>
#include <functional>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <qthread/qthread.h>

#include "worker_timed.hpp"

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
std::uint64_t min_delay = 0;
std::uint64_t max_delay = 0;
std::uint64_t total_delay = 0;
std::uint64_t seed = 0;
bool header = false;

///////////////////////////////////////////////////////////////////////////////
void print_results(std::uint64_t cores, double walltime)
{
    if (header)
        std::cout << "OS-threads,Seed,Tasks,Minimum Delay (iterations),"
                     "Maximum Delay (iterations),Total Delay (iterations),"
                     "Total Walltime (seconds),Walltime per Task (seconds)\n";

    std::string const cores_str = hpx::util::format("{},", cores);
    std::string const seed_str = hpx::util::format("{},", seed);
    std::string const tasks_str = hpx::util::format("{},", tasks);

    std::string const min_delay_str = hpx::util::format("{},", min_delay);
    std::string const max_delay_str = hpx::util::format("{},", max_delay);
    std::string const total_delay_str = hpx::util::format("{},", total_delay);

    hpx::util::format_to(std::cout,
        "{:-21} {:-21} {:-21} {:-21} {:-21} {:-21} {:10.12}\n", cores_str,
        seed_str, tasks_str, min_delay_str, max_delay_str, total_delay_str,
        walltime, walltime / tasks);
}

///////////////////////////////////////////////////////////////////////////////
std::uint64_t shuffler(std::mt19937_64& prng, std::uint64_t high)
{
    if (high == 0)
        throw std::logic_error("high value was 0");

    // Our range is [0, x).
    std::uniform_int_distribution<std::uint64_t> dist(0, high - 1);

    return dist(prng);
}

///////////////////////////////////////////////////////////////////////////////
extern "C" aligned_t worker_func(void* p)
{
    std::uint64_t const delay_ = reinterpret_cast<std::uint64_t>(p);

    worker_timed(delay_ * 1000);

    ++donecount;

    return aligned_t();
}

///////////////////////////////////////////////////////////////////////////////
int qthreads_main(variables_map& vm)
{
    {
        ///////////////////////////////////////////////////////////////////////
        // Initialize the PRNG seed.
        if (!seed)
            seed = std::uint64_t(std::time(nullptr));

        ///////////////////////////////////////////////////////////////////////
        // Validate command-line arguments.
        if (0 == tasks)
            throw std::invalid_argument("count of 0 tasks specified\n");

        if (min_delay > max_delay)
            throw std::invalid_argument("minimum delay cannot be larger than "
                                        "maximum delay\n");

        if (min_delay > total_delay)
            throw std::invalid_argument("minimum delay cannot be larger than"
                                        "total delay\n");

        if (max_delay > total_delay)
            throw std::invalid_argument("maximum delay cannot be larger than "
                                        "total delay\n");

        if ((min_delay * tasks) > total_delay)
            throw std::invalid_argument("minimum delay is too small for the "
                                        "specified total delay and number of "
                                        "tasks\n");

        if ((max_delay * tasks) < total_delay)
            throw std::invalid_argument("maximum delay is too small for the "
                                        "specified total delay and number of "
                                        "tasks\n");

        ///////////////////////////////////////////////////////////////////////
        // Randomly generate a description of the heterogeneous workload.
        std::vector<std::uint64_t> payloads;
        payloads.reserve(tasks);

        // For random numbers, we use a 64-bit specialization of stdlib's
        // mersenne twister engine (good uniform distribution up to 311
        // dimensions, cycle length 2 ^ 19937 - 1)
        std::mt19937_64 prng(seed);

        std::uint64_t current_sum = 0;

        for (std::uint64_t i = 0; i < tasks; ++i)
        {
            // Credit to Spencer Ruport for putting this algorithm on
            // stackoverflow.
            std::uint64_t const low_calc =
                (total_delay - current_sum) - (max_delay * (tasks - 1 - i));

            bool const negative =
                (total_delay - current_sum) < (max_delay * (tasks - 1 - i));

            std::uint64_t const low =
                (negative || (low_calc < min_delay)) ? min_delay : low_calc;

            std::uint64_t const high_calc =
                (total_delay - current_sum) - (min_delay * (tasks - 1 - i));

            std::uint64_t const high =
                (high_calc > max_delay) ? max_delay : high_calc;

            // Our range is [low, high].
            std::uniform_int_distribution<std::uint64_t> dist(low, high);

            std::uint64_t const payload = dist(prng);

            if (payload < min_delay)
                throw std::logic_error("task delay is below minimum");

            if (payload > max_delay)
                throw std::logic_error("task delay is above maximum");

            current_sum += payload;
            payloads.push_back(payload);
        }

        // Randomly shuffle the entire sequence to deal with drift.
        std::random_device random_device;
        std::mt19937 generator(random_device());
        std::shuffle(payloads.begin(), payloads.end(), std::move(generator));

        ///////////////////////////////////////////////////////////////////////
        // Validate the payloads.
        if (payloads.size() != tasks)
            throw std::logic_error("incorrect number of tasks generated");

        std::uint64_t const payloads_sum =
            std::accumulate(payloads.begin(), payloads.end(), 0LLU);
        if (payloads_sum != total_delay)
            throw std::logic_error("incorrect total delay generated");

        ///////////////////////////////////////////////////////////////////////
        // Start the clock.
        high_resolution_timer t;

        ///////////////////////////////////////////////////////////////////////
        // Queue the tasks in a serial loop.
        for (std::uint64_t i = 0; i < tasks; ++i)
        {
            void* const ptr = reinterpret_cast<void*>(payloads[i]);
            qthread_fork(&worker_func, ptr, nullptr);
        }

        ///////////////////////////////////////////////////////////////////////
        // Wait for the work to finish.
        do
        {
            // Yield until all our null qthreads are done.
            qthread_yield();
        } while (donecount != tasks);

        ///////////////////////////////////////////////////////////////////////
        // Print the results.
        print_results(qthread_num_workers(), t.elapsed());
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    ///////////////////////////////////////////////////////////////////////////
    // Parse command line.
    variables_map vm;

    options_description cmdline("Usage: " HPX_APPLICATION_STRING " [options]");

    cmdline.add_options()("help,h", "print out program usage (this message)")

        ("shepherds,s", value<std::uint64_t>()->default_value(1),
            "number of shepherds to use")

            ("workers-per-shepherd,w", value<std::uint64_t>()->default_value(1),
                "number of worker OS-threads per shepherd")

                ("tasks", value<std::uint64_t>(&tasks)->default_value(500000),
                    "number of tasks to invoke")

                    ("min-delay",
                        value<std::uint64_t>(&min_delay)->default_value(0),
                        "minimum number of iterations in the delay loop")

                        ("max-delay",
                            value<std::uint64_t>(&max_delay)->default_value(0),
                            "maximum number of iterations in the delay loop")

                            ("total-delay",
                                value<std::uint64_t>(&total_delay)
                                    ->default_value(0),
                                "total number of delay iterations to be "
                                "executed")

                                ("seed",
                                    value<std::uint64_t>(&seed)->default_value(
                                        0),
                                    "seed for the pseudo random number "
                                    "generator (if 0, a seed is "
                                    "chosen based on the current system time)")

                                    ("no-header",
                                        "do not print out the csv header row");

    store(command_line_parser(argc, argv).options(cmdline).run(), vm);

    notify(vm);

    // Print help screen.
    if (vm.count("help"))
    {
        std::cout << cmdline;
        return 0;
    }

    if (vm.count("no-header"))
        header = false;

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
