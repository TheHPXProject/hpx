//  Copyright (c) 2026 Rohan Pattanayak
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Compile-time verification of policy_traits and the is_*_execution_policy
// traits derived from it, for every thrust execution policy. Nothing here
// executes at runtime; if this file compiles, every static_assert below has
// already been checked by the compiler.

#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/thrust.hpp>

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
// A stand-in executor/parameters pair used to instantiate the two thrust
// shim templates below. thrust_policy_shim and thrust_task_policy_shim are
// unconstrained templates, so any pair of types works for this check.
struct stub_executor
{
};
struct stub_parameters
{
};

// The 6 thrust execution policies. Arguments to check_policy_traits are, in
// order: is_policy, is_rebound, is_parallel, is_sequenced, is_unsequenced,
// is_async, is_vectorpack.
static_assert(check_policy_traits<hpx::thrust::thrust_policy, true, false, true,
    false, false, false, false>());
static_assert(check_policy_traits<
    hpx::thrust::thrust_policy_shim<stub_executor, stub_parameters>, true, true,
    true, false, false, false, false>());
static_assert(check_policy_traits<hpx::thrust::thrust_host_policy, true, false,
    true, false, false, false, false>());
static_assert(check_policy_traits<hpx::thrust::thrust_device_policy, true,
    false, true, false, false, false, false>());
static_assert(check_policy_traits<hpx::thrust::thrust_task_policy, true, false,
    true, false, false, true, false>());
static_assert(check_policy_traits<
    hpx::thrust::thrust_task_policy_shim<stub_executor, stub_parameters>, true,
    true, true, false, false, true, false>());

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
