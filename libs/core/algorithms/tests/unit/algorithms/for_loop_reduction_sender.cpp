//  Copyright (c) 2016 Hartmut Kaiser
//  Copyright (c) 2024 Tobias Wukovitsch
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
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "test_utils.hpp"

///////////////////////////////////////////////////////////////////////////////
int seed = std::random_device{}();
std::mt19937 gen(seed);

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_for_loop_reduction_plus_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::size_t sum = 0;

    tt::sync_wait(ex::just(iterator(std::begin(c)), iterator(std::end(c)),
                      hpx::experimental::reduction_plus(sum),
                      [](iterator it, std::size_t& sum) { sum += *it; }) |
        hpx::experimental::for_loop(ex_policy.on(exec)));

    // verify values
    std::size_t sum2 =
        std::accumulate(std::begin(c), std::end(c), std::size_t(0));
    HPX_TEST_EQ(sum, sum2);
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_for_loop_reduction_multiplies_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::size_t prod = 0;

    tt::sync_wait(ex::just(iterator(std::begin(c)), iterator(std::end(c)),
                      hpx::experimental::reduction_multiplies(prod),
                      [](iterator it, std::size_t& prod) { prod *= *it; }) |
        hpx::experimental::for_loop(ex_policy.on(exec)));

    // verify values
    std::size_t prod2 = std::accumulate(std::begin(c), std::end(c),
        std::size_t(1), std::multiplies<std::size_t>());
    HPX_TEST_EQ(prod, prod2);
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_for_loop_reduction_min_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::size_t minval = c[0];

    tt::sync_wait(ex::just(iterator(std::begin(c)), iterator(std::end(c)),
                      hpx::experimental::reduction_min(minval),
                      [](iterator it, std::size_t& minval) {
                          minval = (std::min) (minval, *it);
                      }) |
        hpx::experimental::for_loop(ex_policy.on(exec)));

    // verify values
    std::size_t minval2 = std::accumulate(std::begin(c), std::end(c), c[0],
        hpx::parallel::detail::min_of<std::size_t>());
    HPX_TEST_EQ(minval, minval2);
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_for_loop_reduction_sender()
{
    using namespace hpx::execution;
    auto const sync = hpx::launch::sync;
    auto const async = hpx::launch::async;

    test_for_loop_reduction_plus_sender(sync, seq(task), IteratorTag());
    test_for_loop_reduction_plus_sender(async, par(task), IteratorTag());
    test_for_loop_reduction_plus_sender(async, par_unseq(task), IteratorTag());

    test_for_loop_reduction_multiplies_sender(sync, seq(task), IteratorTag());
    test_for_loop_reduction_multiplies_sender(async, par(task), IteratorTag());
    test_for_loop_reduction_multiplies_sender(
        async, par_unseq(task), IteratorTag());

    test_for_loop_reduction_min_sender(sync, seq(task), IteratorTag());
    test_for_loop_reduction_min_sender(async, par(task), IteratorTag());
    test_for_loop_reduction_min_sender(async, par_unseq(task), IteratorTag());
}

void for_loop_reduction_test_sender()
{
    test_for_loop_reduction_sender<std::random_access_iterator_tag>();
    test_for_loop_reduction_sender<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename LnPolicy, typename ExPolicy>
void test_for_loop_reduction_bit_and_idx_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::size_t bits = ~std::size_t(0);

    tt::sync_wait(
        ex::just(0, c.size(), hpx::experimental::reduction_bit_and(bits),
            [&c](std::size_t i, std::size_t& bits) { bits &= c[i]; }) |
        hpx::experimental::for_loop(ex_policy.on(exec)));

    // verify values
    std::size_t bits2 = std::accumulate(std::begin(c), std::end(c),
        ~std::size_t(0), std::bit_and<std::size_t>());
    HPX_TEST_EQ(bits, bits2);
}

template <typename LnPolicy, typename ExPolicy>
void test_for_loop_reduction_bit_or_idx_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::size_t bits = 0;

    tt::sync_wait(
        ex::just(0, c.size(), hpx::experimental::reduction_bit_or(bits),
            [&c](std::size_t i, std::size_t& bits) { bits |= c[i]; }) |
        hpx::experimental::for_loop(ex_policy.on(exec)));

    // verify values
    std::size_t bits2 = std::accumulate(
        std::begin(c), std::end(c), std::size_t(0), std::bit_or<std::size_t>());
    HPX_TEST_EQ(bits, bits2);
}

void for_loop_reduction_test_idx_sender()
{
    using namespace hpx::execution;
    auto const sync = hpx::launch::sync;
    auto const async = hpx::launch::async;

    test_for_loop_reduction_bit_and_idx_sender(sync, seq(task));
    test_for_loop_reduction_bit_and_idx_sender(async, par(task));
    test_for_loop_reduction_bit_and_idx_sender(async, par_unseq(task));

    test_for_loop_reduction_bit_or_idx_sender(sync, seq(task));
    test_for_loop_reduction_bit_or_idx_sender(async, par(task));
    test_for_loop_reduction_bit_or_idx_sender(async, par_unseq(task));
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    for_loop_reduction_test_sender();
    for_loop_reduction_test_idx_sender();

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
