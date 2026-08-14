//  Copyright (C) 2012-2026 Hartmut Kaiser
//  (C) Copyright 2008-10 Anthony Williams
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
struct X
{
    int i;

    X()
      : i(42)
    {
    }

    X(X&& other)
      : i(other.i)
    {
        other.i = 0;
    }

    ~X() {}
};

int make_int()
{
    return 42;
}

int throw_runtime_error()
{
    throw std::runtime_error("42");
}

void set_promise_thread(hpx::promise<int>* p)
{
    p->set_value(42);
}

struct my_exception
{
};

void set_promise_exception_thread(hpx::promise<int>* p)
{
    p->set_exception(std::make_exception_ptr(my_exception()));
}

///////////////////////////////////////////////////////////////////////////////
void test_store_value_from_thread()
{
    hpx::promise<int> pi2;
    hpx::future<int> fi2(pi2.get_future());
    hpx::thread t(&set_promise_thread, &pi2);
    fi2.wait();
    HPX_TEST(fi2.is_ready());
    HPX_TEST(fi2.has_value());
    HPX_TEST(!fi2.has_exception());
    int j = fi2.get();
    HPX_TEST_EQ(j, 42);
    t.join();
}

///////////////////////////////////////////////////////////////////////////////
void test_store_exception()
{
    hpx::promise<int> pi3;
    hpx::future<int> fi3 = pi3.get_future();
    hpx::thread t(&set_promise_exception_thread, &pi3);
    fi3.wait();

    HPX_TEST(fi3.is_ready());
    HPX_TEST(!fi3.has_value());
    HPX_TEST(fi3.has_exception());
    try
    {
        fi3.get();
        HPX_TEST(false);
    }
    catch (my_exception)
    {
        HPX_TEST(true);
    }
    t.join();
}

///////////////////////////////////////////////////////////////////////////////
void test_initial_state()
{
    hpx::future<int> fi;
    HPX_TEST(!fi.is_ready());
    HPX_TEST(!fi.has_value());
    HPX_TEST(!fi.has_exception());
    try
    {
        fi.get();
        HPX_TEST(false);
    }
    catch (hpx::exception const& e)
    {
        HPX_TEST_EQ(e.get_error(), hpx::error::no_state);
    }
    catch (...)
    {
        HPX_TEST(false);
    }
}

///////////////////////////////////////////////////////////////////////////////
void test_waiting_future()
{
    hpx::promise<int> pi;
    hpx::future<int> fi;
    fi = pi.get_future();

    HPX_TEST(!fi.is_ready());
    HPX_TEST(!fi.has_value());
    HPX_TEST(!fi.has_exception());

    // fulfill the promise so the destructor of promise is happy.
    pi.set_value(0);
}

///////////////////////////////////////////////////////////////////////////////
void test_cannot_get_future_twice()
{
    hpx::promise<int> pi;
    pi.get_future();

    try
    {
        pi.get_future();
        HPX_TEST(false);
    }
    catch (hpx::exception const& e)
    {
        HPX_TEST_EQ(e.get_error(), hpx::error::future_already_retrieved);
    }
    catch (...)
    {
        HPX_TEST(false);
    }
}

///////////////////////////////////////////////////////////////////////////////
void test_set_value_updates_future_status()
{
    hpx::promise<int> pi;
    hpx::future<int> fi;
    fi = pi.get_future();

    pi.set_value(42);

    HPX_TEST(fi.is_ready());
    HPX_TEST(fi.has_value());
    HPX_TEST(!fi.has_exception());
}

///////////////////////////////////////////////////////////////////////////////
void test_set_value_can_be_retrieved()
{
    hpx::promise<int> pi;
    hpx::future<int> fi;
    fi = pi.get_future();

    pi.set_value(42);

    fi.wait();
    HPX_TEST(fi.is_ready());
    HPX_TEST(fi.has_value());
    HPX_TEST(!fi.has_exception());
    int i = fi.get();
    HPX_TEST_EQ(i, 42);
}

///////////////////////////////////////////////////////////////////////////////
void test_set_value_can_be_moved()
{
    hpx::promise<int> pi;
    hpx::future<int> fi;
    fi = pi.get_future();

    pi.set_value(42);

    fi.wait();
    HPX_TEST(fi.is_ready());
    HPX_TEST(fi.has_value());
    HPX_TEST(!fi.has_exception());
    int i = 0;
    HPX_TEST(i = fi.get());
    HPX_TEST_EQ(i, 42);
}

///////////////////////////////////////////////////////////////////////////////
void test_future_from_packaged_task_is_waiting()
{
    hpx::packaged_task<int()> pt(make_int);
    hpx::future<int> fi = pt.get_future();

    HPX_TEST(!fi.is_ready());
    HPX_TEST(!fi.has_value());
    HPX_TEST(!fi.has_exception());
}

///////////////////////////////////////////////////////////////////////////////
void test_invoking_a_packaged_task_populates_future()
{
    hpx::packaged_task<int()> pt(make_int);
    hpx::future<int> fi = pt.get_future();

    pt();

    HPX_TEST(fi.is_ready());
    HPX_TEST(fi.has_value());
    HPX_TEST(!fi.has_exception());

    int i = fi.get();
    HPX_TEST_EQ(i, 42);
}

///////////////////////////////////////////////////////////////////////////////
void test_invoking_a_packaged_task_twice_throws()
{
    hpx::packaged_task<int()> pt(make_int);

    pt();
    try
    {
        pt();
        HPX_TEST(false);
    }
    catch (hpx::exception const& e)
    {
        HPX_TEST_EQ(e.get_error(), hpx::error::promise_already_satisfied);
    }
    catch (...)
    {
        HPX_TEST(false);
    }
    // retrieve the future so the destructor of packaged_task is happy.
    // Otherwise, an exception will be tried to set to future_data which
    // leads to another exception to the fact that the future has already been
    // set.
    pt.get_future().get();
}

///////////////////////////////////////////////////////////////////////////////
void test_cannot_get_future_twice_from_task()
{
    hpx::packaged_task<int()> pt(make_int);
    pt.get_future();
    try
    {
        pt.get_future();
        HPX_TEST(false);
    }
    catch (hpx::exception const& e)
    {
        HPX_TEST_EQ(e.get_error(), hpx::error::future_already_retrieved);
    }
    catch (...)
    {
        HPX_TEST(false);
    }
}

void test_task_stores_exception_if_function_throws()
{
    hpx::packaged_task<int()> pt(throw_runtime_error);
    hpx::future<int> fi = pt.get_future();

    pt();

    HPX_TEST(fi.is_ready());
    HPX_TEST(!fi.has_value());
    HPX_TEST(fi.has_exception());
    try
    {
        fi.get();
        HPX_TEST(false);
    }
    catch (std::exception&)
    {
        HPX_TEST(true);
    }
    catch (...)
    {
        HPX_TEST(!"Unknown exception thrown");
    }
}

void test_void_promise()
{
    hpx::promise<void> p;
    hpx::future<void> f = p.get_future();

    p.set_value();
    HPX_TEST(f.is_ready());
    HPX_TEST(f.has_value());
    HPX_TEST(!f.has_exception());
}

void test_reference_promise()
{
    hpx::promise<int&> p;
    hpx::future<int&> f = p.get_future();
    int i = 42;
    p.set_value(i);
    HPX_TEST(f.is_ready());
    HPX_TEST(f.has_value());
    HPX_TEST(!f.has_exception());
    HPX_TEST_EQ(&f.get(), &i);
}

void do_nothing() {}

void test_task_returning_void()
{
    hpx::packaged_task<void()> pt(do_nothing);
    hpx::future<void> fi = pt.get_future();

    pt();

    HPX_TEST(fi.is_ready());
    HPX_TEST(fi.has_value());
    HPX_TEST(!fi.has_exception());
}

int global_ref_target = 0;

int& return_ref()
{
    return global_ref_target;
}

void test_task_returning_reference()
{
    hpx::packaged_task<int&()> pt(return_ref);
    hpx::future<int&> fi = pt.get_future();

    pt();

    HPX_TEST(fi.is_ready());
    HPX_TEST(fi.has_value());
    HPX_TEST(!fi.has_exception());
    int& i = fi.get();
    HPX_TEST_EQ(&i, &global_ref_target);
}

void test_future_for_move_only_udt()
{
    hpx::promise<X> pt;
    hpx::future<X> fi = pt.get_future();

    pt.set_value(X());
    X res(fi.get());
    HPX_TEST_EQ(res.i, 42);
}

void test_future_for_string()
{
    hpx::promise<std::string> pt;
    hpx::future<std::string> fi1 = pt.get_future();

    pt.set_value(std::string("hello"));
    std::string res(fi1.get());
    HPX_TEST_EQ(res, "hello");

    hpx::promise<std::string> pt2;
    fi1 = pt2.get_future();

    std::string const s = "goodbye";

    pt2.set_value(s);
    res = fi1.get();
    HPX_TEST_EQ(res, "goodbye");

    hpx::promise<std::string> pt3;
    fi1 = pt3.get_future();

    std::string s2 = "foo";

    pt3.set_value(s2);
    res = fi1.get();
    HPX_TEST_EQ(res, "foo");
}

hpx::spinlock callback_mutex;
unsigned callback_called = 0;

void wait_callback(hpx::future<int>)
{
    std::lock_guard<hpx::spinlock> lk(callback_mutex);
    ++callback_called;
}

void promise_set_value(hpx::promise<int>& pi)
{
    try
    {
        pi.set_value(42);
    }
    // NOLINTNEXTLINE(bugprone-empty-catch)
    catch (...)
    {
    }
}

void test_wait_callback()
{
    callback_called = 0;
    hpx::promise<int> pi;
    hpx::future<int> fi = pi.get_future();

    hpx::future<void> ft = fi.then(&wait_callback);
    hpx::thread t(&promise_set_value, std::ref(pi));

    ft.wait();

    t.join();

    HPX_TEST_EQ(callback_called, 1U);
    ft.wait();
    ft.wait();
    ft.get();
    HPX_TEST_EQ(callback_called, 1U);
}

void do_nothing_callback(hpx::promise<int>& /*pi*/)
{
    std::lock_guard<hpx::spinlock> lk(callback_mutex);
    ++callback_called;
}

void test_wait_callback_with_timed_wait()
{
    callback_called = 0;
    hpx::promise<int> pi;
    hpx::future<int> fi = pi.get_future();

    hpx::future<void> fv =
        fi.then(hpx::bind(&do_nothing_callback, std::ref(pi)));

    int state = static_cast<int>(fv.wait_for(std::chrono::milliseconds(100)));
    HPX_TEST_EQ(state, static_cast<int>(hpx::future_status::timeout));
    HPX_TEST_EQ(callback_called, 0U);

    state = static_cast<int>(fv.wait_for(std::chrono::milliseconds(100)));
    HPX_TEST_EQ(state, static_cast<int>(hpx::future_status::timeout));
    state = static_cast<int>(fv.wait_for(std::chrono::milliseconds(100)));
    HPX_TEST_EQ(state, static_cast<int>(hpx::future_status::timeout));
    HPX_TEST_EQ(callback_called, 0U);

    pi.set_value(42);

    state = static_cast<int>(fv.wait_for(std::chrono::milliseconds(100)));
    HPX_TEST_EQ(state, static_cast<int>(hpx::future_status::ready));

    HPX_TEST_EQ(callback_called, 1U);
}

///////////////////////////////////////////////////////////////////////////////
void test_wait_for_returns_as_soon_as_value_is_set()
{
    hpx::promise<int> p;
    hpx::future<int> f = p.get_future();

    hpx::thread t([&p]() {
        hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
        p.set_value(42);
    });

    std::chrono::steady_clock::time_point const start =
        std::chrono::steady_clock::now();
    hpx::future_status const status = f.wait_for(std::chrono::seconds(2));
    std::chrono::steady_clock::time_point const end =
        std::chrono::steady_clock::now();

    HPX_TEST_EQ(
        static_cast<int>(status), static_cast<int>(hpx::future_status::ready));
    HPX_TEST(end - start < std::chrono::milliseconds(1500));
    HPX_TEST_EQ(f.get(), 42);

    t.join();
}

void test_wait_until_returns_as_soon_as_value_is_set()
{
    hpx::promise<int> p;
    hpx::future<int> f = p.get_future();

    hpx::thread t([&p]() {
        hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
        p.set_value(42);
    });

    std::chrono::steady_clock::time_point const start =
        std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point const abs_time =
        start + std::chrono::seconds(2);
    hpx::future_status const status = f.wait_until(abs_time);
    std::chrono::steady_clock::time_point const end =
        std::chrono::steady_clock::now();

    HPX_TEST_EQ(
        static_cast<int>(status), static_cast<int>(hpx::future_status::ready));
    HPX_TEST(end - start < std::chrono::milliseconds(1500));
    HPX_TEST_EQ(f.get(), 42);

    t.join();
}

void test_wait_for_returns_as_soon_as_exception_is_set()
{
    hpx::promise<int> p;
    hpx::future<int> f = p.get_future();

    hpx::thread t([&p]() {
        hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
        p.set_exception(std::make_exception_ptr(my_exception()));
    });

    std::chrono::steady_clock::time_point const start =
        std::chrono::steady_clock::now();
    hpx::future_status const status = f.wait_for(std::chrono::seconds(10));
    std::chrono::steady_clock::time_point const end =
        std::chrono::steady_clock::now();

    HPX_TEST_EQ(
        static_cast<int>(status), static_cast<int>(hpx::future_status::ready));
    HPX_TEST(end - start < std::chrono::seconds(5));

    try
    {
        f.get();
        HPX_TEST(false);
    }
    catch (my_exception)
    {
        HPX_TEST(true);
    }

    t.join();
}

void test_wait_for_times_out_when_never_set()
{
    hpx::promise<int> p;
    hpx::future<int> const f = p.get_future();

    std::chrono::steady_clock::time_point const start =
        std::chrono::steady_clock::now();
    hpx::future_status const status =
        f.wait_for(std::chrono::milliseconds(200));
    std::chrono::steady_clock::time_point const end =
        std::chrono::steady_clock::now();

    HPX_TEST_EQ(static_cast<int>(status),
        static_cast<int>(hpx::future_status::timeout));
    HPX_TEST(end - start >= std::chrono::milliseconds(150));

    // fulfill the promise so the destructor of promise is happy.
    p.set_value(0);
}

void test_packaged_task_can_be_moved()
{
    hpx::packaged_task<int()> pt(make_int);
    hpx::future<int> const fi = pt.get_future();
    HPX_TEST(!fi.is_ready());

    hpx::packaged_task<int()> pt2(std::move(pt));
    HPX_TEST(!fi.is_ready());

    try
    {
        pt();    // NOLINT
        HPX_TEST(!"Can invoke moved task!");
    }
    catch (hpx::exception const& e)
    {
        HPX_TEST_EQ(e.get_error(), hpx::error::no_state);
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    HPX_TEST(!fi.is_ready());

    pt2();

    HPX_TEST(fi.is_ready());
}

void test_destroying_a_promise_stores_broken_promise()
{
    hpx::future<int> f;

    {
        hpx::promise<int> p;
        f = p.get_future();
    }

    HPX_TEST(f.is_ready());
    HPX_TEST(f.has_exception());
    try
    {
        f.get();
        HPX_TEST(false);    // shouldn't get here
    }
    catch (hpx::exception const& e)
    {
        HPX_TEST_EQ(e.get_error(), hpx::error::broken_promise);
    }
    catch (...)
    {
        HPX_TEST(false);
    }
}

void test_destroying_a_packaged_task_stores_broken_task()
{
    hpx::future<int> f;

    {
        hpx::packaged_task<int()> p(make_int);
        f = p.get_future();
    }

    HPX_TEST(f.is_ready());
    HPX_TEST(f.has_exception());
    try
    {
        f.get();
        HPX_TEST(false);    // shouldn't get here
    }
    catch (hpx::exception const& e)
    {
        HPX_TEST_EQ(e.get_error(), hpx::error::broken_promise);
    }
    catch (...)
    {
        HPX_TEST(false);
    }
}

void test_assign_to_void()
{
    hpx::future<void> f1 = hpx::make_ready_future(42);
    f1.get();

    hpx::shared_future<void> f2 = hpx::make_ready_future(42).share();
    f2.get();
}

///////////////////////////////////////////////////////////////////////////////
using hpx::program_options::options_description;
using hpx::program_options::variables_map;

int hpx_main(variables_map&)
{
    {
        test_store_value_from_thread();
        test_store_exception();
        test_initial_state();
        test_waiting_future();
        test_cannot_get_future_twice();
        test_set_value_updates_future_status();
        test_set_value_can_be_retrieved();
        test_set_value_can_be_moved();
        test_future_from_packaged_task_is_waiting();
        test_invoking_a_packaged_task_populates_future();
        test_invoking_a_packaged_task_twice_throws();
        test_cannot_get_future_twice_from_task();
        test_task_stores_exception_if_function_throws();
        test_void_promise();
        test_reference_promise();
        test_task_returning_void();
        test_task_returning_reference();
        test_future_for_move_only_udt();
        test_future_for_string();
        test_wait_callback();
        test_wait_callback_with_timed_wait();
        test_wait_for_returns_as_soon_as_value_is_set();
        test_wait_until_returns_as_soon_as_value_is_set();
        test_wait_for_returns_as_soon_as_exception_is_set();
        test_wait_for_times_out_when_never_set();
        test_packaged_task_can_be_moved();
        test_destroying_a_promise_stores_broken_promise();
        test_destroying_a_packaged_task_stores_broken_task();
        test_assign_to_void();
    }

    hpx::local::finalize();
    return hpx::util::report_errors();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    // Configure application-specific options
    options_description cmdline("Usage: " HPX_APPLICATION_STRING " [options]");

    // We force this test to use several threads by default.
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.desc_cmdline = cmdline;
    init_args.cfg = cfg;

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
