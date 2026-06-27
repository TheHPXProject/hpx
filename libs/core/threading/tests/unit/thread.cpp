// Copyright (C) 2012-2022 Hartmut Kaiser
// Copyright (C) 2001-2003 William E. Kempf
// Copyright (C) 2008 Anthony Williams
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/barrier.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <chrono>
#include <functional>
#include <memory>

using hpx::program_options::options_description;
using hpx::program_options::variables_map;

///////////////////////////////////////////////////////////////////////////////
inline void set_description(char const* test_name)
{
    hpx::threads::set_thread_description(
        hpx::threads::get_self_id(), test_name);
}

///////////////////////////////////////////////////////////////////////////////
template <typename Clock, typename Duration>
inline int time_cmp(std::chrono::time_point<Clock, Duration> const& xt1,
    std::chrono::time_point<Clock, Duration> const& xt2)
{
    if (xt1 == xt2)
        return 0;
    return xt1 > xt2 ? 1 : -1;
}

template <typename Clock, typename Duration, typename Rep, typename Period>
inline bool in_range(std::chrono::time_point<Clock, Duration> const& xt,
    std::chrono::duration<Rep, Period> const& d)
{
    std::chrono::time_point<Clock, Duration> const now = Clock::now();
    std::chrono::time_point<Clock, Duration> const mint = now - d;
    return time_cmp(xt, mint) >= 0 && time_cmp(xt, now) <= 0;
}

///////////////////////////////////////////////////////////////////////////////
template <typename F>
void timed_test(F func, int /*secs*/)
{
    hpx::thread thrd(func);
    thrd.join();

    // FIXME: implement execution monitor to verify in-time execution and to
    //        prevent deadlocks
}

///////////////////////////////////////////////////////////////////////////////
int test_value = 0;

void simple_thread()
{
    test_value = 999;
}

void comparison_thread(hpx::thread::id parent)
{
    hpx::thread::id const my_id = hpx::this_thread::get_id();
    HPX_TEST_NEQ(my_id, parent);

    hpx::thread::id const my_id2 = hpx::this_thread::get_id();
    HPX_TEST_EQ(my_id, my_id2);

    hpx::thread::id const no_thread_id = hpx::thread::id();
    HPX_TEST_NEQ(my_id, no_thread_id);
}

///////////////////////////////////////////////////////////////////////////////
void test_sleep()
{
    set_description("test_sleep");

    std::chrono::system_clock::time_point const now =
        std::chrono::system_clock::now();
    hpx::this_thread::sleep_for(std::chrono::seconds(3));

    // Ensure it's in a range instead of checking actual equality due to time
    // lapse
    HPX_TEST(in_range(now, std::chrono::seconds(4)));    //-V112
}

///////////////////////////////////////////////////////////////////////////////
void do_test_creation()
{
    test_value = 0;
    hpx::thread thrd(&simple_thread);
    thrd.join();
    HPX_TEST_EQ(test_value, 999);
}

void test_creation()
{
    set_description("test_creation");
    timed_test(&do_test_creation, 1);
}

///////////////////////////////////////////////////////////////////////////////
void do_test_id_comparison()
{
    hpx::thread::id const self = hpx::this_thread::get_id();
    hpx::thread thrd(&comparison_thread, self);
    thrd.join();
}

void test_id_comparison()
{
    set_description("test_id_comparison");
    timed_test(&do_test_id_comparison, 1);
}

///////////////////////////////////////////////////////////////////////////////
void interruption_point_thread(hpx::spinlock* m, bool* failed)
{
    std::unique_lock<hpx::spinlock> lk(*m);
    [[maybe_unused]] hpx::util::ignore_while_checking il(&lk);

    hpx::this_thread::interruption_point();
    *failed = true;
}

void do_test_thread_interrupts_at_interruption_point()
{
    hpx::spinlock m;
    bool failed = false;
    std::unique_lock<hpx::spinlock> lk(m);
    hpx::thread thrd(&interruption_point_thread, &m, &failed);
    thrd.interrupt();
    lk.unlock();
    thrd.join();
    HPX_TEST(!failed);
}

void test_thread_interrupts_at_interruption_point()
{
    set_description("test_thread_interrupts_at_interruption_point");
    timed_test(&do_test_thread_interrupts_at_interruption_point, 1);
}

///////////////////////////////////////////////////////////////////////////////
void disabled_interruption_point_thread(
    hpx::spinlock* m, hpx::barrier<>* b, bool* failed)
{
    hpx::this_thread::disable_interruption dc;
    b->arrive_and_wait();
    try
    {
        std::lock_guard<hpx::spinlock> lk(*m);
        hpx::this_thread::interruption_point();
        *failed = false;
    }
    catch (...)
    {
        b->arrive_and_wait();
        throw;
    }
    b->arrive_and_wait();
}

void do_test_thread_no_interrupt_if_interrupts_disabled_at_interruption_point()
{
    hpx::spinlock m;
    hpx::barrier<> b(2);
    bool caught = false;
    bool failed = true;
    hpx::thread thrd(&disabled_interruption_point_thread, &m, &b, &failed);

    // Make sure the test thread has been started and marked itself
    // to disable interrupts.
    b.arrive_and_wait();
    try
    {
        std::unique_lock<hpx::spinlock> lk(m);
        [[maybe_unused]] hpx::util::ignore_while_checking il(&lk);

        thrd.interrupt();
    }
    catch (hpx::exception& e)
    {
        HPX_TEST_EQ(e.get_error(), hpx::error::thread_not_interruptable);
        caught = true;
    }

    b.arrive_and_wait();

    thrd.join();
    HPX_TEST(!failed);
    HPX_TEST(caught);
}

void test_thread_no_interrupt_if_interrupts_disabled_at_interruption_point()
{
    set_description("test_thread_no_interrupt_if_interrupts_disabled_at\
                    _interruption_point");
    timed_test(
        &do_test_thread_no_interrupt_if_interrupts_disabled_at_interruption_point,
        1);
}

///////////////////////////////////////////////////////////////////////////////
struct non_copyable_callable
{
    unsigned value;

    non_copyable_callable()
      : value(0)
    {
    }

    non_copyable_callable(non_copyable_callable const&) = delete;
    non_copyable_callable& operator=(non_copyable_callable const&) = delete;

    void operator()()
    {
        value = 999;
    }
};

void do_test_creation_through_reference_wrapper()
{
    non_copyable_callable f;

    hpx::thread thrd(std::ref(f));
    thrd.join();
    HPX_TEST_EQ(f.value, 999u);
}

void test_creation_through_reference_wrapper()
{
    set_description("test_creation_through_reference_wrapper");
    timed_test(&do_test_creation_through_reference_wrapper, 1);
}

///////////////////////////////////////////////////////////////////////////////
// struct long_running_thread
// {
//     std::condition_variable cond;
//     std::mutex mut;
//     bool done;
//
//     long_running_thread()
//       : done(false)
//     {}
//
//     void operator()()
//     {
//         std::lock_guard<std::mutex> lk(mut);
//         while(!done)
//         {
//             cond.wait(lk);
//         }
//     }
// };
//
// void do_test_timed_join()
// {
//     long_running_thread f;
//     hpx::thread thrd(std::ref(f));
//     HPX_TEST(thrd.joinable());
//     std::chrono::system_clock::time_point xt =
//         std::chrono::system_clock::now()
//       + std::chrono::seconds(3);
//     bool const joined=thrd.timed_join(xt);
//     HPX_TEST(in_range(xt, std::chrono::seconds(2)));
//     HPX_TEST(!joined);
//     HPX_TEST(thrd.joinable());
//     {
//         std::lock_guard<std::mutex> lk(f.mut);
//         f.done=true;
//         f.cond.notify_one();
//     }
//
//     xt = std::chrono::system_clock::now()
//       + std::chrono::seconds(3);
//     bool const joined2=thrd.timed_join(xt);
//     std::chrono::system_clock::time_point const now =
//         std::chrono::system_clock::now();
//     HPX_TEST(xt>now);
//     HPX_TEST(joined2);
//     HPX_TEST(!thrd.joinable());
// }
//
// void test_timed_join()
// {
//     timed_test(&do_test_timed_join, 10);
// }

void simple_sync_thread(
    std::shared_ptr<hpx::barrier<>> b1, std::shared_ptr<hpx::barrier<>> b2)
{
    b1->arrive_and_wait();    // wait for both threads to be started
    // ... do nothing
    b2->arrive_and_wait();    // wait for the tests to be completed
}

void test_swap()
{
    set_description("test_swap");

    std::shared_ptr<hpx::barrier<>> b1 = std::make_shared<hpx::barrier<>>(3);
    std::shared_ptr<hpx::barrier<>> b2 = std::make_shared<hpx::barrier<>>(3);

    hpx::thread t1(&simple_sync_thread, b1, b2);
    hpx::thread t2(&simple_sync_thread, b1, b2);

    b1->arrive_and_wait();    // wait for both threads to be started

    hpx::thread::id id1 = t1.get_id();
    hpx::thread::id id2 = t2.get_id();

    t1.swap(t2);
    HPX_TEST_EQ(t1.get_id(), id2);
    HPX_TEST_EQ(t2.get_id(), id1);

    swap(t1, t2);
    HPX_TEST_EQ(t1.get_id(), id1);
    HPX_TEST_EQ(t2.get_id(), id2);

    b2->arrive_and_wait();    // wait for the tests to be completed

    t1.join();
    t2.join();
}

void test_double_join()
{
    set_description("test_double_join");

    hpx::thread t([]() {});
    t.join();

    bool threw = true;
    bool caught = false;
    try
    {
        t.join();
        threw = false;
    }
    catch (hpx::exception& e)
    {
        HPX_TEST_EQ(e.get_error(), hpx::error::invalid_status);
        caught = true;
    }

    HPX_TEST(threw);
    HPX_TEST(caught);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(variables_map&)
{
    {
        test_sleep();
        test_creation();
        test_id_comparison();
        test_thread_interrupts_at_interruption_point();
        test_thread_no_interrupt_if_interrupts_disabled_at_interruption_point();
        test_creation_through_reference_wrapper();
        test_swap();
        test_double_join();
    }

    hpx::local::finalize();
    return hpx::util::report_errors();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    // Configure application-specific options
    options_description cmdline("Usage: " HPX_APPLICATION_STRING " [options]");

    // Initialize and run HPX
    hpx::local::init_params init_args;
    init_args.desc_cmdline = cmdline;

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
