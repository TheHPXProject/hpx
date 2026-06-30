//  Copyright (c) 2014-2020 Hartmut Kaiser
//  Copyright (c)      2024 Tobias Wukovitsch
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/algorithm.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/numeric.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>

#include "test_utils.hpp"

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_transform_reduce_sender(
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
    std::iota(std::begin(c), std::end(c), std::rand());

    using result_type = hpx::tuple<std::size_t, std::size_t>;

    using hpx::get;
    using hpx::make_tuple;

    auto reduce_op = [](result_type v1, result_type v2) -> result_type {
        return make_tuple(get<0>(v1) * get<0>(v2), get<1>(v1) * get<1>(v2));
    };

    auto convert_op = [](std::size_t val) -> result_type {
        return make_tuple(val, val);
    };

    result_type const init = make_tuple(std::size_t(1), std::size_t(1));

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    {
        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c)), iterator(std::end(c)), init,
                reduce_op, convert_op) |
            hpx::transform_reduce(ex_policy.on(exec)));
        result_type result = hpx::get<0>(*snd_result);

        // verify values
        result_type expected = std::accumulate(std::begin(c), std::end(c), init,
            [&reduce_op, &convert_op](result_type res, std::size_t val) {
                return reduce_op(res, convert_op(val));
            });

        HPX_TEST_EQ(get<0>(result), get<0>(expected));
        HPX_TEST_EQ(get<1>(result), get<1>(expected));
    }

    {
        // edge case: empty range

        auto snd_result = tt::sync_wait(
            ex::just(iterator(std::begin(c)), iterator(std::begin(c)), init,
                reduce_op, convert_op) |
            hpx::transform_reduce(ex_policy.on(exec)));
        result_type result = hpx::get<0>(*snd_result);

        // verify values
        result_type expected = std::accumulate(std::begin(c), std::begin(c),
            init, [&reduce_op, &convert_op](result_type res, std::size_t val) {
                return reduce_op(res, convert_op(val));
            });

        HPX_TEST(result == init);
        HPX_TEST_EQ(get<0>(result), get<0>(expected));
        HPX_TEST_EQ(get<1>(result), get<1>(expected));
    }
}

template <typename IteratorTag>
void transform_reduce_sender_test()
{
    using namespace hpx::execution;
    test_transform_reduce_sender(hpx::launch::sync, seq(task), IteratorTag());
    test_transform_reduce_sender(hpx::launch::sync, unseq(task), IteratorTag());

    test_transform_reduce_sender(hpx::launch::async, par(task), IteratorTag());
    test_transform_reduce_sender(
        hpx::launch::async, par_unseq(task), IteratorTag());
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    transform_reduce_sender_test<std::forward_iterator_tag>();
    transform_reduce_sender_test<std::random_access_iterator_tag>();

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
