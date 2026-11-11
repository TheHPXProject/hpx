//  Copyright (c) 2026 Shivansh Singh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <exception>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace ex = hpx::execution::experimental;

void test_typed_error()
{
    // P2300 sender that completes with non-exception error (int 42)
    auto snd = ex::just_error(42);

    // Bridge this sender to an hpx::future via as_future
    hpx::future<int> f = ex::as_future<int>(std::move(snd));

    bool caught_error = false;
    try
    {
        (void) f.get();
    }
    catch (int e)
    {
        HPX_TEST_EQ(e, 42);
        caught_error = true;
    }
    catch (...)
    {
        HPX_TEST_MSG(false, "Caught unexpected exception type");
    }

    HPX_TEST(caught_error);
}

void test_error_code()
{
    auto ec = std::make_error_code(std::errc::invalid_argument);
    auto snd = ex::just_error(ec);
    hpx::future<int> f = ex::as_future<int>(std::move(snd));

    bool caught_error = false;
    try
    {
        (void) f.get();
    }
    catch (std::system_error const& e)
    {
        HPX_TEST_EQ(e.code(), ec);
        caught_error = true;
    }
    catch (...)
    {
        HPX_TEST_MSG(false, "Caught unexpected exception type");
    }

    HPX_TEST(caught_error);
}

void test_exception_ptr()
{
    auto ep = std::make_exception_ptr(std::runtime_error("test error"));
    auto snd = ex::just_error(ep);
    hpx::future<int> f = ex::as_future<int>(std::move(snd));

    bool caught_error = false;
    try
    {
        (void) f.get();
    }
    catch (std::runtime_error const& e)
    {
        HPX_TEST_EQ(std::string(e.what()), std::string("test error"));
        caught_error = true;
    }
    catch (...)
    {
        HPX_TEST_MSG(false, "Caught unexpected exception type");
    }

    HPX_TEST(caught_error);
}

void test_error_code_void()
{
    auto ec = std::make_error_code(std::errc::permission_denied);
    auto snd = ex::just_error(ec);
    hpx::future<void> f = ex::as_future<void>(std::move(snd));

    bool caught_error = false;
    try
    {
        f.get();
    }
    catch (std::system_error const& e)
    {
        HPX_TEST_EQ(e.code(), ec);
        caught_error = true;
    }
    catch (...)
    {
        HPX_TEST_MSG(false, "Caught unexpected exception type");
    }

    HPX_TEST(caught_error);
}

void test_make_future_typed_error()
{
    auto snd = ex::just_error(42);
    auto f = ex::make_future(std::move(snd));

    bool caught_error = false;
    try
    {
        f.get();
    }
    catch (int e)
    {
        HPX_TEST_EQ(e, 42);
        caught_error = true;
    }
    catch (...)
    {
        HPX_TEST_MSG(false, "Caught unexpected exception type");
    }

    HPX_TEST(caught_error);
}

void test_make_future_error_code()
{
    auto ec = std::make_error_code(std::errc::invalid_argument);
    auto snd = ex::just_error(ec);
    auto f = ex::make_future(std::move(snd));

    bool caught_error = false;
    try
    {
        f.get();
    }
    catch (std::system_error const& e)
    {
        HPX_TEST_EQ(e.code(), ec);
        caught_error = true;
    }
    catch (...)
    {
        HPX_TEST_MSG(false, "Caught unexpected exception type");
    }

    HPX_TEST(caught_error);
}

void test_make_future_exception_ptr()
{
    auto ep = std::make_exception_ptr(std::runtime_error("test error"));
    auto snd = ex::just_error(ep);
    auto f = ex::make_future(std::move(snd));

    bool caught_error = false;
    try
    {
        f.get();
    }
    catch (std::runtime_error const& e)
    {
        HPX_TEST_EQ(std::string(e.what()), std::string("test error"));
        caught_error = true;
    }
    catch (...)
    {
        HPX_TEST_MSG(false, "Caught unexpected exception type");
    }

    HPX_TEST(caught_error);
}

void test_make_future_error_code_void()
{
    auto ec = std::make_error_code(std::errc::permission_denied);
    auto snd = ex::just_error(ec);
    auto f = ex::make_future(std::move(snd));

    bool caught_error = false;
    try
    {
        f.get();
    }
    catch (std::system_error const& e)
    {
        HPX_TEST_EQ(e.code(), ec);
        caught_error = true;
    }
    catch (...)
    {
        HPX_TEST_MSG(false, "Caught unexpected exception type");
    }

    HPX_TEST(caught_error);
}

int hpx_main()
{
    test_typed_error();
    test_error_code();
    test_exception_ptr();
    test_error_code_void();

    test_make_future_typed_error();
    test_make_future_error_code();
    test_make_future_exception_ptr();
    test_make_future_error_code_void();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
