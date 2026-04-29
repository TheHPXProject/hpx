//  Copyright (c) 2015 Daniel Bourgeois
//  Copyright (c) 2021 Karame M.Shokooh
//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/iterator_support/tests/iter_sent.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/numeric.hpp>

#include <cstddef>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>

#include "test_utils.hpp"

////////////////////////////////////////////////////////////////////////////
template <typename Policy, typename ExPolicy>
void test_sen_direct(Policy l, ExPolicy&& policy)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    std::vector<std::size_t> c(10007);
    std::iota(c.begin(), c.end(), 1);
    std::vector<std::size_t> d(10007);
    std::vector<std::size_t> d_ans(10007);

    namespace ex = hpx::execution::experimental;

    using scheduler_t = ex::thread_pool_policy_scheduler<Policy>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(l));
    auto it = hpx::ranges::adjacent_difference(policy.on(exec), std::begin(c),
        sentinel<std::size_t>{10007}, std::begin(d));

    std::adjacent_difference(std::begin(c), std::end(c), std::begin(d_ans));

    HPX_TEST(std::equal(std::begin(d), std::end(d) - 1, std::begin(d_ans)));
    HPX_TEST(std::end(d) - 1 == it);
}

template <typename Policy, typename ExPolicy>
void test_adjacent_difference_direct(Policy l, ExPolicy&& policy)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    std::vector<std::size_t> c = test::random_iota(10007);
    std::vector<std::size_t> d(10007);
    std::vector<std::size_t> d_ans(10007);

    namespace ex = hpx::execution::experimental;

    using scheduler_t = ex::thread_pool_policy_scheduler<Policy>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(l));
    auto it =
        hpx::ranges::adjacent_difference(policy.on(exec), c, std::begin(d));

    std::adjacent_difference(std::begin(c), std::end(c), std::begin(d_ans));

    HPX_TEST(std::equal(std::begin(d), std::end(d), std::begin(d_ans)));
    HPX_TEST(std::end(d) == it);
}

template <typename Policy, typename ExPolicy>
void test_adjacent_difference_async_direct(Policy l, ExPolicy&& p)
{
    static_assert(hpx::is_execution_policy_v<ExPolicy>,
        "hpx::is_execution_policy_v<ExPolicy>");

    std::vector<std::size_t> c = test::random_iota(10007);
    std::vector<std::size_t> d(10007);
    std::vector<std::size_t> d_ans(10007);

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;

    using scheduler_t = ex::thread_pool_policy_scheduler<Policy>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(l));
    auto result = tt::sync_wait(
        hpx::ranges::adjacent_difference(p.on(exec), c, std::begin(d)));

    std::adjacent_difference(std::begin(c), std::end(c), std::begin(d_ans));

    HPX_TEST(std::equal(std::begin(d), std::end(d), std::begin(d_ans)));
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    HPX_TEST(std::end(d) == hpx::get<0>(*result));
}

void adjacent_difference_test()
{
    using namespace hpx::execution;
    test_adjacent_difference_direct(hpx::launch::sync, seq);
    test_adjacent_difference_direct(hpx::launch::sync, unseq);

    test_adjacent_difference_direct(hpx::launch::async, par);
    test_adjacent_difference_direct(hpx::launch::async, par_unseq);

    test_sen_direct(hpx::launch::sync, seq);
    test_sen_direct(hpx::launch::sync, unseq);

    test_sen_direct(hpx::launch::async, par);
    test_sen_direct(hpx::launch::async, par_unseq);

    test_adjacent_difference_async_direct(hpx::launch::sync, seq(task));
    test_adjacent_difference_async_direct(hpx::launch::async, par(task));
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    adjacent_difference_test();
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
