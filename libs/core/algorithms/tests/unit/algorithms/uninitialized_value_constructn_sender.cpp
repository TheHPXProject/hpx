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
#include <cstring>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include "test_utils.hpp"

struct value_constructable
{
    int value_;
};

std::size_t const data_size = 10007;

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_uninitialized_value_construct_n_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    using base_iterator = value_constructable*;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    value_constructable* p = (value_constructable*) std::malloc(
        data_size * sizeof(value_constructable));
    std::memset(
        static_cast<void*>(p), 0xcd, data_size * sizeof(value_constructable));

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    tt::sync_wait(ex::just(iterator(p), data_size) |
        hpx::uninitialized_value_construct_n(ex_policy.on(exec)));

    std::size_t count = 0;
    std::for_each(p, p + data_size, [&count](value_constructable v1) {
        HPX_TEST_EQ(v1.value_, 0);
        ++count;
    });
    HPX_TEST_EQ(count, data_size);

    std::free(p);
}

template <typename IteratorTag>
void uninitialized_value_construct_n_sender_test()
{
    using namespace hpx::execution;
    test_uninitialized_value_construct_n_sender(
        hpx::launch::sync, seq(task), IteratorTag());
    test_uninitialized_value_construct_n_sender(
        hpx::launch::sync, unseq(task), IteratorTag());

    test_uninitialized_value_construct_n_sender(
        hpx::launch::async, par(task), IteratorTag());
    test_uninitialized_value_construct_n_sender(
        hpx::launch::async, par_unseq(task), IteratorTag());
}

///////////////////////////////////////////////////////////////////////////////
template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_uninitialized_value_construct_n_exception_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    using data_type = test::count_instances_v<value_constructable>;
    using base_iterator = data_type*;
    using decorated_iterator =
        test::decorated_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    data_type* p = (data_type*) std::malloc(data_size * sizeof(data_type));
    std::memset(static_cast<void*>(p), 0xcd, data_size * sizeof(data_type));

    std::atomic<std::size_t> throw_after(std::rand() % data_size);    //-V104
    std::size_t throw_after_ = throw_after.load();

    data_type::instance_count.store(0);
    data_type::max_instance_count.store(0);

    bool caught_exception = false;
    try
    {
        auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

        tt::sync_wait(ex::just(decorated_iterator(p,
                                   [&throw_after]() {
                                       if (throw_after-- == 0)
                                           throw std::runtime_error("test");
                                   }),
                          data_size) |
            hpx::uninitialized_value_construct_n(ex_policy.on(exec)));

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
    HPX_TEST_LTE(throw_after_, data_type::max_instance_count.load());

    std::free(p);
}

template <typename IteratorTag>
void uninitialized_value_construct_n_exception_sender_test()
{
    using namespace hpx::execution;
    test_uninitialized_value_construct_n_exception_sender(
        hpx::launch::sync, seq(task), IteratorTag());
    test_uninitialized_value_construct_n_exception_sender(
        hpx::launch::sync, unseq(task), IteratorTag());

    test_uninitialized_value_construct_n_exception_sender(
        hpx::launch::async, par(task), IteratorTag());
    test_uninitialized_value_construct_n_exception_sender(
        hpx::launch::async, par_unseq(task), IteratorTag());
}

///////////////////////////////////////////////////////////////////////////////
template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_uninitialized_value_construct_n_bad_alloc_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    using data_type = test::count_instances_v<value_constructable>;
    using base_iterator = data_type*;
    using decorated_iterator =
        test::decorated_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    data_type* p = (data_type*) std::malloc(data_size * sizeof(data_type));
    std::memset(static_cast<void*>(p), 0xcd, data_size * sizeof(data_type));

    std::atomic<std::size_t> throw_after(std::rand() % data_size);    //-V104
    std::size_t throw_after_ = throw_after.load();

    data_type::instance_count.store(0);
    data_type::max_instance_count.store(0);

    bool caught_bad_alloc = false;
    try
    {
        auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

        tt::sync_wait(ex::just(decorated_iterator(p,
                                   [&throw_after]() {
                                       if (throw_after-- == 0)
                                           throw std::bad_alloc();
                                   }),
                          data_size) |
            hpx::uninitialized_value_construct_n(ex_policy.on(exec)));

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
    HPX_TEST_LTE(throw_after_, data_type::max_instance_count.load());

    std::free(p);
}

template <typename IteratorTag>
void uninitialized_value_construct_n_bad_alloc_sender_tests()
{
    using namespace hpx::execution;
    test_uninitialized_value_construct_n_bad_alloc_sender(
        hpx::launch::sync, seq(task), IteratorTag());
    test_uninitialized_value_construct_n_bad_alloc_sender(
        hpx::launch::sync, unseq(task), IteratorTag());

    test_uninitialized_value_construct_n_bad_alloc_sender(
        hpx::launch::async, par(task), IteratorTag());
    test_uninitialized_value_construct_n_bad_alloc_sender(
        hpx::launch::async, par_unseq(task), IteratorTag());
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    uninitialized_value_construct_n_sender_test<
        std::random_access_iterator_tag>();
    uninitialized_value_construct_n_sender_test<std::forward_iterator_tag>();

    uninitialized_value_construct_n_exception_sender_test<
        std::random_access_iterator_tag>();
    uninitialized_value_construct_n_exception_sender_test<
        std::forward_iterator_tag>();

    uninitialized_value_construct_n_bad_alloc_sender_tests<
        std::random_access_iterator_tag>();
    uninitialized_value_construct_n_bad_alloc_sender_tests<
        std::forward_iterator_tag>();

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
