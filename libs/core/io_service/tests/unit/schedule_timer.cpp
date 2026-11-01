//  Copyright (c) 2026 Pratyksh Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/io_service.hpp>
#include <hpx/modules/testing.hpp>

#include <asio/io_context.hpp>

#include <atomic>
#include <chrono>
#include <exception>
#include <semaphore>
#include <utility>

namespace ex = hpx::execution::experimental;

namespace {
    struct completion_state
    {
        bool value = false;
        bool error = false;
        bool stopped = false;
    };

    struct timer_receiver
    {
        using receiver_concept = ex::receiver_t;

        completion_state& state;
        ex::inplace_stop_token token;

        void set_value() && noexcept
        {
            state.value = true;
        }

        void set_error(std::exception_ptr) && noexcept
        {
            state.error = true;
        }

        void set_stopped() && noexcept
        {
            state.stopped = true;
        }

        auto get_env() const noexcept
        {
            return ex::prop{ex::get_stop_token_t{}, token};
        }
    };

    struct plain_timer_receiver
    {
        using receiver_concept = ex::receiver_t;

        completion_state& state;

        void set_value() && noexcept
        {
            state.value = true;
        }

        void set_error(std::exception_ptr) && noexcept
        {
            state.error = true;
        }

        void set_stopped() && noexcept
        {
            state.stopped = true;
        }
    };

    enum class completion_kind
    {
        none,
        value,
        error,
        stopped,
        duplicate
    };

    struct concurrent_completion_state
    {
        void complete(completion_kind const kind) noexcept
        {
            completion_kind expected = completion_kind::none;
            if (!result.compare_exchange_strong(expected, kind))
            {
                result.store(completion_kind::duplicate);
                return;
            }
            done.release();
        }

        std::atomic<completion_kind> result{completion_kind::none};
        std::binary_semaphore done{0};
    };

    struct concurrent_timer_receiver
    {
        using receiver_concept = ex::receiver_t;

        concurrent_completion_state& state;
        ex::inplace_stop_token token;

        void set_value() && noexcept
        {
            state.complete(completion_kind::value);
        }

        void set_error(std::exception_ptr) && noexcept
        {
            state.complete(completion_kind::error);
        }

        void set_stopped() && noexcept
        {
            state.complete(completion_kind::stopped);
        }

        auto get_env() const noexcept
        {
            return ex::prop{ex::get_stop_token_t{}, token};
        }
    };

    void test_schedule_after()
    {
        ::asio::io_context context;
        completion_state state;
        ex::inplace_stop_source stop_source;

        auto sender = hpx::io::experimental::schedule_after(
            context, std::chrono::milliseconds(1));

        static_assert(ex::sender_in<decltype(sender)>);
        static_assert(ex::sender_of<decltype(sender), ex::set_value_t()>);

        auto operation = ex::connect(
            std::move(sender), timer_receiver{state, stop_source.get_token()});
        ex::start(operation);

        HPX_TEST(context.run() != 0);
        HPX_TEST(state.value);
        HPX_TEST(!state.error);
        HPX_TEST(!state.stopped);
    }

    void test_schedule_at()
    {
        ::asio::io_context context;
        completion_state state;
        ex::inplace_stop_source stop_source;

        auto operation = ex::connect(hpx::io::experimental::schedule_at(context,
                                         std::chrono::steady_clock::now()),
            timer_receiver{state, stop_source.get_token()});
        ex::start(operation);

        HPX_TEST(context.run() != 0);
        HPX_TEST(state.value);
        HPX_TEST(!state.error);
        HPX_TEST(!state.stopped);
    }

    void test_default_environment()
    {
        ::asio::io_context context;
        completion_state state;

        auto operation = ex::connect(hpx::io::experimental::schedule_after(
                                         context, std::chrono::milliseconds(1)),
            plain_timer_receiver{state});
        ex::start(operation);

        HPX_TEST(context.run() != 0);
        HPX_TEST(state.value);
        HPX_TEST(!state.error);
        HPX_TEST(!state.stopped);
    }

    void test_composition()
    {
        ::asio::io_context context;
        completion_state state;
        bool continuation_called = false;

        auto sender = hpx::io::experimental::schedule_after(
                          context, std::chrono::milliseconds(1)) |
            ex::then([&continuation_called]() noexcept {
                continuation_called = true;
            });
        auto operation =
            ex::connect(std::move(sender), plain_timer_receiver{state});
        ex::start(operation);

        HPX_TEST(context.run() != 0);
        HPX_TEST(continuation_called);
        HPX_TEST(state.value);
        HPX_TEST(!state.error);
        HPX_TEST(!state.stopped);
    }

    void test_stop_before_start()
    {
        ::asio::io_context context;
        completion_state state;
        ex::inplace_stop_source stop_source;
        stop_source.request_stop();

        auto operation = ex::connect(hpx::io::experimental::schedule_after(
                                         context, std::chrono::hours(1)),
            timer_receiver{state, stop_source.get_token()});
        ex::start(operation);

        HPX_TEST(context.run() != 0);
        HPX_TEST(!state.value);
        HPX_TEST(!state.error);
        HPX_TEST(state.stopped);
    }

    void test_stop_after_start()
    {
        ::asio::io_context context;
        completion_state state;
        ex::inplace_stop_source stop_source;

        auto operation = ex::connect(hpx::io::experimental::schedule_after(
                                         context, std::chrono::hours(1)),
            timer_receiver{state, stop_source.get_token()});
        ex::start(operation);
        stop_source.request_stop();

        HPX_TEST(context.run() != 0);
        HPX_TEST(!state.value);
        HPX_TEST(!state.error);
        HPX_TEST(state.stopped);
    }

    void test_concurrent_stop()
    {
        hpx::util::io_service_pool pool(1);
        HPX_TEST(pool.run(false));

        for (int i = 0; i != 100; ++i)
        {
            concurrent_completion_state state;
            ex::inplace_stop_source stop_source;

            auto operation =
                ex::connect(hpx::io::experimental::schedule_after(
                                pool.get_io_service(), std::chrono::hours(1)),
                    concurrent_timer_receiver{state, stop_source.get_token()});
            ex::start(operation);
            stop_source.request_stop();
            state.done.acquire();

            HPX_TEST(state.result.load() == completion_kind::stopped);
        }

        pool.stop();
        pool.join();
    }
}    // namespace

int main()
{
    test_schedule_after();
    test_schedule_at();
    test_default_environment();
    test_composition();
    test_stop_before_start();
    test_stop_after_start();
    test_concurrent_stop();

    return hpx::util::report_errors();
}
