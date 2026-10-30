//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \page hpx::threads::with_task_size
/// \headerfile hpx/functional.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/tracing.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx::threads {

    ///////////////////////////////////////////////////////////////////////////
    /// A function that carries the expected run time of the task it runs in.
    /// The size is a template argument, so the wrapper adds no storage.
    HPX_CXX_CORE_EXPORT template <typename F, task_size Size>
    struct sized_function
    {
        F f;

        // constrained, so that the wrapper is invocable exactly as f is
        // (callers detect which arguments f takes)
        template <typename... Ts>
            requires(hpx::is_invocable_v<F&, Ts...>)
        decltype(auto) operator()(Ts&&... ts)
        {
            return HPX_INVOKE(f, HPX_FORWARD(Ts, ts)...);
        }

        template <typename... Ts>
            requires(hpx::is_invocable_v<F const&, Ts...>)
        decltype(auto) operator()(Ts&&... ts) const
        {
            return HPX_INVOKE(f, HPX_FORWARD(Ts, ts)...);
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Returns a function that carries the given task size.
    ///
    /// \param f Function whose task is expected to run for about \a Size
    HPX_CXX_CORE_EXPORT template <task_size Size, typename F>
    constexpr sized_function<std::decay_t<F>, Size> with_task_size(F&& f)
    {
        return {HPX_FORWARD(F, f)};
    }

    /// \brief Returns a function that is expected to run for about a
    ///        nanosecond, for instance a loop body comparing or adding
    ///        builtin types.
    ///
    /// \param f Function to wrap
    HPX_CXX_CORE_EXPORT template <typename F>
    constexpr sized_function<std::decay_t<F>, task_size::nano> nano_task(F&& f)
    {
        return {HPX_FORWARD(F, f)};
    }

    /// \brief Returns a function whose task is expected to run briefly.
    ///
    /// \param f Function to wrap
    HPX_CXX_CORE_EXPORT template <typename F>
    constexpr sized_function<std::decay_t<F>, task_size::small_> small_task(
        F&& f)
    {
        return {HPX_FORWARD(F, f)};
    }

    /// \brief Returns a function whose task is expected to run for a while.
    ///
    /// \param f Function to wrap
    HPX_CXX_CORE_EXPORT template <typename F>
    constexpr sized_function<std::decay_t<F>, task_size::medium> medium_task(
        F&& f)
    {
        return {HPX_FORWARD(F, f)};
    }

    /// \brief Returns a function whose task is expected to run for long.
    ///
    /// \param f Function to wrap
    HPX_CXX_CORE_EXPORT template <typename F>
    constexpr sized_function<std::decay_t<F>, task_size::large> large_task(
        F&& f)
    {
        return {HPX_FORWARD(F, f)};
    }
}    // namespace hpx::threads

namespace hpx {

    HPX_CXX_CORE_EXPORT using threads::large_task;
    HPX_CXX_CORE_EXPORT using threads::medium_task;
    HPX_CXX_CORE_EXPORT using threads::nano_task;
    HPX_CXX_CORE_EXPORT using threads::small_task;
    HPX_CXX_CORE_EXPORT using threads::with_task_size;
}    // namespace hpx

///////////////////////////////////////////////////////////////////////////////
// The size given to the wrapper wins over the default.
template <typename F, hpx::threads::task_size Size,
    hpx::threads::task_size Default>
struct hpx::traits::get_task_size<hpx::threads::sized_function<F, Size>,
    Default>
{
    static constexpr threads::task_size value = Size;

    [[nodiscard]] static constexpr threads::task_size call(
        hpx::threads::sized_function<F, Size> const&) noexcept
    {
        return Size;
    }
};

namespace hpx::threads::detail {

    // A function that has the size of another function, which may be known
    // at run time only. A task that runs that other function once can be
    // given its size this way.
    HPX_CXX_CORE_EXPORT template <typename F>
    struct size_of_function
    {
        F f;
        task_size size;

        // constrained, so that the wrapper is invocable exactly as f is
        // (callers detect which arguments f takes)
        template <typename... Ts>
            requires(hpx::is_invocable_v<F&, Ts...>)
        decltype(auto) operator()(Ts&&... ts)
        {
            return HPX_INVOKE(f, HPX_FORWARD(Ts, ts)...);
        }

        template <typename... Ts>
            requires(hpx::is_invocable_v<F const&, Ts...>)
        decltype(auto) operator()(Ts&&... ts) const
        {
            return HPX_INVOKE(f, HPX_FORWARD(Ts, ts)...);
        }
    };

    // Returns f with the size of sized.
    HPX_CXX_CORE_EXPORT template <typename Sized, typename F>
    constexpr size_of_function<std::decay_t<F>> with_task_size_of(
        Sized const& sized, F&& f)
    {
        return {HPX_FORWARD(F, f), traits::get_task_size<Sized>::call(sized)};
    }

    // Store the size of f in the hint of the thread that runs it. Task sizes
    // come from functions only: a size the hint got from elsewhere, for
    // instance from an executor or a launch policy, is replaced, and the size
    // is unknown if f has none.
    HPX_CXX_CORE_EXPORT template <typename F>
    constexpr void set_task_size(
        thread_schedule_hint& hint, F const& f) noexcept
    {
        hint.task_size(traits::get_task_size<F>::call(f));
    }
}    // namespace hpx::threads::detail

template <typename F, hpx::threads::task_size Default>
struct hpx::traits::get_task_size<hpx::threads::detail::size_of_function<F>,
    Default>
{
    static constexpr threads::task_size value = Default;

    [[nodiscard]] static constexpr threads::task_size call(
        hpx::threads::detail::size_of_function<F> const& f) noexcept
    {
        return f.size != threads::task_size::unknown ? f.size : Default;
    }
};

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
// The wrapped function keeps its address and annotation.
template <typename F, hpx::threads::task_size Size>
struct hpx::traits::get_function_address<hpx::threads::sized_function<F, Size>>
{
    static constexpr std::size_t call(
        hpx::threads::sized_function<F, Size> const& f) noexcept
    {
        return get_function_address<F>::call(f.f);
    }
};

template <typename F, hpx::threads::task_size Size>
struct hpx::traits::get_function_annotation<
    hpx::threads::sized_function<F, Size>>
{
    static constexpr char const* call(
        hpx::threads::sized_function<F, Size> const& f) noexcept
    {
        return get_function_annotation<F>::call(f.f);
    }
};

template <typename F, hpx::threads::task_size Size>
struct hpx::traits::get_function_annotation_tracing<
    hpx::threads::sized_function<F, Size>>
{
    static hpx::tracing::annotation_handle call(
        hpx::threads::sized_function<F, Size> const& f) noexcept
    {
        return get_function_annotation_tracing<F>::call(f.f);
    }
};

template <typename F>
struct hpx::traits::get_function_address<
    hpx::threads::detail::size_of_function<F>>
{
    static constexpr std::size_t call(
        hpx::threads::detail::size_of_function<F> const& f) noexcept
    {
        return get_function_address<F>::call(f.f);
    }
};

template <typename F>
struct hpx::traits::get_function_annotation<
    hpx::threads::detail::size_of_function<F>>
{
    static constexpr char const* call(
        hpx::threads::detail::size_of_function<F> const& f) noexcept
    {
        return get_function_annotation<F>::call(f.f);
    }
};

template <typename F>
struct hpx::traits::get_function_annotation_tracing<
    hpx::threads::detail::size_of_function<F>>
{
    static hpx::tracing::annotation_handle call(
        hpx::threads::detail::size_of_function<F> const& f) noexcept
    {
        return get_function_annotation_tracing<F>::call(f.f);
    }
};
#endif
