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
#include <functional>
#include <numeric>
#include <queue>
#include <random>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace {
    using vector_type = hpx::partitioned_vector<int>;

    enum class input_kind
    {
        random,
        reversed
    };

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

    /// Construct deterministic random or reversed benchmark input.
    ///
    /// \param size The number of values to generate.
    /// \param kind The requested input distribution.
    /// \param seed The random seed used for random input.
    /// \return The generated input values.
    std::vector<int> make_input(
        std::size_t size, input_kind kind, std::uint32_t seed)
    {
        std::vector<int> input(size);
        if (kind == input_kind::reversed)
        {
            std::iota(input.rbegin(), input.rend(), 0);
        }
        else
        {
            std::mt19937 generator(seed);
            std::uniform_int_distribution<int> distribution;
            std::generate(input.begin(), input.end(),
                [&]() { return distribution(generator); });
        }
        return input;
    }

    /// Perform the centralized k-way merge used by the first segmented sort
    /// implementation after its physical partitions have been sorted.
    ///
    /// \tparam LocalIter The local iterator type of each physical run.
    /// \param runs The sorted physical runs to gather, merge, and redistribute.
    template <typename LocalIter>
    void centralized_kway_merge(
        hpx::parallel::detail::segmented_sort_block<LocalIter> const& runs)
    {
        using value_type = typename std::iterator_traits<LocalIter>::value_type;
        auto values = hpx::parallel::detail::segmented_sort_fetch_block(
            hpx::execution::par, runs);
        std::vector<std::size_t> offsets;
        std::vector<std::size_t> counts;
        offsets.reserve(runs.size());
        counts.reserve(runs.size());
        std::size_t offset = 0;
        for (auto const& run : runs)
        {
            offsets.push_back(offset);
            auto const count = static_cast<std::size_t>(run.size());
            counts.push_back(count);
            offset += count;
        }

        struct cursor
        {
            std::size_t position;
            std::size_t end;
        };

        auto const compare = [&](cursor const& lhs, cursor const& rhs) {
            return values[rhs.position] < values[lhs.position];
        };
        std::priority_queue<cursor, std::vector<cursor>, decltype(compare)>
            heap(compare);
        for (std::size_t i = 0; i != counts.size(); ++i)
        {
            if (counts[i] != 0)
            {
                heap.push({offsets[i], offsets[i] + counts[i]});
            }
        }

        std::size_t destination = 0;
        std::vector<value_type> output;
        output.reserve(counts.front());
        while (!heap.empty())
        {
            auto current = heap.top();
            heap.pop();
            output.push_back(HPX_MOVE(values[current.position]));
            if (++current.position != current.end)
            {
                heap.push(current);
            }
            if (output.size() == counts[destination])
            {
                hpx::parallel::detail::segmented_sort_block<LocalIter> block{
                    runs[destination]};
                [[maybe_unused]] auto const last =
                    hpx::parallel::detail::segmented_sort_store_block(
                        hpx::execution::par, block, output.begin());
                HPX_ASSERT(last == output.end());
                output.clear();
                if (++destination != runs.size())
                {
                    output.reserve(counts[destination]);
                }
            }
        }
        HPX_ASSERT(destination == runs.size());
    }

    /// Run the centralized k-way segmented sort retained as a benchmark.
    ///
    /// \param values The distributed vector to sort.
    void centralized_kway_sort(vector_type& values)
    {
        auto runs = hpx::parallel::detail::segmented_sort_runs(
            values.begin(), values.end());
        if (runs.empty())
        {
            return;
        }
        hpx::parallel::detail::segmented_sort_local_runs(hpx::execution::par,
            runs, std::less<int>{}, hpx::identity_v, std::false_type{});
        centralized_kway_merge(runs);
    }

    /// Gather, sort, and scatter the full input on the benchmark driver.
    ///
    /// \param values The distributed vector to sort.
    /// \param positions The global positions to gather and update.
    void gather_sort_scatter(
        vector_type& values, std::vector<std::size_t> const& positions)
    {
        auto gathered = values.get_values(hpx::launch::sync, positions);
        std::sort(gathered.begin(), gathered.end());
        values.set_values(hpx::launch::sync, positions, gathered);
    }

    /// Report all three sorting approaches for one benchmark configuration.
    ///
    /// \param size The total number of values.
    /// \param test_count The number of timed iterations.
    /// \param partitions_per_locality The partition density.
    /// \param kind The input distribution.
    /// \param seed The deterministic random seed.
    /// \param localities The participating localities.
    void run_configuration(std::size_t size, std::size_t test_count,
        std::size_t partitions_per_locality, input_kind kind,
        std::uint32_t seed, std::vector<hpx::id_type> const& localities)
    {
        auto const partitions = partitions_per_locality * localities.size();
        vector_type values(
            size, 0, hpx::container_layout(partitions, localities));
        std::vector<std::size_t> positions(size);
        std::iota(positions.begin(), positions.end(), std::size_t(0));
        auto const input = make_input(size, kind, seed);
        auto const reset = [&]() { set_input(values, positions, input); };
        auto const distribution =
            kind == input_kind::random ? "random" : "reversed";
        auto const context = std::string(distribution) +
            "/localities=" + std::to_string(localities.size()) +
            "/partitions_per_locality=" +
            std::to_string(partitions_per_locality) +
            "/partitions=" + std::to_string(partitions);

        hpx::util::perftests_report(
            "partitioned_vector_sort", "distributed/" + context, test_count, 0,
            [&]() {
                hpx::sort(hpx::execution::par, values.begin(), values.end());
            },
            reset);
        hpx::util::perftests_report(
            "partitioned_vector_sort", "centralized_kway/" + context,
            test_count, 0, [&]() { centralized_kway_sort(values); }, reset);
        hpx::util::perftests_report(
            "partitioned_vector_sort", "gather_sort_scatter/" + context,
            test_count, 0, [&]() { gather_sort_scatter(values, positions); },
            reset);
    }
}    // namespace

/// Compare distributed sorting with both centralized implementations.
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
    auto const requested_partitions_per_locality =
        vm["partitions_per_locality"].as<std::size_t>();
    auto const seed = vm["seed"].as<std::uint32_t>();
    auto const localities = hpx::find_all_localities();

    if (test_count == 0)
    {
        return hpx::finalize();
    }

    hpx::util::perftests_init(vm, "partitioned_vector_sort_performance");
    std::vector<std::size_t> partitions_per_locality;
    if (requested_partitions_per_locality != 0)
    {
        partitions_per_locality.push_back(requested_partitions_per_locality);
    }
    else
    {
        partitions_per_locality = {8, 16, 32, 64};
    }

    for (auto const partitions : partitions_per_locality)
    {
        run_configuration(
            size, test_count, partitions, input_kind::random, seed, localities);
        run_configuration(size, test_count, partitions, input_kind::reversed,
            seed, localities);
    }
    hpx::util::perftests_print_times();
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
        "number of values")("partitions_per_locality",
        hpx::program_options::value<std::size_t>()->default_value(0),
        "partitions per locality (zero benchmarks 8, 16, 32, and 64)")(
        "test_count",
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
