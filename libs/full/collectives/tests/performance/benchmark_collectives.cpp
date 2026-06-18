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
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace hpx::collectives;

constexpr char const* scatter_direct_basename = "/test/scatter_direct/";
constexpr char const* reduce_direct_basename = "/test/reduce_direct/";
constexpr char const* broadcast_direct_basename = "/test/broadcast_direct/";
constexpr char const* gather_direct_basename = "/test/gather_direct/";
constexpr char const* all_reduce_direct_basename = "/test/all_reduce_direct/";
constexpr char const* all_gather_direct_basename = "/test/all_gather_direct/";
constexpr char const* all_to_all_direct_basename = "/test/all_to_all_direct/";
constexpr char const* barrier_bench_basename = "/test/barrier_bench/";

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

std::vector<std::vector<int>> generate_matrix(
    int localities, int test_size, int start_val)
{
    std::vector<std::vector<int>> result;
    for (int i = 0; i < localities; ++i)
    {
        result.push_back(std::vector<int>(test_size, start_val + i));
    }
    return result;
}

void create_parent_dir(std::filesystem::path const& file_path)
{
    // Create parent directory if does not exist
    std::filesystem::path dir_path = file_path.parent_path();
    if (!std::filesystem::exists(dir_path))
    {
        if (std::filesystem::create_directories(dir_path))
        {
            std::cout << "Directory created: " << dir_path << "\n";
        }
        else
        {
            throw std::runtime_error(
                "Failed to create directory: " + dir_path.string());
        }
    }
}

std::pair<double, double> compute_moments(std::vector<double> const& data)
{
    // Compute mean
    double sum = 0.0;
    for (double x : data)
    {
        sum += x;
    }
    double mean = sum / static_cast<double>(data.size());

    // Compute variance (population variance)
    double varianceSum = 0.0;
    for (double x : data)
    {
        varianceSum += (x - mean) * (x - mean);
    }
    double variance = varianceSum / static_cast<double>(data.size());

    return std::make_pair(mean, variance);
}

void write_to_file(std::string const& collective, std::string const& type,
    int arity, std::size_t num_l, int lpn, int size, std::size_t iterations,
    std::vector<double> const& result)
{
    // Compute mean and variance
    auto moments = compute_moments(result);
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
                      "Iterations:        {9}\n"
                      "Mean runtime:      {10}\n"
                      "Variance:          {11}\n";
    hpx::util::format_to(std::cout, msg, collective, type, arity, nodes, num_l,
        lpn, threads, size, iterations, moments.first, moments.second)
        << std::flush;

    // Create directory
    std::string runtime_file_path =
        "result/hpx/" + collective + "/runtimes_" + collective + "_" + type;
    if (arity != -1 && type == "hierarchical")
    {
        runtime_file_path = runtime_file_path + "_" + std::to_string(arity);
    }
    runtime_file_path = runtime_file_path + ".txt";
    create_parent_dir(runtime_file_path);

    // Add header if necessary
    std::string const header = "collective;type;arity;nodes;localities;lpn;"
                               "threads;size;iterations;mean;variance\n";
    // Read existing content
    std::ifstream infile(runtime_file_path);
    std::stringstream buffer;
    buffer << infile.rdbuf();
    std::string contents = buffer.str();
    infile.close();
    // Only append if header not present
    if (contents.find(header) == std::string::npos)
    {
        std::ofstream outfile(runtime_file_path, std::ios_base::app);
        outfile << header;
        outfile.close();
    }

    // Add runtimes
    std::ofstream outfile;
    outfile.open(runtime_file_path, std::ios_base::app);
    outfile << collective << ";" << type << ";" << arity << ";" << nodes << ";"
            << num_l << ";" << lpn << ";" << threads << ";" << size << ";"
            << iterations << ";" << moments.first << ";" << moments.second
            << "\n";
    outfile.close();
}

////////////////////////////////////////////////////////////////////////////////////////
// Hierarchical collectives
void test_scatter_hierarchical(int arity, int lpn, std::size_t iterations,
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
    // Barrier for synchronization
    char const* const barrier_test_name = "/test/barrier/hierarchical";
    hpx::distributed::barrier barrier(barrier_test_name);
    // Result vector
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<std::vector<int>> send_data;
    std::vector<int> recv_data;
    hpx::future<std::vector<int>> ft_data;

    for (std::size_t i = 0; i != iterations; ++i)
    {
        if (this_locality == 0)
        {
            send_data = generate_matrix(static_cast<int>(num_localities),
                test_size, static_cast<int>(42 + i));
        }

        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        if (this_locality == 0)
        {
            // NOLINTNEXTLINE(bugprone-use-after-move)
            ft_data = scatter_to(communicators, std::move(send_data),
                this_site_arg(this_locality), generation_arg(i + 1));
        }
        else
        {
            ft_data = scatter_from<std::vector<int>>(communicators,
                this_site_arg(this_locality), generation_arg(i + 1));
        }
        recv_data = ft_data.get();
        // Synchronize
        barrier.wait();
        // Write runtime into vector
        result[i] = timer.elapsed();

        // Check for correctness
        for (std::size_t check : recv_data)
        {
            HPX_TEST_EQ(42 + i + this_locality, check);
        }
    }

    if (this_locality == 0)
    {
        std::string const mod_name = fallback_threshold < 0 ?
            std::string("hierarchical") :
            "hierarchical_t" + std::to_string(fallback_threshold);
        write_to_file(operation, mod_name, arity, num_localities, lpn,
            test_size, iterations, result);
    }
}

void test_reduce_hierarchical(int arity, int lpn, std::size_t iterations,
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
    // Barrier for synchronization
    char const* const barrier_test_name = "/test/barrier/hierarchical";
    hpx::distributed::barrier barrier(barrier_test_name);
    // Result vector
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<int> send_data;
    std::vector<int> recv_data;
    hpx::future<std::vector<int>> ft_data;

    for (std::size_t i = 0; i != iterations; ++i)
    {
        send_data = std::vector<int>(test_size, static_cast<int>(i));

        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        if (this_locality == 0)
        {
            ft_data =
                // NOLINTNEXTLINE(bugprone-use-after-move)
                reduce_here(communicators, std::move(send_data), vector_adder{},
                    this_site_arg(this_locality), generation_arg(i + 1));
            recv_data = ft_data.get();
        }
        else
        {
            hpx::future<void> finished = reduce_there(communicators,
                // NOLINTNEXTLINE(bugprone-use-after-move)
                std::move(send_data), vector_adder{},
                this_site_arg(this_locality), generation_arg(i + 1));
            finished.get();
        }
        // Synchronize
        barrier.wait();
        // Write runtime into vector
        result[i] = timer.elapsed();

        // Check for correctness
        if (this_locality == 0)
        {
            HPX_TEST_EQ(
                i * num_localities, static_cast<std::size_t>(recv_data[0]));
        }
    }

    if (this_locality == 0)
    {
        std::string const mod_name = fallback_threshold < 0 ?
            std::string("hierarchical") :
            "hierarchical_t" + std::to_string(fallback_threshold);
        write_to_file(operation, mod_name, arity, num_localities, lpn,
            test_size, iterations, result);
    }
}

void test_broadcast_hierarchical(int arity, int lpn, std::size_t iterations,
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
    // Barrier for synchronization
    char const* const barrier_test_name = "/test/barrier/hierarchical";
    hpx::distributed::barrier barrier(barrier_test_name);
    // Result vector
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<int> send_data;
    std::vector<int> recv_data;
    hpx::future<std::vector<int>> ft_data;

    for (std::size_t i = 0; i != iterations; ++i)
    {
        if (this_locality == 0)
        {
            send_data = std::vector<int>(test_size, static_cast<int>(i));
        }

        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        if (this_locality == 0)
        {
            // NOLINTNEXTLINE(bugprone-use-after-move)
            ft_data = broadcast_to(communicators, std::move(send_data),
                this_site_arg(this_locality), generation_arg(i + 1));
        }
        else
        {
            ft_data = broadcast_from<std::vector<int>>(communicators,
                this_site_arg(this_locality), generation_arg(i + 1));
        }
        recv_data = ft_data.get();
        // Synchronize
        barrier.wait();
        // Write runtime into vector
        result[i] = timer.elapsed();

        // Check for correctness
        if (this_locality == 0)
        {
            HPX_TEST_EQ(i, static_cast<std::size_t>(recv_data[0]));
        }
    }

    if (this_locality == 0)
    {
        std::string const mod_name = fallback_threshold < 0 ?
            std::string("hierarchical") :
            "hierarchical_t" + std::to_string(fallback_threshold);
        write_to_file(operation, mod_name, arity, num_localities, lpn,
            test_size, iterations, result);
    }
}

void test_gather_hierarchical(int arity, int lpn, std::size_t iterations,
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
    // Barrier for synchronization
    char const* const barrier_test_name = "/test/barrier/hierarchical";
    hpx::distributed::barrier barrier(barrier_test_name);
    // Result vector
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<int> send_data;
    std::vector<std::vector<int>> recv_data;
    hpx::future<std::vector<std::vector<int>>> ft_data;

    for (std::size_t i = 0; i != iterations; ++i)
    {
        send_data =
            std::vector<int>(test_size, static_cast<int>(i + this_locality));

        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        if (this_locality == 0)
        {
            // NOLINTNEXTLINE(bugprone-use-after-move)
            ft_data = gather_here(communicators, std::move(send_data),
                this_site_arg(this_locality), generation_arg(i + 1));
            recv_data = ft_data.get();
        }
        else
        {
            hpx::future<void> finished =
                // NOLINTNEXTLINE(bugprone-use-after-move)
                gather_there(communicators, std::move(send_data),
                    this_site_arg(this_locality), generation_arg(i + 1));
            finished.get();
        }
        // Synchronize
        barrier.wait();
        // Write runtime into vector
        result[i] = timer.elapsed();

        // Check for correctness
        if (this_locality == 0)
        {
            for (int j = 0; j < static_cast<int>(num_localities); ++j)
            {
                HPX_TEST_EQ(i + j, static_cast<std::size_t>(recv_data[j][0]));
            }
        }
    }

    if (this_locality == 0)
    {
        std::string const mod_name = fallback_threshold < 0 ?
            std::string("hierarchical") :
            "hierarchical_t" + std::to_string(fallback_threshold);
        write_to_file(operation, mod_name, arity, num_localities, lpn,
            test_size, iterations, result);
    }
}

void test_all_reduce_hierarchical(int arity, int lpn, std::size_t iterations,
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
    // Barrier for synchronization
    char const* const barrier_test_name = "/test/barrier/hierarchical";
    hpx::distributed::barrier barrier(barrier_test_name);
    // Result vector
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<int> send_data;
    std::vector<int> recv_data;

    for (std::size_t i = 0; i != iterations; ++i)
    {
        send_data = std::vector<int>(test_size, static_cast<int>(i));

        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        // NOLINTNEXTLINE(bugprone-use-after-move)
        hpx::future<std::vector<int>> ft_data =
            all_reduce(communicators, std::move(send_data), vector_adder{},
                this_site_arg(this_locality), generation_arg(i + 1));
        recv_data = ft_data.get();

        // Synchronize
        barrier.wait();
        // Write runtime into vector
        result[i] = timer.elapsed();

        // Check for correctness: every site should have the sum
        HPX_TEST_EQ(i * num_localities, static_cast<std::size_t>(recv_data[0]));
    }

    if (this_locality == 0)
    {
        std::string const mod_name = fallback_threshold < 0 ?
            std::string("hierarchical") :
            "hierarchical_t" + std::to_string(fallback_threshold);
        write_to_file(operation, mod_name, arity, num_localities, lpn,
            test_size, iterations, result);
    }
}

void test_barrier_hierarchical(int arity, int lpn, std::size_t iterations,
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
    char const* const barrier_sync_name = "/test/barrier/hierarchical";
    hpx::distributed::barrier sync(barrier_sync_name);
    std::vector<double> result(iterations, 0.0);

    for (std::size_t i = 0; i != iterations; ++i)
    {
        hpx::chrono::high_resolution_timer const timer;
        hpx::collectives::barrier(
            communicators, this_site_arg(this_locality), generation_arg(i + 1))
            .get();
        sync.wait();
        result[i] = timer.elapsed();
    }

    if (this_locality == 0)
    {
        std::string const mod_name = fallback_threshold < 0 ?
            std::string("hierarchical") :
            "hierarchical_t" + std::to_string(fallback_threshold);
        write_to_file(operation, mod_name, arity, num_localities, lpn,
            test_size, iterations, result);
    }
}

////////////////////////////////////////////////////////////////////////////////////////
// One shot collectives
void test_one_shot_use_scatter(int lpn, std::size_t iterations, int test_size,
    std::string const& operation)
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
    // Result vector
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<std::vector<int>> send_data;
    std::vector<int> recv_data;
    hpx::future<std::vector<int>> ft_data;

    for (std::size_t i = 0; i != iterations; ++i)
    {
        if (this_locality == 0)
        {
            send_data = generate_matrix(static_cast<int>(num_localities),
                test_size, static_cast<int>(42 + i));
        }

        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        if (this_locality == 0)
        {
            // NOLINTNEXTLINE(bugprone-use-after-move)
            ft_data = scatter_to(scatter_direct_basename, std::move(send_data),
                num_sites_arg(num_localities), this_site_arg(this_locality),
                generation_arg(i + 1));
        }
        else
        {
            ft_data = scatter_from<std::vector<int>>(scatter_direct_basename,
                this_site_arg(this_locality), generation_arg(i + 1));
        }
        recv_data = ft_data.get();
        // Synchronize
        barrier.wait();
        // Write runtime into vector
        result[i] = timer.elapsed();

        // Check for correctness
        for (std::size_t check : recv_data)
        {
            HPX_TEST_EQ(42 + i + this_locality, check);
        }
    }

    if (this_locality == 0)
    {
        write_to_file(operation, "single_use", -1, num_localities, lpn,
            test_size, iterations, result);
    }
}

void test_one_shot_use_reduce(int lpn, std::size_t iterations, int test_size,
    std::string const& operation)
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
    // Result vector
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<int> send_data;
    std::vector<int> recv_data;
    hpx::future<std::vector<int>> ft_data;

    for (std::size_t i = 0; i != iterations; ++i)
    {
        send_data = std::vector<int>(test_size, static_cast<int>(i));

        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        if (this_locality == 0)
        {
            // NOLINTNEXTLINE(bugprone-use-after-move)
            ft_data = reduce_here(reduce_direct_basename, std::move(send_data),
                vector_adder{}, num_sites_arg(num_localities),
                this_site_arg(this_locality), generation_arg(i + 1));
            recv_data = ft_data.get();
        }
        else
        {
            hpx::future<void> finished =
                // NOLINTNEXTLINE(bugprone-use-after-move)
                reduce_there(reduce_direct_basename, std::move(send_data),
                    this_site_arg(this_locality), generation_arg(i + 1));
            finished.get();
        }
        // Synchronize
        barrier.wait();
        // Write runtime into vector
        result[i] = timer.elapsed();

        // Check for correctness
        if (this_locality == 0)
        {
            HPX_TEST_EQ(
                i * num_localities, static_cast<std::size_t>(recv_data[0]));
        }
    }

    if (this_locality == 0)
    {
        write_to_file(operation, "single_use", -1, num_localities, lpn,
            test_size, iterations, result);
    }
}

void test_one_shot_use_broadcast(int lpn, std::size_t iterations, int test_size,
    std::string const& operation)
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
    // Result vector
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<int> send_data;
    std::vector<int> recv_data;
    hpx::future<std::vector<int>> ft_data;

    for (std::size_t i = 0; i != iterations; ++i)
    {
        if (this_locality == 0)
        {
            send_data = std::vector<int>(test_size, static_cast<int>(i));
        }

        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        if (this_locality == 0)
        {
            ft_data = broadcast_to(broadcast_direct_basename,
                // NOLINTNEXTLINE(bugprone-use-after-move)
                std::move(send_data), num_sites_arg(num_localities),
                this_site_arg(this_locality), generation_arg(i + 1));
        }
        else
        {
            ft_data =
                broadcast_from<std::vector<int>>(broadcast_direct_basename,
                    this_site_arg(this_locality), generation_arg(i + 1));
        }
        recv_data = ft_data.get();
        // Synchronize
        barrier.wait();
        // Write runtime into vector
        result[i] = timer.elapsed();

        // Check for correctness
        if (this_locality == 0)
        {
            HPX_TEST_EQ(i, static_cast<std::size_t>(recv_data[0]));
        }
    }

    if (this_locality == 0)
    {
        write_to_file(operation, "single_use", -1, num_localities, lpn,
            test_size, iterations, result);
    }
}

void test_one_shot_use_gather(int lpn, std::size_t iterations, int test_size,
    std::string const& operation)
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
    // Result vector
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<int> send_data;
    std::vector<std::vector<int>> recv_data;
    hpx::future<std::vector<std::vector<int>>> ft_data;

    for (std::size_t i = 0; i != iterations; ++i)
    {
        send_data =
            std::vector<int>(test_size, static_cast<int>(i + this_locality));

        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        if (this_locality == 0)
        {
            // NOLINTNEXTLINE(bugprone-use-after-move)
            ft_data = gather_here(gather_direct_basename, std::move(send_data),
                num_sites_arg(num_localities), this_site_arg(this_locality),
                generation_arg(i + 1));
            recv_data = ft_data.get();
        }
        else
        {
            hpx::future<void> finished =
                // NOLINTNEXTLINE(bugprone-use-after-move)
                gather_there(gather_direct_basename, std::move(send_data),
                    this_site_arg(this_locality), generation_arg(i + 1));
            finished.get();
        }
        // Synchronize
        barrier.wait();
        // Write runtime into vector
        result[i] = timer.elapsed();

        // Check for correctness
        if (this_locality == 0)
        {
            for (int j = 0; j < static_cast<int>(num_localities); ++j)
            {
                HPX_TEST_EQ(i + j, static_cast<std::size_t>(recv_data[j][0]));
            }
        }
    }

    if (this_locality == 0)
    {
        write_to_file(operation, "single_use", -1, num_localities, lpn,
            test_size, iterations, result);
    }
}

void test_one_shot_use_all_reduce(int lpn, std::size_t iterations,
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
    // Result vector
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<int> send_data;
    std::vector<int> recv_data;

    for (std::size_t i = 0; i != iterations; ++i)
    {
        send_data = std::vector<int>(test_size, static_cast<int>(i));

        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        hpx::future<std::vector<int>> ft_data =
            // NOLINTNEXTLINE(bugprone-use-after-move)
            all_reduce(all_reduce_direct_basename, std::move(send_data),
                vector_adder{}, num_sites_arg(num_localities),
                this_site_arg(this_locality), generation_arg(i + 1));
        recv_data = ft_data.get();
        // Synchronize
        barrier.wait();
        // Write runtime into vector
        result[i] = timer.elapsed();

        // Check for correctness
        HPX_TEST_EQ(i * num_localities, static_cast<std::size_t>(recv_data[0]));
    }

    if (this_locality == 0)
    {
        write_to_file(operation, "single_use", -1, num_localities, lpn,
            test_size, iterations, result);
    }
}

////////////////////////////////////////////////////////////////////////////////////////
// Multi-use shot collectives
void test_multiple_use_with_generation_scatter(int lpn, std::size_t iterations,
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
    // Result vector
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<std::vector<int>> send_data;
    std::vector<int> recv_data;
    hpx::future<std::vector<int>> ft_data;

    for (std::size_t i = 0; i != iterations; ++i)
    {
        if (this_locality == 0)
        {
            send_data = generate_matrix(static_cast<int>(num_localities),
                test_size, static_cast<int>(42 + i));
        }

        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        if (this_locality == 0)
        {
            // NOLINTNEXTLINE(bugprone-use-after-move)
            ft_data = scatter_to(scatter_direct_client, std::move(send_data),
                generation_arg(i + 1));
        }
        else
        {
            ft_data = scatter_from<std::vector<int>>(
                scatter_direct_client, generation_arg(i + 1));
        }
        recv_data = ft_data.get();
        // Synchronize
        barrier.wait();
        // Write runtime into vector
        result[i] = timer.elapsed();

        // Check for correctness
        for (std::size_t check : recv_data)
        {
            HPX_TEST_EQ(42 + i + this_locality, check);
        }
    }

    if (this_locality == 0)
    {
        write_to_file(operation, "multi_use", -1, num_localities, lpn,
            test_size, iterations, result);
    }
}

void test_multiple_use_with_generation_reduce(int lpn, std::size_t iterations,
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
    // Result vector
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<int> send_data;
    std::vector<int> recv_data;
    hpx::future<std::vector<int>> ft_data;

    for (std::size_t i = 0; i != iterations; ++i)
    {
        send_data = std::vector<int>(test_size, static_cast<int>(i));

        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        if (this_locality == 0)
        {
            // NOLINTNEXTLINE(bugprone-use-after-move)
            ft_data = reduce_here(reduce_direct_client, std::move(send_data),
                vector_adder{}, generation_arg(i + 1));
            recv_data = ft_data.get();
        }
        else
        {
            hpx::future<void> finished = reduce_there(reduce_direct_client,
                // NOLINTNEXTLINE(bugprone-use-after-move)
                std::move(send_data), generation_arg(i + 1));
            finished.get();
        }
        // Synchronize
        barrier.wait();
        // Write runtime into vector
        result[i] = timer.elapsed();

        // Check for correctness
        if (this_locality == 0)
        {
            HPX_TEST_EQ(
                i * num_localities, static_cast<std::size_t>(recv_data[0]));
        }
    }

    if (this_locality == 0)
    {
        write_to_file(operation, "multi_use", -1, num_localities, lpn,
            test_size, iterations, result);
    }
}

void test_multiple_use_with_generation_broadcast(int lpn,
    std::size_t iterations, int test_size, std::string const& operation)
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
    // Result vector
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<int> send_data;
    std::vector<int> recv_data;
    hpx::future<std::vector<int>> ft_data;

    for (std::size_t i = 0; i != iterations; ++i)
    {
        if (this_locality == 0)
        {
            send_data = std::vector<int>(test_size, static_cast<int>(i));
        }

        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        if (this_locality == 0)
        {
            ft_data = broadcast_to(broadcast_direct_client,
                // NOLINTNEXTLINE(bugprone-use-after-move)
                std::move(send_data), generation_arg(i + 1));
        }
        else
        {
            ft_data = broadcast_from<std::vector<int>>(
                broadcast_direct_client, generation_arg(i + 1));
        }
        recv_data = ft_data.get();
        // Synchronize
        barrier.wait();
        // Write runtime into vector
        result[i] = timer.elapsed();

        // Check for correctness
        if (this_locality == 0)
        {
            HPX_TEST_EQ(i, static_cast<std::size_t>(recv_data[0]));
        }
    }

    if (this_locality == 0)
    {
        write_to_file(operation, "multi_use", -1, num_localities, lpn,
            test_size, iterations, result);
    }
}

void test_multiple_use_with_generation_gather(
    int lpn, std::size_t iterations, int test_size, std::string operation)
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
    // Result vector
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<int> send_data;
    std::vector<std::vector<int>> recv_data;
    hpx::future<std::vector<std::vector<int>>> ft_data;

    for (std::size_t i = 0; i != iterations; ++i)
    {
        send_data =
            std::vector<int>(test_size, static_cast<int>(i + this_locality));

        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        if (this_locality == 0)
        {
            // NOLINTNEXTLINE(bugprone-use-after-move)
            ft_data = gather_here(gather_direct_client, std::move(send_data),
                generation_arg(i + 1));
            recv_data = ft_data.get();
        }
        else
        {
            hpx::future<void> finished = gather_there(gather_direct_client,
                // NOLINTNEXTLINE(bugprone-use-after-move)
                std::move(send_data), generation_arg(i + 1));
            finished.get();
        }
        // Synchronize
        barrier.wait();
        // Write runtime into vector
        result[i] = timer.elapsed();

        // Check for correctness
        if (this_locality == 0)
        {
            for (int j = 0; j < static_cast<int>(num_localities); ++j)
            {
                HPX_TEST_EQ(i + j, static_cast<std::size_t>(recv_data[j][0]));
            }
        }
    }

    if (this_locality == 0)
    {
        write_to_file(operation, "multi_use", -1, num_localities, lpn,
            test_size, iterations, result);
    }
}

void test_multiple_use_with_generation_all_reduce(int lpn,
    std::size_t iterations, int test_size, std::string const& operation)
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
    // Result vector
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<int> send_data;
    std::vector<int> recv_data;

    for (std::size_t i = 0; i != iterations; ++i)
    {
        send_data = std::vector<int>(test_size, static_cast<int>(i));

        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        hpx::future<std::vector<int>> ft_data =
            // NOLINTNEXTLINE(bugprone-use-after-move)
            all_reduce(all_reduce_direct_client, std::move(send_data),
                vector_adder{}, generation_arg(i + 1));
        recv_data = ft_data.get();
        // Synchronize
        barrier.wait();
        // Write runtime into vector
        result[i] = timer.elapsed();

        // Check for correctness
        HPX_TEST_EQ(i * num_localities, static_cast<std::size_t>(recv_data[0]));
    }

    if (this_locality == 0)
    {
        write_to_file(operation, "multi_use", -1, num_localities, lpn,
            test_size, iterations, result);
    }
}

void test_multiple_use_with_generation_barrier(int lpn, std::size_t iterations,
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
    std::vector<double> result(iterations, 0.0);

    for (std::size_t i = 0; i != iterations; ++i)
    {
        hpx::chrono::high_resolution_timer const timer;
        hpx::collectives::barrier(
            barrier_client, this_site_arg(), generation_arg(i + 1))
            .get();
        sync.wait();
        result[i] = timer.elapsed();
    }

    if (this_locality == 0)
    {
        write_to_file(operation, "multi_use", -1, num_localities, lpn,
            test_size, iterations, result);
    }
}

void test_all_gather_hierarchical(int arity, int lpn, std::size_t iterations,
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
    // Barrier for synchronization
    char const* const barrier_test_name = "/test/barrier/hierarchical";
    hpx::distributed::barrier barrier(barrier_test_name);
    // Result vector
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<int> send_data;
    std::vector<std::vector<int>> recv_data;
    for (std::size_t i = 0; i != iterations; ++i)
    {
        send_data =
            std::vector<int>(test_size, static_cast<int>(i + this_locality));
        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        // NOLINTNEXTLINE(bugprone-use-after-move)
        hpx::future<std::vector<std::vector<int>>> ft_data =
            all_gather(communicators, std::move(send_data),
                this_site_arg(this_locality), generation_arg(i + 1));
        recv_data = ft_data.get();
        // Synchronize
        barrier.wait();
        // Write runtime into vector
        result[i] = timer.elapsed();
        // Check for correctness: every site contributed (i + site), so
        // recv_data[j][0] must equal (i + j) at every site.
        for (int j = 0; j < static_cast<int>(num_localities); ++j)
        {
            HPX_TEST_EQ(i + j, static_cast<std::size_t>(recv_data[j][0]));
        }
    }
    if (this_locality == 0)
    {
        std::string const mod_name = fallback_threshold < 0 ?
            std::string("hierarchical") :
            "hierarchical_t" + std::to_string(fallback_threshold);
        write_to_file(operation, mod_name, arity, num_localities, lpn,
            test_size, iterations, result);
    }
}

void test_all_to_all_hierarchical(int arity, int lpn, std::size_t iterations,
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
    // Barrier for synchronization
    char const* const barrier_test_name = "/test/barrier/hierarchical";
    hpx::distributed::barrier barrier(barrier_test_name);
    // Result vector
    std::vector<double> result(iterations, 0.0);
    // The all_to_all payload contract requires exactly num_sites elements
    // per site, so payload scaling happens through the per-destination
    // block size.
    std::size_t const block_size = std::max<std::size_t>(
        1, static_cast<std::size_t>(test_size) / num_localities);
    // Data
    std::vector<std::vector<std::uint32_t>> send_data;
    std::vector<std::vector<std::uint32_t>> recv_data;
    for (std::size_t i = 0; i != iterations; ++i)
    {
        // Each destination block carries the source id
        send_data.assign(num_localities,
            std::vector<std::uint32_t>(
                block_size, static_cast<std::uint32_t>(this_locality)));
        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        hpx::future<std::vector<std::vector<std::uint32_t>>> ft_data =
            // NOLINTNEXTLINE(bugprone-use-after-move)
            all_to_all(communicators, std::move(send_data),
                this_site_arg(this_locality), generation_arg(i + 1));
        recv_data = ft_data.get();
        // Synchronize
        barrier.wait();
        // Write runtime into vector
        result[i] = timer.elapsed();
        // Check for correctness on the first iteration only: result block s
        // originated at site s. Later iterations are timed unchecked.
        if (i == 0)
        {
            HPX_TEST_EQ(recv_data.size(), num_localities);
            for (std::size_t s = 0; s != num_localities; ++s)
            {
                HPX_TEST_EQ(recv_data[s].size(), block_size);
                HPX_TEST_EQ(static_cast<std::size_t>(recv_data[s][0]), s);
            }
        }
    }
    if (this_locality == 0)
    {
        std::string const mod_name = fallback_threshold < 0 ?
            std::string("hierarchical") :
            "hierarchical_t" + std::to_string(fallback_threshold);
        write_to_file(operation, mod_name, arity, num_localities, lpn,
            test_size, iterations, result);
    }
}
////////////////////////////////////////////////////////////////////////////////////////
void test_one_shot_use_all_gather(int lpn, std::size_t iterations,
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
    // Result vector
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<int> send_data;
    std::vector<std::vector<int>> recv_data;
    for (std::size_t i = 0; i != iterations; ++i)
    {
        send_data =
            std::vector<int>(test_size, static_cast<int>(i + this_locality));
        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        hpx::future<std::vector<std::vector<int>>> ft_data =
            // NOLINTNEXTLINE(bugprone-use-after-move)
            all_gather(all_gather_direct_basename, std::move(send_data),
                num_sites_arg(num_localities), this_site_arg(this_locality),
                generation_arg(i + 1));
        recv_data = ft_data.get();
        // Synchronize
        barrier.wait();
        // Write runtime into vector
        result[i] = timer.elapsed();
        // Check for correctness
        for (int j = 0; j < static_cast<int>(num_localities); ++j)
        {
            HPX_TEST_EQ(i + j, static_cast<std::size_t>(recv_data[j][0]));
        }
    }
    if (this_locality == 0)
    {
        write_to_file(operation, "single_use", -1, num_localities, lpn,
            test_size, iterations, result);
    }
}
////////////////////////////////////////////////////////////////////////////////////////
void test_multiple_use_with_generation_all_gather(int lpn,
    std::size_t iterations, int test_size, std::string const& operation)
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
    // Result vector
    std::vector<double> result(iterations, 0.0);
    // Data
    std::vector<int> send_data;
    std::vector<std::vector<int>> recv_data;
    for (std::size_t i = 0; i != iterations; ++i)
    {
        send_data =
            std::vector<int>(test_size, static_cast<int>(i + this_locality));
        // Time collective
        hpx::chrono::high_resolution_timer const timer;
        hpx::future<std::vector<std::vector<int>>> ft_data =
            // NOLINTNEXTLINE(bugprone-use-after-move)
            all_gather(all_gather_direct_client, std::move(send_data),
                generation_arg(i + 1));
        recv_data = ft_data.get();
        // Synchronize
        barrier.wait();
        // Write runtime into vector
        result[i] = timer.elapsed();
        // Check for correctness
        for (int j = 0; j < static_cast<int>(num_localities); ++j)
        {
            HPX_TEST_EQ(i + j, static_cast<std::size_t>(recv_data[j][0]));
        }
    }
    if (this_locality == 0)
    {
        write_to_file(operation, "multi_use", -1, num_localities, lpn,
            test_size, iterations, result);
    }
}
int hpx_main(hpx::program_options::variables_map& vm)
{
    int const arity = vm["arity"].as<int>();
    int const lpn = vm["lpn"].as<int>();
    int const test_size = vm["test_size"].as<int>();
    std::string const operation = vm["operation"].as<std::string>();
    int const iterations = vm["iterations"].as<int>();
    int const fallback_threshold = vm["fallback_threshold"].as<int>();

    if (hpx::get_num_localities(hpx::launch::sync) > 1)
    {
        if (operation == "scatter")
        {
            if (arity == -1)
            {
                test_one_shot_use_scatter(
                    lpn, iterations, test_size, operation);
                test_multiple_use_with_generation_scatter(
                    lpn, iterations, test_size, operation);
            }
            else
            {
                test_scatter_hierarchical(arity, lpn, iterations, test_size,
                    operation, fallback_threshold);
            }
        }
        else if (operation == "reduce")
        {
            if (arity == -1)
            {
                test_one_shot_use_reduce(lpn, iterations, test_size, operation);
                test_multiple_use_with_generation_reduce(
                    lpn, iterations, test_size, operation);
            }
            else
            {
                test_reduce_hierarchical(arity, lpn, iterations, test_size,
                    operation, fallback_threshold);
            }
        }
        else if (operation == "broadcast")
        {
            if (arity == -1)
            {
                test_one_shot_use_broadcast(
                    lpn, iterations, test_size, operation);
                test_multiple_use_with_generation_broadcast(
                    lpn, iterations, test_size, operation);
            }
            else
            {
                test_broadcast_hierarchical(arity, lpn, iterations, test_size,
                    operation, fallback_threshold);
            }
        }
        else if (operation == "gather")
        {
            if (arity == -1)
            {
                test_one_shot_use_gather(lpn, iterations, test_size, operation);
                test_multiple_use_with_generation_gather(
                    lpn, iterations, test_size, operation);
            }
            else
            {
                test_gather_hierarchical(arity, lpn, iterations, test_size,
                    operation, fallback_threshold);
            }
        }
        else if (operation == "all_reduce")
        {
            if (arity == -1)
            {
                test_one_shot_use_all_reduce(
                    lpn, iterations, test_size, operation);
                test_multiple_use_with_generation_all_reduce(
                    lpn, iterations, test_size, operation);
            }
            else
            {
                test_all_reduce_hierarchical(arity, lpn, iterations, test_size,
                    operation, fallback_threshold);
            }
        }
        else if (operation == "all_gather")
        {
            if (arity == -1)
            {
                test_one_shot_use_all_gather(
                    lpn, iterations, test_size, operation);
                test_multiple_use_with_generation_all_gather(
                    lpn, iterations, test_size, operation);
            }
            else
            {
                test_all_gather_hierarchical(arity, lpn, iterations, test_size,
                    operation, fallback_threshold);
            }
        }
        else if (operation == "all_to_all")
        {
            if (arity != -1)
            {
                test_all_to_all_hierarchical(arity, lpn, iterations, test_size,
                    operation, fallback_threshold);
            }
        }
        else if (operation == "barrier")
        {
            if (arity == -1)
            {
                test_multiple_use_with_generation_barrier(
                    lpn, iterations, test_size, operation);
            }
            else
            {
                test_barrier_hierarchical(arity, lpn, iterations, test_size,
                    operation, fallback_threshold);
            }
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
            "all_reduce, all_gather, all_to_all, barrier)")
        ("fallback_threshold", value<int>()->default_value(-1),
            "Flat fallback threshold for hierarchical mode. -1 uses library "
            "default (16). Set to 0 to force tree construction. Only meaningful "
            "with hierarchical mode.");
    // clang-format on

    std::vector<std::string> const cfg = {"hpx.run_hpx_main!=1"};

    hpx::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ(hpx::init(argc, argv, init_args), 0);
    return hpx::util::report_errors();
}

#endif
