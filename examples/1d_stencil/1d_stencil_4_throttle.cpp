//  Copyright (c) 2014-2022 Hartmut Kaiser
//  Copyright (c) 2014 Patricia Grubel
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

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>

#include <hpx/algorithm.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/performance_counters.hpp>
#include <hpx/include/util.hpp>
#include <hpx/modules/iterator_support.hpp>

#include "print_time_results.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <apex_api.hpp>

using hpx::id_type;
using hpx::performance_counters::counter_value;
using hpx::performance_counters::get_counter;
using hpx::performance_counters::performance_counter;
using hpx::performance_counters::status_is_valid;
static bool counters_initialized = false;
static char const* counter_name = "/threadqueue{{locality#{}/total}}/length";
apex_policy_handle* periodic_policy_handle;

performance_counter get_counter()
{
    // Resolve the GID of the performances counter using it's symbolic name.
    std::uint32_t const prefix = hpx::get_locality_id();
    return performance_counter(hpx::util::format(counter_name, prefix));
}

void setup_counters()
{
    try
    {
        performance_counter counter(get_counter());
        // We need to explicitly start all counters before we can use them. For
        // certain counters this could be a no-op, in which case start will return
        // 'false'.
        counter.start(hpx::launch::sync);
        std::cout << "Counters initialized! " << counter.get_id() << std::endl;
        counter_value value = counter.get_counter_value(hpx::launch::sync);
        std::cout << "Active threads " << value.get_value<int>() << std::endl;
    }
    catch (hpx::exception const& e)
    {
        std::cerr
            << "apex_policy_engine_active_thread_count: caught exception: "
            << e.what() << std::endl;
    }
    counters_initialized = true;
}

bool test_function(apex_context const&)
{
    if (!counters_initialized)
        return false;
    try
    {
        performance_counter counter(get_counter());
        counter_value value1 = counter.get_counter_value(hpx::launch::sync);
        apex::sample_value("thread_queue_length", value1.get_value<int>());
        std::cout << "Policy: thread_queue_length = " << value1.get_value<int>()
                  << std::endl;
        return APEX_NOERROR;
    }
    catch (hpx::exception const& e)
    {
        std::cerr
            << "apex_policy_engine_active_thread_count: caught exception: "
            << e.what() << std::endl;
        return APEX_ERROR;
    }
}

void register_policies()
{
    periodic_policy_handle =
        apex::register_periodic_policy(100000, test_function);

    apex::setup_timer_throttling(std::string("thread_queue_length"),
        APEX_MINIMIZE_ACCUMULATED, APEX_ACTIVE_HARMONY, 200000);

    // to just set a power cap, do this.
    //apex::setup_power_cap_throttling();
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
        {
            next[i] = heat(middle[i - 1], middle[i], middle[i + 1]);
        }

        next[size - 1] = heat(middle[size - 2], middle[size - 1], right[0]);

        return next;
    }

    // do all the work on 'np' partitions, 'nx' data points each, for 'nt'
    // time steps, limit depth of dependency tree to 'nd'
    hpx::future<space> do_work(
        std::size_t np, std::size_t nx, std::size_t nt, std::uint64_t nd)
    {
        using hpx::dataflow;
        using hpx::unwrapping;

        // U[t][i] is the state of position i at time t.
        std::vector<space> U(2);
        for (space& s : U)
            s.resize(np);

        // Initial conditions: f(0, i) = i
        auto range = hpx::util::counting_shape(np);
        using hpx::execution::par;
        hpx::ranges::for_each(par, range, [&U, nx](std::size_t i) {
            U[0][i] = hpx::make_ready_future(partition_data(nx, double(i)));
        });

        // limit depth of dependency tree
        auto sem = std::make_shared<hpx::sliding_semaphore>(nd);

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

            // every nd time steps, attach additional continuation which will
            // trigger the semaphore once computation has reached this point
            if ((t % nd) == 0)
            {
                next[0].then([sem, t](partition&&) {
                    // inform semaphore about new lower limit
                    sem->signal(t);
                });
            }

            // suspend if the tree has become too deep, the continuation above
            // will resume this thread once the computation has caught up
            sem->wait(t);
        }

        // Return the solution at time-step 'nt'.
        return hpx::when_all(U[nt % 2]);
    }
};

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    std::size_t np = vm["np"].as<std::size_t>();    // Number of partitions.
    std::size_t nx = vm["nx"].as<std::size_t>();    // Number of grid points.
    std::size_t nt = vm["nt"].as<std::size_t>();    // Number of steps.
    std::uint64_t nd =
        vm["nd"].as<std::uint64_t>();    // Max depth of dep tree.

    if (vm.count("no-header"))
        header = false;

    // Create the stepper object
    stepper step;

    // Measure execution time.
    std::uint64_t t = hpx::chrono::high_resolution_clock::now();

    // Execute nt time steps on nx grid points and print the final solution.
    hpx::future<stepper::space> result = step.do_work(np, nx, nt, nd);

    stepper::space solution = result.get();
    hpx::wait_all(solution);

    std::uint64_t elapsed = hpx::chrono::high_resolution_clock::now() - t;

    // We're done with the periodic policy now, so disable it.
    apex::deregister_policy(periodic_policy_handle);
    apex::shutdown_throttling();

    // Print the final solution
    if (vm.count("results"))
    {
        for (std::size_t i = 0; i != np; ++i)
            std::cout << "U[" << i << "] = " << solution[i].get() << std::endl;
    }

    std::uint64_t const os_thread_count = hpx::get_os_thread_count();
    print_time_results(os_thread_count, elapsed, nx, np, nt, header);

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
        ("nx", value<std::size_t>()->default_value(100),
         "Local x dimension (of each partition)")
        ("nt", value<std::size_t>()->default_value(450),
         "Number of time steps")
        ("nd", value<std::uint64_t>()->default_value(10),
         "Number of time steps to allow the dependency tree to grow to")
        ("np", value<std::size_t>()->default_value(1000),
         "Number of partitions")
        ("k", value<double>(&k)->default_value(0.5),
         "Heat transfer coefficient (default: 0.5)")
        ("dt", value<double>(&dt)->default_value(1.0),
         "Timestep unit (default: 1.0[s])")
        ("dx", value<double>(&dx)->default_value(1.0),
         "Local x dimension")
        ( "no-header", "do not print out the csv header row")
    ;
    // clang-format on

    hpx::register_startup_function(&setup_counters);
    hpx::register_startup_function(&register_policies);
    hpx::register_startup_function(&apex::print_options);

    // Initialize and run HPX
    hpx::init_params init_args;
    init_args.desc_cmdline = desc_commandline;

    return hpx::init(argc, argv, init_args);
}
#endif
