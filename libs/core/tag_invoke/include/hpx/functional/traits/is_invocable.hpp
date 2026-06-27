//  Copyright (c) 2007-2023 Hartmut Kaiser
//  Copyright (c) 2013-2015 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// hpxinspect:nodeprecatedname:is_callable

/// \file is_invocable.hpp
/// \page hpx::is_invocable, hpx::is_invocable_r
/// \headerfile hpx/type_traits.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/functional/detail/invoke.hpp>

#include <type_traits>
#include <utility>

namespace hpx {

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        template <typename T, typename Enable = void>
        struct is_invocable_impl : std::false_type
        {
            static_assert(
                std::is_function_v<T>, "Argument must be of the form F(Ts...)");
        };

        template <typename F, typename... Ts>
        struct is_invocable_impl<F(Ts...),
            std::void_t<decltype(HPX_INVOKE(
                std::declval<F>(), std::declval<Ts>()...))>> : std::true_type
        {
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename T, typename R, typename Enable = void>
        struct is_invocable_r_impl : std::false_type
        {
        };

        // clang-format off
        template <typename F, typename... Ts, typename R>
        struct is_invocable_r_impl<F(Ts...), R,
            std::void_t<decltype(
                HPX_INVOKE(std::declval<F>(), std::declval<Ts>()...))>>
          : std::integral_constant<bool,
                std::is_void_v<R> ||
                    std::is_convertible_v<decltype(HPX_INVOKE(std::declval<F>(),
                                              std::declval<Ts>()...)),
                        R>>
        // clang-format on
        {
        };
    }    // namespace detail

    /// Determines whether \a F can be invoked with the arguments \a Ts....
    /// Formally, determines whether
    /// \code
    ///     INVOKE(std::declval<F>(), std::declval<Ts>()...)
    /// \endcode
    /// is well-formed when treated as an unevaluated operand, where \a INVOKE
    /// is the operation defined in \a Callable.
    ///
    /// \details F, R and all types in the parameter pack Ts shall each be a
    ///          complete type, (possibly cv-qualified) void, or an array of
    ///          unknown bound. Otherwise, the behavior is undefined. If an
    ///          instantiation of a template above depends, directly or
    ///          indirectly, on an incomplete type, and that instantiation could
    ///          yield a different result if that type were hypothetically
    ///          completed, the behavior is undefined.
    HPX_CXX_CORE_EXPORT template <typename F, typename... Ts>
    struct is_invocable : hpx::detail::is_invocable_impl<F && (Ts && ...)>
    {
    };

    /// Determines whether \a F can be invoked with the arguments \a Ts... to
    /// yield a result that is convertible to \a R and the implicit conversion
    /// does not bind a reference to a temporary object (since C++23). If \a R
    /// is \a cv void, the result can be any type. Formally, determines whether
    /// \code
    ///     INVOKE<R>(std::declval<F>(), std::declval<Ts>()...)
    /// \endcode
    /// is well-formed when treated as an unevaluated operand, where \a INVOKE
    /// is the operation defined in \a Callable.
    /// \copydetails is_invocable
    HPX_CXX_CORE_EXPORT template <typename R, typename F, typename... Ts>
    struct is_invocable_r
      : hpx::detail::is_invocable_r_impl<F && (Ts && ...), R>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename F, typename... Ts>
    inline constexpr bool is_invocable_v = is_invocable<F, Ts...>::value;

    HPX_CXX_CORE_EXPORT template <typename R, typename F, typename... Ts>
    inline constexpr bool is_invocable_r_v = is_invocable_r<R, F, Ts...>::value;

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename Sig, bool Invocable>
        struct is_nothrow_invocable_impl : std::false_type
        {
        };

        template <typename F, typename... Ts>
        struct is_nothrow_invocable_impl<F(Ts...), true>
          : std::integral_constant<bool,
                noexcept(std::declval<F>()(std::declval<Ts>()...))>
        {
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename F, typename... Ts>
    struct is_nothrow_invocable
      : detail::is_nothrow_invocable_impl<F(Ts...), is_invocable_v<F, Ts...>>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename F, typename... Ts>
    inline constexpr bool is_nothrow_invocable_v =
        is_nothrow_invocable<F, Ts...>::value;
}    // namespace hpx
