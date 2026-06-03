//  Copyright (c) 2007-2024 Hartmut Kaiser
//  Copyright (c) 2024 Tobias Wukovitsch
//  Copyright (c) 2026 Siddharth
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
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>
#include <vector>

#include "test_utils.hpp"

constexpr std::size_t arr_size = 10007;

template <typename LnPolicy, typename ExPolicy, typename IteratorTag>
void test_shift_right_sender(
    LnPolicy ln_policy, ExPolicy&& ex_policy, IteratorTag)
{
    static_assert(hpx::is_async_execution_policy_v<ExPolicy>,
        "hpx::is_async_execution_policy_v<ExPolicy>");

    using base_iterator = std::vector<std::size_t>::iterator;
    using iterator = test::test_iterator<base_iterator, IteratorTag>;

    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using scheduler_t = ex::thread_pool_policy_scheduler<LnPolicy>;

    std::vector<std::size_t> c(arr_size);
    std::iota(std::begin(c), std::end(c), std::rand());
    std::vector<std::size_t> d = c;

    auto exec = ex::explicit_scheduler_executor(scheduler_t(ln_policy));

    // verify shift by 0 is a no-op and returns first
    auto result_zero =
        tt::sync_wait(ex::just(iterator(std::begin(c)), iterator(std::end(c)),
                          std::size_t(0)) |
            hpx::shift_right(ex_policy.on(exec)));
    HPX_TEST(std::equal(std::begin(c), std::end(c), std::begin(d)));
    HPX_TEST(hpx::get<0>(result_zero) == iterator(std::begin(c)));

    std::size_t n =
        (static_cast<std::size_t>(std::rand()) % (arr_size - 1)) + 1;

    tt::sync_wait(ex::just(iterator(std::begin(c)), iterator(std::end(c)), n) |
        hpx::shift_right(ex_policy.on(exec)));

    std::move_backward(std::begin(d),
        std::end(d) - static_cast<std::ptrdiff_t>(n), std::end(d));

    // verify values
    HPX_TEST(std::equal(std::begin(c) + static_cast<std::ptrdiff_t>(n),
        std::end(c), std::begin(d) + static_cast<std::ptrdiff_t>(n)));

    // ensure shift by more than the range length returns last
    auto result_over =
        tt::sync_wait(ex::just(iterator(std::begin(c)), iterator(std::end(c)),
                          static_cast<std::size_t>(arr_size + 1)) |
            hpx::shift_right(ex_policy.on(exec)));
    HPX_TEST(hpx::get<0>(result_over) == iterator(std::end(c)));
}

template <typename IteratorTag>
void shift_right_sender_test()
{
    using namespace hpx::execution;
    test_shift_right_sender(hpx::launch::sync, seq(task), IteratorTag());
    test_shift_right_sender(hpx::launch::sync, unseq(task), IteratorTag());

    test_shift_right_sender(hpx::launch::async, par(task), IteratorTag());
    test_shift_right_sender(hpx::launch::async, par_unseq(task), IteratorTag());
}

int hpx_main(hpx::program_options::variables_map& vm)
{
    unsigned int seed = (unsigned int) std::time(nullptr);
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::srand(seed);

    shift_right_sender_test<std::forward_iterator_tag>();
    shift_right_sender_test<std::random_access_iterator_tag>();

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
