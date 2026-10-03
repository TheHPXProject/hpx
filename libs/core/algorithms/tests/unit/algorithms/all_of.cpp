//  Copyright (c) 2014-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

// Apple Clang and Clang V19.1.1 ICE's while compiling this file
#if !defined(HPX_APPLE_CLANG_VERSION) &&                                       \
    (!defined(HPX_CLANG_VERSION) || HPX_CLANG_VERSION != 190101)

#include <hpx/init.hpp>

#include <cstddef>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include "all_of_tests.hpp"

////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_all_of()
{
    struct proj
    {
        //This projection should cause tests to fail if it is not applied
        //because it causes predicate to evaluate the opposite
        constexpr std::size_t operator()(std::size_t x) const
        {
            return !static_cast<bool>(x);
        }
    };
    using namespace hpx::execution;

    test_all_of(IteratorTag());
    test_all_of_ranges_seq(IteratorTag(), proj());

    test_all_of(seq, IteratorTag());
    test_all_of(par, IteratorTag());
    test_all_of(par_unseq, IteratorTag());

    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_all_of_ranges(seq, IteratorTag(), proj());
        test_all_of_ranges(par, IteratorTag(), proj());
        test_all_of_ranges(par_unseq, IteratorTag(), proj());
    }

    test_all_of_async(seq(task), IteratorTag());
    test_all_of_async(par(task), IteratorTag());

    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_all_of_ranges_async(seq(task), IteratorTag(), proj());
        test_all_of_ranges_async(par(task), IteratorTag(), proj());
    }
}

// template <typename IteratorTag>
// void test_all_of_exec()
// {
//     using namespace hpx::execution;
//
//     {
//         hpx::threads::executors::local_priority_queue_executor exec;
//         test_all_of(par(exec), IteratorTag());
//     }
//     {
//         hpx::threads::executors::local_priority_queue_executor exec;
//         test_all_of(task(exec), IteratorTag());
//     }
//
//     {
//         hpx::threads::executors::local_priority_queue_executor exec;
//         test_all_of(execution_policy(par(exec)), IteratorTag());
//     }
//     {
//         hpx::threads::executors::local_priority_queue_executor exec;
//         test_all_of(execution_policy(task(exec)), IteratorTag());
//     }
// }

void all_of_test()
{
    test_all_of<std::random_access_iterator_tag>();
    test_all_of<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_all_of_exception()
{
    using namespace hpx::execution;

    test_all_of_exception(IteratorTag());

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_all_of_exception(seq, IteratorTag());
    test_all_of_exception(par, IteratorTag());

    test_all_of_exception_async(seq(task), IteratorTag());
    test_all_of_exception_async(par(task), IteratorTag());
}

void all_of_exception_test()
{
    test_all_of_exception<std::random_access_iterator_tag>();
    test_all_of_exception<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_all_of_bad_alloc()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    test_all_of_bad_alloc(seq, IteratorTag());
    test_all_of_bad_alloc(par, IteratorTag());

    test_all_of_bad_alloc_async(seq(task), IteratorTag());
    test_all_of_bad_alloc_async(par(task), IteratorTag());
}

void all_of_bad_alloc_test()
{
    test_all_of_bad_alloc<std::random_access_iterator_tag>();
    test_all_of_bad_alloc<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = static_cast<unsigned int>(std::time(nullptr));
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    all_of_test();
    all_of_exception_test();
    all_of_bad_alloc_test();
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

    // By default, this test should run on all available cores
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}

#else

int main()
{
    return 0;
}

#endif
