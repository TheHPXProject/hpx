//  Copyright (c) 2026 Dominic Marcello
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/init.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/testing.hpp>

#include <array>
#include <atomic>
#include <stdexcept>
#include <string>
#include <utility>

namespace ex = hpx::execution::experimental;

// Exercise host-side HPX senders in a CUDA translation unit. The test needs
// an HPX thread pool, but does not require a GPU.
void test_sender_chain()
{
    constexpr int count = 37;
    std::array<std::atomic<int>, count> visits{};
    std::atomic<int> completions{0};

    ex::unique_any_sender<> sender{ex::just()};
    sender = std::move(sender) | ex::transfer(ex::thread_pool_scheduler{}) |
        ex::bulk(count, [&visits](int index) { ++visits[index]; }) |
        ex::then([&completions]() { ++completions; }) | ex::ensure_started();

    auto result = hpx::this_thread::experimental::sync_wait(std::move(sender));
    HPX_TEST(result.has_value());
    HPX_TEST_EQ(completions.load(), 1);
    for (auto const& visit : visits)
    {
        HPX_TEST_EQ(visit.load(), 1);
    }
}

void test_sender_error()
{
    bool caught = false;
    try
    {
        ex::unique_any_sender<> sender{ex::just()};
        sender = std::move(sender) | ex::transfer(ex::thread_pool_scheduler{}) |
            ex::then(
                []() { throw std::runtime_error("expected sender error"); }) |
            ex::ensure_started();
        hpx::this_thread::experimental::sync_wait(std::move(sender));
    }
    catch (std::runtime_error const& error)
    {
        caught = true;
        HPX_TEST_EQ(std::string(error.what()), "expected sender error");
    }
    HPX_TEST(caught);
}

int hpx_main()
{
    test_sender_chain();
    test_sender_error();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
