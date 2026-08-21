//  Copyright (c) 2026 Arpit Khandelwal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This compile-only test verifies that the managed_component_dtor_policy
// explicit specialization for promise_lco correctly selects
// managed_object_is_lifetime_controlled.
//
// Without the fix in promise_lco.hpp (adding the second template argument
// 'void' to match the primary template's Enable parameter), the explicit
// specialization is ill-formed and the primary template is selected instead,
// returning managed_object_controls_lifetime. Clang emits:
//
//   error: too few template arguments for class template
//          'managed_component_dtor_policy'
//
// which cascades into 20+ TU build failures.

#include <hpx/async_distributed/detail/promise_lco.hpp>
#include <hpx/components_base/traits/managed_component_policies.hpp>

#include <type_traits>

// The dtor policy for promise_lco<R, RR> must be
// managed_object_is_lifetime_controlled, not managed_object_controls_lifetime.
static_assert(std::is_same_v<hpx::traits::managed_component_dtor_policy_t<
                                 hpx::lcos::detail::promise_lco<int, int>>,
                  hpx::traits::managed_object_is_lifetime_controlled>,
    "managed_component_dtor_policy for promise_lco<int, int> must be "
    "managed_object_is_lifetime_controlled; specialization in promise_lco.hpp "
    "is missing the required second template argument 'void'");

static_assert(
    std::is_same_v<
        hpx::traits::managed_component_dtor_policy_t<
            hpx::lcos::detail::promise_lco<void, hpx::util::unused_type>>,
        hpx::traits::managed_object_is_lifetime_controlled>,
    "managed_component_dtor_policy for promise_lco<void, unused_type> must be "
    "managed_object_is_lifetime_controlled");

int main()
{
    return 0;
}
