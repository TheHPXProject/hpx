//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <exception>
#include <string>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
void test_transform()
{
    // value: f returns a non-future, transform wraps it
    auto f = hpx::make_ready_future(42);
    auto f2 = hpx::futures::transform(
        std::move(f), [](int val) { return std::to_string(val); });
    HPX_TEST_EQ(f2.get(), "42");

    // void: f takes no argument
    auto fv = hpx::make_ready_future();
    auto fv2 = hpx::futures::transform(std::move(fv), []() { return 10; });
    HPX_TEST_EQ(fv2.get(), 10);
}

void test_and_then()
{
    // and_then delegates to .then() directly.
    // f receives the future directly (HPX convention for .then()).
    auto f = hpx::make_ready_future(42);
    auto f2 = hpx::futures::and_then(std::move(f),
        [](hpx::future<int> val) { return std::to_string(val.get()); });
    HPX_TEST_EQ(f2.get(), "42");
}

void test_or_else()
{
    // exception path: f receives exception_ptr and returns T directly
    auto f1 = hpx::make_exceptional_future<int>(
        hpx::exception(hpx::error::bad_parameter, "test"));
    auto f2 = hpx::futures::or_else(
        std::move(f1), [](std::exception_ptr const& /*e*/) { return 100; });
    HPX_TEST_EQ(f2.get(), 100);

    // exception path: f takes no argument
    auto f3 = hpx::make_exceptional_future<int>(
        hpx::exception(hpx::error::bad_parameter, "test"));
    auto f4 = hpx::futures::or_else(std::move(f3), []() { return 200; });
    HPX_TEST_EQ(f4.get(), 200);

    // no-exception: value passes through unchanged, f is not called
    auto f5 = hpx::make_ready_future(42);
    auto f6 = hpx::futures::or_else(
        std::move(f5), [](std::exception_ptr const&) { return -1; });
    HPX_TEST_EQ(f6.get(), 42);
}

///////////////////////////////////////////////////////////////////////////////
void test_shared_transform()
{
    hpx::shared_future<int> f = hpx::make_ready_future(42);
    auto f2 =
        hpx::futures::transform(f, [](int val) { return std::to_string(val); });
    HPX_TEST_EQ(f2.get(), "42");
    HPX_TEST_EQ(f.get(), 42);
}

void test_shared_and_then()
{
    hpx::shared_future<int> f = hpx::make_ready_future(42);
    auto f2 = hpx::futures::and_then(f,
        [](hpx::shared_future<int> val) { return std::to_string(val.get()); });
    HPX_TEST_EQ(f2.get(), "42");
}

void test_shared_or_else()
{
    hpx::shared_future<int> f = hpx::make_exceptional_future<int>(
        hpx::exception(hpx::error::bad_parameter, "test"));
    auto f2 =
        hpx::futures::or_else(f, [](std::exception_ptr const&) { return 300; });
    HPX_TEST_EQ(f2.get(), 300);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_transform();
    test_and_then();
    test_or_else();

    test_shared_transform();
    test_shared_and_then();
    test_shared_or_else();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
