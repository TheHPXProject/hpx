//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <chrono>
#include <stdexcept>
#include <utility>
#include <vector>

hpx::future<int> make_future()
{
    return hpx::make_ready_future_after(std::chrono::milliseconds(50), 42);
}

void test_wait_all_for()
{
    // all futures become ready well before the timeout elapses
    {
        auto f1 = make_future();

        auto const [status, has_exceptional_future] =
            hpx::wait_all_for_nothrow(std::chrono::seconds(10), f1);
        HPX_TEST(status == hpx::future_status::ready);
        HPX_TEST(!has_exceptional_future);
        HPX_TEST(f1.is_ready());
    }
    {
        std::vector<hpx::future<int>> future_array;
        future_array.push_back(make_future());
        future_array.push_back(make_future());

        auto const [status, has_exceptional_future] =
            hpx::wait_all_for_nothrow(std::chrono::seconds(10), future_array);
        HPX_TEST(status == hpx::future_status::ready);
        HPX_TEST(!has_exceptional_future);

        for (auto& f : future_array)
        {
            HPX_TEST(f.is_ready() && !f.has_exception());
        }
    }
    {
        auto f1 = make_future();
        auto f2 = make_future();

        auto const [status, has_exceptional_future] =
            hpx::wait_all_for_nothrow(std::chrono::seconds(10), f1, f2);
        HPX_TEST(status == hpx::future_status::ready);
        HPX_TEST(!has_exceptional_future);

        HPX_TEST(f1.is_ready() && !f1.has_exception());
        HPX_TEST(f2.is_ready() && !f2.has_exception());
    }

    {
        std::vector<hpx::future<int>> future_array;
        future_array.push_back(make_future());
        future_array.push_back(make_future());

        auto const [status, has_exceptional_future] = hpx::wait_all_for_nothrow(
            std::chrono::seconds(10), future_array.begin(), future_array.end());
        HPX_TEST(status == hpx::future_status::ready);
        HPX_TEST(!has_exceptional_future);

        for (auto& f : future_array)
        {
            HPX_TEST(f.is_ready() && !f.has_exception());
        }
    }

    // an exceptional future that becomes ready before the timeout elapses
    // does not throw with the _nothrow version...
    {
        std::vector<hpx::future<int>> future_array;
        future_array.push_back(make_future());
        future_array.push_back(
            hpx::make_exceptional_future<int>(std::runtime_error("")));

        bool caught_exception = false;
        try
        {
            auto const [status, has_exceptional_future] =
                hpx::wait_all_for_nothrow(
                    std::chrono::seconds(10), future_array);
            HPX_TEST(status == hpx::future_status::ready);
            HPX_TEST(has_exceptional_future);

            HPX_TEST(
                future_array[0].is_ready() && !future_array[0].has_exception());
            HPX_TEST(
                future_array[1].is_ready() && future_array[1].has_exception());
        }
        catch (...)
        {
            caught_exception = true;
        }
        HPX_TEST(!caught_exception);
    }

    // ...but does rethrow with the throwing version
    {
        std::vector<hpx::future<int>> future_array;
        future_array.push_back(make_future());
        future_array.push_back(
            hpx::make_exceptional_future<int>(std::runtime_error("")));

        bool caught_exception = false;
        try
        {
            hpx::wait_all_for(std::chrono::seconds(10), future_array);
            HPX_TEST(false);
        }
        catch (std::runtime_error const&)
        {
            caught_exception = true;
        }
        catch (...)
        {
            HPX_TEST(false);
        }
        HPX_TEST(caught_exception);
    }
    {
        std::vector<hpx::future<int>> future_array;
        future_array.push_back(make_future());
        future_array.push_back(
            hpx::make_exceptional_future<int>(std::runtime_error("")));

        bool caught_exception = false;
        try
        {
            hpx::wait_all_for(std::chrono::seconds(10), future_array.begin(),
                future_array.end());
            HPX_TEST(false);
        }
        catch (std::runtime_error const&)
        {
            caught_exception = true;
        }
        catch (...)
        {
            HPX_TEST(false);
        }
        HPX_TEST(caught_exception);
    }
    {
        auto f1 = make_future();
        auto f2 = hpx::make_exceptional_future<int>(std::runtime_error(""));

        bool caught_exception = false;
        try
        {
            hpx::wait_all_for(std::chrono::seconds(10), f1, f2);
            HPX_TEST(false);
        }
        catch (std::runtime_error const&)
        {
            caught_exception = true;
        }
        catch (...)
        {
            HPX_TEST(false);
        }
        HPX_TEST(caught_exception);
    }

    // a future that never becomes ready causes wait_all_for to time out
    // instead of hanging indefinitely, while leaving all input futures
    // valid and unaffected
    {
        hpx::promise<int> p;
        hpx::future<int> never_ready = p.get_future();

        auto f1 = make_future();

        auto const [status, has_exceptional_future] = hpx::wait_all_for_nothrow(
            std::chrono::milliseconds(500), f1, never_ready);

        HPX_TEST(status == hpx::future_status::timeout);
        HPX_TEST(!has_exceptional_future);

        HPX_TEST(f1.is_ready() && !f1.has_exception());
        HPX_TEST(!never_ready.is_ready());

        // make sure the never_ready future is still usable afterward
        p.set_value(0);
        HPX_TEST(never_ready.get() == 0);
    }
    {
        hpx::promise<int> p;
        hpx::future<int> never_ready = p.get_future();

        std::vector<hpx::future<int>> future_array;
        future_array.push_back(make_future());
        future_array.push_back(HPX_MOVE(never_ready));

        auto const [status, has_exceptional_future] =
            hpx::wait_all_for_nothrow(std::chrono::milliseconds(200),
                future_array.begin(), future_array.end());

        HPX_TEST(status == hpx::future_status::timeout);
        HPX_TEST(!has_exceptional_future);

        HPX_TEST(future_array[0].is_ready());
        HPX_TEST(!future_array[1].is_ready());

        // make sure the never_ready future is still usable afterward
        p.set_value(0);
        HPX_TEST(future_array[1].get() == 0);
    }
    {
        hpx::promise<int> p;
        hpx::future<int> never_ready = p.get_future();

        bool caught_exception = false;
        try
        {
            HPX_TEST(hpx::wait_all_for(std::chrono::milliseconds(200),
                         never_ready) == hpx::future_status::timeout);
        }
        catch (...)
        {
            caught_exception = true;
        }
        HPX_TEST(!caught_exception);
        HPX_TEST(!never_ready.is_ready());

        p.set_value(0);
    }
}

void test_wait_all_for_n()
{
    {
        std::vector<hpx::future<int>> future_array;
        future_array.push_back(make_future());
        future_array.push_back(make_future());

        auto const [status, has_exceptional_future] =
            hpx::wait_all_for_n_nothrow(std::chrono::seconds(10),
                future_array.begin(), future_array.size());
        HPX_TEST(status == hpx::future_status::ready);
        HPX_TEST(!has_exceptional_future);

        for (auto& f : future_array)
        {
            HPX_TEST(f.is_ready());
        }
    }
    {
        std::vector<hpx::future<int>> future_array;
        future_array.push_back(make_future());
        future_array.push_back(
            hpx::make_exceptional_future<int>(std::runtime_error("")));

        bool caught_exception = false;
        try
        {
            hpx::wait_all_for_n(std::chrono::seconds(10), future_array.begin(),
                future_array.size());
            HPX_TEST(false);
        }
        catch (std::runtime_error const&)
        {
            caught_exception = true;
        }
        catch (...)
        {
            HPX_TEST(false);
        }
        HPX_TEST(caught_exception);
    }
    {
        hpx::promise<int> p;
        hpx::future<int> never_ready = p.get_future();

        std::vector<hpx::future<int>> future_array;
        future_array.push_back(std::move(never_ready));

        auto const [status, has_exceptional_future] =
            hpx::wait_all_for_n_nothrow(std::chrono::milliseconds(200),
                future_array.begin(), future_array.size());

        HPX_TEST(status == hpx::future_status::timeout);
        HPX_TEST(!has_exceptional_future);

        HPX_TEST(!future_array[0].is_ready());
        p.set_value(0);
    }
}

int hpx_main()
{
    test_wait_all_for();
    test_wait_all_for_n();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
