//  Copyright (c) 2014 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/chrono.hpp>
#include <hpx/init.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

#define COL_SHIFT 1000.00    // Constant to shift column index
#define ROW_SHIFT 0.001      // Constant to shift row index

bool verbose = false;

typedef std::vector<double> block;
typedef double* sub_block;

void transpose(
    sub_block A, sub_block B, std::size_t block_order, std::size_t tile_size);
double test_results(std::size_t order, std::size_t block_order,
    std::vector<block> const& trans);

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    std::size_t order = vm["matrix_size"].as<std::size_t>();
    std::size_t iterations = vm["iterations"].as<std::size_t>();
    std::size_t num_blocks = vm["num_blocks"].as<std::size_t>();
    std::size_t tile_size = order;

    if (vm.count("tile_size"))
        tile_size = vm["tile_size"].as<std::size_t>();

    verbose = vm.count("verbose") ? true : false;

    // keep the byte count in 64 bits, order is only as wide as std::size_t
    std::uint64_t bytes =
        2 * sizeof(double) * static_cast<std::uint64_t>(order) * order;

    std::size_t block_order = order / num_blocks;
    std::size_t col_block_size = order * block_order;

    std::vector<block> A(num_blocks, block(col_block_size));
    std::vector<block> B(num_blocks, block(col_block_size));

    std::cout << "Serial Matrix transpose: B = A^T\n"
              << "Matrix order          = " << order << "\n";
    if (tile_size < order)
        std::cout << "Tile size             = " << tile_size << "\n";
    else
        std::cout << "Untiled\n";
    std::cout << "Number of iterations  = " << iterations << "\n";

    // Fill the original matrix, set transpose to known garbage value.
    for (std::size_t b = 0; b < num_blocks; ++b)
    {
        for (std::size_t i = 0; i < order; ++i)
        {
            for (std::size_t j = 0; j < block_order; ++j)
            {
                double col_val =
                    COL_SHIFT * static_cast<double>(b * block_order + j);

                A[b][i * block_order + j] =
                    col_val + ROW_SHIFT * static_cast<double>(i);
                B[b][i * block_order + j] = -1.0;
            }
        }
    }

    double errsq = 0.0;
    double avgtime = 0.0;
    double maxtime = 0.0;
    double mintime =
        366.0 * 24.0 * 3600.0;    // set the minimum time to a large value;
                                  // one leap year should be enough
    for (std::size_t iter = 0; iter < iterations; ++iter)
    {
        hpx::chrono::high_resolution_timer t;

        for (std::size_t b = 0; b < num_blocks; ++b)
        {
            for (std::size_t phase = 0; phase < num_blocks; ++phase)
            {
                std::size_t const block_size = block_order * block_order;
                std::size_t const from_block = phase;
                std::size_t const from_phase = b;
                std::size_t const A_offset = from_phase * block_size;
                std::size_t const B_offset = phase * block_size;
                transpose(&A[from_block][A_offset], &B[b][B_offset],
                    block_order, tile_size);
            }
        }

        double elapsed = t.elapsed();

        if (iter > 0 || iterations == 1)    // Skip the first iteration
        {
            avgtime = avgtime + elapsed;
            maxtime = (std::max) (maxtime, elapsed);
            mintime = (std::min) (mintime, elapsed);
        }

        errsq += test_results(order, block_order, B);
    }    // end of iter loop

    // Analyze and output results

    double epsilon = 1.e-8;
    if (errsq < epsilon)
    {
        std::cout << "Solution validates\n";
        avgtime = avgtime /
            static_cast<double>(
                (std::max) (iterations - 1, static_cast<std::size_t>(1)));
        std::cout << "Rate (MB/s): "
                  << 1.e-6 * static_cast<double>(bytes) / mintime << ", "
                  << "Avg time (s): " << avgtime << ", "
                  << "Min time (s): " << mintime << ", "
                  << "Max time (s): " << maxtime << "\n";

        if (verbose)
            std::cout << "Squared errors: " << errsq << "\n";
    }
    else
    {
        std::cout << "ERROR: Aggregate squared error " << errsq
                  << " exceeds threshold " << epsilon << "\n";
        std::terminate();
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;

    options_description desc_commandline;
    // clang-format off
    desc_commandline.add_options()
        ("matrix_size", value<std::size_t>()->default_value(1024),
         "Matrix Size")
        ("iterations", value<std::size_t>()->default_value(10),
         "# iterations")
        ("tile_size", value<std::size_t>(),
        "Number of tiles to divide the individual matrix blocks for improved "
         "cache and TLB performance")
        ("num_blocks", value<std::size_t>()->default_value(256),
        "Number of blocks to divide the individual matrix blocks for improved "
         "cache and TLB performance")
        ("verbose", "Verbose output");
    // clang-format on

    // Initialize and run HPX, this example is serial and therefore only needs
    // one thread. We just use hpx::init to parse our command line arguments
    std::vector<std::string> const cfg = {"hpx.os_threads!=1"};

    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    return hpx::local::init(hpx_main, argc, argv, init_args);
}

void transpose(
    sub_block A, sub_block B, std::size_t block_order, std::size_t tile_size)
{
    if (tile_size < block_order)
    {
        for (std::size_t i = 0; i < block_order; i += tile_size)
        {
            for (std::size_t j = 0; j < block_order; j += tile_size)
            {
                std::size_t i_max = (std::min) (block_order, i + tile_size);
                for (std::size_t it = i; it < i_max; ++it)
                {
                    std::size_t j_max = (std::min) (block_order, j + tile_size);
                    for (std::size_t jt = j; jt < j_max; ++jt)
                    {
                        B[it + block_order * jt] = A[jt + block_order * it];
                    }
                }
            }
        }
    }
    else
    {
        for (std::size_t i = 0; i < block_order; ++i)
        {
            for (std::size_t j = 0; j < block_order; ++j)
            {
                B[i + block_order * j] = A[j + block_order * i];
            }
        }
    }
}

double test_results(
    std::size_t order, std::size_t block_order, std::vector<block> const& trans)
{
    double errsq = 0.0;

    for (std::size_t b = 0; b < trans.size(); ++b)
    {
        for (std::size_t i = 0; i < order; ++i)
        {
            double col_val = COL_SHIFT * static_cast<double>(i);
            for (std::size_t j = 0; j < block_order; ++j)
            {
                double diff = trans[b][i * block_order + j] -
                    (col_val +
                        ROW_SHIFT * static_cast<double>(b * block_order + j));
                errsq += diff * diff;
            }
        }
    }

    if (verbose)
        std::cout << " Squared sum of differences: " << errsq << "\n";

    return errsq;
}
