//  Copyright (c) 2026 Priyanshi Sharma
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/testing.hpp>

#if defined(HPX_HAVE_CXX26_REFLECTION)

#include <hpx/modules/actions_base.hpp>

#include <cstddef>
#include <string>
#include <type_traits>

namespace app {

    int compute(double x, double y)
    {
        return (int) (x + y);
    }

    void broadcast(int n) noexcept
    {
        (void) n;
    }

    double transform(float x, int n)
    {
        return x * n;
    }

}    // namespace app

namespace app::nested {

    int deep_compute(int x, int y)
    {
        return x + y;
    }

}    // namespace app::nested

int main()
{
    // Test: action name extraction for simple namespace function
    {
        HPX_ACTION(app::compute, compute_action);
        HPX_TEST_EQ(std::string(compute_action::get_action_name(nullptr)),
            std::string("plain action(app::compute)"));
    }

    // Test: action name extraction for void noexcept function
    {
        HPX_ACTION(app::broadcast, broadcast_action);
        HPX_TEST_EQ(std::string(broadcast_action::get_action_name(nullptr)),
            std::string("plain action(app::broadcast)"));
    }

    // Test: action name extraction for multiple parameters
    {
        HPX_ACTION(app::transform, transform_action);
        HPX_TEST_EQ(std::string(transform_action::get_action_name(nullptr)),
            std::string("plain action(app::transform)"));
    }

    // Test: action name extraction for nested namespace
    {
        HPX_ACTION(app::nested::deep_compute, deep_action);
        HPX_TEST_EQ(std::string(deep_action::get_action_name(nullptr)),
            std::string("plain action(app::nested::deep_compute)"));
    }

    // Test: arity extraction
    {
        HPX_ACTION(app::compute, compute_action);
        HPX_TEST_EQ(compute_action::arity, std::size_t(2));
    }

    {
        HPX_ACTION(app::broadcast, broadcast_action);
        HPX_TEST_EQ(broadcast_action::arity, std::size_t(1));
    }

    // Test: function invocation via func_ptr
    {
        HPX_ACTION(app::compute, compute_action);
        HPX_TEST_EQ(compute_action::func_ptr(3.0, 4.0), 7);
    }

    // Test: auto-registration static member exists (compile-time check)
    // This verifies that reflect_action has the invocation_count_registrar_
    // static member which enables automatic registration without HPX_REGISTER_ACTION
    {
        HPX_ACTION(app::compute, compute_action);
        using registrar_type =
            decltype(compute_action::invocation_count_registrar_);
        static_assert(!std::is_void_v<registrar_type>,
            "invocation_count_registrar_ must exist");
    }
    // Test: invoke() works for noexcept functions (verifies that
    // unqualify_function_type_t correctly strips noexcept from the
    // function type so basic_action instantiation succeeds)
    {
        HPX_ACTION(app::broadcast, broadcast_action);
        hpx::naming::address_type lva{};
        hpx::naming::component_type comptype{};
        broadcast_action::invoke(lva, comptype, 42);
        static_assert(broadcast_action::arity == std::size_t(1),
            "arity must be a compile-time constant");
    }
    return hpx::util::report_errors();
}

#else

int main()
{
    return 0;
}

#endif
