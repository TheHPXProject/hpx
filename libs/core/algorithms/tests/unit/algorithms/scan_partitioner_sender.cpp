//  Copyright (c) 2026 Divyanshi Talreja
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "sender_algorithm_test_utils.hpp"

#include <hpx/modules/algorithms.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <exception>
#include <iterator>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

enum class failure_phase
{
    none,
    first,
    prefix,
    final
};

struct phase_counts
{
    std::atomic<std::size_t> first{0};
    std::atomic<std::size_t> prefix{0};
    std::atomic<std::size_t> final{0};
    std::atomic<std::size_t> reduction{0};
};

template <typename Policy>
auto make_scan_sender(Policy policy, std::vector<int>& data,
    std::shared_ptr<phase_counts> counts,
    failure_phase failure = failure_phase::none)
{
    using partitioner = hpx::parallel::util::scan_partitioner<Policy, int, int>;

    return partitioner::call(
        HPX_MOVE(policy), data.begin(), data.size(), 0,
        [counts, failure](auto first, std::size_t size) {
            ++counts->first;
            if (failure == failure_phase::first)
                throw std::runtime_error("first scan phase failed");
            return std::accumulate(first, std::next(first, size), 0);
        },
        [counts, failure](int left, int right) {
            ++counts->prefix;
            if (failure == failure_phase::prefix)
                throw std::runtime_error("scan prefix phase failed");
            return left + right;
        },
        [counts, failure](auto, std::size_t, int) {
            ++counts->final;
            if (failure == failure_phase::final)
                throw std::runtime_error("final scan phase failed");
        },
        [counts](
            std::vector<int>&& prefixes, std::vector<hpx::future<void>>&&) {
            ++counts->reduction;
            return prefixes.back();
        });
}

void test_deferred_and_independent_operations()
{
    namespace ex = hpx::execution::experimental;
    using namespace hpx::execution;

    auto exec = ex::explicit_scheduler_executor(
        ex::thread_pool_policy_scheduler(hpx::launch::async));
    auto policy = par(task).on(exec).with(ex::num_cores(2));
    auto data = sender_test::values(10007);
    int const expected = std::accumulate(data.begin(), data.end(), 0);
    auto counts = std::make_shared<phase_counts>();

    auto sender = make_scan_sender(policy, data, counts);
    auto sender_copy = sender;
    HPX_TEST_EQ(counts->first.load(), std::size_t(0));
    HPX_TEST_EQ(counts->prefix.load(), std::size_t(0));
    HPX_TEST_EQ(counts->final.load(), std::size_t(0));

    auto first_result = sender_test::wait(HPX_MOVE(sender));
    HPX_TEST_EQ(hpx::get<0>(*first_result), expected);
    std::size_t const first_calls = counts->first.load();
    std::size_t const prefix_calls = counts->prefix.load();
    std::size_t const final_calls = counts->final.load();
    HPX_TEST(first_calls > 1);
    HPX_TEST(prefix_calls != 0);
    HPX_TEST(final_calls != 0);
    HPX_TEST_EQ(counts->reduction.load(), std::size_t(1));

    auto second_result = sender_test::wait(HPX_MOVE(sender_copy));
    HPX_TEST_EQ(hpx::get<0>(*second_result), expected);
    HPX_TEST_EQ(counts->first.load(), 2 * first_calls);
    HPX_TEST_EQ(counts->prefix.load(), 2 * prefix_calls);
    HPX_TEST_EQ(counts->final.load(), 2 * final_calls);
    HPX_TEST_EQ(counts->reduction.load(), std::size_t(2));
}

void test_failure(failure_phase failure)
{
    namespace ex = hpx::execution::experimental;
    namespace tt = hpx::this_thread::experimental;
    using namespace hpx::execution;

    auto exec = ex::explicit_scheduler_executor(
        ex::thread_pool_policy_scheduler(hpx::launch::async));
    auto data = sender_test::values(10007);
    auto counts = std::make_shared<phase_counts>();
    auto sender = make_scan_sender(par(task).on(exec), data, counts, failure);

    try
    {
        (void) tt::sync_wait(HPX_MOVE(sender));
        HPX_TEST(false);
    }
    catch (hpx::exception_list const& errors)
    {
        HPX_TEST(errors.size() != 0);
    }
}

void test_synchronous_scheduler_results()
{
    namespace ex = hpx::execution::experimental;
    using namespace hpx::execution;

    auto exec = ex::explicit_scheduler_executor(
        ex::thread_pool_policy_scheduler(hpx::launch::async));
    std::vector<int> input{1, 1, 2, 2, 3, 3, 4, 4};
    std::vector<int> output(input.size(), -1);

    auto copied = hpx::copy_if(par.on(exec), input.begin(), input.end(),
        output.begin(), [](int value) { return value % 2 == 0; });
    std::vector<int> const even{2, 2, 4, 4};
    HPX_TEST(
        copied == output.begin() + static_cast<std::ptrdiff_t>(even.size()));
    HPX_TEST(std::equal(even.begin(), even.end(), output.begin()));

    std::fill(output.begin(), output.end(), -1);
    auto unique = hpx::unique_copy(
        par.on(exec), input.begin(), input.end(), output.begin());
    std::vector<int> const distinct{1, 2, 3, 4};
    HPX_TEST(unique ==
        output.begin() + static_cast<std::ptrdiff_t>(distinct.size()));
    HPX_TEST(std::equal(distinct.begin(), distinct.end(), output.begin()));
}

void test_task_scheduler_results()
{
    namespace ex = hpx::execution::experimental;
    using namespace hpx::execution;

    auto exec = ex::explicit_scheduler_executor(
        ex::thread_pool_policy_scheduler(hpx::launch::async));
    auto policy = par(task).on(exec);
    for (std::vector<int> const& input : {std::vector<int>{},
             std::vector<int>{1}, std::vector<int>{1, 1, 2, 2, 3, 3, 4, 4}})
    {
        std::vector<int> output(input.size(), -1);
        std::vector<int> expected;
        std::copy_if(input.begin(), input.end(), std::back_inserter(expected),
            [](int value) { return value % 2 == 0; });

        auto copy_sender = hpx::copy_if(policy, input.begin(), input.end(),
            output.begin(), [](int value) { return value % 2 == 0; });
        HPX_TEST(std::all_of(output.begin(), output.end(),
            [](int value) { return value == -1; }));
        auto copy_result = sender_test::wait(HPX_MOVE(copy_sender));
        HPX_TEST(hpx::get<0>(*copy_result) ==
            output.begin() + static_cast<std::ptrdiff_t>(expected.size()));
        HPX_TEST(std::equal(expected.begin(), expected.end(), output.begin()));

        std::fill(output.begin(), output.end(), -1);
        expected.clear();
        std::unique_copy(
            input.begin(), input.end(), std::back_inserter(expected));
        auto unique_sender = hpx::unique_copy(
            policy, input.begin(), input.end(), output.begin());
        HPX_TEST(std::all_of(output.begin(), output.end(),
            [](int value) { return value == -1; }));
        auto unique_result = sender_test::wait(HPX_MOVE(unique_sender));
        HPX_TEST(hpx::get<0>(*unique_result) ==
            output.begin() + static_cast<std::ptrdiff_t>(expected.size()));
        HPX_TEST(std::equal(expected.begin(), expected.end(), output.begin()));
    }
}

int hpx_main()
{
    test_synchronous_scheduler_results();
    test_task_scheduler_results();
    test_deferred_and_independent_operations();
    test_failure(failure_phase::first);
    test_failure(failure_phase::prefix);
    test_failure(failure_phase::final);
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return sender_test::main(argc, argv, hpx_main);
}
