//  Copyright (c) 2026 Rohan Pattanayak
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// Tests for the orthogonal rebind_policy_executor_t and
/// rebind_policy_parameters_t customization points and their
/// construction-side counterparts. Type-level properties are checked with
/// static_assert; the construction tests run.

#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/testing.hpp>

// hpx::execution::detail::parallel_policy_shim,
// hpx::execution::parallel_executor, and hpx::execution::sequenced_executor
// live in the executors module, not the execution module, so
// hpx/modules/execution.hpp does not transitively provide them. Pulling
// them in through the executors module's own generated header, rather
// than including the raw hpx/executors headers directly, keeps this
// working under the C++20 modules build, where those raw headers are
// already brought in through the module import.
#include <hpx/modules/executors.hpp>

#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace exd = hpx::execution::detail;
namespace hpxexp = hpx::execution::experimental;

///////////////////////////////////////////////////////////////////////////
// The default implementation, exercised through a built-in execution
// policy that derives from hpx::execution::detail::execution_policy.
namespace default_customization_point_tests {

    using policy_type = hpx::execution::parallel_policy;
    using new_executor_type = hpx::execution::sequenced_executor;
    using new_parameters_type = hpxexp::static_chunk_size;

    // sequenced_executor's category (sequenced_execution_tag) is not
    // weaker than parallel_executor's (parallel_execution_tag), so this
    // rebind satisfies the category check rebind_policy_executor_t's
    // default implementation enforces internally (see
    // hpx/execution/executors/rebind_policy.hpp). That check is exercised
    // for real below, at the point rebind_policy_executor_t is actually
    // used; it is not repeated here directly against
    // hpx::execution::experimental::detail::is_not_weaker_v, since that
    // is an unexported, module-internal implementation detail and is not
    // reachable from this translation unit under the C++20 modules build.

    // Rebinding the executor leaves the policy's current executor
    // parameters untouched.
    using rebound_by_executor =
        hpxexp::rebind_policy_executor_t<policy_type, new_executor_type>;

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
        hpxexp::rebind_policy_parameters_t<policy_type, new_parameters_type>;

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
        hpxexp::rebind_policy_parameters_t<rebound_by_executor,
            new_parameters_type>;

    using rebound_both_axes_combined =
        policy_type::template rebind<new_executor_type,
            new_parameters_type>::type;

    static_assert(std::is_same_v<rebound_both_axes_separately,
                      rebound_both_axes_combined>,
        "rebinding executor and parameters independently, one after the "
        "other, is equivalent to rebinding both at once");

    // The order-independence contract holds structurally for the default
    // implementation: it always funnels through the same combined
    // rebind<Executor_, Parameters_>::type mechanism regardless of which
    // axis is rebound first.
    static_assert(hpxexp::rebind_policy_order_independent_v<policy_type,
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

    using rebound_by_executor = hpxexp::rebind_policy_executor_t<policy_type,
        other_duck_typed_executor>;

    static_assert(std::is_same_v<rebound_by_executor,
                      duck_typed_policy_shim<other_duck_typed_executor,
                          duck_typed_parameters>>,
        "the default implementation works for a policy that does not "
        "derive from hpx::execution::detail::execution_policy");

    using rebound_by_parameters =
        hpxexp::rebind_policy_parameters_t<policy_type,
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

namespace hpx::execution::experimental {

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
}    // namespace hpx::execution::experimental

namespace direct_specialization_tests {

    static_assert(
        std::is_same_v<
            hpxexp::rebind_policy_executor_t<opaque_policy, some_executor>,
            opaque_policy_rebound_by_executor>,
        "a policy without the default's required members can opt in by "
        "specializing rebind_policy_executor directly");

    static_assert(
        std::is_same_v<
            hpxexp::rebind_policy_parameters_t<opaque_policy, some_parameters>,
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

namespace hpx::execution::experimental {

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
}    // namespace hpx::execution::experimental

namespace two_axis_specialization_tests {

    using policy_type = two_axis_policy<executor_a, parameters_a>;

    static_assert(std::is_same_v<
                      hpxexp::rebind_policy_executor_t<policy_type, executor_b>,
                      two_axis_policy<executor_b, parameters_a>>,
        "the executor-axis specialization only changes the executor");

    static_assert(
        std::is_same_v<
            hpxexp::rebind_policy_parameters_t<policy_type, parameters_b>,
            two_axis_policy<executor_a, parameters_b>>,
        "the parameters-axis specialization only changes the parameters");

    static_assert(hpxexp::rebind_policy_order_independent_v<policy_type,
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

}    // namespace construction_state_tests

namespace hpx::execution::experimental {

    // create_rebound_policy's single-argument overloads are constrained on
    // hpx::executor_any/hpx::executor_parameters; labeled_executor and
    // labeled_parameters only need to satisfy those constraints, not behave
    // as functioning executors or parameters, since nothing here ever
    // executes anything through them.
    template <>
    struct is_one_way_executor<construction_state_tests::labeled_executor>
      : std::true_type
    {
    };

    template <>
    struct is_executor_parameters<construction_state_tests::labeled_parameters>
      : std::true_type
    {
    };

}    // namespace hpx::execution::experimental

namespace construction_state_tests {

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
// Rebinding only the executor obtains the unchanged parameters type through
// hpx::execution::experimental::extract_executor_parameters_t, the same way
// create_rebound_policy did before the per-axis customization points were
// introduced: a policy without a nested executor_parameters_type falls back
// to sequential_executor_parameters, and an explicit specialization of
// extract_executor_parameters is honored.
namespace parameter_extraction_tests {

    struct tagged_executor
    {
        using execution_category = hpx::execution::parallel_execution_tag;

        int id = 0;
    };

    struct custom_parameters
    {
        int id = 0;
    };

    // No nested executor_parameters_type member.
    template <typename Executor, typename Parameters>
    struct no_parameters_member_policy
    {
        using execution_category = hpx::execution::parallel_execution_tag;

        template <typename Executor_, typename Parameters_>
        struct rebind
        {
            using type = no_parameters_member_policy<Executor_, Parameters_>;
        };

        no_parameters_member_policy(Executor exec, Parameters params)
          : exec_(exec)
          , params_(params)
        {
        }

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

    // Same shape, but with an explicit extract_executor_parameters
    // specialization below.
    template <typename Executor, typename Parameters>
    struct extracted_parameters_policy
      : no_parameters_member_policy<Executor, Parameters>
    {
        using no_parameters_member_policy<Executor,
            Parameters>::no_parameters_member_policy;

        template <typename Executor_, typename Parameters_>
        struct rebind
        {
            using type = extracted_parameters_policy<Executor_, Parameters_>;
        };
    };

}    // namespace parameter_extraction_tests

namespace hpx::execution::experimental {

    template <>
    struct is_one_way_executor<parameter_extraction_tests::tagged_executor>
      : std::true_type
    {
    };

    template <typename Executor, typename Parameters>
    struct extract_executor_parameters<parameter_extraction_tests::
            extracted_parameters_policy<Executor, Parameters>>
    {
        using type = Parameters;
    };

}    // namespace hpx::execution::experimental

namespace parameter_extraction_tests {

    using fallback_policy = no_parameters_member_policy<tagged_executor,
        hpx::execution::experimental::sequential_executor_parameters>;

    static_assert(
        std::is_same_v<
            hpxexp::rebind_policy_executor_t<fallback_policy, tagged_executor>,
            fallback_policy>,
        "a policy without executor_parameters_type keeps "
        "sequential_executor_parameters when rebinding the executor");

    using specialized_policy =
        extracted_parameters_policy<tagged_executor, custom_parameters>;

    static_assert(
        std::is_same_v<hpxexp::rebind_policy_executor_t<specialized_policy,
                           tagged_executor>,
            specialized_policy>,
        "an explicit extract_executor_parameters specialization is honored "
        "when rebinding the executor");

    void run()
    {
        fallback_policy const fallback(tagged_executor{1}, {});

        auto rebound_fallback =
            hpx::execution::experimental::create_rebound_policy(
                fallback, tagged_executor{2});

        HPX_TEST_EQ(rebound_fallback.executor().id, 2);

        specialized_policy const specialized(
            tagged_executor{1}, custom_parameters{3});

        auto rebound_specialized =
            hpx::execution::experimental::create_rebound_policy(
                specialized, tagged_executor{2});

        HPX_TEST_EQ(rebound_specialized.executor().id, 2);
        HPX_TEST_EQ(rebound_specialized.parameters().id, 3);
    }
}    // namespace parameter_extraction_tests

///////////////////////////////////////////////////////////////////////////
// A policy that cannot be constructed from just (executor, parameters): it
// needs a label that only the original policy knows. It opts in to both
// construction-side customization points, and the label has to survive
// rebinding along either axis, through create_rebound_policy as well as
// through the per-axis function objects directly.
namespace custom_construction_tests {

    using construction_state_tests::labeled_executor;
    using construction_state_tests::labeled_parameters;

    template <typename Executor, typename Parameters>
    struct three_argument_policy
    {
        using executor_type = Executor;
        using executor_parameters_type = Parameters;

        template <typename Executor_, typename Parameters_>
        struct rebind
        {
            using type = three_argument_policy<Executor_, Parameters_>;
        };

        three_argument_policy(int label, Executor exec, Parameters params)
          : label_(label)
          , exec_(exec)
          , params_(params)
        {
        }

        int label() const
        {
            return label_;
        }

        Executor executor() const
        {
            return exec_;
        }

        Parameters parameters() const
        {
            return params_;
        }

        int label_;
        Executor exec_;
        Parameters params_;
    };

}    // namespace custom_construction_tests

namespace hpx::execution::experimental {

    template <typename Executor, typename Parameters, typename NewExecutor>
    struct construct_rebound_policy_executor<
        custom_construction_tests::three_argument_policy<Executor, Parameters>,
        NewExecutor>
    {
        using result_type =
            custom_construction_tests::three_argument_policy<NewExecutor,
                Parameters>;

        template <typename Executor_>
        static result_type call(
            custom_construction_tests::three_argument_policy<Executor,
                Parameters> const& policy,
            Executor_&& exec)
        {
            return result_type(policy.label(), std::forward<Executor_>(exec),
                policy.parameters());
        }
    };

    template <typename Executor, typename Parameters, typename NewParameters>
    struct construct_rebound_policy_parameters<
        custom_construction_tests::three_argument_policy<Executor, Parameters>,
        NewParameters>
    {
        using result_type =
            custom_construction_tests::three_argument_policy<Executor,
                NewParameters>;

        template <typename Parameters_>
        static result_type call(
            custom_construction_tests::three_argument_policy<Executor,
                Parameters> const& policy,
            Parameters_&& params)
        {
            return result_type(policy.label(), policy.executor(),
                std::forward<Parameters_>(params));
        }
    };
}    // namespace hpx::execution::experimental

namespace custom_construction_tests {

    void run()
    {
        using policy_type =
            three_argument_policy<labeled_executor, labeled_parameters>;

        policy_type const policy(
            42, labeled_executor{1}, labeled_parameters{1});

        auto rebound_by_executor =
            hpx::execution::experimental::create_rebound_policy(
                policy, labeled_executor{2});

        HPX_TEST_EQ(rebound_by_executor.label(), 42);
        HPX_TEST_EQ(rebound_by_executor.executor().id, 2);
        HPX_TEST_EQ(rebound_by_executor.parameters().id, 1);

        auto rebound_by_parameters =
            hpx::execution::experimental::create_rebound_policy(
                policy, labeled_parameters{2});

        HPX_TEST_EQ(rebound_by_parameters.label(), 42);
        HPX_TEST_EQ(rebound_by_parameters.executor().id, 1);
        HPX_TEST_EQ(rebound_by_parameters.parameters().id, 2);

        // The per-axis function objects dispatch to the same hooks.
        auto rebound_directly = hpxexp::create_rebound_policy_parameters(
            hpxexp::create_rebound_policy_executor(policy, labeled_executor{3}),
            labeled_parameters{4});

        HPX_TEST_EQ(rebound_directly.label(), 42);
        HPX_TEST_EQ(rebound_directly.executor().id, 3);
        HPX_TEST_EQ(rebound_directly.parameters().id, 4);
    }
}    // namespace custom_construction_tests

///////////////////////////////////////////////////////////////////////////
/// The member functions of the exd::execution_policy CRTP base (on(), with()
/// and the scheduling property queries) rebind through
/// create_rebound_policy_executor and create_rebound_policy_parameters. A
/// policy derived from that base that carries a label its construction-side
/// specializations have to carry over must keep the label along each of
/// these paths.

using construction_state_tests::labeled_executor;
using construction_state_tests::labeled_parameters;
using parameter_extraction_tests::tagged_executor;

/// An executor that supports the scheduling properties the execution_policy
/// CRTP base forwards through query() members, so each property path can be
/// exercised without a running thread pool.
struct scheduling_executor
{
    using execution_category = hpx::execution::parallel_execution_tag;

    scheduling_executor query(
        hpxexp::with_priority_t, hpx::threads::thread_priority value) const
    {
        scheduling_executor exec = *this;
        exec.priority = value;
        return exec;
    }

    hpx::threads::thread_priority query(hpxexp::get_priority_t) const
    {
        return priority;
    }

    scheduling_executor query(
        hpxexp::with_stacksize_t, hpx::threads::thread_stacksize value) const
    {
        scheduling_executor exec = *this;
        exec.stacksize = value;
        return exec;
    }

    hpx::threads::thread_stacksize query(hpxexp::get_stacksize_t) const
    {
        return stacksize;
    }

    scheduling_executor query(
        hpxexp::with_processing_units_count_t, std::size_t value) const
    {
        scheduling_executor exec = *this;
        exec.cores = value;
        return exec;
    }

    /// Used by the execution_policy query that derives the number of cores
    /// from an executor parameters object.
    std::size_t query(hpxexp::processing_units_count_t,
        labeled_parameters const& params, hpx::chrono::steady_duration const&,
        std::size_t) const
    {
        return static_cast<std::size_t>(params.id);
    }

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
    scheduling_executor query(
        hpxexp::with_annotation_t, char const* value) const
    {
        scheduling_executor exec = *this;
        exec.annotation = value;
        return exec;
    }

    char const* query(hpxexp::get_annotation_t) const
    {
        return annotation;
    }
#endif

    int id = 0;
    hpx::threads::thread_priority priority =
        hpx::threads::thread_priority::default_;
    hpx::threads::thread_stacksize stacksize =
        hpx::threads::thread_stacksize::default_;
    std::size_t cores = 0;
    char const* annotation = nullptr;
};

template <>
struct hpxexp::is_one_way_executor<scheduling_executor> : std::true_type
{
};

/// A policy derived from the execution_policy CRTP base that cannot be
/// constructed from just (executor, parameters).
template <typename Executor, typename Parameters>
struct labeled_crtp_policy
  : exd::execution_policy<labeled_crtp_policy, Executor, Parameters>
{
    using base_type =
        exd::execution_policy<labeled_crtp_policy, Executor, Parameters>;

    template <typename Executor_, typename Parameters_>
    labeled_crtp_policy(int label, Executor_&& exec, Parameters_&& params)
      : base_type(
            std::forward<Executor_>(exec), std::forward<Parameters_>(params))
      , label_(label)
    {
    }

    int label() const
    {
        return label_;
    }

    int label_;
};

template <typename Executor, typename Parameters, typename NewExecutor>
struct hpxexp::construct_rebound_policy_executor<
    labeled_crtp_policy<Executor, Parameters>, NewExecutor>
{
    using result_type = labeled_crtp_policy<NewExecutor, Parameters>;

    template <typename Executor_>
    static result_type call(
        labeled_crtp_policy<Executor, Parameters> const& policy,
        Executor_&& exec)
    {
        return result_type(
            policy.label(), std::forward<Executor_>(exec), policy.parameters());
    }
};

template <typename Executor, typename Parameters, typename NewParameters>
struct hpxexp::construct_rebound_policy_parameters<
    labeled_crtp_policy<Executor, Parameters>, NewParameters>
{
    using result_type = labeled_crtp_policy<Executor, NewParameters>;

    template <typename Parameters_>
    static result_type call(
        labeled_crtp_policy<Executor, Parameters> const& policy,
        Parameters_&& params)
    {
        return result_type(policy.label(), policy.executor(),
            std::forward<Parameters_>(params));
    }
};

using labeled_crtp_policy_type =
    labeled_crtp_policy<scheduling_executor, labeled_parameters>;

void crtp_member_rebind_tests()
{
    labeled_crtp_policy_type const policy(
        42, scheduling_executor{1}, labeled_parameters{1});

    /// on() with an rvalue executor.
    auto rebound_by_on = policy.on(scheduling_executor{2});

    static_assert(
        std::is_same_v<decltype(rebound_by_on), labeled_crtp_policy_type>);

    HPX_TEST_EQ(rebound_by_on.label(), 42);
    HPX_TEST_EQ(rebound_by_on.executor().id, 2);
    HPX_TEST_EQ(rebound_by_on.parameters().id, 1);

    /// on() with an lvalue executor.
    scheduling_executor const exec{3};
    auto rebound_by_on_lvalue = policy.on(exec);

    static_assert(std::is_same_v<decltype(rebound_by_on_lvalue),
        labeled_crtp_policy_type>);

    HPX_TEST_EQ(rebound_by_on_lvalue.label(), 42);
    HPX_TEST_EQ(rebound_by_on_lvalue.executor().id, 3);
    HPX_TEST_EQ(rebound_by_on_lvalue.parameters().id, 1);

    /// on() called on an rvalue policy.
    auto rebound_from_rvalue = labeled_crtp_policy_type(
        7, scheduling_executor{1}, labeled_parameters{1})
                                   .on(scheduling_executor{4});

    static_assert(std::is_same_v<decltype(rebound_from_rvalue),
        labeled_crtp_policy_type>);

    HPX_TEST_EQ(rebound_from_rvalue.label(), 7);
    HPX_TEST_EQ(rebound_from_rvalue.executor().id, 4);

    /// on() to a different executor type.
    auto rebound_to_other_executor = policy.on(tagged_executor{5});

    static_assert(std::is_same_v<decltype(rebound_to_other_executor),
        labeled_crtp_policy<tagged_executor, labeled_parameters>>);

    HPX_TEST_EQ(rebound_to_other_executor.label(), 42);
    HPX_TEST_EQ(rebound_to_other_executor.executor().id, 5);
    HPX_TEST_EQ(rebound_to_other_executor.parameters().id, 1);

    /// with() rebinds the parameters.
    auto rebound_by_with = policy.with(labeled_parameters{2});

    static_assert(
        std::is_same_v<decltype(rebound_by_with), labeled_crtp_policy_type>);

    HPX_TEST_EQ(rebound_by_with.label(), 42);
    HPX_TEST_EQ(rebound_by_with.executor().id, 1);
    HPX_TEST_EQ(rebound_by_with.parameters().id, 2);

    /// with() to a different parameters type.
    auto rebound_to_other_parameters =
        policy.with(hpxexp::static_chunk_size(4));

    static_assert(std::is_same_v<decltype(rebound_to_other_parameters),
        labeled_crtp_policy<scheduling_executor, hpxexp::static_chunk_size>>);

    HPX_TEST_EQ(rebound_to_other_parameters.label(), 42);
    HPX_TEST_EQ(rebound_to_other_parameters.executor().id, 1);

    /// The generic scheduling property query.
    auto rebound_by_priority =
        hpxexp::with_priority(policy, hpx::threads::thread_priority::high);

    static_assert(std::is_same_v<decltype(rebound_by_priority),
        labeled_crtp_policy_type>);

    HPX_TEST_EQ(rebound_by_priority.label(), 42);
    HPX_TEST_EQ(rebound_by_priority.executor().id, 1);
    HPX_TEST_EQ(rebound_by_priority.parameters().id, 1);
    HPX_TEST(hpxexp::get_priority(rebound_by_priority) ==
        hpx::threads::thread_priority::high);

    auto rebound_by_stacksize =
        hpxexp::with_stacksize(policy, hpx::threads::thread_stacksize::medium);

    HPX_TEST_EQ(rebound_by_stacksize.label(), 42);
    HPX_TEST(hpxexp::get_stacksize(rebound_by_stacksize) ==
        hpx::threads::thread_stacksize::medium);

    /// The processing units count query taking a number of cores.
    auto rebound_by_cores =
        policy.query(hpxexp::with_processing_units_count, std::size_t(4));

    static_assert(
        std::is_same_v<decltype(rebound_by_cores), labeled_crtp_policy_type>);

    HPX_TEST_EQ(rebound_by_cores.label(), 42);
    HPX_TEST_EQ(rebound_by_cores.executor().cores, std::size_t(4));
    HPX_TEST_EQ(rebound_by_cores.parameters().id, 1);

    /// The processing units count query taking executor parameters.
    auto rebound_by_parameters_cores = policy.query(
        hpxexp::with_processing_units_count, labeled_parameters{6});

    static_assert(std::is_same_v<decltype(rebound_by_parameters_cores),
        labeled_crtp_policy_type>);

    HPX_TEST_EQ(rebound_by_parameters_cores.label(), 42);
    HPX_TEST_EQ(rebound_by_parameters_cores.executor().cores, std::size_t(6));
    HPX_TEST_EQ(rebound_by_parameters_cores.parameters().id, 1);

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
    /// The annotation query.
    char const* annotation = "rebind";
    auto rebound_by_annotation = hpxexp::with_annotation(policy, annotation);

    static_assert(std::is_same_v<decltype(rebound_by_annotation),
        labeled_crtp_policy_type>);

    HPX_TEST_EQ(rebound_by_annotation.label(), 42);
    HPX_TEST(hpxexp::get_annotation(rebound_by_annotation) == annotation);
#endif
}

///////////////////////////////////////////////////////////////////////////
/// The rebind traits decay Policy, so cv- and reference-qualified policy
/// types rebind to the same type, and the function objects accept policies
/// of any value category.

static_assert(
    std::is_same_v<hpxexp::rebind_policy_executor_t<
                       labeled_crtp_policy_type const&, tagged_executor>,
        hpxexp::rebind_policy_executor_t<labeled_crtp_policy_type,
            tagged_executor>>);
static_assert(
    std::is_same_v<hpxexp::rebind_policy_executor_t<labeled_crtp_policy_type&&,
                       tagged_executor const&>,
        hpxexp::rebind_policy_executor_t<labeled_crtp_policy_type,
            tagged_executor>>);
static_assert(std::is_same_v<
    hpxexp::rebind_policy_parameters_t<labeled_crtp_policy_type const,
        hpxexp::static_chunk_size&>,
    hpxexp::rebind_policy_parameters_t<labeled_crtp_policy_type,
        hpxexp::static_chunk_size>>);
static_assert(
    std::is_same_v<hpxexp::rebind_policy_parameters_t<labeled_crtp_policy_type&,
                       hpxexp::static_chunk_size&&>,
        hpxexp::rebind_policy_parameters_t<labeled_crtp_policy_type,
            hpxexp::static_chunk_size>>);

void qualified_policy_rebind_tests()
{
    labeled_crtp_policy_type policy(
        42, scheduling_executor{1}, labeled_parameters{1});

    auto from_lvalue =
        hpxexp::create_rebound_policy_executor(policy, tagged_executor{2});
    HPX_TEST_EQ(from_lvalue.label(), 42);
    HPX_TEST_EQ(from_lvalue.executor().id, 2);

    auto from_const_lvalue = hpxexp::create_rebound_policy_executor(
        std::as_const(policy), tagged_executor{3});
    HPX_TEST_EQ(from_const_lvalue.label(), 42);
    HPX_TEST_EQ(from_const_lvalue.executor().id, 3);

    auto from_rvalue = hpxexp::create_rebound_policy_parameters(
        labeled_crtp_policy_type(policy), labeled_parameters{4});
    HPX_TEST_EQ(from_rvalue.label(), 42);
    HPX_TEST_EQ(from_rvalue.parameters().id, 4);

    auto from_const_rvalue = hpxexp::create_rebound_policy_parameters(
        std::move(std::as_const(policy)), labeled_parameters{5});
    HPX_TEST_EQ(from_const_rvalue.label(), 42);
    HPX_TEST_EQ(from_const_rvalue.parameters().id, 5);
}

///////////////////////////////////////////////////////////////////////////
/// The category check and the check that a construction-side specialization
/// produces rebind_policy_*_t are constraints, so a rejected rebind can be
/// detected instead of failing to compile.

template <typename Policy, typename Executor>
concept executor_rebindable =
    requires { typename hpxexp::rebind_policy_executor_t<Policy, Executor>; };

template <typename Policy, typename Parameters>
concept parameters_rebindable = requires {
    typename hpxexp::rebind_policy_parameters_t<Policy, Parameters>;
};

template <typename Policy, typename Executor>
concept executor_constructible =
    std::invocable<hpxexp::create_rebound_policy_executor_t const&, Policy,
        Executor>;

template <typename Policy, typename Parameters>
concept parameters_constructible =
    std::invocable<hpxexp::create_rebound_policy_parameters_t const&, Policy,
        Parameters>;

/// A sequenced policy can't be rebound to a parallel executor.
static_assert(executor_rebindable<hpx::execution::sequenced_policy,
    hpx::execution::sequenced_executor>);
static_assert(!executor_rebindable<hpx::execution::sequenced_policy,
    hpx::execution::parallel_executor>);
static_assert(!executor_constructible<hpx::execution::sequenced_policy const&,
    hpx::execution::parallel_executor>);
static_assert(executor_rebindable<hpx::execution::parallel_policy,
    hpx::execution::sequenced_executor>);
static_assert(executor_constructible<hpx::execution::parallel_policy const&,
    hpx::execution::sequenced_executor>);

/// A policy whose current executor is weaker than the policy itself can't
/// be rebound to new parameters.
template <typename Executor, typename Parameters>
struct overstated_category_policy
{
    using execution_category = hpx::execution::sequenced_execution_tag;
    using executor_type = Executor;
    using executor_parameters_type = Parameters;

    template <typename Executor_, typename Parameters_>
    struct rebind
    {
        using type = overstated_category_policy<Executor_, Parameters_>;
    };
};

static_assert(!parameters_rebindable<
    overstated_category_policy<hpx::execution::parallel_executor,
        labeled_parameters>,
    hpxexp::static_chunk_size>);
static_assert(!parameters_constructible<
    overstated_category_policy<hpx::execution::parallel_executor,
        labeled_parameters> const&,
    hpxexp::static_chunk_size>);
static_assert(parameters_rebindable<
    overstated_category_policy<hpx::execution::sequenced_executor,
        labeled_parameters>,
    hpxexp::static_chunk_size>);

/// A policy whose construction-side specializations return the original
/// policy type instead of the rebound one.
template <typename Executor, typename Parameters>
struct misconstructed_policy
{
    using executor_type = Executor;
    using executor_parameters_type = Parameters;

    template <typename Executor_, typename Parameters_>
    struct rebind
    {
        using type = misconstructed_policy<Executor_, Parameters_>;
    };
};

template <typename Executor, typename Parameters, typename NewExecutor>
struct hpxexp::construct_rebound_policy_executor<
    misconstructed_policy<Executor, Parameters>, NewExecutor>
{
    template <typename Executor_>
    static misconstructed_policy<Executor, Parameters> call(
        misconstructed_policy<Executor, Parameters> const& policy, Executor_&&)
    {
        return policy;
    }
};

template <typename Executor, typename Parameters, typename NewParameters>
struct hpxexp::construct_rebound_policy_parameters<
    misconstructed_policy<Executor, Parameters>, NewParameters>
{
    template <typename Parameters_>
    static misconstructed_policy<Executor, Parameters> call(
        misconstructed_policy<Executor, Parameters> const& policy,
        Parameters_&&)
    {
        return policy;
    }
};

using misconstructed_policy_type =
    misconstructed_policy<labeled_executor, labeled_parameters>;

static_assert(
    executor_rebindable<misconstructed_policy_type, scheduling_executor>);
static_assert(!executor_constructible<misconstructed_policy_type const&,
    scheduling_executor>);
static_assert(parameters_rebindable<misconstructed_policy_type,
    hpxexp::static_chunk_size>);
static_assert(!parameters_constructible<misconstructed_policy_type const&,
    hpxexp::static_chunk_size>);

///////////////////////////////////////////////////////////////////////////
/// The predefined policies rebind through the same path.

template <typename Policy>
void standard_policy_rebind_test(Policy const& policy)
{
    using executor_type = typename Policy::executor_type;
    using parameters_type = typename Policy::executor_parameters_type;

    auto rebound_by_on = policy.on(hpx::execution::sequenced_executor{});

    static_assert(std::is_same_v<decltype(rebound_by_on),
        hpxexp::rebind_policy_executor_t<Policy,
            hpx::execution::sequenced_executor>>);
    static_assert(
        std::is_same_v<typename decltype(rebound_by_on)::executor_type,
            hpx::execution::sequenced_executor>);
    static_assert(std::is_same_v<
        typename decltype(rebound_by_on)::executor_parameters_type,
        parameters_type>);

    auto rebound_by_with = policy.with(hpxexp::static_chunk_size(4));

    static_assert(std::is_same_v<decltype(rebound_by_with),
        hpxexp::rebind_policy_parameters_t<Policy, hpxexp::static_chunk_size>>);
    static_assert(
        std::is_same_v<typename decltype(rebound_by_with)::executor_type,
            executor_type>);
    static_assert(std::is_same_v<
        typename decltype(rebound_by_with)::executor_parameters_type,
        hpxexp::static_chunk_size>);

    auto rebound_directly = hpxexp::create_rebound_policy(
        policy, hpx::execution::sequenced_executor{});

    static_assert(
        std::is_same_v<decltype(rebound_directly), decltype(rebound_by_on)>);
}

void standard_policy_rebind_tests()
{
    standard_policy_rebind_test(hpx::execution::seq);
    standard_policy_rebind_test(hpx::execution::par);
    standard_policy_rebind_test(hpx::execution::par_unseq);
}

///////////////////////////////////////////////////////////////////////////
int main()
{
    construction_state_tests::run();
    parameter_extraction_tests::run();
    custom_construction_tests::run();
    crtp_member_rebind_tests();
    qualified_policy_rebind_tests();
    standard_policy_rebind_tests();
    return hpx::util::report_errors();
}
