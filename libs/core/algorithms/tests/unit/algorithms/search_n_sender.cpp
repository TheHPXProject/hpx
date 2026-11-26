//  Copyright (c) 2026 Divyanshi Talreja
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "sender_algorithm_test_utils.hpp"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <vector>

struct move_sensitive_pred
{
    std::shared_ptr<int> state = std::make_shared<int>(1);

    bool operator()(int lhs, int rhs) const
    {
        return state != nullptr && lhs == rhs;
    }
};

template <typename Policy>
void test(Policy policy)
{
    for (std::size_t n : {0, 1, 17, 10007})
    {
        std::vector<int> data(n, 1);
        auto const first_offset = static_cast<std::ptrdiff_t>(n / 3);
        auto const last_offset = static_cast<std::ptrdiff_t>(2 * n / 3);
        std::fill(data.begin() + first_offset, data.begin() + last_offset, 7);
        for (std::size_t count :
            {std::size_t(0), std::size_t(1), std::size_t(3), n + 1})
        {
            for (int value : {1, 7, 42})
            {
                auto expected =
                    std::search_n(data.begin(), data.end(), count, value);
                auto result = sender_test::wait(hpx::search_n(
                    policy, data.begin(), data.end(), count, value));
                HPX_TEST(hpx::get<0>(*result) == expected);
            }
        }
    }

    std::vector<int> multi_chunk(10007, 1);
    multi_chunk[multi_chunk.size() - 2] = 7;
    multi_chunk.back() = 7;
    auto result = sender_test::wait(hpx::search_n(policy, multi_chunk.begin(),
        multi_chunk.end(), 2, 7, move_sensitive_pred{}));
    HPX_TEST(hpx::get<0>(*result) == multi_chunk.end() - 2);
}

void test_deferred_projection()
{
    std::vector<int> data(10007, 1);
    std::atomic<int> projections{0};
    auto projection = [&projections](int value) {
        projections.fetch_add(1, std::memory_order_relaxed);
        return value;
    };
    auto exec = sender_test::ex::explicit_scheduler_executor(
        sender_test::ex::thread_pool_policy_scheduler(hpx::launch::async));
    auto policy = hpx::execution::par(hpx::execution::task).on(exec);

    auto sender = hpx::search_n(policy, data.begin(), data.end(), 2, 1,
        std::equal_to<int>{}, projection);
    HPX_TEST_EQ(projections.load(std::memory_order_relaxed), 0);
    auto result = sender_test::wait(std::move(sender));
    HPX_TEST(hpx::get<0>(*result) == data.begin());
    HPX_TEST(projections.load(std::memory_order_relaxed) > 0);

    projections.store(0, std::memory_order_relaxed);
    auto empty_sender = hpx::search_n(policy, data.begin(), data.end(), 0, 1,
        std::equal_to<int>{}, projection);
    auto empty_result = sender_test::wait(std::move(empty_sender));
    HPX_TEST(hpx::get<0>(*empty_result) == data.begin());
    HPX_TEST_EQ(projections.load(std::memory_order_relaxed), 0);

    auto negative_sender = hpx::search_n(policy, data.begin(), data.end(), -2,
        1, std::equal_to<int>{}, projection);
    auto negative_result = sender_test::wait(std::move(negative_sender));
    HPX_TEST(hpx::get<0>(*negative_result) == data.begin());
    HPX_TEST_EQ(projections.load(std::memory_order_relaxed), 0);
}

void test_deferred_projection_error()
{
    std::vector<int> data{1, 1, 1};
    auto exec = sender_test::ex::explicit_scheduler_executor(
        sender_test::ex::thread_pool_policy_scheduler(hpx::launch::async));
    auto policy = hpx::execution::par(hpx::execution::task).on(exec);

    auto sender = hpx::search_n(policy, data.begin(), data.end(), 2, 1,
        std::equal_to<int>{},
        [](int) -> int { throw std::runtime_error("projection failed"); });
    bool caught = false;
    try
    {
        (void) sender_test::wait(std::move(sender));
    }
    catch (hpx::exception_list const&)
    {
        caught = true;
    }
    HPX_TEST(caught);
}

void test_synchronous_scheduler()
{
    std::vector<int> data{1, 1, 7, 7, 7};
    auto exec = sender_test::ex::explicit_scheduler_executor(
        sender_test::ex::thread_pool_policy_scheduler(hpx::launch::async));
    auto policy = hpx::execution::par.on(exec);

    HPX_TEST(hpx::search_n(policy, data.begin(), data.end(), 2, 7) ==
        data.begin() + 2);
    HPX_TEST(
        hpx::search_n(policy, data.begin(), data.end(), 0, 7) == data.begin());
    HPX_TEST(
        hpx::search_n(policy, data.begin(), data.end(), 6, 7) == data.end());
}

int hpx_main()
{
    sender_test::policies([](auto policy) { test(policy); });
    test_deferred_projection();
    test_deferred_projection_error();
    test_synchronous_scheduler();
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    return sender_test::main(argc, argv, hpx_main);
}
