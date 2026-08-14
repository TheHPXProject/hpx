//  Copyright (c) 2014-2020 Hartmut Kaiser
//  Copyright (c) 2024 Tobias Wukovitsch
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/memory.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "test_utils.hpp"

std::atomic<std::size_t> destruct_count(0);
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);

struct destructable
{
    destructable()
      : value_(0)
    {
    }

    ~destructable()
    {
        ++destruct_count;
    }

    std::uint32_t value_;
};

std::size_t const data_size = 10007;

////////////////////////////////////////////////////////////////////////////
template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_destroy_n_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = destructable*;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    destructable* p =
        (destructable*) std::malloc(data_size * sizeof(destructable));

    // value-initialize data in array
    std::for_each(p, p + data_size, [](destructable& d) {
        ::new (static_cast<void*>(std::addressof(d))) destructable;
    });

    destruct_count.store(0);

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    tt::sync_wait(
        ex::just(iterator(p), -1) | hpx::destroy_n(ex_policy.on(exec)));
    HPX_TEST_EQ(destruct_count.load(), static_cast<std::size_t>(0));

    tt::sync_wait(
        ex::just(iterator(p), data_size) | hpx::destroy_n(ex_policy.on(exec)));

    HPX_TEST_EQ(destruct_count.load(), data_size);

    std::free(p);
}

template <typename IteratorTag>
void destroy_n_sender_test()
{
    using namespace hpx::execution;
    test_destroy_n_sender(hpx::launch::sync, seq(task), IteratorTag());
    test_destroy_n_sender(hpx::launch::sync, unseq(task), IteratorTag());

    test_destroy_n_sender(hpx::launch::async, par(task), IteratorTag());
    test_destroy_n_sender(hpx::launch::async, par_unseq(task), IteratorTag());
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    destroy_n_sender_test<std::forward_iterator_tag>();
    destroy_n_sender_test<std::random_access_iterator_tag>();

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
