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

// Component server types must be at namespace scope (not local) because
// reflect_component_action uses parent_of(F) which requires a named entity.

namespace app {

    struct compute_server
    {
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
            return static_cast<double>(x) * n;
        }
    };

    // actions made by the macros that are used inside a class
    struct macro_server
    {
        int compute(int x)
        {
            return x;
        }

        HPX_DEFINE_COMPONENT_ACTION(macro_server, compute, compute_action)
        HPX_DEFINE_COMPONENT_DIRECT_ACTION(
            macro_server, compute, compute_direct_action)
    };

}    // namespace app

namespace app::nested {

    struct deep_server
    {
        int deep_compute(int x, int y)
        {
            return x + y;
        }
    };

}    // namespace app::nested

int main()
{
    // Test: component_type extraction via parent_of reflection
    {
        HPX_COMPONENT_ACTION(app::compute_server, compute, compute_action);
        static_assert(
            std::is_same_v<compute_action::component_type, app::compute_server>,
            "component_type must be app::compute_server");
    }

    // Test: result_type extraction
    {
        HPX_COMPONENT_ACTION(app::compute_server, compute, compute_action);
        static_assert(std::is_same_v<compute_action::result_type, int>,
            "result_type must be int");
    }

    // Test: void result_type for noexcept member function
    {
        HPX_COMPONENT_ACTION(app::compute_server, broadcast, broadcast_action);
        static_assert(std::is_same_v<broadcast_action::result_type, void>,
            "result_type must be void");
    }

    // Test: arity extraction for multi-parameter member function
    {
        HPX_COMPONENT_ACTION(app::compute_server, compute, compute_action);
        HPX_TEST_EQ(compute_action::arity, std::size_t(2));
    }

    // Test: arity extraction for single-parameter noexcept member function
    {
        HPX_COMPONENT_ACTION(app::compute_server, broadcast, broadcast_action);
        HPX_TEST_EQ(broadcast_action::arity, std::size_t(1));
    }

    // Test: arity extraction for mixed-type parameters
    {
        HPX_COMPONENT_ACTION(app::compute_server, transform, transform_action);
        HPX_TEST_EQ(transform_action::arity, std::size_t(2));
    }

    // Test: component_type for nested namespace server
    {
        HPX_COMPONENT_ACTION(
            app::nested::deep_server, deep_compute, deep_action);
        static_assert(std::is_same_v<deep_action::component_type,
                          app::nested::deep_server>,
            "component_type must be app::nested::deep_server");
    }

    // Test: auto-registration static member exists
    // Verifies that reflect_component_action has invocation_count_registrar_
    // enabling automatic registration without HPX_REGISTER_ACTION.
    {
        HPX_COMPONENT_ACTION(app::compute_server, compute, compute_action);
        using registrar_type =
            decltype(compute_action::invocation_count_registrar_);
        static_assert(!std::is_void_v<registrar_type>,
            "invocation_count_registrar_ must exist for component actions");

        // Invocation counters are looked up by the name of the action, so the
        // counter has to be registered under the action and not its base.
        // Without Derived, the template itself is the action.
        using hpx::actions::detail::register_action_invocation_count;
        HPX_COMPONENT_DIRECT_ACTION(
            app::compute_server, compute, compute_direct_action);
        using template_type = hpx::actions::reflect_component_action<
            ^^app::compute_server::compute>;

        static_assert(std::is_same_v<registrar_type,
            register_action_invocation_count<compute_action>>);
        static_assert(std::is_same_v<
            decltype(compute_direct_action::invocation_count_registrar_),
            register_action_invocation_count<compute_direct_action>>);
        static_assert(
            std::is_same_v<decltype(template_type::invocation_count_registrar_),
                register_action_invocation_count<template_type>>);
    }

    // Test: HPX_COMPONENT_ACTION produces a type derived from
    // reflect_component_action (the macro always wraps in a named struct,
    // so it is never the same type as reflect_component_action itself --
    // this verifies the inheritance relationship instead).
    {
        HPX_COMPONENT_ACTION(app::compute_server, compute, compute_action);
        static_assert(
            std::is_base_of_v<hpx::actions::reflect_component_action<
                                  ^^app::compute_server::compute,
                                  app::compute_server, compute_action>,
                compute_action>,
            "compute_action must derive from reflect_component_action");
    }

    // Test: the action the macros make is its own derived_type, so the
    // traits that HPX_ACTION_HAS_PRIORITY and similar macros specialize for
    // it are used (see #8019)
    {
        HPX_COMPONENT_ACTION(app::compute_server, compute, compute_action);
        HPX_COMPONENT_DIRECT_ACTION(
            app::compute_server, compute, compute_direct_action);

        static_assert(
            std::is_same_v<compute_action::derived_type, compute_action>);
        static_assert(std::is_same_v<compute_direct_action::derived_type,
            compute_direct_action>);
        static_assert(
            std::is_same_v<app::macro_server::compute_action::derived_type,
                app::macro_server::compute_action>);
        static_assert(std::is_same_v<
            app::macro_server::compute_direct_action::derived_type,
            app::macro_server::compute_direct_action>);
    }

    // Test: actual runtime invocation through invoke() on a real component
    // instance. This exercises the component_invoke() dispatch path and
    // would catch regressions in how the member function is spliced and
    // called (e.g. missing address-of on a member-function splice).
    {
        HPX_COMPONENT_ACTION(app::compute_server, compute, compute_action);

        app::compute_server server{};
        hpx::naming::address_type lva = static_cast<void*>(&server);

        int result = compute_action::invoke(
            lva, hpx::naming::component_type{}, 3.0, 4.0);
        HPX_TEST_EQ(result, 7);
    }

    // Test: runtime invocation for noexcept void member function
    {
        HPX_COMPONENT_ACTION(app::compute_server, broadcast, broadcast_action);

        app::compute_server server{};
        hpx::naming::address_type lva = static_cast<void*>(&server);

        broadcast_action::invoke(lva, hpx::naming::component_type{}, 42);
    }

    // Test: runtime invocation for nested-namespace component
    {
        HPX_COMPONENT_ACTION(
            app::nested::deep_server, deep_compute, deep_action);

        app::nested::deep_server server{};
        hpx::naming::address_type lva = static_cast<void*>(&server);

        int result =
            deep_action::invoke(lva, hpx::naming::component_type{}, 5, 9);
        HPX_TEST_EQ(result, 14);
    }

    // Test: reflect_component_direct_action reports direct-execution
    // semantics (no thread spawned) and still dispatches correctly.
    {
        using direct_action_type =
            hpx::actions::reflect_component_direct_action<
                ^^app::compute_server::compute>;

        static_assert(direct_action_type::direct_execution::value,
            "reflect_component_direct_action must report direct_execution");
        static_assert(direct_action_type::get_action_type() ==
                hpx::actions::action_flavor::direct_action,
            "reflect_component_direct_action must report action_flavor::"
            "direct_action");

        app::compute_server server{};
        hpx::naming::address_type lva = static_cast<void*>(&server);
        int result = direct_action_type::invoke(
            lva, hpx::naming::component_type{}, 6.0, 8.0);
        HPX_TEST_EQ(result, 14);
    }

    return hpx::util::report_errors();
}

#else

int main()
{
    return 0;
}

#endif
