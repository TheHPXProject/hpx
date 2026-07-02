//  Copyright (c) 2015-2020 Hartmut Kaiser
//  Copyright (c) 2026 Siddharth
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "test_utils.hpp"

// default comparison (operator<)
template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_set_difference_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    std::vector<std::size_t> c1 = test::random_fill(10007);
    std::vector<std::size_t> c2 = test::random_fill(c1.size());
    std::sort(std::begin(c1), std::end(c1));
    std::sort(std::begin(c2), std::end(c2));

    std::vector<std::size_t> c3(c1.size());
    std::vector<std::size_t> c4(c1.size());

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    auto result =
        tt::sync_wait(ex::just(iterator(std::begin(c1)), iterator(std::end(c1)),
                          std::begin(c2), std::end(c2), std::begin(c3)) |
            hpx::set_difference(ex_policy.on(exec)));

    auto const expected_last = std::set_difference(std::begin(c1), std::end(c1),
        std::begin(c2), std::end(c2), std::begin(c4));

    // the returned destination iterator marks the end of the produced range
    auto const count = std::distance(std::begin(c4), expected_last);
    auto const result_last = hpx::get<0>(result.value());
    HPX_TEST(result_last == std::begin(c3) + count);

    // verify values
    HPX_TEST(std::equal(std::begin(c3), result_last, std::begin(c4)));
}

// custom comparator (operator>) -- exercises the F predicate through the
// sender pipeline
template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_set_difference_sender_comp(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    auto const comp = [](std::size_t lhs, std::size_t rhs) {
        return lhs > rhs;
    };

    std::vector<std::size_t> c1 = test::random_fill(10007);
    std::vector<std::size_t> c2 = test::random_fill(c1.size());
    std::sort(std::begin(c1), std::end(c1), comp);
    std::sort(std::begin(c2), std::end(c2), comp);

    std::vector<std::size_t> c3(c1.size());
    std::vector<std::size_t> c4(c1.size());

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    auto result =
        tt::sync_wait(ex::just(iterator(std::begin(c1)), iterator(std::end(c1)),
                          std::begin(c2), std::end(c2), std::begin(c3), comp) |
            hpx::set_difference(ex_policy.on(exec)));

    auto const expected_last = std::set_difference(std::begin(c1), std::end(c1),
        std::begin(c2), std::end(c2), std::begin(c4), comp);

    auto const count = std::distance(std::begin(c4), expected_last);
    auto const result_last = hpx::get<0>(result.value());
    HPX_TEST(result_last == std::begin(c3) + count);

    HPX_TEST(std::equal(std::begin(c3), result_last, std::begin(c4)));
}

// degenerate ranges -- both empty inputs now flow through set_operation's
// sentinel-initialized set_chunk_data instead of an early return
template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_set_difference_sender_empty(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    // empty first range -> empty result, dest unchanged
    {
        std::vector<std::size_t> c1;
        std::vector<std::size_t> c2 = test::random_fill(10007);
        std::sort(std::begin(c2), std::end(c2));
        std::vector<std::size_t> c3(c2.size(), static_cast<std::size_t>(-1));

        auto result = tt::sync_wait(
            ex::just(iterator(std::begin(c1)), iterator(std::end(c1)),
                std::begin(c2), std::end(c2), std::begin(c3)) |
            hpx::set_difference(ex_policy.on(exec)));

        auto const result_last = hpx::get<0>(result.value());
        HPX_TEST(result_last == std::begin(c3));
        HPX_TEST(std::all_of(std::begin(c3), std::end(c3),
            [](std::size_t v) { return v == static_cast<std::size_t>(-1); }));
    }

    // empty second range -> result equals first range
    {
        std::vector<std::size_t> c1 = test::random_fill(10007);
        std::sort(std::begin(c1), std::end(c1));
        std::vector<std::size_t> c2;
        std::vector<std::size_t> c3(c1.size());

        auto result = tt::sync_wait(
            ex::just(iterator(std::begin(c1)), iterator(std::end(c1)),
                std::begin(c2), std::end(c2), std::begin(c3)) |
            hpx::set_difference(ex_policy.on(exec)));

        auto const result_last = hpx::get<0>(result.value());
        HPX_TEST(result_last == std::begin(c3) + c1.size());
        HPX_TEST(std::equal(std::begin(c3), result_last, std::begin(c1)));
    }
}

template <typename IteratorTag>
void set_difference_sender_test()
{
    using namespace hpx::execution;
    test_set_difference_sender(hpx::launch::sync, seq(task), IteratorTag());
    test_set_difference_sender(hpx::launch::sync, unseq(task), IteratorTag());
    test_set_difference_sender(hpx::launch::async, par(task), IteratorTag());
    test_set_difference_sender(
        hpx::launch::async, par_unseq(task), IteratorTag());

    test_set_difference_sender_comp(
        hpx::launch::sync, seq(task), IteratorTag());
    test_set_difference_sender_comp(
        hpx::launch::sync, unseq(task), IteratorTag());
    test_set_difference_sender_comp(
        hpx::launch::async, par(task), IteratorTag());
    test_set_difference_sender_comp(
        hpx::launch::async, par_unseq(task), IteratorTag());

    test_set_difference_sender_empty(
        hpx::launch::sync, seq(task), IteratorTag());
    test_set_difference_sender_empty(
        hpx::launch::sync, unseq(task), IteratorTag());
    test_set_difference_sender_empty(
        hpx::launch::async, par(task), IteratorTag());
    test_set_difference_sender_empty(
        hpx::launch::async, par_unseq(task), IteratorTag());
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = static_cast<unsigned int>(std::time(nullptr));
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    set_difference_sender_test<std::forward_iterator_tag>();
    set_difference_sender_test<std::random_access_iterator_tag>();

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
