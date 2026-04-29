//  Copyright (c) 2021 ETH Zurich
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include "algorithm_test_utils.hpp"

#include <atomic>
#include <exception>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace ex = hpx::execution::experimental;
namespace tt = hpx::this_thread::experimental;

// NOTE: This is not a conforming sync_wait implementation. It only exists to
// check that the tag_invoke overload is called.
std::optional<std::tuple<>> tag_invoke(tt::sync_wait_t, custom_sender2 s)
{
    s.tag_invoke_overload_called = true;
    return {};
}

// NOLINTBEGIN(bugprone-unchecked-optional-access)
int hpx_main()
{
    // Success path
    {
        std::atomic<bool> start_called{false};
        std::atomic<bool> connect_called{false};
        std::atomic<bool> tag_invoke_overload_called{false};
        tt::sync_wait(custom_sender{
            start_called, connect_called, tag_invoke_overload_called});
        HPX_TEST(start_called);
        HPX_TEST(connect_called);
        HPX_TEST(!tag_invoke_overload_called);
    }

    {
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(ex::just(3))), 3);
    }

    {
        // rvalue arguments
        auto result = *tt::sync_wait(ex::just(3, 4.0));
        HPX_TEST_EQ(hpx::get<0>(result), 3);
        HPX_TEST_EQ(hpx::get<1>(result), 4.0);
    }

    {
        // lvalue arguments
        int i = 3;
        double d = 4.0;
        auto result = *tt::sync_wait(ex::just(i, d));
        HPX_TEST_EQ(hpx::get<0>(result), 3);
        HPX_TEST_EQ(hpx::get<1>(result), 4.0);
    }

    {
        auto result = *tt::sync_wait(ex::just(3, 4.0, std::string("42")));
        HPX_TEST_EQ(hpx::get<0>(result), 3);
        HPX_TEST_EQ(hpx::get<1>(result), 4.0);
        HPX_TEST_EQ(hpx::get<2>(result), std::string("42"));
    }

    {
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(ex::just(
                                    custom_type_non_default_constructible{42})))
                        .x,
            42);
    }

    {
        HPX_TEST_EQ(
            hpx::get<0>(
                *tt::sync_wait(ex::just(
                    custom_type_non_default_constructible_non_copyable{42})))
                .x,
            42);
    }

    // operator| overload

    {
        std::atomic<bool> start_called{false};
        std::atomic<bool> connect_called{false};
        std::atomic<bool> tag_invoke_overload_called{false};
        tt::sync_wait(custom_sender{
            start_called, connect_called, tag_invoke_overload_called});
        HPX_TEST(start_called);
        HPX_TEST(connect_called);
        HPX_TEST(!tag_invoke_overload_called);
    }

    {
        HPX_TEST_EQ(hpx::get<0>(*tt::sync_wait(ex::just(3))), 3);
    }

    // tag_invoke overload
    {
        std::atomic<bool> start_called{false};
        std::atomic<bool> connect_called{false};
        std::atomic<bool> tag_invoke_overload_called{false};
        tt::sync_wait(custom_sender2{custom_sender{
            start_called, connect_called, tag_invoke_overload_called}});
        HPX_TEST(!start_called);
        HPX_TEST(!connect_called);
        HPX_TEST(tag_invoke_overload_called);
    }

    // Failure path
    {
        bool exception_thrown = false;
        try
        {
            tt::sync_wait(error_sender{});
            HPX_TEST(false);
        }
        catch (std::runtime_error const& e)
        {
            HPX_TEST_EQ(std::string(e.what()), std::string("error"));
            exception_thrown = true;
        }
        HPX_TEST(exception_thrown);
    }

    // cancellation path
    {
        auto result = tt::sync_wait(stopped_sender_with_value_type{});
        HPX_TEST(!result);    // returned optional should be empty
    }

    return hpx::local::finalize();
}
// NOLINTEND(bugprone-unchecked-optional-access)

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
