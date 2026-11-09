//  Copyright (c) 2026 Pratyksh Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>

#include <hpx/include/parallel_sort.hpp>
#include <hpx/include/partitioned_vector.hpp>
#include <hpx/include/partitioned_vector_predef.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <random>
#include <string>
#include <vector>

namespace {
    using vector_type = hpx::partitioned_vector<int>;

    /// Restore the distributed input before a timed benchmark iteration.
    void set_input(vector_type& values,
        std::vector<std::size_t> const& positions,
        std::vector<int> const& input)
    {
        values.set_values(hpx::launch::sync, positions, input);
    }
}    // namespace

/// Compare distributed sorting with a centralized gather-sort-scatter.
int hpx_main(hpx::program_options::variables_map& vm)
{
    if (hpx::get_locality_id() != 0)
    {
        return 0;
    }

    auto const size = vm["vector_size"].as<std::size_t>();
    auto const test_count = vm["test_count"].as<std::size_t>();
    auto partitions = vm["partitions"].as<std::size_t>();
    auto const seed = vm["seed"].as<std::uint32_t>();
    auto const localities = hpx::find_all_localities();
    if (partitions == 0)
    {
        partitions = localities.size();
    }

    vector_type values(size, 0, hpx::container_layout(partitions, localities));
    std::vector<std::size_t> positions(size);
    std::iota(positions.begin(), positions.end(), std::size_t(0));
    std::vector<int> input(size);
    std::mt19937 generator(seed);
    std::uniform_int_distribution<int> distribution;
    std::generate(
        input.begin(), input.end(), [&]() { return distribution(generator); });

    hpx::util::perftests_init(vm, "partitioned_vector_sort_performance");
    hpx::util::perftests_report(
        "partitioned_vector_sort", "distributed", test_count, 1,
        [&]() { hpx::sort(hpx::execution::par, values.begin(), values.end()); },
        [&]() { set_input(values, positions, input); });

    hpx::util::perftests_report(
        "partitioned_vector_sort", "centralized_gather_sort", test_count, 1,
        [&]() {
            auto gathered = values.get_values(hpx::launch::sync, positions);
            std::sort(gathered.begin(), gathered.end());
            values.set_values(hpx::launch::sync, positions, gathered);
        },
        [&]() { set_input(values, positions, input); });

    hpx::util::perftests_print_times();
    return hpx::finalize();
}

/// Configure and launch the distributed performance benchmark.
int main(int argc, char* argv[])
{
    hpx::program_options::options_description cmdline(
        "usage: " HPX_APPLICATION_STRING " [options]");
    cmdline.add_options()("vector_size",
        hpx::program_options::value<std::size_t>()->default_value(1000000),
        "number of values")("partitions",
        hpx::program_options::value<std::size_t>()->default_value(0),
        "number of partitions (zero uses one per locality)")("test_count",
        hpx::program_options::value<std::size_t>()->default_value(5),
        "number of timed iterations")("seed",
        hpx::program_options::value<std::uint32_t>()->default_value(0),
        "random-number seed");
    hpx::util::perftests_cfg(cmdline);

    hpx::init_params init_args;
    init_args.desc_cmdline = cmdline;
    init_args.cfg = {"hpx.os_threads=all", "hpx.run_hpx_main!=1"};
    return hpx::init(argc, argv, init_args);
}
#endif
