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

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

namespace {
    using vector_type = hpx::partitioned_vector<int>;

    /// Restore the distributed input before a timed benchmark iteration.
    ///
    /// \param values The distributed vector to restore.
    /// \param positions The global positions corresponding to \a input.
    /// \param input The values to store at \a positions.
    void set_input(vector_type& values,
        std::vector<std::size_t> const& positions,
        std::vector<int> const& input)
    {
        values.set_values(hpx::launch::sync, positions, input);
    }

    struct benchmark_sample
    {
        double distributed = 0.0;
        double centralized = 0.0;
        bool distributed_first = true;
    };

    /// Measure both operations in alternating order with untimed input resets.
    ///
    /// \tparam Reset A nullary callable that restores the benchmark input.
    /// \tparam Distributed A nullary callable running the distributed sort.
    /// \tparam Centralized A nullary callable running the centralized sort.
    /// \param test_count The number of timed samples for each operation.
    /// \param distributed_first Whether the first sample runs the distributed
    ///        operation first. Subsequent samples alternate the order.
    /// \param reset The input restoration callable.
    /// \param distributed The distributed operation to measure.
    /// \param centralized The centralized operation to measure.
    /// \return The raw timings and execution order for every sample.
    template <typename Reset, typename Distributed, typename Centralized>
    std::vector<benchmark_sample> measure(std::size_t test_count,
        bool distributed_first, Reset&& reset, Distributed&& distributed,
        Centralized&& centralized)
    {
        using clock = std::chrono::steady_clock;

        reset();
        distributed();
        reset();
        centralized();

        std::vector<benchmark_sample> samples;
        samples.reserve(test_count);
        auto measure_one = [&](auto&& operation) {
            reset();
            auto const start = clock::now();
            operation();
            return std::chrono::duration<double>(clock::now() - start).count();
        };
        for (std::size_t i = 0; i != test_count; ++i)
        {
            benchmark_sample sample;
            sample.distributed_first = distributed_first == (i % 2 == 0);
            if (sample.distributed_first)
            {
                sample.distributed = measure_one(distributed);
                sample.centralized = measure_one(centralized);
            }
            else
            {
                sample.centralized = measure_one(centralized);
                sample.distributed = measure_one(distributed);
            }
            samples.push_back(sample);
        }
        return samples;
    }
}    // namespace

/// Compare distributed sorting with a centralized gather-sort-scatter.
///
/// \param vm The parsed benchmark command-line options.
/// \return Zero after initiating HPX shutdown, or immediately on localities
///         that do not run the benchmark driver.
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

    if (test_count == 0)
    {
        return hpx::finalize();
    }

    auto const reset = [&]() { set_input(values, positions, input); };
    auto const samples = measure(
        test_count, (seed % 2) == 0, reset,
        [&]() { hpx::sort(hpx::execution::par, values.begin(), values.end()); },
        [&]() {
            auto gathered = values.get_values(hpx::launch::sync, positions);
            std::sort(gathered.begin(), gathered.end());
            values.set_values(hpx::launch::sync, positions, gathered);
        });

    double distributed = 0.0;
    double centralized = 0.0;
    for (std::size_t i = 0; i != samples.size(); ++i)
    {
        auto const& sample = samples[i];
        distributed += sample.distributed;
        centralized += sample.centralized;
        std::cout << "partitioned_vector_sort sample " << i << " first "
                  << (sample.distributed_first ? "distributed" : "centralized")
                  << " distributed " << sample.distributed << " s centralized "
                  << sample.centralized << " s\n";
    }
    distributed /= static_cast<double>(samples.size());
    centralized /= static_cast<double>(samples.size());

    std::cout << "partitioned_vector_sort distributed " << distributed
              << " s\npartitioned_vector_sort centralized_gather_sort "
              << centralized << " s\n";
    return hpx::finalize();
}

/// Configure and launch the distributed performance benchmark.
///
/// \param argc The number of command-line arguments.
/// \param argv The command-line argument array.
/// \return The HPX runtime exit code.
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

    hpx::init_params init_args;
    init_args.desc_cmdline = cmdline;
    init_args.cfg = {"hpx.os_threads=all", "hpx.run_hpx_main!=1"};
    return hpx::init(argc, argv, init_args);
}
#endif
