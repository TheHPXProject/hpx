//  Copyright (c) 2026 Rohan Pattanayak
//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file policy_traits.hpp
/// \page policy_traits, policy_traits_default

#pragma once

#include <hpx/config.hpp>

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace hpx::execution {

    namespace detail {

        /// \brief Default value for every policy_traits member.
        ///
        /// Every specialization of policy_traits should derive from
        /// policy_traits_default; every member it does not redeclare is
        /// inherited as false. The primary template reaches the same defaults
        /// through member detection. This also means a new member added here is
        /// picked up by every existing specialization automatically, instead of
        /// silently being left undefined until each specialization is updated
        /// by hand.
        HPX_CXX_CORE_EXPORT struct policy_traits_default
        {
            /// Whether Policy is a recognized HPX execution policy.
            static constexpr bool is_policy = false;

            /// Whether Policy was produced by rebinding an executor and/or a
            /// set of executor parameters onto another execution policy.
            static constexpr bool is_rebound = false;

            /// Whether Policy permits its algorithm to run in parallel across
            /// more than one execution agent.
            static constexpr bool is_parallel = false;

            /// Whether Policy requires its algorithm to run on a single
            /// execution agent, without parallelization.
            static constexpr bool is_sequenced = false;

            /// Whether Policy permits its algorithm to be vectorized.
            static constexpr bool is_unsequenced = false;

            /// Whether Policy runs its algorithm asynchronously, returning a
            /// future rather than blocking the calling thread.
            static constexpr bool is_async = false;

            /// Whether Policy operates on vector packs rather than individual
            /// elements.
            static constexpr bool is_vectorpack = false;

#if defined(HPX_HAVE_DATAPAR)
            /// Datapar policies expose the number of vector lanes they cover
            static constexpr std::size_t num_lanes = 0;
#endif
        };
    }    // namespace detail

#define HPX_DEFINE_POLICY_CONCEPT(type, name)                                  \
    namespace detail {                                                         \
        template <typename T>                                                  \
        concept has_##name = requires {                                        \
            { T::name } -> std::convertible_to<type>;                          \
        };                                                                     \
    } /**/

    HPX_DEFINE_POLICY_CONCEPT(bool, is_policy);
    HPX_DEFINE_POLICY_CONCEPT(bool, is_rebound);
    HPX_DEFINE_POLICY_CONCEPT(bool, is_parallel);
    HPX_DEFINE_POLICY_CONCEPT(bool, is_sequenced);
    HPX_DEFINE_POLICY_CONCEPT(bool, is_unsequenced);
    HPX_DEFINE_POLICY_CONCEPT(bool, is_async);
    HPX_DEFINE_POLICY_CONCEPT(bool, is_vectorpack);
#if defined(HPX_HAVE_DATAPAR)
    HPX_DEFINE_POLICY_CONCEPT(std::size_t, num_lanes);
#endif

#undef HPX_DEFINE_POLICY_CONCEPT

#define HPX_DEFINE_POLICY_TRAIT(type, name, default_value)                     \
    static constexpr type name = []() {                                        \
        if constexpr (detail::has_##name<Policy>)                              \
        {                                                                      \
            return Policy::name;                                               \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            return default_value;                                              \
        }                                                                      \
    }() /**/

    /// \brief Canonical description of the properties of a concrete execution
    ///        policy type.
    ///
    /// A new execution policy is made known to the is_execution_policy,
    /// is_parallel_execution_policy, is_sequenced_execution_policy,
    /// is_unsequenced_execution_policy, is_async_execution_policy,
    /// is_rebound_execution_policy, and is_vectorpack_execution_policy
    /// customization points in one of two equivalent ways:
    ///
    /// 1. Members on the policy (preferred for types you own). Declare
    ///    `static constexpr bool` members named `is_policy`, `is_rebound`,
    ///    `is_parallel`, `is_sequenced`, `is_unsequenced`, `is_async`, and
    ///    `is_vectorpack` on the policy type. Declare only the ones that are
    ///    true. The primary template detects each member and reports `false`
    ///    for any that is missing or not convertible to `bool`. Datapar
    ///    policies may also declare `static constexpr std::size_t num_lanes`
    ///    (`0` means native width).
    ///
    /// 2. Specialization (for types you cannot modify). Specialize
    ///    `policy_traits<Policy>` and derive it from
    ///    `hpx::execution::detail::policy_traits_default`. Redeclare only the
    ///    members that differ from the default; the rest are inherited as
    ///    `false` (or `0` for `num_lanes`). The primary template does not
    ///    derive from `policy_traits_default`, so a specialization that omits
    ///    the base class will lack every member it does not declare.
    ///
    /// \code
    ///     // 1. members
    ///     struct my_policy {
    ///         static constexpr bool is_policy = true;
    ///         static constexpr bool is_parallel = true;
    ///     };
    ///
    ///     // 2. specialization template <> struct
    ///     hpx::execution::policy_traits<third_party_policy>
    ///       : hpx::execution::detail::policy_traits_default
    ///     {
    ///         static constexpr bool is_policy = true;
    ///         static constexpr bool is_sequenced = true;
    ///     };
    /// \endcode
    ///
    /// Notes:
    /// - Detection is by member name, so any type that happens to declare a
    ///   member called `is_policy` or `is_parallel` is picked up.
    /// - The traits are evaluated once per type. Specialize or complete the
    ///   type before the first use.
    /// - Under SVE, `num_lanes` is the requested lane count, while the actual
    ///   pack width is the hardware vector length.
    HPX_CXX_CORE_EXPORT template <typename Policy>
    struct policy_traits
    {
        /// Whether Policy is a recognized HPX execution policy.
        HPX_DEFINE_POLICY_TRAIT(bool, is_policy, false);

        /// Whether Policy was produced by rebinding an executor and/or a
        /// set of executor parameters onto another execution policy.
        HPX_DEFINE_POLICY_TRAIT(bool, is_rebound, false);

        /// Whether Policy permits its algorithm to run in parallel across
        /// more than one execution agent.
        HPX_DEFINE_POLICY_TRAIT(bool, is_parallel, false);

        /// Whether Policy requires its algorithm to run on a single
        /// execution agent, without parallelization.
        HPX_DEFINE_POLICY_TRAIT(bool, is_sequenced, false);

        /// Whether Policy permits its algorithm to be vectorized.
        HPX_DEFINE_POLICY_TRAIT(bool, is_unsequenced, false);

        /// Whether Policy runs its algorithm asynchronously, returning a
        /// future rather than blocking the calling thread.
        HPX_DEFINE_POLICY_TRAIT(bool, is_async, false);

        /// Whether Policy operates on vector packs rather than individual
        /// elements.
        HPX_DEFINE_POLICY_TRAIT(bool, is_vectorpack, false);

#if defined(HPX_HAVE_DATAPAR)
        /// Number of vector lanes requested by a datapar policy (0 selects the
        /// native pack width, 1 is scalar). This is a request, not a guarantee:
        /// backends that cannot honor fixed lane counts (e.g., SVE) use their
        /// hardware vector length.
        HPX_DEFINE_POLICY_TRAIT(std::size_t, num_lanes, 0);
#endif
    };

#undef HPX_DEFINE_POLICY_TRAIT
}    // namespace hpx::execution
