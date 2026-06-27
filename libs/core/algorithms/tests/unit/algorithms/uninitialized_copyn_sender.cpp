//  Copyright (c) 2025 Zhengnan Hua
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/init.hpp>
#include <hpx/memory.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "test_utils.hpp"

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_uninitialized_copy_n_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    std::vector<std::size_t> c(10007);
    std::vector<std::size_t> d(c.size());
    std::iota(std::begin(c), std::end(c), std::rand());

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    // When the size is -1, the algorithm should not do anything.
    tt::sync_wait(ex::just(iterator(std::begin(c)), -1, std::begin(d)) |
        hpx::uninitialized_copy_n(ex_policy.on(exec)));
    HPX_TEST(std::all_of(
        std::begin(d), std::end(d), [](std::size_t i) { return i == 0; }));

    tt::sync_wait(ex::just(iterator(std::begin(c)), c.size(), std::begin(d)) |
        hpx::uninitialized_copy_n(ex_policy.on(exec)));

    std::size_t count = 0;
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d),
        [&count](std::size_t v1, std::size_t v2) -> bool {
            HPX_TEST_EQ(v1, v2);
            ++count;
            return v1 == v2;
        }));
    HPX_TEST_EQ(count, d.size());
}

template <typename IteratorTag>
void uninitialized_copy_n_sender_test()
{
    using namespace hpx::execution;
    test_uninitialized_copy_n_sender(
        hpx::launch::sync, seq(task), IteratorTag());
    test_uninitialized_copy_n_sender(
        hpx::launch::sync, unseq(task), IteratorTag());

    test_uninitialized_copy_n_sender(
        hpx::launch::async, par(task), IteratorTag());
    test_uninitialized_copy_n_sender(
        hpx::launch::async, par_unseq(task), IteratorTag());
}

////////////////////////////////////////////////////////////////////////////
template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_uninitialized_copy_n_exception_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<test::count_instances>::iterator;
    using decorated_iterator =
        test::decorated_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;

    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    std::vector<test::count_instances> c(10007);
    std::vector<test::count_instances> d(c.size());
    std::iota(std::begin(c), std::end(c), std::rand());

    std::atomic<std::size_t> throw_after(std::rand() % c.size());    //-V104
    test::count_instances::instance_count.store(0);

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    bool caught_exception = false;
    try
    {
        tt::sync_wait(ex::just(decorated_iterator(std::begin(c),
                                   [&throw_after]() {
                                       if (throw_after-- == 0)
                                           throw std::runtime_error("test");
                                   }),
                          c.size(), std::begin(d)) |
            hpx::uninitialized_copy_n(ex_policy.on(exec)));
        HPX_TEST(false);
    }
    catch (hpx::exception_list const& e)
    {
        caught_exception = true;
        test::test_num_exceptions<ExPolicy, IteratorTag>::call(ex_policy, e);
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_exception);
    HPX_TEST_EQ(test::count_instances::instance_count.load(), std::size_t(0));
}

template <typename IteratorTag>
void uninitialized_copy_n_exception_sender_test()
{
    using namespace hpx::execution;
    test_uninitialized_copy_n_exception_sender(
        hpx::launch::sync, seq(task), IteratorTag());
    test_uninitialized_copy_n_exception_sender(
        hpx::launch::sync, unseq(task), IteratorTag());

    test_uninitialized_copy_n_exception_sender(
        hpx::launch::async, par(task), IteratorTag());
    test_uninitialized_copy_n_exception_sender(
        hpx::launch::async, par_unseq(task), IteratorTag());
}

////////////////////////////////////////////////////////////////////////////
template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_uninitialized_copy_n_bad_alloc_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<test::count_instances>::iterator;
    using decorated_iterator =
        test::decorated_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;

    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    std::vector<test::count_instances> c(10007);
    std::vector<test::count_instances> d(c.size());
    std::iota(std::begin(c), std::end(c), std::rand());

    std::atomic<std::size_t> throw_after(std::rand() % c.size());    //-V104
    test::count_instances::instance_count.store(0);

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    bool caught_bad_alloc = false;
    try
    {
        tt::sync_wait(ex::just(decorated_iterator(std::begin(c),
                                   [&throw_after]() {
                                       if (throw_after-- == 0)
                                           throw std::bad_alloc();
                                   }),
                          c.size(), std::begin(d)) |
            hpx::uninitialized_copy_n(ex_policy.on(exec)));
        HPX_TEST(false);
    }
    catch (std::bad_alloc const&)
    {
        caught_bad_alloc = true;
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(caught_bad_alloc);
    HPX_TEST_EQ(test::count_instances::instance_count.load(), std::size_t(0));
}

template <typename IteratorTag>
void uninitialized_copy_n_bad_alloc_sender_test()
{
    using namespace hpx::execution;
    test_uninitialized_copy_n_bad_alloc_sender(
        hpx::launch::sync, seq(task), IteratorTag());
    test_uninitialized_copy_n_bad_alloc_sender(
        hpx::launch::sync, unseq(task), IteratorTag());

    test_uninitialized_copy_n_bad_alloc_sender(
        hpx::launch::async, par(task), IteratorTag());
    test_uninitialized_copy_n_bad_alloc_sender(
        hpx::launch::async, par_unseq(task), IteratorTag());
}

////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    uninitialized_copy_n_sender_test<std::forward_iterator_tag>();
    uninitialized_copy_n_sender_test<std::random_access_iterator_tag>();

    uninitialized_copy_n_exception_sender_test<std::forward_iterator_tag>();
    uninitialized_copy_n_exception_sender_test<
        std::random_access_iterator_tag>();

    uninitialized_copy_n_bad_alloc_sender_test<std::forward_iterator_tag>();
    uninitialized_copy_n_bad_alloc_sender_test<
        std::random_access_iterator_tag>();

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
