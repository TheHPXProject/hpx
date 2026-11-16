//  Copyright (c) 2026 K S Nithurshen
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \page hpx::threads::with_stack_size
/// \headerfile hpx/functional.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/tracing.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx::threads {

    ///////////////////////////////////////////////////////////////////////////
    /// A function that carries the stack size of the thread it runs in. The
    /// stack size is a template argument, so the wrapper adds no storage.
    HPX_CXX_CORE_EXPORT template <typename F, thread_stacksize StackSize>
    struct stack_sized_function
    {
        // a function knows the stack it needs, not the one of the thread
        // that launches it
        static_assert(StackSize != thread_stacksize::unknown &&
                StackSize != thread_stacksize::current,
            "a function can be given the stack size small_, medium, large, "
            "huge or nostack only");

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
    /// \brief Returns a function that carries the given stack size.
    ///
    /// The stack size of the function wins over the one of a launch policy
    /// or an executor that runs it.
    ///
    /// \tparam StackSize The stack size the thread that runs \a f needs, one
    ///                   of small_, medium, large, huge or nostack
    /// \param f Function to wrap
    HPX_CXX_CORE_EXPORT template <thread_stacksize StackSize, typename F>
    constexpr stack_sized_function<std::decay_t<F>, StackSize> with_stack_size(
        F&& f)
    {
        return {HPX_FORWARD(F, f)};
    }

    /// \brief Returns a function that does not suspend, so that it can run
    ///        without a stack of its own (thread_stacksize::nostack).
    ///
    /// \param f Function to wrap, which must not suspend
    HPX_CXX_CORE_EXPORT template <typename F>
    constexpr stack_sized_function<std::decay_t<F>, thread_stacksize::nostack>
    no_stack(F&& f)
    {
        return {HPX_FORWARD(F, f)};
    }

    /// \brief Returns a function that runs on a small stack.
    ///
    /// \param f Function to wrap
    HPX_CXX_CORE_EXPORT template <typename F>
    constexpr stack_sized_function<std::decay_t<F>, thread_stacksize::small_>
    small_stack(F&& f)
    {
        return {HPX_FORWARD(F, f)};
    }

    /// \brief Returns a function that runs on a medium sized stack.
    ///
    /// \param f Function to wrap
    HPX_CXX_CORE_EXPORT template <typename F>
    constexpr stack_sized_function<std::decay_t<F>, thread_stacksize::medium>
    medium_stack(F&& f)
    {
        return {HPX_FORWARD(F, f)};
    }

    /// \brief Returns a function that needs a large stack, for instance as
    ///        it recurses deeply.
    ///
    /// \param f Function to wrap
    HPX_CXX_CORE_EXPORT template <typename F>
    constexpr stack_sized_function<std::decay_t<F>, thread_stacksize::large>
    large_stack(F&& f)
    {
        return {HPX_FORWARD(F, f)};
    }

    /// \brief Returns a function that needs a very large stack.
    ///
    /// \param f Function to wrap
    HPX_CXX_CORE_EXPORT template <typename F>
    constexpr stack_sized_function<std::decay_t<F>, thread_stacksize::huge>
    huge_stack(F&& f)
    {
        return {HPX_FORWARD(F, f)};
    }
}    // namespace hpx::threads

namespace hpx {

    HPX_CXX_CORE_EXPORT using threads::huge_stack;
    HPX_CXX_CORE_EXPORT using threads::large_stack;
    HPX_CXX_CORE_EXPORT using threads::medium_stack;
    HPX_CXX_CORE_EXPORT using threads::no_stack;
    HPX_CXX_CORE_EXPORT using threads::small_stack;
    HPX_CXX_CORE_EXPORT using threads::with_stack_size;
}    // namespace hpx

///////////////////////////////////////////////////////////////////////////////
// The stack size given to the wrapper wins over the default and over the
// stack size of the function it wraps, so the outer of two nested wrappers
// wins.
template <typename F, hpx::threads::thread_stacksize StackSize,
    hpx::threads::thread_stacksize Default>
struct hpx::traits::get_stack_size<
    hpx::threads::stack_sized_function<F, StackSize>, Default>
{
    static constexpr threads::thread_stacksize value = StackSize;

    [[nodiscard]] static constexpr threads::thread_stacksize call(
        hpx::threads::stack_sized_function<F, StackSize> const&) noexcept
    {
        return StackSize;
    }
};

// The wrapped function keeps its task size.
template <typename F, hpx::threads::thread_stacksize StackSize,
    hpx::threads::task_size Size>
struct hpx::traits::get_task_size<
    hpx::threads::stack_sized_function<F, StackSize>, Size>
  : get_task_size<F, Size>
{
    [[nodiscard]] static constexpr threads::task_size call(
        hpx::threads::stack_sized_function<F, StackSize> const& f) noexcept
    {
        return get_task_size<F, Size>::call(f.f);
    }
};

namespace hpx::threads::detail {

    // Replace stack_size with the stack size of f if f has one. The stack size
    // of a function wins over the one of a launch policy, an executor or
    // thread_init_data, which a function without a stack size keeps. f is
    // read before it is moved into the thread that runs it, as for
    // set_task_size.
    HPX_CXX_CORE_EXPORT template <typename F>
    constexpr void set_stack_size(
        thread_stacksize& stack_size, F const& f) noexcept
    {
        if (thread_stacksize const s = traits::get_stack_size<F>::call(f);
            s != thread_stacksize::unknown)
        {
            stack_size = s;
        }
    }
}    // namespace hpx::threads::detail

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
// The wrapped function keeps its address and annotation.
template <typename F, hpx::threads::thread_stacksize StackSize>
struct hpx::traits::get_function_address<
    hpx::threads::stack_sized_function<F, StackSize>>
{
    static constexpr std::size_t call(
        hpx::threads::stack_sized_function<F, StackSize> const& f) noexcept
    {
        return get_function_address<F>::call(f.f);
    }
};

template <typename F, hpx::threads::thread_stacksize StackSize>
struct hpx::traits::get_function_annotation<
    hpx::threads::stack_sized_function<F, StackSize>>
{
    static constexpr char const* call(
        hpx::threads::stack_sized_function<F, StackSize> const& f) noexcept
    {
        return get_function_annotation<F>::call(f.f);
    }
};

template <typename F, hpx::threads::thread_stacksize StackSize>
struct hpx::traits::get_function_annotation_tracing<
    hpx::threads::stack_sized_function<F, StackSize>>
{
    static hpx::tracing::annotation_handle call(
        hpx::threads::stack_sized_function<F, StackSize> const& f) noexcept
    {
        return get_function_annotation_tracing<F>::call(f.f);
    }
};
#endif
