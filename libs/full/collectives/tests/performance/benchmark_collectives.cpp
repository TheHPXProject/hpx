//  Copyright (c) 2025 Lukas Zeil
//  Copyright (c) 2025 Alexander Strack
//  Copyright (c) 2025 Hartmut Kaiser
//  Copyright (c) 2026 Anshuman Agrawal
//  Copyright (c) 2026 Abhishek Bansal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/filesystem.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <numeric>
#include <string>
#include <system_error>
#include <vector>

using namespace hpx::collectives;

constexpr char const* scatter_direct_basename = "/test/scatter_direct/";
constexpr char const* reduce_direct_basename = "/test/reduce_direct/";
constexpr char const* broadcast_direct_basename = "/test/broadcast_direct/";
constexpr char const* gather_direct_basename = "/test/gather_direct/";
constexpr char const* all_reduce_direct_basename = "/test/all_reduce_direct/";
constexpr char const* all_gather_direct_basename = "/test/all_gather_direct/";
constexpr char const* all_to_all_direct_basename = "/test/all_to_all_direct/";
constexpr char const* inclusive_scan_direct_basename =
    "/test/inclusive_scan_direct/";
constexpr char const* exclusive_scan_direct_basename =
    "/test/exclusive_scan_direct/";
constexpr char const* barrier_bench_basename = "/test/barrier_bench/";

// Measurement scaffolding: which all_to_all exchange path the one-shot
// benchmark asks for. The dispatch table below fixes the one-shot signature
// for every operation, so this is threaded as a file-scope option rather than
// as an extra parameter on all ten of them. -1 leaves the library default.
int pairwise_threshold_option = -1;

pairwise_threshold_arg benchmark_pairwise_threshold(
    std::size_t const block_size) noexcept
{
    if (pairwise_threshold_option == -1)
    {
        return pairwise_threshold_arg();
    }

    std::size_t const threshold =
        static_cast<std::size_t>(pairwise_threshold_option);
    std::size_t const threshold_elements =
        (threshold + sizeof(int) - 1) / sizeof(int);

    // The benchmark gives every site the same row size, so it can make the
    // globally consistent choice that the general vector-row API cannot infer.
    // Zero forces pairwise; any non-zero value keeps an unmeasurable vector row
    // on the routed path.
    bool const use_pairwise =
        threshold == 0 || block_size >= threshold_elements;
    return pairwise_threshold_arg(use_pairwise ? 0 : 1);
}

constexpr char const* exclusive_scan_basename = "/test/exclusive_scan_direct/";
constexpr char const* inclusive_scan_basename = "/test/inclusive_scan_direct/";

// Element-wise maximum of two equally sized timing vectors. Every benchmark
// keeps its per-iteration elapsed times local and aggregates them with a
// single reduction after the measurement loop, off the measured path.
struct vector_double_max
{
    std::vector<double> operator()(
        std::vector<double> const& a, std::vector<double> const& b) const
    {
        if (a.size() != b.size())
        {
            throw std::runtime_error("Vector sizes must match!");
        }

        std::vector<double> result(a.size());
        for (std::size_t i = 0; i != a.size(); ++i)
        {
            result[i] = (std::max) (a[i], b[i]);
        }
        return result;
    }
};

struct vector_adder
{
    std::vector<int> operator()(
        std::vector<int> const& a, std::vector<int> const& b) const
    {
        if (a.size() != b.size())
        {
            throw std::runtime_error("Vector sizes must match!");
        }

        std::vector<int> result(a.size());
        for (size_t i = 0; i < a.size(); ++i)
        {
            result[i] = a[i] + b[i];
        }
        return result;
    }
};

void create_parent_dir(std::filesystem::path const& file_path)
{
    // Create parent directory if it does not exist. is_directory() (rather
    // than exists()) also catches a regular file occupying the path.
    std::filesystem::path const dir_path = file_path.parent_path();
    if (dir_path.empty() || std::filesystem::is_directory(dir_path))
    {
        return;
    }
    std::error_code ec;
    if (!std::filesystem::create_directories(dir_path, ec) &&
        !std::filesystem::is_directory(dir_path))
    {
        // create_directories() also returns false when another process won
        // the race and created the directory first; only a genuine failure
        // (it still does not exist) is an error.
        throw std::runtime_error("Failed to create directory: " +
            hpx::filesystem::to_string(dir_path) + " (" + ec.message() + ")");
    }
}

struct Stats
{
    double mean = 0.0;
    double variance = 0.0;
    double stddev = 0.0;
    double min = 0.0;
    double max = 0.0;
    double median = 0.0;
};

Stats compute_moments(std::vector<double> data)
{
    if (data.empty())
        return {};
    double const n = static_cast<double>(data.size());

    double const sum = std::accumulate(data.begin(), data.end(), 0.0);
    double const mean = sum / n;

    double varianceSum = 0.0;
    for (double x : data)
        varianceSum += (x - mean) * (x - mean);
    double const variance = data.size() > 1 ? varianceSum / (n - 1.0) : 0.0;
    double const stddev = std::sqrt(variance);

    // Median and min/max: sort in place (data is a local copy)
    std::sort(data.begin(), data.end());
    std::size_t const mid = data.size() / 2;
    double const median =
        (data.size() % 2 == 0) ? 0.5 * (data[mid - 1] + data[mid]) : data[mid];

    return Stats{mean, variance, stddev, data.front(), data.back(), median};
}

void write_to_file(std::string const& collective, std::string const& type,
    int arity, std::size_t num_l, int lpn, int size,
    std::size_t warmup_iterations, std::size_t cooldown_iterations,
    std::size_t iterations, std::vector<double>&& result)
{
    // Compute statistics
    Stats const stats = compute_moments(std::move(result));
    // Compute nodes
    std::size_t nodes = num_l / static_cast<std::size_t>(lpn);
    auto threads = hpx::get_os_thread_count();
    // Print info
    std::string msg = "\nCollective:        {1}\n"
                      "Type:              {2}\n"
                      "Arity:             {3}\n"
                      "Nodes:             {4}\n"
                      "Localities:        {5}\n"
                      "Localities/Node:   {6}\n"
                      "HPX threads:       {7}\n"
                      "Size/Locality:     {8}\n"
                      "Warmup iterations: {9}\n"
                      "Cooldown:          {10}\n"
                      "Iterations:        {11}\n"
                      "Mean runtime:      {12}\n"
                      "Variance:          {13}\n"
                      "Stddev:            {14}\n"
                      "Min:               {15}\n"
                      "Max:               {16}\n"
                      "Median:            {17}\n";
    hpx::util::format_to(std::cout, msg, collective, type, arity, nodes, num_l,
        lpn, threads, size, warmup_iterations, cooldown_iterations, iterations,
        stats.mean, stats.variance, stats.stddev, stats.min, stats.max,
        stats.median)
        << std::flush;

    // Determine active parcelport (bootstrap port = highest-priority enabled port).
    // Guard on num_l > 1: single-locality runs have no distributed runtime/parcelport.
    std::string pp_type = "local";
    if (num_l > 1)
    {
        auto& ph = hpx::get_runtime_distributed().get_parcel_handler();
        if (auto pp = ph.get_bootstrap_parcelport())
            pp_type = pp->type();
    }

    // Create directory: result/hpx/<parcelport>/<num_localities>/<collective>/
    std::string runtime_file_path = "result/hpx/" + pp_type + "/" +
        std::to_string(num_l) + "/" + collective + "/runtimes_" + collective +
        "_" + type;
    if (arity != -1 && type == "hierarchical")
    {
        runtime_file_path = runtime_file_path + "_" + std::to_string(arity);
    }
    runtime_file_path = runtime_file_path + ".txt";
    create_parent_dir(runtime_file_path);

    // Add header if necessary
    std::string const header =
        "collective;type;arity;nodes;localities;lpn;"
        "threads;size;warmup;cooldown;iterations;mean;variance;stddev;min;max;"
        "median\n";
    // Write the header only once, when the file is new or empty. Peeking for
    // EOF avoids reading the whole results file on every append. When the
    // file already has content, its first line is checked against the
    // current header so a schema change (e.g. an added column) from an
    // older build is flagged instead of silently misaligning columns.
    bool need_header = true;
    if (std::ifstream in{runtime_file_path}; in.good())
    {
        need_header = (in.peek() == std::ifstream::traits_type::eof());
        if (!need_header)
        {
            std::string existing_header;
            std::getline(in, existing_header);
            if (existing_header + "\n" != header)
            {
                std::cerr << "WARNING: " << runtime_file_path
                          << " has a different column layout than this "
                             "build produces (likely from an older build); "
                             "appending anyway.\n";
            }
        }
    }

    // Add runtimes
    std::ofstream outfile(runtime_file_path, std::ios_base::app);
    if (!outfile)
    {
        throw std::runtime_error(
            "Failed to open output file: " + runtime_file_path);
    }
    if (need_header)
    {
        outfile << header;
    }
    hpx::util::format_to(outfile,
        "{1};{2};{3};{4};{5};{6};{7};{8};{9};{10};{11};{12};{13};{14};{15};"
        "{16};{17}\n",
        collective, type, arity, nodes, num_l, lpn, threads, size,
        warmup_iterations, cooldown_iterations, iterations, stats.mean,
        stats.variance, stats.stddev, stats.min, stats.max, stats.median);
}

////////////////////////////////////////////////////////////////////////////////////////
// Hierarchical collectives
void test_scatter_hierarchical(int arity, int lpn, std::size_t iterations,
    std::size_t warmup_iterations, std::size_t cooldown_iterations,
    int test_size, std::string const& operation, int fallback_threshold)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Create hierarchical communicators
    auto communicators =
        create_hierarchical_communicator(scatter_direct_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality),
            arity_arg(arity), generation_arg(1), root_site_arg(0),
            fallback_threshold < 0 ?
                flat_fallback_threshold_arg() :
                flat_fallback_threshold_arg(
                    static_cast<std::size_t>(fallback_threshold)));
    // Sync communicator with the same arity/fallback_threshold as the tested
    // one, so it's hierarchical exactly when that one is (own communicator
    // avoids colliding with the tested collective's generations). A flat
    // sync here would let idle sites flood locality 0 while it's still
    // walking its own tree.
    auto const sync_communicators = create_hierarchical_communicator(
        "/test/sync_barrier/scatter/hierarchical/",
        num_sites_arg(num_localities), this_site_arg(this_locality),
        arity_arg(arity), generation_arg(1), root_site_arg(0),
        fallback_threshold < 0 ?
            flat_fallback_threshold_arg() :
            flat_fallback_threshold_arg(
                static_cast<std::size_t>(fallback_threshold)));
    // Per-iteration times stay local; reduced once after the loop, off path.
    auto const timing_comm =
        create_communicator("/test/timing_reduce/scatter/hierarchical/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<std::vector<int>> send_data;
    if (this_locality == 0)
    {
        send_data.assign(num_localities,
            std::vector<int>(static_cast<std::size_t>(test_size), 0));
    }
    std::vector<int> recv_data;
    hpx::future<std::vector<int>> ft_data;

    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        if (this_locality == 0)
        {
            for (std::size_t j = 0; j < num_localities; ++j)
            {
                std::fill(send_data[j].begin(), send_data[j].end(),
                    static_cast<int>(42 + i) + static_cast<int>(j));
            }
        }

        hpx::collectives::barrier(sync_communicators,
            this_site_arg(this_locality), generation_arg(i + 1))
            .get();
        // Time collective
        auto iter_data = send_data;
        hpx::chrono::high_resolution_timer const timer;
        if (this_locality == 0)
        {
            ft_data = scatter_to(communicators, std::move(iter_data),
                this_site_arg(this_locality), generation_arg(i + 1));
        }
        else
        {
            ft_data = scatter_from<std::vector<int>>(communicators,
                this_site_arg(this_locality), generation_arg(i + 1));
        }
        recv_data = ft_data.get();
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;

        // Check for correctness
        HPX_TEST_EQ(static_cast<std::size_t>(test_size), recv_data.size());
        for (int check : recv_data)
        {
            if (!HPX_TEST_EQ(
                    static_cast<int>(42 + i) + static_cast<int>(this_locality),
                    check))
                break;
        }
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        std::string const mod_name = fallback_threshold < 0 ?
            std::string("hierarchical") :
            "hierarchical_t" + std::to_string(fallback_threshold);
        write_to_file(operation, mod_name, arity, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}

void test_reduce_hierarchical(int arity, int lpn, std::size_t iterations,
    std::size_t warmup_iterations, std::size_t cooldown_iterations,
    int test_size, std::string const& operation, int fallback_threshold)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Create communicator
    auto communicators =
        create_hierarchical_communicator(reduce_direct_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality),
            arity_arg(arity), generation_arg(1), root_site_arg(0),
            fallback_threshold < 0 ?
                flat_fallback_threshold_arg() :
                flat_fallback_threshold_arg(
                    static_cast<std::size_t>(fallback_threshold)));
    // Sync communicator with the same arity/fallback_threshold as the tested
    // one, so it's hierarchical exactly when that one is (own communicator
    // avoids colliding with the tested collective's generations). A flat
    // sync here would let idle sites flood locality 0 while it's still
    // walking its own tree.
    auto const sync_communicators = create_hierarchical_communicator(
        "/test/sync_barrier/reduce/hierarchical/",
        num_sites_arg(num_localities), this_site_arg(this_locality),
        arity_arg(arity), generation_arg(1), root_site_arg(0),
        fallback_threshold < 0 ?
            flat_fallback_threshold_arg() :
            flat_fallback_threshold_arg(
                static_cast<std::size_t>(fallback_threshold)));
    // Per-iteration times stay local; reduced once after the loop, off path.
    auto const timing_comm =
        create_communicator("/test/timing_reduce/reduce/hierarchical/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<int> recv_data;
    hpx::future<std::vector<int>> ft_data;

    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        std::vector<int> iter_data(
            static_cast<std::size_t>(test_size), static_cast<int>(i));

        hpx::collectives::barrier(sync_communicators,
            this_site_arg(this_locality), generation_arg(i + 1))
            .get();
        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        if (this_locality == 0)
        {
            ft_data =
                reduce_here(communicators, std::move(iter_data), vector_adder{},
                    this_site_arg(this_locality), generation_arg(i + 1));
            recv_data = ft_data.get();
        }
        else
        {
            hpx::future<void> finished = reduce_there(communicators,
                std::move(iter_data), vector_adder{},
                this_site_arg(this_locality), generation_arg(i + 1));
            finished.get();
        }
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;

        // Check for correctness
        if (this_locality == 0)
        {
            HPX_TEST_EQ(static_cast<std::size_t>(test_size), recv_data.size());
            for (int value : recv_data)
            {
                if (!HPX_TEST_EQ(
                        static_cast<int>(i) * static_cast<int>(num_localities),
                        value))
                {
                    break;
                }
            }
        }
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        std::string const mod_name = fallback_threshold < 0 ?
            std::string("hierarchical") :
            "hierarchical_t" + std::to_string(fallback_threshold);
        write_to_file(operation, mod_name, arity, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}

void test_broadcast_hierarchical(int arity, int lpn, std::size_t iterations,
    std::size_t warmup_iterations, std::size_t cooldown_iterations,
    int test_size, std::string const& operation, int fallback_threshold)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Create communicator
    auto communicators =
        create_hierarchical_communicator(broadcast_direct_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality),
            arity_arg(arity), generation_arg(1), root_site_arg(0),
            fallback_threshold < 0 ?
                flat_fallback_threshold_arg() :
                flat_fallback_threshold_arg(
                    static_cast<std::size_t>(fallback_threshold)));
    // Sync communicator with the same arity/fallback_threshold as the tested
    // one, so it's hierarchical exactly when that one is (own communicator
    // avoids colliding with the tested collective's generations). A flat
    // sync here would let idle sites flood locality 0 while it's still
    // walking its own tree.
    auto const sync_communicators = create_hierarchical_communicator(
        "/test/sync_barrier/broadcast/hierarchical/",
        num_sites_arg(num_localities), this_site_arg(this_locality),
        arity_arg(arity), generation_arg(1), root_site_arg(0),
        fallback_threshold < 0 ?
            flat_fallback_threshold_arg() :
            flat_fallback_threshold_arg(
                static_cast<std::size_t>(fallback_threshold)));
    // Per-iteration times stay local; reduced once after the loop, off path.
    auto const timing_comm =
        create_communicator("/test/timing_reduce/broadcast/hierarchical/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<int> recv_data;
    hpx::future<std::vector<int>> ft_data;

    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        hpx::collectives::barrier(sync_communicators,
            this_site_arg(this_locality), generation_arg(i + 1))
            .get();
        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        if (this_locality == 0)
        {
            ft_data = broadcast_to(communicators,
                std::vector<int>(
                    static_cast<std::size_t>(test_size), static_cast<int>(i)),
                this_site_arg(this_locality), generation_arg(i + 1));
        }
        else
        {
            ft_data = broadcast_from<std::vector<int>>(communicators,
                this_site_arg(this_locality), generation_arg(i + 1));
        }
        recv_data = ft_data.get();
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;

        // Check for correctness
        if (this_locality == 0)
        {
            HPX_TEST_EQ(static_cast<std::size_t>(test_size), recv_data.size());
            for (int value : recv_data)
            {
                if (!HPX_TEST_EQ(static_cast<int>(i), value))
                {
                    break;
                }
            }
        }
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        std::string const mod_name = fallback_threshold < 0 ?
            std::string("hierarchical") :
            "hierarchical_t" + std::to_string(fallback_threshold);
        write_to_file(operation, mod_name, arity, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}

void test_gather_hierarchical(int arity, int lpn, std::size_t iterations,
    std::size_t warmup_iterations, std::size_t cooldown_iterations,
    int test_size, std::string const& operation, int fallback_threshold)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Create communicator
    auto communicators =
        create_hierarchical_communicator(gather_direct_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality),
            arity_arg(arity), generation_arg(1), root_site_arg(0),
            fallback_threshold < 0 ?
                flat_fallback_threshold_arg() :
                flat_fallback_threshold_arg(
                    static_cast<std::size_t>(fallback_threshold)));
    // Sync communicator with the same arity/fallback_threshold as the tested
    // one, so it's hierarchical exactly when that one is (own communicator
    // avoids colliding with the tested collective's generations). A flat
    // sync here would let idle sites flood locality 0 while it's still
    // walking its own tree.
    auto const sync_communicators = create_hierarchical_communicator(
        "/test/sync_barrier/gather/hierarchical/",
        num_sites_arg(num_localities), this_site_arg(this_locality),
        arity_arg(arity), generation_arg(1), root_site_arg(0),
        fallback_threshold < 0 ?
            flat_fallback_threshold_arg() :
            flat_fallback_threshold_arg(
                static_cast<std::size_t>(fallback_threshold)));
    // Per-iteration times stay local; reduced once after the loop, off path.
    auto const timing_comm =
        create_communicator("/test/timing_reduce/gather/hierarchical/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<std::vector<int>> recv_data;
    hpx::future<std::vector<std::vector<int>>> ft_data;

    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        std::vector<int> iter_data(static_cast<std::size_t>(test_size),
            static_cast<int>(i + this_locality));

        hpx::collectives::barrier(sync_communicators,
            this_site_arg(this_locality), generation_arg(i + 1))
            .get();
        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        if (this_locality == 0)
        {
            ft_data = gather_here(communicators, std::move(iter_data),
                this_site_arg(this_locality), generation_arg(i + 1));
            recv_data = ft_data.get();
        }
        else
        {
            hpx::future<void> finished =
                gather_there(communicators, std::move(iter_data),
                    this_site_arg(this_locality), generation_arg(i + 1));
            finished.get();
        }
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;

        // Check for correctness
        if (this_locality == 0)
        {
            HPX_TEST_EQ(num_localities, recv_data.size());
            for (std::size_t j = 0; j != recv_data.size(); ++j)
            {
                HPX_TEST_EQ(
                    static_cast<std::size_t>(test_size), recv_data[j].size());
                for (int value : recv_data[j])
                {
                    if (!HPX_TEST_EQ(static_cast<int>(i + j), value))
                    {
                        break;
                    }
                }
            }
        }
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        std::string const mod_name = fallback_threshold < 0 ?
            std::string("hierarchical") :
            "hierarchical_t" + std::to_string(fallback_threshold);
        write_to_file(operation, mod_name, arity, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}

void test_all_reduce_hierarchical(int arity, int lpn, std::size_t iterations,
    std::size_t warmup_iterations, std::size_t cooldown_iterations,
    int test_size, std::string const& operation, int fallback_threshold)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Create hierarchical communicators
    auto communicators =
        create_hierarchical_communicator(all_reduce_direct_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality),
            arity_arg(arity), generation_arg(1), root_site_arg(0),
            fallback_threshold < 0 ?
                flat_fallback_threshold_arg() :
                flat_fallback_threshold_arg(
                    static_cast<std::size_t>(fallback_threshold)));
    // Sync communicator with the same arity/fallback_threshold as the tested
    // one, so it's hierarchical exactly when that one is (own communicator
    // avoids colliding with the tested collective's generations). A flat
    // sync here would let idle sites flood locality 0 while it's still
    // walking its own tree.
    auto const sync_communicators = create_hierarchical_communicator(
        "/test/sync_barrier/all_reduce/hierarchical/",
        num_sites_arg(num_localities), this_site_arg(this_locality),
        arity_arg(arity), generation_arg(1), root_site_arg(0),
        fallback_threshold < 0 ?
            flat_fallback_threshold_arg() :
            flat_fallback_threshold_arg(
                static_cast<std::size_t>(fallback_threshold)));
    // Per-iteration times stay local; reduced once after the loop, off path.
    auto const timing_comm =
        create_communicator("/test/timing_reduce/all_reduce/hierarchical/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<int> recv_data;

    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        std::vector<int> iter_data(
            static_cast<std::size_t>(test_size), static_cast<int>(i));

        hpx::collectives::barrier(sync_communicators,
            this_site_arg(this_locality), generation_arg(i + 1))
            .get();
        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        hpx::future<std::vector<int>> ft_data =
            all_reduce(communicators, std::move(iter_data), vector_adder{},
                this_site_arg(this_locality), generation_arg(i + 1));
        recv_data = ft_data.get();

        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;

        // Check for correctness: every site should have the sum
        HPX_TEST_EQ(static_cast<std::size_t>(test_size), recv_data.size());
        for (int value : recv_data)
        {
            if (!HPX_TEST_EQ(
                    static_cast<int>(i) * static_cast<int>(num_localities),
                    value))
            {
                break;
            }
        }
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        std::string const mod_name = fallback_threshold < 0 ?
            std::string("hierarchical") :
            "hierarchical_t" + std::to_string(fallback_threshold);
        write_to_file(operation, mod_name, arity, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}

void test_inclusive_scan_hierarchical(int arity, int lpn,
    std::size_t iterations, std::size_t warmup_iterations,
    std::size_t cooldown_iterations, int test_size,
    std::string const& operation, int fallback_threshold)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Create hierarchical communicators
    auto communicators =
        create_hierarchical_communicator(inclusive_scan_direct_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality),
            arity_arg(arity), generation_arg(1), root_site_arg(0),
            fallback_threshold < 0 ?
                flat_fallback_threshold_arg() :
                flat_fallback_threshold_arg(
                    static_cast<std::size_t>(fallback_threshold)));
    // Sync communicator with the same arity/fallback_threshold as the tested
    // one, so it's hierarchical exactly when that one is (own communicator
    // avoids colliding with the tested collective's generations). A flat
    // sync here would let idle sites flood locality 0 while it's still
    // walking its own tree.
    auto const sync_communicators = create_hierarchical_communicator(
        "/test/sync_barrier/inclusive_scan/hierarchical/",
        num_sites_arg(num_localities), this_site_arg(this_locality),
        arity_arg(arity), generation_arg(1), root_site_arg(0),
        fallback_threshold < 0 ?
            flat_fallback_threshold_arg() :
            flat_fallback_threshold_arg(
                static_cast<std::size_t>(fallback_threshold)));
    // Per-iteration times stay local; reduced once after the loop, off path.
    auto const timing_comm =
        create_communicator("/test/timing_reduce/inclusive_scan/hierarchical/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<int> send_data;
    std::vector<int> recv_data;

    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        send_data =
            std::vector<int>(test_size, static_cast<int>(i + this_locality));

        hpx::collectives::barrier(sync_communicators,
            this_site_arg(this_locality), generation_arg(i + 1))
            .get();
        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        hpx::future<std::vector<int>> ft_data =
            // NOLINTNEXTLINE(bugprone-use-after-move)
            inclusive_scan(communicators, std::move(send_data), vector_adder{},
                this_site_arg(this_locality), generation_arg(i + 1));
        recv_data = ft_data.get();

        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;

        // Check for correctness
        int expected = 0;
        for (std::size_t j = 0; j != this_locality + 1; ++j)
        {
            expected += static_cast<int>(i + j);
        }
        HPX_TEST_EQ(static_cast<std::size_t>(test_size), recv_data.size());
        for (int value : recv_data)
        {
            if (!HPX_TEST_EQ(expected, value))
            {
                break;
            }
        }
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        std::string const mod_name = fallback_threshold < 0 ?
            std::string("hierarchical") :
            "hierarchical_t" + std::to_string(fallback_threshold);
        write_to_file(operation, mod_name, arity, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}

void test_exclusive_scan_hierarchical(int arity, int lpn,
    std::size_t iterations, std::size_t warmup_iterations,
    std::size_t cooldown_iterations, int test_size,
    std::string const& operation, int fallback_threshold)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Create hierarchical communicators
    auto communicators =
        create_hierarchical_communicator(exclusive_scan_direct_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality),
            arity_arg(arity), generation_arg(1), root_site_arg(0),
            fallback_threshold < 0 ?
                flat_fallback_threshold_arg() :
                flat_fallback_threshold_arg(
                    static_cast<std::size_t>(fallback_threshold)));
    // Sync communicator with the same arity/fallback_threshold as the tested
    // one, so it's hierarchical exactly when that one is (own communicator
    // avoids colliding with the tested collective's generations). A flat
    // sync here would let idle sites flood locality 0 while it's still
    // walking its own tree.
    auto const sync_communicators = create_hierarchical_communicator(
        "/test/sync_barrier/exclusive_scan/hierarchical/",
        num_sites_arg(num_localities), this_site_arg(this_locality),
        arity_arg(arity), generation_arg(1), root_site_arg(0),
        fallback_threshold < 0 ?
            flat_fallback_threshold_arg() :
            flat_fallback_threshold_arg(
                static_cast<std::size_t>(fallback_threshold)));
    // Per-iteration times stay local; reduced once after the loop, off path.
    auto const timing_comm =
        create_communicator("/test/timing_reduce/exclusive_scan/hierarchical/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<int> send_data;
    std::vector<int> init_data;
    std::vector<int> recv_data;

    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        send_data =
            std::vector<int>(test_size, static_cast<int>(i + this_locality));
        init_data = std::vector<int>(test_size, static_cast<int>(i));

        hpx::collectives::barrier(sync_communicators,
            this_site_arg(this_locality), generation_arg(i + 1))
            .get();
        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        hpx::future<std::vector<int>> ft_data =
            // NOLINTNEXTLINE(bugprone-use-after-move)
            exclusive_scan(communicators, std::move(send_data),
                // NOLINTNEXTLINE(bugprone-use-after-move)
                std::move(init_data), vector_adder{},
                this_site_arg(this_locality), generation_arg(i + 1));
        recv_data = ft_data.get();

        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;

        // Check for correctness
        int expected = static_cast<int>(i);
        for (std::size_t j = 0; j != this_locality; ++j)
        {
            expected += static_cast<int>(i + j);
        }
        HPX_TEST_EQ(static_cast<std::size_t>(test_size), recv_data.size());
        for (int value : recv_data)
        {
            if (!HPX_TEST_EQ(expected, value))
            {
                break;
            }
        }
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        std::string const mod_name = fallback_threshold < 0 ?
            std::string("hierarchical") :
            "hierarchical_t" + std::to_string(fallback_threshold);
        write_to_file(operation, mod_name, arity, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}

void test_barrier_hierarchical(int arity, int lpn, std::size_t iterations,
    std::size_t warmup_iterations, std::size_t cooldown_iterations,
    int test_size, std::string const& operation, int fallback_threshold)
{
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    auto communicators =
        create_hierarchical_communicator(barrier_bench_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality),
            arity_arg(arity), generation_arg(1), root_site_arg(0),
            fallback_threshold < 0 ?
                flat_fallback_threshold_arg() :
                flat_fallback_threshold_arg(
                    static_cast<std::size_t>(fallback_threshold)));
    // Sync communicator with the same arity/fallback_threshold as the tested
    // one, so it's hierarchical exactly when that one is (own communicator
    // avoids colliding with the tested collective's generations). A flat
    // sync here would let idle sites flood locality 0 while it's still
    // walking its own tree.
    auto const sync_communicators = create_hierarchical_communicator(
        "/test/sync_barrier/barrier/hierarchical/",
        num_sites_arg(num_localities), this_site_arg(this_locality),
        arity_arg(arity), generation_arg(1), root_site_arg(0),
        fallback_threshold < 0 ?
            flat_fallback_threshold_arg() :
            flat_fallback_threshold_arg(
                static_cast<std::size_t>(fallback_threshold)));
    // Per-iteration times stay local; reduced once after the loop, off path.
    auto const timing_comm =
        create_communicator("/test/timing_reduce/barrier/hierarchical/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::vector<double> result(iterations, 0.0);

    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        hpx::collectives::barrier(sync_communicators,
            this_site_arg(this_locality), generation_arg(i + 1))
            .get();
        hpx::chrono::high_resolution_timer const timer;
        hpx::collectives::barrier(
            communicators, this_site_arg(this_locality), generation_arg(i + 1))
            .get();
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        std::string const mod_name = fallback_threshold < 0 ?
            std::string("hierarchical") :
            "hierarchical_t" + std::to_string(fallback_threshold);
        write_to_file(operation, mod_name, arity, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}

////////////////////////////////////////////////////////////////////////////////////////
// One shot collectives
void test_one_shot_use_scatter(int lpn, std::size_t iterations,
    std::size_t warmup_iterations, std::size_t cooldown_iterations,
    int test_size, std::string const& operation)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Barrier for synchronization
    char const* const barrier_test_name = "/test/barrier/single";
    hpx::distributed::barrier barrier(barrier_test_name);
    // Per-iteration times stay local; reduced once after the loop, off path.
    auto const timing_comm =
        create_communicator("/test/timing_reduce/scatter/one_shot/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<std::vector<int>> send_data;
    if (this_locality == 0)
    {
        send_data.assign(num_localities,
            std::vector<int>(static_cast<std::size_t>(test_size), 0));
    }
    std::vector<int> recv_data;
    hpx::future<std::vector<int>> ft_data;

    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        if (this_locality == 0)
        {
            for (std::size_t j = 0; j < num_localities; ++j)
            {
                std::fill(send_data[j].begin(), send_data[j].end(),
                    static_cast<int>(42 + i) + static_cast<int>(j));
            }
        }

        barrier.wait();
        // Time collective
        auto iter_data = send_data;
        hpx::chrono::high_resolution_timer const timer;
        if (this_locality == 0)
        {
            ft_data = scatter_to(scatter_direct_basename, std::move(iter_data),
                num_sites_arg(num_localities), this_site_arg(this_locality),
                generation_arg(i + 1));
        }
        else
        {
            ft_data = scatter_from<std::vector<int>>(scatter_direct_basename,
                this_site_arg(this_locality), generation_arg(i + 1));
        }
        recv_data = ft_data.get();
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;

        // Check for correctness
        HPX_TEST_EQ(static_cast<std::size_t>(test_size), recv_data.size());
        for (int check : recv_data)
        {
            if (!HPX_TEST_EQ(
                    static_cast<int>(42 + i) + static_cast<int>(this_locality),
                    check))
                break;
        }
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        write_to_file(operation, "single_use", -1, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}

void test_one_shot_use_reduce(int lpn, std::size_t iterations,
    std::size_t warmup_iterations, std::size_t cooldown_iterations,
    int test_size, std::string const& operation)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Barrier for synchronization
    char const* const barrier_test_name = "/test/barrier/single";
    hpx::distributed::barrier barrier(barrier_test_name);
    // Per-iteration times stay local; reduced once after the loop, off path.
    auto const timing_comm =
        create_communicator("/test/timing_reduce/reduce/one_shot/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<int> recv_data;
    hpx::future<std::vector<int>> ft_data;

    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        std::vector<int> iter_data(
            static_cast<std::size_t>(test_size), static_cast<int>(i));

        barrier.wait();
        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        if (this_locality == 0)
        {
            ft_data = reduce_here(reduce_direct_basename, std::move(iter_data),
                vector_adder{}, num_sites_arg(num_localities),
                this_site_arg(this_locality), generation_arg(i + 1));
            recv_data = ft_data.get();
        }
        else
        {
            hpx::future<void> finished =
                reduce_there(reduce_direct_basename, std::move(iter_data),
                    this_site_arg(this_locality), generation_arg(i + 1));
            finished.get();
        }
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;

        // Check for correctness
        if (this_locality == 0)
        {
            HPX_TEST_EQ(static_cast<std::size_t>(test_size), recv_data.size());
            for (int value : recv_data)
            {
                if (!HPX_TEST_EQ(
                        static_cast<int>(i) * static_cast<int>(num_localities),
                        value))
                {
                    break;
                }
            }
        }
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        write_to_file(operation, "single_use", -1, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}

void test_one_shot_use_broadcast(int lpn, std::size_t iterations,
    std::size_t warmup_iterations, std::size_t cooldown_iterations,
    int test_size, std::string const& operation)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Barrier for synchronization
    char const* const barrier_test_name = "/test/barrier/single";
    hpx::distributed::barrier barrier(barrier_test_name);
    // Per-iteration times stay local; reduced once after the loop, off path.
    auto const timing_comm =
        create_communicator("/test/timing_reduce/broadcast/one_shot/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<int> recv_data;
    hpx::future<std::vector<int>> ft_data;

    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        barrier.wait();
        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        if (this_locality == 0)
        {
            ft_data = broadcast_to(broadcast_direct_basename,
                std::vector<int>(
                    static_cast<std::size_t>(test_size), static_cast<int>(i)),
                num_sites_arg(num_localities), this_site_arg(this_locality),
                generation_arg(i + 1));
        }
        else
        {
            ft_data =
                broadcast_from<std::vector<int>>(broadcast_direct_basename,
                    this_site_arg(this_locality), generation_arg(i + 1));
        }
        recv_data = ft_data.get();
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;

        // Check for correctness
        if (this_locality == 0)
        {
            HPX_TEST_EQ(static_cast<std::size_t>(test_size), recv_data.size());
            for (int value : recv_data)
            {
                if (!HPX_TEST_EQ(static_cast<int>(i), value))
                {
                    break;
                }
            }
        }
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        write_to_file(operation, "single_use", -1, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}

void test_one_shot_use_gather(int lpn, std::size_t iterations,
    std::size_t warmup_iterations, std::size_t cooldown_iterations,
    int test_size, std::string const& operation)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Barrier for synchronization
    char const* const barrier_test_name = "/test/barrier/single";
    hpx::distributed::barrier barrier(barrier_test_name);
    // Per-iteration times stay local; reduced once after the loop, off path.
    auto const timing_comm =
        create_communicator("/test/timing_reduce/gather/one_shot/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<std::vector<int>> recv_data;
    hpx::future<std::vector<std::vector<int>>> ft_data;

    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        std::vector<int> iter_data(static_cast<std::size_t>(test_size),
            static_cast<int>(i + this_locality));

        barrier.wait();
        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        if (this_locality == 0)
        {
            ft_data = gather_here(gather_direct_basename, std::move(iter_data),
                num_sites_arg(num_localities), this_site_arg(this_locality),
                generation_arg(i + 1));
            recv_data = ft_data.get();
        }
        else
        {
            hpx::future<void> finished =
                gather_there(gather_direct_basename, std::move(iter_data),
                    this_site_arg(this_locality), generation_arg(i + 1));
            finished.get();
        }
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;

        // Check for correctness
        if (this_locality == 0)
        {
            HPX_TEST_EQ(num_localities, recv_data.size());
            for (std::size_t j = 0; j != recv_data.size(); ++j)
            {
                HPX_TEST_EQ(
                    static_cast<std::size_t>(test_size), recv_data[j].size());
                for (int value : recv_data[j])
                {
                    if (!HPX_TEST_EQ(static_cast<int>(i + j), value))
                    {
                        break;
                    }
                }
            }
        }
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        write_to_file(operation, "single_use", -1, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}

void test_one_shot_use_all_reduce(int lpn, std::size_t iterations,
    std::size_t warmup_iterations, std::size_t cooldown_iterations,
    int test_size, std::string const& operation)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Barrier for synchronization
    char const* const barrier_test_name = "/test/barrier/single";
    hpx::distributed::barrier barrier(barrier_test_name);
    // Per-iteration times stay local; reduced once after the loop, off path.
    auto const timing_comm =
        create_communicator("/test/timing_reduce/all_reduce/one_shot/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<int> recv_data;

    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        std::vector<int> iter_data(
            static_cast<std::size_t>(test_size), static_cast<int>(i));

        barrier.wait();
        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        hpx::future<std::vector<int>> ft_data =
            all_reduce(all_reduce_direct_basename, std::move(iter_data),
                vector_adder{}, num_sites_arg(num_localities),
                this_site_arg(this_locality), generation_arg(i + 1));
        recv_data = ft_data.get();
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;

        // Check for correctness
        HPX_TEST_EQ(static_cast<std::size_t>(test_size), recv_data.size());
        for (int value : recv_data)
        {
            if (!HPX_TEST_EQ(
                    static_cast<int>(i) * static_cast<int>(num_localities),
                    value))
            {
                break;
            }
        }
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        write_to_file(operation, "single_use", -1, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}

////////////////////////////////////////////////////////////////////////////////////////
// Multi-use shot collectives
void test_multiple_use_with_generation_scatter(int lpn, std::size_t iterations,
    std::size_t warmup_iterations, std::size_t cooldown_iterations,
    int test_size, std::string const& operation)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Create communicator
    auto const scatter_direct_client =
        create_communicator(scatter_direct_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality));
    // Barrier for synchronization
    char const* const barrier_test_name = "/test/barrier/generation";
    hpx::distributed::barrier barrier(barrier_test_name);
    // Per-iteration times stay local; reduced once after the loop, off path.
    auto const timing_comm =
        create_communicator("/test/timing_reduce/scatter/multi_use/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<std::vector<int>> send_data;
    if (this_locality == 0)
    {
        send_data.assign(num_localities,
            std::vector<int>(static_cast<std::size_t>(test_size), 0));
    }
    std::vector<int> recv_data;
    hpx::future<std::vector<int>> ft_data;

    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        if (this_locality == 0)
        {
            for (std::size_t j = 0; j < num_localities; ++j)
            {
                std::fill(send_data[j].begin(), send_data[j].end(),
                    static_cast<int>(42 + i) + static_cast<int>(j));
            }
        }

        barrier.wait();
        // Time collective
        auto iter_data = send_data;
        hpx::chrono::high_resolution_timer const timer;
        if (this_locality == 0)
        {
            ft_data = scatter_to(scatter_direct_client, std::move(iter_data),
                generation_arg(i + 1));
        }
        else
        {
            ft_data = scatter_from<std::vector<int>>(
                scatter_direct_client, generation_arg(i + 1));
        }
        recv_data = ft_data.get();
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;

        // Check for correctness
        HPX_TEST_EQ(static_cast<std::size_t>(test_size), recv_data.size());
        for (int check : recv_data)
        {
            if (!HPX_TEST_EQ(
                    static_cast<int>(42 + i) + static_cast<int>(this_locality),
                    check))
                break;
        }
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        write_to_file(operation, "multi_use", -1, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}

void test_multiple_use_with_generation_reduce(int lpn, std::size_t iterations,
    std::size_t warmup_iterations, std::size_t cooldown_iterations,
    int test_size, std::string const& operation)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Create communicator
    auto const reduce_direct_client =
        create_communicator(reduce_direct_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality));
    // Barrier for synchronization
    char const* const barrier_test_name = "/test/barrier/generation";
    hpx::distributed::barrier barrier(barrier_test_name);
    // Per-iteration times stay local; reduced once after the loop, off path.
    auto const timing_comm =
        create_communicator("/test/timing_reduce/reduce/multi_use/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<int> recv_data;
    hpx::future<std::vector<int>> ft_data;

    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        std::vector<int> iter_data(
            static_cast<std::size_t>(test_size), static_cast<int>(i));

        barrier.wait();
        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        if (this_locality == 0)
        {
            ft_data = reduce_here(reduce_direct_client, std::move(iter_data),
                vector_adder{}, generation_arg(i + 1));
            recv_data = ft_data.get();
        }
        else
        {
            hpx::future<void> finished = reduce_there(reduce_direct_client,
                std::move(iter_data), generation_arg(i + 1));
            finished.get();
        }
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;

        // Check for correctness
        if (this_locality == 0)
        {
            HPX_TEST_EQ(static_cast<std::size_t>(test_size), recv_data.size());
            for (int value : recv_data)
            {
                if (!HPX_TEST_EQ(
                        static_cast<int>(i) * static_cast<int>(num_localities),
                        value))
                {
                    break;
                }
            }
        }
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        write_to_file(operation, "multi_use", -1, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}

void test_multiple_use_with_generation_broadcast(int lpn,
    std::size_t iterations, std::size_t warmup_iterations,
    std::size_t cooldown_iterations, int test_size,
    std::string const& operation)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Create communicator
    auto const broadcast_direct_client =
        create_communicator(broadcast_direct_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality));
    // Barrier for synchronization
    char const* const barrier_test_name = "/test/barrier/generation";
    hpx::distributed::barrier barrier(barrier_test_name);
    // Per-iteration times stay local; reduced once after the loop, off path.
    auto const timing_comm =
        create_communicator("/test/timing_reduce/broadcast/multi_use/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<int> recv_data;
    hpx::future<std::vector<int>> ft_data;

    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        barrier.wait();
        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        if (this_locality == 0)
        {
            ft_data = broadcast_to(broadcast_direct_client,
                std::vector<int>(
                    static_cast<std::size_t>(test_size), static_cast<int>(i)),
                generation_arg(i + 1));
        }
        else
        {
            ft_data = broadcast_from<std::vector<int>>(
                broadcast_direct_client, generation_arg(i + 1));
        }
        recv_data = ft_data.get();
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;

        // Check for correctness
        if (this_locality == 0)
        {
            HPX_TEST_EQ(static_cast<std::size_t>(test_size), recv_data.size());
            for (int value : recv_data)
            {
                if (!HPX_TEST_EQ(static_cast<int>(i), value))
                {
                    break;
                }
            }
        }
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        write_to_file(operation, "multi_use", -1, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}

void test_multiple_use_with_generation_gather(int lpn, std::size_t iterations,
    std::size_t warmup_iterations, std::size_t cooldown_iterations,
    int test_size, std::string const& operation)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Create communicator
    auto const gather_direct_client =
        create_communicator(gather_direct_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality));
    // Barrier for synchronization
    char const* const barrier_test_name = "/test/barrier/generation";
    hpx::distributed::barrier barrier(barrier_test_name);
    // Per-iteration times stay local; reduced once after the loop, off path.
    auto const timing_comm =
        create_communicator("/test/timing_reduce/gather/multi_use/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<std::vector<int>> recv_data;
    hpx::future<std::vector<std::vector<int>>> ft_data;

    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        std::vector<int> iter_data(static_cast<std::size_t>(test_size),
            static_cast<int>(i + this_locality));

        barrier.wait();
        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        if (this_locality == 0)
        {
            ft_data = gather_here(gather_direct_client, std::move(iter_data),
                generation_arg(i + 1));
            recv_data = ft_data.get();
        }
        else
        {
            hpx::future<void> finished = gather_there(gather_direct_client,
                std::move(iter_data), generation_arg(i + 1));
            finished.get();
        }
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;

        // Check for correctness
        if (this_locality == 0)
        {
            HPX_TEST_EQ(num_localities, recv_data.size());
            for (std::size_t j = 0; j != recv_data.size(); ++j)
            {
                HPX_TEST_EQ(
                    static_cast<std::size_t>(test_size), recv_data[j].size());
                for (int value : recv_data[j])
                {
                    if (!HPX_TEST_EQ(static_cast<int>(i + j), value))
                    {
                        break;
                    }
                }
            }
        }
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        write_to_file(operation, "multi_use", -1, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}

void test_multiple_use_with_generation_all_reduce(int lpn,
    std::size_t iterations, std::size_t warmup_iterations,
    std::size_t cooldown_iterations, int test_size,
    std::string const& operation)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Create communicator
    auto const all_reduce_direct_client =
        create_communicator(all_reduce_direct_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality));
    // Barrier for synchronization
    char const* const barrier_test_name = "/test/barrier/generation";
    hpx::distributed::barrier barrier(barrier_test_name);
    // Per-iteration times stay local; reduced once after the loop, off path.
    auto const timing_comm =
        create_communicator("/test/timing_reduce/all_reduce/multi_use/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<int> recv_data;

    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        std::vector<int> iter_data(
            static_cast<std::size_t>(test_size), static_cast<int>(i));

        barrier.wait();
        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        hpx::future<std::vector<int>> ft_data =
            all_reduce(all_reduce_direct_client, std::move(iter_data),
                vector_adder{}, generation_arg(i + 1));
        recv_data = ft_data.get();
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;

        // Check for correctness
        HPX_TEST_EQ(static_cast<std::size_t>(test_size), recv_data.size());
        for (int value : recv_data)
        {
            if (!HPX_TEST_EQ(
                    static_cast<int>(i) * static_cast<int>(num_localities),
                    value))
            {
                break;
            }
        }
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        write_to_file(operation, "multi_use", -1, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}

void test_one_shot_use_barrier(int lpn, std::size_t iterations,
    std::size_t warmup_iterations, std::size_t cooldown_iterations,
    int test_size, std::string const& operation)
{
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    char const* const barrier_sync_name = "/test/barrier/single";
    hpx::distributed::barrier sync(barrier_sync_name);
    auto const timing_comm =
        create_communicator("/test/timing_reduce/barrier/one_shot/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::vector<double> result(iterations, 0.0);

    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        // Create a new communicator per iteration (one-shot semantics)
        auto const barrier_comm = create_communicator(barrier_bench_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality),
            generation_arg(i + 1));
        sync.wait();
        hpx::chrono::high_resolution_timer const timer;
        hpx::collectives::barrier(
            barrier_comm, this_site_arg(this_locality), generation_arg(1))
            .get();
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        write_to_file(operation, "single_use", -1, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}

void test_multiple_use_with_generation_barrier(int lpn, std::size_t iterations,
    std::size_t warmup_iterations, std::size_t cooldown_iterations,
    int test_size, std::string const& operation)
{
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    auto const barrier_client = create_communicator(barrier_bench_basename,
        num_sites_arg(num_localities), this_site_arg(this_locality));
    char const* const barrier_sync_name = "/test/barrier/generation";
    hpx::distributed::barrier sync(barrier_sync_name);
    auto const timing_comm =
        create_communicator("/test/timing_reduce/barrier/multi_use/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::vector<double> result(iterations, 0.0);

    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        sync.wait();
        hpx::chrono::high_resolution_timer const timer;
        hpx::collectives::barrier(
            barrier_client, this_site_arg(this_locality), generation_arg(i + 1))
            .get();
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        write_to_file(operation, "multi_use", -1, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}

void test_all_gather_hierarchical(int arity, int lpn, std::size_t iterations,
    std::size_t warmup_iterations, std::size_t cooldown_iterations,
    int test_size, std::string const& operation, int fallback_threshold)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Create hierarchical communicators with optional threshold override
    auto communicators =
        create_hierarchical_communicator(all_gather_direct_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality),
            arity_arg(arity), generation_arg(1), root_site_arg(0),
            fallback_threshold < 0 ?
                flat_fallback_threshold_arg() :
                flat_fallback_threshold_arg(
                    static_cast<std::size_t>(fallback_threshold)));
    // Sync communicator with the same arity/fallback_threshold as the tested
    // one, so it's hierarchical exactly when that one is (own communicator
    // avoids colliding with the tested collective's generations). A flat
    // sync here would let idle sites flood locality 0 while it's still
    // walking its own tree.
    auto const sync_communicators = create_hierarchical_communicator(
        "/test/sync_barrier/all_gather/hierarchical/",
        num_sites_arg(num_localities), this_site_arg(this_locality),
        arity_arg(arity), generation_arg(1), root_site_arg(0),
        fallback_threshold < 0 ?
            flat_fallback_threshold_arg() :
            flat_fallback_threshold_arg(
                static_cast<std::size_t>(fallback_threshold)));
    // Per-iteration times stay local; reduced once after the loop, off path.
    auto const timing_comm =
        create_communicator("/test/timing_reduce/all_gather/hierarchical/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<std::vector<int>> recv_data;
    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        std::vector<int> iter_data(static_cast<std::size_t>(test_size),
            static_cast<int>(i + this_locality));
        hpx::collectives::barrier(sync_communicators,
            this_site_arg(this_locality), generation_arg(i + 1))
            .get();
        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        hpx::future<std::vector<std::vector<int>>> ft_data =
            all_gather(communicators, std::move(iter_data),
                this_site_arg(this_locality), generation_arg(i + 1));
        recv_data = ft_data.get();
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;
        // Check for correctness: every site contributed (i + site), so
        // site j's whole block must equal (i + j).
        HPX_TEST_EQ(num_localities, recv_data.size());
        for (std::size_t j = 0; j != recv_data.size(); ++j)
        {
            HPX_TEST_EQ(
                static_cast<std::size_t>(test_size), recv_data[j].size());
            for (int value : recv_data[j])
            {
                if (!HPX_TEST_EQ(static_cast<int>(i + j), value))
                {
                    break;
                }
            }
        }
    }
    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        std::string const mod_name = fallback_threshold < 0 ?
            std::string("hierarchical") :
            "hierarchical_t" + std::to_string(fallback_threshold);
        write_to_file(operation, mod_name, arity, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}

void test_one_shot_use_all_to_all(int lpn, std::size_t iterations,
    std::size_t warmup_iterations, std::size_t cooldown_iterations,
    int test_size, std::string const& operation)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Barrier for synchronization
    char const* const barrier_test_name = "/test/barrier/single";
    hpx::distributed::barrier barrier(barrier_test_name);
    // Per-iteration times stay local; reduced once after the loop, off path.
    auto const timing_comm =
        create_communicator("/test/timing_reduce/all_to_all/one_shot/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::vector<double> result(iterations, 0.0);
    std::size_t const block_size = static_cast<std::size_t>(test_size);
    std::vector<std::vector<int>> send_data(
        num_localities, std::vector<int>(block_size, 0));
    std::vector<std::vector<int>> recv_data;

    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        for (std::size_t j = 0; j < num_localities; ++j)
        {
            std::fill(send_data[j].begin(), send_data[j].end(),
                static_cast<int>(this_locality + j + i));
        }
        barrier.wait();
        // Time collective
        auto iter_data = send_data;
        hpx::chrono::high_resolution_timer const timer;
        recv_data = all_to_all(all_to_all_direct_basename, std::move(iter_data),
            num_sites_arg(num_localities), this_site_arg(this_locality),
            generation_arg(i + 1), root_site_arg(0),
            benchmark_pairwise_threshold(block_size))
                        .get();
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;
        // Correctness: recv_data[s][*] == s + this_locality + i
        HPX_TEST_EQ(recv_data.size(), num_localities);
        for (std::size_t s = 0; s != num_localities; ++s)
        {
            HPX_TEST_EQ(recv_data[s].size(), block_size);
            for (int value : recv_data[s])
            {
                if (!HPX_TEST_EQ(
                        value, static_cast<int>(s + this_locality + i)))
                {
                    break;
                }
            }
        }
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        std::string const mod_name = pairwise_threshold_option == -1 ?
            "single_use" :
            "single_use_p" + std::to_string(pairwise_threshold_option);
        write_to_file(operation, mod_name, -1, num_localities, lpn, test_size,
            warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}

void test_multiple_use_with_generation_all_to_all(int lpn,
    std::size_t iterations, std::size_t warmup_iterations,
    std::size_t cooldown_iterations, int test_size,
    std::string const& operation)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Create communicator once for all iterations
    auto const comm = create_communicator(all_to_all_direct_basename,
        num_sites_arg(num_localities), this_site_arg(this_locality));
    // Barrier for synchronization
    char const* const barrier_test_name = "/test/barrier/generation";
    hpx::distributed::barrier barrier(barrier_test_name);
    // Per-iteration times stay local; reduced once after the loop, off path.
    auto const timing_comm =
        create_communicator("/test/timing_reduce/all_to_all/multi_use/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::vector<double> result(iterations, 0.0);
    std::size_t const block_size = static_cast<std::size_t>(test_size);
    std::vector<std::vector<int>> send_data(
        num_localities, std::vector<int>(block_size, 0));
    std::vector<std::vector<int>> recv_data;

    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        for (std::size_t j = 0; j < num_localities; ++j)
        {
            std::fill(send_data[j].begin(), send_data[j].end(),
                static_cast<int>(this_locality + j + i));
        }
        barrier.wait();
        // Time collective
        auto iter_data = send_data;
        hpx::chrono::high_resolution_timer const timer;
        recv_data = all_to_all(comm, std::move(iter_data),
            this_site_arg(this_locality), generation_arg(i + 1))
                        .get();
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;
        // Correctness: recv_data[s][*] == s + this_locality + i
        HPX_TEST_EQ(recv_data.size(), num_localities);
        for (std::size_t s = 0; s != num_localities; ++s)
        {
            HPX_TEST_EQ(recv_data[s].size(), block_size);
            for (int value : recv_data[s])
            {
                if (!HPX_TEST_EQ(
                        value, static_cast<int>(s + this_locality + i)))
                {
                    break;
                }
            }
        }
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        write_to_file(operation, "multi_use", -1, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}

void test_all_to_all_hierarchical(int arity, int lpn, std::size_t iterations,
    std::size_t warmup_iterations, std::size_t cooldown_iterations,
    int test_size, std::string const& operation, int fallback_threshold)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Create hierarchical communicators with optional threshold override
    auto communicators =
        create_hierarchical_communicator(all_to_all_direct_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality),
            arity_arg(arity), generation_arg(1), root_site_arg(0),
            fallback_threshold < 0 ?
                flat_fallback_threshold_arg() :
                flat_fallback_threshold_arg(
                    static_cast<std::size_t>(fallback_threshold)));
    // Sync communicator with the same arity/fallback_threshold as the tested
    // one, so it's hierarchical exactly when that one is (own communicator
    // avoids colliding with the tested collective's generations). A flat
    // sync here would let idle sites flood locality 0 while it's still
    // walking its own tree.
    auto const sync_communicators = create_hierarchical_communicator(
        "/test/sync_barrier/all_to_all/hierarchical/",
        num_sites_arg(num_localities), this_site_arg(this_locality),
        arity_arg(arity), generation_arg(1), root_site_arg(0),
        fallback_threshold < 0 ?
            flat_fallback_threshold_arg() :
            flat_fallback_threshold_arg(
                static_cast<std::size_t>(fallback_threshold)));
    // Per-iteration times stay local; reduced once after the loop, off path.
    auto const timing_comm =
        create_communicator("/test/timing_reduce/all_to_all/hierarchical/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::vector<double> result(iterations, 0.0);
    // Each destination block carries test_size ints
    std::size_t const block_size = static_cast<std::size_t>(test_size);
    std::vector<std::vector<int>> send_data(
        num_localities, std::vector<int>(block_size, 0));
    std::vector<std::vector<int>> recv_data;
    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        // Refill: block j carries (this_locality + j + i)
        for (std::size_t j = 0; j < num_localities; ++j)
        {
            std::fill(send_data[j].begin(), send_data[j].end(),
                static_cast<int>(this_locality + j + i));
        }
        hpx::collectives::barrier(sync_communicators,
            this_site_arg(this_locality), generation_arg(i + 1))
            .get();
        // Time collective
        // all_to_all consumes its payload via &&; copy the pre-filled buffer
        // so the pre-allocated shape survives for the next iteration.
        auto iter_data = send_data;
        hpx::chrono::high_resolution_timer const timer;
        hpx::future<std::vector<std::vector<int>>> ft_data =
            all_to_all(communicators, std::move(iter_data),
                this_site_arg(this_locality), generation_arg(i + 1));
        recv_data = ft_data.get();
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;
        // Correctness: recv_data[s][*] == s + this_locality + i
        HPX_TEST_EQ(recv_data.size(), num_localities);
        for (std::size_t s = 0; s != num_localities; ++s)
        {
            HPX_TEST_EQ(recv_data[s].size(), block_size);
            for (int value : recv_data[s])
            {
                if (!HPX_TEST_EQ(
                        value, static_cast<int>(s + this_locality + i)))
                {
                    break;
                }
            }
        }
    }
    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        std::string const mod_name = fallback_threshold < 0 ?
            std::string("hierarchical") :
            "hierarchical_t" + std::to_string(fallback_threshold);
        write_to_file(operation, mod_name, arity, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}
////////////////////////////////////////////////////////////////////////////////////////
void test_one_shot_use_all_gather(int lpn, std::size_t iterations,
    std::size_t warmup_iterations, std::size_t cooldown_iterations,
    int test_size, std::string const& operation)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Barrier for synchronization
    char const* const barrier_test_name = "/test/barrier/single";
    hpx::distributed::barrier barrier(barrier_test_name);
    // Per-iteration times stay local; reduced once after the loop, off path.
    auto const timing_comm =
        create_communicator("/test/timing_reduce/all_gather/one_shot/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<std::vector<int>> recv_data;
    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        std::vector<int> iter_data(static_cast<std::size_t>(test_size),
            static_cast<int>(i + this_locality));
        barrier.wait();
        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        hpx::future<std::vector<std::vector<int>>> ft_data =
            all_gather(all_gather_direct_basename, std::move(iter_data),
                num_sites_arg(num_localities), this_site_arg(this_locality),
                generation_arg(i + 1));
        recv_data = ft_data.get();
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;
        // Check for correctness
        HPX_TEST_EQ(num_localities, recv_data.size());
        for (std::size_t j = 0; j != recv_data.size(); ++j)
        {
            HPX_TEST_EQ(
                static_cast<std::size_t>(test_size), recv_data[j].size());
            for (int value : recv_data[j])
            {
                if (!HPX_TEST_EQ(static_cast<int>(i + j), value))
                {
                    break;
                }
            }
        }
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        write_to_file(operation, "single_use", -1, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}
////////////////////////////////////////////////////////////////////////////////////////
void test_multiple_use_with_generation_all_gather(int lpn,
    std::size_t iterations, std::size_t warmup_iterations,
    std::size_t cooldown_iterations, int test_size,
    std::string const& operation)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Create communicator
    auto const all_gather_direct_client =
        create_communicator(all_gather_direct_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality));
    // Barrier for synchronization
    char const* const barrier_test_name = "/test/barrier/generation";
    hpx::distributed::barrier barrier(barrier_test_name);
    // Per-iteration times stay local; reduced once after the loop, off path.
    auto const timing_comm =
        create_communicator("/test/timing_reduce/all_gather/multi_use/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<std::vector<int>> recv_data;
    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        std::vector<int> iter_data(static_cast<std::size_t>(test_size),
            static_cast<int>(i + this_locality));
        barrier.wait();
        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        hpx::future<std::vector<std::vector<int>>> ft_data =
            all_gather(all_gather_direct_client, std::move(iter_data),
                generation_arg(i + 1));
        recv_data = ft_data.get();
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;
        // Check for correctness
        HPX_TEST_EQ(num_localities, recv_data.size());
        for (std::size_t j = 0; j != recv_data.size(); ++j)
        {
            HPX_TEST_EQ(
                static_cast<std::size_t>(test_size), recv_data[j].size());
            for (int value : recv_data[j])
            {
                if (!HPX_TEST_EQ(static_cast<int>(i + j), value))
                {
                    break;
                }
            }
        }
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        write_to_file(operation, "multi_use", -1, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}
////////////////////////////////////////////////////////////////////////////////////////
void test_one_shot_use_exclusive_scan(int lpn, std::size_t iterations,
    std::size_t warmup_iterations, std::size_t cooldown_iterations,
    int test_size, std::string const& operation)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Barrier for synchronization
    char const* const barrier_test_name = "/test/barrier/single";
    hpx::distributed::barrier barrier(barrier_test_name);
    // Timing communicator
    auto const timing_comm =
        create_communicator("/test/timing_reduce/exclusive_scan/one_shot/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::size_t const block_size = static_cast<std::size_t>(test_size);
    std::vector<double> result(iterations, 0.0);
    std::vector<int> recv_data;
    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        std::vector<int> value(
            block_size, static_cast<int>(this_locality + 1 + i));
        barrier.wait();
        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        recv_data = exclusive_scan(exclusive_scan_basename, std::move(value),
            std::vector<int>(block_size, 0), vector_adder{},
            num_sites_arg(num_localities), this_site_arg(this_locality),
            generation_arg(i + 1))
                        .get();
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;
        int expected = 0;
        for (std::size_t j = 0; j < this_locality; ++j)
            expected += static_cast<int>(j + 1 + i);
        HPX_TEST_EQ(recv_data.size(), block_size);
        for (int value : recv_data)
        {
            if (!HPX_TEST_EQ(value, expected))
            {
                break;
            }
        }
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        write_to_file(operation, "single_use", -1, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}
////////////////////////////////////////////////////////////////////////////////////////
void test_multiple_use_with_generation_exclusive_scan(int lpn,
    std::size_t iterations, std::size_t warmup_iterations,
    std::size_t cooldown_iterations, int test_size,
    std::string const& operation)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Create communicator
    auto const exclusive_scan_client =
        create_communicator(exclusive_scan_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality));
    // Barrier for synchronization
    char const* const barrier_test_name = "/test/barrier/generation";
    hpx::distributed::barrier barrier(barrier_test_name);
    // Timing communicator
    auto const timing_comm =
        create_communicator("/test/timing_reduce/exclusive_scan/multi_use/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::size_t const block_size = static_cast<std::size_t>(test_size);
    std::vector<double> result(iterations, 0.0);
    std::vector<int> recv_data;
    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        std::vector<int> value(
            block_size, static_cast<int>(this_locality + 1 + i));
        barrier.wait();
        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        recv_data = exclusive_scan(exclusive_scan_client, std::move(value),
            std::vector<int>(block_size, 0), vector_adder{},
            generation_arg(i + 1))
                        .get();
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;
        int expected = 0;
        for (std::size_t j = 0; j < this_locality; ++j)
            expected += static_cast<int>(j + 1 + i);
        HPX_TEST_EQ(recv_data.size(), block_size);
        for (int value : recv_data)
        {
            if (!HPX_TEST_EQ(value, expected))
            {
                break;
            }
        }
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        write_to_file(operation, "multi_use", -1, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}
void test_one_shot_use_inclusive_scan(int lpn, std::size_t iterations,
    std::size_t warmup_iterations, std::size_t cooldown_iterations,
    int test_size, std::string const& operation)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Barrier for synchronization
    char const* const barrier_test_name = "/test/barrier/single";
    hpx::distributed::barrier barrier(barrier_test_name);
    // Timing communicator
    auto const timing_comm =
        create_communicator("/test/timing_reduce/inclusive_scan/one_shot/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::size_t const block_size = static_cast<std::size_t>(test_size);
    std::vector<double> result(iterations, 0.0);
    std::vector<int> recv_data;
    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        std::vector<int> value(
            block_size, static_cast<int>(this_locality + 1 + i));
        barrier.wait();
        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        recv_data = inclusive_scan(inclusive_scan_basename, std::move(value),
            vector_adder{}, num_sites_arg(num_localities),
            this_site_arg(this_locality), generation_arg(i + 1))
                        .get();
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;
        int expected = 0;
        for (std::size_t j = 0; j <= this_locality; ++j)
            expected += static_cast<int>(j + 1 + i);
        HPX_TEST_EQ(recv_data.size(), block_size);
        for (int value : recv_data)
        {
            if (!HPX_TEST_EQ(value, expected))
            {
                break;
            }
        }
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        write_to_file(operation, "single_use", -1, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}
////////////////////////////////////////////////////////////////////////////////////////
void test_multiple_use_with_generation_inclusive_scan(int lpn,
    std::size_t iterations, std::size_t warmup_iterations,
    std::size_t cooldown_iterations, int test_size,
    std::string const& operation)
{
    // Get parameters
    std::size_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::size_t const this_locality = hpx::get_locality_id();
    // Ensure at least two localities
    HPX_TEST_LTE(static_cast<std::size_t>(2), num_localities);
    // Create communicator
    auto const inclusive_scan_client =
        create_communicator(inclusive_scan_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality));
    // Barrier for synchronization
    char const* const barrier_test_name = "/test/barrier/generation";
    hpx::distributed::barrier barrier(barrier_test_name);
    // Timing communicator
    auto const timing_comm =
        create_communicator("/test/timing_reduce/inclusive_scan/multi_use/",
            num_sites_arg(num_localities), this_site_arg(this_locality));
    std::size_t const block_size = static_cast<std::size_t>(test_size);
    std::vector<double> result(iterations, 0.0);
    std::vector<int> recv_data;
    for (std::size_t i = 0;
        i != warmup_iterations + iterations + cooldown_iterations; ++i)
    {
        std::vector<int> value(
            block_size, static_cast<int>(this_locality + 1 + i));
        barrier.wait();
        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        recv_data = inclusive_scan(inclusive_scan_client, std::move(value),
            vector_adder{}, generation_arg(i + 1))
                        .get();
        double const elapsed = timer.elapsed();
        if (i >= warmup_iterations && i < warmup_iterations + iterations)
            result[i - warmup_iterations] = elapsed;
        int expected = 0;
        for (std::size_t j = 0; j <= this_locality; ++j)
            expected += static_cast<int>(j + 1 + i);
        HPX_TEST_EQ(recv_data.size(), block_size);
        for (int value : recv_data)
        {
            if (!HPX_TEST_EQ(value, expected))
            {
                break;
            }
        }
    }

    // Aggregate per-iteration maxima once, off the measured path.
    reduce(timing_comm, result, vector_double_max{},
        this_site_arg(this_locality), generation_arg(1));

    if (this_locality == 0)
    {
        write_to_file(operation, "multi_use", -1, num_localities, lpn,
            test_size, warmup_iterations, cooldown_iterations, iterations,
            std::move(result));
    }
}
struct benchmarking_functions
{
    hpx::function<void(
        int, std::size_t, std::size_t, std::size_t, int, std::string const&)>
        one_shot;
    hpx::function<void(
        int, std::size_t, std::size_t, std::size_t, int, std::string const&)>
        multiple_use;
    hpx::function<void(int, int, std::size_t, std::size_t, std::size_t, int,
        std::string const&, int)>
        hierarchical;
};

int hpx_main(hpx::program_options::variables_map& vm)
{
    int const arity = vm["arity"].as<int>();
    int const lpn = vm["lpn"].as<int>();
    int const test_size = vm["test_size"].as<int>();
    std::string const operation = vm["operation"].as<std::string>();
    int const iterations = vm["iterations"].as<int>();
    int const fallback_threshold = vm["fallback_threshold"].as<int>();
    int const warmup_iterations = vm["warmup_iterations"].as<int>();
    int const cooldown_iterations = vm["cooldown_iterations"].as<int>();
    bool const skip_one_shot = vm["skip_one_shot"].as<bool>();
    pairwise_threshold_option = vm["pairwise_threshold"].as<int>();

    if (iterations <= 0 || warmup_iterations < 0 || cooldown_iterations < 0 ||
        test_size <= 0 || lpn <= 0 || pairwise_threshold_option < -1)
    {
        std::cout << "error: iterations and test_size and lpn must be > 0; "
                     "warmup_iterations and cooldown_iterations must be >= 0; "
                     "pairwise_threshold must be >= -1\n";
        return hpx::finalize();
    }

    if (hpx::get_num_localities(hpx::launch::sync) > 1)
    {
        std::map<std::string, benchmarking_functions> const benchmarking = {
            {"scatter",
                {test_one_shot_use_scatter,
                    test_multiple_use_with_generation_scatter,
                    test_scatter_hierarchical}},
            {"reduce",
                {test_one_shot_use_reduce,
                    test_multiple_use_with_generation_reduce,
                    test_reduce_hierarchical}},
            {"broadcast",
                {test_one_shot_use_broadcast,
                    test_multiple_use_with_generation_broadcast,
                    test_broadcast_hierarchical}},
            {"gather",
                {test_one_shot_use_gather,
                    test_multiple_use_with_generation_gather,
                    test_gather_hierarchical}},
            {"all_reduce",
                {test_one_shot_use_all_reduce,
                    test_multiple_use_with_generation_all_reduce,
                    test_all_reduce_hierarchical}},
            {"all_gather",
                {test_one_shot_use_all_gather,
                    test_multiple_use_with_generation_all_gather,
                    test_all_gather_hierarchical}},
            {"all_to_all",
                {test_one_shot_use_all_to_all,
                    test_multiple_use_with_generation_all_to_all,
                    test_all_to_all_hierarchical}},
            {"barrier",
                {test_one_shot_use_barrier,
                    test_multiple_use_with_generation_barrier,
                    test_barrier_hierarchical}},
            {"exclusive_scan",
                {test_one_shot_use_exclusive_scan,
                    test_multiple_use_with_generation_exclusive_scan,
                    test_exclusive_scan_hierarchical}},
            {"inclusive_scan",
                {test_one_shot_use_inclusive_scan,
                    test_multiple_use_with_generation_inclusive_scan,
                    test_inclusive_scan_hierarchical}},
        };

        auto it = benchmarking.find(operation);
        if (it == benchmarking.end())
        {
            std::cout << "error: unknown operation '" << operation << "'\n";
            return hpx::finalize();
        }
        if (arity == -1)
        {
            if (!skip_one_shot)
            {
                it->second.one_shot(lpn, iterations,
                    static_cast<std::size_t>(warmup_iterations),
                    static_cast<std::size_t>(cooldown_iterations), test_size,
                    operation);
            }
            it->second.multiple_use(lpn, iterations,
                static_cast<std::size_t>(warmup_iterations),
                static_cast<std::size_t>(cooldown_iterations), test_size,
                operation);
        }
        else
        {
            it->second.hierarchical(arity, lpn, iterations,
                static_cast<std::size_t>(warmup_iterations),
                static_cast<std::size_t>(cooldown_iterations), test_size,
                operation, fallback_threshold);
        }
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;

    // clang-format off
    options_description desc_commandline;
    desc_commandline.add_options()
        ("arity", value<int>()->default_value(-1), "Arity of the operation")
        ("lpn", value<int>()->default_value(2),
            "Number of localities per Node")
        ("test_size", value<int>()->default_value(2),
            "Number of Integers One Locality receives")
        ("iterations", value<int>()->default_value(10),
            "Number of Iteration the collective is executed")
        ("operation", value<std::string>()->default_value("scatter"),
            "Collective Operation (scatter, reduce, broadcast, gather, "
            "all_reduce, all_gather, all_to_all, barrier, "
            "exclusive_scan, inclusive_scan)")
        ("fallback_threshold", value<int>()->default_value(-1),
            "Flat fallback threshold for hierarchical mode. -1 uses library "
            "default (16). Set to 0 to force tree construction. Only meaningful "
            "with hierarchical mode.")
        ("warmup_iterations", value<int>()->default_value(3),
            "Number of warmup iterations before the timed loop (default 3)")
        ("cooldown_iterations", value<int>()->default_value(0),
            "Number of extra iterations run after the measured ones and "
            "discarded, giving the last measured iteration a successor too "
            "(default 0)")
        ("pairwise_threshold", value<int>()->default_value(-1),
            "Per-pair payload size in bytes at or above which the one-shot "
            "all_to_all exchanges rows directly between sites. -1 uses the "
            "library default. Set to 0 to force the direct exchange. Only "
            "meaningful with --operation=all_to_all and --arity=-1.")
        ("skip_one_shot",
            value<bool>()->implicit_value(true)->default_value(false),
            "Skip the one-shot (single_use) benchmark when --arity=-1, "
            "running only the multi_use benchmark. Accepts a bare "
            "--skip_one_shot or an explicit --skip_one_shot=true/false.");
    // clang-format on

    std::vector<std::string> const cfg = {"hpx.run_hpx_main!=1"};

    hpx::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ(hpx::init(argc, argv, init_args), 0);
    return hpx::util::report_errors();
}

#endif
