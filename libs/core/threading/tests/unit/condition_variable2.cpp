//  Copyright (c) 2020-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  Parts of this code were inspired by https://github.com/josuttis/jthread. The
//  original code was published by Nicolai Josuttis and Lewis Baker under the
//  Creative Commons Attribution 4.0 International License
//  (http://creativecommons.org/licenses/by/4.0/).

#include <hpx/functional.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <array>
#include <chrono>
#include <functional>
#include <mutex>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// helper to call wait()
void cv_wait(hpx::stop_token stoken, int /* id */, bool& ready,
    hpx::mutex& ready_mtx, hpx::condition_variable_any& ready_cv,
    bool notify_called)
{
    try
    {
        {
            std::unique_lock<hpx::mutex> lg{ready_mtx};
            ready_cv.wait(lg, stoken, [&ready] { return ready; });
            if (stoken.stop_requested())
            {
                throw "interrupted";
            }
        }
        HPX_TEST(!stoken.stop_requested());
        HPX_TEST(notify_called);
    }
    catch (char const* e)
    {
        HPX_TEST(!notify_called);
    }
    catch (...)
    {
        HPX_TEST(false);
    }
}

///////////////////////////////////////////////////////////////////////////////
void test_cv(bool call_notify)
{
    bool ready = false;
    hpx::mutex ready_mtx;
    hpx::condition_variable_any ready_cv;

    {
        hpx::jthread t1(
            [&ready, &ready_mtx, &ready_cv, call_notify](hpx::stop_token it) {
                {
                    std::unique_lock<hpx::mutex> lg{ready_mtx};
                    while (!it.stop_requested() && !ready)
                    {
                        ready_cv.wait_for(lg, std::chrono::milliseconds(100));
                    }
                }
                HPX_TEST(call_notify != it.stop_requested());
            });

        hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (call_notify)
        {
            {
                std::lock_guard<hpx::mutex> lg(ready_mtx);
                ready = true;
            }    // release lock

            ready_cv.notify_one();
            t1.join();
        }
    }    // leave scope of t1 without join() or detach() (signals cancellation)
}

///////////////////////////////////////////////////////////////////////////////
void test_cv_pred(bool call_notify)
{
    bool ready = false;
    hpx::mutex ready_mtx;
    hpx::condition_variable_any ready_cv;

    {
        hpx::jthread t1(
            [&ready, &ready_mtx, &ready_cv, call_notify](hpx::stop_token st) {
                try
                {
                    std::unique_lock<hpx::mutex> lg{ready_mtx};
                    ready_cv.wait(lg, st, [&ready] { return ready; });
                    if (st.stop_requested())
                    {
                        throw "interrupted";
                    }
                    HPX_TEST(call_notify);
                }
                catch (std::exception const&)
                {
                    // should be no std::exception
                    HPX_TEST(false);
                }
                catch (char const*)
                {
                    HPX_TEST(!call_notify);
                }
                catch (...)
                {
                    HPX_TEST(false);
                }
                HPX_TEST(call_notify != st.stop_requested());
            });

        hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (call_notify)
        {
            {
                std::lock_guard<hpx::mutex> lg(ready_mtx);
                ready = true;
            }    // release lock

            ready_cv.notify_one();
            t1.join();
        }
    }    // leave scope of t1 without join() or detach() (signals cancellation)
}

///////////////////////////////////////////////////////////////////////////////
void test_cv_thread_no_pred(bool call_notify)
{
    bool ready = false;
    hpx::mutex ready_mtx;
    hpx::condition_variable_any ready_cv;

    hpx::stop_source is;
    {
        hpx::thread t1([&ready, &ready_mtx, &ready_cv, st = is.get_token(),
                           call_notify] {
            {
                std::unique_lock<hpx::mutex> lg{ready_mtx};
                bool ret = ready_cv.wait(lg, st, [&ready] { return ready; });
                if (ret)
                {
                    HPX_TEST(!st.stop_requested());
                    HPX_TEST(call_notify);
                }
                else if (st.stop_requested())
                {
                    HPX_TEST(!call_notify);
                }
            }
        });

        hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
        HPX_TEST(!is.stop_requested());
        hpx::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (call_notify)
        {
            {
                std::lock_guard<hpx::mutex> lg(ready_mtx);
                ready = true;
            }    // release lock

            ready_cv.notify_one();
        }
        else
        {
            is.request_stop();
        }
        t1.join();
    }    // leave scope of t1 without join() or detach() (signals cancellation)
}

///////////////////////////////////////////////////////////////////////////////
void test_cv_thread_pred(bool call_notify)
{
    bool ready = false;
    hpx::mutex ready_mtx;
    hpx::condition_variable_any ready_cv;

    hpx::stop_source is;
    {
        hpx::thread t1(
            [&ready, &ready_mtx, &ready_cv, st = is.get_token(), call_notify] {
                bool ret;
                {
                    std::unique_lock<hpx::mutex> lg{ready_mtx};
                    ret = ready_cv.wait(lg, st, [&ready] { return ready; });
                    if (ret)
                    {
                        HPX_TEST(!st.stop_requested());
                    }
                    else
                    {
                        HPX_TEST(st.stop_requested());
                    }
                }
                HPX_TEST(call_notify != st.stop_requested());
            });

        hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
        HPX_TEST(!is.stop_requested());
        hpx::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (call_notify)
        {
            {
                std::lock_guard<hpx::mutex> lg(ready_mtx);
                ready = true;
            }    // release lock

            ready_cv.notify_one();
        }
        else
        {
            is.request_stop();
        }
        t1.join();
    }    // leave scope of t1 without join() or detach() (signals cancellation)
}

///////////////////////////////////////////////////////////////////////////////
void test_minimal_wait(int sec)
{
    // duration until interrupt is called
    auto duration = std::chrono::seconds{sec};

    try
    {
        bool ready = false;
        hpx::mutex ready_mtx;
        hpx::condition_variable_any ready_cv;

        {
            hpx::jthread t1(
                [&ready, &ready_mtx, &ready_cv, duration](hpx::stop_token st) {
                    try
                    {
                        auto t0 = std::chrono::steady_clock::now();
                        {
                            std::unique_lock<hpx::mutex> lg{ready_mtx};
                            ready_cv.wait(lg, st, [&ready] { return ready; });
                        }
                        HPX_TEST(std::chrono::steady_clock::now() <
                            t0 + duration + std::chrono::seconds(1));
                    }
                    catch (...)
                    {
                        HPX_TEST(false);
                    }
                });

            hpx::this_thread::sleep_for(duration);
        }    // leave scope of t1 without join() or detach() (signals cancellation)
    }
    catch (...)
    {
        HPX_TEST(false);
    }
}

///////////////////////////////////////////////////////////////////////////////
void test_minimal_wait_for(int sec1, int sec2)
{
    // test the basic timed CV wait API
    auto dur_int = std::chrono::seconds{sec1};
    auto dur_wait = std::chrono::seconds{sec2};

    try
    {
        bool ready = false;
        hpx::mutex ready_mtx;
        hpx::condition_variable_any ready_cv;

        {
            hpx::jthread t1([&ready, &ready_mtx, &ready_cv, dur_int, dur_wait](
                                hpx::stop_token st) {
                try
                {
                    auto t0 = std::chrono::steady_clock::now();
                    {
                        std::unique_lock<hpx::mutex> lg{ready_mtx};
                        ready_cv.wait_for(
                            lg, st, dur_wait, [&ready] { return ready; });
                    }
                    HPX_TEST(std::chrono::steady_clock::now() <
                        t0 + dur_int + std::chrono::seconds(5));
                    HPX_TEST(std::chrono::steady_clock::now() <
                        t0 + dur_wait + std::chrono::seconds(5));
                }
                catch (...)
                {
                    HPX_TEST(false);
                }
            });

            hpx::this_thread::sleep_for(dur_int);
        }    // leave scope of t1 without join() or detach() (signals cancellation)
    }
    catch (...)
    {
        HPX_TEST(false);
    }
}

///////////////////////////////////////////////////////////////////////////////
template <typename Duration>
void test_timed_cv(
    bool call_notify, bool /* call_interrupt */, Duration duration)
{
    // test the basic jthread API
    bool ready = false;
    hpx::mutex ready_mtx;
    hpx::condition_variable_any ready_cv;

    {
        hpx::jthread t1([&ready, &ready_mtx, &ready_cv, call_notify, duration](
                            hpx::stop_token st) {
            auto t0 = std::chrono::steady_clock::now();
            int times_done{0};
            while (times_done < 3)
            {
                {
                    std::unique_lock<hpx::mutex> lg{ready_mtx};
                    auto ret = ready_cv.wait_for(
                        lg, st, duration, [&ready] { return ready; });
                    if (duration > std::chrono::seconds(5))
                    {
                        HPX_TEST(
                            std::chrono::steady_clock::now() < t0 + duration);
                    }
                    if (ret)
                    {
                        HPX_TEST(ready);
                        HPX_TEST(!st.stop_requested());
                        HPX_TEST(call_notify);
                        ++times_done;
                    }
                    else if (st.stop_requested())
                    {
                        HPX_TEST(!ready);
                        HPX_TEST(!call_notify);
                        ++times_done;
                    }
                }
            }
        });

        HPX_TEST(!t1.get_stop_source().stop_requested());

        if (call_notify)
        {
            {
                std::lock_guard<hpx::mutex> lg(ready_mtx);
                ready = true;
            }    // release lock

            hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
            ready_cv.notify_one();
        }
        else
        {
            t1.request_stop();
        }
    }    // leave scope of t1 without join() or detach() (signals cancellation)
}

///////////////////////////////////////////////////////////////////////////////
template <typename Duration>
void test_timed_wait(bool call_notify, bool call_interrupt, Duration duration)
{
    // test the basic jthread API

    // ready_ instead of ready to not clash with state::ready and work around
    // potential bug in GCC 10
    bool ready_ = false;
    hpx::mutex ready_mtx;
    hpx::condition_variable_any ready_cv;

    enum class state
    {
        loop,
        ready,
        interrupted
    };

    state t1_feedback{state::loop};
    {
        hpx::jthread t1([&ready_, &ready_mtx, &ready_cv, call_notify, duration,
                            &t1_feedback](hpx::stop_token st) {
            auto t0 = std::chrono::steady_clock::now();
            int times_done{0};
            while (times_done < 3)
            {
                try
                {
                    std::unique_lock<hpx::mutex> lg{ready_mtx};
                    auto ret = ready_cv.wait_for(
                        lg, st, duration, [&ready_] { return ready_; });
                    if (st.stop_requested())
                    {
                        throw "interrupted";
                    }
                    if (duration > std::chrono::seconds(5))
                    {
                        HPX_TEST(
                            std::chrono::steady_clock::now() < t0 + duration);
                    }

                    if (ret)
                    {
                        t1_feedback = state::ready;
                        HPX_TEST(ready_);
                        HPX_TEST(!st.stop_requested());
                        HPX_TEST(call_notify);
                        ++times_done;
                    }
                }
                catch (char const*)
                {
                    t1_feedback = state::interrupted;
                    HPX_TEST(!ready_);
                    HPX_TEST(!call_notify);
                    ++times_done;
                    if (times_done >= 3)
                    {
                        return;
                    }
                }
                catch (...)
                {
                    HPX_TEST(false);
                }
            }
        });

        hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
        HPX_TEST(!t1.get_stop_source().stop_requested());
        hpx::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (call_notify)
        {
            {
                std::lock_guard<hpx::mutex> lg(ready_mtx);
                ready_ = true;
            }    // release lock

            auto t0 = std::chrono::steady_clock::now();
            ready_cv.notify_one();
            while (t1_feedback != state::ready)
            {
                hpx::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
            HPX_TEST(std::chrono::steady_clock::now() <
                t0 + std::chrono::seconds(5));
        }
        else if (call_interrupt)
        {
            auto t0 = std::chrono::steady_clock::now();
            t1.request_stop();
            while (t1_feedback != state::interrupted)
            {
                hpx::this_thread::sleep_for(std::chrono::milliseconds(200));
            }
            HPX_TEST(std::chrono::steady_clock::now() <
                t0 + std::chrono::seconds(5));
        }
    }    // leave scope of t1 without join() or detach() (signals cancellation)

    auto t0 = std::chrono::steady_clock::now();
    while (t1_feedback == state::loop)
    {
        hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    HPX_TEST(std::chrono::steady_clock::now() < t0 + std::chrono::seconds(5));
}

///////////////////////////////////////////////////////////////////////////////
template <int NumExtraCV>
void test_many_cvs(bool call_notify, bool call_interrupt)
{
    {
        // thread t0 with CV:
        bool ready = false;
        hpx::mutex ready_mtx;
        hpx::condition_variable_any ready_cv;

        // don't forget to initialize with {} here !!!
        std::array<bool, NumExtraCV> arr_ready{};
        std::array<hpx::mutex, NumExtraCV> arr_ready_mtx{};
        std::array<hpx::condition_variable_any, NumExtraCV> arr_ready_cv{};
        std::vector<hpx::jthread> vthreads_deferred;

        hpx::jthread t0(hpx::bind_back(cv_wait, 0, std::ref(ready),
            std::ref(ready_mtx), std::ref(ready_cv), call_notify));
        {
            auto t0ssource = t0.get_stop_source();
            hpx::this_thread::sleep_for(std::chrono::milliseconds(100));

            // starts thread concurrently calling request_stop() for the same
            // token
            std::vector<hpx::jthread> vthreads;
            for (int idx = 0; idx < NumExtraCV; ++idx)
            {
                hpx::this_thread::sleep_for(std::chrono::microseconds(100));
                hpx::jthread t(
                    [idx, t0stoken = t0ssource.get_token(), &arr_ready,
                        &arr_ready_mtx, &arr_ready_cv, call_notify] {
                        // use interrupt token of t0 instead
                        // NOTE: disables signaling interrupts directly to the thread
                        cv_wait(t0stoken, idx + 1, arr_ready[idx],
                            arr_ready_mtx[idx], arr_ready_cv[idx], call_notify);
                    });
                vthreads.push_back(std::move(t));
            }

            hpx::this_thread::sleep_for(std::chrono::milliseconds(100));

            if (call_notify)
            {
                {
                    std::lock_guard<hpx::mutex> lg(ready_mtx);
                    ready = true;
                }    // release lock

                ready_cv.notify_one();
                t0.join();

                for (int idx = 0; idx < NumExtraCV; ++idx)
                {
                    {
                        std::lock_guard<hpx::mutex> lg(arr_ready_mtx[idx]);
                        arr_ready[idx] = true;
                        arr_ready_cv[idx].notify_one();
                    }    // release lock
                }
            }
            else if (call_interrupt)
            {
                t0.request_stop();
            }
            else
            {
                // Move ownership of the threads to a scope that will
                // destruct after thread t0 has destructed (and thus
                // signaled cancellation and joined) but before the
                // condition_variable/mutex/ready-flag objects that
                // they reference have been destroyed.
                vthreads_deferred = std::move(vthreads);
            }
        }    // leave scope of additional threads (already notified/interrupted
        // or detached)
    }    // leave scope of t0 without join() or detach() (signals cancellation)
}

///////////////////////////////////////////////////////////////////////////////
// [thread.condition.condvarany] returns pred() after stop is requested.
// Requesting stop from the first predicate call must not skip that final
// evaluation, and must not report false when the predicate is then true.
template <typename Wait>
void check_cv_any_stop_from_pred(
    hpx::condition_variable_any& cv, Wait&& wait, bool become_ready)
{
    hpx::stop_source source;
    int calls = 0;
    bool ready = false;

    bool const result = wait(source.get_token(), [&] {
        ++calls;
        if (calls == 1)
        {
            ready = become_ready;
            source.request_stop();
            return false;
        }
        // The final evaluation still holds the user lock. Notifying this
        // condition variable must not deadlock on its internal mutex.
        cv.notify_all();
        return ready;
    });

    HPX_TEST_EQ(result, become_ready);
    HPX_TEST_EQ(calls, 2);
    HPX_TEST(source.stop_requested());
}

void test_cv_any_stop_from_pred()
{
    hpx::mutex mtx;
    hpx::condition_variable_any cv;
    std::unique_lock<hpx::mutex> lock(mtx);

    check_cv_any_stop_from_pred(
        cv,
        [&](hpx::stop_token st, auto&& pred) {
            return cv.wait(lock, std::move(st), std::move(pred));
        },
        true);
    check_cv_any_stop_from_pred(
        cv,
        [&](hpx::stop_token st, auto&& pred) {
            return cv.wait(lock, std::move(st), std::move(pred));
        },
        false);
    check_cv_any_stop_from_pred(
        cv,
        [&](hpx::stop_token st, auto&& pred) {
            return cv.wait_for(
                lock, std::move(st), std::chrono::seconds(60), std::move(pred));
        },
        true);
    check_cv_any_stop_from_pred(
        cv,
        [&](hpx::stop_token st, auto&& pred) {
            return cv.wait_until(lock, std::move(st),
                std::chrono::steady_clock::now() + std::chrono::seconds(60),
                std::move(pred));
        },
        true);

    HPX_TEST(lock.owns_lock());
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    std::set_terminate([]() { HPX_TEST(false); });

    try
    {
        test_cv_any_stop_from_pred();

        test_minimal_wait(0);
        test_minimal_wait(1);
        test_minimal_wait_for(0, 0);
        test_minimal_wait_for(0, 2);    // 0s for interrupt, 2s for wait
        test_minimal_wait_for(2, 0);    // 2s for interrupt, 0s for wait
        test_minimal_wait_for(1, 3);    // 1s for interrupt, 3s for wait
        test_minimal_wait_for(3, 1);    // 3s for interrupt, 1s for wait

        test_cv_thread_no_pred(false);    // signal cancellation
        test_cv_thread_no_pred(true);     // call notify()

        test_cv(false);    // signal cancellation
        test_cv(true);     // call notify()

        test_cv_pred(false);    // signal cancellation
        test_cv_pred(true);     // call notify()

        test_cv_thread_pred(false);    // signal cancellation
        test_cv_thread_pred(true);     // call notify()

        // call notify(), don't call request_stop()
        test_timed_cv(true, false, std::chrono::milliseconds(200));
        // don't call notify, call request_stop()
        test_timed_cv(false, true, std::chrono::milliseconds(200));
        // don't call notify, don't call request_stop()
        test_timed_cv(false, false, std::chrono::milliseconds(200));
        //         // call notify(), don't call request_stop()
        //         test_timed_cv(true, false, 60s);
        //         // don't call notify, call request_stop()
        //         test_timed_cv(false, true, 60s);
        //         // don't call notify, don't call request_stop()
        //         test_timed_cv(false, false, 60s);

        // call notify(), don't call request_stop()
        test_timed_wait(true, false, std::chrono::milliseconds(200));
        // don't call notify, call request_stop()
        test_timed_wait(false, true, std::chrono::milliseconds(200));
        // don't call notify, don't call request_stop()
        test_timed_wait(false, false, std::chrono::milliseconds(200));
        // call notify(), don't call request_stop()
        //         test_timed_wait(true, false, 60s);
        //         // don't call notify, call request_stop()
        //         test_timed_wait(false, true, 60s);
        //         // don't call notify, don't call request_stop()
        //         test_timed_wait(false, false, 60s);

        // call notify(), don't call request_stop()
        test_many_cvs<9>(true, false);
        // don't call notify, call request_stop()
        test_many_cvs<9>(false, true);
        // don't call notify, don't call request_stop() (implicit interrupt)
        test_many_cvs<9>(false, false);
    }
    catch (...)
    {
        HPX_TEST(false);
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
