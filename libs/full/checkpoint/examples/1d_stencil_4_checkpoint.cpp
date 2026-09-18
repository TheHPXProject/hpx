//  Copyright (c) 2014-2025 Hartmut Kaiser
//  Copyright (c) 2014 Patricia Grubel
//  Copyright (c) 2018 Adrian Serio
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This is the fourth in a series of examples demonstrating the development of a
// fully distributed solver for a simple 1D heat distribution problem.
//
// This example builds on example three. It futurizes the code from that
// example. Compared to example two this code runs much more efficiently. It
// allows for changing the amount of work executed in one HPX thread which
// enables tuning the performance for the optimal grain size of the computation.
// This example is still fully local but demonstrates nice scalability on SMP
// machines.
//
// In this variation of stencil we use the save_checkpoint and
// restore_checkpoint functions to back up the state of the application every n
// time steps.
//

#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/modules/checkpoint.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/serialization.hpp>

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
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
// Our partition data type
struct partition_data
{
private:
    using buffer_type = hpx::serialization::serialize_buffer<double>;

public:
    partition_data() = default;

    explicit partition_data(std::size_t size)
      : data_(new double[size], size, buffer_type::take)
      , size_(size)
    {
    }

    partition_data(std::size_t size, double initial_value)
      : data_(new double[size], size, buffer_type::take)
      , size_(size)
    {
        double const base_value = initial_value * static_cast<double>(size);
        for (std::size_t i = 0; i != size; ++i)
            data_[i] = base_value + static_cast<double>(i);
    }

    partition_data(partition_data const& old_part)
      : data_(new double[old_part.size()], old_part.size(), buffer_type::take)
      , size_(old_part.size())
    {
        for (std::size_t i = 0; i < old_part.size(); i++)
        {
            data_[i] = old_part[i];
        }
    }

    double& operator[](std::size_t idx)
    {
        return data_[idx];
    }
    double operator[](std::size_t idx) const
    {
        return data_[idx];
    }

    std::size_t size() const
    {
        return size_;
    }

private:
    buffer_type data_;
    std::size_t size_ = 0;

    // Serialization Definitions
    friend class hpx::serialization::access;
    template <typename Volume>
    void serialize(Volume& vol, unsigned int const)
    {
        // clang-format off
        vol & data_ & size_;
        // clang-format on
    }
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
// Checkpoint Function

struct backup
{
    std::vector<hpx::util::checkpoint> bin;
    std::string file_name_;

    backup(std::string const& file_name, std::size_t np)
      : bin(np)
      , file_name_(file_name)
    {
    }
    backup(backup&& old) noexcept
      : bin(std::move(old.bin))
      , file_name_(std::move(old.file_name_))
    {
    }
    ~backup() = default;

    void save(partition_data const& status, std::size_t index)
    {
        bin[index] = hpx::util::save_checkpoint(hpx::launch::sync, status);
    }

    void write()
    {
        hpx::util::checkpoint const archive_data =
            hpx::util::save_checkpoint(hpx::launch::sync, bin);

        // Make sure file stream is binary for Windows/Mac machines
        std::ofstream file_archive(
            file_name_, std::ios::binary | std::ios::out);
        if (file_archive.is_open())
        {
            file_archive << archive_data;
        }
        else
        {
            std::cout << "Error opening file!" << std::endl;
        }
    }

    void revive(std::vector<std::vector<hpx::shared_future<partition_data>>>& U,
        std::size_t nx)
    {
        hpx::util::checkpoint temp_archive;
        // Make sure file stream is binary for Windows/Mac machines
        std::ifstream ist(file_name_, std::ios::binary | std::ios::in);
        ist >> temp_archive;
        hpx::util::restore_checkpoint(temp_archive, bin);
        for (std::size_t i = 0; i < U[0].size(); i++)
        {
            partition_data temp(nx, static_cast<double>(i));
            hpx::util::restore_checkpoint(bin[i], temp);
            //Check
            for (std::size_t e = 0; e < temp.size(); e++)
            {
                std::cout << temp[e] << ", ";
            }
            std::cout << std::endl;
            U[0][i] = hpx::make_ready_future(temp);
        }
    }
};

void print(
    std::vector<std::vector<hpx::shared_future<partition_data>>> const& U)
{
    for (std::size_t out = 0; out < U[0].size(); out++)
    {
        partition_data print_buff(U[0][out].get());
        for (std::size_t inner = 0; inner < print_buff.size(); inner++)
        {
            std::cout << print_buff[inner] << ", ";
            if (inner % 9 == 0 && inner != 0)
                std::cout << std::endl;
        }
    }
    std::cout << std::endl;
}
void print_space(std::vector<hpx::shared_future<partition_data>> const& next)
{
    for (std::size_t out = 0; out < next.size(); out++)
    {
        partition_data print_buff(next[out].get());
        for (std::size_t inner = 0; inner < print_buff.size(); inner++)
        {
            std::cout << print_buff[inner] << ", ";
            if (inner % 9 == 0 && inner != 0)
                std::cout << std::endl;
        }
    }
    std::cout << std::endl;
}

///////////////////////////////////////////////////////////////////////////////
struct stepper
{
    // Our data for one time step
    using partition = hpx::shared_future<partition_data>;
    using space = std::vector<partition>;

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
        std::size_t const size = middle.size();
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
    static hpx::future<space> do_work(std::size_t np, std::size_t nx,
        std::size_t nt, std::uint64_t nd, std::size_t cp, std::string rsf,
        std::string fn)
    {
        using hpx::dataflow;
        using hpx::unwrapping;

        // Set up Check-pointing
        std::size_t num_c = nt / cp;    // Number of checkpoints to be made
        std::cout << "Number of checkpoints to be made: " << num_c << std::endl;
        std::vector<std::string> v_file_names(num_c, fn);
        std::vector<backup> container;

        // Initialize checkpoint file names
        for (std::size_t i = 0; i < num_c; i++)
        {
            v_file_names[i] =
                v_file_names[i] + "_" + std::to_string((i + 1) * cp);
            container.push_back(backup(v_file_names[i], np));
        }

        // Container to wait on all held futures
        std::vector<hpx::future<void>> backup_complete;

        // U[t][i] is the state of position i at time t.
        std::vector<space> U(2);
        for (space& s : U)
            s.resize(np);

        // Initial conditions: f(0, i) = i
        auto range = hpx::util::counting_shape(np);
        using hpx::execution::par;
        hpx::ranges::for_each(par, range, [&U, nx](std::size_t i) {
            U[0][i] = hpx::make_ready_future(
                partition_data(nx, static_cast<double>(i)));
        });

        //Initialize from backup
        if (rsf != "")
        {
            backup restart(rsf, np);
            restart.revive(U, nx);
        }

        //Check
        std::cout << "Initialization Check" << std::endl;
        print(U);

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

                //Checkpoint
                if (t % cp == 0 && t != 0)
                {
                    next[i] =
                        next[i].then([&container, i, t, cp](partition&& p) {
                            partition_data value(p.get());
                            container[(t / cp) - 1].save(value, i);
                            partition f_value = hpx::make_ready_future(value);
                            return f_value;
                        });
                }
            }

            //Print Checkpoint to file
            if (t % cp == 0 && t != 0)
            {
                hpx::future<void> f_print = hpx::when_all(next).then(
                    [&container, t, cp](hpx::future<space>&&) {
                        container[(t / cp) - 1].write();
                    });
                backup_complete.push_back(std::move(f_print));
            }

            //Check
            if (t % cp == 0 && t != 0)
            {
                std::cout << "Checkpoint Check:" << std::endl;
                print_space(next);
            }

            // every nd time steps, attach additional continuation which will
            // trigger the semaphore once computation has reached this point
            if ((t % nd) == 0)
            {
                next[0].then([sem, t](partition&&) {
                    // inform semaphore about new lower limit
                    sem->signal(static_cast<std::int64_t>(t));
                });
            }

            // suspend if the tree has become too deep, the continuation above
            // will resume this thread once the computation has caught up
            sem->wait(static_cast<std::int64_t>(t));
        }

        // Wait on Checkpoint Printing
        hpx::wait_all(backup_complete);

        //Begin Test
        //Create a new test vector and resize it
        std::vector<space> Z(2);
        for (space& y : Z)
        {
            y.resize(np);
        }

        backup test(v_file_names[0], np);
        std::cout << std::endl;
        std::cout << "Revive Check:" << std::endl;
        test.revive(Z, nx);
        std::cout << std::endl;
        //End Test

        // Return the solution at time-step 'nt'.
        return hpx::when_all(U[nt % 2]);
    }
};

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    std::size_t const np =
        vm["np"].as<std::size_t>();    // Number of partitions.
    std::size_t const nx =
        vm["nx"].as<std::size_t>();    // Number of grid points.
    std::size_t const nt = vm["nt"].as<std::size_t>();    // Number of steps.
    std::uint64_t const nd =
        vm["nd"].as<std::uint64_t>();    // Max depth of dep tree.
    std::size_t const cp =
        vm["cp"].as<std::size_t>();    // Num. steps to checkpoint
    std::string const rsf = vm["restart-file"].as<std::string>();
    std::string const fn = vm["output-file"].as<std::string>();

    if (vm.count("no-header"))
        header = false;

    // Create the stepper object
    stepper step;

    // Measure execution time.
    std::uint64_t const t = hpx::chrono::high_resolution_clock::now();

    // Execute nt time steps on nx grid points and print the final solution.
    hpx::future<stepper::space> result =
        step.do_work(np, nx, nt, nd, cp, rsf, fn);

    stepper::space solution = result.get();
    hpx::wait_all(solution);

    std::uint64_t const elapsed = hpx::chrono::high_resolution_clock::now() - t;

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

    desc_commandline.add_options()(
        "results", "print generated results (default: false)")("nx",
        value<std::size_t>()->default_value(10),
        "Local x dimension (of each partition)")("nt",
        value<std::size_t>()->default_value(45),
        "Number of time steps")("nd", value<std::uint64_t>()->default_value(10),
        "Number of time steps to allow the dependency tree to grow to")("np",
        value<std::size_t>()->default_value(10),
        "Number of partitions")("k", value<double>(&k)->default_value(0.5),
        "Heat transfer coefficient (default: 0.5)")("dt",
        value<double>(&dt)->default_value(1.0),
        "Timestep unit (default: 1.0[s])")(
        "dx", value<double>(&dx)->default_value(1.0), "Local x dimension")("cp",
        value<std::size_t>()->default_value(44),
        "Number of steps to checkpoint")(
        "no-header", "do not print out the csv header row")("restart-file",
        value<std::string>()->default_value(""),
        "Start application from restart file")("output-file",
        value<std::string>()->default_value("1d.archive"),
        "Base name of archive file");

    // Initialize and run HPX
    hpx::init_params init_args;
    init_args.desc_cmdline = desc_commandline;

    return hpx::init(argc, argv, init_args);
}
