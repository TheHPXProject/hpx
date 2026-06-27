//  Copyright (c) 2017 Antoine TRAN TAN
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/actions_base/plain_action.hpp>

#include <string>
#include <type_traits>
#include <utility>

namespace hpx::actions {

    /// \cond NOINTERNAL

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        // Helpers to convert a lambda to an action
        struct addr_add
        {
            template <typename T>
            friend std::remove_reference_t<T>* operator+(addr_add, T&& t)
            {
                return &t;
            }
        };

        template <typename F, typename ReturnType, typename... Args>
        struct lambda_action
          : basic_action<detail::plain_function, ReturnType(Args...),
                lambda_action<F, ReturnType, Args...>>
        {
            using derived_type = lambda_action;

            static std::string get_action_name(
                naming::address::address_type /*lva*/)
            {
                return "lambda action()";
            }

            template <typename... Ts>
            static ReturnType invoke(naming::address::address_type /*lva*/,
                naming::address::component_type /*comptype*/, Ts&&... vs)
            {
                // yes, this looks like this code was dereferencing a nullptr...
                int* dummy = nullptr;
                return reinterpret_cast<F const&>(*dummy)(    // -V522
                    HPX_FORWARD(Ts, vs)...);
            }
        };

        template <typename F, typename T>
        struct extract_lambda_action
        {
        };

        // Specialization for lambdas
        template <typename F, typename ClassType, typename ReturnType,
            typename... Args>
        struct extract_lambda_action<F,
            ReturnType (ClassType::*)(Args...) const>
        {
            using type = lambda_action<F, ReturnType, Args...>;
        };

        template <typename F, typename T>
        using extract_lambda_action_t =
            typename extract_lambda_action<F, T>::type;

        template <typename F>
        struct action_from_lambda
        {
            using type = extract_lambda_action_t<F, decltype(&F::operator())>;
        };

        template <typename F>
        using action_from_lambda_t = typename action_from_lambda<F>::type;

        struct action_maker
        {
            template <typename F>
            constexpr action_from_lambda_t<F> operator+=(F*) const noexcept
            {
                static_assert(std::is_empty_v<F>,
                    "lambda_to_action() needs and only needs a lambda with "
                    "empty capture list");

                return action_from_lambda_t<F>();
            }
        };
    }    // namespace detail

    HPX_CXX_EXPORT template <typename F>
    constexpr auto lambda_to_action(F&& f) noexcept
        -> decltype(hpx::actions::detail::action_maker() +=
            true ? nullptr : hpx::actions::detail::addr_add() + f)
    {
        return hpx::actions::detail::action_maker() +=
            true ? nullptr : hpx::actions::detail::addr_add() + f;
    }
}    // namespace hpx::actions
