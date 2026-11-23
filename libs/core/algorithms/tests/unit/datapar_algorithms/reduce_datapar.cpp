//  Copyright (c) 2022 Srinivas Yadav
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/datapar.hpp>
#include <hpx/init.hpp>

#include <cstddef>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "../algorithms/reduce_tests.hpp"

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_reduce1()
{
    using namespace hpx::execution;

    test_reduce1(simd, IteratorTag());
    test_reduce1(par_simd, IteratorTag());

    test_reduce1_async(simd(task), IteratorTag());
    test_reduce1_async(par_simd(task), IteratorTag());
}

void reduce_test1()
{
    test_reduce1<std::random_access_iterator_tag>();
    test_reduce1<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_reduce2()
{
    using namespace hpx::execution;

    test_reduce2(simd, IteratorTag());
    test_reduce2(par_simd, IteratorTag());

    test_reduce2_async(simd(task), IteratorTag());
    test_reduce2_async(par_simd(task), IteratorTag());
}

void reduce_test2()
{
    test_reduce2<std::random_access_iterator_tag>();
    test_reduce2<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_reduce3()
{
    using namespace hpx::execution;

    test_reduce3(simd, IteratorTag());
    test_reduce3(par_simd, IteratorTag());

    test_reduce3_async(simd(task), IteratorTag());
    test_reduce3_async(par_simd(task), IteratorTag());
}

void reduce_test3()
{
    test_reduce3<std::random_access_iterator_tag>();
    test_reduce3<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = std::random_device{}();
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    reduce_test1();
    reduce_test2();
    reduce_test3();
    return hpx::local::finalize();
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
    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
