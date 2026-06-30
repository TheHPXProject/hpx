//  Copyright (c) 2015 Daniel Bourgeois
//  Copyright (c) 2024 Tobias Wukovitsch
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <vector>

#include "test_utils.hpp"

////////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);
std::uniform_int_distribution<> dis(0, 99);

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_is_partitioned_sender(
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
    //fill first half of array with even numbers and second half
    //with odd numbers
    std::fill(std::begin(c),
        std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2),
        2 * (dis(gen)));
    std::fill(std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2),
        std::end(c), 2 * (dis(gen)) + 1);

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    {
        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c)), iterator(std::end(c)),
                [](std::size_t n) { return n % 2 == 0; }) |
            hpx::is_partitioned(ex_policy.on(exec)));

        bool parted = hpx::get<0>(*snd_result);

        HPX_TEST(parted);
    }

    {
        // edge case: empty range
        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c)), iterator(std::begin(c)),
                [](std::size_t) { return true; }) |
            hpx::is_partitioned(ex_policy.on(exec)));

        auto parted = hpx::get<0>(*snd_result);

        HPX_TEST(parted);
    }

    {
        // edge case: range of length 1
        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c)), iterator(++std::begin(c)),
                [](std::size_t) { return true; }) |
            hpx::is_partitioned(ex_policy.on(exec)));

        auto parted = hpx::get<0>(*snd_result);

        HPX_TEST(parted);
    }
}

template <typename IteratorTag>
void is_partitioned_sender_test()
{
    using namespace hpx::execution;
    test_is_partitioned_sender(hpx::launch::sync, seq(task), IteratorTag());
    test_is_partitioned_sender(hpx::launch::sync, unseq(task), IteratorTag());

    test_is_partitioned_sender(hpx::launch::async, par(task), IteratorTag());
    test_is_partitioned_sender(
        hpx::launch::async, par_unseq(task), IteratorTag());
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    is_partitioned_sender_test<std::forward_iterator_tag>();
    is_partitioned_sender_test<std::random_access_iterator_tag>();

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
