//  Copyright (c) 2011 Thomas Heller
//  Copyright (c) 2013-2025 Hartmut Kaiser
//  Copyright (c) 2014-2015 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file move_only_function.hpp
/// \page hpx::move_only_function
/// \headerfile hpx/functional.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/functional/detail/basic_function.hpp>
#include <hpx/functional/detail/function_registration.hpp>
#include <hpx/functional/traits/get_function_address.hpp>
#include <hpx/functional/traits/get_function_annotation.hpp>
#include <hpx/modules/tag_invoke.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    /// Class template hpx::move_only_function is a general-purpose polymorphic
    /// function wrapper. hpx::move_only_function objects can store and invoke
    /// any constructible (not required to be move constructible) Callable
    /// target -- functions, lambda expressions, bind expressions, or other
    /// function objects, as well as pointers to member functions and pointers
    /// to member objects.
    ///
    /// The stored callable object is called the target of
    /// hpx::move_only_function. If an hpx::move_only_function contains no
    /// target, it is called empty. Unlike hpx::function, invoking an empty
    /// hpx::move_only_function results in undefined behavior.
    ///
    /// hpx::move_only_functions supports every possible combination of
    /// cv-qualifiers, ref-qualifiers, and noexcept-specifiers not including
    /// volatile provided in its template parameter. These qualifiers and
    /// specifier (if any) are added to its operator(). hpx::move_only_function
    /// satisfies the requirements of MoveConstructible and MoveAssignable, but
    /// does not satisfy CopyConstructible or CopyAssignable.
    HPX_CXX_CORE_EXPORT template <typename Sig, bool Serializable = false>
    class move_only_function;

    template <typename R, typename... Ts, bool Serializable>
    class move_only_function<R(Ts...), Serializable>
      : public util::detail::basic_function<R(Ts...), false, Serializable>
    {
        using base_type =
            util::detail::basic_function<R(Ts...), false, Serializable>;

    public:
        using result_type = R;

        constexpr move_only_function(std::nullptr_t = nullptr) noexcept {}

        move_only_function(move_only_function const&) = delete;
        move_only_function(move_only_function&&) noexcept = default;
        move_only_function& operator=(move_only_function const&) = delete;
        move_only_function& operator=(move_only_function&&) noexcept = default;

        ~move_only_function() = default;

        // the split SFINAE prevents MSVC from eagerly instantiating things
        template <typename F, typename FD = std::decay_t<F>,
            typename Enable1 =
                std::enable_if_t<!std::is_same_v<FD, move_only_function>>,
            typename Enable2 =
                std::enable_if_t<is_invocable_r_v<R, FD&, Ts...>>>
        move_only_function(F&& f)
        {
            assign(HPX_FORWARD(F, f));
        }

        // the split SFINAE prevents MSVC from eagerly instantiating things
        template <typename F, typename FD = std::decay_t<F>,
            typename Enable1 =
                std::enable_if_t<!std::is_same_v<FD, move_only_function>>,
            typename Enable2 =
                std::enable_if_t<is_invocable_r_v<R, FD&, Ts...>>>
        move_only_function& operator=(F&& f)
        {
            assign(HPX_FORWARD(F, f));
            return *this;
        }

        using base_type::operator();
        using base_type::assign;
        using base_type::empty;
        using base_type::reset;
        using base_type::target;
    };

    namespace distributed {

        // serializable move_only_function is equivalent to
        // hpx::distributed::move_only_function
        HPX_CXX_CORE_EXPORT template <typename Sig>
        using move_only_function = hpx::move_only_function<Sig, true>;
    }    // namespace distributed
}    // namespace hpx

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
///////////////////////////////////////////////////////////////////////////////
namespace hpx::traits {

    HPX_CXX_CORE_EXPORT template <typename Sig, bool Serializable>
    struct get_function_address<hpx::move_only_function<Sig, Serializable>>
    {
        [[nodiscard]] static constexpr std::size_t call(
            hpx::move_only_function<Sig, Serializable> const& f) noexcept
        {
            return f.get_function_address();
        }
    };

    HPX_CXX_CORE_EXPORT template <typename Sig, bool Serializable>
    struct get_function_annotation<hpx::move_only_function<Sig, Serializable>>
    {
        [[nodiscard]] static constexpr char const* call(
            hpx::move_only_function<Sig, Serializable> const& f) noexcept
        {
            return f.get_function_annotation();
        }
    };

    HPX_CXX_CORE_EXPORT template <typename Sig, bool Serializable>
    struct get_function_annotation_tracing<
        hpx::move_only_function<Sig, Serializable>>
    {
        [[nodiscard]] static hpx::tracing::annotation_handle call(
            hpx::move_only_function<Sig, Serializable> const& f) noexcept
        {
            return f.get_function_annotation_tracing();
        }
    };
}    // namespace hpx::traits
#endif
