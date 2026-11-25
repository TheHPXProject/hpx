//  Copyright (c) 2015 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>

#include "foreach_tests.hpp"

///////////////////////////////////////////////////////////////////////////////
void test_persistent_executitor_parameters()
{
    typedef std::random_access_iterator_tag iterator_tag;
    {
        hpx::execution::experimental::persistent_auto_chunk_size p;
        auto policy = hpx::execution::par.with(p);
        test_for_each(policy, iterator_tag());
    }

    {
        hpx::execution::experimental::persistent_auto_chunk_size p;
        auto policy = hpx::execution::par(hpx::execution::task).with(p);
        test_for_each_async(policy, iterator_tag());
    }

    hpx::execution::parallel_executor par_exec;

    {
        hpx::execution::experimental::persistent_auto_chunk_size p;
        auto policy = hpx::execution::par.on(par_exec).with(p);
        test_for_each(policy, iterator_tag());
    }

    {
        hpx::execution::experimental::persistent_auto_chunk_size p;
        auto policy =
            hpx::execution::par(hpx::execution::task).on(par_exec).with(p);
        test_for_each_async(policy, iterator_tag());
    }
}

void test_persistent_executitor_parameters_ref()
{
    using namespace hpx::parallel;

    typedef std::random_access_iterator_tag iterator_tag;

    {
        hpx::execution::experimental::persistent_auto_chunk_size p;
        test_for_each(hpx::execution::par.with(std::ref(p)), iterator_tag());
    }

    {
        hpx::execution::experimental::persistent_auto_chunk_size p;
        test_for_each_async(
            hpx::execution::par(hpx::execution::task).with(std::ref(p)),
            iterator_tag());
    }

    hpx::execution::parallel_executor par_exec;

    {
        hpx::execution::experimental::persistent_auto_chunk_size p;
        test_for_each(
            hpx::execution::par.on(par_exec).with(std::ref(p)), iterator_tag());
    }

    {
        hpx::execution::experimental::persistent_auto_chunk_size p;
        test_for_each_async(hpx::execution::par(hpx::execution::task)
                                .on(par_exec)
                                .with(std::ref(p)),
            iterator_tag());
    }
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    test_persistent_executitor_parameters();
    test_persistent_executitor_parameters_ref();

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
