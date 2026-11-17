//  Copyright (c) 2026 Mohammad Izaan
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
#include <iterator>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy>
double run_lexicographical_compare_benchmark(int test_count, ExPolicy policy,
    std::vector<int>& c, std::vector<int>& d, std::size_t mismatch_pos)
{
    std::uint64_t time = 0;
    bool dummy = false;

    for (int i = 0; i < test_count; ++i)
    {
        c[mismatch_pos] = 1;
        d[mismatch_pos] = 2;

        std::uint64_t elapsed = hpx::chrono::high_resolution_clock::now();
        bool result = hpx::lexicographical_compare(
            policy, c.begin(), c.end(), d.begin(), d.end());
        time += hpx::chrono::high_resolution_clock::now() - elapsed;
        dummy ^= result;

        c[mismatch_pos] = 0;
        d[mismatch_pos] = 0;
    }

    if (!dummy)
    {
        std::cout << "";
    }

    return (static_cast<double>(time) * 1e-9) / test_count;
}

double run_lexicographical_compare_benchmark_std(int test_count,
    std::vector<int>& c, std::vector<int>& d, std::size_t mismatch_pos)
{
    std::uint64_t time = 0;
    bool dummy = false;

    for (int i = 0; i < test_count; ++i)
    {
        c[mismatch_pos] = 1;
        d[mismatch_pos] = 2;

        std::uint64_t elapsed = hpx::chrono::high_resolution_clock::now();
        bool result = std::lexicographical_compare(
            c.begin(), c.end(), d.begin(), d.end());
        time += hpx::chrono::high_resolution_clock::now() - elapsed;
        dummy ^= result;

        c[mismatch_pos] = 0;
        d[mismatch_pos] = 0;
    }

    if (!dummy)
    {
        std::cout << "";
    }

    return (static_cast<double>(time) * 1e-9) / test_count;
}

///////////////////////////////////////////////////////////////////////////////
void run_benchmark(std::size_t vector_size, int test_count)
{
    std::cout << "* Preparing Benchmark..." << std::endl;

    std::vector<int> c(vector_size, 0);
    std::vector<int> d(vector_size, 0);
    std::size_t mismatch_pos = vector_size - 1;

    std::cout << "* Running Benchmark (mismatch at index " << mismatch_pos
              << ")..." << std::endl;

    double time_std = run_lexicographical_compare_benchmark_std(
        test_count, c, d, mismatch_pos);
    double time_seq = run_lexicographical_compare_benchmark(
        test_count, hpx::execution::seq, c, d, mismatch_pos);
    double time_simd = run_lexicographical_compare_benchmark(
        test_count, hpx::execution::simd, c, d, mismatch_pos);
    double time_par = run_lexicographical_compare_benchmark(
        test_count, hpx::execution::par, c, d, mismatch_pos);
    double time_par_unseq = run_lexicographical_compare_benchmark(
        test_count, hpx::execution::par_unseq, c, d, mismatch_pos);
    double time_par_simd = run_lexicographical_compare_benchmark(
        test_count, hpx::execution::par_simd, c, d, mismatch_pos);

    std::cout << "\n-------------- Benchmark Result --------------"
              << std::endl;
    auto fmt = "lexicographical_compare ({1}) : {2}(sec)";
    hpx::util::format_to(std::cout, fmt, "std", time_std) << std::endl;
    hpx::util::format_to(std::cout, fmt, "seq", time_seq) << std::endl;
    hpx::util::format_to(std::cout, fmt, "simd", time_simd) << std::endl;
    hpx::util::format_to(std::cout, fmt, "par", time_par) << std::endl;
    hpx::util::format_to(std::cout, fmt, "par_unseq", time_par_unseq)
        << std::endl;
    hpx::util::format_to(std::cout, fmt, "par_simd", time_par_simd)
        << std::endl;
    std::cout << "----------------------------------------------" << std::endl;
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    std::size_t vector_size = vm["vector_size"].as<std::size_t>();
    int test_count = vm["test_count"].as<int>();

    if (test_count < 1 || vector_size < 1)
    {
        std::cerr << "test_count and vector_size must be >= 1" << std::endl;
        HPX_TEST(false);
        return hpx::local::finalize();
    }

    std::size_t const os_threads = hpx::get_os_thread_count();

    std::cout << "-------------- Benchmark Config --------------" << std::endl;
    std::cout << "vector_size  : " << vector_size << std::endl;
    std::cout << "test_count   : " << test_count << std::endl;
    std::cout << "os_threads   : " << os_threads << std::endl;
    std::cout << "----------------------------------------------\n"
              << std::endl;

    run_benchmark(vector_size, test_count);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;
    options_description desc_commandline(
        "usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("vector_size",
        value<std::size_t>()->default_value(10000000),
        "size of vector (default: 10000000)")("test_count",
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
