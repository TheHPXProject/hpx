//  Copyright (c) 2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/testing.hpp>

#include "algorithm_test_utils.hpp"

#include <exception>
#include <type_traits>
#include <utility>

bool set_value_sched_called = false;
bool set_value_delegatee_sched_called = false;
bool set_value_allocator_called = false;
bool set_value_stop_token_called = false;

namespace ex = hpx::execution::experimental;

namespace mylib {

    using sched = my_namespace::my_scheduler;

    using sched_env_t = ex::prop<ex::get_scheduler_t, sched>;

    using delegatee_sched = my_namespace::my_scheduler_template<0>;

    using delegatee_sched_env_t = ex::env<sched_env_t,
        ex::prop<ex::get_delegatee_scheduler_t, delegatee_sched>>;

    struct allocator
    {
    };

    using allocator_env_t = ex::env<delegatee_sched_env_t,
        ex::prop<ex::get_allocator_t, allocator>>;

    struct stop_token
    {
        // TODO: Find out the correct type for this alias.
        // Based on:
        // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/
        // p2300r10.html#design-cancellation-summary
        template <typename T>
        using callback_type = void;

        bool stop_requested() const noexcept
        {
            return false;
        };
        bool stop_possible() const noexcept
        {
            return true;
        };

        friend bool operator==([[maybe_unused]] stop_token const& a,
            [[maybe_unused]] stop_token const& b)
        {
            return false;
        }
    };

    // clang-format off
    using stop_token_env_t = ex::env<allocator_env_t,
        ex::prop<ex::get_stop_token_t, stop_token>>;
    // clang-format on

    struct receiver_1
    {
        using receiver_concept = ex::receiver_t;

        void set_stopped() && noexcept {}

        void set_error(std::exception_ptr) && noexcept {}

        void set_value(sched) && noexcept
        {
            set_value_sched_called = true;
        }

        void set_value(delegatee_sched) && noexcept
        {
            set_value_delegatee_sched_called = true;
        }

        void set_value(allocator) && noexcept
        {
            set_value_allocator_called = true;
        }

        void set_value(stop_token) && noexcept
        {
            set_value_stop_token_called = true;
        }

        auto get_env() const noexcept
        {
            auto sched_env = ex::prop(ex::get_scheduler_t{}, sched());
            static_assert(std::is_same_v<decltype(sched_env), sched_env_t>,
                "must return sched_env");
            auto delegatee_sched_env = ex::env(std::move(sched_env),
                ex::prop(ex::get_delegatee_scheduler_t{}, delegatee_sched()));
            static_assert(std::is_same_v<decltype(delegatee_sched_env),
                              delegatee_sched_env_t>,
                "must return delegatee_sched_env");
            auto allocator_env = ex::env(std::move(delegatee_sched_env),
                ex::prop(ex::get_allocator_t{}, allocator()));
            static_assert(
                std::is_same_v<decltype(allocator_env), allocator_env_t>,
                "must return allocator_env");
            auto stop_token_env = ex::env(std::move(allocator_env),
                ex::prop(ex::get_stop_token_t{}, stop_token()));
            static_assert(
                std::is_same_v<decltype(stop_token_env), stop_token_env_t>,
                "must return stop_token_env");

            return stop_token_env;
        }
    };
}    // namespace mylib

int main()
{
    {
        mylib::receiver_1 rcv;
        auto env = ex::get_env(rcv);

        auto sched = ex::get_scheduler(env);
        static_assert(std::is_same_v<decltype(sched), mylib::sched>,
            "must return mylib::sched");

        auto delegatee_sched = ex::get_delegatee_scheduler(env);
        static_assert(
            std::is_same_v<decltype(delegatee_sched), mylib::delegatee_sched>,
            "must return mylib::delegatee_sched");

        auto allocator = ex::get_allocator(env);
        static_assert(std::is_same_v<decltype(allocator), mylib::allocator>,
            "must return mylib::allocator");

        auto stop_token = ex::get_stop_token(env);
        static_assert(std::is_same_v<decltype(stop_token), mylib::stop_token>,
            "must return mylib::stop_token");
    }
    {
        mylib::receiver_1 rcv;
        auto os = ex::connect(ex::get_scheduler(), rcv);
        ex::start(os);
        HPX_TEST(set_value_sched_called);
    }
    {
        mylib::receiver_1 rcv;
        auto os = ex::connect(ex::get_delegatee_scheduler(), rcv);
        ex::start(os);
        HPX_TEST(set_value_delegatee_sched_called);
    }
    {
        mylib::receiver_1 rcv;
        auto os = ex::connect(ex::get_allocator(), rcv);
        ex::start(os);
        HPX_TEST(set_value_allocator_called);
    }
    {
        mylib::receiver_1 rcv;
        auto os = ex::connect(ex::get_stop_token(), rcv);
        ex::start(os);
        HPX_TEST(set_value_stop_token_called);
    }

    return hpx::util::report_errors();
}
