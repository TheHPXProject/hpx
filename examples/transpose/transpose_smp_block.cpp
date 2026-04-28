//  Copyright (c) 2014 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/numeric.hpp>

#include <algorithm>
#include <cstdint>
#include <exception>
#include <iostream>
#include <vector>

#define COL_SHIFT 1000.00    // Constant to shift column index
#define ROW_SHIFT 0.001      // Constant to shift row index

bool verbose = false;

typedef std::vector<double> block;
typedef double* sub_block;

void transpose(sub_block A, sub_block B, std::uint64_t block_order,
    std::uint64_t tile_size);
double test_results(std::uint64_t order, std::uint64_t block_order,
    std::vector<block> const& trans);

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    std::uint64_t order = vm["matrix_size"].as<std::uint64_t>();
    std::uint64_t iterations = vm["iterations"].as<std::uint64_t>();
    std::uint64_t num_blocks = vm["num_blocks"].as<std::uint64_t>();
    std::uint64_t tile_size = order;

    if (vm.count("tile_size"))
        tile_size = vm["tile_size"].as<std::uint64_t>();

    verbose = vm.count("verbose") ? true : false;

    std::uint64_t bytes =
        static_cast<std::uint64_t>(2 * sizeof(double) * order * order);

    std::uint64_t block_order = order / num_blocks;
    std::uint64_t col_block_size = order * block_order;

    std::vector<block> A(num_blocks, block(col_block_size));
    std::vector<block> B(num_blocks, block(col_block_size));

    std::cout << "Serial Matrix transpose: B = A^T\n"
              << "Matrix order          = " << order << "\n";
    if (tile_size < order)
        std::cout << "Tile size             = " << tile_size << "\n";
    else
        std::cout << "Untiled\n";
    std::cout << "Number of iterations  = " << iterations << "\n";

    using hpx::execution::par;
    using hpx::execution::task;
    using hpx::ranges::for_each;

    std::uint64_t const start = 0;

    // Fill the original matrix, set transpose to known garbage value.
    auto range = hpx::util::counting_shape(start, num_blocks);
    for_each(par, range, [&](std::uint64_t b) {
        for (std::uint64_t i = 0; i < order; ++i)
        {
            for (std::uint64_t j = 0; j < block_order; ++j)
            {
                double col_val =
                    COL_SHIFT * static_cast<double>(b * block_order + j);

                A[b][i * block_order + j] =
                    col_val + ROW_SHIFT * static_cast<double>(i);
                B[b][i * block_order + j] = -1.0;
            }
        }
    });

    double errsq = 0.0;
    double avgtime = 0.0;
    double maxtime = 0.0;
    double mintime =
        366.0 * 24.0 * 3600.0;    // set the minimum time to a large value;
                                  // one leap year should be enough
    for (std::uint64_t iter = 0; iter < iterations; ++iter)
    {
        hpx::chrono::high_resolution_timer t;

        auto range = hpx::util::counting_shape(start, num_blocks);

        std::vector<hpx::shared_future<void>> transpose_futures;
        transpose_futures.resize(num_blocks);

        for (std::uint64_t b = 0; b < num_blocks; ++b)
        {
            transpose_futures[b] =
                for_each(par(task), range, [&, b](std::uint64_t phase) {
                    std::uint64_t const block_size = block_order * block_order;
                    std::uint64_t const from_block = phase;
                    std::uint64_t const from_phase = b;
                    std::uint64_t const A_offset = from_phase * block_size;
                    std::uint64_t const B_offset = phase * block_size;

                    transpose(&A[from_block][A_offset], &B[b][B_offset],
                        block_order, tile_size);
                }).share();
        }

        hpx::wait_all(transpose_futures);

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
                (std::max) (iterations - 1, static_cast<std::uint64_t>(1)));
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
        ("matrix_size", value<std::uint64_t>()->default_value(1024),
         "Matrix Size")
        ("iterations", value<std::uint64_t>()->default_value(10),
         "# iterations")
        ("tile_size", value<std::uint64_t>(),
         "Number of tiles to divide the individual matrix blocks for improved "
         "cache and TLB performance")
        ("num_blocks", value<std::uint64_t>()->default_value(256),
         "Number of blocks to divide the individual matrix blocks for improved "
         "cache and TLB performance")
        ( "verbose", "Verbose output")
    ;
    // clang-format on

    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;

    return hpx::local::init(hpx_main, argc, argv, init_args);
}

void transpose(sub_block A, sub_block B, std::uint64_t block_order,
    std::uint64_t tile_size)
{
    if (tile_size < block_order)
    {
        for (std::uint64_t i = 0; i < block_order; i += tile_size)
        {
            for (std::uint64_t j = 0; j < block_order; j += tile_size)
            {
                std::uint64_t i_max = (std::min) (block_order, i + tile_size);
                std::uint64_t j_max = (std::min) (block_order, j + tile_size);

                for (std::uint64_t it = i; it < i_max; ++it)
                {
                    for (std::uint64_t jt = j; jt < j_max; ++jt)
                    {
                        B[it + block_order * jt] = A[jt + block_order * it];
                    }
                }
            }
        }
    }
    else
    {
        for (std::uint64_t i = 0; i < block_order; ++i)
        {
            for (std::uint64_t j = 0; j < block_order; ++j)
            {
                B[i + block_order * j] = A[j + block_order * i];
            }
        }
    }
}

double test_results(std::uint64_t order, std::uint64_t block_order,
    std::vector<block> const& trans)
{
    using hpx::transform_reduce;
    using hpx::execution::par;

    std::uint64_t const start = 0;
    std::uint64_t const end = trans.size();

    // Fill the original matrix, set transpose to known garbage value.
    auto range = hpx::util::counting_shape(start, end);
    double errsq = transform_reduce(
        par, std::begin(range), std::end(range), 0.0,
        [](double lhs, double rhs) { return lhs + rhs; },
        [&](std::uint64_t b) -> double {
            double errsq = 0.0;
            for (std::uint64_t i = 0; i < order; ++i)
            {
                double col_val = COL_SHIFT * static_cast<double>(i);
                for (std::uint64_t j = 0; j < block_order; ++j)
                {
                    double diff = trans[b][i * block_order + j] -
                        (col_val +
                            ROW_SHIFT *
                                static_cast<double>(b * block_order + j));
                    errsq += diff * diff;
                }
            }
            return errsq;
        });

    if (verbose)
        std::cout << " Squared sum of differences: " << errsq << "\n";

    return errsq;
}
