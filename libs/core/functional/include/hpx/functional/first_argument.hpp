//  Copyright (c) 2017 Antoine Tran Tan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/functional/traits/is_action.hpp>
#include <hpx/modules/datastructures.hpp>

#include <type_traits>

namespace hpx::util {

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        template <typename Tuple>
        struct tuple_first_argument;

        template <>
        struct tuple_first_argument<hpx::tuple<>>
        {
            using type = std::false_type;
        };

        template <typename Arg0, typename... Args>
        struct tuple_first_argument<hpx::tuple<Arg0, Args...>>
        {
            using type = std::decay_t<Arg0>;
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename F>
        struct function_first_argument;

        template <typename ReturnType>
        struct function_first_argument<ReturnType (*)()>
        {
            using type = std::false_type;
        };

        template <typename ReturnType, typename Arg0, typename... Args>
        struct function_first_argument<ReturnType (*)(Arg0, Args...)>
        {
            using type = std::decay_t<Arg0>;
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename F>
        struct lambda_first_argument;

        template <typename ClassType, typename ReturnType>
        struct lambda_first_argument<ReturnType (ClassType::*)() const>
        {
            using type = std::false_type;
        };

        template <typename ClassType, typename ReturnType, typename Arg0,
            typename... Args>
        struct lambda_first_argument<ReturnType (ClassType::*)(Arg0, Args...)
                const>
        {
            using type = std::decay_t<Arg0>;
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename F, typename Enable = void>
    struct first_argument
    {
    };

    // Specialization for actions
    template <typename F>
    struct first_argument<F, std::enable_if_t<hpx::traits::is_action_v<F>>>
      : detail::tuple_first_argument<typename F::arguments_type>
    {
    };

    // Specialization for functions
    template <typename F>
    struct first_argument<F,
        std::enable_if_t<!hpx::traits::is_action_v<F> &&
            std::is_function_v<std::remove_pointer_t<F>>>>
      : detail::function_first_argument<F>
    {
    };

    // Specialization for lambdas
    template <typename F>
    struct first_argument<F,
        std::enable_if_t<!hpx::traits::is_action_v<F> &&
            !std::is_function_v<std::remove_pointer_t<F>>>>
      : detail::lambda_first_argument<decltype(&F::operator())>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename F>
    using first_argument_t = typename first_argument<F>::type;
}    // namespace hpx::util
