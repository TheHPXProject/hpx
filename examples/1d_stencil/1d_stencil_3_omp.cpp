//  Copyright (c) 2014-2015 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This is the third in a series of examples demonstrating the development of a
// fully distributed solver for a simple 1D heat distribution problem.
//
// This example takes the code from example one and introduces a partitioning
// of the 1D grid into groups of grid partitions which are handled at the same time.
// The purpose is to be able to control the amount of work performed. The
// example is still fully serial, no parallelization is performed.
//
// The only difference to 1d_stencil_3 is that this example uses OpenMP for
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

inline std::size_t idx(std::size_t i, std::size_t size)
{
    return (std::int64_t(i) < 0) ? (i + size) % size : i % size;
}

///////////////////////////////////////////////////////////////////////////////
// Our partition_data data type
struct partition_data
{
    partition_data(std::size_t size = 0)
      : data_(size)
    {
    }

    partition_data(std::size_t size, double initial_value)
      : data_(size)
    {
        double base_value = initial_value * double(size);
        for (std::ptrdiff_t i = 0; i != static_cast<std::ptrdiff_t>(size); ++i)
            data_[i] = base_value + double(i);
    }

    double& operator[](std::size_t idx)
    {
        return data_[static_cast<std::ptrdiff_t>(idx)];
    }
    double operator[](std::size_t idx) const
    {
        return data_[static_cast<std::ptrdiff_t>(idx)];
    }

    std::size_t size() const
    {
        return data_.size();
    }

private:
    std::vector<double> data_;
};

std::ostream& operator<<(std::ostream& os, partition_data const& c)
{
    os << "{";
    for (std::size_t i = 0; i != c.size(); ++i)
    {
        if (i != 0)
            os << ", ";
        os << c[i];
    }
    os << "}";
    return os;
}

///////////////////////////////////////////////////////////////////////////////
struct stepper
{
    // Our data for one time step
    typedef partition_data partition;
    typedef std::vector<partition> space;

    // Our operator
    static double heat(double left, double middle, double right)
    {
        return middle + (k * dt / (dx * dx)) * (left - 2 * middle + right);
    }

    // The partitioned operator, it invokes the heat operator above on all
    // elements of a partition.
    static partition_data heat_part(partition_data const& left,
        partition_data const& middle, partition_data const& right)
    {
        std::size_t size = middle.size();
        partition_data next(size);

        next[0] = heat(left[size - 1], middle[0], middle[1]);

        for (std::size_t i = 1; i != size - 1; ++i)
            next[i] = heat(middle[i - 1], middle[i], middle[i + 1]);

        next[size - 1] = heat(middle[size - 2], middle[size - 1], right[0]);

        return next;
    }

    // do all the work on 'np' partitions, 'nx' data points each, for 'nt'
    // time steps
    space do_work(std::size_t np, std::size_t nx, std::size_t nt)
    {
        // U[t][i] is the state of position i at time t.
        std::vector<space> U(2);
        for (space& s : U)
            s.resize(np);

// Initial conditions: f(0, i) = i
#pragma omp parallel
        {
// Visual Studio requires OMP loop variables to be signed :/
#pragma omp for schedule(static)
            for (std::int64_t i = 0; i < (std::int64_t) np; ++i)
                U[0][i] = partition_data(nx, double(i));

            // Actual time step loop
            for (std::size_t t = 0; t != nt; ++t)
            {
                space const& current = U[t % 2];
                space& next = U[(t + 1) % 2];

// Visual Studio requires OMP loop variables to be signed :/
#pragma omp for schedule(static)
                for (std::int64_t i = 0; i < (std::int64_t) np; ++i)
                    next[i] = heat_part(current[idx(i - 1, np)], current[i],
                        current[idx(i + 1, np)]);
            }
        }
        // Return the solution at time-step 'nt'.
        return U[nt % 2];
    }
};

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    std::size_t np = vm["np"].as<std::size_t>();    // Number of partitions.
    std::size_t nx = vm["nx"].as<std::size_t>();    // Number of grid points.
    std::size_t nt = vm["nt"].as<std::size_t>();    // Number of steps.

    if (vm.count("no-header"))
        header = false;

    // Create the stepper object
    stepper step;

    // Measure execution time.
    std::uint64_t t = now();

    // Execute nt time steps on nx grid points and print the final solution.
    stepper::space solution = step.do_work(np, nx, nt);

    std::uint64_t elapsed = now() - t;

    // Print the final solution
    if (vm.count("results"))
    {
        for (std::size_t i = 0; i != np; ++i)
            std::cout << "U[" << i << "] = " << solution[i] << std::endl;
    }
    //  std::uint64_t const os_thread_count = omp_get_num_threads();
#pragma omp parallel
#pragma omp master
    print_time_results(omp_get_num_threads(), elapsed, nx, np, nt, header);

    return 0;
}

int main(int argc, char* argv[])
{
    namespace po = hpx::program_options;

    po::options_description desc_commandline;
    // clang-format off
    desc_commandline.add_options()
        ("results", "print generated results (default: false)")
        ("nx", po::value<std::size_t>()->default_value(10),
         "Local x dimension (of each partition)")
        ("nt", po::value<std::size_t>()->default_value(45),
         "Number of time steps")
        ("np", po::value<std::size_t>()->default_value(10),
         "Number of partitions")
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
