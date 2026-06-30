//  Copyright (c) 2014-2020 Hartmut Kaiser
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

///////////////////////////////////////////////////////////////////////////////
unsigned int seed = std::random_device{}();
std::mt19937 gen(seed);

///////////////////////////////////////////////////////////////////////////////
template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_includes1_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    std::vector<std::size_t> c1(10007);
    std::uniform_int_distribution<> dis(0, static_cast<int>(c1.size() - 1));
    std::size_t start = dis(gen);
    std::uniform_int_distribution<> dist(
        0, static_cast<int>(c1.size() - start - 1));
    std::size_t end = start + dist(gen);

    std::size_t first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);

    HPX_TEST_LTE(start, end);

    base_iterator start_it =
        std::next(std::begin(c1), static_cast<std::ptrdiff_t>(start));
    base_iterator end_it =
        std::next(std::begin(c1), static_cast<std::ptrdiff_t>(end));

    {
        auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

        auto snd_result =
            tt::sync_wait(ex::just(iterator(std::begin(c1)),
                              iterator(std::end(c1)), start_it, end_it) |
                hpx::includes(ex_policy.on(exec)));

        bool result = hpx::get<0>(*snd_result);

        bool expected =
            std::includes(std::begin(c1), std::end(c1), start_it, end_it);

        // verify values
        HPX_TEST_EQ(result, expected);
    }

    {
        // make sure std::less is not violated by incrementing one of the
        // elements
        std::transform(std::begin(c1), std::end(c1), std::begin(c1),
            [](std::size_t val) { return 2 * val; });

        std::vector<std::size_t> c2;
        std::copy(start_it, end_it, std::back_inserter(c2));

        if (!c2.empty())
        {
            std::uniform_int_distribution<> dis(
                0, static_cast<int>(c2.size() - 1));
            ++c2[dis(gen)];    //-V104

            auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

            auto snd_result = tt::sync_wait(
                ex::just(iterator(std::begin(c1)), iterator(std::end(c1)),
                    std::begin(c2), std::end(c2)) |
                hpx::includes(ex_policy.on(exec)));

            bool result = hpx::get<0>(*snd_result);

            bool expected = std::includes(
                std::begin(c1), std::end(c1), std::begin(c2), std::end(c2));

            // verify values
            HPX_TEST_EQ(result, expected);
        }
    }
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_includes2_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    std::vector<std::size_t> c1(10007);
    std::size_t first_value = gen();    //-V101
    std::iota(std::begin(c1), std::end(c1), first_value);

    std::uniform_int_distribution<> dis(0, static_cast<int>(c1.size() - 1));
    std::size_t start = dis(gen);
    std::uniform_int_distribution<> dist(
        0, static_cast<int>(c1.size() - start - 1));
    std::size_t end = start + dist(gen);

    HPX_TEST_LTE(start, end);

    base_iterator start_it =
        std::next(std::begin(c1), static_cast<std::ptrdiff_t>(start));
    base_iterator end_it =
        std::next(std::begin(c1), static_cast<std::ptrdiff_t>(end));

    {
        auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c1)), iterator(std::end(c1)), start_it,
                end_it, std::less<std::size_t>()) |
            hpx::includes(ex_policy.on(exec)));

        bool result = hpx::get<0>(*snd_result);

        bool expected = std::includes(std::begin(c1), std::end(c1), start_it,
            end_it, std::less<std::size_t>());

        // verify values
        HPX_TEST_EQ(result, expected);
    }

    {
        // make sure std::less is not violated by incrementing one of the
        // elements
        std::transform(std::begin(c1), std::end(c1), std::begin(c1),
            [](std::size_t val) { return 2 * val; });

        std::vector<std::size_t> c2;
        std::copy(start_it, end_it, std::back_inserter(c2));

        if (!c2.empty())
        {
            std::uniform_int_distribution<> dis(
                0, static_cast<int>(c2.size() - 1));
            ++c2[dis(gen)];    //-V104

            auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

            auto snd_result = tt::sync_wait(
                ex::just(iterator(std::begin(c1)), iterator(std::end(c1)),
                    std::begin(c2), std::end(c2), std::less<std::size_t>()) |
                hpx::includes(ex_policy.on(exec)));

            bool result = hpx::get<0>(*snd_result);

            bool expected = std::includes(std::begin(c1), std::end(c1),
                std::begin(c2), std::end(c2), std::less<std::size_t>());

            // verify values
            HPX_TEST_EQ(result, expected);
        }
    }
}

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_includes_edge_cases_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
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
    std::size_t first_value = gen();    //-V101
    std::iota(std::begin(c), std::end(c), first_value);

    {
        // only first range empty

        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c)), iterator(std::begin(c)),
                std::begin(c), std::end(c), std::less<std::size_t>{}) |
            hpx::includes(ex_policy.on(exec)));

        bool result = hpx::get<0>(*snd_result);

        bool expected = std::includes(std::begin(c), std::begin(c),
            std::begin(c), std::end(c), std::less<std::size_t>{});

        HPX_TEST(!result);
        HPX_TEST_EQ(result, expected);
    }

    {
        // only second range empty

        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c)), iterator(std::end(c)),
                std::begin(c), std::begin(c), std::less<std::size_t>{}) |
            hpx::includes(ex_policy.on(exec)));

        bool result = hpx::get<0>(*snd_result);

        bool expected = std::includes(std::begin(c), std::end(c), std::begin(c),
            std::begin(c), std::less<std::size_t>{});

        HPX_TEST(result);
        HPX_TEST_EQ(result, expected);
    }

    {
        // both ranges empty

        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c)), iterator(std::begin(c)),
                std::begin(c), std::begin(c), std::less<std::size_t>{}) |
            hpx::includes(ex_policy.on(exec)));

        bool result = hpx::get<0>(*snd_result);

        bool expected = std::includes(std::begin(c), std::begin(c),
            std::begin(c), std::begin(c), std::less<std::size_t>{});

        HPX_TEST(result);
        HPX_TEST_EQ(result, expected);
    }
}

template <typename IteratorTag>
void includes_sender_test1()
{
    using namespace hpx::execution;
    test_includes1_sender(hpx::launch::sync, seq(task), IteratorTag());
    test_includes1_sender(hpx::launch::sync, unseq(task), IteratorTag());

    test_includes1_sender(hpx::launch::async, par(task), IteratorTag());
    test_includes1_sender(hpx::launch::async, par_unseq(task), IteratorTag());
}

template <typename IteratorTag>
void includes_sender_test2()
{
    using namespace hpx::execution;
    test_includes2_sender(hpx::launch::sync, seq(task), IteratorTag());
    test_includes2_sender(hpx::launch::sync, unseq(task), IteratorTag());

    test_includes2_sender(hpx::launch::async, par(task), IteratorTag());
    test_includes2_sender(hpx::launch::async, par_unseq(task), IteratorTag());
}

template <typename IteratorTag>
void includes_sender_test_edge_cases()
{
    using namespace hpx::execution;
    test_includes_edge_cases_sender(
        hpx::launch::sync, seq(task), IteratorTag());
    test_includes_edge_cases_sender(
        hpx::launch::sync, unseq(task), IteratorTag());

    test_includes_edge_cases_sender(
        hpx::launch::async, par(task), IteratorTag());
    test_includes_edge_cases_sender(
        hpx::launch::async, par_unseq(task), IteratorTag());
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    gen.seed(seed);

    includes_sender_test1<std::forward_iterator_tag>();
    includes_sender_test1<std::random_access_iterator_tag>();

    includes_sender_test2<std::forward_iterator_tag>();
    includes_sender_test2<std::random_access_iterator_tag>();

    includes_sender_test_edge_cases<std::forward_iterator_tag>();
    includes_sender_test_edge_cases<std::random_access_iterator_tag>();

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
