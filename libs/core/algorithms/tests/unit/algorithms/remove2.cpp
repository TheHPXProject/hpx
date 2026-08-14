//  Copyright (c) 2017 Taeguk Kwon
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "remove_tests.hpp"
#include "test_utils.hpp"

////////////////////////////////////////////////////////////////////////////
void remove_test()
{
    std::cout << "--- remove_test ---" << std::endl;
    test_remove<std::forward_iterator_tag>();
}

void remove_exception_test()
{
    std::cout << "--- remove_exception_test ---" << std::endl;
    test_remove_exception<std::forward_iterator_tag>();
}

void remove_bad_alloc_test()
{
    std::cout << "--- remove_bad_alloc_test ---" << std::endl;
    test_remove_bad_alloc<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    remove_tests_seed_rng(seed);
    std::srand(seed);

    remove_test();
    remove_exception_test();
    remove_bad_alloc_test();

    std::cout << "Test Finish!" << std::endl;

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
