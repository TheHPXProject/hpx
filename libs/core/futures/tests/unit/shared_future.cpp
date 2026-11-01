//  Copyright (C) 2012 Hartmut Kaiser
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
    hpx::shared_future<int> fi2(pi2.get_future());
    hpx::thread t(&set_promise_thread, &pi2);
    int j = fi2.get();
    HPX_TEST_EQ(j, 42);
    HPX_TEST(fi2.is_ready());
    HPX_TEST(fi2.has_value());
    HPX_TEST(!fi2.has_exception());
    t.join();
}

///////////////////////////////////////////////////////////////////////////////
void test_store_exception()
{
    hpx::promise<int> pi3;
    hpx::shared_future<int> fi3 = pi3.get_future();
    hpx::thread t(&set_promise_exception_thread, &pi3);
    try
    {
        fi3.get();
        HPX_TEST(false);
    }
    catch (my_exception)
    {
        HPX_TEST(true);
    }

    HPX_TEST(fi3.is_ready());
    HPX_TEST(!fi3.has_value());
    HPX_TEST(fi3.has_exception());
    t.join();
}

///////////////////////////////////////////////////////////////////////////////
void test_initial_state()
{
    hpx::shared_future<int> fi;
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
    hpx::shared_future<int> fi;
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
    hpx::shared_future<int> fi;
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
    hpx::shared_future<int> fi;
    fi = pi.get_future();

    pi.set_value(42);

    int i = fi.get();
    HPX_TEST_EQ(i, 42);
    HPX_TEST(fi.is_ready());
    HPX_TEST(fi.has_value());
    HPX_TEST(!fi.has_exception());
}

///////////////////////////////////////////////////////////////////////////////
void test_set_value_can_be_moved()
{
    hpx::promise<int> pi;
    hpx::shared_future<int> fi;
    fi = pi.get_future();

    pi.set_value(42);

    int i = 0;
    HPX_TEST(i = fi.get());
    HPX_TEST_EQ(i, 42);
    HPX_TEST(fi.is_ready());
    HPX_TEST(fi.has_value());
    HPX_TEST(!fi.has_exception());
}

///////////////////////////////////////////////////////////////////////////////
void test_future_from_packaged_task_is_waiting()
{
    hpx::packaged_task<int()> pt(make_int);
    hpx::shared_future<int> fi = pt.get_future();

    HPX_TEST(!fi.is_ready());
    HPX_TEST(!fi.has_value());
    HPX_TEST(!fi.has_exception());
}

///////////////////////////////////////////////////////////////////////////////
void test_invoking_a_packaged_task_populates_future()
{
    hpx::packaged_task<int()> pt(make_int);
    hpx::shared_future<int> fi = pt.get_future();

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
    // Otherwise an exception will be tried to set to future_data which
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
    hpx::shared_future<int> fi = pt.get_future();

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
    hpx::shared_future<void> f = p.get_future();

    p.set_value();
    HPX_TEST(f.is_ready());
    HPX_TEST(f.has_value());
    HPX_TEST(!f.has_exception());
}

void test_reference_promise()
{
    hpx::promise<int&> p;
    hpx::shared_future<int&> f = p.get_future();
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
    hpx::shared_future<void> fi = pt.get_future();

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
    hpx::shared_future<int&> fi = pt.get_future();

    pt();

    HPX_TEST(fi.is_ready());
    HPX_TEST(fi.has_value());
    HPX_TEST(!fi.has_exception());
    int& i = fi.get();
    HPX_TEST_EQ(&i, &global_ref_target);
}

void test_shared_future()
{
    hpx::packaged_task<int()> pt(make_int);
    hpx::shared_future<int> fi = pt.get_future();

    hpx::shared_future<int> sf(std::move(fi));

    pt();

    HPX_TEST(sf.is_ready());
    HPX_TEST(sf.has_value());
    HPX_TEST(!sf.has_exception());

    int i = sf.get();
    HPX_TEST_EQ(i, 42);
}

void test_copies_of_shared_future_become_ready_together()
{
    hpx::packaged_task<int()> pt(make_int);
    hpx::shared_future<int> fi = pt.get_future();

    hpx::shared_future<int> sf1(std::move(fi));
    hpx::shared_future<int> sf2(sf1);
    hpx::shared_future<int> sf3;

    sf3 = sf1;
    HPX_TEST(!sf1.is_ready());
    HPX_TEST(!sf2.is_ready());
    HPX_TEST(!sf3.is_ready());

    pt();

    HPX_TEST(sf1.is_ready());
    HPX_TEST(sf1.has_value());
    HPX_TEST(!sf1.has_exception());
    int i = sf1.get();
    HPX_TEST_EQ(i, 42);

    HPX_TEST(sf2.is_ready());
    HPX_TEST(sf2.has_value());
    HPX_TEST(!sf2.has_exception());
    i = sf2.get();
    HPX_TEST_EQ(i, 42);

    HPX_TEST(sf3.is_ready());
    HPX_TEST(sf3.has_value());
    HPX_TEST(!sf3.has_exception());
    i = sf3.get();
    HPX_TEST_EQ(i, 42);
}

void test_shared_future_can_be_move_assigned_from_shared_future()
{
    hpx::packaged_task<int()> pt(make_int);
    hpx::shared_future<int> fi = pt.get_future();

    hpx::shared_future<int> sf;
    sf = std::move(fi);
    HPX_TEST(!fi.valid());    // NOLINT

    HPX_TEST(!sf.is_ready());
    HPX_TEST(!sf.has_value());
    HPX_TEST(!sf.has_exception());
}

void test_shared_future_void()
{
    hpx::packaged_task<void()> pt(do_nothing);
    hpx::shared_future<void> fi = pt.get_future();

    hpx::shared_future<void> sf(std::move(fi));
    HPX_TEST(!fi.valid());    // NOLINT

    pt();

    HPX_TEST(sf.is_ready());
    HPX_TEST(sf.has_value());
    HPX_TEST(!sf.has_exception());
    sf.get();
}

void test_shared_future_ref()
{
    hpx::promise<int&> p;
    hpx::shared_future<int&> f(p.get_future());
    int i = 42;
    p.set_value(i);
    HPX_TEST(f.is_ready());
    HPX_TEST(f.has_value());
    HPX_TEST(!f.has_exception());
    HPX_TEST_EQ(&f.get(), &i);
}

// #7680: shared_future::get() must not invalidate the future
void test_shared_future_get_keeps_valid()
{
    hpx::shared_future<int> const f = hpx::make_ready_future(42).share();

    int const& result = f.get();
    HPX_TEST(f.valid());
    HPX_TEST_EQ(result, 42);
    HPX_TEST_EQ(f.get(), 42);
    HPX_TEST_EQ(&f.get(), &result);

    hpx::shared_future<void> const fv = hpx::make_ready_future().share();
    fv.get();
    HPX_TEST(fv.valid());
    fv.get();
}

void test_shared_future_for_string()
{
    hpx::promise<std::string> pt;
    hpx::shared_future<std::string> fi1 = pt.get_future();

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

void wait_callback(hpx::shared_future<int>)
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
    catch (...)
    {
        HPX_TEST(false);
    }
}

void test_wait_callback()
{
    callback_called = 0;
    hpx::promise<int> pi;
    hpx::shared_future<int> fi = pi.get_future();

    hpx::future<void> cbf = fi.then(&wait_callback);
    hpx::thread t(&promise_set_value, std::ref(pi));

    fi.wait();
    cbf.wait();

    t.join();

    HPX_TEST_EQ(callback_called, 1U);
    HPX_TEST_EQ(fi.get(), 42);
    fi.wait();
    fi.wait();
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
    hpx::shared_future<int> fi = pi.get_future();

    hpx::shared_future<void> fv =
        fi.then(hpx::bind(&do_nothing_callback, std::ref(pi)));

    int state = int(fv.wait_for(std::chrono::milliseconds(100)));
    HPX_TEST_EQ(state, int(hpx::future_status::timeout));
    HPX_TEST_EQ(callback_called, 0U);

    state = int(fv.wait_for(std::chrono::milliseconds(100)));
    HPX_TEST_EQ(state, int(hpx::future_status::timeout));
    state = int(fv.wait_for(std::chrono::milliseconds(100)));
    HPX_TEST_EQ(state, int(hpx::future_status::timeout));
    HPX_TEST_EQ(callback_called, 0U);

    pi.set_value(42);

    state = int(fv.wait_for(std::chrono::milliseconds(100)));
    HPX_TEST_EQ(state, int(hpx::future_status::ready));

    HPX_TEST_EQ(callback_called, 1U);
}

void test_packaged_task_can_be_moved()
{
    hpx::packaged_task<int()> pt(make_int);
    hpx::shared_future<int> fi = pt.get_future();
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
    hpx::shared_future<int> f;

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
    hpx::shared_future<int> f;

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

///////////////////////////////////////////////////////////////////////////////
int make_int_slowly()
{
    hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
    return 42;
}

void test_wait_for_either_of_two_futures_1()
{
    hpx::packaged_task<int()> pt1(make_int_slowly);
    hpx::shared_future<int> f1(pt1.get_future());
    hpx::packaged_task<int()> pt2(make_int_slowly);
    hpx::shared_future<int> f2(pt2.get_future());

    pt1();

    hpx::future<hpx::when_any_result<
        hpx::tuple<hpx::shared_future<int>, hpx::shared_future<int>>>>
        r = hpx::when_any(f1, f2);
    hpx::tuple<hpx::shared_future<int>, hpx::shared_future<int>> t =
        r.get().futures;

    HPX_TEST(f1.is_ready());
    HPX_TEST(!f2.is_ready());
    HPX_TEST_EQ(f1.get(), 42);

    HPX_TEST(hpx::get<0>(t).is_ready());
    HPX_TEST_EQ(hpx::get<0>(t).get(), 42);
}

void test_wait_for_either_of_two_futures_2()
{
    hpx::packaged_task<int()> pt(make_int_slowly);
    hpx::shared_future<int> f1(pt.get_future());
    hpx::packaged_task<int()> pt2(make_int_slowly);
    hpx::shared_future<int> f2(pt2.get_future());

    pt2();

    hpx::future<hpx::when_any_result<
        hpx::tuple<hpx::shared_future<int>, hpx::shared_future<int>>>>
        r = hpx::when_any(f1, f2);
    hpx::tuple<hpx::shared_future<int>, hpx::shared_future<int>> t =
        r.get().futures;

    HPX_TEST(!f1.is_ready());
    HPX_TEST(f2.is_ready());
    HPX_TEST_EQ(f2.get(), 42);

    HPX_TEST(hpx::get<1>(t).is_ready());
    HPX_TEST_EQ(hpx::get<1>(t).get(), 42);
}

void test_wait_for_either_of_two_futures_list_1()
{
    std::vector<hpx::shared_future<int>> futures;
    hpx::packaged_task<int()> pt1(make_int_slowly);
    futures.push_back(pt1.get_future());
    hpx::packaged_task<int()> pt2(make_int_slowly);
    futures.push_back(pt2.get_future());

    pt1();

    hpx::future<hpx::when_any_result<std::vector<hpx::shared_future<int>>>> r =
        hpx::when_any(futures);
    hpx::when_any_result<std::vector<hpx::shared_future<int>>> raw = r.get();

    HPX_TEST_EQ(raw.index, 0u);

    std::vector<hpx::shared_future<int>> t = std::move(raw.futures);

    HPX_TEST(futures[0].is_ready());
    HPX_TEST(!futures[1].is_ready());
    HPX_TEST_EQ(futures[0].get(), 42);

    HPX_TEST(t[0].is_ready());
    HPX_TEST_EQ(t[0].get(), 42);
}

void test_wait_for_either_of_two_futures_list_2()
{
    std::vector<hpx::shared_future<int>> futures;
    hpx::packaged_task<int()> pt1(make_int_slowly);
    futures.push_back(pt1.get_future());
    hpx::packaged_task<int()> pt2(make_int_slowly);
    futures.push_back(pt2.get_future());

    pt2();

    hpx::future<hpx::when_any_result<std::vector<hpx::shared_future<int>>>> r =
        hpx::when_any(futures);
    hpx::when_any_result<std::vector<hpx::shared_future<int>>> raw = r.get();

    HPX_TEST_EQ(raw.index, 1u);

    std::vector<hpx::shared_future<int>> t = std::move(raw.futures);

    HPX_TEST(!futures[0].is_ready());
    HPX_TEST(futures[1].is_ready());
    HPX_TEST_EQ(futures[1].get(), 42);

    HPX_TEST(t[1].is_ready());
    HPX_TEST_EQ(t[1].get(), 42);
}

void test_wait_for_either_of_three_futures_1()
{
    hpx::packaged_task<int()> pt1(make_int_slowly);
    hpx::shared_future<int> f1(pt1.get_future());
    hpx::packaged_task<int()> pt2(make_int_slowly);
    hpx::shared_future<int> f2(pt2.get_future());
    hpx::packaged_task<int()> pt3(make_int_slowly);
    hpx::shared_future<int> f3(pt3.get_future());

    pt1();

    hpx::future<hpx::when_any_result<hpx::tuple<hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>>>>
        r = hpx::when_any(f1, f2, f3);
    hpx::tuple<hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>>
        t = r.get().futures;

    HPX_TEST(f1.is_ready());
    HPX_TEST(!f2.is_ready());
    HPX_TEST(!f3.is_ready());
    HPX_TEST_EQ(f1.get(), 42);

    HPX_TEST(hpx::get<0>(t).is_ready());
    HPX_TEST_EQ(hpx::get<0>(t).get(), 42);
}

void test_wait_for_either_of_three_futures_2()
{
    hpx::packaged_task<int()> pt1(make_int_slowly);
    hpx::shared_future<int> f1(pt1.get_future());
    hpx::packaged_task<int()> pt2(make_int_slowly);
    hpx::shared_future<int> f2(pt2.get_future());
    hpx::packaged_task<int()> pt3(make_int_slowly);
    hpx::shared_future<int> f3(pt3.get_future());

    pt2();

    hpx::future<hpx::when_any_result<hpx::tuple<hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>>>>
        r = hpx::when_any(f1, f2, f3);
    hpx::tuple<hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>>
        t = r.get().futures;

    HPX_TEST(!f1.is_ready());
    HPX_TEST(f2.is_ready());
    HPX_TEST(!f3.is_ready());
    HPX_TEST_EQ(f2.get(), 42);

    HPX_TEST(hpx::get<1>(t).is_ready());
    HPX_TEST_EQ(hpx::get<1>(t).get(), 42);
}

void test_wait_for_either_of_three_futures_3()
{
    hpx::packaged_task<int()> pt1(make_int_slowly);
    hpx::shared_future<int> f1(pt1.get_future());
    hpx::packaged_task<int()> pt2(make_int_slowly);
    hpx::shared_future<int> f2(pt2.get_future());
    hpx::packaged_task<int()> pt3(make_int_slowly);
    hpx::shared_future<int> f3(pt3.get_future());

    pt3();

    hpx::future<hpx::when_any_result<hpx::tuple<hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>>>>
        r = hpx::when_any(f1, f2, f3);
    hpx::tuple<hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>>
        t = r.get().futures;

    HPX_TEST(!f1.is_ready());
    HPX_TEST(!f2.is_ready());
    HPX_TEST(f3.is_ready());
    HPX_TEST_EQ(f3.get(), 42);

    HPX_TEST(hpx::get<2>(t).is_ready());
    HPX_TEST_EQ(hpx::get<2>(t).get(), 42);
}

void test_wait_for_either_of_four_futures_1()
{
    hpx::packaged_task<int()> pt1(make_int_slowly);
    hpx::shared_future<int> f1(pt1.get_future());
    hpx::packaged_task<int()> pt2(make_int_slowly);
    hpx::shared_future<int> f2(pt2.get_future());
    hpx::packaged_task<int()> pt3(make_int_slowly);
    hpx::shared_future<int> f3(pt3.get_future());
    hpx::packaged_task<int()> pt4(make_int_slowly);
    hpx::shared_future<int> f4(pt4.get_future());

    pt1();

    hpx::future<hpx::when_any_result<
        hpx::tuple<hpx::shared_future<int>, hpx::shared_future<int>,
            hpx::shared_future<int>, hpx::shared_future<int>>>>
        r = hpx::when_any(f1, f2, f3, f4);
    hpx::tuple<hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>>
        t = r.get().futures;

    HPX_TEST(f1.is_ready());
    HPX_TEST(!f2.is_ready());
    HPX_TEST(!f3.is_ready());
    HPX_TEST(!f4.is_ready());
    HPX_TEST_EQ(f1.get(), 42);

    HPX_TEST(hpx::get<0>(t).is_ready());
    HPX_TEST_EQ(hpx::get<0>(t).get(), 42);
}

void test_wait_for_either_of_four_futures_2()
{
    hpx::packaged_task<int()> pt1(make_int_slowly);
    hpx::shared_future<int> f1(pt1.get_future());
    hpx::packaged_task<int()> pt2(make_int_slowly);
    hpx::shared_future<int> f2(pt2.get_future());
    hpx::packaged_task<int()> pt3(make_int_slowly);
    hpx::shared_future<int> f3(pt3.get_future());
    hpx::packaged_task<int()> pt4(make_int_slowly);
    hpx::shared_future<int> f4(pt4.get_future());

    pt2();

    hpx::future<hpx::when_any_result<
        hpx::tuple<hpx::shared_future<int>, hpx::shared_future<int>,
            hpx::shared_future<int>, hpx::shared_future<int>>>>
        r = hpx::when_any(f1, f2, f3, f4);
    hpx::tuple<hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>>
        t = r.get().futures;

    HPX_TEST(!f1.is_ready());
    HPX_TEST(f2.is_ready());
    HPX_TEST(!f3.is_ready());
    HPX_TEST(!f4.is_ready());
    HPX_TEST_EQ(f2.get(), 42);

    HPX_TEST(hpx::get<1>(t).is_ready());
    HPX_TEST_EQ(hpx::get<1>(t).get(), 42);
}

void test_wait_for_either_of_four_futures_3()
{
    hpx::packaged_task<int()> pt1(make_int_slowly);
    hpx::shared_future<int> f1(pt1.get_future());
    hpx::packaged_task<int()> pt2(make_int_slowly);
    hpx::shared_future<int> f2(pt2.get_future());
    hpx::packaged_task<int()> pt3(make_int_slowly);
    hpx::shared_future<int> f3(pt3.get_future());
    hpx::packaged_task<int()> pt4(make_int_slowly);
    hpx::shared_future<int> f4(pt4.get_future());

    pt3();

    hpx::future<hpx::when_any_result<
        hpx::tuple<hpx::shared_future<int>, hpx::shared_future<int>,
            hpx::shared_future<int>, hpx::shared_future<int>>>>
        r = hpx::when_any(f1, f2, f3, f4);
    hpx::tuple<hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>>
        t = r.get().futures;

    HPX_TEST(!f1.is_ready());
    HPX_TEST(!f2.is_ready());
    HPX_TEST(f3.is_ready());
    HPX_TEST(!f4.is_ready());
    HPX_TEST_EQ(f3.get(), 42);

    HPX_TEST(hpx::get<2>(t).is_ready());
    HPX_TEST_EQ(hpx::get<2>(t).get(), 42);
}

void test_wait_for_either_of_four_futures_4()
{
    hpx::packaged_task<int()> pt1(make_int_slowly);
    hpx::shared_future<int> f1(pt1.get_future());
    hpx::packaged_task<int()> pt2(make_int_slowly);
    hpx::shared_future<int> f2(pt2.get_future());
    hpx::packaged_task<int()> pt3(make_int_slowly);
    hpx::shared_future<int> f3(pt3.get_future());
    hpx::packaged_task<int()> pt4(make_int_slowly);
    hpx::shared_future<int> f4(pt4.get_future());

    pt4();

    hpx::future<hpx::when_any_result<
        hpx::tuple<hpx::shared_future<int>, hpx::shared_future<int>,
            hpx::shared_future<int>, hpx::shared_future<int>>>>
        r = hpx::when_any(f1, f2, f3, f4);
    hpx::tuple<hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>>
        t = r.get().futures;

    HPX_TEST(!f1.is_ready());
    HPX_TEST(!f2.is_ready());
    HPX_TEST(!f3.is_ready());
    HPX_TEST(f4.is_ready());
    HPX_TEST_EQ(f4.get(), 42);

    HPX_TEST(hpx::get<3>(t).is_ready());
    HPX_TEST_EQ(hpx::get<3>(t).get(), 42);
}

void test_wait_for_either_of_five_futures_1_from_list()
{
    std::vector<hpx::shared_future<int>> futures;

    hpx::packaged_task<int()> pt1(make_int_slowly);
    hpx::shared_future<int> f1(pt1.get_future());
    futures.push_back(f1);
    hpx::packaged_task<int()> pt2(make_int_slowly);
    hpx::shared_future<int> f2(pt2.get_future());
    futures.push_back(f2);
    hpx::packaged_task<int()> pt3(make_int_slowly);
    hpx::shared_future<int> f3(pt3.get_future());
    futures.push_back(f3);
    hpx::packaged_task<int()> pt4(make_int_slowly);
    hpx::shared_future<int> f4(pt4.get_future());
    futures.push_back(f4);
    hpx::packaged_task<int()> pt5(make_int_slowly);
    hpx::shared_future<int> f5(pt5.get_future());
    futures.push_back(f5);

    pt1();

    hpx::future<hpx::when_any_result<std::vector<hpx::shared_future<int>>>> r =
        hpx::when_any(futures);
    hpx::when_any_result<std::vector<hpx::shared_future<int>>> raw = r.get();

    HPX_TEST_EQ(raw.index, 0u);

    std::vector<hpx::shared_future<int>> t = std::move(raw.futures);

    HPX_TEST(f1.is_ready());
    HPX_TEST(!f2.is_ready());
    HPX_TEST(!f3.is_ready());
    HPX_TEST(!f4.is_ready());
    HPX_TEST(!f5.is_ready());
    HPX_TEST_EQ(f1.get(), 42);

    HPX_TEST(t[0].is_ready());
    HPX_TEST_EQ(t[0].get(), 42);
}

void test_wait_for_either_of_five_futures_1_from_list_iterators()
{
    std::vector<hpx::shared_future<int>> futures;

    hpx::packaged_task<int()> pt1(make_int_slowly);
    hpx::shared_future<int> f1(pt1.get_future());
    futures.push_back(f1);
    hpx::packaged_task<int()> pt2(make_int_slowly);
    hpx::shared_future<int> f2(pt2.get_future());
    futures.push_back(f2);
    hpx::packaged_task<int()> pt3(make_int_slowly);
    hpx::shared_future<int> f3(pt3.get_future());
    futures.push_back(f3);
    hpx::packaged_task<int()> pt4(make_int_slowly);
    hpx::shared_future<int> f4(pt4.get_future());
    futures.push_back(f4);
    hpx::packaged_task<int()> pt5(make_int_slowly);
    hpx::shared_future<int> f5(pt5.get_future());
    futures.push_back(f5);

    pt1();

    hpx::future<hpx::when_any_result<std::vector<hpx::shared_future<int>>>> r =
        hpx::when_any(futures.begin(), futures.end());
    hpx::when_any_result<std::vector<hpx::shared_future<int>>> raw = r.get();

    HPX_TEST_EQ(raw.index, 0u);

    std::vector<hpx::shared_future<int>> t = std::move(raw.futures);

    HPX_TEST(f1.is_ready());
    HPX_TEST(!f2.is_ready());
    HPX_TEST(!f3.is_ready());
    HPX_TEST(!f4.is_ready());
    HPX_TEST(!f5.is_ready());
    HPX_TEST_EQ(f1.get(), 42);

    HPX_TEST(t[0].is_ready());
    HPX_TEST_EQ(t[0].get(), 42);
}

void test_wait_for_either_of_five_futures_1()
{
    hpx::packaged_task<int()> pt1(make_int_slowly);
    hpx::shared_future<int> f1(pt1.get_future());
    hpx::packaged_task<int()> pt2(make_int_slowly);
    hpx::shared_future<int> f2(pt2.get_future());
    hpx::packaged_task<int()> pt3(make_int_slowly);
    hpx::shared_future<int> f3(pt3.get_future());
    hpx::packaged_task<int()> pt4(make_int_slowly);
    hpx::shared_future<int> f4(pt4.get_future());
    hpx::packaged_task<int()> pt5(make_int_slowly);
    hpx::shared_future<int> f5(pt5.get_future());

    pt1();

    hpx::future<hpx::when_any_result<hpx::tuple<hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>>>>
        r = hpx::when_any(f1, f2, f3, f4, f5);
    hpx::tuple<hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>>
        t = r.get().futures;

    HPX_TEST(f1.is_ready());
    HPX_TEST(!f2.is_ready());
    HPX_TEST(!f3.is_ready());
    HPX_TEST(!f4.is_ready());
    HPX_TEST(!f5.is_ready());
    HPX_TEST_EQ(f1.get(), 42);

    HPX_TEST(hpx::get<0>(t).is_ready());
    HPX_TEST_EQ(hpx::get<0>(t).get(), 42);
}

void test_wait_for_either_of_five_futures_2()
{
    hpx::packaged_task<int()> pt1(make_int_slowly);
    hpx::shared_future<int> f1(pt1.get_future());
    hpx::packaged_task<int()> pt2(make_int_slowly);
    hpx::shared_future<int> f2(pt2.get_future());
    hpx::packaged_task<int()> pt3(make_int_slowly);
    hpx::shared_future<int> f3(pt3.get_future());
    hpx::packaged_task<int()> pt4(make_int_slowly);
    hpx::shared_future<int> f4(pt4.get_future());
    hpx::packaged_task<int()> pt5(make_int_slowly);
    hpx::shared_future<int> f5(pt5.get_future());

    pt2();

    hpx::future<hpx::when_any_result<hpx::tuple<hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>>>>
        r = hpx::when_any(f1, f2, f3, f4, f5);
    hpx::tuple<hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>>
        t = r.get().futures;

    HPX_TEST(!f1.is_ready());
    HPX_TEST(f2.is_ready());
    HPX_TEST(!f3.is_ready());
    HPX_TEST(!f4.is_ready());
    HPX_TEST(!f5.is_ready());
    HPX_TEST_EQ(f2.get(), 42);

    HPX_TEST(hpx::get<1>(t).is_ready());
    HPX_TEST_EQ(hpx::get<1>(t).get(), 42);
}

void test_wait_for_either_of_five_futures_3()
{
    hpx::packaged_task<int()> pt1(make_int_slowly);
    hpx::shared_future<int> f1(pt1.get_future());
    hpx::packaged_task<int()> pt2(make_int_slowly);
    hpx::shared_future<int> f2(pt2.get_future());
    hpx::packaged_task<int()> pt3(make_int_slowly);
    hpx::shared_future<int> f3(pt3.get_future());
    hpx::packaged_task<int()> pt4(make_int_slowly);
    hpx::shared_future<int> f4(pt4.get_future());
    hpx::packaged_task<int()> pt5(make_int_slowly);
    hpx::shared_future<int> f5(pt5.get_future());

    pt3();

    hpx::future<hpx::when_any_result<hpx::tuple<hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>>>>
        r = hpx::when_any(f1, f2, f3, f4, f5);
    hpx::tuple<hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>>
        t = r.get().futures;

    HPX_TEST(!f1.is_ready());
    HPX_TEST(!f2.is_ready());
    HPX_TEST(f3.is_ready());
    HPX_TEST(!f4.is_ready());
    HPX_TEST(!f5.is_ready());
    HPX_TEST_EQ(f3.get(), 42);

    HPX_TEST(hpx::get<2>(t).is_ready());
    HPX_TEST_EQ(hpx::get<2>(t).get(), 42);
}

void test_wait_for_either_of_five_futures_4()
{
    hpx::packaged_task<int()> pt1(make_int_slowly);
    hpx::shared_future<int> f1(pt1.get_future());
    hpx::packaged_task<int()> pt2(make_int_slowly);
    hpx::shared_future<int> f2(pt2.get_future());
    hpx::packaged_task<int()> pt3(make_int_slowly);
    hpx::shared_future<int> f3(pt3.get_future());
    hpx::packaged_task<int()> pt4(make_int_slowly);
    hpx::shared_future<int> f4(pt4.get_future());
    hpx::packaged_task<int()> pt5(make_int_slowly);
    hpx::shared_future<int> f5(pt5.get_future());

    pt4();

    hpx::future<hpx::when_any_result<hpx::tuple<hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>>>>
        r = hpx::when_any(f1, f2, f3, f4, f5);
    hpx::tuple<hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>>
        t = r.get().futures;

    HPX_TEST(!f1.is_ready());
    HPX_TEST(!f2.is_ready());
    HPX_TEST(!f3.is_ready());
    HPX_TEST(f4.is_ready());
    HPX_TEST(!f5.is_ready());
    HPX_TEST_EQ(f4.get(), 42);

    HPX_TEST(hpx::get<3>(t).is_ready());
    HPX_TEST_EQ(hpx::get<3>(t).get(), 42);
}

void test_wait_for_either_of_five_futures_5()
{
    hpx::packaged_task<int()> pt1(make_int_slowly);
    hpx::shared_future<int> f1(pt1.get_future());
    hpx::packaged_task<int()> pt2(make_int_slowly);
    hpx::shared_future<int> f2(pt2.get_future());
    hpx::packaged_task<int()> pt3(make_int_slowly);
    hpx::shared_future<int> f3(pt3.get_future());
    hpx::packaged_task<int()> pt4(make_int_slowly);
    hpx::shared_future<int> f4(pt4.get_future());
    hpx::packaged_task<int()> pt5(make_int_slowly);
    hpx::shared_future<int> f5(pt5.get_future());

    pt5();

    hpx::future<hpx::when_any_result<hpx::tuple<hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>>>>
        r = hpx::when_any(f1, f2, f3, f4, f5);
    hpx::tuple<hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>>
        t = r.get().futures;

    HPX_TEST(!f1.is_ready());
    HPX_TEST(!f2.is_ready());
    HPX_TEST(!f3.is_ready());
    HPX_TEST(!f4.is_ready());
    HPX_TEST(f5.is_ready());
    HPX_TEST_EQ(f5.get(), 42);

    HPX_TEST(hpx::get<4>(t).is_ready());
    HPX_TEST_EQ(hpx::get<4>(t).get(), 42);
}

///////////////////////////////////////////////////////////////////////////////
// void test_wait_for_either_invokes_callbacks()
// {
//     callback_called = 0;
//     hpx::packaged_task<int()> pt1(make_int_slowly);
//     hpx::shared_future<int> fi = pt1.get_future();
//     hpx::packaged_task<int()> pt2(make_int_slowly);
//     hpx::shared_future<int> fi2 = pt2.get_future();
//     pt1.set_wait_callback(wait_callback_for_task);
//
//     hpx::thread t(std::move(pt));
//
//     boost::wait_for_any(fi, fi2);
//     HPX_TEST_EQ(callback_called, 1U);
//     HPX_TEST_EQ(fi.get(), 42);
// }

// void test_wait_for_any_from_range()
// {
//     unsigned const count = 10;
//     for(unsigned i = 0; i < count; ++i)
//     {
//         hpx::packaged_task<int()> tasks[count];
//         hpx::shared_future<int> futures[count];
//         for(unsigned j = 0; j < count; ++j)
//         {
//             tasks[j] =
//               std::move(hpx::packaged_task<int()>(make_int_slowly));
//             futures[j] = tasks[j].get_future();
//         }
//         hpx::thread t(std::move(tasks[i]));
//
//         hpx::lcos::wait_any(futures, futures);
//
//         hpx::shared_future<int>* const future =
//              boost::wait_for_any(futures, futures+count);
//
//         HPX_TEST_EQ(future, (futures + i));
//         for(unsigned j = 0; j < count; ++j)
//         {
//             if (j != i)
//             {
//                 HPX_TEST(!futures[j].is_ready());
//             }
//             else
//             {
//                 HPX_TEST(futures[j].is_ready());
//             }
//         }
//         HPX_TEST_EQ(futures[i].get(), 42);
//     }
// }

void test_wait_for_all_from_list()
{
    unsigned const count = 10;
    std::vector<hpx::shared_future<int>> futures;
    for (unsigned j = 0; j < count; ++j)
    {
        hpx::lcos::local::futures_factory<int()> task(make_int_slowly);
        futures.push_back(task.get_future());
        task.post();
    }

    hpx::future<std::vector<hpx::shared_future<int>>> r =
        hpx::when_all(futures);

    std::vector<hpx::shared_future<int>> result = r.get();

    HPX_TEST_EQ(futures.size(), result.size());
    for (unsigned j = 0; j < count; ++j)
    {
        HPX_TEST(futures[j].is_ready());
        HPX_TEST(result[j].is_ready());
    }
}

void test_wait_for_all_from_list_iterators()
{
    unsigned const count = 10;
    std::vector<hpx::shared_future<int>> futures;
    for (unsigned j = 0; j < count; ++j)
    {
        hpx::lcos::local::futures_factory<int()> task(make_int_slowly);
        futures.push_back(task.get_future());
        task.post();
    }

    hpx::future<std::vector<hpx::shared_future<int>>> r =
        hpx::when_all(futures.begin(), futures.end());

    std::vector<hpx::shared_future<int>> result = r.get();

    HPX_TEST_EQ(futures.size(), result.size());
    for (unsigned j = 0; j < count; ++j)
    {
        HPX_TEST(futures[j].is_ready());
        HPX_TEST(result[j].is_ready());
    }
}

void test_wait_for_all_two_futures()
{
    hpx::lcos::local::futures_factory<int()> pt1(make_int_slowly);
    hpx::shared_future<int> f1 = pt1.get_future();
    pt1.post();
    hpx::lcos::local::futures_factory<int()> pt2(make_int_slowly);
    hpx::shared_future<int> f2 = pt2.get_future();
    pt2.post();

    typedef hpx::tuple<hpx::shared_future<int>, hpx::shared_future<int>>
        result_type;
    hpx::future<result_type> r = hpx::when_all(f1, f2);

    result_type result = r.get();

    HPX_TEST(hpx::get<0>(result).is_ready());
    HPX_TEST(hpx::get<1>(result).is_ready());
    HPX_TEST(f1.is_ready());
    HPX_TEST(f2.is_ready());
}

void test_wait_for_all_three_futures()
{
    hpx::lcos::local::futures_factory<int()> pt1(make_int_slowly);
    hpx::shared_future<int> f1 = pt1.get_future();
    pt1.post();
    hpx::lcos::local::futures_factory<int()> pt2(make_int_slowly);
    hpx::shared_future<int> f2 = pt2.get_future();
    pt2.post();
    hpx::lcos::local::futures_factory<int()> pt3(make_int_slowly);
    hpx::shared_future<int> f3 = pt3.get_future();
    pt3.post();

    typedef hpx::tuple<hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>>
        result_type;
    hpx::future<result_type> r = hpx::when_all(f1, f2, f3);

    result_type result = r.get();

    HPX_TEST(hpx::get<0>(result).is_ready());
    HPX_TEST(hpx::get<1>(result).is_ready());
    HPX_TEST(hpx::get<2>(result).is_ready());
    HPX_TEST(f1.is_ready());
    HPX_TEST(f2.is_ready());
    HPX_TEST(f3.is_ready());
}

void test_wait_for_all_four_futures()
{
    hpx::lcos::local::futures_factory<int()> pt1(make_int_slowly);
    hpx::shared_future<int> f1 = pt1.get_future();
    pt1.post();
    hpx::lcos::local::futures_factory<int()> pt2(make_int_slowly);
    hpx::shared_future<int> f2 = pt2.get_future();
    pt2.post();
    hpx::lcos::local::futures_factory<int()> pt3(make_int_slowly);
    hpx::shared_future<int> f3 = pt3.get_future();
    pt3.post();
    hpx::lcos::local::futures_factory<int()> pt4(make_int_slowly);
    hpx::shared_future<int> f4 = pt4.get_future();
    pt4.post();

    typedef hpx::tuple<hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>>
        result_type;
    hpx::future<result_type> r = hpx::when_all(f1, f2, f3, f4);

    result_type result = r.get();

    HPX_TEST(hpx::get<0>(result).is_ready());
    HPX_TEST(hpx::get<1>(result).is_ready());
    HPX_TEST(hpx::get<2>(result).is_ready());
    HPX_TEST(hpx::get<3>(result).is_ready());
    HPX_TEST(f1.is_ready());
    HPX_TEST(f2.is_ready());
    HPX_TEST(f3.is_ready());
    HPX_TEST(f4.is_ready());
}

void test_wait_for_all_five_futures()
{
    hpx::lcos::local::futures_factory<int()> pt1(make_int_slowly);
    hpx::shared_future<int> f1 = pt1.get_future();
    pt1.post();
    hpx::lcos::local::futures_factory<int()> pt2(make_int_slowly);
    hpx::shared_future<int> f2 = pt2.get_future();
    pt2.post();
    hpx::lcos::local::futures_factory<int()> pt3(make_int_slowly);
    hpx::shared_future<int> f3 = pt3.get_future();
    pt3.post();
    hpx::lcos::local::futures_factory<int()> pt4(make_int_slowly);
    hpx::shared_future<int> f4 = pt4.get_future();
    pt4.post();
    hpx::lcos::local::futures_factory<int()> pt5(make_int_slowly);
    hpx::shared_future<int> f5 = pt5.get_future();
    pt5.post();

    typedef hpx::tuple<hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>>
        result_type;
    hpx::future<result_type> r = hpx::when_all(f1, f2, f3, f4, f5);

    result_type result = r.get();

    HPX_TEST(hpx::get<0>(result).is_ready());
    HPX_TEST(hpx::get<1>(result).is_ready());
    HPX_TEST(hpx::get<2>(result).is_ready());
    HPX_TEST(hpx::get<3>(result).is_ready());
    HPX_TEST(hpx::get<4>(result).is_ready());
    HPX_TEST(f1.is_ready());
    HPX_TEST(f2.is_ready());
    HPX_TEST(f3.is_ready());
    HPX_TEST(f4.is_ready());
    HPX_TEST(f5.is_ready());
}

void test_wait_for_two_out_of_five_futures()
{
    unsigned const count = 2;

    hpx::packaged_task<int()> pt1(make_int_slowly);
    hpx::shared_future<int> f1 = pt1.get_future();
    hpx::packaged_task<int()> pt2(make_int_slowly);
    hpx::shared_future<int> f2 = pt2.get_future();
    pt2();
    hpx::packaged_task<int()> pt3(make_int_slowly);
    hpx::shared_future<int> f3 = pt3.get_future();
    hpx::packaged_task<int()> pt4(make_int_slowly);
    hpx::shared_future<int> f4 = pt4.get_future();
    pt4();
    hpx::packaged_task<int()> pt5(make_int_slowly);
    hpx::shared_future<int> f5 = pt5.get_future();

    typedef hpx::when_some_result<hpx::tuple<hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>>>
        result_type;
    hpx::future<result_type> r = hpx::when_some(count, f1, f2, f3, f4, f5);

    result_type result = r.get();

    HPX_TEST(!f1.is_ready());
    HPX_TEST(f2.is_ready());
    HPX_TEST(!f3.is_ready());
    HPX_TEST(f4.is_ready());
    HPX_TEST(!f5.is_ready());

    HPX_TEST_EQ(result.indices.size(), count);
    HPX_TEST(!hpx::get<0>(result.futures).is_ready());
    HPX_TEST(hpx::get<1>(result.futures).is_ready());
    HPX_TEST(!hpx::get<2>(result.futures).is_ready());
    HPX_TEST(hpx::get<3>(result.futures).is_ready());
    HPX_TEST(!hpx::get<4>(result.futures).is_ready());
}

void test_wait_for_three_out_of_five_futures()
{
    unsigned const count = 3;

    hpx::packaged_task<int()> pt1(make_int_slowly);
    hpx::shared_future<int> f1 = pt1.get_future();
    pt1();
    hpx::packaged_task<int()> pt2(make_int_slowly);
    hpx::shared_future<int> f2 = pt2.get_future();
    hpx::packaged_task<int()> pt3(make_int_slowly);
    hpx::shared_future<int> f3 = pt3.get_future();
    pt3();
    hpx::packaged_task<int()> pt4(make_int_slowly);
    hpx::shared_future<int> f4 = pt4.get_future();
    hpx::packaged_task<int()> pt5(make_int_slowly);
    hpx::shared_future<int> f5 = pt5.get_future();
    pt5();

    typedef hpx::when_some_result<hpx::tuple<hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>,
        hpx::shared_future<int>, hpx::shared_future<int>>>
        result_type;
    hpx::future<result_type> r = hpx::when_some(count, f1, f2, f3, f4, f5);

    result_type result = r.get();

    HPX_TEST(f1.is_ready());
    HPX_TEST(!f2.is_ready());
    HPX_TEST(f3.is_ready());
    HPX_TEST(!f4.is_ready());
    HPX_TEST(f5.is_ready());

    HPX_TEST_EQ(result.indices.size(), count);
    HPX_TEST(hpx::get<0>(result.futures).is_ready());
    HPX_TEST(!hpx::get<1>(result.futures).is_ready());
    HPX_TEST(hpx::get<2>(result.futures).is_ready());
    HPX_TEST(!hpx::get<3>(result.futures).is_ready());
    HPX_TEST(hpx::get<4>(result.futures).is_ready());
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
        test_shared_future();
        test_copies_of_shared_future_become_ready_together();
        test_shared_future_can_be_move_assigned_from_shared_future();
        test_shared_future_void();
        test_shared_future_ref();
        test_shared_future_get_keeps_valid();
        test_shared_future_for_string();
        test_wait_callback();
        test_wait_callback_with_timed_wait();
        test_packaged_task_can_be_moved();
        test_destroying_a_promise_stores_broken_promise();
        test_destroying_a_packaged_task_stores_broken_task();
        test_wait_for_either_of_two_futures_1();
        test_wait_for_either_of_two_futures_2();
        test_wait_for_either_of_two_futures_list_1();
        test_wait_for_either_of_two_futures_list_2();
        test_wait_for_either_of_three_futures_1();
        test_wait_for_either_of_three_futures_2();
        test_wait_for_either_of_three_futures_3();
        test_wait_for_either_of_four_futures_1();
        test_wait_for_either_of_four_futures_2();
        test_wait_for_either_of_four_futures_3();
        test_wait_for_either_of_four_futures_4();
        test_wait_for_either_of_five_futures_1_from_list();
        test_wait_for_either_of_five_futures_1_from_list_iterators();
        test_wait_for_either_of_five_futures_1();
        test_wait_for_either_of_five_futures_2();
        test_wait_for_either_of_five_futures_3();
        test_wait_for_either_of_five_futures_4();
        test_wait_for_either_of_five_futures_5();
        //         test_wait_for_either_invokes_callbacks();
        //         test_wait_for_any_from_range();
        test_wait_for_all_from_list();
        test_wait_for_all_from_list_iterators();
        test_wait_for_all_two_futures();
        test_wait_for_all_three_futures();
        test_wait_for_all_four_futures();
        test_wait_for_all_five_futures();
        test_wait_for_two_out_of_five_futures();
        test_wait_for_three_out_of_five_futures();
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
    std::vector<std::string> const cfg = {"hpx.os_threads=4"};

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.desc_cmdline = cmdline;
    init_args.cfg = cfg;

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
