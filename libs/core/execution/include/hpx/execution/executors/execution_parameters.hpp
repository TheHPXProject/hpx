//  Copyright (c) 2016 Marcin Copik
//  Copyright (c) 2016-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/execution/detail/execution_parameter_callbacks.hpp>
#include <hpx/execution/detail/future_exec.hpp>
#include <hpx/execution/executors/execution.hpp>
#include <hpx/execution/executors/execution_parameters_fwd.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/preprocessor.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/type_support.hpp>

#include <concepts>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::execution::experimental::detail {

    /// \cond NOINTERNAL

    ///////////////////////////////////////////////////////////////////////
    template <typename Property, template <typename> class CheckForProperty>
    struct get_parameters_property_t final
      : hpx::functional::detail::tag_fallback<
            get_parameters_property_t<Property, CheckForProperty>>
    {
    private:
        using derived_property_t =
            get_parameters_property_t<Property, CheckForProperty>;

        template <typename T>
        using check_for_property = CheckForProperty<std::decay_t<T>>;

        template <typename Executor, typename Parameters>
            requires(!hpx::traits::is_executor_parameters_v<Parameters> ||
                !check_for_property<Parameters>::value)
        friend HPX_FORCEINLINE constexpr decltype(auto) tag_fallback_invoke(
            derived_property_t, Executor&& /*exec*/, Parameters&& /*params*/,
            Property prop) noexcept
        {
            return std::make_pair(prop, prop);
        }

        ///////////////////////////////////////////////////////////////////
        // Parameters directly supports property
        template <typename Executor, typename Parameters>
            requires(hpx::traits::is_executor_parameters_v<Parameters> &&
                check_for_property<Parameters>::value)
        friend HPX_FORCEINLINE constexpr decltype(auto) tag_fallback_invoke(
            derived_property_t, Executor&& exec, Parameters&& params,
            Property /*prop*/) noexcept
        {
            return std::pair<Parameters&&, Executor&&>(
                HPX_FORWARD(Parameters, params), HPX_FORWARD(Executor, exec));
        }

        ///////////////////////////////////////////////////////////////////
        // Executor directly supports property
        template <typename Executor, typename Parameters>
            requires(hpx::traits::is_executor_any_v<Executor> &&
                check_for_property<Executor>::value)
        friend HPX_FORCEINLINE constexpr decltype(auto) tag_invoke(
            derived_property_t, Executor&& exec, Parameters&& params,
            Property /*prop*/) noexcept
        {
            return std::pair<Executor&&, Parameters&&>(
                HPX_FORWARD(Executor, exec), HPX_FORWARD(Parameters, params));
        }
    };

    ///////////////////////////////////////////////////////////////////////
    // define member traits
    HPX_HAS_MEMBER_XXX_TRAIT_DEF(get_chunk_size)

    ///////////////////////////////////////////////////////////////////////
    // default property implementation allowing to handle get_chunk_size
    struct get_chunk_size_property
    {
        // default implementation
        template <typename Target>
        HPX_FORCEINLINE static constexpr std::size_t get_chunk_size(Target,
            hpx::chrono::steady_duration const&, std::size_t,
            std::size_t) noexcept
        {
            // return zero for the chunk-size, which will tell the
            // implementation to calculate the chunk size either based on a
            // specified maximum number of chunks or based on some internal
            // rule (if no maximum number of chunks was given)
            return 0;
        }
    };

    //////////////////////////////////////////////////////////////////////
    // Generate a type that is guaranteed to support get_chunk_size
    using get_parameters_chunk_size_t =
        get_parameters_property_t<get_chunk_size_property,
            has_get_chunk_size_t>;

    inline constexpr get_parameters_chunk_size_t get_parameters_chunk_size{};

    //////////////////////////////////////////////////////////////////////
    // customization point for interface get_chunk_size()
    template <typename Parameters, typename Executor_>
    struct get_chunk_size_fn_helper<Parameters, Executor_,
        std::enable_if_t<hpx::traits::is_executor_any_v<Executor_>>>
    {
        template <typename Executor>
        HPX_FORCEINLINE static constexpr std::size_t call(Parameters& params,
            Executor&& exec,
            hpx::chrono::steady_duration const& iteration_duration,
            std::size_t cores, std::size_t num_tasks)
        {
            auto get_prop = get_parameters_chunk_size(
                HPX_FORWARD(Executor, exec), params, get_chunk_size_property{});

            return get_prop.first.get_chunk_size(
                HPX_FORWARD(decltype(get_prop.second), get_prop.second),
                iteration_duration, cores, num_tasks);
        }

        template <typename AnyParameters, typename Executor>
        HPX_FORCEINLINE static constexpr std::size_t call(AnyParameters params,
            Executor&& exec,
            hpx::chrono::steady_duration const& iteration_duration,
            std::size_t cores, std::size_t num_tasks)
        {
            return call(static_cast<Parameters&>(params),
                HPX_FORWARD(Executor, exec), iteration_duration, cores,
                num_tasks);
        }
    };

    ///////////////////////////////////////////////////////////////////////
    // define member traits
    HPX_HAS_MEMBER_XXX_TRAIT_DEF(measure_iteration)

    ///////////////////////////////////////////////////////////////////////
    // default property implementation allowing to handle measure_iteration
    struct measure_iteration_property
    {
        // default implementation
        template <typename Target, typename F>
        HPX_FORCEINLINE static constexpr decltype(auto) measure_iteration(
            Target, F&&, std::size_t) noexcept
        {
            // return zero for the iteration duration, which will tell the
            // implementation to calculate the chunk size either based on a
            // specified maximum number of chunks or based on some internal
            // rule (if no maximum number of chunks was given)
            return hpx::chrono::null_duration;
        }
    };

    //////////////////////////////////////////////////////////////////////
    // Generate a type that is guaranteed to support measure_iteration
    using measure_iteration_t =
        get_parameters_property_t<measure_iteration_property,
            has_measure_iteration_t>;

    inline constexpr measure_iteration_t get_parameters_measure_iteration{};

    //////////////////////////////////////////////////////////////////////
    // customization point for interface measure_iteration()
    template <typename Parameters, typename Executor_>
    struct measure_iteration_fn_helper<Parameters, Executor_,
        std::enable_if_t<hpx::traits::is_executor_any_v<Executor_>>>
    {
        template <typename Executor, typename F>
        HPX_FORCEINLINE static constexpr decltype(auto) call(
            Parameters& params, Executor&& exec, F&& f, std::size_t num_tasks)
        {
            auto get_prop =
                get_parameters_measure_iteration(HPX_FORWARD(Executor, exec),
                    params, measure_iteration_property{});

            return get_prop.first.measure_iteration(
                HPX_FORWARD(decltype(get_prop.second), get_prop.second),
                HPX_FORWARD(F, f), num_tasks);
        }

        template <typename AnyParameters, typename Executor, typename F>
        HPX_FORCEINLINE static constexpr decltype(auto) call(
            AnyParameters params, Executor&& exec, F&& f, std::size_t num_tasks)
        {
            return call(static_cast<Parameters&>(params),
                HPX_FORWARD(Executor, exec), HPX_FORWARD(F, f), num_tasks);
        }
    };

    ///////////////////////////////////////////////////////////////////////
    // define member traits
    HPX_HAS_MEMBER_XXX_TRAIT_DEF(maximal_number_of_chunks)

    ///////////////////////////////////////////////////////////////////////
    // default property implementation allowing to handle
    // maximal_number_of_chunks
    struct maximal_number_of_chunks_property
    {
        // default implementation

        // different versions of clang-format disagree
        template <typename Target>
        HPX_FORCEINLINE static constexpr std::size_t maximal_number_of_chunks(
            Target, std::size_t, std::size_t) noexcept
        {
            // return zero chunks which will tell the implementation to
            // calculate the number of chunks either based on a
            // specified chunk size or based on some internal rule (if no
            // chunk-size was given)
            return 0;
        }
    };

    //////////////////////////////////////////////////////////////////////
    // Generate a type that is guaranteed to support
    // maximal_number_of_chunks
    using get_maximal_number_of_chunks_t =
        get_parameters_property_t<maximal_number_of_chunks_property,
            has_maximal_number_of_chunks_t>;

    inline constexpr get_maximal_number_of_chunks_t
        get_maximal_number_of_chunks{};

    ///////////////////////////////////////////////////////////////////////
    // customization point for interface maximal_number_of_chunks()
    template <typename Parameters, typename Executor_>
    struct maximal_number_of_chunks_fn_helper<Parameters, Executor_,
        std::enable_if_t<hpx::traits::is_executor_any_v<Executor_>>>
    {
        template <typename Executor>
        HPX_FORCEINLINE static constexpr std::size_t call(Parameters& params,
            Executor&& exec, std::size_t cores, std::size_t num_tasks)
        {
            auto get_prop =
                get_maximal_number_of_chunks(HPX_FORWARD(Executor, exec),
                    params, maximal_number_of_chunks_property{});

            return get_prop.first.maximal_number_of_chunks(
                HPX_FORWARD(decltype(get_prop.second), get_prop.second), cores,
                num_tasks);
        }

        template <typename AnyParameters, typename Executor>
        HPX_FORCEINLINE static constexpr std::size_t call(AnyParameters params,
            Executor&& exec, std::size_t cores, std::size_t num_tasks)
        {
            return call(static_cast<Parameters&>(params),
                HPX_FORWARD(Executor, exec), cores, num_tasks);
        }
    };

    ///////////////////////////////////////////////////////////////////////
    // define member traits
    HPX_HAS_MEMBER_XXX_TRAIT_DEF(reset_thread_distribution)

    ///////////////////////////////////////////////////////////////////////
    // default property implementation allowing to handle
    // reset_thread_distribution
    struct reset_thread_distribution_property
    {
        // default implementation
        template <typename Target>
        HPX_FORCEINLINE static constexpr void reset_thread_distribution(
            Target) noexcept
        {
        }
    };

    //////////////////////////////////////////////////////////////////////
    // Generate a type that is guaranteed to support
    // reset_thread_distribution
    using get_reset_thread_distribution_t =
        get_parameters_property_t<reset_thread_distribution_property,
            has_reset_thread_distribution_t>;

    inline constexpr get_reset_thread_distribution_t
        get_reset_thread_distribution{};

    ///////////////////////////////////////////////////////////////////////
    // customization point for interface reset_thread_distribution()
    template <typename Parameters, typename Executor_>
    struct reset_thread_distribution_fn_helper<Parameters, Executor_,
        std::enable_if_t<hpx::traits::is_executor_any_v<Executor_>>>
    {
        template <typename Executor>
        HPX_FORCEINLINE static constexpr void call(
            Parameters& params, Executor&& exec)
        {
            auto get_prop =
                get_reset_thread_distribution(HPX_FORWARD(Executor, exec),
                    params, reset_thread_distribution_property{});

            get_prop.first.reset_thread_distribution(
                HPX_FORWARD(decltype(get_prop.second), get_prop.second));
        }

        template <typename AnyParameters, typename Executor>
        HPX_FORCEINLINE static constexpr void call(
            AnyParameters params, Executor&& exec)
        {
            call(static_cast<Parameters&>(params), HPX_FORWARD(Executor, exec));
        }
    };

    ///////////////////////////////////////////////////////////////////////
    // define member traits
    HPX_HAS_MEMBER_XXX_TRAIT_DEF(processing_units_count)

    ///////////////////////////////////////////////////////////////////////
    // default property implementation allowing to handle
    // processing_units_count_property
    struct processing_units_count_property
    {
        // default implementation
        template <typename Target>
        HPX_FORCEINLINE static std::size_t processing_units_count(Target&&,
            hpx::chrono::steady_duration const& = hpx::chrono::null_duration,
            std::size_t = 0)
        {
            return hpx::parallel::execution::detail::get_os_thread_count();
        }
    };

    //////////////////////////////////////////////////////////////////////
    // Generate a type that is guaranteed to support
    // processing_units_count_property
    using get_processing_units_count_target_t =
        get_parameters_property_t<processing_units_count_property,
            has_processing_units_count_t>;

    inline constexpr get_processing_units_count_target_t
        get_processing_units_count_target{};

    ///////////////////////////////////////////////////////////////////////
    // customization point for interface processing_units_count()
    template <typename Parameters, typename Executor_>
    struct processing_units_count_fn_helper<Parameters, Executor_,
        std::enable_if_t<hpx::traits::is_executor_any_v<Executor_>>>
    {
        template <typename Executor>
        HPX_FORCEINLINE static constexpr std::size_t call(Parameters& params,
            Executor&& exec,
            hpx::chrono::steady_duration const& iteration_duration,
            std::size_t num_tasks)
        {
            auto get_prop =
                get_processing_units_count_target(HPX_FORWARD(Executor, exec),
                    params, processing_units_count_property{});

            return get_prop.first.processing_units_count(
                HPX_FORWARD(decltype(get_prop.second), get_prop.second),
                iteration_duration, num_tasks);
        }

        template <typename AnyParameters, typename Executor>
        HPX_FORCEINLINE static constexpr std::size_t call(AnyParameters params,
            Executor&& exec,
            hpx::chrono::steady_duration const& iteration_duration,
            std::size_t num_tasks)
        {
            return call(static_cast<Parameters&>(params),
                HPX_FORWARD(Executor, exec), iteration_duration, num_tasks);
        }
    };

    template <typename Executor_>
    struct processing_units_count_fn_helper<void, Executor_,
        std::enable_if_t<hpx::traits::is_executor_any_v<Executor_>>>
    {
        template <typename Executor>
        HPX_FORCEINLINE static constexpr std::size_t call(Executor&& exec,
            hpx::chrono::steady_duration const& iteration_duration,
            std::size_t num_tasks)
        {
            return processing_units_count_property::processing_units_count(
                HPX_FORWARD(Executor, exec), iteration_duration, num_tasks);
        }
    };

    ///////////////////////////////////////////////////////////////////////
    // define member traits
    HPX_HAS_MEMBER_XXX_TRAIT_DEF(mark_begin_execution)

    ///////////////////////////////////////////////////////////////////////
    // default property implementation allowing to handle
    // mark_begin_execution
    struct mark_begin_execution_property
    {
        // default implementation
        template <typename Target>
        HPX_FORCEINLINE static constexpr void mark_begin_execution(
            Target) noexcept
        {
        }
    };

    //////////////////////////////////////////////////////////////////////
    // Generate a type that is guaranteed to support
    // mark_begin_execution
    using get_mark_begin_execution_t =
        get_parameters_property_t<mark_begin_execution_property,
            has_mark_begin_execution_t>;

    inline constexpr get_mark_begin_execution_t get_mark_begin_execution{};

    ///////////////////////////////////////////////////////////////////////
    // customization point for interface mark_begin_execution()
    template <typename Parameters, typename Executor_>
    struct mark_begin_execution_fn_helper<Parameters, Executor_,
        std::enable_if_t<hpx::traits::is_executor_any_v<Executor_>>>
    {
        template <typename Executor>
        HPX_FORCEINLINE static constexpr void call(
            Parameters& params, Executor&& exec)
        {
            auto get_prop =
                get_mark_begin_execution(HPX_FORWARD(Executor, exec), params,
                    mark_begin_execution_property{});

            get_prop.first.mark_begin_execution(
                HPX_FORWARD(decltype(get_prop.second), get_prop.second));
        }

        template <typename AnyParameters, typename Executor>
        HPX_FORCEINLINE static constexpr void call(
            AnyParameters params, Executor&& exec)
        {
            call(static_cast<Parameters&>(params), HPX_FORWARD(Executor, exec));
        }
    };

    ///////////////////////////////////////////////////////////////////////
    // define member traits
    HPX_HAS_MEMBER_XXX_TRAIT_DEF(mark_end_of_scheduling)

    ///////////////////////////////////////////////////////////////////////
    // default property implementation allowing to handle
    // mark_end_of_scheduling
    struct mark_end_of_scheduling_property
    {
        // default implementation
        template <typename Target>
        HPX_FORCEINLINE static constexpr void mark_end_of_scheduling(
            Target) noexcept
        {
        }
    };

    //////////////////////////////////////////////////////////////////////
    // Generate a type that is guaranteed to support
    // mark_end_of_scheduling
    using get_mark_end_of_scheduling_t =
        get_parameters_property_t<mark_end_of_scheduling_property,
            has_mark_end_of_scheduling_t>;

    inline constexpr get_mark_end_of_scheduling_t get_mark_end_of_scheduling{};

    ///////////////////////////////////////////////////////////////////////
    // customization point for interface mark_end_of_scheduling()
    template <typename Parameters, typename Executor_>
    struct mark_end_of_scheduling_fn_helper<Parameters, Executor_,
        std::enable_if_t<hpx::traits::is_executor_any_v<Executor_>>>
    {
        template <typename Executor>
        HPX_FORCEINLINE static constexpr void call(
            Parameters& params, Executor&& exec)
        {
            auto get_prop =
                get_mark_end_of_scheduling(HPX_FORWARD(Executor, exec), params,
                    mark_end_of_scheduling_property{});

            get_prop.first.mark_end_of_scheduling(
                HPX_FORWARD(decltype(get_prop.second), get_prop.second));
        }

        template <typename AnyParameters, typename Executor>
        HPX_FORCEINLINE static constexpr void call(
            AnyParameters params, Executor&& exec)
        {
            call(static_cast<Parameters&>(params), HPX_FORWARD(Executor, exec));
        }
    };

    ///////////////////////////////////////////////////////////////////////
    // define member traits
    HPX_HAS_MEMBER_XXX_TRAIT_DEF(mark_end_execution)

    ///////////////////////////////////////////////////////////////////////
    // default property implementation allowing to handle
    // mark_end_execution
    struct mark_end_execution_property
    {
        // default implementation
        template <typename Target>
        HPX_FORCEINLINE static constexpr void mark_end_execution(
            Target) noexcept
        {
        }
    };

    //////////////////////////////////////////////////////////////////////
    // Generate a type that is guaranteed to support
    // mark_end_execution
    using get_mark_end_execution_t =
        get_parameters_property_t<mark_end_execution_property,
            has_mark_end_execution_t>;

    inline constexpr get_mark_end_execution_t get_mark_end_execution{};

    ///////////////////////////////////////////////////////////////////////
    // customization point for interface mark_end_execution()
    template <typename Parameters, typename Executor_>
    struct mark_end_execution_fn_helper<Parameters, Executor_,
        std::enable_if_t<hpx::traits::is_executor_any_v<Executor_>>>
    {
        template <typename Executor>
        HPX_FORCEINLINE static constexpr void call(
            Parameters& params, Executor&& exec)
        {
            auto get_prop = get_mark_end_execution(HPX_FORWARD(Executor, exec),
                params, mark_end_execution_property{});

            get_prop.first.mark_end_execution(
                HPX_FORWARD(decltype(get_prop.second), get_prop.second));
        }

        template <typename AnyParameters, typename Executor>
        HPX_FORCEINLINE static constexpr void call(
            AnyParameters params, Executor&& exec)
        {
            call(static_cast<Parameters&>(params), HPX_FORWARD(Executor, exec));
        }
    };

    ///////////////////////////////////////////////////////////////////////
    // define member traits
    HPX_HAS_MEMBER_XXX_TRAIT_DEF(mark_partition)

    ///////////////////////////////////////////////////////////////////////
    // default property implementation allowing to handle
    // mark_partition
    struct mark_partition_property
    {
        // default implementation
        template <typename Target, typename... Args>
        HPX_FORCEINLINE static constexpr void mark_partition(
            Target, std::size_t, Args&&...) noexcept
        {
        }
    };

    //////////////////////////////////////////////////////////////////////
    // Generate a type that is guaranteed to support
    // mark_partition
    using get_mark_partition_t =
        get_parameters_property_t<mark_partition_property,
            has_mark_partition_t>;

    inline constexpr get_mark_partition_t get_mark_partition{};

    ///////////////////////////////////////////////////////////////////////
    // customization point for interface mark_partition()
    template <typename Parameters, typename Executor_>
    struct mark_partition_fn_helper<Parameters, Executor_,
        std::enable_if_t<hpx::traits::is_executor_any_v<Executor_>>>
    {
        template <typename Executor, typename... Args>
        HPX_FORCEINLINE static constexpr void call(Parameters& params,
            Executor&& exec, std::size_t partition, Args&&... args)
        {
            auto get_prop = get_mark_partition(
                HPX_FORWARD(Executor, exec), params, mark_partition_property{});

            get_prop.first.mark_partition(
                HPX_FORWARD(decltype(get_prop.second), get_prop.second),
                partition, HPX_FORWARD(Args, args)...);
        }

        template <typename AnyParameters, typename Executor, typename... Args>
        HPX_FORCEINLINE static constexpr void call(AnyParameters params,
            Executor&& exec, std::size_t partition, Args&&... args)
        {
            call(static_cast<Parameters&>(params), HPX_FORWARD(Executor, exec),
                partition, HPX_FORWARD(Args, args)...);
        }
    };

    ///////////////////////////////////////////////////////////////////////
    // define member traits
    HPX_HAS_MEMBER_XXX_TRAIT_DEF(collect_execution_parameters)

    ///////////////////////////////////////////////////////////////////////
    // default property implementation allowing to handle
    // collect_execution_parameters
    struct collect_execution_parameters_property
    {
        // default implementation
        template <typename Target>
        HPX_FORCEINLINE static constexpr void collect_execution_parameters(
            Target, std::size_t, std::size_t, std::size_t, std::size_t) noexcept
        {
        }
    };

    //////////////////////////////////////////////////////////////////////
    // Generate a type that is guaranteed to support
    // collect_execution_parameters
    using get_collect_execution_parameters_t =
        get_parameters_property_t<collect_execution_parameters_property,
            has_collect_execution_parameters_t>;

    inline constexpr get_collect_execution_parameters_t
        get_collect_execution_parameters{};

    ///////////////////////////////////////////////////////////////////////
    // customization point for interface collect_execution_parameters()
    template <typename Parameters, typename Executor_>
    struct collect_execution_parameters_fn_helper<Parameters, Executor_,
        std::enable_if_t<hpx::traits::is_executor_any_v<Executor_>>>
    {
        template <typename Executor>
        HPX_FORCEINLINE static constexpr void call(Parameters& params,
            Executor&& exec, std::size_t num_elements, std::size_t num_cores,
            std::size_t num_chunks, std::size_t chunk_size)
        {
            auto get_prop =
                get_collect_execution_parameters(HPX_FORWARD(Executor, exec),
                    params, collect_execution_parameters_property{});

            get_prop.first.collect_execution_parameters(
                HPX_FORWARD(decltype(get_prop.second), get_prop.second),
                num_elements, num_cores, num_chunks, chunk_size);
        }

        template <typename AnyParameters, typename Executor>
        HPX_FORCEINLINE static constexpr void call(
            AnyParameters params, Executor&& exec)
        {
            call(static_cast<Parameters&>(params), HPX_FORWARD(Executor, exec));
        }
    };
    /// \endcond

    /// \cond NOINTERNAL
    template <bool... Flags>
    struct parameters_type_counter;

    template <>
    struct parameters_type_counter<>
    {
        static constexpr int value = 0;
    };

    /// Return the number of parameters which are true
    template <bool Flag1, bool... Flags>
    struct parameters_type_counter<Flag1, Flags...>
    {
        static constexpr int value =
            Flag1 + parameters_type_counter<Flags...>::value;
    };

    ///////////////////////////////////////////////////////////////////////
    template <typename T>
    struct unwrapper : T
    {
        // default constructor is needed for serialization purposes
        unwrapper()
            requires(std::is_constructible_v<T>)
          : T()
        {
        }

        // generic poor-man's forwarding constructor
        template <typename U>
            requires(!std::same_as<std::decay_t<U>, unwrapper>)
        unwrapper(U&& u)
          : T(HPX_FORWARD(U, u))
        {
        }
    };

    ///////////////////////////////////////////////////////////////////////
    template <typename T, typename Wrapper, typename Enable = void>
    struct maximal_number_of_chunks_call_helper
    {
    };

    template <typename T, typename Wrapper>
    struct maximal_number_of_chunks_call_helper<T, Wrapper,
        std::enable_if_t<has_maximal_number_of_chunks_v<T>>>
    {
        template <typename Executor>
        HPX_FORCEINLINE std::size_t maximal_number_of_chunks(
            Executor&& exec, std::size_t cores, std::size_t num_tasks) const
        {
            auto& wrapped =
                static_cast<unwrapper<Wrapper> const*>(this)->member_.get();
            return wrapped.maximal_number_of_chunks(
                HPX_FORWARD(Executor, exec), cores, num_tasks);
        }
    };

    ///////////////////////////////////////////////////////////////////////
    template <typename T, typename Wrapper, typename Enable = void>
    struct get_chunk_size_call_helper
    {
    };

    template <typename T, typename Wrapper>
    struct get_chunk_size_call_helper<T, Wrapper,
        std::enable_if_t<has_get_chunk_size_v<T>>>
    {
        template <typename Executor>
        HPX_FORCEINLINE std::size_t get_chunk_size(Executor&& exec,
            hpx::chrono::steady_duration const& iteration_duration,
            std::size_t cores, std::size_t num_tasks) const
        {
            auto& wrapped =
                static_cast<unwrapper<Wrapper> const*>(this)->member_.get();
            return wrapped.get_chunk_size(HPX_FORWARD(Executor, exec),
                iteration_duration, cores, num_tasks);
        }
    };

    ///////////////////////////////////////////////////////////////////////
    template <typename T, typename Wrapper, typename Enable = void>
    struct measure_iteration_call_helper
    {
    };

    template <typename T, typename Wrapper>
    struct measure_iteration_call_helper<T, Wrapper,
        std::enable_if_t<has_measure_iteration_v<T>>>
    {
        template <typename Executor, typename F>
        HPX_FORCEINLINE std::size_t measure_iteration(Executor&& exec, F&& f,
            std::size_t cores, std::size_t num_tasks) const
        {
            auto& wrapped =
                static_cast<unwrapper<Wrapper> const*>(this)->member_.get();
            return wrapped.measure_iteration(HPX_FORWARD(Executor, exec),
                HPX_FORWARD(F, f), cores, num_tasks);
        }
    };

    ///////////////////////////////////////////////////////////////////////
    template <typename T, typename Wrapper, typename Enable = void>
    struct mark_begin_execution_call_helper
    {
    };

    template <typename T, typename Wrapper>
    struct mark_begin_execution_call_helper<T, Wrapper,
        std::enable_if_t<has_mark_begin_execution_v<T>>>
    {
        template <typename Executor>
        HPX_FORCEINLINE void mark_begin_execution(Executor&& exec)
        {
            auto& wrapped =
                static_cast<unwrapper<Wrapper>*>(this)->member_.get();
            wrapped.mark_begin_execution(HPX_FORWARD(Executor, exec));
        }
    };

    ///////////////////////////////////////////////////////////////////////
    template <typename T, typename Wrapper, typename Enable = void>
    struct mark_end_of_scheduling_call_helper
    {
    };

    template <typename T, typename Wrapper>
    struct mark_end_of_scheduling_call_helper<T, Wrapper,
        std::enable_if_t<has_mark_begin_execution_v<T>>>
    {
        template <typename Executor>
        HPX_FORCEINLINE void mark_end_of_scheduling(Executor&& exec)
        {
            auto& wrapped =
                static_cast<unwrapper<Wrapper>*>(this)->member_.get();
            wrapped.mark_end_of_scheduling(HPX_FORWARD(Executor, exec));
        }
    };

    ///////////////////////////////////////////////////////////////////////
    template <typename T, typename Wrapper, typename Enable = void>
    struct mark_end_execution_call_helper
    {
    };

    template <typename T, typename Wrapper>
    struct mark_end_execution_call_helper<T, Wrapper,
        std::enable_if_t<has_mark_begin_execution_v<T>>>
    {
        template <typename Executor>
        HPX_FORCEINLINE void mark_end_execution(Executor&& exec)
        {
            auto& wrapped =
                static_cast<unwrapper<Wrapper>*>(this)->member_.get();
            wrapped.mark_end_execution(HPX_FORWARD(Executor, exec));
        }
    };

    ///////////////////////////////////////////////////////////////////////
    template <typename T, typename Wrapper, typename Enable = void>
    struct mark_partition_call_helper
    {
    };

    template <typename T, typename Wrapper>
    struct mark_partition_call_helper<T, Wrapper,
        std::enable_if_t<has_mark_partition_v<T>>>
    {
        template <typename Executor, typename... Args>
        HPX_FORCEINLINE void mark_partition(
            Executor&& exec, std::size_t partition, Args&&... args)
        {
            auto& wrapped =
                static_cast<unwrapper<Wrapper>*>(this)->member_.get();
            wrapped.mark_partition(HPX_FORWARD(Executor, exec), partition,
                HPX_FORWARD(Args, args)...);
        }
    };

    ///////////////////////////////////////////////////////////////////////
    template <typename T, typename Wrapper, typename Enable = void>
    struct processing_units_count_call_helper
    {
    };

    template <typename T, typename Wrapper>
    struct processing_units_count_call_helper<T, Wrapper,
        std::enable_if_t<has_processing_units_count_v<T>>>
    {
        template <typename Executor>
        HPX_FORCEINLINE std::size_t processing_units_count(
            Executor&& exec) const
        {
            auto& wrapped =
                static_cast<unwrapper<Wrapper> const*>(this)->member_.get();
            return wrapped.processing_units_count(HPX_FORWARD(Executor, exec));
        }
    };

    ///////////////////////////////////////////////////////////////////////
    template <typename T, typename Wrapper, typename Enable = void>
    struct reset_thread_distribution_call_helper
    {
    };

    template <typename T, typename Wrapper>
    struct reset_thread_distribution_call_helper<T, Wrapper,
        std::enable_if_t<has_reset_thread_distribution_v<T>>>
    {
        template <typename Executor>
        HPX_FORCEINLINE void reset_thread_distribution(Executor&& exec)
        {
            auto& wrapped =
                static_cast<unwrapper<Wrapper>*>(this)->member_.get();
            wrapped.reset_thread_distribution(HPX_FORWARD(Executor, exec));
        }
    };

    ///////////////////////////////////////////////////////////////////////
    template <typename T, typename Wrapper, typename Enable = void>
    struct collect_execution_parameters_call_helper
    {
    };

    template <typename T, typename Wrapper>
    struct collect_execution_parameters_call_helper<T, Wrapper,
        std::enable_if_t<has_collect_execution_parameters_v<T>>>
    {
        template <typename Executor>
        HPX_FORCEINLINE void collect_execution_parameters(Executor&& exec,
            std::size_t num_elements, std::size_t num_cores,
            std::size_t num_chunks, std::size_t chunk_size)
        {
            auto& wrapped =
                static_cast<unwrapper<Wrapper>*>(this)->member_.get();
            wrapped.collect_execution_parameters(HPX_FORWARD(Executor, exec),
                num_elements, num_cores, num_chunks, chunk_size);
        }
    };

    ///////////////////////////////////////////////////////////////////////
    template <typename T>
    struct base_member_helper
    {
        explicit constexpr base_member_helper(T t) noexcept
          : member_(HPX_MOVE(t))
        {
        }

        T member_;
    };

    ///////////////////////////////////////////////////////////////////////
    template <typename T>
    struct unwrapper<::std::reference_wrapper<T>>
      : base_member_helper<::std::reference_wrapper<T>>
      , maximal_number_of_chunks_call_helper<T, std::reference_wrapper<T>>
      , get_chunk_size_call_helper<T, std::reference_wrapper<T>>
      , measure_iteration_call_helper<T, std::reference_wrapper<T>>
      , mark_begin_execution_call_helper<T, std::reference_wrapper<T>>
      , mark_end_of_scheduling_call_helper<T, std::reference_wrapper<T>>
      , mark_end_execution_call_helper<T, std::reference_wrapper<T>>
      , mark_partition_call_helper<T, std::reference_wrapper<T>>
      , processing_units_count_call_helper<T, std::reference_wrapper<T>>
      , reset_thread_distribution_call_helper<T, std::reference_wrapper<T>>
      , collect_execution_parameters_call_helper<T, std::reference_wrapper<T>>
    {
        using wrapper_type = std::reference_wrapper<T>;

        constexpr explicit unwrapper(wrapper_type wrapped_param) noexcept
          : base_member_helper<wrapper_type>(HPX_MOVE(wrapped_param))
        {
        }
    };

    ///////////////////////////////////////////////////////////////////////
#define HPX_STATIC_ASSERT_ON_PARAMETERS_AMBIGUITY(func)                        \
    static_assert(                                                             \
        parameters_type_counter<                                               \
            HPX_PP_CAT(hpx::execution::experimental::detail::has_, func) <     \
            hpx::util::decay_unwrap_t<Params>>::value... > ::value <= 1,       \
        "Passing more than one executor parameters type "                      \
        "exposing " HPX_PP_STRINGIZE(func) " is not possible") /**/

    template <typename... Params>
    struct executor_parameters : unwrapper<Params>...
    {
        static_assert(
            hpx::util::all_of_v<
                hpx::traits::is_executor_parameters<std::decay_t<Params>>...>,
            "All passed parameters must be a proper executor parameters "
            "objects");
        static_assert(sizeof...(Params) >= 2,
            "This type is meant to be used with at least 2 parameters "
            "objects");

        HPX_STATIC_ASSERT_ON_PARAMETERS_AMBIGUITY(get_chunk_size);
        HPX_STATIC_ASSERT_ON_PARAMETERS_AMBIGUITY(measure_iteration);
        HPX_STATIC_ASSERT_ON_PARAMETERS_AMBIGUITY(mark_begin_execution);
        HPX_STATIC_ASSERT_ON_PARAMETERS_AMBIGUITY(mark_end_of_scheduling);
        HPX_STATIC_ASSERT_ON_PARAMETERS_AMBIGUITY(mark_end_execution);
        HPX_STATIC_ASSERT_ON_PARAMETERS_AMBIGUITY(mark_partition);
        HPX_STATIC_ASSERT_ON_PARAMETERS_AMBIGUITY(processing_units_count);
        HPX_STATIC_ASSERT_ON_PARAMETERS_AMBIGUITY(maximal_number_of_chunks);
        HPX_STATIC_ASSERT_ON_PARAMETERS_AMBIGUITY(reset_thread_distribution);
        HPX_STATIC_ASSERT_ON_PARAMETERS_AMBIGUITY(collect_execution_parameters);

        constexpr executor_parameters()
            requires(hpx::util::all_of_v<std::is_constructible<Params>...>)
          : unwrapper<Params>()...
        {
        }

        template <typename... Params_>
            requires(sizeof...(Params) == sizeof...(Params_))
        constexpr explicit executor_parameters(Params_&&... params)
          : unwrapper<Params>(HPX_FORWARD(Params_, params))...
        {
        }

    private:
        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, unsigned int const /* version */)
        {
            (hpx::serialization::detail::serialize_one(
                 ar, serialization::base_object<Params>(*this)),
                ...);
        }
    };

#undef HPX_STATIC_ASSERT_ON_PARAMETERS_AMBIGUITY

    /// \endcond
}    // namespace hpx::execution::experimental::detail

namespace hpx::execution::experimental {

    ///////////////////////////////////////////////////////////////////////////
    // specialize trait for the type-combiner
    HPX_CXX_CORE_EXPORT template <typename... Parameters>
    struct is_executor_parameters<detail::executor_parameters<Parameters...>>
      : hpx::util::all_of<hpx::traits::is_executor_parameters<Parameters>...>
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename... Params>
    struct executor_parameters_join
    {
        using type = detail::executor_parameters<std::decay_t<Params>...>;
    };

    HPX_CXX_CORE_EXPORT template <typename... Params>
    constexpr HPX_FORCEINLINE executor_parameters_join<Params...>::type
    join_executor_parameters(Params&&... params)
    {
        using joined_params = executor_parameters_join<Params...>::type;
        return joined_params(HPX_FORWARD(Params, params)...);
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Param>
    struct executor_parameters_join<Param>
    {
        using type = Param;
    };

    HPX_CXX_CORE_EXPORT template <typename Param>
    constexpr HPX_FORCEINLINE Param&& join_executor_parameters(
        Param&& param) noexcept
    {
        static_assert(
            hpx::traits::is_executor_parameters_v<std::decay_t<Param>>,
            "The passed parameter must be a proper executor parameters object");

        return HPX_FORWARD(Param, param);
    }
}    // namespace hpx::execution::experimental
