//  Copyright (c) 2021 Srinivas Yadav
//  Copyright (c) 2014 Grant Mercer
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "adjacentfind_tests.hpp"

////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_adjacent_find()
{
    using namespace hpx::execution;
    test_adjacent_find(seq, IteratorTag());
    test_adjacent_find(par, IteratorTag());
    test_adjacent_find(par_unseq, IteratorTag());

    test_adjacent_find_async(seq(task), IteratorTag());
    test_adjacent_find_async(par(task), IteratorTag());
}

void adjacent_find_test()
{
    test_adjacent_find<std::random_access_iterator_tag>();
    test_adjacent_find<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_adjacent_find_exception()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_adjacent_find_exception(seq, IteratorTag());
    test_adjacent_find_exception(par, IteratorTag());

    test_adjacent_find_exception_async(seq(task), IteratorTag());
    test_adjacent_find_exception_async(par(task), IteratorTag());
}

void adjacent_find_exception_test()
{
    test_adjacent_find_exception<std::random_access_iterator_tag>();
    test_adjacent_find_exception<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_adjacent_find_bad_alloc()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_adjacent_find_bad_alloc(seq, IteratorTag());
    test_adjacent_find_bad_alloc(par, IteratorTag());

    test_adjacent_find_bad_alloc_async(seq(task), IteratorTag());
    test_adjacent_find_bad_alloc_async(par(task), IteratorTag());
}

void adjacent_find_bad_alloc_test()
{
    test_adjacent_find_bad_alloc<std::random_access_iterator_tag>();
    test_adjacent_find_bad_alloc<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////
void adjacent_find_edge_case_test()
{
    using namespace hpx::execution;
    test_adjacent_find_edge_cases(seq);
    test_adjacent_find_edge_cases(par);
    test_adjacent_find_edge_cases(par_unseq);
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    adjacent_find_test();
    adjacent_find_exception_test();
    adjacent_find_bad_alloc_test();
    adjacent_find_edge_case_test();
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
