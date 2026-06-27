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
        void start() & {}
    };

    struct state_3
    {
        void start() & noexcept
        {
            start_called = true;
        }
    };

    struct state_4
    {
        void start() & {}
    };

    struct state_5
    {
        void start() & noexcept
        {
            start_called = true;
        }
    };    // Added semicolon here

    template <bool Noexcept>
    struct state
    {
        void start() && noexcept(Noexcept)
        {
            HPX_TEST(false);
        }

        void start() & noexcept(Noexcept)
        {
            started = 1;
        }

        void start() const& noexcept(Noexcept)
        {
            started = 2;
        }
    };

    class indestructible_state
    {
    private:
        ~indestructible_state() {}

    public:
        void start() & noexcept
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
        static_assert(noexcept(std::declval<mylib::state<true>>().start()));
        static_assert(noexcept(std::declval<mylib::state<true>&&>().start()));
        static_assert(noexcept(std::declval<mylib::state<true>&>().start()));
        static_assert(
            noexcept(std::declval<mylib::state<true> const&>().start()));

        // rvalues can't be used via the start CPO
        static_assert(!hpx::is_invocable_v<ex::start_t, mylib::state<true>>);
        static_assert(!hpx::is_invocable_v<ex::start_t, mylib::state<true>&&>);

        // lvalues can be used via the start CPO and don't throw
        static_assert(noexcept(ex::start(std::declval<mylib::state<true>&>())));
        static_assert(
            std::is_nothrow_invocable_v<ex::start_t, mylib::state<true>&>);
    }

    {
        // verify test class
        static_assert(!noexcept(std::declval<mylib::state<false>>().start()));
        static_assert(!noexcept(std::declval<mylib::state<false>&&>().start()));
        static_assert(!noexcept(std::declval<mylib::state<false>&>().start()));
        static_assert(
            !noexcept(std::declval<mylib::state<false> const&>().start()));

        // stdexec's start_t constraint only checks .start() member exists;
        // noexcept is enforced via static_assert inside the body, not SFINAE.
        // So start_t IS invocable even on non-noexcept operation states.
        static_assert(hpx::is_invocable_v<ex::start_t, mylib::state<false>&>);
        static_assert(
            hpx::is_invocable_v<ex::start_t, mylib::state<false> const&>);
        // rvalues/temporaries still not invocable (start_t takes _Op&)
        static_assert(!hpx::is_invocable_v<ex::start_t, mylib::state<false>>);
        static_assert(!hpx::is_invocable_v<ex::start_t, mylib::state<false>&&>);
    }

    {
        static_assert(
            noexcept(ex::start(std::declval<mylib::indestructible_state&>())));
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
