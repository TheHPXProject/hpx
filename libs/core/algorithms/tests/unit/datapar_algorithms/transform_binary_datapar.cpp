//  Copyright (c) 2014-2016 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/datapar.hpp>
#include <hpx/init.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "../algorithms/transform_binary_tests.hpp"

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_transform_binary()
{
    using namespace hpx::execution;

    test_transform_binary(simd, IteratorTag());
    test_transform_binary(par_simd, IteratorTag());

    test_transform_binary_async(simd(task), IteratorTag());
    test_transform_binary_async(par_simd(task), IteratorTag());

    test_transform_binary(fixed_size_simd<1>, IteratorTag());
    test_transform_binary(par_fixed_size_simd<1>, IteratorTag());

    test_transform_binary_async(fixed_size_simd<2>(task), IteratorTag());
    test_transform_binary_async(par_fixed_size_simd<2>(task), IteratorTag());

    test_transform_binary(fixed_size_simd<1>, IteratorTag());
    test_transform_binary(par_fixed_size_simd<1>, IteratorTag());

    test_transform_binary_async(fixed_size_simd<2>(task), IteratorTag());
    test_transform_binary_async(par_fixed_size_simd<2>(task), IteratorTag());
}

void transform_binary_test()
{
    test_transform_binary<std::random_access_iterator_tag>();
    test_transform_binary<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = static_cast<unsigned int>(std::time(nullptr));
    if (vm.contains("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    transform_binary_test();
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
