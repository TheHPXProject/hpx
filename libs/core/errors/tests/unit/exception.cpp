//  Copyright (c) 2019 Thomas Heller
//  Copyright (c) 2017 Agustin Berge
//  Copyright (c) 2017 Google
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/errors.hpp>
#include <hpx/modules/testing.hpp>

#include <exception>
#include <thread>

void throw_always()
{
    HPX_THROW_EXCEPTION(
        hpx::error::no_success, "throw_always", "simulated error");
}

std::exception_ptr test_transport()
{
    std::exception_ptr ptr;
    try
    {
        throw_always();
    }
    catch (...)
    {
        ptr = std::current_exception();
        HPX_TEST_EQ(
            hpx::get_error_what(ptr), "simulated error: HPX(no_success)");
        HPX_TEST_EQ(hpx::get_error_function_name(ptr), "throw_always");
    }

    return ptr;
}

int main()
{
    bool exception_caught = false;

    try
    {
        throw_always();
    }
    catch (...)
    {
        exception_caught = true;
        auto ptr = std::current_exception();
        HPX_TEST_EQ(
            hpx::get_error_what(ptr), "simulated error: HPX(no_success)");
        HPX_TEST_EQ(hpx::get_error_function_name(ptr), "throw_always");
    }
    HPX_TEST(exception_caught);

    exception_caught = false;
    try
    {
        throw_always();
    }
    catch (hpx::exception& e)
    {
        exception_caught = true;
        HPX_TEST_EQ(hpx::get_error_what(e), "simulated error: HPX(no_success)");
        HPX_TEST_EQ(hpx::get_error_function_name(e), "throw_always");
    }
    HPX_TEST(exception_caught);

    exception_caught = false;
    try
    {
        throw_always();
    }
    catch (hpx::exception_info& e)
    {
        exception_caught = true;
        HPX_TEST_EQ(hpx::get_error_what(e), "simulated error: HPX(no_success)");
        HPX_TEST_EQ(hpx::get_error_function_name(e), "throw_always");
    }
    HPX_TEST(exception_caught);

    {
        std::exception_ptr ptr = test_transport();
        HPX_TEST(ptr);
        HPX_TEST_EQ(
            hpx::get_error_what(ptr), "simulated error: HPX(no_success)");
        HPX_TEST_EQ(hpx::get_error_function_name(ptr), "throw_always");
    }

    {
        std::exception_ptr ptr;
        std::thread t([&ptr]() { ptr = test_transport(); });
        t.join();
        HPX_TEST(ptr);
        HPX_TEST_EQ(
            hpx::get_error_what(ptr), "simulated error: HPX(no_success)");
        HPX_TEST_EQ(hpx::get_error_function_name(ptr), "throw_always");
    }

    // an error embedding the message of another one with the same code shows
    // the code once, a different code is kept
    {
        std::exception_ptr const inner = test_transport();

        exception_caught = false;
        try
        {
            HPX_THROW_EXCEPTION(hpx::error::no_success, "wrap_same",
                "wrapped: {}", hpx::get_error_what(inner));
        }
        catch (hpx::exception const& e)
        {
            exception_caught = true;
            HPX_TEST_EQ(hpx::get_error_what(e),
                "wrapped: simulated error: HPX(no_success)");
        }
        HPX_TEST(exception_caught);

        exception_caught = false;
        try
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter, "wrap_other",
                "wrapped: {}", hpx::get_error_what(inner));
        }
        catch (hpx::exception const& e)
        {
            exception_caught = true;
            HPX_TEST_EQ(hpx::get_error_what(e),
                "wrapped: simulated error: HPX(no_success): "
                "HPX(bad_parameter)");
        }
        HPX_TEST(exception_caught);

        // the same code elsewhere in the message is the caller's and is kept
        exception_caught = false;
        try
        {
            HPX_THROW_EXCEPTION(hpx::error::no_success, "interior",
                "remote status: HPX(no_success); local status: ok");
        }
        catch (hpx::exception const& e)
        {
            exception_caught = true;
            HPX_TEST_EQ(hpx::get_error_what(e),
                "remote status: HPX(no_success); local status: ok: "
                "HPX(no_success)");
        }
        HPX_TEST(exception_caught);
    }

    return hpx::util::report_errors();
}
