//  Copyright (c) 2026 Dominic Marcello
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/testing.hpp>

#include <stdexec/execution.hpp>

#include <memory>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <variant>

struct stopped_receiver
{
    using receiver_concept = stdexec::receiver_t;
    bool& stopped;

    void set_stopped() noexcept
    {
        stopped = true;
    }
};

// Exercise host-side senders in a CUDA translation unit. No GPU is needed to
// run this test; nvcc must parse and instantiate the sender machinery correctly.
int main()
{
    int calls = 0;
    auto value = stdexec::sync_wait(
        stdexec::just(41) | stdexec::then([&calls](int input) {
            ++calls;
            return input + 1;
        }));
    HPX_TEST(value.has_value());
    if (value)
    {
        HPX_TEST_EQ(std::get<0>(*value), 42);
    }
    HPX_TEST_EQ(calls, 1);

    auto moved = stdexec::sync_wait(stdexec::just(std::make_unique<int>(17)) |
        stdexec::then([](std::unique_ptr<int> input) { return *input; }));
    HPX_TEST(moved.has_value());
    if (moved)
    {
        HPX_TEST_EQ(std::get<0>(*moved), 17);
    }

    auto multiple = stdexec::sync_wait(stdexec::just(3, 4) |
        stdexec::then([](int left, int right) { return left + right; }));
    HPX_TEST(multiple.has_value());
    if (multiple)
    {
        HPX_TEST_EQ(std::get<0>(*multiple), 7);
    }

    auto variant = stdexec::sync_wait_with_variant(stdexec::just(9));
    HPX_TEST(variant.has_value());
    if (variant)
    {
        HPX_TEST_EQ(std::get<0>(std::get<0>(*variant)), 9);
    }

    bool stopped = false;
    auto stopped_operation =
        stdexec::connect(stdexec::just_stopped(), stopped_receiver{stopped});
    stdexec::start(stopped_operation);
    HPX_TEST(stopped);
    auto recovered = stdexec::sync_wait(
        stdexec::just_stopped() | stdexec::upon_stopped([] { return 23; }));
    HPX_TEST(recovered.has_value());
    if (recovered)
    {
        HPX_TEST_EQ(std::get<0>(*recovered), 23);
    }

    bool caught = false;
    try
    {
        stdexec::sync_wait(stdexec::just() | stdexec::then([]() -> int {
            throw std::runtime_error("expected sender error");
        }));
    }
    catch (std::runtime_error const&)
    {
        caught = true;
    }
    HPX_TEST(caught);

    return hpx::util::report_errors();
}
