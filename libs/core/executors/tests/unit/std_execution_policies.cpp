//  Copyright (c) 2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

///////////////////////////////////////////////////////////////////////////////
void static_checks()
{
    // std::execution::sequenced_policy, parallel_policy, and
    // parallel_unsequenced_policy are only visible under C++17 std
    // execution policy support; guard so the types are not referenced
    // when the feature is not available.
#if defined(HPX_HAVE_CXX17_STD_EXECUTION_POLICES)
    static_assert(
        hpx::is_execution_policy<std::execution::sequenced_policy>::value,
        "hpx::is_execution_policy<std::execution::sequenced_policy>::value");
    static_assert(
        hpx::is_execution_policy<std::execution::parallel_policy>::value,
        "hpx::is_execution_policy<std::execution::parallel_policy>::value");
    static_assert(hpx::is_execution_policy<
                      std::execution::parallel_unsequenced_policy>::value,
        "hpx::is_execution_policy<std::execution::parallel_unsequenced_policy>:"
        ":value");

    static_assert(hpx::is_sequenced_execution_policy<
                      std::execution::sequenced_policy>::value,
        "hpx::is_sequenced_execution_policy<std::execution::sequenced_policy>::"
        "value");
    static_assert(!hpx::is_sequenced_execution_policy<
                      std::execution::parallel_policy>::value,
        "!hpx::is_sequenced_execution_policy<std::execution::parallel_policy>::"
        "value");
    static_assert(!hpx::is_sequenced_execution_policy<
                      std::execution::parallel_unsequenced_policy>::value,
        "!hpx::is_sequenced_execution_policy<std::execution::parallel_"
        "unsequenced_policy>::value");

    static_assert(!hpx::is_parallel_execution_policy<
                      std::execution::sequenced_policy>::value,
        "!hpx::is_sequenced_execution_policy<std::execution::sequenced_policy>:"
        ":value");
    static_assert(hpx::is_parallel_execution_policy<
                      std::execution::parallel_policy>::value,
        "hpx::is_parallel_execution_policy<std::execution::parallel_policy>::"
        "value");
    static_assert(hpx::is_parallel_execution_policy<
                      std::execution::parallel_unsequenced_policy>::value,
        "hpx::is_parallel_execution_policy<std::execution::parallel_"
        "unsequenced_policy>::value");
#endif

#if defined(HPX_HAVE_CXX20_STD_EXECUTION_POLICES)
    static_assert(
        hpx::is_execution_policy<std::execution::unsequenced_policy>::value,
        "hpx::is_execution_policy<std::execution::unsequenced_policy>::value");
    static_assert(hpx::is_sequenced_execution_policy<
                      std::execution::unsequenced_policy>::value,
        "hpx::is_sequenced_execution_policy<std::execution::unsequenced_policy>"
        "::value");
    static_assert(!hpx::is_parallel_execution_policy<
                      std::execution::unsequenced_policy>::value,
        "!hpx::is_parallel_execution_policy<std::execution::unsequenced_policy>"
        "::value");
#endif

#if defined(HPX_HAVE_CXX17_STD_EXECUTION_POLICES)
    // The remaining four traits are false for every std:: execution policy
    // tested above; none of them is a rebound policy (they are not produced
    // by rebinding an executor or parameters onto another policy), none
    // runs asynchronously, and none operates on vector packs.
    static_assert(!hpx::is_rebound_execution_policy<
                      std::execution::sequenced_policy>::value,
        "!hpx::is_rebound_execution_policy<std::execution::sequenced_policy>"
        "::value");
    static_assert(!hpx::is_rebound_execution_policy<
                      std::execution::parallel_policy>::value,
        "!hpx::is_rebound_execution_policy<std::execution::parallel_policy>::"
        "value");
    static_assert(!hpx::is_rebound_execution_policy<
                      std::execution::parallel_unsequenced_policy>::value,
        "!hpx::is_rebound_execution_policy<std::execution::parallel_"
        "unsequenced_policy>::value");

    static_assert(!hpx::is_async_execution_policy<
                      std::execution::sequenced_policy>::value,
        "!hpx::is_async_execution_policy<std::execution::sequenced_policy>::"
        "value");
    static_assert(
        !hpx::is_async_execution_policy<std::execution::parallel_policy>::value,
        "!hpx::is_async_execution_policy<std::execution::parallel_policy>::"
        "value");
    static_assert(!hpx::is_async_execution_policy<
                      std::execution::parallel_unsequenced_policy>::value,
        "!hpx::is_async_execution_policy<std::execution::parallel_"
        "unsequenced_policy>::value");

    static_assert(!hpx::is_vectorpack_execution_policy<
                      std::execution::sequenced_policy>::value,
        "!hpx::is_vectorpack_execution_policy<std::execution::sequenced_"
        "policy>::value");
    static_assert(!hpx::is_vectorpack_execution_policy<
                      std::execution::parallel_policy>::value,
        "!hpx::is_vectorpack_execution_policy<std::execution::parallel_"
        "policy>::value");
    static_assert(!hpx::is_vectorpack_execution_policy<
                      std::execution::parallel_unsequenced_policy>::value,
        "!hpx::is_vectorpack_execution_policy<std::execution::parallel_"
        "unsequenced_policy>::value");

    // is_unsequenced_execution_policy is true for the two std:: policies
    // whose execution may be vectorized, mirroring the corresponding
    // hpx::execution::detail::unsequenced_policy_shim and
    // parallel_unsequenced_policy_shim specializations in
    // execution_policy.hpp, and false for the two that must not be
    // vectorized.
    static_assert(!hpx::is_unsequenced_execution_policy<
                      std::execution::sequenced_policy>::value,
        "!hpx::is_unsequenced_execution_policy<std::execution::sequenced_"
        "policy>::value");
    static_assert(!hpx::is_unsequenced_execution_policy<
                      std::execution::parallel_policy>::value,
        "!hpx::is_unsequenced_execution_policy<std::execution::parallel_"
        "policy>::value");
    static_assert(hpx::is_unsequenced_execution_policy<
                      std::execution::parallel_unsequenced_policy>::value,
        "hpx::is_unsequenced_execution_policy<std::execution::parallel_"
        "unsequenced_policy>::value");

#endif
#if defined(HPX_HAVE_CXX20_STD_EXECUTION_POLICES)
    static_assert(!hpx::is_rebound_execution_policy<
                      std::execution::unsequenced_policy>::value,
        "!hpx::is_rebound_execution_policy<std::execution::unsequenced_"
        "policy>::value");
    static_assert(!hpx::is_async_execution_policy<
                      std::execution::unsequenced_policy>::value,
        "!hpx::is_async_execution_policy<std::execution::unsequenced_policy>"
        "::value");
    static_assert(!hpx::is_vectorpack_execution_policy<
                      std::execution::unsequenced_policy>::value,
        "!hpx::is_vectorpack_execution_policy<std::execution::unsequenced_"
        "policy>::value");
    static_assert(hpx::is_unsequenced_execution_policy<
                      std::execution::unsequenced_policy>::value,
        "hpx::is_unsequenced_execution_policy<std::execution::unsequenced_"
        "policy>::value");
#endif
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    static_checks();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    // Initialize and run HPX
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
