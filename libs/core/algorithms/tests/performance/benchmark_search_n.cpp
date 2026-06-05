//  Copyright (c) 2026 Arivoli Ramamoorthy
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/chrono.hpp>
#include <hpx/datapar.hpp>
#include <hpx/format.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/program_options.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy>
double run_search_n_benchmark(
    int test_count, ExPolicy policy, std::vector<int>& c, int needle_count)
{
    std::uint64_t time = 0;
    std::ptrdiff_t acc = 0;
    std::size_t n = c.size();

    for (int i = 0; i < test_count; ++i)
    {
        // Place needle in the last quarter, varying slightly each run
        std::size_t pos = (n * 3 / 4) +
            (static_cast<std::size_t>(i) * 1000003) % (n / 4 - needle_count);
        c[pos] = 2;
        for (int j = 1; j < needle_count; ++j)
            c[pos + j] = 2;

        std::uint64_t elapsed = hpx::chrono::high_resolution_clock::now();
        auto result =
            hpx::search_n(policy, std::begin(c), std::end(c), needle_count, 2);
        time += hpx::chrono::high_resolution_clock::now() - elapsed;
        acc += std::distance(std::begin(c), result);

        // Reset
        for (int j = 0; j < needle_count; ++j)
            c[pos + j] = 1;
    }

    if (acc < 0)
        std::cout << "";    // prevent acc from being optimized away

    return (static_cast<double>(time) * 1e-9) / test_count;
}

double run_search_n_benchmark_std(
    int test_count, std::vector<int>& c, int needle_count)
{
    std::uint64_t time = 0;
    std::ptrdiff_t acc = 0;
    std::size_t n = c.size();

    for (int i = 0; i < test_count; ++i)
    {
        std::size_t pos = (n * 3 / 4) +
            (static_cast<std::size_t>(i) * 1000003) % (n / 4 - needle_count);
        c[pos] = 2;
        for (int j = 1; j < needle_count; ++j)
            c[pos + j] = 2;

        std::uint64_t elapsed = hpx::chrono::high_resolution_clock::now();
        auto result =
            std::search_n(std::begin(c), std::end(c), needle_count, 2);
        time += hpx::chrono::high_resolution_clock::now() - elapsed;
        acc += std::distance(std::begin(c), result);

        for (int j = 0; j < needle_count; ++j)
            c[pos + j] = 1;
    }

    if (acc < 0)
        std::cout << "";

    return (static_cast<double>(time) * 1e-9) / test_count;
}

///////////////////////////////////////////////////////////////////////////////
void run_benchmark(std::size_t vector_size, int test_count, int needle_count)
{
    std::cout << "* Preparing Benchmark..." << std::endl;

    std::vector<int> c(vector_size, 1);

    std::cout << "* Running Benchmark..." << std::endl;

    double time_std = run_search_n_benchmark_std(test_count, c, needle_count);
    double time_seq = run_search_n_benchmark(
        test_count, hpx::execution::seq, c, needle_count);
    double time_par = run_search_n_benchmark(
        test_count, hpx::execution::par, c, needle_count);
    double time_simd = run_search_n_benchmark(
        test_count, hpx::execution::simd, c, needle_count);
    double time_par_simd = run_search_n_benchmark(
        test_count, hpx::execution::par_simd, c, needle_count);

    std::cout << "\n-------------- Benchmark Result --------------"
              << std::endl;
    auto fmt = "search_n ({1}) : {2}(sec)";
    hpx::util::format_to(std::cout, fmt, "std", time_std) << std::endl;
    hpx::util::format_to(std::cout, fmt, "seq", time_seq) << std::endl;
    hpx::util::format_to(std::cout, fmt, "par", time_par) << std::endl;
    hpx::util::format_to(std::cout, fmt, "simd", time_simd) << std::endl;
    hpx::util::format_to(std::cout, fmt, "par_simd", time_par_simd)
        << std::endl;
    std::cout << "----------------------------------------------" << std::endl;
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    std::size_t vector_size = vm["vector_size"].as<std::size_t>();
    int test_count = vm["test_count"].as<int>();
    int needle_count = vm["needle_count"].as<int>();

    if (needle_count < 1 ||
        vector_size < 4 * static_cast<std::size_t>(needle_count) + 1)
    {
        std::cerr << "vector_size must be at least 4 * needle_count + 1 "
                     "and needle_count must be >= 1"
                  << std::endl;
        return hpx::local::finalize();
    }

    std::size_t const os_threads = hpx::get_os_thread_count();

    std::cout << "-------------- Benchmark Config --------------" << std::endl;
    std::cout << "vector_size  : " << vector_size << std::endl;
    std::cout << "needle_count : " << needle_count << std::endl;
    std::cout << "test_count   : " << test_count << std::endl;
    std::cout << "os_threads   : " << os_threads << std::endl;
    std::cout << "----------------------------------------------\n"
              << std::endl;

    run_benchmark(vector_size, test_count, needle_count);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;
    options_description desc_commandline(
        "usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("vector_size",
        value<std::size_t>()->default_value(10000000),
        "size of vector (default: 10000000)")("needle_count",
        value<int>()->default_value(8),
        "length of the subsequence to search for (default: 8)")("test_count",
        value<int>()->default_value(10),
        "number of runs to average (default: 10)");

    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
