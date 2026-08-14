//  Copyright (c) 2014-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This is the fifth in a series of examples demonstrating the development of a
// fully distributed solver for a simple 1D heat distribution problem.
//
// This example builds on example four. It adds the possibility to distribute
// both - the locality of the partitions and the locality of where the
// heat_part code is executed. The overall code however still runs on one
// locality only (it is always using hpx::find_here() as the target locality).
//
// This example adds all the boilerplate needed for enabling distributed
// operation. Instead of calling (local) functions we invoke the corresponding
// actions.

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/serialization.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "print_time_results.hpp"

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
struct partition_data
{
private:
    // This example runs on a single locality, so it does not actually exercise
    // serialization at runtime. We still use serialize_buffer here because
    // action arguments must remain serializable and the ownership semantics of
    // the partition buffer are explicit with this type.
    typedef hpx::serialization::serialize_buffer<double> buffer_type;

public:
    partition_data()
      : size_(0)
    {
    }

    explicit partition_data(std::size_t size)
      : data_(new double[size], size, buffer_type::take)
      , size_(size)
    {
    }

    partition_data(std::size_t size, double initial_value)
      : data_(new double[size], size, buffer_type::take)
      , size_(size)
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
        return size_;
    }

private:
    // Serialization support is required even though 1d_stencil_5 itself is not
    // a fully distributed example and does not rely on serialization during
    // execution. Action arguments still need to provide serialization support.
    friend class hpx::serialization::access;

    template <typename Archive>
    void serialize(Archive& ar, unsigned int const)
    {
        ar & data_ & size_;
    }

private:
    buffer_type data_;
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
// This is the server side representation of the data. We expose this as a HPX
// component which allows for it to be created and accessed remotely through
// a global address (hpx::id_type).
struct partition_server : hpx::components::component_base<partition_server>
{
    // construct new instances
    partition_server() {}

    partition_server(partition_data const& data)
      : data_(data)
    {
    }

    partition_server(std::size_t size, double initial_value)
      : data_(size, initial_value)
    {
    }

    // access data
    partition_data get_data() const
    {
        return data_;
    }

    // Every member function which has to be invoked remotely needs to be
    // wrapped into a component action. The macro below defines a new type
    // 'get_data_action' which represents the (possibly remote) member function
    // partition::get_data().
    HPX_DEFINE_COMPONENT_DIRECT_ACTION(
        partition_server, get_data, get_data_action)

private:
    partition_data data_;
};

// The macros below are necessary to generate the code required for exposing
// our partition type remotely.
//
// HPX_REGISTER_COMPONENT() exposes the component creation
// through hpx::new_<>().
typedef hpx::components::component<partition_server> partition_server_type;
HPX_REGISTER_COMPONENT(partition_server_type, partition_server)

// HPX_REGISTER_ACTION() exposes the component member function for remote
// invocation.
typedef partition_server::get_data_action get_data_action;
HPX_REGISTER_ACTION(get_data_action)

///////////////////////////////////////////////////////////////////////////////
// This is a client side helper class allowing to hide some of the tedious
// boilerplate.
struct partition : hpx::components::client_base<partition, partition_server>
{
    typedef hpx::components::client_base<partition, partition_server> base_type;

    partition() {}

    // Create new component on locality 'where' and initialize the held data
    partition(hpx::id_type where, std::size_t size, double initial_value)
      : base_type(hpx::new_<partition_server>(where, size, initial_value))
    {
    }

    // Create a new component on the locality co-located to the id 'where'. The
    // new instance will be initialized from the given partition_data.
    partition(hpx::id_type where, partition_data&& data)
      : base_type(
            hpx::new_<partition_server>(hpx::colocated(where), std::move(data)))
    {
    }

    // Attach a future representing a (possibly remote) partition.
    partition(hpx::future<hpx::id_type>&& id)
      : base_type(std::move(id))
    {
    }

    // Unwrap a future<partition> (a partition already holds a future to the id of the
    // referenced object, thus unwrapping accesses this inner future).
    partition(hpx::future<partition>&& c)
      : base_type(std::move(c))
    {
    }

    ///////////////////////////////////////////////////////////////////////////
    // Invoke the (remote) member function which gives us access to the data.
    // This is a pure helper function hiding the async.
    hpx::future<partition_data> get_data() const
    {
        return hpx::async(get_data_action(), get_id());
    }
};

///////////////////////////////////////////////////////////////////////////////
struct stepper
{
    // Our data for one time step
    typedef std::vector<partition> space;

    // Our operator
    static double heat(double left, double middle, double right)
    {
        return middle + (k * dt / (dx * dx)) * (left - 2 * middle + right);
    }

    // The partitioned operator, it invokes the heat operator above on all elements
    // of a partition.
    static partition_data heat_part_data(partition_data const& left,
        partition_data const& middle, partition_data const& right)
    {
        // create new partition_data instance for next time step
        std::size_t size = middle.size();
        partition_data next(size);

        next[0] = heat(left[size - 1], middle[0], middle[1]);

        for (std::size_t i = 1; i != size - 1; ++i)
            next[i] = heat(middle[i - 1], middle[i], middle[i + 1]);

        next[size - 1] = heat(middle[size - 2], middle[size - 1], right[0]);

        return next;
    }

    static partition heat_part(
        partition const& left, partition const& middle, partition const& right)
    {
        using hpx::dataflow;
        using hpx::unwrapping;

        return dataflow(
            unwrapping([middle](partition_data const& l,
                           partition_data const& m, partition_data const& r) {
                // The new partition_data will be allocated on the same
                // locality as 'middle'.
                return partition(middle.get_id(), heat_part_data(l, m, r));
            }),
            left.get_data(), middle.get_data(), right.get_data());
    }

    // do all the work on 'np' partitions, 'nx' data points each, for 'nt'
    // time steps
    space do_work(std::size_t np, std::size_t nx, std::size_t nt);
};

// Static member functions can be exposed as actions as well. That allows to invoke
// those remotely. The macro HPX_PLAIN_ACTION() defines a new action type
// 'heat_part_action' which wraps the static member function
// stepper::heat_part(). It can be used to call that function on a given locality.
HPX_PLAIN_ACTION(stepper::heat_part, heat_part_action)

///////////////////////////////////////////////////////////////////////////////
// do all the work on 'np' partitions, 'nx' data points each, for 'nt'
// time steps
stepper::space stepper::do_work(std::size_t np, std::size_t nx, std::size_t nt)
{
    using hpx::dataflow;

    // U[t][i] is the state of position i at time t.
    std::vector<space> U(2);
    for (space& s : U)
        s.resize(np);

    // Initial conditions: f(0, i) = i
    for (std::size_t i = 0; i != np; ++i)
        U[0][i] = partition(hpx::find_here(), nx, double(i));

    using hpx::placeholders::_1;
    using hpx::placeholders::_2;
    using hpx::placeholders::_3;
    auto Op = hpx::bind(heat_part_action(), hpx::find_here(), _1, _2, _3);

    // Actual time step loop
    for (std::size_t t = 0; t != nt; ++t)
    {
        space const& current = U[t % 2];
        space& next = U[(t + 1) % 2];

        for (std::size_t i = 0; i != np; ++i)
        {
            next[i] = dataflow(hpx::launch::async, Op, current[idx(i, -1, np)],
                current[i], current[idx(i, +1, np)]);
        }
    }

    // Return the solution at time-step 'nt'.
    return U[nt % 2];
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    std::uint64_t np = vm["np"].as<std::uint64_t>();    // Number of partitions.
    std::uint64_t nx =
        vm["nx"].as<std::uint64_t>();    // Number of grid points.
    std::uint64_t nt = vm["nt"].as<std::uint64_t>();    // Number of steps.

    if (vm.count("no-header"))
        header = false;

    // Create the stepper object
    stepper step;

    // Measure execution time.
    std::uint64_t t = hpx::chrono::high_resolution_clock::now();

    // Execute nt time steps on nx grid points and print the final solution.
    stepper::space solution = step.do_work(np, nx, nt);
    for (std::size_t i = 0; i != np; ++i)
        solution[i].get_data().wait();

    std::uint64_t elapsed = hpx::chrono::high_resolution_clock::now() - t;

    // Print the final solution
    if (vm.count("results"))
    {
        for (std::size_t i = 0; i != np; ++i)
        {
            std::cout << "U[" << i << "] = " << solution[i].get_data().get()
                      << std::endl;
        }
    }

    std::uint64_t const num_worker_threads = hpx::get_num_worker_threads();
    hpx::future<std::uint32_t> locs = hpx::get_num_localities();
    print_time_results(
        locs.get(), num_worker_threads, elapsed, nx, np, nt, header);

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;

    options_description desc_commandline;
    desc_commandline.add_options()(
        "results", "print generated results (default: false)")("nx",
        value<std::uint64_t>()->default_value(10),
        "Local x dimension (of each partition)")("nt",
        value<std::uint64_t>()->default_value(45),
        "Number of time steps")("np", value<std::uint64_t>()->default_value(10),
        "Number of partitions")("k", value<double>(&k)->default_value(0.5),
        "Heat transfer coefficient (default: 0.5)")("dt",
        value<double>(&dt)->default_value(1.0),
        "Timestep unit (default: 1.0[s])")(
        "dx", value<double>(&dx)->default_value(1.0), "Local x dimension")(
        "no-header", "do not print out the csv header row");

    // Initialize and run HPX
    hpx::init_params init_args;
    init_args.desc_cmdline = desc_commandline;

    return hpx::init(argc, argv, init_args);
}
#endif
