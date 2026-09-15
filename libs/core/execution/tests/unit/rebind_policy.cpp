//  Copyright (c) 2026 Rohan Pattanayak
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// Compile-time tests for the orthogonal rebind_policy_executor_t and
/// rebind_policy_parameters_t customization points. Everything here is
/// checked with static_assert; nothing needs to run.

#include <hpx/execution.hpp>
#include <hpx/execution/executors/rebind_policy.hpp>
#include <hpx/modules/testing.hpp>

#include <type_traits>

namespace exd = hpx::execution::detail;

///////////////////////////////////////////////////////////////////////////
// The default implementation, exercised through a built-in execution
// policy that derives from hpx::execution::detail::execution_policy.
namespace default_customization_point_tests {

    using policy_type = hpx::execution::sequenced_policy;
    using new_executor_type = hpx::execution::parallel_executor;
    using new_parameters_type = hpx::execution::experimental::static_chunk_size;

    // Rebinding the executor leaves the policy's current executor
    // parameters untouched.
    using rebound_by_executor =
        exd::rebind_policy_executor_t<policy_type, new_executor_type>;

    static_assert(std::is_same_v<rebound_by_executor,
                      exd::sequenced_policy_shim<new_executor_type,
                          policy_type::executor_parameters_type>>,
        "rebind_policy_executor_t only changes the executor");

    static_assert(std::is_same_v<typename rebound_by_executor::executor_type,
                      new_executor_type>,
        "rebind_policy_executor_t rebinds to the requested executor");

    static_assert(
        std::is_same_v<typename rebound_by_executor::executor_parameters_type,
            typename policy_type::executor_parameters_type>,
        "rebind_policy_executor_t preserves the current executor parameters");

    // Rebinding the parameters leaves the policy's current executor
    // untouched.
    using rebound_by_parameters =
        exd::rebind_policy_parameters_t<policy_type, new_parameters_type>;

    static_assert(
        std::is_same_v<rebound_by_parameters,
            exd::sequenced_policy_shim<typename policy_type::executor_type,
                new_parameters_type>>,
        "rebind_policy_parameters_t only changes the executor parameters");

    static_assert(std::is_same_v<typename rebound_by_parameters::executor_type,
                      typename policy_type::executor_type>,
        "rebind_policy_parameters_t preserves the current executor");

    static_assert(
        std::is_same_v<typename rebound_by_parameters::executor_parameters_type,
            new_parameters_type>,
        "rebind_policy_parameters_t rebinds to the requested parameters");

    // Applying both customization points in sequence, one axis at a time,
    // is equivalent to rebinding both axes at once through the combined
    // rebind<Executor_, Parameters_>::type mechanism.
    using rebound_both_axes_separately =
        exd::rebind_policy_parameters_t<rebound_by_executor,
            new_parameters_type>;

    using rebound_both_axes_combined =
        typename policy_type::template rebind<new_executor_type,
            new_parameters_type>::type;

    static_assert(std::is_same_v<rebound_both_axes_separately,
                      rebound_both_axes_combined>,
        "rebinding executor and parameters independently, one after the "
        "other, is equivalent to rebinding both at once");

}    // namespace default_customization_point_tests

///////////////////////////////////////////////////////////////////////////
// A policy type that is not shaped as template <typename, typename> class
// Derived, and does not derive from hpx::execution::detail::execution_policy
// at all, but still exposes a nested rebind<Executor_, Parameters_>::type
// member template together with executor_type / executor_parameters_type
// members. It still gets both customization points for free through the
// default implementation.
namespace non_crtp_policy_tests {

    struct duck_typed_executor
    {
    };

    struct other_duck_typed_executor
    {
    };

    struct duck_typed_parameters
    {
    };

    struct other_duck_typed_parameters
    {
    };

    template <typename Executor, typename Parameters>
    struct duck_typed_policy_shim
    {
        using executor_type = Executor;
        using executor_parameters_type = Parameters;

        template <typename Executor_, typename Parameters_>
        struct rebind
        {
            using type = duck_typed_policy_shim<Executor_, Parameters_>;
        };
    };

    using policy_type =
        duck_typed_policy_shim<duck_typed_executor, duck_typed_parameters>;

    using rebound_by_executor =
        exd::rebind_policy_executor_t<policy_type, other_duck_typed_executor>;

    static_assert(std::is_same_v<rebound_by_executor,
                      duck_typed_policy_shim<other_duck_typed_executor,
                          duck_typed_parameters>>,
        "the default implementation works for a policy that does not "
        "derive from hpx::execution::detail::execution_policy");

    using rebound_by_parameters = exd::rebind_policy_parameters_t<policy_type,
        other_duck_typed_parameters>;

    static_assert(std::is_same_v<rebound_by_parameters,
                      duck_typed_policy_shim<duck_typed_executor,
                          other_duck_typed_parameters>>,
        "the default implementation works for a policy that does not "
        "derive from hpx::execution::detail::execution_policy");

}    // namespace non_crtp_policy_tests

///////////////////////////////////////////////////////////////////////////
// A policy type that exposes none of the members the default
// implementation relies on. It can still participate by specializing
// rebind_policy_executor and rebind_policy_parameters directly, and the
// two specializations are independent of one another.
namespace direct_specialization_tests {

    struct opaque_policy
    {
    };

    struct some_executor
    {
    };

    struct some_parameters
    {
    };

    struct opaque_policy_rebound_by_executor
    {
    };

    struct opaque_policy_rebound_by_parameters
    {
    };

}    // namespace direct_specialization_tests

namespace hpx::execution::detail {

    template <typename Executor>
    struct rebind_policy_executor<direct_specialization_tests::opaque_policy,
        Executor>
    {
        using type =
            direct_specialization_tests::opaque_policy_rebound_by_executor;
    };

    template <typename Parameters>
    struct rebind_policy_parameters<direct_specialization_tests::opaque_policy,
        Parameters>
    {
        using type =
            direct_specialization_tests::opaque_policy_rebound_by_parameters;
    };
}    // namespace hpx::execution::detail

namespace direct_specialization_tests {

    static_assert(
        std::is_same_v<
            exd::rebind_policy_executor_t<opaque_policy, some_executor>,
            opaque_policy_rebound_by_executor>,
        "a policy without the default's required members can opt in by "
        "specializing rebind_policy_executor directly");

    static_assert(
        std::is_same_v<
            exd::rebind_policy_parameters_t<opaque_policy, some_parameters>,
            opaque_policy_rebound_by_parameters>,
        "a policy without the default's required members can opt in by "
        "specializing rebind_policy_parameters directly, independently of "
        "rebind_policy_executor");

}    // namespace direct_specialization_tests

///////////////////////////////////////////////////////////////////////////
int main()
{
    return hpx::util::report_errors();
}
