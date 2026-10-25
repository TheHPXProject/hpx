//  Copyright (c) 2020 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  Parts of this code were inspired by https://github.com/josuttis/jthread. The
//  original code was published by Nicolai Josuttis and Lewis Baker under the
//  Creative Commons Attribution 4.0 International License
//  (http://creativecommons.org/licenses/by/4.0/).

#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/thread.hpp>

#include <atomic>
#include <chrono>
#include <string>
#include <type_traits>
#include <utility>

////////////////////////////////////////////////////////////////////////////////
void test_jthread_without_token()
{
    // test the basic jthread API (not taking stop_token arg)
    HPX_TEST(hpx::jthread::hardware_concurrency() ==
        hpx::thread::hardware_concurrency());

    hpx::stop_token stoken;
    HPX_TEST(!stoken.stop_possible());

    {
        hpx::jthread::id id{hpx::this_thread::get_id()};
        std::atomic<bool> all_set{false};

        hpx::jthread t([&id, &all_set] {    // NOTE: no stop_token passed
            // check some values of the started thread
            id = hpx::this_thread::get_id();
            all_set.store(true);

            // wait until loop is done (no interrupt checked)
            for (int c = 9; c >= 0; --c)
            {
                hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });

        // wait until t has set all initial values
        for ([[maybe_unused]] int i = 0; !all_set.load(); ++i)
        {
            hpx::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // and check all values
        HPX_TEST(t.joinable());
        HPX_TEST(id == t.get_id());
        stoken = t.get_stop_token();
        HPX_TEST(!stoken.stop_requested());
    }    // leave scope of t without join() or detach() (signals cancellation)

    HPX_TEST(stoken.stop_requested());
}

////////////////////////////////////////////////////////////////////////////////
void test_jthread_with_token()
{
    hpx::stop_source ssource;
    hpx::stop_source origsource;
    HPX_TEST(ssource.stop_possible());
    HPX_TEST(!ssource.stop_requested());

    {
        hpx::jthread::id id{hpx::this_thread::get_id()};
        std::atomic<bool> all_set{false};
        std::atomic<bool> done{false};
        hpx::jthread t(
            [&id, &all_set, &done](hpx::stop_token stoptoken) {
                // check some values of the started thread
                id = hpx::this_thread::get_id();
                all_set.store(true);

                // wait until interrupt is signaled
                for ([[maybe_unused]] int i = 0; !stoptoken.stop_requested();
                    ++i)
                {
                    hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
                }

                done.store(true);
            },
            ssource.get_token());

        // wait until t has set all initial values
        for ([[maybe_unused]] int i = 0; !all_set.load(); ++i)
        {
            hpx::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // and check all values
        HPX_TEST(t.joinable());
        HPX_TEST(id == t.get_id());

        hpx::this_thread::sleep_for(std::chrono::milliseconds(100));

        origsource = std::move(ssource);
        ssource = t.get_stop_source();
        HPX_TEST(!ssource.stop_requested());

        auto ret = ssource.request_stop();
        HPX_TEST(ret);

        ret = ssource.request_stop();
        HPX_TEST(!ret);
        HPX_TEST(ssource.stop_requested());
        HPX_TEST(!done.load());
        HPX_TEST(!origsource.stop_requested());

        hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
        origsource.request_stop();
    }    // leave scope of t without join() or detach() (signals cancellation)

    HPX_TEST(origsource.stop_requested());
    HPX_TEST(ssource.stop_requested());
}

////////////////////////////////////////////////////////////////////////////////
void test_join()
{
    // test jthread join()
    hpx::stop_source ssource;
    HPX_TEST(ssource.stop_possible());

    {
        hpx::jthread t([](hpx::stop_token stoken) {
            // wait until interrupt is signaled (due to calling request_stop()
            // for the token)
            for ([[maybe_unused]] int i = 0; !stoken.stop_requested(); ++i)
            {
                hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
        ssource = t.get_stop_source();

        // let another thread signal cancellation after some time
        hpx::jthread t2([ssource]() mutable {
            // just wait for a while
            hpx::this_thread::sleep_for(std::chrono::milliseconds(100));

            // signal interrupt to other thread
            ssource.request_stop();
        });

        // wait for all thread to finish
        t2.join();
        HPX_TEST(!t2.joinable());
        HPX_TEST(t.joinable());

        t.join();
        HPX_TEST(!t.joinable());
    }    // leave scope of t without join() or detach() (signals cancellation)
}

////////////////////////////////////////////////////////////////////////////////
void test_detach()
{
    // test jthread detach()
    hpx::stop_source ssource;
    HPX_TEST(ssource.stop_possible());
    std::atomic<bool> finally_interrupted{false};

    {
        hpx::jthread t0;
        hpx::jthread::id id{hpx::this_thread::get_id()};
        bool is_interrupted;
        hpx::stop_token interrupt_token;
        std::atomic<bool> all_set{false};

        hpx::jthread t([&id, &is_interrupted, &interrupt_token, &all_set,
                           &finally_interrupted](hpx::stop_token stoken) {
            // check some values of the started thread
            id = hpx::this_thread::get_id();
            interrupt_token = stoken;
            is_interrupted = stoken.stop_requested();
            HPX_TEST(stoken.stop_possible());
            HPX_TEST(!is_interrupted);
            all_set.store(true);

            // wait until interrupt is signaled (due to calling request_stop()
            // for the token)
            for ([[maybe_unused]] int i = 0; !stoken.stop_requested(); ++i)
            {
                hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            finally_interrupted.store(true);
        });

        // wait until t has set all initial values
        for ([[maybe_unused]] int i = 0; !all_set.load(); ++i)
        {
            hpx::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // and check all values
        HPX_TEST(!t0.joinable());
        HPX_TEST(t.joinable());
        HPX_TEST(id == t.get_id());
        HPX_TEST(!is_interrupted);
        HPX_TEST(interrupt_token == t.get_stop_source().get_token());

        ssource = t.get_stop_source();
        HPX_TEST(interrupt_token.stop_possible());
        HPX_TEST(!interrupt_token.stop_requested());

        t.detach();
        HPX_TEST(!t.joinable());
    }    // leave scope of t without join() or detach()

    // finally signal cancellation
    HPX_TEST(!finally_interrupted.load());
    ssource.request_stop();

    // and check consequences
    HPX_TEST(ssource.stop_requested());
    for (int i = 0; !finally_interrupted.load() && i < 100; ++i)
    {
        hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    HPX_TEST(finally_interrupted.load());
}

////////////////////////////////////////////////////////////////////////////////
void test_hpx_thread()
{
    // test the extended hpx::thread API
    hpx::thread t0;
    hpx::thread::id id{hpx::this_thread::get_id()};
    std::atomic<bool> all_set{false};
    hpx::stop_source shall_die;
    hpx::thread t([&id, &all_set, shall_die = shall_die.get_token()] {
        // check some supplementary values of the started thread
        id = hpx::this_thread::get_id();
        all_set.store(true);

        // and wait until cancellation is signaled via passed token
        bool caught_exception = false;
        try
        {
            for ([[maybe_unused]] int i = 0;; ++i)
            {
                if (shall_die.stop_requested())
                {
                    throw "interrupted";
                }
                hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            HPX_TEST(false);
        }
        catch (std::exception&)
        {
            // "interrupted" not derived from std::exception
            HPX_TEST(false);
        }
        catch (char const* e)
        {
            caught_exception = true;
        }
        catch (...)
        {
            HPX_TEST(false);
        }
        HPX_TEST(caught_exception);
        HPX_TEST(shall_die.stop_requested());
    });

    // wait until t has set all initial values
    for ([[maybe_unused]] int i = 0; !all_set.load(); ++i)
    {
        hpx::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // and check all values
    HPX_TEST(id == t.get_id());

    // signal cancellation via manually installed interrupt token
    shall_die.request_stop();
    t.join();
}

//////////////////////////////////////////////////////////////////////////////
void test_temporarily_disable_token()
{
    // test exchanging the token to disable it temporarily
    enum class State
    {
        init,
        loop,
        disabled,
        restored,
        interrupted
    };

    std::atomic<State> state{State::init};
    hpx::stop_source tis;

    {
        hpx::jthread t([&state](hpx::stop_token stoken) {
            auto actToken = stoken;

            // just loop (no interrupt should occur)
            state.store(State::loop);
            try
            {
                for (int i = 0; i < 10; ++i)
                {
                    if (actToken.stop_requested())
                    {
                        throw "interrupted";
                    }
                    hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
            catch (...)
            {
                HPX_TEST(false);
            }

            // temporarily disable interrupts
            hpx::stop_token interrupt_disabled;
            std::swap(stoken, interrupt_disabled);
            state.store(State::disabled);

            // loop again until interrupt signaled to original interrupt token
            try
            {
                while (!actToken.stop_requested())
                {
                    if (stoken.stop_requested())
                    {
                        throw "interrupted";
                    }
                    hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
                }

                for (int i = 0; i < 10; ++i)
                {
                    hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
            catch (...)
            {
                HPX_TEST(false);
            }
            state.store(State::restored);

            // enable interrupts again
            std::swap(stoken, interrupt_disabled);

            // loop again (should immediately throw)
            HPX_TEST(!interrupt_disabled.stop_requested());
            try
            {
                if (actToken.stop_requested())
                {
                    throw "interrupted";
                }
            }
            catch (char const*)
            {
                state.store(State::interrupted);
            }
        });

        while (state.load() != State::disabled)
        {
            hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        hpx::this_thread::sleep_for(std::chrono::milliseconds(500));
        tis = t.get_stop_source();
    }    // leave scope of t without join() or detach() (signals cancellation)

    HPX_TEST(tis.stop_requested());
    HPX_TEST(state.load() == State::interrupted);
}

///////////////////////////////////////////////////////////////////////////////
void test_jthread_api()
{
    HPX_TEST(hpx::jthread::hardware_concurrency() ==
        hpx::thread::hardware_concurrency());

    hpx::stop_source ssource;
    HPX_TEST(ssource.stop_possible());
    HPX_TEST(ssource.get_token().stop_possible());

    hpx::stop_token stoken;
    HPX_TEST(!stoken.stop_possible());

    // thread with no callable and invalid source
    hpx::jthread t0;
    hpx::jthread::native_handle_type nh = t0.native_handle();
    HPX_TEST(
        (std::is_same<decltype(nh), hpx::thread::native_handle_type>::value));
    HPX_TEST(!t0.joinable());

    hpx::stop_source ssourceStolen{std::move(ssource)};
    // NOLINTNEXTLINE(bugprone-use-after-move)
    HPX_TEST(!ssource.stop_possible());
    HPX_TEST(ssource == t0.get_stop_source());
    HPX_TEST(ssource.get_token() == t0.get_stop_token());

    {
        hpx::jthread::id id{hpx::this_thread::get_id()};
        hpx::stop_token interrupt_token;
        std::atomic<bool> all_set{false};
        hpx::jthread t(
            [&id, &interrupt_token, &all_set](hpx::stop_token stoken) {
                // check some values of the started thread
                id = hpx::this_thread::get_id();
                interrupt_token = stoken;
                HPX_TEST(stoken.stop_possible());
                HPX_TEST(!stoken.stop_requested());
                all_set.store(true);

                // wait until interrupt is signaled (due to destructor of t)
                for ([[maybe_unused]] int i = 0; !stoken.stop_requested(); ++i)
                {
                    hpx::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            });

        // wait until t has set all initial values
        for ([[maybe_unused]] int i = 0; !all_set.load(); ++i)
        {
            hpx::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        // and check all values
        HPX_TEST(t.joinable());
        HPX_TEST(id == t.get_id());
        HPX_TEST(interrupt_token == t.get_stop_source().get_token());
        HPX_TEST(interrupt_token == t.get_stop_token());
        stoken = t.get_stop_source().get_token();
        stoken = t.get_stop_token();
        HPX_TEST(interrupt_token.stop_possible());
        HPX_TEST(!interrupt_token.stop_requested());

        // test swap()
        std::swap(t0, t);
        HPX_TEST(!t.joinable());
        HPX_TEST(hpx::stop_token{} == t.get_stop_source().get_token());
        HPX_TEST(hpx::stop_token{} == t.get_stop_token());
        HPX_TEST(t0.joinable());
        HPX_TEST(id == t0.get_id());
        HPX_TEST(interrupt_token == t0.get_stop_source().get_token());
        HPX_TEST(interrupt_token == t0.get_stop_token());

        // manual swap with move()
        auto ttmp{std::move(t0)};
        t0 = std::move(t);
        t = std::move(ttmp);
        HPX_TEST(!t0.joinable());
        HPX_TEST(hpx::stop_token{} == t0.get_stop_source().get_token());
        HPX_TEST(hpx::stop_token{} == t0.get_stop_token());
        HPX_TEST(t.joinable());
        HPX_TEST(id == t.get_id());
        HPX_TEST(interrupt_token == t.get_stop_source().get_token());
        HPX_TEST(interrupt_token == t.get_stop_token());
    }    // leave scope of t without join() or detach() (signals cancellation)

    HPX_TEST(stoken.stop_requested());
}

void test_jthread_move_assign_to_running()
{
    for (int i = 0; i != 4; ++i)
    {
        std::atomic<bool> started{false};
        std::atomic<bool> finished{false};
        hpx::stop_token interrupt_token;
        hpx::jthread t([&](hpx::stop_token stoken) {
            interrupt_token = stoken;
            started = true;
            while (!stoken.stop_requested())
            {
                hpx::this_thread::yield();
            }
            finished = true;
        });

        while (!started.load())
        {
            hpx::this_thread::yield();
        }

        t = hpx::jthread();

        HPX_TEST(finished.load());
        HPX_TEST(interrupt_token.stop_requested());
        HPX_TEST(!t.joinable());
        HPX_TEST(!t.get_stop_token().stop_possible());
    }
}

////////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    std::set_terminate([]() { HPX_TEST(false); });

    test_jthread_without_token();
    test_jthread_with_token();
    test_join();
    test_detach();
    test_hpx_thread();
    test_temporarily_disable_token();
    test_jthread_api();
    test_jthread_move_assign_to_running();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
