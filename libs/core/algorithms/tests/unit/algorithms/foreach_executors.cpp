//  Copyright (c) 2015 Daniel Bourgeois
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "foreach_tests.hpp"

////////////////////////////////////////////////////////////////////////////////
template <typename ExPolicy>
void test_executors(ExPolicy&& policy, bool test_exceptions = true)
{
    typedef std::random_access_iterator_tag iterator_tag;

    if (test_exceptions)
    {
        test_for_each_exception(policy, iterator_tag());
        test_for_each_bad_alloc(policy, iterator_tag());
    }
    test_for_each(std::forward<ExPolicy>(policy), iterator_tag());
}

template <typename ExPolicy>
void test_executors_async(ExPolicy&& p, bool test_exceptions = true)
{
    typedef std::random_access_iterator_tag iterator_tag;

    if (test_exceptions)
    {
        test_for_each_exception_async(p, iterator_tag());
        test_for_each_bad_alloc_async(p, iterator_tag());
    }
    test_for_each_async(std::forward<ExPolicy>(p), iterator_tag());
}

void for_each_executors_test()
{
    using namespace hpx::execution;

    {
        parallel_executor exec;

        test_executors(par.on(exec));
        test_executors_async(par(task).on(exec));

        test_executors(par_unseq.on(exec), false);
        test_executors_async(par_unseq(task).on(exec), false);
    }

    {
        sequenced_executor exec;

        test_executors(seq.on(exec));
        test_executors_async(seq(task).on(exec));

        test_executors(par.on(exec));
        test_executors_async(par(task).on(exec));

        test_executors(unseq.on(exec), false);
        test_executors_async(unseq(task).on(exec), false);

        test_executors(par_unseq.on(exec), false);
        test_executors_async(par_unseq(task).on(exec), false);
    }
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    for_each_executors_test();
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
