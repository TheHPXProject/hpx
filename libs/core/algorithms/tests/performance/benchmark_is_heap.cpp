///////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2017 Taeguk Kwon
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
///////////////////////////////////////////////////////////////////////////////

#include <hpx/algorithm.hpp>
#include <hpx/chrono.hpp>
#include <hpx/format.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/program_options.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <limits>
#include <random>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
unsigned int seed = (unsigned int) std::random_device{}();
std::mt19937 _rand(seed);
///////////////////////////////////////////////////////////////////////////////

struct random_fill
{
    random_fill()
      : gen(_rand())
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
    if (vm.count("seed"))
    {
        seed = vm["seed"].as<unsigned int>();
        _rand.seed(seed);
    }

    // pull values from cmd
    std::size_t vector_size = vm["vector_size"].as<std::size_t>();
    std::size_t break_pos = vm["break_pos"].as<std::size_t>();
    int test_count = vm["test_count"].as<int>();

    std::size_t const os_threads = hpx::get_os_thread_count();
    HPX_UNUSED(os_threads);

    hpx::util::perftests_init(vm, "benchmark_is_heap");

    if (break_pos > vector_size)
        break_pos = vector_size;

    std::vector<int> v(vector_size);

    // initialize data
    using namespace hpx::execution;
    hpx::generate(par, std::begin(v), std::end(v), random_fill());
    std::make_heap(std::begin(v),
        std::next(std::begin(v), static_cast<std::ptrdiff_t>(break_pos)));
    if (break_pos < vector_size)
        v[break_pos] =
            static_cast<int>((std::numeric_limits<std::size_t>::max)());

    hpx::util::perftests_report("hpx::is_heap", "seq", test_count, [&] {
        (void) hpx::is_heap(seq, std::begin(v), std::end(v));
    });

    hpx::util::perftests_report("hpx::is_heap", "par", test_count, [&] {
        (void) hpx::is_heap(par, std::begin(v), std::end(v));
    });

    hpx::util::perftests_report("hpx::is_heap", "par_unseq", test_count, [&] {
        (void) hpx::is_heap(par_unseq, std::begin(v), std::end(v));
    });

    hpx::util::perftests_print_times();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;
    options_description desc_commandline(
        "usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("vector_size",
        hpx::program_options::value<std::size_t>()->default_value(1000000),
        "size of vector (default: 1000000)")("break_pos",
        hpx::program_options::value<std::size_t>()->default_value(
            (std::numeric_limits<std::size_t>::max)()),
        "a position which breaks max heap (default: vector_size)")("test_count",
        hpx::program_options::value<int>()->default_value(10),
        "number of tests to be averaged (default: 10)")("seed,s",
        hpx::program_options::value<unsigned int>(),
        "the random number generator seed to use for this run");

    // initialize program
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    hpx::util::perftests_cfg(desc_commandline);

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
