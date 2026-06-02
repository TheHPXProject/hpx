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
    }

    // Test: Derived = void default -- action_type is reflect_component_action
    // itself (not a separate derived type)
    {
        HPX_COMPONENT_ACTION(app::compute_server, compute, compute_action);
        static_assert(std::is_same_v<compute_action,
                          hpx::actions::reflect_component_action<
                              ^^app::compute_server::compute>>,
            "default Derived must produce reflect_component_action itself");
    }

    return hpx::util::report_errors();
}

#else

int main()
{
    return 0;
}

#endif
