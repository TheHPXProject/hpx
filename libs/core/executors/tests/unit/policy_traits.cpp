//  Copyright (c) 2026 Rohan Pattanayak
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Compile-time verification of policy_traits and the is_*_execution_policy
// traits derived from it, for every execution policy built into
// hpx::execution and, when enabled, hpx::execution's datapar extensions.
// Nothing here executes at runtime; if this file compiles, every
// static_assert below has already been checked by the compiler.

#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#if defined(HPX_HAVE_DATAPAR)
#include <hpx/datapar.hpp>
#endif

///////////////////////////////////////////////////////////////////////////
// A single policy is checked against all seven policy_traits members at
// once, so a mismatch on any one property points directly at the type that
// caused it rather than requiring the reader to cross-reference several
// separate static_asserts.
template <typename Policy, bool ExpectPolicy, bool ExpectRebound,
    bool ExpectParallel, bool ExpectSequenced, bool ExpectUnsequenced,
    bool ExpectAsync, bool ExpectVectorpack>
constexpr bool check_policy_traits()
{
    using traits = hpx::detail::policy_traits<Policy>;
    static_assert(traits::is_policy == ExpectPolicy);
    static_assert(traits::is_rebound == ExpectRebound);
    static_assert(traits::is_parallel == ExpectParallel);
    static_assert(traits::is_sequenced == ExpectSequenced);
    static_assert(traits::is_unsequenced == ExpectUnsequenced);
    static_assert(traits::is_async == ExpectAsync);
    static_assert(traits::is_vectorpack == ExpectVectorpack);

    // The is_*_execution_policy traits are expected to agree with
    // policy_traits exactly, since they delegate to it by default.
    static_assert(hpx::is_execution_policy_v<Policy> == ExpectPolicy);
    static_assert(hpx::is_rebound_execution_policy_v<Policy> == ExpectRebound);
    static_assert(
        hpx::is_parallel_execution_policy_v<Policy> == ExpectParallel);
    static_assert(
        hpx::is_sequenced_execution_policy_v<Policy> == ExpectSequenced);
    static_assert(
        hpx::is_unsequenced_execution_policy_v<Policy> == ExpectUnsequenced);
    static_assert(hpx::is_async_execution_policy_v<Policy> == ExpectAsync);
    static_assert(
        hpx::is_vectorpack_execution_policy_v<Policy> == ExpectVectorpack);

    return true;
}

///////////////////////////////////////////////////////////////////////////
// The 8 built-in, always-available execution policies. Arguments to
// check_policy_traits are, in order: is_policy, is_rebound, is_parallel,
// is_sequenced, is_unsequenced, is_async, is_vectorpack.
static_assert(check_policy_traits<hpx::execution::sequenced_policy, true, true,
    false, true, false, false, false>());
static_assert(check_policy_traits<hpx::execution::sequenced_task_policy, true,
    true, false, true, false, true, false>());
static_assert(check_policy_traits<hpx::execution::parallel_policy, true, true,
    true, false, false, false, false>());
static_assert(check_policy_traits<hpx::execution::parallel_task_policy, true,
    true, true, false, false, true, false>());
static_assert(check_policy_traits<hpx::execution::unsequenced_policy, true,
    true, false, true, true, false, false>());
static_assert(check_policy_traits<hpx::execution::unsequenced_task_policy, true,
    true, false, true, true, true, false>());
static_assert(check_policy_traits<hpx::execution::parallel_unsequenced_policy,
    true, true, true, false, true, false, false>());
static_assert(
    check_policy_traits<hpx::execution::parallel_unsequenced_task_policy, true,
        true, true, false, true, true, false>());

///////////////////////////////////////////////////////////////////////////
// A type that is not an execution policy at all must default to false on
// every property, exactly as it did before policy_traits existed.
struct not_a_policy
{
};

static_assert(check_policy_traits<not_a_policy, false, false, false, false,
    false, false, false>());

///////////////////////////////////////////////////////////////////////////
// The 4 datapar/vectorpack execution policies, only available when a
// datapar backend is enabled.
#if defined(HPX_HAVE_DATAPAR)
static_assert(check_policy_traits<hpx::execution::simd_policy, true, false,
    false, true, false, false, true>());
static_assert(check_policy_traits<hpx::execution::simd_task_policy, true, false,
    false, true, false, true, true>());
static_assert(check_policy_traits<hpx::execution::par_simd_policy, true, false,
    true, false, false, false, true>());
static_assert(check_policy_traits<hpx::execution::par_simd_task_policy, true,
    false, true, false, false, true, true>());
#endif

///////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
