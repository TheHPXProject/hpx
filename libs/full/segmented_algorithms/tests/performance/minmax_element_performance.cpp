//  Copyright (c) 2016 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>

#include <hpx/include/parallel_generate.hpp>
#include <hpx/include/parallel_minmax.hpp>
#include <hpx/include/partitioned_vector.hpp>
#include <hpx/iostream.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/timing.hpp>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// Define the vector types to be used.
HPX_REGISTER_PARTITIONED_VECTOR(int)
unsigned int seed = (unsigned int) std::random_device{}();

///////////////////////////////////////////////////////////////////////////////
struct random_fill
{
    random_fill()
      : gen(seed)
      , dist(0, RAND_MAX)
    {
    }

    int operator()()
    {
        return dist(gen);
    }

    std::mt19937 gen;
    std::uniform_int_distribution<> dist;

    template <typename Archive>
    void serialize(Archive&, unsigned)
    {
    }
};

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    if (hpx::get_locality_id() == 0)
    {
        // pull values from cmd
        std::size_t size = vm["vector_size"].as<std::size_t>();
        //bool csvoutput = vm.count("csv_output") != 0;
        int test_count = vm["test_count"].as<int>();

        // create as many partitions as we have localities
        hpx::partitioned_vector<int> v(
            size, hpx::container_layout(hpx::find_all_localities()));

        // initialize data
        hpx::generate(hpx::execution::par, v.begin(), v.end(), random_fill());

        hpx::util::perftests_init(vm, "minmax_element_performance");

        // run benchmark
        hpx::util::perftests_report("hpx::minmax", "par", test_count, [&] {
            hpx::minmax_element(hpx::execution::par, v.begin(), v.end());
        });

        hpx::util::perftests_report("hpx::min", "par", test_count,
            [&] { hpx::min_element(hpx::execution::par, v.begin(), v.end()); });

        hpx::util::perftests_report("hpx::max", "par", test_count,
            [&] { hpx::max_element(hpx::execution::par, v.begin(), v.end()); });

        hpx::util::perftests_print_times();

        return hpx::finalize();
    }

    return 0;
}

int main(int argc, char* argv[])
{
    std::srand((unsigned int) std::time(nullptr));

    // initialize program
    std::vector<std::string> const cfg = {
        "hpx.os_threads=all", "hpx.run_hpx_main!=1"};

    hpx::program_options::options_description cmdline(
        "usage: " HPX_APPLICATION_STRING " [options]");

    cmdline.add_options()("vector_size",
        hpx::program_options::value<std::size_t>()->default_value(1000),
        "size of vector (default: 1000)")("test_count",
        hpx::program_options::value<int>()->default_value(100),
        "number of tests to be averaged (default: 100)")(
        "csv_output", "print results in csv format")("seed,s",
        hpx::program_options::value<unsigned int>(),
        "the random number generator seed to use for this run");

    hpx::util::perftests_cfg(cmdline);

    hpx::init_params init_args;
    init_args.desc_cmdline = cmdline;
    init_args.cfg = cfg;

    return hpx::init(argc, argv, init_args);
}
#endif
