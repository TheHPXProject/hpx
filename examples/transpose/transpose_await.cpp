//  Copyright (c) 2014 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if defined(HPX_HAVE_CXX20_COROUTINES)

#include <hpx/hpx.hpp>
#include <hpx/init.hpp>

#include <hpx/algorithm.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/numeric.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#define COL_SHIFT 1000.00    // Constant to shift column index
#define ROW_SHIFT 0.001      // Constant to shift row index

bool verbose = false;

char const* A_block_basename = "/transpose/block/A";
char const* B_block_basename = "/transpose/block/B";

struct sub_block
{
    enum mode
    {
        reference,
        owning
    };

    sub_block()
      : size_(0)
      , data_(0)
      , mode_(reference)
    {
    }

    sub_block(double* data, std::size_t size)
      : size_(size)
      , data_(data)
      , mode_(reference)
    {
    }

    ~sub_block()
    {
        if (data_ && mode_ == owning)
        {
            delete[] data_;
        }
    }

    sub_block(sub_block&& other)
      : size_(other.size_)
      , data_(other.data_)
      , mode_(other.mode_)
    {
        if (mode_ == owning)
        {
            other.data_ = 0;
            other.size_ = 0;
        }
    }

    sub_block& operator=(sub_block&& other)
    {
        size_ = other.size_;
        data_ = other.data_;
        mode_ = other.mode_;
        if (mode_ == owning)
        {
            other.data_ = 0;
            other.size_ = 0;
        }

        return *this;
    }

    double operator[](std::size_t i) const
    {
        HPX_ASSERT(data_);
        return data_[i];
    }

    double& operator[](std::size_t i)
    {
        HPX_ASSERT(data_);
        HPX_ASSERT(mode_ == reference);
        return data_[i];
    }

    void load(hpx::serialization::input_archive& ar, unsigned)
    {
        ar & size_;
        if (size_ > 0)
        {
            data_ = new double[size_];
            hpx::serialization::array<double> arr(data_, size_);
            ar >> arr;
            mode_ = owning;
        }
    }

    void save(hpx::serialization::output_archive& ar, unsigned) const
    {
        ar & size_;
        if (size_ > 0)
        {
            hpx::serialization::array<double> arr(data_, size_);
            ar << arr;
        }
    }

    HPX_SERIALIZATION_SPLIT_MEMBER()

    std::size_t size_;
    double* data_;
    mode mode_;
};

struct block_component : hpx::components::component_base<block_component>
{
    block_component() {}

    block_component(std::size_t size)
      : data_(size)
    {
    }

    sub_block get_sub_block(std::size_t offset, std::size_t size)
    {
        HPX_ASSERT(!data_.empty());
        return sub_block(&data_[offset], size);
    }

    HPX_DEFINE_COMPONENT_ACTION(block_component, get_sub_block)

    std::vector<double> data_;
};

struct block : hpx::components::client_base<block, block_component>
{
    typedef hpx::components::client_base<block, block_component> base_type;
    block() {}

    block(std::size_t id, char const* base_name)
      : base_type(hpx::find_from_basename(base_name, id))
    {
        get_id();
    }

    block(std::size_t id, std::size_t size, char const* base_name)
      : base_type(hpx::new_<block_component>(hpx::find_here(), size))
    {
        hpx::register_with_basename(base_name, get_id(), id);
    }

    hpx::future<sub_block> get_sub_block(
        std::size_t offset, std::size_t size) const
    {
        block_component::get_sub_block_action act;
        return hpx::async(act, get_id(), offset, size);
    }
};

// The macros below are necessary to generate the code required for exposing
// our block_component type remotely.
//
// HPX_REGISTER_COMPONENT() exposes the component creation
// through hpx::new_<>().
typedef hpx::components::component<block_component> block_component_type;
HPX_REGISTER_COMPONENT(block_component_type, block_component)

// HPX_REGISTER_ACTION() exposes the component member function for remote
// invocation.
typedef block_component::get_sub_block_action get_sub_block_action;
HPX_REGISTER_ACTION(get_sub_block_action)

void transpose(sub_block const A, sub_block B, std::size_t block_order,
    std::size_t tile_size);

double test_results(std::size_t order, std::size_t block_order,
    std::vector<block>& trans, std::size_t blocks_start,
    std::size_t blocks_end);

///////////////////////////////////////////////////////////////////////////////
// The returned value type has to be the same as the return type used for
// co_await below
hpx::future<sub_block> transpose_phase(std::vector<block> const& A,
    std::vector<block>& B, std::size_t block_order, std::size_t b,
    std::size_t num_blocks, std::size_t /* num_local_blocks */,
    std::size_t block_size, std::size_t tile_size)
{
    std::size_t const from_phase = b;
    std::size_t const A_offset = from_phase * block_size;

    auto phase_range = hpx::util::counting_shape(num_blocks);
    for (std::size_t phase : phase_range)
    {
        std::size_t const from_block = phase;
        std::size_t const B_offset = phase * block_size;

        hpx::future<sub_block> from =
            A[from_block].get_sub_block(A_offset, block_size);
        hpx::future<sub_block> to = B[b].get_sub_block(B_offset, block_size);

        transpose(co_await from, co_await to, block_order, tile_size);
    }

    co_return sub_block();
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    {
        hpx::id_type here = hpx::find_here();
        bool root = here == hpx::find_root_locality();

        std::size_t num_localities = hpx::get_num_localities().get();

        std::size_t order = vm["matrix_size"].as<std::size_t>();
        std::size_t iterations = vm["iterations"].as<std::size_t>();
        std::size_t num_local_blocks = vm["num_blocks"].as<std::size_t>();
        std::size_t tile_size = order;

        if (vm.count("tile_size"))
            tile_size = vm["tile_size"].as<std::size_t>();

        verbose = vm.count("verbose") ? true : false;

        std::uint64_t bytes = static_cast<std::uint64_t>(2.0 * sizeof(double) *
            static_cast<double>(order) * static_cast<double>(order));

        std::size_t num_blocks = num_localities * num_local_blocks;

        std::size_t block_order = order / num_blocks;
        std::size_t col_block_size = order * block_order;

        std::size_t id = hpx::get_locality_id();

        std::vector<block> A(num_blocks);
        std::vector<block> B(num_blocks);

        std::size_t blocks_start = id * num_local_blocks;
        std::size_t blocks_end = (id + 1) * num_local_blocks;

        // Actually allocate the block components in AGAS
        for (std::size_t b = 0; b != num_blocks; ++b)
        {
            // Allocate block
            if (b >= blocks_start && b != blocks_end)
            {
                A[b] = block(b, col_block_size, A_block_basename);
                B[b] = block(b, col_block_size, B_block_basename);
            }
            // Retrieve the block by it's symbolic name
            else
            {
                A[b] = block(b, A_block_basename);
                B[b] = block(b, B_block_basename);
            }
        }

        if (root)
        {
            std::cout << "Distributed HPX Matrix transpose (await): B = A^T\n"
                      << "Matrix order          = " << order << "\n"
                      << "Matrix local columns  = " << block_order << "\n"
                      << "Number of blocks      = " << num_blocks << "\n"
                      << "Number of localities  = " << num_localities << "\n";
            if (tile_size < order)
                std::cout << "Tile size             = " << tile_size << "\n";
            else
                std::cout << "Untiled\n";
            std::cout << "Number of iterations  = " << iterations << "\n";
        }
        using hpx::execution::par;
        using hpx::ranges::for_each;

        // Fill the original matrix, set transpose to known garbage value.
        auto range = hpx::util::counting_shape(blocks_start, blocks_end);
        for_each(par, range, [&](std::size_t b) {
            std::shared_ptr<block_component> A_ptr =
                hpx::get_ptr<block_component>(A[b].get_id()).get();
            std::shared_ptr<block_component> B_ptr =
                hpx::get_ptr<block_component>(B[b].get_id()).get();

            for (std::size_t i = 0; i != order; ++i)
            {
                for (std::size_t j = 0; j != block_order; ++j)
                {
                    double col_val =
                        COL_SHIFT * static_cast<double>(b * block_order + j);
                    A_ptr->data_[i * block_order + j] = col_val + ROW_SHIFT * i;
                    B_ptr->data_[i * block_order + j] = -1.0;
                }
            }
        });

        double errsq = 0.0;
        double avgtime = 0.0;
        double maxtime = 0.0;
        double mintime =
            366.0 * 24.0 * 3600.0;    // set the minimum time to a large value;
                                      // one leap year should be enough
        for (std::size_t iter = 0; iter < iterations; ++iter)
        {
            hpx::chrono::high_resolution_timer t;

            auto range = hpx::util::counting_shape(blocks_start, blocks_end);

            std::size_t const block_size = block_order * block_order;
            for_each(par, range, [&](std::size_t b) {
                transpose_phase(A, B, block_order, b, num_blocks,
                    num_local_blocks, block_size, tile_size)
                    .get();
            });

            double elapsed = t.elapsed();

            if (iter > 0 || iterations == 1)    // Skip the first iteration
            {
                avgtime = avgtime + elapsed;
                maxtime = (std::max) (maxtime, elapsed);
                mintime = (std::min) (mintime, elapsed);
            }

            if (root)
                errsq += test_results(
                    order, block_order, B, blocks_start, blocks_end);
        }    // end of iter loop

        // Analyze and output results

        double epsilon = 1.e-8;
        if (root)
        {
            if (errsq < epsilon)
            {
                std::cout << "Solution validates\n";
                avgtime = avgtime /
                    static_cast<double>((std::max) (iterations - 1,
                        static_cast<std::size_t>(1)));
                std::cout << "Rate (MB/s): " << 1.e-6 * bytes / mintime << ", "
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
                hpx::terminate();
            }
        }
    }

    return hpx::finalize();
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
        ("num_blocks", value<std::size_t>()->default_value(1),
         "Number of blocks to divide the individual matrix blocks for "
         "improved cache and TLB performance")
        ( "verbose", "Verbose output")
    ;
    // clang-format on

    // Initialize and run HPX, this example requires to run hpx_main on all
    // localities
    std::vector<std::string> const cfg = {"hpx.run_hpx_main!=1"};

    hpx::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    return hpx::init(argc, argv, init_args);
}

void transpose(sub_block const A, sub_block B, std::size_t block_order,
    std::size_t tile_size)
{
    if (tile_size < block_order)
    {
        for (std::size_t i = 0; i != block_order; i += tile_size)
        {
            for (std::size_t j = 0; j != block_order; j += tile_size)
            {
                std::size_t max_i = (std::min) (block_order, i + tile_size);
                std::size_t max_j = (std::min) (block_order, j + tile_size);

                for (std::size_t it = i; it != max_i; ++it)
                {
                    for (std::size_t jt = j; jt != max_j; ++jt)
                    {
                        B[it + block_order * jt] = A[jt + block_order * it];
                    }
                }
            }
        }
    }
    else
    {
        for (std::size_t i = 0; i != block_order; ++i)
        {
            for (std::size_t j = 0; j != block_order; ++j)
            {
                B[i + block_order * j] = A[j + block_order * i];
            }
        }
    }
}

double test_results(std::size_t order, std::size_t block_order,
    std::vector<block>& trans, std::size_t blocks_start, std::size_t blocks_end)
{
    using hpx::transform_reduce;
    using hpx::execution::par;

    // Fill the original matrix, set transpose to known garbage value.
    auto range = hpx::util::counting_shape(blocks_start, blocks_end);
    double errsq = transform_reduce(
        par, std::begin(range), std::end(range), 0.0,
        [](double lhs, double rhs) { return lhs + rhs; },
        [&](std::size_t b) -> double {
            sub_block trans_block =
                trans[b].get_sub_block(0, order * block_order).get();
            double errsq = 0.0;
            for (std::size_t i = 0; i < order; ++i)
            {
                double col_val = COL_SHIFT * i;
                for (std::size_t j = 0; j < block_order; ++j)
                {
                    double diff = trans_block[i * block_order + j] -
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
#else
int main(int, char*[])
{
    return 0;
}
#endif
