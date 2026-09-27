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
void test_for_loop_induction_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(10007);
    std::iota(std::begin(c), std::end(c), gen());

    tt::sync_wait(ex::just(iterator(std::begin(c)), iterator(std::end(c)),
                      hpx::experimental::induction(0),
                      [&d](iterator it, std::size_t i) {
                          *it = 42;
                          d[i] = 42;
                      }) |
        hpx::experimental::for_loop(ex_policy.on(exec)));

    // verify values
    std::size_t count = 0;
    std::for_each(std::begin(c), std::end(c), [&count](std::size_t v) -> void {
        HPX_TEST_EQ(v, static_cast<std::size_t>(42));
        ++count;
    });
    std::for_each(std::begin(d), std::end(d), [](std::size_t v) -> void {
        HPX_TEST_EQ(v, static_cast<std::size_t>(42));
    });
    HPX_TEST_EQ(count, c.size());
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_for_loop_induction_stride_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(10007);
    std::iota(std::begin(c), std::end(c), gen());

    tt::sync_wait(
        ex::just(iterator(std::begin(c)), iterator(std::end(c)),
            hpx::experimental::induction(0), hpx::experimental::induction(0, 2),
            [&d](iterator it, std::size_t i, std::size_t j) {
                *it = 42;
                d[i] = 42;
                HPX_TEST_EQ(2 * i, j);
            }) |
        hpx::experimental::for_loop(ex_policy.on(exec)));

    // verify values
    std::size_t count = 0;
    std::for_each(std::begin(c), std::end(c), [&count](std::size_t v) -> void {
        HPX_TEST_EQ(v, std::size_t(42));
        ++count;
    });
    std::for_each(std::begin(d), std::end(d),
        [](std::size_t v) -> void { HPX_TEST_EQ(v, std::size_t(42)); });
    HPX_TEST_EQ(count, c.size());
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_for_loop_induction_life_out_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::size_t curr = 0;

    tt::sync_wait(ex::just(iterator(std::begin(c)), iterator(std::end(c)),
                      hpx::experimental::induction(curr),
                      [&d](iterator it, std::size_t i) {
                          *it = 42;
                          d[i] = 42;
                      }) |
        hpx::experimental::for_loop(ex_policy.on(exec)));

    HPX_TEST_EQ(curr, c.size());

    // verify values
    std::size_t count = 0;
    std::for_each(std::begin(c), std::end(c), [&count](std::size_t v) -> void {
        HPX_TEST_EQ(v, std::size_t(42));
        ++count;
    });
    std::for_each(std::begin(d), std::end(d),
        [](std::size_t v) -> void { HPX_TEST_EQ(v, std::size_t(42)); });
    HPX_TEST_EQ(count, c.size());
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_for_loop_induction_stride_life_out_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_execution_policy<ExPolicy>::value,
        "hpx::is_execution_policy<ExPolicy>::value");

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(10007);
    std::iota(std::begin(c), std::end(c), gen());

    std::size_t curr1 = 0;
    std::size_t curr2 = 0;

    tt::sync_wait(ex::just(iterator(std::begin(c)), iterator(std::end(c)),
                      hpx::experimental::induction(curr1),
                      hpx::experimental::induction(curr2, 2),
                      [&d](iterator it, std::size_t i, std::size_t j) {
                          *it = 42;
                          d[i] = 42;
                          HPX_TEST_EQ(2 * i, j);
                      }) |
        hpx::experimental::for_loop(ex_policy.on(exec)));

    HPX_TEST_EQ(curr1, c.size());
    HPX_TEST_EQ(curr2, 2 * c.size());

    // verify values
    std::size_t count = 0;
    std::for_each(std::begin(c), std::end(c), [&count](std::size_t v) -> void {
        HPX_TEST_EQ(v, std::size_t(42));
        ++count;
    });
    std::for_each(std::begin(d), std::end(d),
        [](std::size_t v) -> void { HPX_TEST_EQ(v, std::size_t(42)); });
    HPX_TEST_EQ(count, c.size());
}

///////////////////////////////////////////////////////////////////////////////
template <typename IteratorTag>
void test_for_loop_induction_sender()
{
    using namespace hpx::execution;
    auto const sync = hpx::launch::sync;
    auto const async = hpx::launch::async;

    test_for_loop_induction_sender(sync, seq(task), IteratorTag());
    test_for_loop_induction_sender(async, par(task), IteratorTag());
    test_for_loop_induction_sender(async, par_unseq(task), IteratorTag());

    test_for_loop_induction_stride_sender(sync, seq(task), IteratorTag());
    test_for_loop_induction_stride_sender(async, par(task), IteratorTag());
    test_for_loop_induction_stride_sender(
        async, par_unseq(task), IteratorTag());

    test_for_loop_induction_life_out_sender(sync, seq(task), IteratorTag());
    test_for_loop_induction_life_out_sender(async, par(task), IteratorTag());
    test_for_loop_induction_life_out_sender(
        async, par_unseq(task), IteratorTag());

    test_for_loop_induction_stride_life_out_sender(
        sync, seq(task), IteratorTag());
    test_for_loop_induction_stride_life_out_sender(
        async, par(task), IteratorTag());
    test_for_loop_induction_stride_life_out_sender(
        async, par_unseq(task), IteratorTag());
}

void for_loop_induction_test_sender()
{
    test_for_loop_induction_sender<std::random_access_iterator_tag>();
    test_for_loop_induction_sender<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
template <typename LnPolicy, typename ExPolicy>
void test_for_loop_induction_idx_sender(
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

    tt::sync_wait(ex::just(0, c.size(), hpx::experimental::induction(0),
                      [&c](std::size_t i, std::size_t j) {
                          c[i] = 42;
                          HPX_TEST_EQ(i, j);
                      }) |
        hpx::experimental::for_loop(ex_policy.on(exec)));

    // verify values
    std::size_t count = 0;
    std::for_each(std::begin(c), std::end(c), [&count](std::size_t v) -> void {
        HPX_TEST_EQ(v, std::size_t(42));
        ++count;
    });
    HPX_TEST_EQ(count, c.size());
}

template <typename LnPolicy, typename ExPolicy>
void test_for_loop_induction_stride_idx_sender(
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

    tt::sync_wait(ex::just(0, c.size(), hpx::experimental::induction(0),
                      hpx::experimental::induction(0, 2),
                      [&c](std::size_t i, std::size_t j, std::size_t k) {
                          c[i] = 42;
                          HPX_TEST_EQ(i, j);
                          HPX_TEST_EQ(2 * i, k);
                      }) |
        hpx::experimental::for_loop(ex_policy.on(exec)));

    // verify values
    std::size_t count = 0;
    std::for_each(std::begin(c), std::end(c), [&count](std::size_t v) -> void {
        HPX_TEST_EQ(v, std::size_t(42));
        ++count;
    });
    HPX_TEST_EQ(count, c.size());
}

void for_loop_induction_test_idx_sender()
{
    using namespace hpx::execution;
    auto const sync = hpx::launch::sync;
    auto const async = hpx::launch::async;

    test_for_loop_induction_idx_sender(sync, seq(task));
    test_for_loop_induction_idx_sender(async, par(task));
    test_for_loop_induction_idx_sender(async, par_unseq(task));

    test_for_loop_induction_stride_idx_sender(sync, seq(task));
    test_for_loop_induction_stride_idx_sender(async, par(task));
    test_for_loop_induction_stride_idx_sender(async, par_unseq(task));
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    for_loop_induction_test_sender();
    for_loop_induction_test_idx_sender();

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
