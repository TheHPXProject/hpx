//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "worker_timed.hpp"

#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/timing.hpp>

#include <chrono>
#include <cstdint>
#include <ctime>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <hpx/modules/program_options.hpp>

char const* benchmark_name = "Delay Baseline";

using hpx::program_options::command_line_parser;
using hpx::program_options::notify;
using hpx::program_options::options_description;
using hpx::program_options::store;
using hpx::program_options::value;
using hpx::program_options::variables_map;

using hpx::chrono::high_resolution_timer;

using std::cout;

///////////////////////////////////////////////////////////////////////////////
std::uint64_t threads = 1;
std::uint64_t tasks = 500000;
std::uint64_t delay = 5;
bool header = true;

///////////////////////////////////////////////////////////////////////////////
std::string format_build_date()
{
    std::chrono::time_point<std::chrono::system_clock> now =
        std::chrono::system_clock::now();

    std::time_t current_time = std::chrono::system_clock::to_time_t(now);

    std::string ts = std::ctime(&current_time);
    ts.resize(ts.size() - 1);    // remove trailing '\n'
    return ts;
}

///////////////////////////////////////////////////////////////////////////////
void print_results(variables_map& vm, double sum_, double mean_)
{
    if (header)
    {
        cout << "# BENCHMARK: " << benchmark_name << "\n";

        cout << "# VERSION: " << format_build_date() << "\n"
             << "#\n";

        // Note that if we change the number of fields above, we have to
        // change the constant that we add when printing out the field # for
        // performance counters below (e.g. the last_index part).
        cout << "## 0:DELAY:Delay [micro-seconds] - Independent Variable\n"
                "## 1:TASKS:# of Tasks - Independent Variable\n"
                "## 2:OSTHRDS:OS-threads - Independent Variable\n"
                "## 3:WTIME_THR:Total Walltime/Thread [micro-seconds]\n";
    }

    hpx::util::format_to(
        cout, "{} {} {} {:.14g}\n", delay, tasks, threads, mean_);
}

///////////////////////////////////////////////////////////////////////////////
void invoke_n_workers_nowait(double& elapsed, std::uint64_t workers)
{
    // Warmup.
    for (std::uint64_t i = 0; i < tasks; ++i)
    {
        worker_timed(delay * 1000);
    }

    for (std::uint64_t i = 0; i < tasks; ++i)
    {
        worker_timed(delay * 1000);
    }

    // Start the clock.
    high_resolution_timer t;

    for (std::uint64_t i = 0; i < tasks; ++i)
    {
        worker_timed(delay * 1000);
    }

    elapsed = t.elapsed();
}

void invoke_n_workers(
    hpx::util::barrier& b, double& elapsed, std::uint64_t workers)
{
    b.wait();

    invoke_n_workers_nowait(elapsed, workers);
}

///////////////////////////////////////////////////////////////////////////////
int app_main(variables_map& vm)
{
    if (0 == tasks)
        throw std::invalid_argument("error: count of 0 tasks specified\n");

    std::vector<double> elapsed(threads - 1);
    std::vector<std::thread> workers;
    hpx::util::barrier b(threads - 1);

    for (std::uint32_t i = 0; i != threads - 1; ++i)
    {
        workers.push_back(std::thread(
            invoke_n_workers, std::ref(b), std::ref(elapsed[i]), tasks));
    }

    double total_elapsed = 0;

    invoke_n_workers_nowait(total_elapsed, tasks);

    for (std::thread& thread : workers)
    {
        if (thread.joinable())
            thread.join();
    }

    for (std::uint64_t i = 0; i < elapsed.size(); ++i)
    {
        //cout << i << " " << elapsed[i] << "\n";
        total_elapsed += elapsed[i];
    }

    // Print out the results.
    print_results(vm, total_elapsed / double(threads),
        (total_elapsed * 1e6) / double(tasks * threads));

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

        ("threads,t", value<std::uint64_t>(&threads)->default_value(1),
            "number of threads to use")

            ("tasks", value<std::uint64_t>(&tasks)->default_value(500000),
                "number of tasks to invoke")

                ("delay", value<std::uint64_t>(&delay)->default_value(5),
                    "duration of delay in microseconds")

                    ("no-header", "do not print out the csv header row");

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

    return app_main(vm);
}
