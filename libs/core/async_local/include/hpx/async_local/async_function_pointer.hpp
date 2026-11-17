//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file async_function_pointer.hpp
/// \page hpx::async, hpx::post
/// \headerfile hpx/future.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/threading_base.hpp>

#include <type_traits>
#include <utility>

namespace hpx::detail {

    HPX_CXX_CORE_EXPORT template <auto F>
    inline constexpr bool is_function_pointer_v =
        std::is_pointer_v<decltype(F)> &&
        std::is_function_v<std::remove_pointer_t<decltype(F)>>;

    // A launch policy or an executor in front of the arguments says how f
    // runs, as it does for hpx::async(policy, f, ...).
    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_local_scheduling_v =
        traits::is_launch_policy_v<std::decay_t<T>> ||
        traits::is_one_way_executor_v<std::decay_t<T>> ||
        traits::is_two_way_executor_v<std::decay_t<T>>;

    HPX_CXX_CORE_EXPORT template <typename... Ts>
    inline constexpr bool starts_with_local_scheduling_v = false;

    template <typename T, typename... Ts>
    inline constexpr bool starts_with_local_scheduling_v<T, Ts...> =
        is_local_scheduling_v<T>;

    // The call is local unless its first argument is a remote target. The
    // remote overloads of hpx::async<&f> use the same trait, so no call is
    // taken by both.
    HPX_CXX_CORE_EXPORT template <typename... Ts>
    inline constexpr bool is_local_call_v = true;

    template <typename T, typename... Ts>
    inline constexpr bool is_local_call_v<T, Ts...> =
        !traits::is_remote_target_v<T>;

    // F with its size, or F itself if its size is unknown, so that a call
    // without a size runs exactly as hpx::async(F, ...) does.
    HPX_CXX_CORE_EXPORT template <auto F>
    constexpr auto sized_function_pointer() noexcept
    {
        constexpr threads::task_size size = traits::task_size_of_v<F>;
        if constexpr (size == threads::task_size::unknown)
        {
            return F;
        }
        else
        {
            return threads::with_task_size<size>(F);
        }
    }
}    // namespace hpx::detail

namespace hpx {

    /// \brief Runs the function \a F points to asynchronously, as
    ///        hpx::async(F, ts...) does. As \a F is a template argument, the
    ///        task can get the size of the function from
    ///        hpx::traits::task_size_of or, with C++26 annotations, from an
    ///        annotation of the function.
    ///
    /// The call is local unless the first argument is a remote target (an
    /// hpx::id_type, a distribution policy or a client), see
    /// hpx::async<&func>(target, ...) for remote calls. A call whose arguments
    /// don't fit F is not taken.
    // clang-format off
    HPX_CXX_CORE_EXPORT template <auto F, typename... Ts>
        requires(detail::is_function_pointer_v<F> &&
            !detail::starts_with_local_scheduling_v<Ts...> &&
            detail::is_local_call_v<Ts...> &&
            traits::detail::is_deferred_invocable_v<decltype(F), Ts...>)
    HPX_FORCEINLINE decltype(auto) async(Ts&&... ts)
    // clang-format on
    {
        return hpx::async(
            detail::sized_function_pointer<F>(), HPX_FORWARD(Ts, ts)...);
    }

    /// \brief Runs the function \a F points to asynchronously with the given
    ///        launch policy or executor, as hpx::async(where, F, ts...) does.
    // clang-format off
    HPX_CXX_CORE_EXPORT template <auto F, typename Where, typename... Ts>
        requires(detail::is_function_pointer_v<F> &&
            detail::is_local_scheduling_v<Where> &&
            detail::is_local_call_v<Ts...> &&
            traits::detail::is_deferred_invocable_v<decltype(F), Ts...>)
    HPX_FORCEINLINE decltype(auto) async(Where&& where, Ts&&... ts)
    // clang-format on
    {
        return hpx::async(HPX_FORWARD(Where, where),
            detail::sized_function_pointer<F>(), HPX_FORWARD(Ts, ts)...);
    }

    /// \brief Runs the function \a F points to asynchronously without
    ///        returning a future, as hpx::post(F, ts...) does. The task gets
    ///        the size of the function as for hpx::async<&func>(ts...).
    // clang-format off
    HPX_CXX_CORE_EXPORT template <auto F, typename... Ts>
        requires(detail::is_function_pointer_v<F> &&
            !detail::starts_with_local_scheduling_v<Ts...> &&
            detail::is_local_call_v<Ts...> &&
            traits::detail::is_deferred_invocable_v<decltype(F), Ts...>)
    HPX_FORCEINLINE decltype(auto) post(Ts&&... ts)
    // clang-format on
    {
        return hpx::post(
            detail::sized_function_pointer<F>(), HPX_FORWARD(Ts, ts)...);
    }

    /// \brief Runs the function \a F points to on the given executor without
    ///        returning a future, as hpx::post(exec, F, ts...) does.
    // clang-format off
    HPX_CXX_CORE_EXPORT template <auto F, typename Executor, typename... Ts>
        requires(detail::is_function_pointer_v<F> &&
            (traits::is_one_way_executor_v<std::decay_t<Executor>> ||
                traits::is_two_way_executor_v<std::decay_t<Executor>>) &&
            detail::is_local_call_v<Ts...> &&
            traits::detail::is_deferred_invocable_v<decltype(F), Ts...>)
    HPX_FORCEINLINE decltype(auto) post(Executor&& exec, Ts&&... ts)
    // clang-format on
    {
        return hpx::post(HPX_FORWARD(Executor, exec),
            detail::sized_function_pointer<F>(), HPX_FORWARD(Ts, ts)...);
    }
}    // namespace hpx
