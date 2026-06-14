//  Copyright (c) 2016-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/async_base/query_dispatch.hpp>
#include <hpx/execution/traits/executor_traits.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {

    ///////////////////////////////////////////////////////////////////////////
    // Executor information customization points
    namespace detail {

        /// \cond NOINTERNAL
        template <typename Parameters, typename Executor,
            typename Enable = void>
        struct get_chunk_size_fn_helper;

        template <typename Parameters, typename Executor,
            typename Enable = void>
        struct measure_iteration_fn_helper;

        template <typename Parameters, typename Executor,
            typename Enable = void>
        struct maximal_number_of_chunks_fn_helper;

        template <typename Parameters, typename Executor,
            typename Enable = void>
        struct reset_thread_distribution_fn_helper;

        template <typename Parameters, typename Executor,
            typename Enable = void>
        struct processing_units_count_fn_helper;

        template <typename Parameters, typename Executor,
            typename Enable = void>
        struct mark_begin_execution_fn_helper;

        template <typename Parameters, typename Executor,
            typename Enable = void>
        struct mark_end_of_scheduling_fn_helper;

        template <typename Parameters, typename Executor,
            typename Enable = void>
        struct mark_end_execution_fn_helper;

        template <typename Parameters, typename Executor,
            typename Enable = void>
        struct mark_partition_fn_helper;

        template <typename Parameters, typename Executor,
            typename Enable = void>
        struct collect_execution_parameters_fn_helper;
        /// \endcond

        template <typename CPO, typename ExPolicy, typename... Ts>
            requires(hpx::is_execution_policy_v<std::decay_t<ExPolicy>>)
        HPX_FORCEINLINE decltype(auto) forward_to_policy_executor(
            CPO const& cpo, ExPolicy&& policy, Ts&&... ts)
        {
            return cpo(HPX_FORWARD(ExPolicy, policy).executor(),
                HPX_FORWARD(Ts, ts)...);
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // define customization points
    HPX_CXX_CORE_EXPORT inline constexpr struct null_parameters_t
    {
    } null_parameters{};

    /// \cond NOINTERNAL
    template <>
    struct is_executor_parameters<null_parameters_t> : std::true_type
    {
    };
    /// \endcond

    /// Return the number of invocations of the given function \a f which should
    /// be combined into a single task
    ///
    /// \param params   [in] The executor parameters object to use for
    ///                 determining the chunk size for the given number of tasks
    ///                 \a num_tasks.
    /// \param exec     [in] The executor object which will be used
    ///                 for scheduling of the loop iterations.
    /// \param iteration_duration [in] The time one of the tasks require to be
    ///                 executed.
    /// \param cores    [in] The number of cores the number of chunks
    ///                 should be determined for.
    /// \param num_tasks [in] The number of tasks the chunk size should be
    ///                 determined for
    ///
    /// \return         The size of the chunks (number of iterations per chunk)
    ///                 that should be used for parallel execution.
    ///
    HPX_CXX_CORE_EXPORT inline constexpr struct get_chunk_size_t final
    {
        template <typename Parameters, typename Executor>
            requires(hpx::executor_parameters<Parameters> &&
                hpx::executor_any<Executor>)
        HPX_FORCEINLINE decltype(auto) operator()(Parameters&& params,
            Executor&& exec,
            hpx::chrono::steady_duration const& iteration_duration,
            std::size_t cores, std::size_t num_tasks) const
        {
            return detail::get_chunk_size_fn_helper<
                hpx::util::decay_unwrap_t<Parameters>,
                std::decay_t<Executor>>::call(HPX_FORWARD(Parameters, params),
                HPX_FORWARD(Executor, exec), iteration_duration, cores,
                num_tasks);
        }

        template <typename Parameters, typename Executor>
            requires(hpx::executor_parameters<Parameters> &&
                hpx::executor_any<Executor>)
        HPX_FORCEINLINE decltype(auto) operator()(Parameters&& params,
            Executor&& exec, std::size_t cores, std::size_t num_tasks) const
        {
            return (*this)(HPX_FORWARD(Parameters, params),
                HPX_FORWARD(Executor, exec), hpx::chrono::null_duration, cores,
                num_tasks);
        }
    } get_chunk_size{};

    /// Return the measured execution time for one iteration based on running
    /// the given function.
    ///
    /// \param params   [in] The executor parameters object to use for
    ///                 determining the chunk size for the given number of tasks
    ///                 \a num_tasks.
    /// \param exec     [in] The executor object which will be used
    ///                 for scheduling of the loop iterations.
    /// \param f        [in] The function which will be optionally scheduled
    ///                 using the given executor.
    /// \param num_tasks [in] The number of tasks the chunk size should be
    ///                 determined for
    ///
    /// \note  The parameter \a f is expected to be a nullary function
    ///        returning a `std::size_t` representing the number of iteration
    ///        the function has already executed (i.e. which don't have to be
    ///        scheduled anymore).
    ///
    /// \return The execution time for one of the tasks.
    ///
    HPX_CXX_CORE_EXPORT inline constexpr struct measure_iteration_t final
    {
        template <typename Parameters, typename Executor, typename F>
            requires(hpx::executor_parameters<Parameters> &&
                hpx::executor_any<Executor>)
        HPX_FORCEINLINE decltype(auto) operator()(Parameters&& params,
            Executor&& exec, F&& f, std::size_t num_tasks) const
        {
            return detail::measure_iteration_fn_helper<
                hpx::util::decay_unwrap_t<Parameters>,
                std::decay_t<Executor>>::call(HPX_FORWARD(Parameters, params),
                HPX_FORWARD(Executor, exec), HPX_FORWARD(F, f), num_tasks);
        }
    } measure_iteration{};

    /// Return the largest reasonable number of chunks to create for a
    /// single algorithm invocation.
    ///
    /// \param params   [in] The executor parameters object to use for
    ///                 determining the number of chunks for the given
    ///                 number of \a cores.
    /// \param exec     [in] The executor object which will be used
    ///                 for scheduling of the loop iterations.
    /// \param cores    [in] The number of cores the number of chunks
    ///                 should be determined for.
    /// \param num_tasks [in] The number of tasks the chunk size should be
    ///                 determined for
    ///
    HPX_CXX_CORE_EXPORT inline constexpr struct maximal_number_of_chunks_t final
    {
        template <typename Parameters, typename Executor>
            requires(hpx::executor_parameters<Parameters> &&
                hpx::executor_any<Executor>)
        HPX_FORCEINLINE decltype(auto) operator()(Parameters&& params,
            Executor&& exec, std::size_t cores, std::size_t num_tasks) const
        {
            return detail::maximal_number_of_chunks_fn_helper<
                hpx::util::decay_unwrap_t<Parameters>,
                std::decay_t<Executor>>::call(HPX_FORWARD(Parameters, params),
                HPX_FORWARD(Executor, exec), cores, num_tasks);
        }
    } maximal_number_of_chunks{};

    /// Reset the internal round robin thread distribution scheme for the
    /// given executor.
    ///
    /// \param params   [in] The executor parameters object to use for
    ///                 resetting the thread distribution scheme.
    /// \param exec     [in] The executor object to use.
    ///
    /// \note This calls params.reset_thread_distribution(exec) if it exists;
    ///       otherwise it does nothing.
    ///
    HPX_CXX_CORE_EXPORT inline constexpr struct
        reset_thread_distribution_t final
    {
        template <typename Parameters, typename Executor>
            requires(hpx::executor_parameters<Parameters> &&
                hpx::executor_any<Executor>)
        HPX_FORCEINLINE decltype(auto) operator()(
            Parameters&& params, Executor&& exec) const
        {
            return detail::reset_thread_distribution_fn_helper<
                hpx::util::decay_unwrap_t<Parameters>,
                std::decay_t<Executor>>::call(HPX_FORWARD(Parameters, params),
                HPX_FORWARD(Executor, exec));
        }
    } reset_thread_distribution{};

    /// Retrieve the number of (kernel-)threads used by the associated executor.
    ///
    /// \param params [in] The executor parameters object to use as a
    ///              fallback if the executor does not expose
    /// \param exec   [in] The executor object which will be used
    ///               for scheduling of the loop iterations.
    /// \param iteration_duration [in] The time one of the tasks require to be
    ///                 executed.
    /// \param num_tasks [in] The number of tasks the number of cores should be
    ///                 determined for
    ///
    /// \note This calls params.processing_units_count(Executor&&) if it exists;
    ///       otherwise it forwards the request to the executor parameters
    ///       object.
    ///
    /// \return The number of cores to use
    ///
    HPX_CXX_CORE_EXPORT inline constexpr struct processing_units_count_t final
    {
        // Primary: executor has .query() method
        template <typename Parameters, typename Executor>
            requires(hpx::executor_parameters<Parameters> &&
                has_query_v<Executor, processing_units_count_t, Parameters,
                    hpx::chrono::steady_duration const&, std::size_t>)
        HPX_FORCEINLINE decltype(auto) operator()(Parameters&& params,
            Executor&& exec,
            hpx::chrono::steady_duration const& iteration_duration,
            std::size_t num_tasks) const
        {
            return HPX_FORWARD(Executor, exec)
                .query(*this, HPX_FORWARD(Parameters, params),
                    iteration_duration, num_tasks);
        }

        template <typename Parameters, typename Executor>
            requires(hpx::executor_parameters<Parameters> &&
                has_query_v<Executor, processing_units_count_t, Parameters,
                    hpx::chrono::steady_duration const&, std::size_t>)
        HPX_FORCEINLINE decltype(auto) operator()(
            Parameters&& params, Executor&& exec, std::size_t num_tasks) const
        {
            return HPX_FORWARD(Executor, exec)
                .query(*this, HPX_FORWARD(Parameters, params),
                    hpx::chrono::null_duration, num_tasks);
        }

        template <typename Executor>
            requires(has_query_v<Executor, processing_units_count_t,
                null_parameters_t, hpx::chrono::steady_duration const&,
                std::size_t>)
        HPX_FORCEINLINE decltype(auto) operator()(Executor&& exec,
            hpx::chrono::steady_duration const& iteration_duration,
            std::size_t num_tasks) const
        {
            return HPX_FORWARD(Executor, exec)
                .query(*this, null_parameters, iteration_duration, num_tasks);
        }

        template <typename Executor>
            requires(has_query_v<Executor, processing_units_count_t,
                null_parameters_t, hpx::chrono::steady_duration const&,
                std::size_t>)
        HPX_FORCEINLINE decltype(auto) operator()(
            Executor&& exec, std::size_t num_tasks) const
        {
            return HPX_FORWARD(Executor, exec)
                .query(*this, null_parameters, hpx::chrono::null_duration,
                    num_tasks);
        }

        template <typename Parameters, typename Executor>
            requires(hpx::executor_parameters<Parameters> &&
                has_query_v<Executor, processing_units_count_t, Parameters,
                    hpx::chrono::steady_duration const&, std::size_t>)
        HPX_FORCEINLINE decltype(auto) operator()(
            Parameters&& params, Executor&& exec) const
        {
            return (*this)(HPX_FORWARD(Parameters, params),
                HPX_FORWARD(Executor, exec), std::size_t{0});
        }

        template <typename Executor>
            requires(has_query_v<Executor, processing_units_count_t,
                null_parameters_t, hpx::chrono::steady_duration const&,
                std::size_t>)
        HPX_FORCEINLINE decltype(auto) operator()(Executor&& exec) const
        {
            return (*this)(HPX_FORWARD(Executor, exec), std::size_t{0});
        }

        // Fallback: use fn_helper
        template <typename Parameters, typename Executor>
            requires(hpx::executor_parameters<Parameters> &&
                hpx::executor_any<Executor> &&
                !has_query_v<Executor, processing_units_count_t, Parameters,
                    hpx::chrono::steady_duration const&, std::size_t>)
        HPX_FORCEINLINE decltype(auto) operator()(Parameters&& params,
            Executor&& exec,
            hpx::chrono::steady_duration const& iteration_duration,
            std::size_t num_tasks) const
        {
            return detail::processing_units_count_fn_helper<
                hpx::util::decay_unwrap_t<Parameters>,
                std::decay_t<Executor>>::call(HPX_FORWARD(Parameters, params),
                HPX_FORWARD(Executor, exec), iteration_duration, num_tasks);
        }

        template <typename Parameters, typename Executor>
            requires(hpx::executor_parameters<Parameters> &&
                hpx::executor_any<Executor> &&
                !has_query_v<Executor, processing_units_count_t, Parameters,
                    hpx::chrono::steady_duration const&, std::size_t>)
        HPX_FORCEINLINE decltype(auto) operator()(Parameters&& params,
            Executor&& exec, std::size_t num_tasks = 0) const
        {
            return (*this)(HPX_FORWARD(Parameters, params),
                HPX_FORWARD(Executor, exec), hpx::chrono::null_duration,
                num_tasks);
        }

        template <typename Executor>
            requires(hpx::executor_any<Executor> &&
                !hpx::is_execution_policy_v<std::decay_t<Executor>> &&
                !has_query_v<Executor, processing_units_count_t,
                    null_parameters_t, hpx::chrono::steady_duration const&,
                    std::size_t>)
        HPX_FORCEINLINE decltype(auto) operator()(Executor&& exec,
            hpx::chrono::steady_duration const& iteration_duration,
            std::size_t num_tasks) const
        {
            return detail::processing_units_count_fn_helper<null_parameters_t,
                std::decay_t<Executor>>::call(null_parameters,
                HPX_FORWARD(Executor, exec), iteration_duration, num_tasks);
        }

        template <typename Executor>
            requires(hpx::executor_any<Executor> &&
                !hpx::is_execution_policy_v<std::decay_t<Executor>> &&
                !has_query_v<Executor, processing_units_count_t,
                    null_parameters_t, hpx::chrono::steady_duration const&,
                    std::size_t>)
        HPX_FORCEINLINE decltype(auto) operator()(
            Executor&& exec, std::size_t num_tasks = 0) const
        {
            return (*this)(null_parameters, HPX_FORWARD(Executor, exec),
                hpx::chrono::null_duration, num_tasks);
        }

        template <typename ExPolicy>
            requires(hpx::is_execution_policy_v<std::decay_t<ExPolicy>>)
        HPX_FORCEINLINE decltype(auto) operator()(ExPolicy&& policy) const
        {
            return detail::forward_to_policy_executor(
                *this, HPX_FORWARD(ExPolicy, policy));
        }
    } processing_units_count{};

    /// Generate a policy that supports setting the number of cores for
    /// execution.
    HPX_CXX_CORE_EXPORT inline constexpr struct
        with_processing_units_count_t final
    {
        template <typename Executor>
            requires(has_query_v<Executor, with_processing_units_count_t,
                std::size_t>)
        decltype(auto) operator()(Executor&& exec, std::size_t num_cores) const
        {
            return HPX_FORWARD(Executor, exec).query(*this, num_cores);
        }

        template <typename Policy, typename Property>
            requires(hpx::is_execution_policy_v<std::decay_t<Policy>> &&
                has_query_v<Policy, with_processing_units_count_t, Property>)
        decltype(auto) operator()(Policy&& policy, Property&& property) const
        {
            return HPX_FORWARD(Policy, policy)
                .query(*this, HPX_FORWARD(Property, property));
        }
    } with_processing_units_count{};

    /// Mark the begin of a parallel algorithm execution
    ///
    /// \param params [in] The executor parameters object to use as a
    ///               fallback if the executor does not expose
    /// \param exec   [in] The executor object which will be used
    ///               for scheduling of the loop iterations.
    ///
    /// \note This calls params.mark_begin_execution(exec) if it exists;
    ///       otherwise it does nothing.
    ///
    HPX_CXX_CORE_EXPORT inline constexpr struct mark_begin_execution_t final
    {
        template <typename Parameters, typename Executor>
            requires(hpx::executor_parameters<Parameters> &&
                hpx::executor_any<Executor>)
        HPX_FORCEINLINE decltype(auto) operator()(
            Parameters&& params, Executor&& exec) const
        {
            return detail::mark_begin_execution_fn_helper<
                hpx::util::decay_unwrap_t<Parameters>,
                std::decay_t<Executor>>::call(HPX_FORWARD(Parameters, params),
                HPX_FORWARD(Executor, exec));
        }
    } mark_begin_execution{};

    /// Mark the end of scheduling tasks during parallel algorithm execution
    ///
    /// \param params [in] The executor parameters object to use as a
    ///               fallback if the executor does not expose
    /// \param exec   [in] The executor object which will be used
    ///               for scheduling of the loop iterations.
    ///
    /// \note This calls params.mark_begin_execution(exec) if it exists;
    ///       otherwise it does nothing.
    ///
    HPX_CXX_CORE_EXPORT inline constexpr struct mark_end_of_scheduling_t final
    {
        template <typename Parameters, typename Executor>
            requires(hpx::executor_parameters<Parameters> &&
                hpx::executor_any<Executor>)
        HPX_FORCEINLINE decltype(auto) operator()(
            Parameters&& params, Executor&& exec) const
        {
            return detail::mark_end_of_scheduling_fn_helper<
                hpx::util::decay_unwrap_t<Parameters>,
                std::decay_t<Executor>>::call(HPX_FORWARD(Parameters, params),
                HPX_FORWARD(Executor, exec));
        }
    } mark_end_of_scheduling{};

    /// Mark the end of a parallel algorithm execution
    ///
    /// \param params [in] The executor parameters object to use as a
    ///              fallback if the executor does not expose
    /// \param exec   [in] The executor object which will be used
    ///               for scheduling of the loop iterations.
    ///
    /// \note This calls params.mark_end_execution(exec) if it exists;
    ///       otherwise it does nothing.
    ///
    HPX_CXX_CORE_EXPORT inline constexpr struct mark_end_execution_t final
    {
        template <typename Parameters, typename Executor>
            requires(hpx::executor_parameters<Parameters> &&
                hpx::executor_any<Executor>)
        HPX_FORCEINLINE decltype(auto) operator()(
            Parameters&& params, Executor&& exec) const
        {
            return detail::mark_end_execution_fn_helper<
                hpx::util::decay_unwrap_t<Parameters>,
                std::decay_t<Executor>>::call(HPX_FORWARD(Parameters, params),
                HPX_FORWARD(Executor, exec));
        }
    } mark_end_execution{};

    /// Mark custom point in parallel algorithm execution
    ///
    /// \param params [in] The executor parameters object to use as a
    ///              fallback if the executor does not expose
    /// \param exec   [in] The executor object which will be used
    ///               for scheduling of the loop iterations.
    ///
    /// \note This calls params.mark_partition(exec, partition, args...) if it exists;
    ///       otherwise it does nothing.
    ///
    HPX_CXX_CORE_EXPORT inline constexpr struct mark_partition_t final
    {
        template <typename Parameters, typename Executor, typename... Args>
            requires(hpx::executor_parameters<Parameters> &&
                hpx::executor_any<Executor>)
        HPX_FORCEINLINE decltype(auto) operator()(Parameters&& params,
            Executor&& exec, std::size_t partition, Args&&... args) const
        {
            return detail::mark_partition_fn_helper<
                hpx::util::decay_unwrap_t<Parameters>,
                std::decay_t<Executor>>::call(HPX_FORWARD(Parameters, params),
                HPX_FORWARD(Executor, exec), partition,
                HPX_FORWARD(Args, args)...);
        }
    } mark_partition{};

    /// Collect various parameters of the chunking for this parallel algorithm
    /// execution
    ///
    /// \param params [in] The executor parameters object to use as a
    ///              fallback if the executor does not expose
    /// \param exec   [in] The executor object which will be used
    ///               for scheduling of the loop iterations.
    /// \param num_elements [in] The overall number of elements for the
    ///                     algorithm.
    /// \param num_cores    [in] The overall number of cores to utilize
    ///                     for the algorithm.
    /// \param num_chunks   [in] The overall number of chunks for the
    ///                     algorithm.
    /// \param chunk_size   [in] The size of the chunks created for the
    ///                     algorithm.
    ///
    /// \note This calls params.mark_begin_execution(exec) if it exists;
    ///       otherwise it does nothing.
    ///
    HPX_CXX_CORE_EXPORT inline constexpr struct
        collect_execution_parameters_t final
    {
        template <typename Parameters, typename Executor>
            requires(hpx::executor_parameters<Parameters> &&
                hpx::executor_any<Executor>)
        HPX_FORCEINLINE decltype(auto) operator()(Parameters&& params,
            Executor&& exec, std::size_t num_elements, std::size_t num_cores,
            std::size_t num_chunks, std::size_t chunk_size) const
        {
            return detail::collect_execution_parameters_fn_helper<
                hpx::util::decay_unwrap_t<Parameters>,
                std::decay_t<Executor>>::call(HPX_FORWARD(Parameters, params),
                HPX_FORWARD(Executor, exec), num_elements, num_cores,
                num_chunks, chunk_size);
        }
    } collect_execution_parameters{};

    template <>
    struct is_scheduling_property<with_processing_units_count_t>
      : std::true_type
    {
    };
}    // namespace hpx::execution::experimental
