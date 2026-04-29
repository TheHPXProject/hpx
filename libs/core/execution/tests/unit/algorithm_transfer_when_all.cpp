//  Copyright (c) 2021 ETH Zurich
//  Copyright (c) 2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Our implementation of transfer_when_all relies on when_all and transfer.
// Those are thoroughly tested independently. We provide fewer test cases
// here for this reason.

#include <hpx/config.hpp>

// Clang V11 ICE's on this test, Clang V8 reports a bogus constexpr problem
#if !defined(HPX_CLANG_VERSION) ||                                             \
    ((HPX_CLANG_VERSION / 10000) != 11 && (HPX_CLANG_VERSION / 10000) != 8)

#include <hpx/modules/execution.hpp>
#include <hpx/modules/testing.hpp>

#include "algorithm_test_utils.hpp"

#include <atomic>
#include <exception>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#if defined(HPX_CLANG_VERSION)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

namespace ex = hpx::execution::experimental;

int main()
{
    // Success path
    {
        std::atomic<bool> set_value_called{false};
        std::atomic<bool> scheduler_schedule_called{false};
        std::atomic<bool> scheduler_execute_called{false};
        std::atomic<bool> tag_invoke_overload_called{false};

        auto sched = example_scheduler{scheduler_schedule_called,
            scheduler_execute_called, tag_invoke_overload_called};
        auto s = ex::transfer_when_all(sched, ex::just(42));
        static_assert(ex::is_sender_v<decltype(s)>,
            "transfer_when_all must return a sender");

        auto csch =
            ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(s));
        HPX_TEST(sched == csch);

        auto f = [](int x) { HPX_TEST_EQ(x, 42); };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
        HPX_TEST(!tag_invoke_overload_called);
        HPX_TEST(scheduler_schedule_called);
        HPX_TEST(!scheduler_execute_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        std::atomic<bool> scheduler_schedule_called{false};
        std::atomic<bool> scheduler_execute_called{false};
        std::atomic<bool> tag_invoke_overload_called{false};

        auto sched = example_scheduler{scheduler_schedule_called,
            scheduler_execute_called, tag_invoke_overload_called};
        auto s = ex::transfer_when_all(sched, ex::just(42),
            ex::just(std::string("hello")), ex::just(3.14));
        static_assert(ex::is_sender_v<decltype(s)>,
            "transfer_when_all must return a sender");

        auto csch =
            ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(s));
        HPX_TEST(sched == csch);

        auto f = [](int x, std::string y, double z) {
            HPX_TEST_EQ(x, 42);
            HPX_TEST_EQ(y, std::string("hello"));
            HPX_TEST_EQ(z, 3.14);
        };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
        HPX_TEST(!tag_invoke_overload_called);
        HPX_TEST(scheduler_schedule_called);
        HPX_TEST(!scheduler_execute_called);
    }

    {
        std::atomic<bool> set_value_called{false};
        std::atomic<bool> scheduler_schedule_called{false};
        std::atomic<bool> scheduler_execute_called{false};
        std::atomic<bool> tag_invoke_overload_called{false};

        auto sched = example_scheduler{scheduler_schedule_called,
            scheduler_execute_called, tag_invoke_overload_called};
        auto s = ex::transfer_when_all(
            sched, ex::just(), ex::just(std::string("hello")), ex::just(3.14));
        static_assert(ex::is_sender_v<decltype(s)>,
            "transfer_when_all must return a sender");

        auto csch =
            ex::get_completion_scheduler<ex::set_value_t>(ex::get_env(s));
        HPX_TEST(sched == csch);

        auto f = [](std::string y, double z) {
            HPX_TEST_EQ(y, std::string("hello"));
            HPX_TEST_EQ(z, 3.14);
        };
        auto r = callback_receiver<decltype(f)>{f, set_value_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);
        HPX_TEST(set_value_called);
        HPX_TEST(!tag_invoke_overload_called);
        HPX_TEST(scheduler_schedule_called);
        HPX_TEST(!scheduler_execute_called);
    }

    // Failure path
    {
        std::atomic<bool> set_error_called{false};
        std::atomic<bool> scheduler_schedule_called{false};
        std::atomic<bool> scheduler_execute_called{false};
        std::atomic<bool> tag_invoke_overload_called{false};

        auto sched = example_scheduler{scheduler_schedule_called,
            scheduler_execute_called, tag_invoke_overload_called};
        auto s = ex::transfer_when_all(sched, error_typed_sender<double>{});
        auto r = error_callback_receiver<check_exception_ptr>{
            check_exception_ptr{}, set_error_called};
        auto os = ex::connect(std::move(s), std::move(r));
        ex::start(os);

        HPX_TEST(set_error_called);
        HPX_TEST(!tag_invoke_overload_called);
        HPX_TEST(scheduler_schedule_called);
        HPX_TEST(!scheduler_execute_called);
    }

    return hpx::util::report_errors();
}

#if defined(HPX_CLANG_VERSION)
#pragma clang diagnostic pop
#endif
#else
int main()
{
    return 0;
}
#endif
