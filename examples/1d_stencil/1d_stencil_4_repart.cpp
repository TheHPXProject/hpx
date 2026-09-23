//  Copyright (c) 2014-2020 Hartmut Kaiser
//  Copyright (c) 2014 Patricia Grubel
//  Copyright (c) 2015 Oregon University
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This is the fourth in a series of examples demonstrating the development of
// a fully distributed solver for a simple 1D heat distribution problem.
//
// This example builds on example three. It futurizes the code from that
// example. Compared to example two this code runs much more efficiently. It
// allows for changing the amount of work executed in one HPX thread which
// enables tuning the performance for the optimal grain size of the
// computation. This example is still fully local but demonstrates nice
// scalability on SMP machines.

#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>

#include <hpx/algorithm.hpp>
#include <hpx/include/performance_counters.hpp>
#include <hpx/modules/iterator_support.hpp>

#include <memory>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <apex_api.hpp>

#include "print_time_results.hpp"

using hpx::id_type;
using hpx::performance_counters::counter_value;
using hpx::performance_counters::get_counter;
using hpx::performance_counters::performance_counter;
using hpx::performance_counters::status_is_valid;

static bool counters_initialized = false;
static std::string counter_name = "";
static apex_event_type end_iteration_event = APEX_CUSTOM_EVENT_1;
static hpx::id_type counter_id;

void setup_counters()
{
    try
    {
        performance_counter counter(counter_name);
        // We need to explicitly start all counters before we can use them. For
        // certain counters this could be a no-op, in which case start will return
        // 'false'.
        counter.start(hpx::launch::sync);
        counter_id = counter.get_id();
        std::cout << "Counter " << counter_name << " initialized " << counter_id
                  << std::endl;
        counter_value value = counter.get_counter_value(hpx::launch::sync);
        std::cout << "Counter value " << value.get_value<std::int64_t>()
                  << std::endl;
        end_iteration_event = apex::register_custom_event("Repartition");
        counters_initialized = true;
    }
    catch (hpx::exception const& e)
    {
        std::cerr << "1d_stencil_4_repart: caught exception: " << e.what()
                  << std::endl;
        counter_id = hpx::invalid_id;
        return;
    }
}

double get_counter_value()
{
    if (!counters_initialized)
    {
        std::cerr << "get_counter_value(): ERROR: counter was not initialized"
                  << std::endl;
        return false;
    }
    try
    {
        performance_counter counter(counter_id);
        counter_value value1 =
            counter.get_counter_value(hpx::launch::sync, true);
        std::int64_t counter_value = value1.get_value<std::int64_t>();
        std::cerr << "counter_value " << counter_value << std::endl;
        return (double) (counter_value);
    }
    catch (hpx::exception const& e)
    {
        std::cerr << "get_counter_value(): caught exception: " << e.what()
                  << std::endl;
        return (std::numeric_limits<double>::max)();
    }
}

///////////////////////////////////////////////////////////////////////////////
// Command-line variables
bool header = true;    // print csv heading
double k = 0.5;        // heat transfer coefficient
double dt = 1.;        // time step
double dx = 1.;        // grid spacing

inline std::size_t idx(std::size_t i, int dir, std::size_t size)
{
    if (i == 0 && dir == -1)
        return size - 1;
    if (i == size - 1 && dir == +1)
        return 0;

    HPX_ASSERT((i + dir) < size);

    return i + dir;
}

///////////////////////////////////////////////////////////////////////////////
// Our partition data type
struct partition_data
{
public:
    explicit partition_data(std::size_t size)
      : data_(new double[size])
      , size_(size)
    {
    }

    partition_data(std::size_t size, double initial_value)
      : data_(new double[size])
      , size_(size)
    {
        double base_value = initial_value * double(size);
        for (std::ptrdiff_t i = 0; i != static_cast<std::ptrdiff_t>(size); ++i)
            data_[i] = base_value + double(i);
    }

    partition_data(std::size_t size, double const* other)
      : data_(new double[size])
      , size_(size)
    {
        for (std::size_t i = 0; i != size; ++i)
        {
            data_[i] = other[i];
        }
    }

    partition_data(partition_data&& other) noexcept
      : data_(std::move(other.data_))
      , size_(other.size_)
    {
    }

    double& operator[](std::size_t idx)
    {
        return data_[static_cast<std::ptrdiff_t>(idx)];
    }
    double operator[](std::size_t idx) const
    {
        return data_[static_cast<std::ptrdiff_t>(idx)];
    }

    void copy_into_array(double* a) const
    {
        for (std::size_t i = 0; i != size(); ++i)
        {
            a[i] = data_[i];
        }
    }

    std::size_t size() const
    {
        return size_;
    }

private:
    std::unique_ptr<double[]> data_;
    std::size_t size_;
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
    typedef hpx::shared_future<partition_data> partition;
    typedef std::vector<partition> space;

    // Our operator
    static inline double heat(double left, double middle, double right)
    {
        return middle + (k * dt / dx * dx) * (left - 2 * middle + right);
    }

    // The partitioned operator, it invokes the heat operator above on all
    // elements of a partition.
    static partition_data heat_part(partition_data const& left,
        partition_data const& middle, partition_data const& right)
    {
        std::size_t size = middle.size();
        partition_data next(size);

        if (size == 1)
        {
            next[0] = heat(left[0], middle[0], right[0]);
            return next;
        }

        next[0] = heat(left[size - 1], middle[0], middle[1]);

        for (std::size_t i = 1; i < size - 1; ++i)
        {
            next[i] = heat(middle[i - 1], middle[i], middle[i + 1]);
        }

        next[size - 1] = heat(middle[size - 2], middle[size - 1], right[0]);

        return next;
    }

    // do all the work on 'np' partitions, 'nx' data points each, for 'nt'
    // time steps
    hpx::future<space> do_work(std::size_t np, std::size_t nx, std::size_t nt,
        std::shared_ptr<double[]> data)
    {
        using hpx::dataflow;
        using hpx::unwrapping;

        // U[t][i] is the state of position i at time t.
        std::vector<space> U(2);
        for (space& s : U)
            s.resize(np);

        if (!data)
        {
            // Initial conditions: f(0, i) = i
            auto range = hpx::util::counting_shape(np);
            using hpx::execution::par;
            hpx::ranges::for_each(par, range, [&U, nx](std::size_t i) {
                U[0][i] = hpx::make_ready_future(partition_data(nx, double(i)));
            });
        }
        else
        {
            // Initialize from existing data
            auto range = hpx::util::counting_shape(np);
            using hpx::execution::par;
            hpx::ranges::for_each(par, range, [&U, nx, data](std::size_t i) {
                U[0][i] = hpx::make_ready_future(
                    partition_data(nx, data.get() + (i * nx)));
            });
        }

        auto Op = unwrapping(&stepper::heat_part);

        // Actual time step loop
        for (std::size_t t = 0; t != nt; ++t)
        {
            space const& current = U[t % 2];
            space& next = U[(t + 1) % 2];

            for (std::size_t i = 0; i != np; ++i)
            {
                next[i] =
                    dataflow(hpx::launch::async, Op, current[idx(i, -1, np)],
                        current[i], current[idx(i, +1, np)]);
            }
        }

        // Return the solution at time-step 'nt'.
        return hpx::when_all(U[nt % 2]);
    }
};

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    /* Number of partitions dynamically determined
    // Number of partitions.
    // std::uint64_t np = vm["np"].as<std::uint64_t>();
    */

    // Number of grid points.
    std::size_t nx = vm["nx"].as<std::size_t>();
    // Number of steps.
    std::size_t nt = vm["nt"].as<std::size_t>();
    // Number of runs (repartition between runs).
    std::uint64_t nr = vm["nr"].as<std::uint64_t>();

    //std::cerr << "nx = " << nx << std::endl;
    //std::cerr << "nt = " << nt << std::endl;
    //std::cerr << "nr = " << nr << std::endl;

    if (vm.count("no-header"))
        header = false;

    std::uint64_t const os_thread_count = hpx::get_os_thread_count();

    // Find divisors of nx
    std::vector<std::uint64_t> divisors;
    // Start with os_thread_count so we have at least as many
    // partitions as we have HPX threads.
    for (std::uint64_t i = os_thread_count; i < std::sqrt(nx); ++i)
    {
        if (nx % i == 0)
        {
            divisors.push_back(i);
            divisors.push_back(nx / i);
        }
    }
    // This is not necessarily correct (sqrt(x) does not always evenly divide x)
    // and leads to partition size = 1 which we want to avoid
    //divisors.push_back(static_cast<std::uint64_t>(std::sqrt(nx)));
    std::sort(divisors.begin(), divisors.end());

    if (divisors.size() == 0)
    {
        std::cerr << "ERROR: No possible divisors for " << nx
                  << " data elements with at least " << os_thread_count
                  << " partitions and at least two elements per partition."
                  << std::endl;
        return hpx::finalize();
    }

    //std::cerr << "Divisors: ";
    //for(std::uint64_t d : divisors) {
    //    std::cerr << d << " ";
    //}
    //std::cerr << std::endl;

    // Set up APEX tuning
    // The tunable parameter -- how many partitions to divide data into
    long np_index = 1;
    long* tune_params[1] = {nullptr};
    long num_params = 1;
    long mins[1] = {0};
    long maxs[1] = {(long) divisors.size()};
    long steps[1] = {1};
    tune_params[0] = &np_index;
    apex::setup_custom_tuning(get_counter_value, end_iteration_event,
        num_params, tune_params, mins, maxs, steps);

    // Create the stepper object
    stepper step;

    std::shared_ptr<double[]> data;
    for (std::uint64_t i = 0; i < nr; ++i)
    {
        std::uint64_t parts = divisors[np_index];
        std::uint64_t size_per_part = nx / parts;
        std::uint64_t total_size = parts * size_per_part;

        //std::cerr << "parts: " << parts << " Per part: " << size_per_part;
        //std::cerr << " Overall: " << total_size << std::endl;

        // Measure execution time.
        std::uint64_t t = hpx::chrono::high_resolution_clock::now();

        // Execute nt time steps on nx grid points and print the final solution.
        hpx::future<stepper::space> result =
            step.do_work(parts, size_per_part, nt, data);

        stepper::space solution = result.get();
        hpx::wait_all(solution);

        std::uint64_t elapsed = hpx::chrono::high_resolution_clock::now() - t;

        // Get new partition size
        apex::custom_event(end_iteration_event, 0);

        // Gather data together
        if (!data)
            data.reset(new double[total_size]);

        for (std::uint64_t partition = 0; partition != parts; ++partition)
        {
            solution[partition].get().copy_into_array(
                data.get() + (partition * size_per_part));
        }

        // Print the final solution
        if (vm.count("results"))
        {
            for (std::uint64_t i = 0; i != parts; ++i)
                std::cout << "U[" << i << "] = " << solution[i].get()
                          << std::endl;
        }

        print_time_results(
            os_thread_count, elapsed, size_per_part, parts, nt, header);
        header = false;    // only print header once
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;

    // Configure application-specific options.
    options_description desc_commandline;

    // clang-format off
    desc_commandline.add_options()
        ("results", "print generated results (default: false)")
        ("nx", value<std::size_t>()->default_value(10),
         "Local x dimension (of each partition)")
        ("nt", value<std::size_t>()->default_value(45),
         "Number of time steps")
        ("nr", value<std::uint64_t>()->default_value(10),
         "Number of runs")
        ("k", value<double>(&k)->default_value(0.5),
         "Heat transfer coefficient (default: 0.5)")
        ("dt", value<double>(&dt)->default_value(1.0),
         "Timestep unit (default: 1.0[s])")
        ("dx", value<double>(&dx)->default_value(1.0),
         "Local x dimension")
        ( "no-header", "do not print out the csv header row")
        ("counter",
           value<std::string>(&counter_name)->
            default_value("/threads{locality#0/total}/idle-rate"),
            "HPX Counter to minimize for repartitioning")
    ;
    // clang-format off

    hpx::register_startup_function(&setup_counters);

    // Initialize and run HPX
    hpx::init_params init_args;
    init_args.desc_cmdline = desc_commandline;

    return hpx::init(argc, argv, init_args);
}
