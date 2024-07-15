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
double run_is_heap_until_benchmark_std(
    int test_count, std::vector<int> const& v)
{
    std::cout << "--- run_is_heap_until_benchmark_std ---" << std::endl;
    decltype(std::begin(v)) result;
    std::uint64_t time = hpx::chrono::high_resolution_clock::now();

    for (int i = 0; i < test_count; ++i)
    {
        result = std::is_heap_until(std::begin(v), std::end(v));
    }

    time = hpx::chrono::high_resolution_clock::now() - time;

    auto heap_range = std::distance(std::begin(v), result);
    std::cout << "Heap Range : " << heap_range << std::endl;

    return (static_cast<double>(time) * 1e-9) / test_count;
}

///////////////////////////////////////////////////////////////////////////////
double run_is_heap_until_benchmark_seq(
    int test_count, std::vector<int> const& v)
{
    std::cout << "--- run_is_heap_until_benchmark_seq ---" << std::endl;
    decltype(std::begin(v)) result;
    std::uint64_t time = hpx::chrono::high_resolution_clock::now();

    for (int i = 0; i < test_count; ++i)
    {
        using namespace hpx::execution;
        result = hpx::is_heap_until(seq, std::begin(v), std::end(v));
    }

    time = hpx::chrono::high_resolution_clock::now() - time;

    auto heap_range = std::distance(std::begin(v), result);
    std::cout << "Heap Range : " << heap_range << std::endl;

    return (static_cast<double>(time) * 1e-9) / test_count;
}

///////////////////////////////////////////////////////////////////////////////
double run_is_heap_until_benchmark_par(
    int test_count, std::vector<int> const& v)
{
    std::cout << "--- run_is_heap_until_benchmark_par ---" << std::endl;
    decltype(std::begin(v)) result;
    std::uint64_t time = hpx::chrono::high_resolution_clock::now();

    for (int i = 0; i < test_count; ++i)
    {
        using namespace hpx::execution;
        result = hpx::is_heap_until(par, std::begin(v), std::end(v));
    }

    time = hpx::chrono::high_resolution_clock::now() - time;

    auto heap_range = std::distance(std::begin(v), result);
    std::cout << "Heap Range : " << heap_range << std::endl;

    return (static_cast<double>(time) * 1e-9) / test_count;
}

///////////////////////////////////////////////////////////////////////////////
double run_is_heap_until_benchmark_par_unseq(
    int test_count, std::vector<int> const& v)
{
    std::cout << "--- run_is_heap_until_benchmark_par_unseq ---" << std::endl;
    decltype(std::begin(v)) result;
    std::uint64_t time = hpx::chrono::high_resolution_clock::now();

    for (int i = 0; i < test_count; ++i)
    {
        using namespace hpx::execution;
        result = hpx::is_heap_until(par_unseq, std::begin(v), std::end(v));
    }

    time = hpx::chrono::high_resolution_clock::now() - time;

    auto heap_range = std::distance(std::begin(v), result);
    std::cout << "Heap Range : " << heap_range << std::endl;

    return (static_cast<double>(time) * 1e-9) / test_count;
}

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
    hpx::util::perftests_init(vm, "benchmark_is_heap_until");

    std::size_t const os_threads = hpx::get_os_thread_count();
    HPX_UNUSED(os_threads);

    if (break_pos > vector_size)
        break_pos = vector_size;

    // std::cout << "-------------- Benchmark Config --------------" << std::endl;
    // std::cout << "seed        : " << seed << std::endl;
    // std::cout << "vector_size : " << vector_size << std::endl;
    // std::cout << "break_pos   : " << break_pos << std::endl;
    // std::cout << "test_count  : " << test_count << std::endl;
    // std::cout << "os threads  : " << os_threads << std::endl;
    // std::cout << "----------------------------------------------\n"
    //           << std::endl;

    // std::cout << "* Preparing Benchmark..." << std::endl;
    std::vector<int> v(vector_size);

    // initialize data
    using namespace hpx::execution;
    hpx::generate(par, std::begin(v), std::end(v), random_fill());
    std::make_heap(std::begin(v),
        std::next(std::begin(v), static_cast<std::ptrdiff_t>(break_pos)));
    if (break_pos < vector_size)
        v[break_pos] =
            static_cast<int>((std::numeric_limits<std::size_t>::max)());

    // std::cout << "* Running Benchmark..." << std::endl;
    // double time_std = run_is_heap_until_benchmark_std(test_count, v);
    // double time_seq = run_is_heap_until_benchmark_seq(test_count, v);
    // double time_par = run_is_heap_until_benchmark_par(test_count, v);
    // double time_par_unseq =
    //     run_is_heap_until_benchmark_par_unseq(test_count, v);

    // hpx::util::perftests_report("std::heap_until", "seq", test_count, [&]
    // {
    //     decltype(std::begin(v)) result = std::is_heap_until(std::begin(v), std::end(v));
    // });

    hpx::util::perftests_report("hpx::heap_until", "seq", test_count, [&] {
        decltype(std::begin(v)) result =
            hpx::is_heap_until(hpx::execution::seq, std::begin(v), std::end(v));
    });

    hpx::util::perftests_report("hpx::heap_until", "par", test_count, [&] {
        decltype(std::begin(v)) result =
            hpx::is_heap_until(hpx::execution::par, std::begin(v), std::end(v));
    });

    hpx::util::perftests_report(
        "hpx::heap_until", "par_unseq", test_count, [&] {
            decltype(std::begin(v)) result = hpx::is_heap_until(
                hpx::execution::par_unseq, std::begin(v), std::end(v));
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

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;
    hpx::util::perftests_cfg(desc_commandline);

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
