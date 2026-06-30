//  Copyright (c) 2014-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include "test_utils.hpp"

///////////////////////////////////////////////////////////////////////////////
template <typename Policy, typename ExPolicy, typename IteratorTag>
void test_for_each_explicit_sender_direct(
    Policy l, ExPolicy&& policy, IteratorTag)
{
    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    namespace ex = hpx::execution::experimental;

    auto f = [](std::size_t& v) { v = 42; };

    using scheduler_t = ex::thread_pool_policy_scheduler<Policy>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(l));
    auto result = hpx::for_each(
        policy.on(exec), iterator(std::begin(c)), iterator(std::end(c)), f);
    HPX_TEST(result == iterator(std::end(c)));

    // verify values
    std::size_t count = 0;
    std::for_each(std::begin(c), std::end(c), [&count](std::size_t v) -> void {
        HPX_TEST_EQ(v, std::size_t(42));
        ++count;
    });
    HPX_TEST_EQ(count, c.size());
}

///////////////////////////////////////////////////////////////////////////////
template <typename Policy, typename ExPolicy, typename IteratorTag>
void test_for_each_explicit_sender_direct_async(
    Policy l, ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;

    auto f = [](std::size_t& v) { v = 42; };

    using scheduler_t = ex::thread_pool_policy_scheduler<Policy>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(l));

    auto snd_result = tt::sync_wait(hpx::for_each(
        policy.on(exec), iterator(std::begin(c)), iterator(std::end(c)), f));
    auto result = hpx::get<0>(*snd_result);

    HPX_TEST(result == iterator(std::end(c)));

    // verify values
    std::size_t count = 0;
    std::for_each(std::begin(c), std::end(c), [&count](std::size_t v) -> void {
        HPX_TEST_EQ(v, std::size_t(42));
        ++count;
    });
    HPX_TEST_EQ(count, c.size());
}

///////////////////////////////////////////////////////////////////////////////
template <typename Policy, typename ExPolicy, typename IteratorTag>
void test_for_each_explicit_sender(Policy l, ExPolicy&& policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    std::vector<std::size_t> c(10007);
    std::iota(std::begin(c), std::end(c), std::rand());

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;

    auto f = [](std::size_t& v) { v = 42; };

    using scheduler_t = ex::thread_pool_policy_scheduler<Policy>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(l));

    auto snd_result = tt::sync_wait(
        ex::just(iterator(std::begin(c)), iterator(std::end(c)), f) |
        hpx::for_each(policy.on(exec)));
    auto result = hpx::get<0>(*snd_result);

    HPX_TEST(result == iterator(std::end(c)));

    // verify values
    std::size_t count = 0;
    std::for_each(std::begin(c), std::end(c), [&count](std::size_t v) -> void {
        HPX_TEST_EQ(v, std::size_t(42));
        ++count;
    });
    HPX_TEST_EQ(count, c.size());
}

template <typename IteratorTag>
void test_for_each_sender_direct()
{
    using namespace hpx::execution;

    test_for_each_explicit_sender_direct(hpx::launch::sync, seq, IteratorTag());
    test_for_each_explicit_sender_direct(
        hpx::launch::sync, unseq, IteratorTag());
    test_for_each_explicit_sender_direct(
        hpx::launch::async, par, IteratorTag());
    test_for_each_explicit_sender_direct(
        hpx::launch::async, par_unseq, IteratorTag());

    test_for_each_explicit_sender_direct_async(
        hpx::launch::sync, seq(task), IteratorTag());
    test_for_each_explicit_sender_direct_async(
        hpx::launch::sync, unseq(task), IteratorTag());
    test_for_each_explicit_sender_direct_async(
        hpx::launch::async, par(task), IteratorTag());
    test_for_each_explicit_sender_direct_async(
        hpx::launch::async, par_unseq(task), IteratorTag());
}

template <typename IteratorTag>
void test_for_each_sender()
{
    using namespace hpx::execution;

    test_for_each_explicit_sender(hpx::launch::sync, seq(task), IteratorTag());
    test_for_each_explicit_sender(
        hpx::launch::sync, unseq(task), IteratorTag());
    test_for_each_explicit_sender(hpx::launch::async, par(task), IteratorTag());
    test_for_each_explicit_sender(
        hpx::launch::async, par_unseq(task), IteratorTag());
}

void for_each_sender_test_direct()
{
    test_for_each_sender_direct<std::random_access_iterator_tag>();
    test_for_each_sender_direct<std::forward_iterator_tag>();
}

void for_each_sender_test()
{
    test_for_each_sender<std::random_access_iterator_tag>();
    test_for_each_sender<std::forward_iterator_tag>();
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    for_each_sender_test_direct();
    for_each_sender_test();

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
