//  Copyright (c) 2020 Thomas Heller
//  Copyright (c) 2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/testing.hpp>

#include <exception>
#include <string>
#include <type_traits>
#include <utility>

#if defined(HPX_CLANG_VERSION)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

namespace ex = hpx::execution::experimental;

bool start_called = false;
int started = 0;

namespace mylib {
    struct state_1
    {
    };

    struct state_2
    {
        friend void tag_invoke(ex::start_t, state_2&) {}
    };

    struct state_3
    {
        friend void tag_invoke(ex::start_t, state_3&) noexcept
        {
            start_called = true;
        }
    };

    struct state_4
    {
    };

    void tag_invoke(ex::start_t, state_4&) {}

    struct state_5
    {
    };

    void tag_invoke(ex::start_t, state_5&) noexcept
    {
        start_called = true;
    }

    template <bool Noexcept>
    struct state
    {
        friend void tag_invoke(ex::start_t, state&&) noexcept(Noexcept)
        {
            HPX_TEST(false);
        }

        friend void tag_invoke(ex::start_t, state&) noexcept(Noexcept)
        {
            started = 1;
        }

        friend void tag_invoke(ex::start_t, state const&) noexcept(Noexcept)
        {
            started = 2;
        }
    };

    class indestructible_state
    {
    private:
        ~indestructible_state() {}

    public:
        friend void tag_invoke(ex::start_t, indestructible_state&) noexcept
        {
            started = 3;
        }

        indestructible_state() {}
        static void destroy(indestructible_state* state)
        {
            delete state;
        }
    };
}    // namespace mylib

int main()
{
    {
        static_assert(!ex::is_operation_state<mylib::state_1>::value,
            "mylib::state_1 is not an operation state");
        static_assert(ex::is_operation_state<mylib::state_3>::value,
            "mylib::state_3 is an operation state");
        static_assert(ex::is_operation_state<mylib::state_5>::value,
            "mylib::state_5 is an operation state");
    }

    {
        // verify test class
        static_assert(noexcept(
            tag_invoke(ex::start, std::declval<mylib::state<true>>())));
        static_assert(noexcept(
            tag_invoke(ex::start, std::declval<mylib::state<true>&&>())));
        static_assert(noexcept(
            tag_invoke(ex::start, std::declval<mylib::state<true>&>())));
        static_assert(noexcept(
            tag_invoke(ex::start, std::declval<mylib::state<true> const&>())));

        // rvalues can't be used via the start CPO
        static_assert(!hpx::is_invocable_v<ex::start_t, mylib::state<true>>);
        static_assert(!hpx::is_invocable_v<ex::start_t, mylib::state<true>&&>);

        // lvalues can be used via the start CPO and don't throw
        static_assert(noexcept(hpx::functional::tag_invoke(
            ex::start, std::declval<mylib::state<true>&>())));
        static_assert(noexcept(hpx::functional::tag_invoke(
            ex::start, std::declval<mylib::state<true> const&>())));
        static_assert(
            std::is_nothrow_invocable_v<ex::start_t, mylib::state<true>&>);
        static_assert(std::is_nothrow_invocable_v<ex::start_t,
            mylib::state<true> const&>);
    }

    {
        // verify test class
        static_assert(!noexcept(
            tag_invoke(ex::start, std::declval<mylib::state<false>>())));
        static_assert(!noexcept(
            tag_invoke(ex::start, std::declval<mylib::state<false>&&>())));
        static_assert(!noexcept(
            tag_invoke(ex::start, std::declval<mylib::state<false>&>())));
        static_assert(!noexcept(
            tag_invoke(ex::start, std::declval<mylib::state<false> const&>())));

        // none of the operations work via the start CPO if they'd throw
        /*TODO: Check if the following way of invoking the start cpo leads to
         * the required checks by the execution.op_state concept check. That
         * check goes through the operator() of start_t but I am not sure if
         * we ever reach that point when calling the tag_invoke directly.
         */
        //        static_assert(!hpx::is_invocable_v<ex::start_t, mylib::state<false>>);
        //        static_assert(!hpx::is_invocable_v<ex::start_t, mylib::state<false>&&>);
        //        static_assert(!hpx::is_invocable_v<ex::start_t, mylib::state<false>&>);
        //        static_assert(
        //            !hpx::is_invocable_v<ex::start_t, mylib::state<false> const&>);
    }

    {
        static_assert(noexcept(hpx::functional::tag_invoke(
            ex::start, std::declval<mylib::indestructible_state&>())));
        static_assert(noexcept(hpx::functional::tag_invoke(
            ex::start, std::declval<mylib::indestructible_state&>())));
        static_assert(std::is_nothrow_invocable_v<ex::start_t,
            mylib::indestructible_state&>);

        auto* os(new mylib::indestructible_state());

        HPX_TEST_EQ(started, 0);
        ex::start(*os);
        HPX_TEST_EQ(started, 3);

        mylib::indestructible_state::destroy(os);
    }

    {
        mylib::state_3 state;

        ex::start(state);
        HPX_TEST(start_called);
        start_called = false;
    }
    {
        mylib::state_5 state;

        ex::start(state);
        HPX_TEST(start_called);
        start_called = false;
    }

    {
        started = 0;
        mylib::state<true> s;

        HPX_TEST_EQ(started, 0);
        ex::start(s);
        HPX_TEST_EQ(started, 1);
    }

    {
        started = 0;
        mylib::state<true> const s;

        HPX_TEST_EQ(started, 0);
        ex::start(s);
        HPX_TEST_EQ(started, 2);
    }

    return hpx::util::report_errors();
}

#if defined(HPX_CLANG_VERSION)
#pragma clang diagnostic pop
#endif
