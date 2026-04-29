//  Copyright (c) 2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/testing.hpp>

#include <exception>
#include <string>
#include <type_traits>
#include <utility>

#include <iostream>

namespace ex = hpx::execution::experimental;

struct some_env
{
};

namespace mylib {
    struct receiver_1
    {
        using is_receiver = void;
    };

    struct receiver_2
    {
        using is_receiver = void;

        decltype(auto) get_env() const noexcept
        {
            return some_env{};
        }
    };

    inline constexpr struct receiver_env_t final : ex::forwarding_query_t
    {
        // clang-format off
        template <typename Env>
        decltype(auto) operator()(Env const& e) const noexcept
        {
            return e.query(*this);
        }
        // clang-format on
    } receiver_env{};

    auto env3 = ex::prop(receiver_env, 42);
    using env3_t = decltype(env3);

    struct receiver_3
    {
        using is_receiver = void;

        decltype(auto) get_env() const noexcept
        {
            auto f = ex::prop(receiver_env, 42);
            return f;
        }
    };

    // clang-format off
    auto env4 = ex::env(
        std::move(env3), ex::prop(receiver_env, std::string("42")));
    // clang-format on
    using env4_t = decltype(env4);

    struct receiver_4
    {
        using is_receiver = void;

        decltype(auto) get_env() const noexcept
        {
            receiver_3 rcv;

            /* Due to https://github.com/llvm/llvm-project/issues/88077
            * The following line never compiles, and if it does, it misbehaves.
            * return ex::env(
            *           ex::prop(receiver_env, std::string("42")),
            *           ex::get_env(rcv)
            *       );
            *
            * The following is a workaround */

            auto e = ex::get_env(rcv);
            auto p = ex::prop(receiver_env, std::string("42"));

            return ex::env(std::move(e), std::move(p));
        }
    };

    inline constexpr struct receiver_env1_t final : ex::forwarding_query_t
    {
        template <typename Env>
        decltype(auto) operator()(Env const& e) const noexcept
        {
            return e.query(*this);
        }
    } receiver_env1{};

    // clang-format off
    auto env5 =
        ex::env(std::move(env3), ex::prop(receiver_env1, std::string("42")));
    // clang-format on
    using env5_t = decltype(env5);

    struct receiver_5
    {
        using is_receiver = void;

        decltype(auto) get_env() const noexcept
        {
            receiver_3 rcv;
            /* Same as receiver_4
             * This would cause the compiler to crash:
             * return ex::env(
             *    ex::get_env(rcv), ex::prop(receiver_env1, std::string("42")));
             * */
            auto e = ex::get_env(rcv);
            auto p = ex::prop(receiver_env1, std::string("42"));
            return ex::env(std::move(e), std::move(p));
        }
    };
}    // namespace mylib

int main()
{
    using ex::empty_env;

    {
        mylib::receiver_1 rcv;
        auto env = ex::get_env(rcv);
        static_assert(
            std::is_same_v<decltype(env), empty_env>, "must return empty_env");
    }
    {
        mylib::receiver_2 rcv;
        auto env = ex::get_env(rcv);
        static_assert(
            std::is_same_v<decltype(env), some_env>, "must return some_env");
    }
    {
        mylib::receiver_3 rcv;
        auto env = ex::get_env(rcv);
        HPX_TEST_EQ(mylib::receiver_env(env), 42);
    }
    {
        mylib::receiver_4 rcv;
        // The resulting env_ = env(env1, env2) will query env1 first and env2
        // in that order, in order to find the resulting value of some query.
        // In cases when both env1 and env2 support the same query, as seen here
        // with receiver_env the result of env1 is picked.
        auto env = ex::get_env(rcv);
        HPX_TEST_EQ(mylib::receiver_env(env), 42);
    }
    {
        mylib::receiver_5 rcv;
        auto env = ex::get_env(rcv);
        HPX_TEST_EQ(mylib::receiver_env(env), 42);
        HPX_TEST_EQ(mylib::receiver_env1(env), std::string("42"));
    }

    return hpx::util::report_errors();
}
