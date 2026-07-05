//  Copyright (c) 2026 Hartmut Kaiser
//  Copyright (c) 2026 Sai Charan Arvapally
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/execution/executors/execution_parameters.hpp>
#include <hpx/execution/traits/executor_traits.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/timing.hpp>

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace hpx::execution::experimental {

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_CORE_EXPORT template <bool HasVariableChunkSize = false>
        struct wrapped_params_has_variable_chunk_size
        {
        };

        template <>
        struct wrapped_params_has_variable_chunk_size<true>
        {
            using has_variable_chunk_size = void;
        };

        HPX_CXX_CORE_EXPORT template <bool InvokesTestingFunction = false>
        struct wrapped_params_invokes_testing_function
        {
        };

        template <>
        struct wrapped_params_invokes_testing_function<true>
        {
            using invokes_testing_function = void;
        };

        HPX_CXX_CORE_EXPORT template <typename Wrapped, typename Wrapping>
        struct wrapped_params_properties
          : wrapped_params_has_variable_chunk_size<
                extract_has_variable_chunk_size_v<Wrapped> ||
                extract_has_variable_chunk_size_v<Wrapping>>
          , wrapped_params_invokes_testing_function<
                extract_invokes_testing_function_v<Wrapped> ||
                extract_invokes_testing_function_v<Wrapping>>
        {
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename T, typename U>
        struct propagate_reference_type
        {
            using type = U;
        };

        template <typename T, typename U>
        struct propagate_reference_type<T&, U>
        {
            using type = U&;
        };

        template <typename T, typename U>
        struct propagate_reference_type<T const&, U>
        {
            using type = U const&;
        };

        template <typename T, typename U>
        struct propagate_reference_type<T&&, U>
        {
            using type = U&&;
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename T>
        using wrapping_t = propagate_reference_type<T,
            typename std::decay_t<T>::wrapping_type>::type;

        template <typename T>
        [[nodiscard]] HPX_FORCEINLINE constexpr wrapping_t<T>&&
        wrapping_forward(std::remove_reference_t<T>& t) noexcept
        {
            return static_cast<wrapping_t<T>&&>(t.wrapping);
        }

        template <typename T>
        [[nodiscard]] HPX_FORCEINLINE constexpr wrapping_t<T>&&
        wrapping_forward(std::remove_reference_t<T>&& t) noexcept
        {
            static_assert(!std::is_lvalue_reference_v<T>, "bad forward call");
            return static_cast<wrapping_t<T>&&>(t.wrapping);
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename T>
        using wrapped_t = propagate_reference_type<T,
            typename std::decay_t<T>::wrapped_type>::type;

        template <typename T>
        [[nodiscard]] HPX_FORCEINLINE constexpr wrapped_t<T>&& wrapped_forward(
            std::remove_reference_t<T>& t) noexcept
        {
            return static_cast<wrapped_t<T>&&>(t.wrapped);
        }

        template <typename T>
        [[nodiscard]] HPX_FORCEINLINE constexpr wrapped_t<T>&& wrapped_forward(
            std::remove_reference_t<T>&& t) noexcept
        {
            static_assert(!std::is_lvalue_reference_v<T>, "bad forward call");
            return static_cast<wrapped_t<T>&&>(t.wrapped);
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // Detection of parameter member functions (member-based customization).
    // The "wrapping" variants detect a member that takes another parameters
    // object as its first argument (used to wrap/decorate an inner parameter).

    // wrapping get_chunk_size
    HPX_CXX_CORE_EXPORT template <typename Params, typename Executor>
    inline constexpr bool supports_get_chunk_size_v = requires(
        Params&& p, Executor&& e, hpx::chrono::steady_duration const& d) {
        p.get_chunk_size(e, d, std::size_t{}, std::size_t{});
    };

    HPX_CXX_CORE_EXPORT template <typename Params, typename InnerParams,
        typename Executor>
    inline constexpr bool supports_wrapping_get_chunk_size_v =
        requires(Params&& p, InnerParams&& ip, Executor&& e,
            hpx::chrono::steady_duration const& d) {
            p.get_chunk_size(ip, e, d, std::size_t{}, std::size_t{});
        };

    // wrapping measure_iteration
    HPX_CXX_CORE_EXPORT template <typename Params, typename Executor,
        typename F>
    inline constexpr bool supports_measure_iteration_v =
        requires(Params&& p, Executor&& e, F&& f) {
            p.measure_iteration(e, HPX_FORWARD(F, f), std::size_t{});
        };

    HPX_CXX_CORE_EXPORT template <typename Params, typename InnerParams,
        typename Executor, typename F>
    inline constexpr bool supports_wrapping_measure_iteration_v =
        requires(Params&& p, InnerParams&& ip, Executor&& e, F&& f) {
            p.measure_iteration(ip, e, HPX_FORWARD(F, f), std::size_t{});
        };

    // maximal_number_of_chunks
    HPX_CXX_CORE_EXPORT template <typename Params, typename Executor>
    inline constexpr bool supports_maximal_number_of_chunks_v =
        requires(Params&& p, Executor&& e) {
            p.maximal_number_of_chunks(e, std::size_t{}, std::size_t{});
        };

    HPX_CXX_CORE_EXPORT template <typename Params, typename InnerParams,
        typename Executor>
    inline constexpr bool supports_wrapping_maximal_number_of_chunks_v =
        requires(Params&& p, InnerParams&& ip, Executor&& e) {
            p.maximal_number_of_chunks(ip, e, std::size_t{}, std::size_t{});
        };

    // execution_markers: begin_execution
    HPX_CXX_CORE_EXPORT template <typename Params, typename Executor>
    inline constexpr bool supports_mark_begin_execution_v =
        requires(Params&& p, Executor&& e) { p.mark_begin_execution(e); };

    HPX_CXX_CORE_EXPORT template <typename Params, typename InnerParams,
        typename Executor>
    inline constexpr bool supports_wrapping_mark_begin_execution_v =
        requires(Params&& p, InnerParams&& ip, Executor&& e) {
            p.mark_begin_execution(ip, e);
        };

    // execution_markers: end_of_scheduling
    HPX_CXX_CORE_EXPORT template <typename Params, typename Executor>
    inline constexpr bool supports_mark_end_of_scheduling_v =
        requires(Params&& p, Executor&& e) { p.mark_end_of_scheduling(e); };

    HPX_CXX_CORE_EXPORT template <typename Params, typename InnerParams,
        typename Executor>
    inline constexpr bool supports_wrapping_mark_end_of_scheduling_v =
        requires(Params&& p, InnerParams&& ip, Executor&& e) {
            p.mark_end_of_scheduling(ip, e);
        };

    // execution_markers: end_execution
    HPX_CXX_CORE_EXPORT template <typename Params, typename Executor>
    inline constexpr bool supports_mark_end_execution_v =
        requires(Params&& p, Executor&& e) { p.mark_end_execution(e); };

    HPX_CXX_CORE_EXPORT template <typename Params, typename InnerParams,
        typename Executor>
    inline constexpr bool supports_wrapping_mark_end_execution_v =
        requires(Params&& p, InnerParams&& ip, Executor&& e) {
            p.mark_end_execution(ip, e);
        };

    // execution_markers: partition
    HPX_CXX_CORE_EXPORT template <typename Params, typename Executor,
        typename... Args>
    inline constexpr bool supports_mark_partition_v =
        requires(Params&& p, Executor&& e, Args&&... args) {
            p.mark_partition(e, std::size_t{}, HPX_FORWARD(Args, args)...);
        };

    HPX_CXX_CORE_EXPORT template <typename Params, typename InnerParams,
        typename Executor, typename... Args>
    inline constexpr bool supports_wrapping_mark_partition_v =
        requires(Params&& p, InnerParams&& ip, Executor&& e, Args&&... args) {
            p.mark_partition(ip, e, std::size_t{}, HPX_FORWARD(Args, args)...);
        };

    // wrapping processing_units_count
    HPX_CXX_CORE_EXPORT template <typename Params, typename Executor>
    inline constexpr bool supports_processing_units_count_v = requires(
        Params&& p, Executor&& e, hpx::chrono::steady_duration const& d) {
        p.processing_units_count(e, d, std::size_t{});
    };

    HPX_CXX_CORE_EXPORT template <typename Params, typename InnerParams,
        typename Executor>
    inline constexpr bool supports_wrapping_processing_units_count_v =
        requires(Params&& p, InnerParams&& ip, Executor&& e,
            hpx::chrono::steady_duration const& d) {
            p.processing_units_count(ip, e, d, std::size_t{});
        };

    // wrapping reset_thread_distribution
    HPX_CXX_CORE_EXPORT template <typename Params, typename Executor>
    inline constexpr bool supports_reset_thread_distribution_v =
        requires(Params&& p, Executor&& e) { p.reset_thread_distribution(e); };

    HPX_CXX_CORE_EXPORT template <typename Params, typename InnerParams,
        typename Executor>
    inline constexpr bool supports_wrapping_reset_thread_distribution_v =
        requires(Params&& p, InnerParams&& ip, Executor&& e) {
            p.reset_thread_distribution(ip, e);
        };

    // wrapping collect_execution_parameters
    HPX_CXX_CORE_EXPORT template <typename Params, typename Executor>
    inline constexpr bool supports_collect_execution_parameters_v =
        requires(Params&& p, Executor&& e) {
            p.collect_execution_parameters(
                e, std::size_t{}, std::size_t{}, std::size_t{}, std::size_t{});
        };

    HPX_CXX_CORE_EXPORT template <typename Params, typename InnerParams,
        typename Executor>
    inline constexpr bool supports_wrapping_collect_execution_parameters_v =
        requires(Params&& p, InnerParams&& ip, Executor&& e) {
            p.collect_execution_parameters(ip, e, std::size_t{}, std::size_t{},
                std::size_t{}, std::size_t{});
        };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Wrapped, typename Wrapping>
        requires(hpx::executor_parameters<Wrapped> &&
            hpx::executor_parameters<Wrapping>)
    struct wrapped_params : detail::wrapped_params_properties<Wrapped, Wrapping>
    {
        using wrapped_type = Wrapped;
        using wrapping_type = Wrapping;

        template <typename Wrapped_, typename Wrapping_>
        constexpr wrapped_params(Wrapped_&& wrapped, Wrapping_&& wrapping)
          : wrapped(HPX_FORWARD(Wrapped_, wrapped))
          , wrapping(HPX_FORWARD(Wrapping_, wrapping))
        {
        }

        // wrapping get_chunk_size
        template <typename Executor>
        constexpr std::size_t get_chunk_size(Executor&& exec,
            hpx::chrono::steady_duration const& t, std::size_t const cores,
            std::size_t const num_iterations) const
        {
            if constexpr (supports_wrapping_get_chunk_size_v<Wrapping, Wrapped,
                              Executor>)
            {
                return wrapping.get_chunk_size(wrapped,
                    HPX_FORWARD(Executor, exec), t, cores, num_iterations);
            }
            else if constexpr (supports_get_chunk_size_v<Wrapping, Executor>)
            {
                return wrapping.get_chunk_size(
                    HPX_FORWARD(Executor, exec), t, cores, num_iterations);
            }
            else if constexpr (supports_get_chunk_size_v<Wrapped, Executor>)
            {
                return wrapped.get_chunk_size(
                    HPX_FORWARD(Executor, exec), t, cores, num_iterations);
            }
            else
            {
                return detail::get_chunk_size_property::get_chunk_size(
                    HPX_FORWARD(Executor, exec), t, cores, num_iterations);
            }
        }

        // wrapping measure_iteration
        template <typename Executor, typename F>
        constexpr hpx::chrono::steady_duration measure_iteration(
            Executor&& exec, F&& f, std::size_t const num_tasks)
        {
            if constexpr (supports_wrapping_measure_iteration_v<Wrapping,
                              Wrapped, Executor, F>)
            {
                return wrapping.measure_iteration(wrapped,
                    HPX_FORWARD(Executor, exec), HPX_FORWARD(F, f), num_tasks);
            }
            else if constexpr (supports_measure_iteration_v<Wrapping, Executor,
                                   F>)
            {
                return wrapping.measure_iteration(
                    HPX_FORWARD(Executor, exec), HPX_FORWARD(F, f), num_tasks);
            }
            else if constexpr (supports_measure_iteration_v<Wrapped, Executor,
                                   F>)
            {
                return wrapped.measure_iteration(
                    HPX_FORWARD(Executor, exec), HPX_FORWARD(F, f), num_tasks);
            }
            else
            {
                return detail::measure_iteration_property::measure_iteration(
                    HPX_FORWARD(Executor, exec), HPX_FORWARD(F, f), num_tasks);
            }
        }

        // maximal_number_of_chunks
        template <typename Executor>
        constexpr std::size_t maximal_number_of_chunks(Executor&& exec,
            std::size_t const cores, std::size_t const num_tasks) const
        {
            if constexpr (supports_wrapping_maximal_number_of_chunks_v<Wrapping,
                              Wrapped, Executor>)
            {
                return wrapping.maximal_number_of_chunks(
                    wrapped, HPX_FORWARD(Executor, exec), cores, num_tasks);
            }
            else if constexpr (supports_maximal_number_of_chunks_v<Wrapping,
                                   Executor>)
            {
                return wrapping.maximal_number_of_chunks(
                    HPX_FORWARD(Executor, exec), cores, num_tasks);
            }
            else if constexpr (supports_maximal_number_of_chunks_v<Wrapped,
                                   Executor>)
            {
                return wrapped.maximal_number_of_chunks(
                    HPX_FORWARD(Executor, exec), cores, num_tasks);
            }
            else
            {
                return detail::maximal_number_of_chunks_property::
                    maximal_number_of_chunks(
                        HPX_FORWARD(Executor, exec), cores, num_tasks);
            }
        }

        // execution_markers: begin_execution
        template <typename Executor>
        constexpr void mark_begin_execution(Executor&& exec) const
        {
            if constexpr (supports_wrapping_mark_begin_execution_v<Wrapping,
                              Wrapped, Executor>)
            {
                wrapping.mark_begin_execution(
                    wrapped, HPX_FORWARD(Executor, exec));
            }
            else if constexpr (supports_mark_begin_execution_v<Wrapping,
                                   Executor>)
            {
                wrapping.mark_begin_execution(HPX_FORWARD(Executor, exec));
            }
            else if constexpr (supports_mark_begin_execution_v<Wrapped,
                                   Executor>)
            {
                wrapped.mark_begin_execution(HPX_FORWARD(Executor, exec));
            }
        }

        // execution_markers: end_of_scheduling
        template <typename Executor>
        constexpr void mark_end_of_scheduling(Executor&& exec) const
        {
            if constexpr (supports_wrapping_mark_end_of_scheduling_v<Wrapping,
                              Wrapped, Executor>)
            {
                wrapping.mark_end_of_scheduling(
                    wrapped, HPX_FORWARD(Executor, exec));
            }
            else if constexpr (supports_mark_end_of_scheduling_v<Wrapping,
                                   Executor>)
            {
                wrapping.mark_end_of_scheduling(HPX_FORWARD(Executor, exec));
            }
            else if constexpr (supports_mark_end_of_scheduling_v<Wrapped,
                                   Executor>)
            {
                wrapped.mark_end_of_scheduling(HPX_FORWARD(Executor, exec));
            }
        }

        // execution_markers: end_execution
        template <typename Executor>
        constexpr void mark_end_execution(Executor&& exec) const
        {
            if constexpr (supports_wrapping_mark_end_execution_v<Wrapping,
                              Wrapped, Executor>)
            {
                wrapping.mark_end_execution(
                    wrapped, HPX_FORWARD(Executor, exec));
            }
            else if constexpr (supports_mark_end_execution_v<Wrapping,
                                   Executor>)
            {
                wrapping.mark_end_execution(HPX_FORWARD(Executor, exec));
            }
            else if constexpr (supports_mark_end_execution_v<Wrapped, Executor>)
            {
                wrapped.mark_end_execution(HPX_FORWARD(Executor, exec));
            }
        }

        // execution_markers: partition
        template <typename Executor, typename... Args>
        constexpr void mark_partition(
            Executor&& exec, std::size_t partition, Args&&... args) const
        {
            if constexpr (supports_wrapping_mark_partition_v<Wrapping, Wrapped,
                              Executor, Args...>)
            {
                wrapping.mark_partition(wrapped, HPX_FORWARD(Executor, exec),
                    partition, HPX_FORWARD(Args, args)...);
            }
            else if constexpr (supports_mark_partition_v<Wrapping, Executor,
                                   Args...>)
            {
                wrapping.mark_partition(HPX_FORWARD(Executor, exec), partition,
                    HPX_FORWARD(Args, args)...);
            }
            else if constexpr (supports_mark_partition_v<Wrapped, Executor,
                                   Args...>)
            {
                wrapped.mark_partition(HPX_FORWARD(Executor, exec), partition,
                    HPX_FORWARD(Args, args)...);
            }
        }

        // wrapping processing_units_count
        template <typename Executor>
        constexpr std::size_t processing_units_count(Executor&& exec,
            hpx::chrono::steady_duration const& iteration_duration,
            std::size_t const num_tasks) const
        {
            if constexpr (supports_wrapping_processing_units_count_v<Wrapping,
                              Wrapped, Executor>)
            {
                return wrapping.processing_units_count(wrapped,
                    HPX_FORWARD(Executor, exec), iteration_duration, num_tasks);
            }
            else if constexpr (supports_processing_units_count_v<Wrapping,
                                   Executor>)
            {
                return wrapping.processing_units_count(
                    HPX_FORWARD(Executor, exec), iteration_duration, num_tasks);
            }
            else if constexpr (supports_processing_units_count_v<Wrapped,
                                   Executor>)
            {
                return wrapped.processing_units_count(
                    HPX_FORWARD(Executor, exec), iteration_duration, num_tasks);
            }
            else
            {
                // Default implementation when neither has processing_units_count
                return hpx::execution::experimental::processing_units_count(
                    exec, iteration_duration, num_tasks);
            }
        }

        // wrapping reset_thread_distribution
        template <typename Executor>
        constexpr void reset_thread_distribution(Executor&& exec) const
        {
            if constexpr (supports_wrapping_reset_thread_distribution_v<
                              Wrapping, Wrapped, Executor>)
            {
                wrapping.reset_thread_distribution(
                    wrapped, HPX_FORWARD(Executor, exec));
            }
            else if constexpr (supports_reset_thread_distribution_v<Wrapping,
                                   Executor>)
            {
                wrapping.reset_thread_distribution(HPX_FORWARD(Executor, exec));
            }
            else if constexpr (supports_reset_thread_distribution_v<Wrapped,
                                   Executor>)
            {
                wrapped.reset_thread_distribution(HPX_FORWARD(Executor, exec));
            }
        }

        // wrapping collect_execution_parameters
        template <typename Executor>
        constexpr void collect_execution_parameters(Executor&& exec,
            std::size_t const num_elements, std::size_t const num_cores,
            std::size_t const num_chunks, std::size_t const chunk_size) const
        {
            if constexpr (supports_wrapping_collect_execution_parameters_v<
                              Wrapping, Wrapped, Executor>)
            {
                wrapping.collect_execution_parameters(wrapped,
                    HPX_FORWARD(Executor, exec), num_elements, num_cores,
                    num_chunks, chunk_size);
            }
            else if constexpr (supports_collect_execution_parameters_v<Wrapping,
                                   Executor>)
            {
                wrapping.collect_execution_parameters(
                    HPX_FORWARD(Executor, exec), num_elements, num_cores,
                    num_chunks, chunk_size);
            }
            else if constexpr (supports_collect_execution_parameters_v<Wrapped,
                                   Executor>)
            {
                wrapped.collect_execution_parameters(
                    HPX_FORWARD(Executor, exec), num_elements, num_cores,
                    num_chunks, chunk_size);
            }
        }

        Wrapped wrapped;
        Wrapping wrapping;
    };

    template <typename Wrapped, typename Wrapping>
    struct is_executor_parameters<wrapped_params<Wrapped, Wrapping>>
      : std::true_type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename OrgParam, typename Param>
        requires(hpx::executor_parameters<OrgParam> &&
            hpx::executor_parameters<Param>)
    auto rebind_executor_parameters(OrgParam&& org_param, Param&& to_replace)
    {
        return wrapped_params<std::decay_t<OrgParam>, std::decay_t<Param>>(
            HPX_FORWARD(OrgParam, org_param), HPX_FORWARD(Param, to_replace));
    }
}    // namespace hpx::execution::experimental
