//  Taken from the Boost.Thread library
//
// Copyright (C) 2001-2003 William E. Kempf
// Copyright (C) 2007-2008 Anthony Williams
// Copyright (C) 2013 Agustin Berge
// Copyright (C) 2022-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/condition_variable.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/mutex.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace {

    hpx::mutex multiple_wake_mutex;
    hpx::condition_variable multiple_wake_cond;
    unsigned multiple_wake_count = 0;

    void wait_for_condvar_and_increase_count()
    {
        std::unique_lock<hpx::mutex> lk(multiple_wake_mutex);
        multiple_wake_cond.wait(lk);
        ++multiple_wake_count;
    }

    void join_all(std::vector<hpx::thread>& group)
    {
        for (std::size_t i = 0; i < group.size(); ++i)
            group[i].join();
    }

}    // namespace

///////////////////////////////////////////////////////////////////////////////
struct wait_for_flag
{
    hpx::mutex mutex;
    hpx::condition_variable cond_var;
    bool flag;
    unsigned woken;

    wait_for_flag()
      : flag(false)
      , woken(0)
    {
    }

    struct check_flag
    {
        bool const& flag;

        check_flag(bool const& flag_)
          : flag(flag_)
        {
        }

        bool operator()() const
        {
            return flag;
        }
    };

    void wait_without_predicate()
    {
        std::unique_lock<hpx::mutex> lock(mutex);
        while (!flag)
        {
            cond_var.wait(lock);
        }
        ++woken;
    }

    void wait_with_predicate()
    {
        std::unique_lock<hpx::mutex> lock(mutex);
        cond_var.wait(lock, check_flag(flag));
        if (flag)
        {
            ++woken;
        }
    }

    void wait_until_without_predicate()
    {
        std::chrono::system_clock::time_point const timeout =
            std::chrono::system_clock::now() + std::chrono::milliseconds(5);

        std::unique_lock<hpx::mutex> lock(mutex);
        while (!flag)
        {
            if (cond_var.wait_until(lock, timeout) == hpx::cv_status::timeout)
            {
                return;
            }
        }
        ++woken;
    }

    void wait_until_with_predicate()
    {
        std::chrono::system_clock::time_point const timeout =
            std::chrono::system_clock::now() + std::chrono::milliseconds(5);

        std::unique_lock<hpx::mutex> lock(mutex);
        if (cond_var.wait_until(lock, timeout, check_flag(flag)) && flag)
        {
            ++woken;
        }
    }
    void relative_wait_until_with_predicate()
    {
        std::unique_lock<hpx::mutex> lock(mutex);
        if (cond_var.wait_for(
                lock, std::chrono::milliseconds(5), check_flag(flag)) &&
            flag)
        {
            ++woken;
        }
    }
};

void test_condition_notify_one_wakes_from_wait()
{
    wait_for_flag data;

    hpx::thread thread(&wait_for_flag::wait_without_predicate, std::ref(data));

    {
        std::unique_lock<hpx::mutex> lock(data.mutex);
        data.flag = true;
    }

    data.cond_var.notify_one();

    thread.join();
    HPX_TEST(data.woken);
}

void test_condition_notify_one_wakes_from_wait_with_predicate()
{
    wait_for_flag data;

    hpx::thread thread(&wait_for_flag::wait_with_predicate, std::ref(data));

    {
        std::unique_lock<hpx::mutex> lock(data.mutex);
        data.flag = true;
    }

    data.cond_var.notify_one();

    thread.join();
    HPX_TEST(data.woken);
}

void test_condition_notify_one_wakes_from_wait_until()
{
    wait_for_flag data;

    hpx::thread thread(
        &wait_for_flag::wait_until_without_predicate, std::ref(data));

    {
        std::unique_lock<hpx::mutex> lock(data.mutex);
        data.flag = true;
    }

    data.cond_var.notify_one();

    thread.join();
    HPX_TEST(data.woken);
}

void test_condition_notify_one_wakes_from_wait_until_with_predicate()
{
    wait_for_flag data;

    hpx::thread thread(
        &wait_for_flag::wait_until_with_predicate, std::ref(data));

    {
        std::unique_lock<hpx::mutex> lock(data.mutex);
        data.flag = true;
    }

    data.cond_var.notify_one();

    thread.join();
    HPX_TEST(data.woken);
}

void test_condition_notify_one_wakes_from_relative_wait_until_with_predicate()
{
    wait_for_flag data;

    hpx::thread thread(
        &wait_for_flag::relative_wait_until_with_predicate, std::ref(data));

    {
        std::unique_lock<hpx::mutex> lock(data.mutex);
        data.flag = true;
    }

    data.cond_var.notify_one();

    thread.join();
    HPX_TEST(data.woken);
}

void test_multiple_notify_one_calls_wakes_multiple_threads()
{
    multiple_wake_count = 0;

    hpx::thread thread1(wait_for_condvar_and_increase_count);
    hpx::thread thread2(wait_for_condvar_and_increase_count);

    hpx::this_thread::sleep_for(std::chrono::milliseconds(200));
    multiple_wake_cond.notify_one();

    hpx::thread thread3(wait_for_condvar_and_increase_count);

    hpx::this_thread::sleep_for(std::chrono::milliseconds(200));
    multiple_wake_cond.notify_one();
    multiple_wake_cond.notify_one();
    hpx::this_thread::sleep_for(std::chrono::milliseconds(200));

    {
        std::unique_lock<hpx::mutex> lk(multiple_wake_mutex);
        HPX_TEST(multiple_wake_count == 3);
    }

    thread1.join();
    thread2.join();
    thread3.join();
}

///////////////////////////////////////////////////////////////////////////////

void test_condition_notify_all_wakes_from_wait()
{
    std::vector<hpx::thread> group;

    try
    {
        wait_for_flag data;
        for (unsigned i = 0; i < 5; ++i)
        {
            group.push_back(hpx::thread(
                &wait_for_flag::wait_without_predicate, std::ref(data)));
        }

        {
            std::unique_lock<hpx::mutex> lock(data.mutex);
            data.flag = true;
        }

        data.cond_var.notify_all();

        join_all(group);
        HPX_TEST_EQ(data.woken, 5u);
    }
    catch (...)
    {
        join_all(group);
        throw;
    }
}

void test_condition_notify_all_wakes_from_wait_with_predicate()
{
    std::vector<hpx::thread> group;

    try
    {
        wait_for_flag data;
        for (unsigned i = 0; i < 5; ++i)
        {
            group.push_back(hpx::thread(
                &wait_for_flag::wait_with_predicate, std::ref(data)));
        }

        {
            std::unique_lock<hpx::mutex> lock(data.mutex);
            data.flag = true;
        }

        data.cond_var.notify_all();

        join_all(group);
        HPX_TEST_EQ(data.woken, 5u);
    }
    catch (...)
    {
        join_all(group);
        throw;
    }
}

void test_condition_notify_all_wakes_from_wait_until()
{
    std::vector<hpx::thread> group;

    try
    {
        wait_for_flag data;
        for (unsigned i = 0; i < 5; ++i)
        {
            group.push_back(hpx::thread(
                &wait_for_flag::wait_until_without_predicate, std::ref(data)));
        }

        {
            std::unique_lock<hpx::mutex> lock(data.mutex);
            data.flag = true;
        }

        data.cond_var.notify_all();

        join_all(group);
        HPX_TEST_EQ(data.woken, 5u);
    }
    catch (...)
    {
        join_all(group);
        throw;
    }
}

void test_condition_notify_all_wakes_from_wait_until_with_predicate()
{
    std::vector<hpx::thread> group;

    try
    {
        wait_for_flag data;
        for (unsigned i = 0; i < 5; ++i)
        {
            group.push_back(hpx::thread(
                &wait_for_flag::wait_until_with_predicate, std::ref(data)));
        }

        {
            std::unique_lock<hpx::mutex> lock(data.mutex);
            data.flag = true;
        }

        data.cond_var.notify_all();

        join_all(group);
        HPX_TEST_EQ(data.woken, 5u);
    }
    catch (...)
    {
        join_all(group);
        throw;
    }
}

void test_condition_notify_all_wakes_from_relative_wait_until_with_predicate()
{
    std::vector<hpx::thread> group;

    try
    {
        wait_for_flag data;
        for (unsigned i = 0; i < 5; ++i)
        {
            group.push_back(
                hpx::thread(&wait_for_flag ::relative_wait_until_with_predicate,
                    std::ref(data)));
        }

        {
            std::unique_lock<hpx::mutex> lock(data.mutex);
            data.flag = true;
        }

        data.cond_var.notify_all();

        join_all(group);
        HPX_TEST_EQ(data.woken, 5u);
    }
    catch (...)
    {
        join_all(group);
        throw;
    }
}

void test_notify_all_following_notify_one_wakes_all_threads()
{
    multiple_wake_count = 0;

    hpx::thread thread1(wait_for_condvar_and_increase_count);
    hpx::thread thread2(wait_for_condvar_and_increase_count);

    hpx::this_thread::sleep_for(std::chrono::milliseconds(200));
    multiple_wake_cond.notify_one();

    hpx::thread thread3(wait_for_condvar_and_increase_count);

    hpx::this_thread::sleep_for(std::chrono::milliseconds(200));
    multiple_wake_cond.notify_one();
    multiple_wake_cond.notify_all();
    hpx::this_thread::sleep_for(std::chrono::milliseconds(200));

    {
        std::unique_lock<hpx::mutex> lk(multiple_wake_mutex);
        HPX_TEST(multiple_wake_count == 3);
    }

    thread1.join();
    thread2.join();
    thread3.join();
}

///////////////////////////////////////////////////////////////////////////////
struct condition_test_data
{
    condition_test_data()
      : notified(0)
      , awoken(0)
    {
    }

    hpx::mutex mutex;
    hpx::condition_variable condition;
    int notified;
    int awoken;
};

void condition_test_thread(condition_test_data* data)
{
    std::unique_lock<hpx::mutex> lock(data->mutex);
    HPX_TEST(lock ? true : false);
    while (data->notified <= 0)
        data->condition.wait(lock);
    HPX_TEST(lock ? true : false);
    data->awoken++;
}

struct cond_predicate
{
    cond_predicate(int& var, int const val)
      : _var(var)
      , _val(val)
    {
    }

    bool operator()() const
    {
        return _var == _val;
    }

    int& _var;
    int _val;
};

void condition_test_waits(condition_test_data* data)
{
    std::unique_lock<hpx::mutex> lock(data->mutex);
    HPX_TEST(lock ? true : false);

    // Test wait.
    while (data->notified != 1)
        data->condition.wait(lock);
    HPX_TEST(lock ? true : false);
    HPX_TEST_EQ(data->notified, 1);
    data->awoken++;
    data->condition.notify_one();

    // Test predicate wait.
    data->condition.wait(lock, cond_predicate(data->notified, 2));
    HPX_TEST(lock ? true : false);
    HPX_TEST_EQ(data->notified, 2);
    data->awoken++;
    data->condition.notify_one();

    // Test wait_until.
    std::chrono::system_clock::time_point xt =
        std::chrono::system_clock::now() + std::chrono::milliseconds(100);
    while (data->notified != 3)
        data->condition.wait_until(lock, xt);
    HPX_TEST(lock ? true : false);
    HPX_TEST_EQ(data->notified, 3);
    data->awoken++;
    data->condition.notify_one();

    // Test predicate wait_until.
    xt = std::chrono::system_clock::now() + std::chrono::milliseconds(100);
    cond_predicate pred(data->notified, 4);
    HPX_TEST(data->condition.wait_until(lock, xt, pred));
    HPX_TEST(lock ? true : false);
    HPX_TEST(pred());
    HPX_TEST_EQ(data->notified, 4);
    data->awoken++;
    data->condition.notify_one();

    // Test predicate wait_for
    cond_predicate pred_rel(data->notified, 5);
    HPX_TEST(data->condition.wait_for(
        lock, std::chrono::milliseconds(100), pred_rel));
    HPX_TEST(lock ? true : false);
    HPX_TEST(pred_rel());
    HPX_TEST_EQ(data->notified, 5);
    data->awoken++;
    data->condition.notify_one();
}

void test_condition_waits()
{
    typedef std::unique_lock<hpx::mutex> unique_lock;

    condition_test_data data;

    hpx::thread thread(&condition_test_waits, &data);

    {
        unique_lock lock(data.mutex);
        HPX_TEST(lock ? true : false);

        {
            hpx::unlock_guard<unique_lock> ul(lock);
            hpx::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        data.notified++;
        data.condition.notify_one();
        while (data.awoken != 1)
            data.condition.wait(lock);
        HPX_TEST(lock ? true : false);
        HPX_TEST_EQ(data.awoken, 1);

        {
            hpx::unlock_guard<unique_lock> ul(lock);
            hpx::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        data.notified++;
        data.condition.notify_one();
        while (data.awoken != 2)
            data.condition.wait(lock);
        HPX_TEST(lock ? true : false);
        HPX_TEST_EQ(data.awoken, 2);

        {
            hpx::unlock_guard<unique_lock> ul(lock);
            hpx::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        data.notified++;
        data.condition.notify_one();
        while (data.awoken != 3)
            data.condition.wait(lock);
        HPX_TEST(lock ? true : false);
        HPX_TEST_EQ(data.awoken, 3);

        {
            hpx::unlock_guard<unique_lock> ul(lock);
            hpx::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        data.notified++;
        data.condition.notify_one();
        while (data.awoken != 4)
            data.condition.wait(lock);
        HPX_TEST(lock ? true : false);
        HPX_TEST_EQ(data.awoken, 4);

        {
            hpx::unlock_guard<unique_lock> ul(lock);
            hpx::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        data.notified++;
        data.condition.notify_one();
        while (data.awoken != 5)
            data.condition.wait(lock);
        HPX_TEST(lock ? true : false);
        HPX_TEST_EQ(data.awoken, 5);
    }

    thread.join();
    HPX_TEST_EQ(data.awoken, 5);
}

///////////////////////////////////////////////////////////////////////////////

bool fake_predicate()
{
    return false;
}

constexpr std::chrono::milliseconds delay(1000);
constexpr std::chrono::milliseconds timeout_resolution(100);

void test_wait_until_times_out()
{
    hpx::condition_variable cond;
    hpx::mutex m;

    std::unique_lock<hpx::mutex> lock(m);
    std::chrono::system_clock::time_point const start =
        std::chrono::system_clock::now();
    std::chrono::system_clock::time_point const timeout = start + delay;

    while (cond.wait_until(lock, timeout) == hpx::cv_status::no_timeout)
    {
    }

    std::chrono::system_clock::time_point const end =
        std::chrono::system_clock::now();
    HPX_TEST_LTE((delay - timeout_resolution).count(), (end - start).count());
}

void test_wait_until_with_predicate_times_out()
{
    hpx::condition_variable cond;
    hpx::mutex m;

    std::unique_lock<hpx::mutex> lock(m);
    std::chrono::system_clock::time_point const start =
        std::chrono::system_clock::now();
    std::chrono::system_clock::time_point const timeout = start + delay;

    bool const res = cond.wait_until(lock, timeout, fake_predicate);

    std::chrono::system_clock::time_point const end =
        std::chrono::system_clock::now();
    HPX_TEST(!res);
    HPX_TEST_LTE((delay - timeout_resolution).count(), (end - start).count());
}

void test_wait_until_with_predicate_already_expired_returns_immediately()
{
    hpx::condition_variable cond;
    hpx::mutex m;

    std::unique_lock<hpx::mutex> lock(m);
    std::chrono::system_clock::time_point const start =
        std::chrono::system_clock::now();
    std::chrono::system_clock::time_point const already_expired =
        start - std::chrono::seconds(1);

    bool const res = cond.wait_until(lock, already_expired, fake_predicate);

    std::chrono::system_clock::time_point const end =
        std::chrono::system_clock::now();
    HPX_TEST(!res);
    HPX_TEST(end - start < timeout_resolution);
}

void test_relative_wait_until_with_predicate_times_out()
{
    hpx::condition_variable cond;
    hpx::mutex m;

    std::unique_lock<hpx::mutex> lock(m);
    std::chrono::system_clock::time_point const start =
        std::chrono::system_clock::now();

    bool const res = cond.wait_for(lock, delay, fake_predicate);

    std::chrono::system_clock::time_point const end =
        std::chrono::system_clock::now();
    HPX_TEST(!res);
    HPX_TEST_LTE((delay - timeout_resolution).count(), (end - start).count());
}

void test_wait_until_relative_times_out()
{
    hpx::condition_variable cond;
    hpx::mutex m;

    std::unique_lock<hpx::mutex> lock(m);
    std::chrono::system_clock::time_point const start =
        std::chrono::system_clock::now();

    while (cond.wait_for(lock, delay) == hpx::cv_status::no_timeout)
    {
    }

    std::chrono::system_clock::time_point const end =
        std::chrono::system_clock::now();
    HPX_TEST_LTE((delay - timeout_resolution).count(), (end - start).count());
}

///////////////////////////////////////////////////////////////////////////////
// A notify_one/notify_all that fires while the predicate is still false must
// not wake the waiter up: the wait has to keep looping until the predicate
// actually becomes true.
struct spurious_wakeup_data
{
    hpx::mutex mutex;
    hpx::condition_variable cond_var;
    int stage = 0;
    bool woken = false;

    void wait_for_stage_two()
    {
        std::unique_lock<hpx::mutex> lock(mutex);
        bool const pred_satisfied = cond_var.wait_for(
            lock, std::chrono::seconds(10), [this]() { return stage == 2; });
        if (pred_satisfied)
        {
            woken = true;
        }
    }
};

void test_condition_notify_one_ignores_spurious_wakeup()
{
    spurious_wakeup_data data;

    hpx::thread thread(&spurious_wakeup_data::wait_for_stage_two, &data);

    // Give the waiting thread a chance to start waiting.
    hpx::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::chrono::steady_clock::time_point const start =
        std::chrono::steady_clock::now();

    {
        // this notification must not satisfy the predicate (stage != 2) and
        // therefore must not wake the waiter up
        std::unique_lock<hpx::mutex> lock(data.mutex);
        data.stage = 1;
    }
    data.cond_var.notify_one();

    hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
    HPX_TEST(!data.woken);

    {
        // this notification satisfies the predicate and must wake the waiter
        std::unique_lock<hpx::mutex> lock(data.mutex);
        data.stage = 2;
    }
    data.cond_var.notify_one();

    thread.join();

    std::chrono::steady_clock::time_point const end =
        std::chrono::steady_clock::now();

    HPX_TEST(data.woken);
    // the waiter must have returned well before the 10-second timeout
    HPX_TEST(end - start < std::chrono::seconds(5));
}

///////////////////////////////////////////////////////////////////////////////
// Several waiters, each waiting on their own predicate, must each be woken up
// individually as soon as their own condition becomes true, using a single
// notify_all per change.
struct distinct_predicate_data
{
    hpx::mutex mutex;
    hpx::condition_variable cond_var;
    int value = 0;
    std::atomic<unsigned> woken{0};

    void wait_for_value(int threshold)
    {
        std::unique_lock<hpx::mutex> lock(mutex);
        if (cond_var.wait_for(lock, std::chrono::seconds(10),
                [this, threshold]() { return value >= threshold; }))
        {
            ++woken;
        }
    }
};

void test_condition_notify_all_wakes_waiters_with_distinct_predicates()
{
    constexpr unsigned num_threads = 5;

    distinct_predicate_data data;

    std::vector<hpx::thread> threads;
    for (unsigned i = 1; i <= num_threads; ++i)
    {
        threads.push_back(hpx::thread(&distinct_predicate_data::wait_for_value,
            &data, static_cast<int>(i)));
    }

    // Give the waiting threads a chance to start waiting.
    hpx::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::chrono::steady_clock::time_point const start =
        std::chrono::steady_clock::now();

    for (unsigned i = 1; i <= num_threads; ++i)
    {
        {
            std::unique_lock<hpx::mutex> lock(data.mutex);
            data.value = static_cast<int>(i);
        }
        data.cond_var.notify_all();
    }

    join_all(threads);

    std::chrono::steady_clock::time_point const end =
        std::chrono::steady_clock::now();

    HPX_TEST_EQ(data.woken.load(), num_threads);
    // all waiters must have returned well before the 10-second timeout
    HPX_TEST(end - start < std::chrono::seconds(5));
}

///////////////////////////////////////////////////////////////////////////////
using hpx::program_options::options_description;
using hpx::program_options::variables_map;

int hpx_main(variables_map&)
{
    {
        test_condition_notify_one_wakes_from_wait();
        test_condition_notify_one_wakes_from_wait_with_predicate();
        test_condition_notify_one_wakes_from_wait_until();
        test_condition_notify_one_wakes_from_wait_until_with_predicate();
        test_condition_notify_one_wakes_from_relative_wait_until_with_predicate();
        test_multiple_notify_one_calls_wakes_multiple_threads();
    }
    {
        test_condition_notify_all_wakes_from_wait();
        test_condition_notify_all_wakes_from_wait_with_predicate();
        test_condition_notify_all_wakes_from_wait_until();
        test_condition_notify_all_wakes_from_wait_until_with_predicate();
        test_condition_notify_all_wakes_from_relative_wait_until_with_predicate();
        test_notify_all_following_notify_one_wakes_all_threads();
    }
    {
        test_condition_waits();
    }
    {
        test_wait_until_times_out();
        test_wait_until_with_predicate_times_out();
        test_wait_until_with_predicate_already_expired_returns_immediately();
        test_relative_wait_until_with_predicate_times_out();
        test_wait_until_relative_times_out();
    }
    {
        test_condition_notify_one_ignores_spurious_wakeup();
        test_condition_notify_all_wakes_waiters_with_distinct_predicates();
    }

    hpx::local::finalize();
    return hpx::util::report_errors();
}

///////////////////////////////////////////////////////////////////////////////
int main(int const argc, char* argv[])
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
