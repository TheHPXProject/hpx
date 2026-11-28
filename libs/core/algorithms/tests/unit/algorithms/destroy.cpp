//  Copyright (c) 2014-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "destroy_tests.hpp"

////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_destroy()
{
    test_destroy(IteratorTag());

    test_destroy(hpx::execution::seq, IteratorTag());
    test_destroy(hpx::execution::par, IteratorTag());
    test_destroy(hpx::execution::par_unseq, IteratorTag());

    test_destroy_async(
        hpx::execution::seq(hpx::execution::task), IteratorTag());
    test_destroy_async(
        hpx::execution::par(hpx::execution::task), IteratorTag());
}

void destroy_test()
{
    test_destroy<std::random_access_iterator_tag>();
    test_destroy<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_destroy_exception()
{
    test_destroy_exception(IteratorTag());

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_destroy_exception(hpx::execution::seq, IteratorTag());
    test_destroy_exception(hpx::execution::par, IteratorTag());

    test_destroy_exception_async(
        hpx::execution::seq(hpx::execution::task), IteratorTag());
    test_destroy_exception_async(
        hpx::execution::par(hpx::execution::task), IteratorTag());
}

void destroy_exception_test()
{
    test_destroy_exception<std::random_access_iterator_tag>();
    test_destroy_exception<std::forward_iterator_tag>();
}

//////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_destroy_bad_alloc()
{
    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_destroy_bad_alloc(hpx::execution::seq, IteratorTag());
    test_destroy_bad_alloc(hpx::execution::par, IteratorTag());

    test_destroy_bad_alloc_async(
        hpx::execution::seq(hpx::execution::task), IteratorTag());
    test_destroy_bad_alloc_async(
        hpx::execution::par(hpx::execution::task), IteratorTag());
}

void destroy_bad_alloc_test()
{
    test_destroy_bad_alloc<std::random_access_iterator_tag>();
    test_destroy_bad_alloc<std::forward_iterator_tag>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    destroy_test();
    destroy_exception_test();
    destroy_bad_alloc_test();
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
