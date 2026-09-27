//  Copyright (c) 2015 Daniel Bourgeois
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

#include "is_sorted_range_tests_exception.hpp"

////////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_sorted_exception()
{
    using namespace hpx::execution;
    //If the execution policy object is of type vector_execution_policy,
    //  std::terminate shall be called. Therefore, we do not test exceptions
    //  with a vector execution policy
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_sorted_exception(seq, IteratorTag());
        test_sorted_exception(par, IteratorTag());

        test_sorted_exception_async(seq(task), IteratorTag());
        test_sorted_exception_async(par(task), IteratorTag());
    }

    test_sorted_exception_seq(IteratorTag());
}

void sorted_exception_test()
{
    test_sorted_exception<std::random_access_iterator_tag>();
    test_sorted_exception<std::forward_iterator_tag>();

    using namespace hpx::execution;

    test_sorted_exception(seq);
    test_sorted_exception(par);

    test_sorted_exception_async(seq(task));
    test_sorted_exception_async(par(task));

    test_sorted_exception_seq();
}

////////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_sorted_bad_alloc()
{
    using namespace hpx::execution;

    // If the execution policy object is of type vector_execution_policy,
    // std::terminate shall be called. therefore we do not test exceptions
    // with a vector execution policy
    if constexpr (std::is_same_v<IteratorTag, std::random_access_iterator_tag>)
    {
        test_sorted_bad_alloc(par, IteratorTag());
        test_sorted_bad_alloc(seq, IteratorTag());

        test_sorted_bad_alloc_async(seq(task), IteratorTag());
        test_sorted_bad_alloc_async(par(task), IteratorTag());
    }

    test_sorted_bad_alloc_seq(IteratorTag());
}

void sorted_bad_alloc_test()
{
    test_sorted_bad_alloc<std::random_access_iterator_tag>();
    test_sorted_bad_alloc<std::forward_iterator_tag>();

    using namespace hpx::execution;

    test_sorted_bad_alloc(par);
    test_sorted_bad_alloc(seq);

    test_sorted_bad_alloc_async(seq(task));
    test_sorted_bad_alloc_async(par(task));

    test_sorted_bad_alloc_seq();
}

////////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    sorted_exception_test();
    sorted_bad_alloc_test();
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    hpx::local::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
