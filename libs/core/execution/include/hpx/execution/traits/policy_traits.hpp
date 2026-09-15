//  Copyright (c) 2026 Rohan Pattanayak
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/execution/traits/policy_traits.hpp

#pragma once

#include <hpx/config.hpp>

namespace hpx::detail {

    /// \brief Default value for every policy_traits member.
    ///
    /// policy_traits itself, and every specialization of policy_traits,
    /// derives from policy_traits_default, so a specialization only needs
    /// to declare the members that differ from the default; every member
    /// it does not redeclare is inherited as false. This also means a new
    /// member added here is picked up by every existing specialization
    /// automatically, instead of silently being left undefined until each
    /// specialization is updated by hand.
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
    };

    /// \brief Canonical description of the properties of a concrete
    ///        execution policy type.
    ///
    /// A new execution policy is made known to the is_execution_policy,
    /// is_parallel_execution_policy, is_sequenced_execution_policy,
    /// is_unsequenced_execution_policy, is_async_execution_policy,
    /// is_rebound_execution_policy, and is_vectorpack_execution_policy
    /// customization points by specializing policy_traits for that policy's
    /// type, deriving the specialization from policy_traits_default, and
    /// setting only the members that are true, instead of specializing each
    /// of those traits individually.
    ///
    /// The primary template below is the fallback used for any type that is
    /// not a recognized execution policy; it inherits every member as false
    /// from policy_traits_default. The behavior of a program that
    /// specializes any of the individual is_*_execution_policy traits
    /// directly, rather than policy_traits, remains unaffected: an explicit
    /// specialization of one of those traits always takes precedence over
    /// the definition derived from policy_traits.
    HPX_CXX_CORE_EXPORT template <typename Policy>
    struct policy_traits : policy_traits_default
    {
    };
}    // namespace hpx::detail
