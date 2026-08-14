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

#include "test_utils.hpp"
#include "unique_copy_tests.hpp"

////////////////////////////////////////////////////////////////////////////
void unique_copy_test()
{
    std::cout << "--- unique_copy_test ---" << std::endl;
    test_unique_copy<std::random_access_iterator_tag>();
    test_unique_copy<std::bidirectional_iterator_tag>();
    test_unique_copy<std::forward_iterator_tag>();
}

void unique_copy_exception_test()
{
    std::cout << "--- unique_copy_exception_test ---" << std::endl;
    test_unique_copy_exception<std::random_access_iterator_tag>();
    test_unique_copy_exception<std::bidirectional_iterator_tag>();
    test_unique_copy_exception<std::forward_iterator_tag>();
}

void unique_copy_bad_alloc_test()
{
    std::cout << "--- unique_copy_bad_alloc_test ---" << std::endl;
    test_unique_copy_bad_alloc<std::random_access_iterator_tag>();
    test_unique_copy_bad_alloc<std::bidirectional_iterator_tag>();
    test_unique_copy_bad_alloc<std::forward_iterator_tag>();
}

void unique_copy_constraint_test()
{
    std::cout << "--- unique_copy_constraint_test ---" << std::endl;
    // Exercises the corrected predicate constraint in hpx::unique_copy:
    // the requires clause must check iter_value_t<InIter> for both arguments,
    // not iter_value_t<OutIter> for the second argument.
    test_unique_copy_constraint<std::random_access_iterator_tag>();
    test_unique_copy_constraint<std::bidirectional_iterator_tag>();
    test_unique_copy_constraint<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    unique_copy_test();
    unique_copy_exception_test();
    unique_copy_bad_alloc_test();
    unique_copy_constraint_test();

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
