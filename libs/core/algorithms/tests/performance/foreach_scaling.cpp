//  Copyright (c) 2014 Grant Mercer
//  Copyright (c) 2021-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/algorithm.hpp>
#include <hpx/chrono.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>

#include "foreach_scaling_helpers.hpp"

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
std::uint64_t averageout_plain_for(std::size_t vector_size)
{
    std::vector<std::size_t> data_representation(vector_size);
    std::iota(
        std::begin(data_representation), std::end(data_representation), gen());

    std::uint64_t start = hpx::chrono::high_resolution_clock::now();

    // average out 100 executions to avoid varying results
    for (auto i = 0; i < test_count; i++)
    {
        measure_plain_for(data_representation);
    }

    return (hpx::chrono::high_resolution_clock::now() - start) / test_count;
}

std::uint64_t averageout_plain_for_iter(std::size_t vector_size)
{
    std::vector<std::size_t> data_representation(vector_size);
    std::iota(
        std::begin(data_representation), std::end(data_representation), gen());

    std::uint64_t start = hpx::chrono::high_resolution_clock::now();

    // average out 100 executions to avoid varying results
    for (auto i = 0; i < test_count; i++)
    {
        measure_plain_for_iter(data_representation);
    }

    return (hpx::chrono::high_resolution_clock::now() - start) / test_count;
}

///////////////////////////////////////////////////////////////////////////////
template <typename Executor>
std::uint64_t averageout_parallel_foreach(
    std::size_t vector_size, Executor&& exec)
{
    std::vector<std::size_t> data_representation(vector_size);
    std::iota(
        std::begin(data_representation), std::end(data_representation), gen());

    std::uint64_t start = hpx::chrono::high_resolution_clock::now();

    // average out 100 executions to avoid varying results
    for (auto i = 0; i < test_count; i++)
        measure_parallel_foreach(data_representation, exec);

    return (hpx::chrono::high_resolution_clock::now() - start) / test_count;
}

template <typename Executor>
std::uint64_t averageout_task_foreach(std::size_t vector_size, Executor&& exec)
{
    std::shared_ptr<std::vector<std::size_t>> data_representation(
        std::make_shared<std::vector<std::size_t>>(vector_size));

    std::iota(std::begin(*data_representation), std::end(*data_representation),
        gen());

    if (num_overlapping_loops <= 0)
    {
        std::uint64_t start = hpx::chrono::high_resolution_clock::now();

        for (auto i = 0; i < test_count; i++)
            measure_task_foreach(data_representation, exec).wait();

        return (hpx::chrono::high_resolution_clock::now() - start) / test_count;
    }

    std::vector<hpx::shared_future<void>> tests;
    tests.resize(num_overlapping_loops);

    std::uint64_t start = hpx::chrono::high_resolution_clock::now();

    for (auto i = 0; i < test_count; i++)
    {
        hpx::future<void> curr =
            measure_task_foreach(data_representation, exec);
        if (i >= num_overlapping_loops)
            tests[(i - num_overlapping_loops) % tests.size()].wait();
        tests[i % tests.size()] = curr.share();
    }

    hpx::wait_all(tests);
    return (hpx::chrono::high_resolution_clock::now() - start) / test_count;
}

std::uint64_t averageout_sequential_foreach(std::size_t vector_size)
{
    std::vector<std::size_t> data_representation(vector_size);
    std::iota(
        std::begin(data_representation), std::end(data_representation), gen());

    std::uint64_t start = hpx::chrono::high_resolution_clock::now();

    // average out 100 executions to avoid varying results
    for (auto i = 0; i < test_count; i++)
        measure_sequential_foreach(data_representation);

    return (hpx::chrono::high_resolution_clock::now() - start) / test_count;
}

///////////////////////////////////////////////////////////////////////////////
template <typename Executor>
std::uint64_t averageout_parallel_forloop(
    std::size_t vector_size, Executor&& exec)
{
    std::vector<std::size_t> data_representation(vector_size);
    std::iota(
        std::begin(data_representation), std::end(data_representation), gen());

    std::uint64_t start = hpx::chrono::high_resolution_clock::now();

    // average out 100 executions to avoid varying results
    for (auto i = 0; i < test_count; i++)
    {
        auto ann_exec = hpx::experimental::prefer(
            hpx::execution::experimental::with_annotation, exec,
            "averageout_parallel_forloop_" + std::to_string(i));
        measure_parallel_forloop(data_representation, ann_exec);
    }

    return (hpx::chrono::high_resolution_clock::now() - start) / test_count;
}

template <typename Executor>
std::uint64_t averageout_task_forloop(std::size_t vector_size, Executor&& exec)
{
    std::shared_ptr<std::vector<std::size_t>> data_representation(
        std::make_shared<std::vector<std::size_t>>(vector_size));

    std::iota(std::begin(*data_representation), std::end(*data_representation),
        gen());

    if (num_overlapping_loops <= 0)
    {
        std::uint64_t start = hpx::chrono::high_resolution_clock::now();

        for (auto i = 0; i < test_count; i++)
            measure_task_forloop(data_representation, exec).wait();

        return (hpx::chrono::high_resolution_clock::now() - start) / test_count;
    }

    std::vector<hpx::shared_future<void>> tests;
    tests.resize(num_overlapping_loops);

    std::uint64_t start = hpx::chrono::high_resolution_clock::now();

    for (auto i = 0; i < test_count; i++)
    {
        hpx::future<void> curr =
            measure_task_forloop(data_representation, exec);
        if (i >= num_overlapping_loops)
            tests[(i - num_overlapping_loops) % tests.size()].wait();
        tests[i % tests.size()] = curr.share();
    }

    hpx::wait_all(tests);
    return (hpx::chrono::high_resolution_clock::now() - start) / test_count;
}

std::uint64_t averageout_sequential_forloop(std::size_t vector_size)
{
    std::vector<std::size_t> data_representation(vector_size);
    std::iota(
        std::begin(data_representation), std::end(data_representation), gen());

    std::uint64_t start = hpx::chrono::high_resolution_clock::now();

    // average out 100 executions to avoid varying results
    for (auto i = 0; i < test_count; i++)
        measure_sequential_forloop(data_representation);

    return (hpx::chrono::high_resolution_clock::now() - start) / test_count;
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    // pull values from cmd
    std::size_t vector_size = vm["vector_size"].as<std::size_t>();
    bool csvoutput = vm.count("csv_output") != 0;
    delay = vm["work_delay"].as<int>();
    test_count = vm["test_count"].as<int>();
    chunk_size = vm["chunk_size"].as<int>();
    num_overlapping_loops = vm["overlapping_loops"].as<int>();
    disable_stealing = vm.count("disable_stealing");
    fast_idle_mode = vm.count("fast_idle_mode");

    bool enable_all = vm.count("enable_all");
    if (!vm.count("parallel_foreach") && !vm.count("task_foreach") &&
        !vm.count("sequential_foreach") && !vm.count("parallel_forloop") &&
        !vm.count("task_forloop") && !vm.count("sequential_forloop"))
    {
        enable_all = true;
    }

    // verify that input is within domain of program
    if (test_count == 0 || test_count < 0)
    {
        std::cerr << "test_count cannot be zero or negative...\n" << std::flush;
        hpx::local::finalize();
        return -1;
    }
    else if (delay < 0)
    {
        std::cerr << "delay cannot be a negative number...\n" << std::flush;
        hpx::local::finalize();
        return -1;
    }
    else
    {
        if (disable_stealing)
        {
            hpx::threads::remove_scheduler_mode(
                hpx::threads::policies::scheduler_mode::enable_stealing);
        }
        if (fast_idle_mode)
        {
            hpx::threads::add_scheduler_mode(
                hpx::threads::policies::scheduler_mode::fast_idle_mode);
        }

        // results
        std::uint64_t par_time_foreach = 0;
        std::uint64_t task_time_foreach = 0;
        std::uint64_t seq_time_foreach = 0;

        std::uint64_t par_time_forloop = 0;
        std::uint64_t task_time_forloop = 0;
        std::uint64_t seq_time_forloop = 0;

        std::uint64_t plain_time_for = averageout_plain_for(vector_size);
        std::uint64_t plain_time_for_iter =
            averageout_plain_for_iter(vector_size);

        // runs the benchmarks that are enabled with the given executor
        auto run_benchmarks = [&](auto&& par) {
            if (enable_all || vm.count("parallel_foreach"))
            {
                par_time_foreach =
                    averageout_parallel_foreach(vector_size, par);
            }
            if (enable_all || vm.count("task_foreach"))
            {
                task_time_foreach = averageout_task_foreach(vector_size, par);
            }
            if (enable_all || vm.count("sequential_foreach"))
            {
                seq_time_foreach = averageout_sequential_foreach(vector_size);
            }

            if (enable_all || vm.count("parallel_forloop"))
            {
                par_time_forloop =
                    averageout_parallel_forloop(vector_size, par);
            }
            if (enable_all || vm.count("task_forloop"))
            {
                task_time_forloop = averageout_task_forloop(vector_size, par);
            }
            if (enable_all || vm.count("sequential_forloop"))
            {
                seq_time_forloop = averageout_sequential_forloop(vector_size);
            }
        };

        std::string const executor = vm["executor"].as<std::string>();
        if (executor == "forkjoin")
        {
            run_benchmarks(hpx::execution::experimental::fork_join_executor());
        }
        else if (executor == "suspending_forkjoin")
        {
            run_benchmarks(
                hpx::execution::experimental::suspending_fork_join_executor());
        }
        else if (executor == "shared_suspending_forkjoin")
        {
            run_benchmarks(hpx::execution::experimental::
                    shared_suspending_fork_join_executor());
        }
        else if (executor == "parallel")
        {
            run_benchmarks(hpx::execution::parallel_executor());
        }
        else if (executor == "scheduler")
        {
            run_benchmarks(hpx::execution::experimental::scheduler_executor<
                hpx::execution::experimental::thread_pool_scheduler>());
        }
        else if (executor == "sender")
        {
            hpx::execution::experimental::explicit_scheduler_executor<
                hpx::execution::experimental::thread_pool_scheduler>
                p;

            auto par = hpx::execution::experimental::with_priority(
                p, hpx::threads::thread_priority::bound);

            if (enable_all || vm.count("parallel_foreach"))
            {
                par_time_foreach =
                    averageout_parallel_foreach(vector_size, par);
            }
            if (enable_all || vm.count("sequential_foreach"))
            {
                seq_time_foreach = averageout_sequential_foreach(vector_size);
            }

            if (enable_all || vm.count("parallel_forloop"))
            {
                par_time_forloop =
                    averageout_parallel_forloop(vector_size, par);
            }
            if (enable_all || vm.count("sequential_forloop"))
            {
                seq_time_forloop = averageout_sequential_forloop(vector_size);
            }
        }
        else
        {
            std::cerr << "unknown executor option (should be forkjoin, "
                         "suspending_forkjoin, shared_suspending_forkjoin, "
                         "scheduler, sender, or parallel (default)\n"
                      << std::flush;
            hpx::local::finalize();
            return -1;
        }

        if (disable_stealing)
        {
            hpx::threads::add_scheduler_mode(
                hpx::threads::policies::scheduler_mode::enable_stealing);
        }
        if (fast_idle_mode)
        {
            hpx::threads::remove_scheduler_mode(
                hpx::threads::policies::scheduler_mode::fast_idle_mode);
        }

        if (csvoutput)
        {
            std::cout << "," << static_cast<double>(seq_time_foreach) / 1e9
                      << "," << static_cast<double>(par_time_foreach) / 1e9
                      << "," << static_cast<double>(task_time_foreach) / 1e9
                      << "\n"
                      << std::flush;
        }
        else
        {
            // print results(Formatted). setw(x) assures that all output is
            // right justified
            std::cout << std::left
                      << "----------------Parameters---------------------\n"
                      << std::left
                      << "Vector size                       : " << std::right
                      << std::setw(8) << vector_size << "\n"
                      << std::left
                      << "Number of tests                   : " << std::right
                      << std::setw(8) << test_count << "\n"
                      << std::left
                      << "Delay per iteration (nanoseconds) : " << std::right
                      << std::setw(8) << delay << "\n"
                      << std::left
                      << "Display time in                   : " << std::right
                      << std::setw(8) << "Seconds\n"
                      << std::flush;

            std::cout << "-------------Average-(for)---------------------\n"
                      << std::left
                      << "Average execution time (unrolled) : " << std::right
                      << std::setw(8)
                      << static_cast<double>(plain_time_for) / 1e9 << "\n"
                      << std::left
                      << "Average execution time (iter)     : " << std::right
                      << std::setw(8)
                      << static_cast<double>(plain_time_for_iter) / 1e9 << "\n";

            std::cout << "-------------Average-(for_each)----------------\n"
                      << std::left
                      << "Average parallel execution time   : " << std::right
                      << std::setw(8)
                      << static_cast<double>(par_time_foreach) / 1e9 << "\n"
                      << std::left
                      << "Average task execution time       : " << std::right
                      << std::setw(8)
                      << static_cast<double>(task_time_foreach) / 1e9 << "\n"
                      << std::left
                      << "Average sequential execution time : " << std::right
                      << std::setw(8)
                      << static_cast<double>(seq_time_foreach) / 1e9 << "\n"
                      << std::flush;

            std::cout << "-----Execution Time Difference-(for_each)------\n"
                      << std::left
                      << "Parallel Scale                    : " << std::right
                      << std::setw(8)
                      << (static_cast<double>(seq_time_foreach) /
                             static_cast<double>(par_time_foreach))
                      << "\n"
                      << std::left
                      << "Task Scale                        : " << std::right
                      << std::setw(8)
                      << (static_cast<double>(seq_time_foreach) /
                             static_cast<double>(task_time_foreach))
                      << "\n"
                      << std::flush;

            std::cout << "-------------Average-(for_loop)----------------\n"
                      << std::left
                      << "Average parallel execution time   : " << std::right
                      << std::setw(8)
                      << static_cast<double>(par_time_forloop) / 1e9 << "\n"
                      << std::left
                      << "Average task execution time       : " << std::right
                      << std::setw(8)
                      << static_cast<double>(task_time_forloop) / 1e9 << "\n"
                      << std::left
                      << "Average sequential execution time : " << std::right
                      << std::setw(8)
                      << static_cast<double>(seq_time_forloop) / 1e9 << "\n"
                      << std::flush;

            std::cout << "-----Execution Time Difference-(for_loop)------\n"
                      << std::left
                      << "Parallel Scale                    : " << std::right
                      << std::setw(8)
                      << (static_cast<double>(seq_time_forloop) /
                             static_cast<double>(par_time_forloop))
                      << "\n"
                      << std::left
                      << "Task Scale                        : " << std::right
                      << std::setw(8)
                      << (static_cast<double>(seq_time_forloop) /
                             static_cast<double>(task_time_forloop))
                      << "\n";
        }
    }

    return hpx::local::finalize();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    //initialize program
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    using namespace hpx::program_options;

    options_description cmdline("usage: " HPX_APPLICATION_STRING " [options]");

    // clang-format off
    cmdline.add_options()
        ("vector_size", value<std::size_t>()->default_value(1000),
            "size of vector")
        ("work_delay", value<int>()->default_value(1),
            "loop delay per element in nanoseconds")
        ("test_count", value<int>()->default_value(100),
            "number of tests to be averaged")
        ("chunk_size", value<int>()->default_value(0),
            "number of iterations to combine while parallelization")
        ("overlapping_loops", value<int>()->default_value(0),
            "number of overlapping task loops")
        ("csv_output", "print results in csv format")
        ("executor", value<std::string>()->default_value("parallel"),
            "use specified executor (possible values: forkjoin, "
            "suspending_forkjoin, shared_suspending_forkjoin, scheduler, "
            "sender, or parallel (default)")
        ("disable_stealing", "disable thread stealing")
        ("fast_idle_mode", "enable fast idle mode")

        ("enable_all", "enable all benchmarks")
        ("parallel_foreach", "enable parallel_foreach")
        ("task_foreach", "enable task_foreach")
        ("sequential_foreach", "enable sequential_foreach")
        ("parallel_forloop", "enable parallel_forloop")
        ("task_forloop", "enable task_forloop")
        ("sequential_forloop", "enable sequential_forloop")
        ;
    // clang-format on

    hpx::local::init_params init_args;
    init_args.desc_cmdline = cmdline;
    init_args.cfg = cfg;

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
#endif
