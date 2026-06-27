//  Copyright (c) 2014-2025 Hartmut Kaiser
//  Copyright (c) 2016 Marcin Copik
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <functional>
#include <type_traits>

namespace hpx::traits {

    // new executor framework
    HPX_CXX_CORE_EXPORT template <typename Parameters, typename Enable = void>
    struct is_executor_parameters;
}    // namespace hpx::traits

namespace hpx::execution::experimental {

    ///////////////////////////////////////////////////////////////////////////
    // Default sequential executor parameters
    HPX_CXX_CORE_EXPORT struct sequential_executor_parameters
    {
    };

    // If an executor exposes 'executor_parameter_type' this type is assumed to
    // represent the default parameters for the given executor type.
    HPX_CXX_CORE_EXPORT template <typename Executor, typename Enable = void>
    struct extract_executor_parameters
    {
        // by default, assume sequential execution
        using type = sequential_executor_parameters;
    };

    template <typename Executor>
    struct extract_executor_parameters<Executor,
        std::void_t<typename Executor::executor_parameters_type>>
    {
        using type = typename Executor::executor_parameters_type;
    };

    HPX_CXX_CORE_EXPORT template <typename Executor>
    using extract_executor_parameters_t =
        typename extract_executor_parameters<Executor>::type;

    ///////////////////////////////////////////////////////////////////////////
    // If a parameters type exposes an embedded type  'has_variable_chunk_size'
    // it is assumed that the number of loop iterations to combine is different
    // for each of the generated chunks.
    HPX_CXX_CORE_EXPORT template <typename Parameters, typename Enable = void>
    struct extract_has_variable_chunk_size : std::false_type
    {
        // by default, assume equally sized chunks
    };

    template <typename Parameters>
    struct extract_has_variable_chunk_size<Parameters,
        std::void_t<typename Parameters::has_variable_chunk_size>>
      : std::true_type
    {
    };

    template <typename Parameters>
    struct extract_has_variable_chunk_size<::std::reference_wrapper<Parameters>>
      : extract_has_variable_chunk_size<Parameters>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Parameters>
    inline constexpr bool extract_has_variable_chunk_size_v =
        extract_has_variable_chunk_size<Parameters>::value;

    ///////////////////////////////////////////////////////////////////////////
    // If a parameters type exposes an embedded type 'invokes_testing_function'
    // it is assumed that the parameters object uses the given function to
    // determine the number of chunks to apply.
    HPX_CXX_CORE_EXPORT template <typename Parameters, typename Enable = void>
    struct extract_invokes_testing_function : std::false_type
    {
        // by default, assume equally sized chunks
    };

#if !defined(DOXYGEN)
    // doxygen gets confused by the following construct
    template <typename Parameters>
    struct extract_invokes_testing_function<Parameters,
        std::void_t<typename Parameters::invokes_testing_function>>
      : std::true_type
    {
    };
#endif

    template <typename Parameters>
    struct extract_invokes_testing_function<
        ::std::reference_wrapper<Parameters>>
      : extract_invokes_testing_function<Parameters>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Parameters>
    inline constexpr bool extract_invokes_testing_function_v =
        extract_invokes_testing_function<Parameters>::value;

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT template <typename T>
        struct is_executor_parameters : std::false_type
        {
        };

        template <>
        struct is_executor_parameters<sequential_executor_parameters>
          : std::true_type
        {
        };

        template <typename T>
        struct is_executor_parameters<::std::reference_wrapper<T>>
          : hpx::traits::is_executor_parameters<T>
        {
        };
        /// \endcond
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_executor_parameters
      : detail::is_executor_parameters<std::decay_t<T>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_executor_parameters_v =
        is_executor_parameters<T>::value;
}    // namespace hpx::execution::experimental

namespace hpx {

    namespace traits {

        // new executor framework
        HPX_CXX_CORE_EXPORT template <typename Parameters, typename Enable>
        struct is_executor_parameters
          : hpx::execution::experimental::is_executor_parameters<
                std::decay_t<Parameters>>
        {
        };

        HPX_CXX_CORE_EXPORT template <typename T>
        inline constexpr bool is_executor_parameters_v =
            is_executor_parameters<T>::value;
    }    // namespace traits

    HPX_CXX_CORE_EXPORT template <typename Parameters>
    concept executor_parameters =
        hpx::traits::is_executor_parameters_v<Parameters>;
}    // namespace hpx
