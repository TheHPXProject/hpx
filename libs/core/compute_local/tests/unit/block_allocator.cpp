//  Copyright (c) 2016-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx_init.hpp>
#include <hpx/modules/compute_local.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstddef>
#include <iostream>
#include <limits>
#include <new>
#include <random>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
template <typename T>
T* test_block_allocation(
    hpx::compute::host::block_allocator<T>& alloc, std::size_t count)
{
    return alloc.allocate(count);
}

template <typename T>
void test_block_construction(
    hpx::compute::host::block_allocator<T>& alloc, T* p, std::size_t count)
{
    alloc.bulk_construct(p, count);
}

template <typename T>
void test_block_destruction(
    hpx::compute::host::block_allocator<T>& alloc, T* p, std::size_t count)
{
    return alloc.bulk_destroy(p, count);
}

template <typename T>
void test_block_deallocation(
    hpx::compute::host::block_allocator<T>& alloc, T* p, std::size_t count)
{
    return alloc.deallocate(p, count);
}

///////////////////////////////////////////////////////////////////////////////
template <typename T>
void test_bulk_allocator(std::size_t count)
{
    hpx::compute::host::block_allocator<T> alloc;
    T* p = test_block_allocation(alloc, count);
    test_block_construction(alloc, p, count);
    test_block_destruction(alloc, p, count);
    test_block_deallocation(alloc, p, count);
}

///////////////////////////////////////////////////////////////////////////////
std::atomic<std::size_t> construction_count(0);
std::atomic<std::size_t> destruction_count(0);

struct test
{
    test()
    {
        ++construction_count;
    }
    ~test()
    {
        ++destruction_count;
    }
};

struct alignas(64) over_aligned_test
{
    int a;
    double b;
    char pad[48];
};

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::random_device{}();
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::mt19937 gen(seed);
    std::uniform_int_distribution<> dis(1, 512);

    {
        std::size_t count = dis(gen);
        test_bulk_allocator<int>(count);
    }

    {
        std::size_t count = dis(gen);
        test_bulk_allocator<test>(count);
        HPX_TEST_EQ(construction_count.load(), count);
        HPX_TEST_EQ(destruction_count.load(), count);
    }

    test_bulk_allocator<int>(0);

    {
        hpx::compute::host::block_allocator<int> alloc;
        constexpr std::size_t expected_max =
            (std::numeric_limits<std::size_t>::max)() / sizeof(int);
        HPX_TEST_EQ(alloc.max_size(), expected_max);

        bool caught = false;
        try
        {
            alloc.allocate(alloc.max_size() + 1);
        }
        catch (std::bad_array_new_length const&)
        {
            caught = true;
        }
        HPX_TEST(caught);
    }

    {
        std::size_t count = dis(gen);
        test_bulk_allocator<over_aligned_test>(count);

        hpx::compute::host::block_allocator<over_aligned_test> alloc;
        constexpr std::size_t expected_max =
            (std::numeric_limits<std::size_t>::max)() /
            sizeof(over_aligned_test);
        HPX_TEST_EQ(alloc.max_size(), expected_max);

        bool caught = false;
        try
        {
            alloc.allocate(alloc.max_size() + 1);
        }
        catch (std::bad_array_new_length const&)
        {
            caught = true;
        }
        HPX_TEST(caught);
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    // add command line option which controls the random number generator seed
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("seed,s", value<unsigned int>(),
        "the random number generator seed to use for this run");

    // By default this test should run on all available cores
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    // Initialize and run HPX
    hpx::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::init(argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
