//  Copyright (c) 2016-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)
#include <hpx/executors/datapar/detail/execution_policy_mapping_members.hpp>
#include <hpx/executors/datapar/execution_policy_fwd.hpp>
#include <hpx/executors/datapar/execution_policy_mappings.hpp>
#include <hpx/executors/execution_policy.hpp>
#include <hpx/executors/parallel_executor.hpp>
#include <hpx/executors/sequenced_executor.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/properties.hpp>
#include <hpx/modules/serialization.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx::execution {

    namespace detail {

        // Extension: The class simd_task_policy_shim is an execution policy
        // type used as a unique type to disambiguate parallel algorithm
        // overloading based on combining an underlying \a sequenced_task_policy
        // and an executor and indicate that a parallel algorithm's execution
        // may not be parallelized  (has to run sequentially). It should
        // vectorize the execution of the algorithm using the given number of
        // vector lanes N (if N == 0, the native number of vector lanes is
        // used).
        //
        // The algorithm returns a future representing the result of the
        // corresponding algorithm when invoked with the sequenced_policy.
        HPX_CXX_CORE_EXPORT template <std::size_t N>
        struct simd_task_policy_shim
        {
            template <typename Executor, typename Parameters>
            struct policy
              : execution_policy<policy, Executor, Parameters,
                    unsequenced_execution_tag>
              , simd_async_policy_mappings<policy<Executor, Parameters>>
            {
            private:
                using base_type = execution_policy<policy, Executor, Parameters,
                    unsequenced_execution_tag>;

            public:
                /// \cond NOINTERNAL
                static constexpr bool is_policy = true;
                static constexpr bool is_sequenced = true;
                static constexpr bool is_async = true;
                static constexpr bool is_vectorpack = true;

                static constexpr std::size_t num_lanes = N;

                constexpr policy() = default;
#if defined(__NVCC__) || defined(__CUDACC__)
                constexpr ~policy() {}
#endif

                template <typename Executor_, typename Parameters_>
                constexpr policy(Executor_&& exec, Parameters_&& params)
                  : base_type(HPX_FORWARD(Executor_, exec),
                        HPX_FORWARD(Parameters_, params))
                {
                }

                template <typename Executor_, typename Parameters_>
                    requires(!std::is_same_v<policy<Executor_, Parameters_>,
                                 policy> &&
                        std::is_convertible_v<Executor_, Executor> &&
                        std::is_convertible_v<Parameters_, Parameters>)
                explicit constexpr policy(
                    policy<Executor_, Parameters_> const& rhs)
                  : base_type(policy(rhs.executor(), rhs.parameters()))
                {
                }

                template <typename Executor_, typename Parameters_>
                    requires(!std::is_same_v<policy<Executor_, Parameters_>,
                                 policy> &&
                        std::is_convertible_v<Executor_, Executor> &&
                        std::is_convertible_v<Parameters_, Parameters>)
                policy& operator=(policy<Executor_, Parameters_> const& rhs)
                {
                    base_type::operator=(
                        policy(rhs.executor(), rhs.parameters()));
                    return *this;
                }
                /// \endcond
            };
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    /// Extension: The class simd_task_policy is an execution policy type used
    /// as a unique type to disambiguate parallel algorithm overloading and
    /// indicate that a parallel algorithm's execution may not be parallelized
    /// (has to run sequentially). It should vectorize the execution of the
    /// algorithm using the native number of vector lanes.
    ///
    /// The algorithm returns a future representing the result of the
    /// corresponding algorithm when invoked with the sequenced_policy.
    HPX_CXX_CORE_EXPORT using simd_task_policy =
        detail::simd_task_policy_shim<0>::policy<sequenced_executor,
            hpx::traits::executor_parameters_type_t<sequenced_executor>>;

    /// The class fixed_size_simd_task_policy is an execution policy type used
    /// as a unique type to disambiguate parallel algorithm overloading and
    /// require that a parallel algorithm's execution may not be parallelized.
    /// It should vectorize the execution of the algorithm using the given
    /// number of vector lanes.
    ///
    /// The algorithm returns a future representing the result of the
    /// corresponding algorithm when invoked with the sequenced_policy.
    HPX_CXX_CORE_EXPORT template <std::size_t N>
    using fixed_size_simd_task_policy =
        detail::simd_task_policy_shim<N>::template policy<sequenced_executor,
            hpx::traits::executor_parameters_type_t<sequenced_executor>>;

    namespace detail {

        // The class simd_policy is an execution policy type used as a unique
        // type to disambiguate parallel algorithm overloading and require that
        // a parallel algorithm's execution may not be parallelized. It should
        // vectorize the execution of the algorithm using the given number of
        // vector lanes N (if N == 0, the native number of vector lanes is
        // used).
        HPX_CXX_CORE_EXPORT template <std::size_t N>
        struct simd_policy_shim
        {
            template <typename Executor, typename Parameters>
            struct policy
              : execution_policy<policy, Executor, Parameters,
                    unsequenced_execution_tag>
              , simd_sync_policy_mappings<policy<Executor, Parameters>>
            {
            private:
                using base_type = execution_policy<policy, Executor, Parameters,
                    unsequenced_execution_tag>;

            public:
                /// \cond NOINTERNAL
                static constexpr bool is_policy = true;
                static constexpr bool is_sequenced = true;
                static constexpr bool is_vectorpack = true;

                static constexpr std::size_t num_lanes = N;

                constexpr policy() = default;
#if defined(__NVCC__) || defined(__CUDACC__)
                constexpr ~policy() {}
#endif

                template <typename Executor_, typename Parameters_>
                constexpr policy(Executor_&& exec, Parameters_&& params)
                  : base_type(HPX_FORWARD(Executor_, exec),
                        HPX_FORWARD(Parameters_, params))
                {
                }

                template <typename Executor_, typename Parameters_>
                    requires(!std::is_same_v<policy<Executor_, Parameters_>,
                                 policy> &&
                        std::is_convertible_v<Executor_, Executor> &&
                        std::is_convertible_v<Parameters_, Parameters>)
                explicit constexpr policy(
                    policy<Executor_, Parameters_> const& rhs)
                  : base_type(policy(rhs.executor(), rhs.parameters()))
                {
                }

                template <typename Executor_, typename Parameters_>
                    requires(!std::is_same_v<policy<Executor_, Parameters_>,
                                 policy> &&
                        std::is_convertible_v<Executor_, Executor> &&
                        std::is_convertible_v<Parameters_, Parameters>)
                policy& operator=(policy<Executor_, Parameters_> const& rhs)
                {
                    base_type::operator=(
                        policy(rhs.executor(), rhs.parameters()));
                    return *this;
                }
                /// \endcond
            };
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    /// The class simd_policy is an execution policy type used as a unique type
    /// to disambiguate parallel algorithm overloading and require that a
    /// parallel algorithm's execution may not be parallelized. It should
    /// vectorize the execution of the algorithm using the native number of
    /// vector lanes.
    HPX_CXX_CORE_EXPORT using simd_policy =
        detail::simd_policy_shim<0>::policy<sequenced_executor,
            hpx::traits::executor_parameters_type_t<sequenced_executor>>;

    /// Default sequential vectorizing execution policy object.
    HPX_CXX_CORE_EXPORT inline constexpr simd_policy simd{};

    /// The class fixed_size_simd_policy is an execution policy type used as a
    /// unique type to disambiguate parallel algorithm overloading and require
    /// that a parallel algorithm's execution may not be parallelized. It should
    /// vectorize the execution of the algorithm using the given number of
    /// vector lanes.
    HPX_CXX_CORE_EXPORT template <std::size_t N>
    using fixed_size_simd_policy =
        detail::simd_policy_shim<N>::template policy<sequenced_executor,
            hpx::traits::executor_parameters_type_t<sequenced_executor>>;

    /// Sequential vectorizing execution policy object using the given number of
    /// vector lanes.
    HPX_CXX_CORE_EXPORT template <std::size_t N>
    inline constexpr fixed_size_simd_policy<N> fixed_size_simd{};

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        // The class par_simd_policy is an execution policy type used as a
        // unique type to disambiguate parallel algorithm overloading and
        // indicate that a parallel algorithm's execution may be parallelized.
        HPX_CXX_CORE_EXPORT template <std::size_t N>
        struct par_simd_task_policy_shim
        {
            template <typename Executor, typename Parameters>
            struct policy
              : execution_policy<policy, Executor, Parameters,
                    unsequenced_execution_tag>
              , par_simd_async_policy_mappings<policy<Executor, Parameters>>
            {
            private:
                using base_type = execution_policy<policy, Executor, Parameters,
                    unsequenced_execution_tag>;

            public:
                /// \cond NOINTERNAL
                static constexpr bool is_policy = true;
                static constexpr bool is_parallel = true;
                static constexpr bool is_async = true;
                static constexpr bool is_vectorpack = true;

                static constexpr std::size_t num_lanes = N;

                constexpr policy() = default;
#if defined(__NVCC__) || defined(__CUDACC__)
                constexpr ~policy() {}
#endif

                template <typename Executor_, typename Parameters_>
                constexpr policy(Executor_&& exec, Parameters_&& params)
                  : base_type(HPX_FORWARD(Executor_, exec),
                        HPX_FORWARD(Parameters_, params))
                {
                }

                template <typename Executor_, typename Parameters_>
                    requires(!std::is_same_v<policy<Executor_, Parameters_>,
                                 policy> &&
                        std::is_convertible_v<Executor_, Executor> &&
                        std::is_convertible_v<Parameters_, Parameters>)
                explicit constexpr policy(
                    policy<Executor_, Parameters_> const& rhs)
                  : base_type(policy(rhs.executor(), rhs.parameters()))
                {
                }

                template <typename Executor_, typename Parameters_>
                    requires(!std::is_same_v<policy<Executor_, Parameters_>,
                                 policy> &&
                        std::is_convertible_v<Executor_, Executor> &&
                        std::is_convertible_v<Parameters_, Parameters>)
                policy& operator=(policy<Executor_, Parameters_> const& rhs)
                {
                    base_type::operator=(
                        policy(rhs.executor(), rhs.parameters()));
                    return *this;
                }
                /// \endcond
            };
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    /// Extension: The class par_simd_task_policy is an execution policy type
    /// used as a unique type to disambiguate parallel algorithm overloading and
    /// indicate that a parallel algorithm's execution may be parallelized. It
    /// should vectorize the execution of the algorithm using the native number
    /// of vector lanes.
    ///
    /// The algorithm returns a future representing the result of the
    /// corresponding algorithm when invoked with the parallel_policy.
    HPX_CXX_CORE_EXPORT using par_simd_task_policy =
        detail::par_simd_task_policy_shim<0>::policy<parallel_executor,
            hpx::traits::executor_parameters_type_t<parallel_executor>>;

    /// The class par_fixed_size_simd_task_policy is an execution policy type
    /// used as a unique type to disambiguate parallel algorithm overloading and
    /// indicate that a parallel algorithm's execution may be parallelized. It
    /// should vectorize the execution of the algorithm using the given number
    /// of vector lanes.
    ///
    /// The algorithm returns a future representing the result of the
    /// corresponding algorithm when invoked with the parallel_policy.
    HPX_CXX_CORE_EXPORT template <std::size_t N>
    using par_fixed_size_simd_task_policy =
        detail::par_simd_task_policy_shim<N>::template policy<parallel_executor,
            hpx::traits::executor_parameters_type_t<parallel_executor>>;

    namespace detail {

        // The class par_simd_policy_shim is an execution policy type used as a
        // unique type to disambiguate parallel algorithm overloading and
        // indicate that a parallel algorithm's execution may be parallelized.
        // It should vectorize the execution of the algorithm using the given
        // number of vector lanes N (if N == 0, the native number of vector
        // lanes is used).
        HPX_CXX_CORE_EXPORT template <std::size_t N>
        struct par_simd_policy_shim
        {
            template <typename Executor, typename Parameters>
            struct policy
              : execution_policy<policy, Executor, Parameters,
                    unsequenced_execution_tag>
              , par_simd_sync_policy_mappings<policy<Executor, Parameters>>
            {
            private:
                using base_type = execution_policy<policy, Executor, Parameters,
                    unsequenced_execution_tag>;

            public:
                /// \cond NOINTERNAL
                static constexpr bool is_policy = true;
                static constexpr bool is_parallel = true;
                static constexpr bool is_vectorpack = true;

                static constexpr std::size_t num_lanes = N;

                constexpr policy() = default;
#if defined(__NVCC__) || defined(__CUDACC__)
                constexpr ~policy() {}
#endif

                template <typename Executor_, typename Parameters_>
                constexpr policy(Executor_&& exec, Parameters_&& params)
                  : base_type(HPX_FORWARD(Executor_, exec),
                        HPX_FORWARD(Parameters_, params))
                {
                }

                template <typename Executor_, typename Parameters_>
                    requires(!std::is_same_v<policy<Executor_, Parameters_>,
                                 policy> &&
                        std::is_convertible_v<Executor_, Executor> &&
                        std::is_convertible_v<Parameters_, Parameters>)
                explicit constexpr policy(
                    policy<Executor_, Parameters_> const& rhs)
                  : base_type(policy(rhs.executor(), rhs.parameters()))
                {
                }

                template <typename Executor_, typename Parameters_>
                    requires(!std::is_same_v<policy<Executor_, Parameters_>,
                                 policy> &&
                        std::is_convertible_v<Executor_, Executor> &&
                        std::is_convertible_v<Parameters_, Parameters>)
                policy& operator=(policy<Executor_, Parameters_> const& rhs)
                {
                    base_type::operator=(
                        policy(rhs.executor(), rhs.parameters()));
                    return *this;
                }
                /// \endcond
            };
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    /// Extension: The class par_simd_policy is an execution policy type used as
    /// a unique type to disambiguate parallel algorithm overloading and
    /// indicate that a parallel algorithm's execution may be parallelized. It
    /// should vectorize the execution of the algorithm using the native number
    /// of vector lanes.
    HPX_CXX_CORE_EXPORT using par_simd_policy =
        detail::par_simd_policy_shim<0>::policy<parallel_executor,
            hpx::traits::executor_parameters_type_t<parallel_executor>>;

    /// Default parallel vectorizing execution policy object.
    HPX_CXX_CORE_EXPORT inline constexpr par_simd_policy par_simd{};

    /// Extension: The class par_simd_policy is an execution policy type used as
    /// a unique type to disambiguate parallel algorithm overloading and
    /// indicate that a parallel algorithm's execution may be parallelized. It
    /// should vectorize the execution of the algorithm using the given number
    /// of vector lanes.
    HPX_CXX_CORE_EXPORT template <std::size_t N>
    using par_fixed_size_simd_policy =
        detail::par_simd_policy_shim<N>::template policy<parallel_executor,
            hpx::traits::executor_parameters_type_t<parallel_executor>>;

    /// Parallelizing vectorizing execution policy object using the given number
    /// of vector lanes.
    HPX_CXX_CORE_EXPORT template <std::size_t N>
    inline constexpr par_fixed_size_simd_policy<N> par_fixed_size_simd{};

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        // to_simd() on non-datapar policy shims
        ///////////////////////////////////////////////////////////////////////
        template <typename Executor, typename Parameters>
        constexpr auto sequenced_policy_shim<Executor, Parameters>::to_simd()
            const
        {
            return map_execution_policy<simd_policy>(
                *this, hpx::execution::experimental::to_simd);
        }

        template <typename Executor, typename Parameters>
        constexpr auto
        sequenced_task_policy_shim<Executor, Parameters>::to_simd() const
        {
            return map_execution_policy<simd_task_policy>(
                *this, hpx::execution::experimental::to_simd);
        }

        template <typename Executor, typename Parameters>
        constexpr auto parallel_policy_shim<Executor, Parameters>::to_simd()
            const
        {
            return map_execution_policy<par_simd_policy>(
                *this, hpx::execution::experimental::to_simd);
        }

        template <typename Executor, typename Parameters>
        constexpr auto
        parallel_task_policy_shim<Executor, Parameters>::to_simd() const
        {
            return map_execution_policy<par_simd_task_policy>(
                *this, hpx::execution::experimental::to_simd);
        }
    }    // namespace detail
}    // namespace hpx::execution

#include <hpx/executors/datapar/detail/execution_policy_mapping_members_impl.hpp>

#endif
