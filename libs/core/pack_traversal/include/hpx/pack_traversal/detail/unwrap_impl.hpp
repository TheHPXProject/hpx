//  Copyright (c) 2017 Denis Blank
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/pack_traversal/pack_traversal.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx::util::detail {

    /// A mapper that maps futures to its representing type
    ///
    /// The mapper does unwrap futures nested inside futures until the
    /// particular given depth.
    ///
    /// - Depth >  1 -> Depth remaining
    /// - Depth == 1 -> One depth remaining
    /// - Depth == 0 -> Unlimited depths
    template <std::size_t Depth>
    struct future_unwrap_until_depth
    {
        /// This piece of code can't be refactored out using inheritance and
        /// `using Base::operator()` because this isn't taken into account when
        /// doing SFINAE.
        template <typename T,
            std::enable_if_t<traits::is_future_void_v<std::decay_t<T>>>* =
                nullptr>
        auto operator()(T&& future) const -> decltype(spread_this())
        {
            // CUDA needs std::forward here
            std::forward<T>(future).get();
            return spread_this();
        }

        template <typename T,
            std::enable_if_t<!traits::is_future_void_v<std::decay_t<T>>>* =
                nullptr>
        auto operator()(T&& future) const
            -> decltype(map_pack(future_unwrap_until_depth<Depth - 1>{},
                std::forward<T>(future).get()))
        {
            return map_pack(future_unwrap_until_depth<Depth - 1>{},
                std::forward<T>(future).get());
        }
    };

    template <>
    struct future_unwrap_until_depth<1U>
    {
        /// This piece of code can't be refactored out using inheritance and
        /// `using Base::operator()` because this isn't taken into account when
        /// doing SFINAE.
        template <typename T,
            std::enable_if_t<traits::is_future_void_v<std::decay_t<T>>>* =
                nullptr>
        auto operator()(T&& future) const -> decltype(spread_this())
        {
            // CUDA needs std::forward here
            std::forward<T>(future).get();
            return spread_this();
        }

        template <typename T,
            std::enable_if_t<!traits::is_future_void_v<std::decay_t<T>>>* =
                nullptr>
        auto operator()(T&& future) const ->
            typename traits::future_traits<std::decay_t<T>>::result_type
        {
            // CUDA needs std::forward here
            return std::forward<T>(future).get();
        }
    };

    template <>
    struct future_unwrap_until_depth<0U>
    {
        /// This piece of code can't be refactored out using inheritance and
        /// `using Base::operator()` because this isn't taken into account when
        /// doing SFINAE.
        template <typename T,
            std::enable_if_t<traits::is_future_void_v<std::decay_t<T>>>* =
                nullptr>
        auto operator()(T&& future) const -> decltype(spread_this())
        {
            // CUDA needs std::forward here
            std::forward<T>(future).get();
            return spread_this();
        }

        // clang-format off
        template <typename T,
            std::enable_if_t<!traits::is_future_void_v<std::decay_t<T>>>* = nullptr>
        auto operator()(T&& future) const -> decltype(
            map_pack(std::declval<future_unwrap_until_depth const&>(),
                std::forward<T>(future).get()))
        {
            return map_pack(*this, std::forward<T>(future).get());
        }
        // clang-format on
    };

    /// Unwraps the futures contained in the given pack args until the depth
    /// Depth. This is the main entry function for immediate unwraps.
    template <std::size_t Depth, typename... Args>
    auto unwrap_depth_impl(Args&&... args) -> decltype(map_pack(
        future_unwrap_until_depth<Depth>{}, HPX_FORWARD(Args, args)...))
    {
        return map_pack(
            future_unwrap_until_depth<Depth>{}, HPX_FORWARD(Args, args)...);
    }

    /// We use a specialized class here because MSVC has issues with tag
    /// dispatching a function because it does semantical checks before matching
    /// the tag, which leads to false errors.
    template <bool IsFusedInvoke>
    struct invoke_wrapped_invocation_select
    {
        /// Invoke the callable with the tuple argument through invoke_fused
        template <typename C, typename T>
        static auto apply(C&& callable, T&& unwrapped)
            // There is no trait for the invoke_fused result
            -> decltype(hpx::invoke_fused(
                HPX_FORWARD(C, callable), HPX_FORWARD(T, unwrapped)))
        {
            return hpx::invoke_fused(
                HPX_FORWARD(C, callable), HPX_FORWARD(T, unwrapped));
        }
    };
    template <>
    struct invoke_wrapped_invocation_select<false /*IsFusedInvoke*/>
    {
        /// Invoke the callable with the plain argument through invoke, also
        /// when the result is a tuple like type, when we received a single
        /// argument.
        template <typename C, typename T>
        static auto apply(C&& callable, T&& unwrapped) ->
            typename invoke_result<C, T>::type
        {
            return HPX_INVOKE(
                HPX_FORWARD(C, callable), HPX_FORWARD(T, unwrapped));
        }
    };

    /// Deduces to a true_type if the result of unwrap should be fused invoked
    /// which is the case when:
    /// - The callable was called with more than one argument
    /// - The result of the unwrap is a tuple like type
    template <bool HadMultipleArguments, typename T>
    using should_fuse_invoke = std::integral_constant<bool,
        (HadMultipleArguments && traits::is_tuple_like_v<std::decay_t<T>>)>;

    /// Invokes the callable object with the result:
    /// - If the result is a tuple-like type `invoke_fused` is used
    /// - Otherwise `invoke` is used
    template <bool HadMultipleArguments, typename C, typename T>
    auto dispatch_wrapped_invocation_select(C&& callable, T&& unwrapped)
        -> decltype(invoke_wrapped_invocation_select<should_fuse_invoke<
                HadMultipleArguments, T>::value>::apply(HPX_FORWARD(C,
                                                            callable),
            HPX_FORWARD(T, unwrapped)))
    {
        return invoke_wrapped_invocation_select<should_fuse_invoke<
            HadMultipleArguments, T>::value>::apply(HPX_FORWARD(C, callable),
            HPX_FORWARD(T, unwrapped));
    }

    /// Helper for routing non void result types to the corresponding callable
    /// object.
    template <std::size_t Depth, typename Result>
    struct invoke_wrapped_decorate_select
    {
        // clang-format off
        template <typename C, typename... Args>
        static auto apply(C&& callable, Args&&... args)
            -> decltype(dispatch_wrapped_invocation_select<(
                    sizeof...(args) > 1)>(HPX_FORWARD(C, callable),
                unwrap_depth_impl<Depth>(HPX_FORWARD(Args, args)...)))
        {
            return dispatch_wrapped_invocation_select<(sizeof...(args) > 1)>(
                HPX_FORWARD(C, callable),
                unwrap_depth_impl<Depth>(HPX_FORWARD(Args, args)...));
        }
        // clang-format on
    };

    template <std::size_t Depth>
    struct invoke_wrapped_decorate_select<Depth, void>
    {
        template <typename C, typename... Args>
        static auto apply(C&& callable, Args&&... args)
            -> decltype(HPX_FORWARD(C, callable)())
        {
            unwrap_depth_impl<Depth>(HPX_FORWARD(Args, args)...);
            return HPX_FORWARD(C, callable)();
        }
    };

    /// map_pack may return a tuple, a plain type or void choose the
    /// corresponding invocation function accordingly.
    template <std::size_t Depth, typename C, typename... Args>
    auto invoke_wrapped(C&& callable, Args&&... args)
        -> decltype(invoke_wrapped_decorate_select<Depth,
            decltype(unwrap_depth_impl<Depth>(
                HPX_FORWARD(Args, args)...))>::apply(HPX_FORWARD(C, callable),
            HPX_FORWARD(Args, args)...))
    {
        return invoke_wrapped_decorate_select<Depth,
            decltype(unwrap_depth_impl<Depth>(
                HPX_FORWARD(Args, args)...))>::apply(HPX_FORWARD(C, callable),
            HPX_FORWARD(Args, args)...);
    }

    /// Implements the callable object which is returned by n invocation to
    /// hpx::unwrap and similar functions.
    template <typename T, std::size_t Depth>
    class functional_unwrap_impl
    {
        /// The wrapped callable object
        T wrapped_;

    public:
        explicit functional_unwrap_impl(T wrapped) noexcept
          : wrapped_(HPX_MOVE(wrapped))
        {
        }

        template <typename... Args>
        auto operator()(Args&&... args) -> decltype(invoke_wrapped<Depth>(
            std::declval<T&>(), HPX_FORWARD(Args, args)...))
        {
            return invoke_wrapped<Depth>(wrapped_, HPX_FORWARD(Args, args)...);
        }

        template <typename... Args>
        auto operator()(Args&&... args) const -> decltype(invoke_wrapped<Depth>(
            std::declval<T const&>(), HPX_FORWARD(Args, args)...))
        {
            return invoke_wrapped<Depth>(wrapped_, HPX_FORWARD(Args, args)...);
        }
    };

    /// Returns a callable object which unwraps the futures contained in the
    /// given pack args until the depth Depth.
    template <std::size_t Depth, typename T>
    auto functional_unwrap_depth_impl(T&& callable)
        -> functional_unwrap_impl<std::decay_t<T>, Depth>
    {
        return functional_unwrap_impl<std::decay_t<T>, Depth>(
            HPX_FORWARD(T, callable));
    }
}    // namespace hpx::util::detail
