//  Copyright (c) 2014 Grant Mercer
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
#include <string>
#include <vector>

#include "test_utils.hpp"

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_search_sender(LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    std::vector<std::size_t> c(10007);
    // fill vector with random values above 2
    std::fill(std::begin(c), std::end(c), (std::rand() % 100) + 3);
    // create subsequence in middle of vector
    c[c.size() / 2] = 1;
    c[c.size() / 2 + 1] = 2;

    std::vector<std::size_t> h{1, 2};

    {
        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c)), iterator(std::end(c)),
                std::begin(h), std::end(h)) |
            hpx::search(ex_policy.on(exec)));
        iterator index = hpx::get<0>(*snd_result);

        base_iterator test_index =
            std::begin(c) + static_cast<std::ptrdiff_t>(c.size() / 2);

        HPX_TEST(index == iterator(test_index));
    }

    {
        // edge case: only second range is empty

        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c)), iterator(std::end(c)),
                std::begin(h), std::begin(h)) |
            hpx::search(ex_policy.on(exec)));
        auto result = hpx::get<0>(*snd_result);

        auto expected = std::search(iterator(std::begin(c)),
            iterator(std::end(c)), std::begin(h), std::begin(h));

        HPX_TEST(result.base() == std::begin(c));
        HPX_TEST(result == expected);
    }

    {
        // edge case: both ranges are empty

        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c)), iterator(std::begin(c)),
                std::begin(h), std::begin(h)) |
            hpx::search(ex_policy.on(exec)));
        auto result = hpx::get<0>(*snd_result);

        auto expected = std::search(iterator(std::begin(c)),
            iterator(std::begin(c)), std::begin(h), std::begin(h));

        HPX_TEST(result.base() == std::begin(c));
        HPX_TEST(result == expected);
    }

    {
        // edge case: second range is larger than the first range

        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(h)), iterator(std::end(h)),
                std::begin(c), std::end(c)) |
            hpx::search(ex_policy.on(exec)));
        auto result = hpx::get<0>(*snd_result);

        auto expected = std::search(iterator(std::begin(h)),
            iterator(std::end(h)), std::begin(c), std::end(c));

        HPX_TEST(result.base() == std::end(h));
        HPX_TEST(result == expected);
    }
}

template <typename IteratorTag>
void search_sender_test()
{
    using namespace hpx::execution;
    test_search_sender(hpx::launch::sync, seq(task), IteratorTag());
    test_search_sender(hpx::launch::sync, unseq(task), IteratorTag());

    test_search_sender(hpx::launch::async, par(task), IteratorTag());
    test_search_sender(hpx::launch::async, par_unseq(task), IteratorTag());
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    search_sender_test<std::forward_iterator_tag>();
    search_sender_test<std::random_access_iterator_tag>();

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
