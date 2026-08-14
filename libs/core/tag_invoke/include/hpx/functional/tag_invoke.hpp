//  Copyright (c) 2020 Thomas Heller
//  Copyright (c) 2020-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#if defined(DOXYGEN)
namespace hpx::functional {

    inline namespace unspecified {
        /// The `hpx::functional::tag_invoke` name defines a constexpr object
        /// that is invocable with one or more arguments. The first argument
        /// is a 'tag' (typically a CPO). It is only invocable if an overload
        /// of tag_invoke() that accepts the same arguments could be found via
        /// ADL.
        ///
        /// The evaluation of the expression `hpx::tag_invoke(tag, args...)` is
        /// equivalent to evaluating the unqualified call to
        /// `tag_invoke(decay-copy(tag), HPX_FORWARD(Args, args)...)`.
        ///
        /// `hpx::functional::tag_invoke` is implemented against P1895.
        ///
        /// Example:
        /// Defining a new customization point `foo`:
        /// ```
        /// namespace mylib {
        ///     inline constexpr
        ///         struct foo_fn final : hpx::functional::tag<foo_fn>
        ///         {
        ///         } foo{};
        /// }
        /// ```
        ///
        /// Defining an object `bar` which customizes `foo`:
        /// ```
        /// struct bar
        /// {
        ///     int x = 42;
        ///
        ///     friend constexpr int tag_invoke(mylib::foo_fn, bar const& x)
        ///     {
        ///         return b.x;
        ///     }
        /// };
        /// ```
        ///
        /// Using the customization point:
        /// ```
        /// static_assert(42 == mylib::foo(bar{}), "The answer is 42");
        /// ```
        inline constexpr unspecified tag_invoke = unspecified;
    }    // namespace unspecified

    /// `hpx::functional::is_tag_invocable<Tag, Args...>` is std::true_type if
    /// an overload of `tag_invoke(tag, args...)` can be found via ADL.
    template <typename Tag, typename... Args>
    struct is_tag_invocable;

    /// `hpx::functional::is_tag_invocable_v<Tag, Args...>` evaluates to
    /// `hpx::functional::is_tag_invocable<Tag, Args...>::value`
    template <typename Tag, typename... Args>
    constexpr bool is_tag_invocable_v = is_tag_invocable<Tag, Args...>::value;

    /// `hpx::functional::is_nothrow_tag_invocable<Tag, Args...>` is
    /// std::true_type if an overload of `tag_invoke(tag, args...)` can be
    /// found via ADL and is noexcept.
    template <typename Tag, typename... Args>
    struct is_nothrow_tag_invocable;

    /// `hpx::functional::is_tag_invocable_v<Tag, Args...>` evaluates to
    /// `hpx::functional::is_tag_invocable<Tag, Args...>::value`
    template <typename Tag, typename... Args>
    constexpr bool is_nothrow_tag_invocable_v =
        is_nothrow_tag_invocable<Tag, Args...>::value;

    /// `hpx::functional::tag_invoke_result<Tag, Args...>` is the trait
    /// returning the result type of the call hpx::functional::tag_invoke. This
    /// can be used in a SFINAE context.
    template <typename Tag, typename... Args>
    using tag_invoke_result = invoke_result<decltype(tag_invoke), Tag, Args...>;

    /// `hpx::functional::tag_invoke_result_t<Tag, Args...>` evaluates to
    /// `hpx::functional::tag_invoke_result_t<Tag, Args...>::type`
    template <typename Tag, typename... Args>
    using tag_invoke_result_t = typename tag_invoke_result<Tag, Args...>::type;

    /// `hpx::functional::tag<Tag>` defines a base class that implements
    /// the necessary tag dispatching functionality for a given type `Tag`
    template <typename Tag>
    struct tag;

    /// `hpx::functional::tag_noexcept<Tag>` defines a base class that implements
    /// the necessary tag dispatching functionality for a given type `Tag`
    /// The implementation has to be noexcept
    template <typename Tag>
    struct tag_noexcept;
}    // namespace hpx::functional

#else

#include <hpx/config.hpp>
#include <hpx/functional/invoke_result.hpp>
#include <hpx/functional/traits/is_invocable.hpp>
#include <hpx/modules/type_support.hpp>

#include <type_traits>
#include <utility>

namespace hpx::functional {

    HPX_CXX_CORE_EXPORT template <auto& Tag>
    using tag_t = std::decay_t<decltype(Tag)>;

    namespace tag_invoke_t_ns {

        // poison pill
        HPX_CXX_CORE_EXPORT void tag_invoke();

        HPX_CXX_CORE_EXPORT struct tag_invoke_t
        {
            // different versions of clang-format disagree
            // clang-format off
            template <typename Tag, typename... Ts>
            HPX_HOST_DEVICE HPX_FORCEINLINE constexpr auto operator()(
                Tag tag, Ts&&... ts) const
                noexcept(noexcept(
                    tag_invoke(std::declval<Tag>(), HPX_FORWARD(Ts, ts)...)))
                    -> decltype(
                        tag_invoke(std::declval<Tag>(), HPX_FORWARD(Ts, ts)...))
            // clang-format on
            {
                return tag_invoke(tag, HPX_FORWARD(Ts, ts)...);
            }

            friend constexpr bool operator==(
                tag_invoke_t, tag_invoke_t) noexcept
            {
                return true;
            }

            friend constexpr bool operator!=(
                tag_invoke_t, tag_invoke_t) noexcept
            {
                return false;
            }
        };
    }    // namespace tag_invoke_t_ns

    namespace tag_invoke_ns {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        HPX_CXX_CORE_EXPORT inline constexpr tag_invoke_t_ns::tag_invoke_t
            tag_invoke = {};
#else
        HPX_DEVICE static tag_invoke_t_ns::tag_invoke_t const tag_invoke = {};
#endif
    }    // namespace tag_invoke_ns

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Tag, typename... Args>
    using is_tag_invocable =
        hpx::is_invocable<decltype(tag_invoke_ns::tag_invoke), Tag, Args...>;

    HPX_CXX_CORE_EXPORT template <typename Tag, typename... Args>
    inline constexpr bool is_tag_invocable_v =
        is_tag_invocable<Tag, Args...>::value;

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename Sig, bool Invocable>
        struct is_nothrow_tag_invocable_impl;

        template <typename Sig>
        struct is_nothrow_tag_invocable_impl<Sig, false> : std::false_type
        {
        };

        template <typename Tag, typename... Args>
        struct is_nothrow_tag_invocable_impl<
            decltype(tag_invoke_ns::tag_invoke)(Tag, Args...), true>
          : std::integral_constant<bool,
                noexcept(tag_invoke_ns::tag_invoke(
                    std::declval<Tag>(), std::declval<Args>()...))>
        {
        };
    }    // namespace detail

    // CUDA versions less than 11.2 have a template instantiation bug that
    // leaves out certain template arguments and leads to us not being able to
    // correctly check this condition. We default to the more relaxed
    // noexcept(true) to not falsely exclude correct overloads. However, this
    // may lead to noexcept(false) overloads falsely being candidates.
#if !defined(HPX_CUDA_VERSION) || (HPX_CUDA_VERSION >= 1102)
    HPX_CXX_CORE_EXPORT template <typename Tag, typename... Args>
    struct is_nothrow_tag_invocable
      : detail::is_nothrow_tag_invocable_impl<
            decltype(tag_invoke_ns::tag_invoke)(Tag, Args...),
            is_tag_invocable_v<Tag, Args...>>
    {
    };
#else
    HPX_CXX_CORE_EXPORT template <typename Tag, typename... Args>
    struct is_nothrow_tag_invocable : std::true_type
    {
    };
#endif

    HPX_CXX_CORE_EXPORT template <typename Tag, typename... Args>
    inline constexpr bool is_nothrow_tag_invocable_v =
        is_nothrow_tag_invocable<Tag, Args...>::value;

    HPX_CXX_CORE_EXPORT template <typename Tag, typename... Args>
    using tag_invoke_result =
        hpx::util::invoke_result<decltype(tag_invoke_ns::tag_invoke), Tag,
            Args...>;

    HPX_CXX_CORE_EXPORT template <typename Tag, typename... Args>
    using tag_invoke_result_t = typename tag_invoke_result<Tag, Args...>::type;

    ///////////////////////////////////////////////////////////////////////////////
    namespace tag_base_ns {

        // poison pill
        HPX_CXX_CORE_EXPORT void tag_invoke();

        // use this tag type to enable the tag_invoke function overloads
        HPX_CXX_CORE_EXPORT struct enable_tag_invoke_t;

        ///////////////////////////////////////////////////////////////////////////
        // helper base class implementing the tag_invoke logic for CPOs
        HPX_CXX_CORE_EXPORT template <typename Tag, typename Enable>
        struct tag    // NOLINT(bugprone-crtp-constructor-accessibility)
        {
            template <typename... Args,
                typename = std::enable_if_t<meta::value<
                    meta::invoke<Enable, enable_tag_invoke_t, Args&&...>>>>
            HPX_HOST_DEVICE HPX_FORCEINLINE constexpr auto operator()(
                Args&&... args) const
                noexcept(is_nothrow_tag_invocable_v<Tag, Args&&...>)
                    -> tag_invoke_result_t<Tag, Args...>
            {
                return tag_invoke(
                    static_cast<Tag const&>(*this), HPX_FORWARD(Args, args)...);
            }
        };

        HPX_CXX_CORE_EXPORT template <typename Tag, typename Enable>
        // NOLINTNEXTLINE(bugprone-crtp-constructor-accessibility)
        struct tag_noexcept
        {
            template <typename... Args,
                typename =
                    std::enable_if_t<is_nothrow_tag_invocable_v<Tag, Args...> &&
                        meta::value<meta::invoke<Enable, enable_tag_invoke_t,
                            Args&&...>>>>
            HPX_HOST_DEVICE HPX_FORCEINLINE constexpr auto operator()(
                Args&&... args) const noexcept
                -> tag_invoke_result_t<Tag, decltype(args)...>
            {
                return tag_invoke(
                    static_cast<Tag const&>(*this), HPX_FORWARD(Args, args)...);
            }
        };
    }    // namespace tag_base_ns

    inline namespace tag_invoke_base_ns {

        HPX_CXX_CORE_EXPORT template <typename Tag,
            typename Enable = meta::constant<meta::bool_<true>>>
        using tag = tag_base_ns::tag<Tag, Enable>;

        HPX_CXX_CORE_EXPORT template <typename Tag,
            typename Enable = meta::constant<meta::bool_<true>>>
        using tag_noexcept = tag_base_ns::tag_noexcept<Tag, Enable>;

        HPX_CXX_CORE_EXPORT using enable_tag_invoke_t =
            tag_base_ns::enable_tag_invoke_t;
    }    // namespace tag_invoke_base_ns

    inline namespace tag_invoke_f_ns {

        HPX_CXX_CORE_EXPORT using tag_invoke_ns::tag_invoke;
    }    // namespace tag_invoke_f_ns
}    // namespace hpx::functional

#endif
