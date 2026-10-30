//  Copyright (c) 2014-2015 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This is the first in a series of examples demonstrating the development of a
// fully distributed solver for a simple 1D heat distribution problem.
//
// This example provides a serial base line implementation. No parallelization
// is performed.
//
// The only difference to 1d_stencil_1 is that this example uses OpenMP for
// parallelizing the inner loop.

// Include before any Boost header and avoid issues with Intel14/libstdc++4.4
// nullptr
#include <hpx/config.hpp>
#include <hpx/modules/program_options.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

#include <omp.h>

#include "print_time_results.hpp"

///////////////////////////////////////////////////////////////////////////////
// Timer with nanosecond resolution
inline std::uint64_t now()
{
    std::chrono::nanoseconds ns =
        std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(ns.count());
}

///////////////////////////////////////////////////////////////////////////////
bool header = true;    // print csv heading
double k = 0.5;        // heat transfer coefficient
double dt = 1.;        // time step
double dx = 1.;        // grid spacing

///////////////////////////////////////////////////////////////////////////////
struct stepper
{
    // Our partition type
    typedef double partition;

    // Our data for one time step
    typedef std::vector<partition> space;

    // Our operator
    static double heat(double left, double middle, double right)
    {
        return middle + (k * dt / (dx * dx)) * (left - 2 * middle + right);
    }

    // do all the work on 'nx' data points for 'nt' time steps
    space do_work(std::size_t nx, std::size_t nt)
    {
        // U[t][i] is the state of position i at time t.
        std::vector<space> U(2);
        for (space& s : U)
            s.resize(nx);

        // Initial conditions: f(0, i) = i
        for (std::size_t i = 0; i != nx; ++i)
            U[0][i] = double(i);

        // Actual time step loop
        for (std::size_t t = 0; t != nt; ++t)
        {
            space const& current = U[t % 2];
            space& next = U[(t + 1) % 2];

            next[0] = heat(current[nx - 1], current[0], current[1]);

// Visual Studio requires OMP loop variables to be signed :/
#pragma omp parallel for
            for (std::int64_t i = 1; i < std::int64_t(nx - 1); ++i)
                next[i] = heat(current[i - 1], current[i], current[i + 1]);

            next[nx - 1] = heat(current[nx - 2], current[nx - 1], current[0]);
        }

        // Return the solution at time-step 'nt'.
        return U[nt % 2];
    }
};

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    std::size_t nx = vm["nx"].as<std::size_t>();    // Number of grid points.
    std::size_t nt = vm["nt"].as<std::size_t>();    // Number of steps.

    if (vm.count("no-header"))
        header = false;

    // Create the stepper object
    stepper step;

    // Measure execution time.
    std::uint64_t t = now();

    // Execute nt time steps on nx grid points.
    stepper::space solution = step.do_work(nx, nt);

    // Print the final solution
    if (vm.count("results"))
    {
        for (std::size_t i = 0; i != nx; ++i)
            std::cout << "U[" << i << "] = " << solution[i] << std::endl;
    }

    std::uint64_t elapsed = now() - t;

    std::uint64_t const os_thread_count = omp_get_num_threads();
    print_time_results(os_thread_count, elapsed, nx, nt, header);

    return 0;
}

int main(int argc, char* argv[])
{
    namespace po = hpx::program_options;

    po::options_description desc_commandline;
    // clang-format off
    desc_commandline.add_options()
        ("results", "print generated results (default: false)")
        ("nx", po::value<std::size_t>()->default_value(100),
         "Local x dimension")
        ("nt", po::value<std::size_t>()->default_value(45),
         "Number of time steps")
        ("k", po::value<double>(&k)->default_value(0.5),
         "Heat transfer coefficient (default: 0.5)")
        ("dt", po::value<double>(&dt)->default_value(1.0),
         "Timestep unit (default: 1.0[s])")
        ("dx", po::value<double>(&dx)->default_value(1.0),
         "Local x dimension")
        ( "no-header", "do not print out the csv header row")
    ;
    // clang-format on

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
                  .options(desc_commandline)
                  .allow_unregistered()
                  .run(),
        vm);
    po::notify(vm);

    return hpx_main(vm);
}
