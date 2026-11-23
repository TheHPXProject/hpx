//  Copyright (c) 2017 Taeguk Kwon
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "partition_tests.hpp"
#include "test_utils.hpp"

////////////////////////////////////////////////////////////////////////////
void partition_test()
{
    std::cout << "--- partition_test ---" << std::endl;
    test_partition<std::random_access_iterator_tag>();
    test_partition<std::bidirectional_iterator_tag>();
    test_partition<std::forward_iterator_tag>();
}

void partition_exception_test()
{
    std::cout << "--- partition_exception_test ---" << std::endl;
    test_partition_exception<std::random_access_iterator_tag>();
    test_partition_exception<std::bidirectional_iterator_tag>();
    test_partition_exception<std::forward_iterator_tag>();
}

void partition_bad_alloc_test()
{
    std::cout << "--- partition_bad_alloc_test ---" << std::endl;
    test_partition_bad_alloc<std::random_access_iterator_tag>();
    test_partition_bad_alloc<std::bidirectional_iterator_tag>();
    test_partition_bad_alloc<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    _gen.seed(seed);

    partition_test();
    partition_exception_test();
    partition_bad_alloc_test();

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
