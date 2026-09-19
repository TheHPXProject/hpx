//  Copyright (c) 2026 Rohan Pattanayak
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// Compile-time tests for the orthogonal rebind_policy_executor_t and
/// rebind_policy_parameters_t customization points. Everything here is
/// checked with static_assert; nothing needs to run.

#include <hpx/execution.hpp>
#include <hpx/execution/executors/create_rebound_policy.hpp>
#include <hpx/execution/executors/rebind_policy.hpp>
#include <hpx/modules/testing.hpp>

#include <type_traits>

namespace exd = hpx::execution::detail;

///////////////////////////////////////////////////////////////////////////
// The default implementation, exercised through a built-in execution
// policy that derives from hpx::execution::detail::execution_policy.
namespace default_customization_point_tests {

    using policy_type = hpx::execution::parallel_policy;
    using new_executor_type = hpx::execution::sequenced_executor;
    using new_parameters_type = hpx::execution::experimental::static_chunk_size;

    // sequenced_executor's category (sequenced_execution_tag) is not
    // weaker than parallel_policy's (parallel_execution_tag), so this
    // rebind satisfies the same safety check as
    // hpx::execution::experimental::rebind_executor.
    static_assert(hpx::execution::experimental::detail::is_not_weaker_v<
                      new_executor_type::execution_category,
                      policy_type::execution_category>,
        "the executor picked for this test must satisfy the category "
        "check rebind_policy_executor_t enforces");

    // Rebinding the executor leaves the policy's current executor
    // parameters untouched.
    using rebound_by_executor =
        exd::rebind_policy_executor_t<policy_type, new_executor_type>;

    static_assert(std::is_same_v<rebound_by_executor,
                      exd::parallel_policy_shim<new_executor_type,
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
            exd::parallel_policy_shim<typename policy_type::executor_type,
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

    // The order-independence contract holds structurally for the default
    // implementation: it always funnels through the same combined
    // rebind<Executor_, Parameters_>::type mechanism regardless of which
    // axis is rebound first.
    static_assert(exd::rebind_policy_order_independent_v<policy_type,
                      new_executor_type, new_parameters_type>,
        "the default implementation is order-independent");

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
// A policy that specializes both rebind_policy_executor and
// rebind_policy_parameters directly, rather than relying on the default
// implementation. rebind_policy_order_independent_v is not guaranteed
// automatically for such a policy; it is verified explicitly below to
// pin down the invariant this specialization relies on.
namespace two_axis_specialization_tests {

    struct executor_a
    {
    };

    struct executor_b
    {
    };

    struct parameters_a
    {
    };

    struct parameters_b
    {
    };

    // Deliberately not shaped as the CRTP execution_policy base expects
    // (no combined rebind<Executor_, Parameters_>::type member), so it
    // must opt in to both customization points explicitly.
    template <typename Executor, typename Parameters>
    struct two_axis_policy
    {
    };

}    // namespace two_axis_specialization_tests

namespace hpx::execution::detail {

    template <typename Executor, typename Parameters, typename NewExecutor>
    struct rebind_policy_executor<
        two_axis_specialization_tests::two_axis_policy<Executor, Parameters>,
        NewExecutor>
    {
        using type = two_axis_specialization_tests::two_axis_policy<
            std::decay_t<NewExecutor>, Parameters>;
    };

    template <typename Executor, typename Parameters, typename NewParameters>
    struct rebind_policy_parameters<
        two_axis_specialization_tests::two_axis_policy<Executor, Parameters>,
        NewParameters>
    {
        using type = two_axis_specialization_tests::two_axis_policy<Executor,
            std::decay_t<NewParameters>>;
    };
}    // namespace hpx::execution::detail

namespace two_axis_specialization_tests {

    using policy_type = two_axis_policy<executor_a, parameters_a>;

    static_assert(
        std::is_same_v<exd::rebind_policy_executor_t<policy_type, executor_b>,
            two_axis_policy<executor_b, parameters_a>>,
        "the executor-axis specialization only changes the executor");

    static_assert(
        std::is_same_v<
            exd::rebind_policy_parameters_t<policy_type, parameters_b>,
            two_axis_policy<executor_a, parameters_b>>,
        "the parameters-axis specialization only changes the parameters");

    static_assert(exd::rebind_policy_order_independent_v<policy_type,
                      executor_b, parameters_b>,
        "a policy specializing both axes directly must keep rebinding "
        "order-independent");

}    // namespace two_axis_specialization_tests

///////////////////////////////////////////////////////////////////////////
// hpx::execution::experimental::create_rebound_policy's single-argument
// overloads now route through rebind_policy_executor_t and
// rebind_policy_parameters_t (see create_rebound_policy.hpp) instead of
// computing rebind_executor_t inline. The type-level tests above cannot
// tell the difference between "the untouched side was copied from the
// original policy" and "the untouched side was silently defaulted", since
// a stock executor/parameters type carries no observable state. This
// section uses a small policy with an id on each side to assert the
// untouched side's *value*, not just its type, actually survives the
// rebind, and that the requested side's value is the one that lands.
namespace construction_state_tests {

    struct labeled_executor
    {
        int id = 0;
    };

    struct labeled_parameters
    {
        int id = 0;
    };

    template <typename Executor, typename Parameters>
    struct labeled_policy
    {
        using executor_type = Executor;
        using executor_parameters_type = Parameters;

        labeled_policy(Executor exec, Parameters params)
          : exec_(exec)
          , params_(params)
        {
        }

        template <typename Executor_, typename Parameters_>
        struct rebind
        {
            using type = labeled_policy<Executor_, Parameters_>;
        };

        Executor executor() const
        {
            return exec_;
        }

        Parameters parameters() const
        {
            return params_;
        }

        Executor exec_;
        Parameters params_;
    };

    void run()
    {
        using policy_type =
            labeled_policy<labeled_executor, labeled_parameters>;

        policy_type const policy(labeled_executor{1}, labeled_parameters{1});

        // Rebinding only the executor must keep the original parameters'
        // value, not just their type, and must construct with the new
        // executor's value.
        auto rebound_by_executor =
            hpx::execution::experimental::create_rebound_policy(
                policy, labeled_executor{2});

        HPX_TEST_EQ(rebound_by_executor.executor().id, 2);
        HPX_TEST_EQ(
            rebound_by_executor.parameters().id, policy.parameters().id);

        // Rebinding only the parameters must keep the original executor's
        // value, and must construct with the new parameters' value.
        auto rebound_by_parameters =
            hpx::execution::experimental::create_rebound_policy(
                policy, labeled_parameters{2});

        HPX_TEST_EQ(rebound_by_parameters.executor().id, policy.executor().id);
        HPX_TEST_EQ(rebound_by_parameters.parameters().id, 2);
    }
}    // namespace construction_state_tests

///////////////////////////////////////////////////////////////////////////
int main()
{
    construction_state_tests::run();
    return hpx::util::report_errors();
}
