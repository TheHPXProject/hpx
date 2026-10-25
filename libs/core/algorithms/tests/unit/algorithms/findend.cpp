//  Copyright (c) 2021 Srinivas Yadav
//  copyright (c) 2014 Grant Mercer
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "findend_tests.hpp"

template <typename IteratorTag>
void test_find_end1()
{
    using namespace hpx::execution;

    test_find_end1(IteratorTag());

    test_find_end1(seq, IteratorTag());
    test_find_end1(par, IteratorTag());
    test_find_end1(par_unseq, IteratorTag());

    test_find_end1_async(seq(task), IteratorTag());
    test_find_end1_async(par(task), IteratorTag());
}

void find_end_test1()
{
    test_find_end1<std::random_access_iterator_tag>();
    test_find_end1<std::forward_iterator_tag>();
}

template <typename IteratorTag>
void test_find_end2()
{
    using namespace hpx::execution;

    test_find_end2(IteratorTag());

    test_find_end2(seq, IteratorTag());
    test_find_end2(par, IteratorTag());
    test_find_end2(par_unseq, IteratorTag());

    test_find_end2_async(seq(task), IteratorTag());
    test_find_end2_async(par(task), IteratorTag());
}

void find_end_test2()
{
    test_find_end2<std::random_access_iterator_tag>();
    test_find_end2<std::forward_iterator_tag>();
}

template <typename IteratorTag>
void test_find_end3()
{
    using namespace hpx::execution;

    test_find_end3(IteratorTag());

    test_find_end3(seq, IteratorTag());
    test_find_end3(par, IteratorTag());
    test_find_end3(par_unseq, IteratorTag());

    test_find_end3_async(seq(task), IteratorTag());
    test_find_end3_async(par(task), IteratorTag());
}

void find_end_test3()
{
    test_find_end3<std::random_access_iterator_tag>();
    test_find_end3<std::forward_iterator_tag>();
}

template <typename IteratorTag>
void test_find_end4()
{
    using namespace hpx::execution;

    test_find_end4(IteratorTag());

    test_find_end4(seq, IteratorTag());
    test_find_end4(par, IteratorTag());
    test_find_end4(par_unseq, IteratorTag());

    test_find_end4_async(seq(task), IteratorTag());
    test_find_end4_async(par(task), IteratorTag());
}

void find_end_test4()
{
    test_find_end4<std::random_access_iterator_tag>();
    test_find_end4<std::forward_iterator_tag>();
}

template <typename IteratorTag>
void test_find_end5()
{
    using namespace hpx::execution;

    test_find_end5(seq, IteratorTag());
    test_find_end5(par, IteratorTag());
    test_find_end5(par_unseq, IteratorTag());
}

void find_end_test5()
{
    test_find_end5<std::random_access_iterator_tag>();
    test_find_end5<std::forward_iterator_tag>();
}

template <typename IteratorTag>
void test_find_end_exception()
{
    using namespace hpx::execution;

    test_find_end_exception(IteratorTag());

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_find_end_exception(seq, IteratorTag());
    test_find_end_exception(par, IteratorTag());

    test_find_end_exception_async(seq(task), IteratorTag());
    test_find_end_exception_async(par(task), IteratorTag());
}

void find_end_exception_test()
{
    test_find_end_exception<std::random_access_iterator_tag>();
    test_find_end_exception<std::forward_iterator_tag>();
}

template <typename IteratorTag>
void test_find_end_bad_alloc()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_find_end_bad_alloc(seq, IteratorTag());
    test_find_end_bad_alloc(par, IteratorTag());

    test_find_end_bad_alloc_async(seq(task), IteratorTag());
    test_find_end_bad_alloc_async(par(task), IteratorTag());
}

void find_end_bad_alloc_test()
{
    test_find_end_bad_alloc<std::random_access_iterator_tag>();
    test_find_end_bad_alloc<std::forward_iterator_tag>();
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    find_end_test1();
    find_end_test2();
    find_end_test3();
    find_end_test4();
    find_end_test5();
    find_end_exception_test();
    find_end_bad_alloc_test();
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
