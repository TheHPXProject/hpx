//  Copyright (c) 2006, Giovanni P. Deretta
//  Copyright (c) 2012 Hartmut Kaiser
//  Copyright (c) 2011-2014 Bryce Adelstein-Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>

#include <hpx/modules/format.hpp>
#include <hpx/modules/string_util.hpp>

#include <chrono>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "worker_timed.hpp"

char const* benchmark_name = "Context Switching Overhead - HPX";

using namespace hpx::program_options;
using namespace hpx::threads;

using hpx::threads::coroutine_type;
using std::cout;

///////////////////////////////////////////////////////////////////////////////
std::uint64_t payload = 0;
std::uint64_t contexts = 1000;
std::uint64_t iterations = 100000;
std::uint64_t seed = 0;
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
void print_results(double w_M)
{
    if (header)
    {
        cout << "# BENCHMARK: " << benchmark_name << "\n";

        cout << "# VERSION: " << HPX_HAVE_GIT_COMMIT << " "
             << format_build_date() << "\n"
             << "#\n";

        // Note that if we change the number of fields above, we have to
        // change the constant that we add when printing out the field # for
        // performance counters below (e.g. the last_index part).
        cout << "## 0:PLOAD:Payload [micro-seconds] - Independent Variable\n"
                "## 1:OSTHRDS:OS-Threads - Independent Variable\n"
                "## 2:CTXS:# of Contexts - Independent Variable\n"
                "## 3:ITER:# of Iterations - Independent Variable\n"
                "## 4:SEED:PRNG seed - Independent Variable\n"
                "## 5:WTIME_CS:Walltime/Context Switch [nano-seconds]\n";
    }

    std::uint64_t const os_thread_count = hpx::get_os_thread_count();

    double w_T = static_cast<double>(iterations) *
        static_cast<double>(payload) * static_cast<double>(os_thread_count) *
        1e-6;
    //     double E = w_T/w_M;
    double O = w_M - w_T;

    hpx::util::format_to(cout, "{} {} {} {} {} {:.14g}", payload,
        os_thread_count, contexts, iterations, seed,
        (O /
            (2.0 * static_cast<double>(iterations) *
                static_cast<double>(os_thread_count))) *
            1e9);

    cout << "\n";
}

///////////////////////////////////////////////////////////////////////////////
struct kernel
{
    hpx::threads::thread_result_type operator()(thread_restart_state) const
    {
        worker_timed(payload * 1000);

        return hpx::threads::thread_result_type(
            hpx::threads::thread_schedule_state::pending,
            hpx::threads::invalid_thread_id);
    }

    bool operator!() const
    {
        return true;
    }
};

double perform_2n_iterations()
{
    std::vector<coroutine_type*> coroutines;
    std::vector<std::uint64_t> indices;

    coroutines.reserve(contexts);
    indices.reserve(iterations);

    std::mt19937_64 prng(seed);
    std::uniform_int_distribution<std::uint64_t> dist(0, contexts - 1);

    kernel k;

    for (std::uint64_t i = 0; i < contexts; ++i)
    {
        coroutine_type* c =
            new coroutine_type(k, hpx::threads::invalid_thread_id);
        coroutines.push_back(c);
    }

    for (std::uint64_t i = 0; i < iterations; ++i)
        indices.push_back(dist(prng));

    ///////////////////////////////////////////////////////////////////////
    // Warmup
    for (std::uint64_t i = 0; i < iterations; ++i)
    {
        (*coroutines[indices[i]])(hpx::threads::thread_restart_state::signaled);
    }

    hpx::chrono::high_resolution_timer t;

    for (std::uint64_t i = 0; i < iterations; ++i)
    {
        (*coroutines[indices[i]])(hpx::threads::thread_restart_state::signaled);
    }

    double elapsed = t.elapsed();

    for (std::uint64_t i = 0; i < contexts; ++i)
    {
        delete coroutines[i];
    }

    coroutines.clear();

    return elapsed;
}

int hpx_main(variables_map& vm)
{
    {
        if (vm.count("no-header"))
            header = false;

        if (!seed)
            seed = std::uint64_t(std::time(nullptr));

        std::uint64_t const os_thread_count = hpx::get_os_thread_count();

        std::vector<hpx::shared_future<double>> futures;

        std::uint64_t num_thread = hpx::get_worker_thread_num();

        for (std::uint64_t i = 0; i < os_thread_count; ++i)
        {
            if (num_thread == i)
                continue;

            futures.push_back(hpx::async(&perform_2n_iterations));
        }

        double total_elapsed = perform_2n_iterations();

        for (std::uint64_t i = 0; i < futures.size(); ++i)
            total_elapsed += futures[i].get();

        print_results(total_elapsed);
    }

    return hpx::finalize();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    // Configure application-specific options.
    options_description cmdline("usage: " HPX_APPLICATION_STRING " [options]");

    cmdline.add_options()("payload",
        value<std::uint64_t>(&payload)->default_value(0),
        "artificial delay of each coroutine")

        ("contexts", value<std::uint64_t>(&contexts)->default_value(100000),
            "number of contexts use")

            ("iterations",
                value<std::uint64_t>(&iterations)->default_value(100000),
                "number of iterations to invoke (2 * iterations context "
                "switches "
                "will occur)")

                ("seed", value<std::uint64_t>(&seed)->default_value(0),
                    "seed for the pseudo random number generator (if 0, a seed "
                    "is "
                    "chosen based on the current system time)")

                    ("no-header", "do not print out the header");

    // Initialize and run HPX.
    hpx::init_params init_args;
    init_args.desc_cmdline = cmdline;

    return hpx::init(argc, argv, init_args);
}
