//  Copyright (c) 2018 Christopher Ogle
//  Copyright (c) 2020 Hartmut Kaiser
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
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "test_utils.hpp"

unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);

////////////////////////////////////////////////////////////////////////////
template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_ends_with_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<int>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;

    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;
    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    std::uniform_int_distribution<int> dis1(3, 10007);
    auto end1 = dis1(gen);
    std::uniform_int_distribution<int> dis2(2, end1 - 1);
    auto end2 = dis2(gen);

    auto some_ints = std::vector<int>(end1);
    std::iota(some_ints.begin(), some_ints.end(), 1);
    auto some_more_ints = std::vector<int>(end1 - end2 + 1);
    std::iota(some_more_ints.begin(), some_more_ints.end(), end2);
    auto some_wrong_ints = std::vector<int>(end2);
    std::iota(some_wrong_ints.begin(), some_wrong_ints.end(), 1);

    {
        auto snd_result =
            tt::sync_wait(ex::just(iterator(std::begin(some_ints)),
                              iterator(std::end(some_ints)),
                              iterator(std::begin(some_more_ints)),
                              iterator(std::end(some_more_ints))) |
                hpx::ends_with(ex_policy.on(exec)));

        bool result = hpx::get<0>(*snd_result);
        HPX_TEST(result);
    }

    {
        auto snd_result =
            tt::sync_wait(ex::just(iterator(std::begin(some_ints)),
                              iterator(std::end(some_ints)),
                              iterator(std::begin(some_wrong_ints)),
                              iterator(std::end(some_wrong_ints))) |
                hpx::ends_with(ex_policy.on(exec)));

        bool result = hpx::get<0>(*snd_result);
        HPX_TEST(!result);
    }

    {
        // edge case: second ranger large than the first

        auto snd_result =
            tt::sync_wait(ex::just(iterator(std::begin(some_more_ints)),
                              iterator(std::end(some_more_ints)),
                              iterator(std::begin(some_ints)),
                              iterator(std::end(some_ints))) |
                hpx::ends_with(ex_policy.on(exec)));

        bool result = hpx::get<0>(*snd_result);
        HPX_TEST(!result);
    }
}

template <typename IteratorTag>
void ends_with_sender_test()
{
    using namespace hpx::execution;
    test_ends_with_sender(hpx::launch::sync, seq(task), IteratorTag());
    test_ends_with_sender(hpx::launch::sync, unseq(task), IteratorTag());

    test_ends_with_sender(hpx::launch::async, par(task), IteratorTag());
    test_ends_with_sender(hpx::launch::async, par_unseq(task), IteratorTag());
}

void ends_with_test()
{
    ends_with_sender_test<std::random_access_iterator_tag>();
    ends_with_sender_test<std::forward_iterator_tag>();
}

////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    ends_with_test();
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
