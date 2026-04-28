//  Copyright (c) 2026 Hartmut Kaiser
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
    // wrapping get_chunk_size
    HPX_CXX_CORE_EXPORT template <typename Params, typename Executor>
    inline constexpr bool supports_get_chunk_size_v =
        hpx::functional::detail::is_tag_override_invocable_v<get_chunk_size_t,
            Params&&, Executor&&, hpx::chrono::steady_duration const&,
            std::size_t, std::size_t>;

    HPX_CXX_CORE_EXPORT template <typename Params, typename InnerParams,
        typename Executor>
    inline constexpr bool supports_wrapping_get_chunk_size_v =
        hpx::functional::detail::is_tag_override_invocable_v<get_chunk_size_t,
            Params&&, InnerParams&&, Executor&&,
            hpx::chrono::steady_duration const&, std::size_t, std::size_t>;

    // wrapping measure_iteration
    HPX_CXX_CORE_EXPORT template <typename Params, typename Executor,
        typename F>
    inline constexpr bool supports_measure_iteration_v =
        hpx::functional::detail::is_tag_override_invocable_v<
            measure_iteration_t, Params&&, Executor&&, F&&, std::size_t>;

    HPX_CXX_CORE_EXPORT template <typename Params, typename InnerParams,
        typename Executor, typename F>
    inline constexpr bool supports_wrapping_measure_iteration_v =
        hpx::functional::detail::is_tag_override_invocable_v<
            measure_iteration_t, Params&&, InnerParams&&, Executor&&, F&&,
            std::size_t>;

    // maximal_number_of_chunks
    HPX_CXX_CORE_EXPORT template <typename Params, typename Executor>
    inline constexpr bool supports_maximal_number_of_chunks_v =
        hpx::functional::detail::is_tag_override_invocable_v<
            maximal_number_of_chunks_t, Params&&, Executor&&, std::size_t,
            std::size_t>;

    HPX_CXX_CORE_EXPORT template <typename Params, typename InnerParams,
        typename Executor>
    inline constexpr bool supports_wrapping_maximal_number_of_chunks_v =
        hpx::functional::detail::is_tag_override_invocable_v<
            maximal_number_of_chunks_t, Params&&, InnerParams&&, Executor&&,
            std::size_t, std::size_t>;

    // execution_markers: begin_execution
    HPX_CXX_CORE_EXPORT template <typename Params, typename Executor>
    inline constexpr bool supports_mark_begin_execution_v =
        hpx::functional::detail::is_tag_override_invocable_v<
            mark_begin_execution_t, Params&&, Executor&&>;

    HPX_CXX_CORE_EXPORT template <typename Params, typename InnerParams,
        typename Executor>
    inline constexpr bool supports_wrapping_mark_begin_execution_v =
        hpx::functional::detail::is_tag_override_invocable_v<
            mark_begin_execution_t, Params&&, InnerParams&&, Executor&&>;

    // execution_markers: end_of_scheduling
    HPX_CXX_CORE_EXPORT template <typename Params, typename Executor>
    inline constexpr bool supports_mark_end_of_scheduling_v =
        hpx::functional::detail::is_tag_override_invocable_v<
            mark_end_of_scheduling_t, Params&&, Executor&&>;

    HPX_CXX_CORE_EXPORT template <typename Params, typename InnerParams,
        typename Executor>
    inline constexpr bool supports_wrapping_mark_end_of_scheduling_v =
        hpx::functional::detail::is_tag_override_invocable_v<
            mark_end_of_scheduling_t, Params&&, InnerParams&&, Executor&&>;

    // execution_markers: end_execution
    HPX_CXX_CORE_EXPORT template <typename Params, typename Executor>
    inline constexpr bool supports_mark_end_execution_v =
        hpx::functional::detail::is_tag_override_invocable_v<
            mark_end_execution_t, Params&&, Executor&&>;

    HPX_CXX_CORE_EXPORT template <typename Params, typename InnerParams,
        typename Executor>
    inline constexpr bool supports_wrapping_mark_end_execution_v =
        hpx::functional::detail::is_tag_override_invocable_v<
            mark_end_execution_t, Params&&, InnerParams&&, Executor&&>;

    // execution_markers: partition
    HPX_CXX_CORE_EXPORT template <typename Params, typename Executor,
        typename... Args>
    inline constexpr bool supports_mark_partition_v =
        hpx::functional::detail::is_tag_override_invocable_v<mark_partition_t,
            Params&&, Executor&&, std::size_t, Args&&...>;

    HPX_CXX_CORE_EXPORT template <typename Params, typename InnerParams,
        typename Executor, typename... Args>
    inline constexpr bool supports_wrapping_mark_partition_v =
        hpx::functional::detail::is_tag_override_invocable_v<mark_partition_t,
            Params&&, InnerParams&&, Executor&&, std::size_t, Args&&...>;

    // wrapping processing_units_count
    HPX_CXX_CORE_EXPORT template <typename Params, typename Executor>
    inline constexpr bool supports_processing_units_count_v =
        hpx::functional::detail::is_tag_override_invocable_v<
            processing_units_count_t, Params&&, Executor&&,
            hpx::chrono::steady_duration const&, std::size_t>;

    HPX_CXX_CORE_EXPORT template <typename Params, typename InnerParams,
        typename Executor>
    inline constexpr bool supports_wrapping_processing_units_count_v =
        hpx::functional::detail::is_tag_override_invocable_v<
            processing_units_count_t, Params&&, InnerParams&&, Executor&&,
            hpx::chrono::steady_duration const&, std::size_t>;

    // wrapping reset_thread_distribution
    HPX_CXX_CORE_EXPORT template <typename Params, typename Executor>
    inline constexpr bool supports_reset_thread_distribution_v =
        hpx::functional::detail::is_tag_override_invocable_v<
            reset_thread_distribution_t, Params&&, Executor&&>;

    HPX_CXX_CORE_EXPORT template <typename Params, typename InnerParams,
        typename Executor>
    inline constexpr bool supports_wrapping_reset_thread_distribution_v =
        hpx::functional::detail::is_tag_override_invocable_v<
            reset_thread_distribution_t, Params&&, InnerParams&&, Executor&&>;

    // wrapping collect_execution_parameters
    HPX_CXX_CORE_EXPORT template <typename Params, typename Executor>
    inline constexpr bool supports_collect_execution_parameters_v =
        hpx::functional::detail::is_tag_override_invocable_v<
            collect_execution_parameters_t, Params&&, Executor&&, std::size_t,
            std::size_t, std::size_t, std::size_t>;

    HPX_CXX_CORE_EXPORT template <typename Params, typename InnerParams,
        typename Executor>
    inline constexpr bool supports_wrapping_collect_execution_parameters_v =
        hpx::functional::detail::is_tag_override_invocable_v<
            collect_execution_parameters_t, Params&&, InnerParams&&, Executor&&,
            std::size_t, std::size_t, std::size_t, std::size_t>;

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

        // clang-format off
        template <typename Params, typename Executor>
            requires(std::same_as<wrapped_params, std::decay_t<Params>> && (
                supports_wrapping_get_chunk_size_v<detail::wrapping_t<Params>,
                    detail::wrapped_t<Params>, Executor> ||
                supports_get_chunk_size_v<
                    detail::wrapping_t<Params>, Executor> ||
                supports_get_chunk_size_v<
                    detail::wrapped_t<Params>, Executor>
            ))
        // clang-format on
        friend constexpr std::size_t tag_override_invoke(get_chunk_size_t,
            Params&& this_, Executor&& exec,
            hpx::chrono::steady_duration const& t, std::size_t const cores,
            std::size_t const num_iterations)
        {
            if constexpr (supports_wrapping_get_chunk_size_v<
                              detail::wrapping_t<Params>,
                              detail::wrapped_t<Params>, Executor>)
            {
                return get_chunk_size(detail::wrapping_forward<Params>(this_),
                    detail::wrapped_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec), t, cores, num_iterations);
            }
            else if constexpr (supports_get_chunk_size_v<
                                   detail::wrapping_t<Params>, Executor>)
            {
                return get_chunk_size(detail::wrapping_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec), t, cores, num_iterations);
            }
            else
            {
                return get_chunk_size(detail::wrapped_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec), t, cores, num_iterations);
            }
        }

        // wrapping measure_iteration

        // clang-format off
        template <typename Params, typename Executor, typename F>
            requires(std::same_as<wrapped_params, std::decay_t<Params>> && (
                supports_wrapping_measure_iteration_v<detail::wrapping_t<Params>,
                    detail::wrapped_t<Params>, Executor, F> ||
                supports_measure_iteration_v<
                    detail::wrapping_t<Params>, Executor, F> ||
                supports_measure_iteration_v<
                    detail::wrapped_t<Params>, Executor, F>
            ))
        // clang-format on
        friend constexpr hpx::chrono::steady_duration tag_override_invoke(
            measure_iteration_t, Params&& this_, Executor&& exec, F&& f,
            std::size_t const num_tasks)
        {
            if constexpr (supports_wrapping_measure_iteration_v<
                              detail::wrapping_t<Params>,
                              detail::wrapped_t<Params>, Executor, F>)
            {
                return measure_iteration(
                    detail::wrapping_forward<Params>(this_),
                    detail::wrapped_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec), HPX_FORWARD(F, f), num_tasks);
            }
            else if constexpr (supports_measure_iteration_v<
                                   detail::wrapping_t<Params>, Executor, F>)
            {
                return measure_iteration(
                    detail::wrapping_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec), HPX_FORWARD(F, f), num_tasks);
            }
            else
            {
                return measure_iteration(detail::wrapped_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec), HPX_FORWARD(F, f), num_tasks);
            }
        }

        // maximal_number_of_chunks

        // clang-format off
        template <typename Params, typename Executor>
            requires(std::same_as<wrapped_params, std::decay_t<Params>> && (
                supports_wrapping_maximal_number_of_chunks_v<
                    detail::wrapping_t<Params>, detail::wrapped_t<Params>,
                    Executor> ||
                supports_maximal_number_of_chunks_v<
                    detail::wrapping_t<Params>, Executor> ||
                supports_maximal_number_of_chunks_v<
                    detail::wrapped_t<Params>, Executor>
            ))
        // clang-format on
        friend constexpr std::size_t tag_override_invoke(
            maximal_number_of_chunks_t, Params&& this_, Executor&& exec,
            std::size_t const cores, std::size_t const num_tasks)
        {
            if constexpr (supports_wrapping_maximal_number_of_chunks_v<
                              detail::wrapping_t<Params>,
                              detail::wrapped_t<Params>, Executor>)
            {
                return maximal_number_of_chunks(
                    detail::wrapping_forward<Params>(this_),
                    detail::wrapped_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec), cores, num_tasks);
            }
            else if constexpr (supports_maximal_number_of_chunks_v<
                                   detail::wrapping_t<Params>, Executor>)
            {
                return maximal_number_of_chunks(
                    detail::wrapping_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec), cores, num_tasks);
            }
            else
            {
                return maximal_number_of_chunks(
                    detail::wrapped_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec), cores, num_tasks);
            }
        }

        // execution_markers: begin_execution

        // clang-format off
        template <typename Params, typename Executor>
            requires(std::same_as<wrapped_params, std::decay_t<Params>> && (
                supports_wrapping_mark_begin_execution_v<
                    detail::wrapping_t<Params>, detail::wrapped_t<Params>,
                    Executor> ||
                supports_mark_begin_execution_v<
                    detail::wrapping_t<Params>, Executor> ||
                supports_mark_begin_execution_v<
                    detail::wrapped_t<Params>, Executor>
            ))
        // clang-format on
        friend constexpr void tag_override_invoke(
            mark_begin_execution_t, Params&& this_, Executor&& exec)
        {
            if constexpr (supports_wrapping_mark_begin_execution_v<
                              detail::wrapping_t<Params>,
                              detail::wrapped_t<Params>, Executor>)
            {
                mark_begin_execution(detail::wrapping_forward<Params>(this_),
                    detail::wrapped_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec));
            }
            if constexpr (supports_mark_begin_execution_v<
                              detail::wrapping_t<Params>, Executor>)
            {
                mark_begin_execution(detail::wrapping_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec));
            }
            else
            {
                mark_begin_execution(detail::wrapped_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec));
            }
        }

        // execution_markers: end_of_scheduling

        // clang-format off
        template <typename Params, typename Executor>
            requires(std::same_as<wrapped_params, std::decay_t<Params>> && (
                supports_wrapping_mark_end_of_scheduling_v<
                    detail::wrapping_t<Params>, detail::wrapped_t<Params>,
                    Executor> ||
                supports_mark_end_of_scheduling_v<
                    detail::wrapping_t<Params>, Executor> ||
                supports_mark_end_of_scheduling_v<
                    detail::wrapped_t<Params>, Executor>
            ))
        // clang-format on
        friend constexpr void tag_override_invoke(
            mark_end_of_scheduling_t, Params&& this_, Executor&& exec)
        {
            if constexpr (supports_wrapping_mark_end_of_scheduling_v<
                              detail::wrapping_t<Params>,
                              detail::wrapped_t<Params>, Executor>)
            {
                mark_end_of_scheduling(detail::wrapping_forward<Params>(this_),
                    detail::wrapped_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec));
            }
            else if constexpr (supports_mark_end_of_scheduling_v<
                                   detail::wrapping_t<Params>, Executor>)
            {
                mark_end_of_scheduling(detail::wrapping_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec));
            }
            else
            {
                mark_end_of_scheduling(detail::wrapped_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec));
            }
        }

        // execution_markers: end_execution

        // clang-format off
        template <typename Params, typename Executor>
            requires(std::same_as<wrapped_params, std::decay_t<Params>> && (
                supports_wrapping_mark_end_execution_v<
                    detail::wrapping_t<Params>, detail::wrapped_t<Params>,
                    Executor> ||
                supports_mark_end_execution_v<
                    detail::wrapping_t<Params>, Executor> ||
                supports_mark_end_execution_v<
                    detail::wrapped_t<Params>, Executor>
            ))
        // clang-format on
        friend constexpr void tag_override_invoke(
            mark_end_execution_t, Params&& this_, Executor&& exec)
        {
            if constexpr (supports_wrapping_mark_end_execution_v<
                              detail::wrapping_t<Params>,
                              detail::wrapped_t<Params>, Executor>)
            {
                mark_end_execution(detail::wrapping_forward<Params>(this_),
                    detail::wrapped_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec));
            }
            else if constexpr (supports_mark_end_execution_v<
                                   detail::wrapping_t<Params>, Executor>)
            {
                mark_end_execution(detail::wrapping_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec));
            }
            else
            {
                mark_end_execution(detail::wrapped_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec));
            }
        }

        // execution_markers: partition

        // clang-format off
        template <typename Params, typename Executor, typename... Args>
            requires(std::same_as<wrapped_params, std::decay_t<Params>> && (
                supports_wrapping_mark_partition_v<
                    detail::wrapping_t<Params>, detail::wrapped_t<Params>,
                    Executor, Args...> ||
                supports_mark_partition_v<
                    detail::wrapping_t<Params>, Executor, Args...> ||
                supports_mark_partition_v<
                    detail::wrapped_t<Params>, Executor, Args...>
            ))
        // clang-format on
        friend constexpr void tag_override_invoke(mark_partition_t,
            Params&& this_, Executor&& exec, std::size_t partition,
            Args&&... args)
        {
            if constexpr (supports_wrapping_mark_partition_v<
                              detail::wrapping_t<Params>,
                              detail::wrapped_t<Params>, Executor, Args...>)
            {
                mark_partition(detail::wrapping_forward<Params>(this_),
                    detail::wrapped_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec), partition,
                    HPX_FORWARD(Args, args)...);
            }
            else if constexpr (supports_mark_partition_v<
                                   detail::wrapping_t<Params>, Executor,
                                   Args...>)
            {
                mark_partition(detail::wrapping_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec), partition,
                    HPX_FORWARD(Args, args)...);
            }
            else
            {
                mark_partition(detail::wrapped_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec), partition,
                    HPX_FORWARD(Args, args)...);
            }
        }

        // wrapping processing_units_count

        // clang-format off
        template <typename Params, typename Executor>
            requires(std::same_as<wrapped_params, std::decay_t<Params>> && (
                supports_wrapping_processing_units_count_v<
                    detail::wrapping_t<Params>, detail::wrapped_t<Params>,
                    Executor> ||
                supports_processing_units_count_v<
                    detail::wrapping_t<Params>, Executor> ||
                supports_processing_units_count_v<
                    detail::wrapped_t<Params>, Executor>
            ))
        // clang-format on
        friend constexpr std::size_t tag_override_invoke(
            processing_units_count_t, Params&& this_, Executor&& exec,
            hpx::chrono::steady_duration const& iteration_duration,
            std::size_t const num_tasks)
        {
            if constexpr (supports_wrapping_processing_units_count_v<
                              detail::wrapping_t<Params>,
                              detail::wrapped_t<Params>, Executor>)
            {
                return processing_units_count(
                    detail::wrapping_forward<Params>(this_),
                    detail::wrapped_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec), iteration_duration, num_tasks);
            }
            else if constexpr (supports_processing_units_count_v<
                                   detail::wrapping_t<Params>, Executor>)
            {
                return processing_units_count(
                    detail::wrapping_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec), iteration_duration, num_tasks);
            }
            else
            {
                return processing_units_count(
                    detail::wrapped_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec), iteration_duration, num_tasks);
            }
        }

        // wrapping reset_thread_distribution

        // clang-format off
        template <typename Params, typename Executor>
            requires(std::same_as<wrapped_params, std::decay_t<Params>> && (
                supports_wrapping_reset_thread_distribution_v<
                     detail::wrapping_t<Params>, detail::wrapped_t<Params>,
                     Executor> ||
                supports_reset_thread_distribution_v<
                     detail::wrapping_t<Params>, Executor> ||
                supports_reset_thread_distribution_v<
                    detail::wrapped_t<Params>, Executor>
            ))
        // clang-format on
        friend constexpr void tag_override_invoke(
            reset_thread_distribution_t, Params&& this_, Executor&& exec)
        {
            if constexpr (supports_wrapping_reset_thread_distribution_v<
                              detail::wrapping_t<Params>,
                              detail::wrapped_t<Params>, Executor>)
            {
                reset_thread_distribution(
                    detail::wrapping_forward<Params>(this_),
                    detail::wrapped_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec));
            }
            else if constexpr (supports_reset_thread_distribution_v<
                                   detail::wrapping_t<Params>, Executor>)
            {
                reset_thread_distribution(
                    detail::wrapping_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec));
            }
            else
            {
                reset_thread_distribution(
                    detail::wrapped_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec));
            }
        }

        // wrapping collect_execution_parameters

        // clang-format off
        template <typename Params, typename Executor>
            requires(std::same_as<wrapped_params, std::decay_t<Params>> && (
                supports_wrapping_collect_execution_parameters_v<
                    detail::wrapping_t<Params>, detail::wrapped_t<Params>,
                    Executor> ||
                supports_collect_execution_parameters_v<
                    detail::wrapping_t<Params>, Executor> ||
                supports_collect_execution_parameters_v<
                    detail::wrapped_t<Params>, Executor>
            ))
        // clang-format on
        friend constexpr void tag_override_invoke(
            hpx::execution::experimental::collect_execution_parameters_t,
            Params&& this_, Executor&& exec, std::size_t const num_elements,
            std::size_t const num_cores, std::size_t const num_chunks,
            std::size_t const chunk_size)
        {
            if constexpr (supports_wrapping_collect_execution_parameters_v<
                              detail::wrapping_t<Params>,
                              detail::wrapped_t<Params>, Executor>)
            {
                collect_execution_parameters(
                    detail::wrapping_forward<Params>(this_),
                    detail::wrapped_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec), num_elements, num_cores,
                    num_chunks, chunk_size);
            }
            else if constexpr (supports_collect_execution_parameters_v<
                                   detail::wrapping_t<Params>, Executor>)
            {
                collect_execution_parameters(
                    detail::wrapping_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec), num_elements, num_cores,
                    num_chunks, chunk_size);
            }
            else
            {
                collect_execution_parameters(
                    detail::wrapped_forward<Params>(this_),
                    HPX_FORWARD(Executor, exec), num_elements, num_cores,
                    num_chunks, chunk_size);
            }
        }

        Wrapped wrapped;
        Wrapping wrapping;
    };

    HPX_CXX_CORE_EXPORT template <typename Wrapped, typename Wrapping>
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
