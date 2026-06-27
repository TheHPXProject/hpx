//  Copyright (c) 2021 Srinivas Yadav
//  copyright (c) 2014 Grant Mercer
//  Copyright (c) 2024 Aneek Barman
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include "findfirstof_tests.hpp"

template <typename IteratorTag>
void test_find_first_of()
{
    using namespace hpx::execution;

    test_find_first_of(IteratorTag());

    test_find_first_of(seq, IteratorTag());
    test_find_first_of(par, IteratorTag());
    test_find_first_of(par_unseq, IteratorTag());

    test_find_first_of_async(seq(task), IteratorTag());
    test_find_first_of_async(par(task), IteratorTag());
}

void find_first_of_test()
{
    test_find_first_of<std::random_access_iterator_tag>();
    test_find_first_of<std::forward_iterator_tag>();
}

template <typename IteratorTag>
void test_find_first_of_exception()
{
    using namespace hpx::execution;

    test_find_first_of_exception(IteratorTag());

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_find_first_of_exception(seq, IteratorTag());
    test_find_first_of_exception(par, IteratorTag());

    test_find_first_of_exception_async(seq(task), IteratorTag());
    test_find_first_of_exception_async(par(task), IteratorTag());
}

void find_first_of_exception_test()
{
    test_find_first_of_exception<std::random_access_iterator_tag>();
    test_find_first_of_exception<std::forward_iterator_tag>();
}

template <typename IteratorTag>
void test_find_first_of_bad_alloc()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_find_first_of_bad_alloc(seq, IteratorTag());
    test_find_first_of_bad_alloc(par, IteratorTag());

    test_find_first_of_bad_alloc_async(seq(task), IteratorTag());
    test_find_first_of_bad_alloc_async(par(task), IteratorTag());
}

void find_first_of_bad_alloc_test()
{
    test_find_first_of_bad_alloc<std::random_access_iterator_tag>();
    test_find_first_of_bad_alloc<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_find_first_of_edge_cases()
{
    using namespace hpx::execution;

    // 1. Invocation count and simple boundary checks (random access only)
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_find_first_of_edge_cases(seq);
        test_find_first_of_edge_cases(par);
        test_find_first_of_edge_cases(par_unseq);
    }

    // 2. Empty needle: exercised individually per policy
    test_find_first_of_empty_needle(seq, IteratorTag());
    test_find_first_of_empty_needle(par, IteratorTag());
    test_find_first_of_empty_needle(par_unseq, IteratorTag());

    // 3. Cross-policy consistency
    test_find_first_of_cross_policy(IteratorTag());
}

void find_first_of_edge_cases_test()
{
    test_find_first_of_edge_cases<std::random_access_iterator_tag>();
    test_find_first_of_edge_cases<std::forward_iterator_tag>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    find_first_of_test();
    find_first_of_exception_test();
    find_first_of_bad_alloc_test();
    find_first_of_edge_cases_test();

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
