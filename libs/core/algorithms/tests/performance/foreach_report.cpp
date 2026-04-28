//  Copyright (c) 2021 ETH Zurich
//  Copyright (c) 2021-2022 Hartmut Kaiser
//  Copyright (c) 2014 Grant Mercer
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/algorithm.hpp>
#include <hpx/chrono.hpp>
#include <hpx/compute.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

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
int hpx_main(hpx::program_options::variables_map& vm)
{
    std::size_t vector_size = vm["vector_size"].as<std::size_t>();
    delay = vm["work_delay"].as<int>();
    test_count = vm["test_count"].as<int>();
    chunk_size = vm["chunk_size"].as<int>();
    disable_stealing = vm.count("disable_stealing");
    fast_idle_mode = vm.count("fast_idle_mode");

    hpx::util::perftests_init(vm);

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

    {
        std::vector<std::size_t> data_representation(vector_size);
        std::iota(std::begin(data_representation),
            std::end(data_representation), gen());

        {
            hpx::execution::experimental::scheduler_executor<
                hpx::execution::experimental::thread_pool_scheduler>
                exec;
            hpx::util::perftests_report("for_each", "scheduler_executor",
                test_count,
                [&]() { measure_parallel_foreach(data_representation, exec); });
        }

#if defined(HPX_HAVE_STDEXEC)
        {
            hpx::execution::experimental::scheduler_executor<
                hpx::execution::experimental::parallel_scheduler>
                exec(hpx::execution::experimental::get_parallel_scheduler());
            hpx::util::perftests_report("for_each", "parallel_scheduler",
                test_count,
                [&]() { measure_parallel_foreach(data_representation, exec); });
        }
#endif

        {
            hpx::execution::parallel_executor exec;
            hpx::util::perftests_report("for_each", "parallel_executor",
                test_count,
                [&]() { measure_parallel_foreach(data_representation, exec); });
        }

        {
            hpx::execution::experimental::fork_join_executor exec;
            hpx::util::perftests_report("for_each", "fork_join_executor",
                test_count,
                [&]() { measure_parallel_foreach(data_representation, exec); });
        }

        hpx::util::perftests_print_times();
    }

    return hpx::local::finalize();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
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
        ("disable_stealing", "disable thread stealing")
        ("fast_idle_mode", "enable fast idle mode")
        ;
    // clang-format on

    hpx::util::perftests_cfg(cmdline);
    hpx::local::init_params init_args;
    init_args.desc_cmdline = cmdline;
    init_args.cfg = {"hpx.os_threads=all"};

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
#endif
