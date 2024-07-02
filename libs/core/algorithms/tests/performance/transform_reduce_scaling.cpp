//  Copyright (c) 2014 Grant Mercer
//  Copyright (c) 2020-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/chrono.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/numeric.hpp>
#include <hpx/modules/testing.hpp>

#include "worker_timed.hpp"

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
int test_count = 100;
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);

struct Point
{
    double x, y;
};

///////////////////////////////////////////////////////////////////////////////
void transform_reduce(std::size_t size)
{
    std::vector<Point> data_representation(
        size, Point{double(gen()), double(gen())});

    // invoke transform_reduce
    double result = hpx::transform_reduce(hpx::execution::par,
        std::begin(data_representation), std::end(data_representation), 0.0,
        std::plus<double>(), [](Point r) { return r.x * r.y; });
    HPX_UNUSED(result);
}

void transform_reduce_old(std::size_t size)
{
    std::vector<Point> data_representation(
        size, Point{double(gen()), double(gen())});

    //invoke old reduce
    Point result = hpx::ranges::reduce(hpx::execution::par,
        std::begin(data_representation), std::end(data_representation),
        Point{0.0, 0.0}, [](Point res, Point curr) {
            return Point{res.x * res.y + curr.x * curr.y, 1.0};
        });
    HPX_UNUSED(result);
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    std::size_t vector_size = vm["vector_size"].as<std::size_t>();
    test_count = vm["test_count"].as<int>();
    hpx::util::perftests_init(vm, "transform_reduce_scaling");
    if (test_count < 0 || test_count == 0)
    {
        std::cout << "test_count cannot be less than zero...\n" << std::flush;
    }
    else
    {
        hpx::util::perftests_report("transform_reduce", "par", test_count, [&]
        {
            transform_reduce(vector_size);
        });

        hpx::util::perftests_report("transform_reduce_old", "par", test_count, [&]
        {
            transform_reduce_old(vector_size);
        });

        hpx::util::perftests_print_times();
    }
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    hpx::program_options::options_description cmdline(
        "usage: " HPX_APPLICATION_STRING " [options]");

    cmdline.add_options()("vector_size",
        hpx::program_options::value<std::size_t>()->default_value(1000),
        "size of vector")

        ("test_count",
            hpx::program_options::value<int>()->default_value(100),
            "number of tests to take average from");

    hpx::local::init_params init_args;
    init_args.desc_cmdline = cmdline;
    init_args.cfg = cfg;
    hpx::util::perftests_cfg(cmdline);

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
