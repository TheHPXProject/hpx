//  Copyright (c) 2026 Priyanshi Sharma
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#include <hpx/config.hpp>
#include <hpx/modules/testing.hpp>

#if defined(HPX_HAVE_CXX26_REFLECTION) &&                                      \
    defined(HPX_HAVE_CXX26_REFLECTION_ANNOTATIONS)
#include <hpx/modules/actions_base.hpp>

#include <cstddef>
#include <type_traits>

///////////////////////////////////////////////////////////////////////////////
// Two annotated functions and one non-annotated function.
// Tests specifically exercise annotation-based discovery -- these checks
// would FAIL if annotations were not present or not detected correctly.
namespace annotated_test {

    [[= hpx::actions::detail::remote_function{}]] int increment(int x)
    {
        return x + 1;
    }

    [[= hpx::actions::detail::remote_function{}]] int add(int x, int y)
    {
        return x + y;
    }

    // Not annotated -- annotation discovery must exclude this function.
    // If is_annotated_remote_fn incorrectly included it, count would be 3.
    int local_helper(int x)
    {
        return x;
    }

}    // namespace annotated_test

///////////////////////////////////////////////////////////////////////////////
// These static_asserts require annotation support to pass.
// Without [[=hpx::actions::detail::remote_function{}]], count would be 0.
static_assert(hpx::actions::detail::count_annotated_fns(^^annotated_test) == 2,
    "annotation discovery must find exactly 2 annotated functions; "
    "would be 0 without [[=hpx::actions::detail::remote_function{}]]");

// local_helper has no annotation -- must be excluded by discovery.
// This specifically tests that is_annotated_remote_fn uses annotations,
// not just any function predicate.
static_assert(!hpx::actions::detail::is_annotated_remote_fn(
                  ^^annotated_test::local_helper),
    "non-annotated function must not be discovered");

// increment IS annotated -- must be included.
static_assert(
    hpx::actions::detail::is_annotated_remote_fn(^^annotated_test::increment),
    "annotated function must be discovered");

// get_annotated_fns returns exactly the annotated subset.
static_assert(
    hpx::actions::detail::get_annotated_fns<^^annotated_test>().size() == 2,
    "get_annotated_fns must return exactly 2 entries");

int main()
{
    // Verify reflect_action is well-formed for annotation-discovered functions.
    // func_ptr invocation confirms the action type resolves correctly.
    {
        using inc_action =
            hpx::actions::reflect_action<^^annotated_test::increment>;
        // arity=1 confirms the annotation-discovered function signature
        HPX_TEST_EQ(inc_action::arity, std::size_t(1));
        HPX_TEST_EQ(inc_action::func_ptr(41), 42);
    }
    {
        using add_action = hpx::actions::reflect_action<^^annotated_test::add>;
        HPX_TEST_EQ(add_action::arity, std::size_t(2));
        HPX_TEST_EQ(add_action::func_ptr(20, 22), 42);
    }

    return hpx::util::report_errors();
}

#else

int main()
{
    return 0;
}

#endif    // HPX_HAVE_CXX26_REFLECTION && HPX_HAVE_CXX26_REFLECTION_ANNOTATIONS
