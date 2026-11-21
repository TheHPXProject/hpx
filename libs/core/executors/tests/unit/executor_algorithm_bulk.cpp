//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace ex = hpx::execution::experimental;

void test_sequential_bulk()
{
    hpx::execution::sequenced_executor exec;
    std::atomic<int> call_count{0};

    // Obtain a P2300 scheduler from the executor via query(get_scheduler_t{})
    auto sched = exec.query(ex::get_scheduler_t{});

    auto snd = ex::schedule(sched) | ex::bulk(10, [&](int i) {
        (void) i;
        ++call_count;
        // Should run sequentially, so it's safe
    });

    hpx::this_thread::experimental::sync_wait(snd);

    HPX_TEST_EQ(call_count.load(), 10);
}

void test_parallel_bulk()
{
    hpx::execution::parallel_executor exec;
    std::atomic<int> call_count{0};

    auto sched = exec.query(ex::get_scheduler_t{});

    auto snd = ex::schedule(sched) | ex::bulk(1000, [&](int i) {
        (void) i;
        ++call_count;
    });

    hpx::this_thread::experimental::sync_wait(snd);

    HPX_TEST_EQ(call_count.load(), 1000);
}

void test_parallel_bulk_with_value()
{
    hpx::execution::parallel_executor exec;
    std::atomic<int> call_count{0};

    auto sched = exec.query(ex::get_scheduler_t{});

    auto snd = ex::schedule(sched) | ex::then([]() { return 42; }) |
        ex::bulk(100, [&](int i, int val) {
            (void) i;
            HPX_TEST_EQ(val, 42);
            ++call_count;
        });

    hpx::this_thread::experimental::sync_wait(snd);

    HPX_TEST_EQ(call_count.load(), 100);
}

inline constexpr struct custom_bulk_query_t final : ex::forwarding_query_t
{
    template <typename Env>
        requires(requires(Env const& e, custom_bulk_query_t q) { e.query(q); })
    constexpr decltype(auto) operator()(Env const& e) const noexcept
    {
        return e.query(*this);
    }
} custom_bulk_query{};

template <typename F>
struct env_query_sender
{
    using sender_concept = ex::sender_t;

    template <typename Env>
    static consteval auto get_completion_signatures() noexcept
    {
        return ex::completion_signatures<ex::set_value_t()>{};
    }

    F f;

    template <typename Receiver>
    struct operation_state
    {
        using operation_state_concept = ex::operation_state_t;
        HPX_NO_UNIQUE_ADDRESS std::decay_t<Receiver> receiver;
        F f;

        void start() & noexcept
        {
            auto env = ex::get_env(receiver);
            f(env);
            ex::set_value(HPX_MOVE(receiver));
        }
    };

    template <typename Receiver>
    auto connect(Receiver&& r) &&
    {
        return operation_state<Receiver>{HPX_FORWARD(Receiver, r), f};
    }
};

struct test_env_receiver
{
    using receiver_concept = ex::receiver_t;

    std::atomic<bool>& set_value_called;
    hpx::stop_token stop_token;

    void set_value() && noexcept
    {
        set_value_called = true;
    }

    void set_stopped() && noexcept {}

    void set_error(std::exception_ptr) && noexcept {}

    auto get_env() const noexcept
    {
        return ex::make_env(ex::prop(custom_bulk_query, 42),
            ex::prop(ex::get_stop_token, stop_token));
    }
};

void test_bulk_receiver_env_forwarding()
{
    hpx::execution::sequenced_executor exec;
    auto sched = exec.query(ex::get_scheduler_t{});

    std::atomic<bool> env_queried{false};
    std::atomic<bool> receiver_called{false};
    std::atomic<int> bulk_count{0};
    hpx::stop_source stop_source;

    auto upstream = env_query_sender{[&](auto env) {
        env_queried = true;
        HPX_TEST_EQ(custom_bulk_query(env), 42);
        HPX_TEST_EQ(ex::get_stop_token(env).stop_possible(), true);
    }};

    auto snd = sched.bulk(std::move(upstream), 10, [&](int i) {
        HPX_TEST_EQ(i, bulk_count);
        ++bulk_count;
    });

    auto r = test_env_receiver{receiver_called, stop_source.get_token()};
    auto os = ex::connect(std::move(snd), std::move(r));
    ex::start(os);

    HPX_TEST(env_queried);
    HPX_TEST_EQ(bulk_count.load(), 10);
    HPX_TEST(receiver_called);
}

int hpx_main()
{
    test_sequential_bulk();
    test_parallel_bulk();
    test_parallel_bulk_with_value();
    test_bulk_receiver_env_forwarding();
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    std::vector<std::string> const cfg = {"hpx.os_threads=all"};
    hpx::local::init_params init_args;
    init_args.cfg = cfg;

    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv, init_args), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
