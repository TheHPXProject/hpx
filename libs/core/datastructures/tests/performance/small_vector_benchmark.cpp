//  Copyright (c) 2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/chrono.hpp>
#include <hpx/functional.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
template <typename Container>
void fill(std::size_t size)
{
    Container cont;
    for (std::size_t i = 0; i != size; ++i)
    {
        cont.push_back(typename Container::value_type{});
    }
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    // pull values from cmd
    std::size_t repeat = vm["test_count"].as<std::size_t>();
    std::size_t size = vm["vector_size"].as<std::size_t>();

    hpx::util::perftests_init(vm, "small_vector_benchmark");

    // int

    hpx::util::perftests_report("hpx::small_vector", "int, 1", repeat,
        [&] { fill<hpx::detail::small_vector<int, 1>>(size); });

    hpx::util::perftests_report("hpx::small_vector", "int, 2", repeat,
        [&] { fill<hpx::detail::small_vector<int, 2>>(size); });

    hpx::util::perftests_report("hpx::small_vector", "int, 4", repeat,
        [&] { fill<hpx::detail::small_vector<int, 4>>(size); });

    hpx::util::perftests_report("hpx::small_vector", "int, 8", repeat,
        [&] { fill<hpx::detail::small_vector<int, 8>>(size); });

    hpx::util::perftests_report("hpx::small_vector", "int, 16", repeat,
        [&] { fill<hpx::detail::small_vector<int, 16>>(size); });

    // hpx::move_only_function<void()>

    hpx::util::perftests_report(
        "hpx::small_vector", "fxn:void(), 1", repeat, [&] {
            fill<hpx::detail::small_vector<hpx::move_only_function<void()>, 1>>(
                size);
        });

    hpx::util::perftests_report(
        "hpx::small_vector", "fxn:void(), 2", repeat, [&] {
            fill<hpx::detail::small_vector<hpx::move_only_function<void()>, 2>>(
                size);
        });

    hpx::util::perftests_report(
        "hpx::small_vector", "fxn:void(), 4", repeat, [&] {
            fill<hpx::detail::small_vector<hpx::move_only_function<void()>, 4>>(
                size);
        });

    hpx::util::perftests_report(
        "hpx::small_vector", "fxn:void(), 8", repeat, [&] {
            fill<hpx::detail::small_vector<hpx::move_only_function<void()>, 8>>(
                size);
        });

    hpx::util::perftests_report(
        "hpx::small_vector", "fxn:void(), 16", repeat, [&] {
            fill<
                hpx::detail::small_vector<hpx::move_only_function<void()>, 16>>(
                size);
        });

    hpx::util::perftests_print_times();

    return hpx::local::finalize();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    //initialize program
    std::vector<std::string> const cfg = {"hpx.os_threads=1"};

    using namespace hpx::program_options;

    options_description cmdline("usage: " HPX_APPLICATION_STRING " [options]");

    // clang-format off
    cmdline.add_options()
        ("vector_size", value<std::size_t>()->default_value(10),
            "size of vector")
        ("test_count", value<std::size_t>()->default_value(100000),
            "number of tests to be averaged")
        ;
    // clang-format on

    hpx::util::perftests_cfg(cmdline);

    hpx::local::init_params init_args;
    init_args.desc_cmdline = cmdline;
    init_args.cfg = cfg;

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
