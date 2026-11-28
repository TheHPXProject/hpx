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
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/tracing.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ratio>
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

    ///////////////////////////////////////////////////////////////////////////
    /// A function whose calls each run for about a given time: the size
    /// that time falls into (see task_size_of_duration) and the time
    /// itself, which task_size_chunk_size chunks a loop of such a body by. For
    /// a loop body the time is that of one element of the loop.
    HPX_CXX_CORE_EXPORT template <typename F>
    struct timed_function
    {
        F f;
        task_size_and_time timing;

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

    /// \brief Returns a function whose calls each run for about the given
    ///        time, for instance a loop body that adds two doubles in about
    ///        a quarter of a nanosecond.
    ///
    /// \param f    Function to wrap
    /// \param time The run time of one call of \a f. Times are kept in
    ///             picoseconds, saturated at the largest number of them
    ///             (about 106 days); a time of zero or less gives no size.
    HPX_CXX_CORE_EXPORT template <typename F, typename Rep, typename Period>
    constexpr timed_function<std::decay_t<F>> timed_task(
        F&& f, std::chrono::duration<Rep, Period> const& time)
    {
        using input = std::chrono::duration<Rep, Period>;
        if (time <= input::zero())
        {
            return {HPX_FORWARD(F, f),
                task_size_and_time{
                    task_size::unknown, hpx::chrono::picoseconds(0)}};
        }

        // compared in the given unit, with a representation that holds
        // both: converting a longer time to picoseconds would overflow, and
        // converting the largest number of picoseconds to a narrower
        // representation (std::int32_t) would wrap
        using wide = std::chrono::duration<
            std::common_type_t<Rep, hpx::chrono::picoseconds::rep>, Period>;
        hpx::chrono::picoseconds ps = (hpx::chrono::picoseconds::max) ();
        if constexpr (std::is_floating_point_v<Rep>)
        {
            // in picoseconds of the given representation, compared strictly:
            // the largest number of picoseconds rounds up to 2^63 there
            Rep const count = std::chrono::duration_cast<
                std::chrono::duration<Rep, std::pico>>(time)
                                  .count();
            if (count <
                static_cast<Rep>((hpx::chrono::picoseconds::max) ().count()))
            {
                ps = hpx::chrono::picoseconds(
                    static_cast<hpx::chrono::picoseconds::rep>(count));
            }
        }
        else if constexpr (std::ratio_less_v<Period, std::pico>)
        {
            ps = std::chrono::duration_cast<hpx::chrono::picoseconds>(time);
        }
        else if (time <= std::chrono::duration_cast<wide>(
                             (hpx::chrono::picoseconds::max) ()))
        {
            ps = std::chrono::duration_cast<hpx::chrono::picoseconds>(time);
        }
        task_size const size = task_size_of_duration(ps);
        return {HPX_FORWARD(F, f),
            task_size_and_time{size,
                size == task_size::unknown ? hpx::chrono::picoseconds(0) : ps}};
    }
}    // namespace hpx::threads

namespace hpx {

    HPX_CXX_CORE_EXPORT using threads::large_task;
    HPX_CXX_CORE_EXPORT using threads::medium_task;
    HPX_CXX_CORE_EXPORT using threads::nano_task;
    HPX_CXX_CORE_EXPORT using threads::small_task;
    HPX_CXX_CORE_EXPORT using threads::timed_task;
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

// The wrapped function keeps its stack size.
template <typename F, hpx::threads::task_size Size,
    hpx::threads::thread_stacksize Default>
struct hpx::traits::get_stack_size<hpx::threads::sized_function<F, Size>,
    Default> : get_stack_size<F, Default>
{
    [[nodiscard]] static constexpr threads::thread_stacksize call(
        hpx::threads::sized_function<F, Size> const& f) noexcept
    {
        return get_stack_size<F, Default>::call(f.f);
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

        // the run time of a call of the other function, zero if it says
        // nothing finer than its size (see get_task_time)
        hpx::chrono::picoseconds time;

        // the bytes a call of the other function reads and writes, zero if
        // its time doesn't follow from them (see get_task_bytes)
        std::size_t bytes;

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

    // Returns f with the size of sized, called with arguments of the types
    // Args if they are given (see traits::get_task_size_for). The size is
    // read before f is moved, so sized may be a part of f.
    HPX_CXX_CORE_EXPORT template <typename... Args, typename F, typename Sized>
    constexpr size_of_function<std::decay_t<F>> with_task_size_of(
        F&& f, Sized const& sized)
    {
        task_size const size =
            traits::get_task_size_for<Sized, Args...>::call(sized);
        hpx::chrono::picoseconds const time =
            traits::get_task_time<Sized>::call(sized);
        std::size_t const bytes = traits::get_task_bytes<Sized>::call(sized);
        return {HPX_FORWARD(F, f), size, time, bytes};
    }

    // Returns f with the given size, for instance a size read from a
    // function before it is moved into f.
    HPX_CXX_CORE_EXPORT template <typename F>
    constexpr size_of_function<std::decay_t<F>> with_task_size(
        F&& f, task_size const size)
    {
        return {HPX_FORWARD(F, f), size, hpx::chrono::picoseconds(0), 0};
    }

    // Returns f with the given size and run time of a call,
    // for instance read from a function before it is moved into f.
    HPX_CXX_CORE_EXPORT template <typename F>
    constexpr size_of_function<std::decay_t<F>> with_task_size(
        F&& f, task_size_and_time const size)
    {
        return {HPX_FORWARD(F, f), size.size, size.time, size.bytes};
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

template <typename F>
struct hpx::traits::get_task_time<hpx::threads::detail::size_of_function<F>>
{
    [[nodiscard]] static constexpr hpx::chrono::picoseconds call(
        hpx::threads::detail::size_of_function<F> const& f) noexcept
    {
        return f.time.count() != 0 ? f.time : get_task_time<F>::call(f.f);
    }
};

template <typename F>
struct hpx::traits::get_task_bytes<hpx::threads::detail::size_of_function<F>>
{
    [[nodiscard]] static constexpr std::size_t call(
        hpx::threads::detail::size_of_function<F> const& f) noexcept
    {
        return f.bytes != 0 ? f.bytes : get_task_bytes<F>::call(f.f);
    }
};

// The size that stands for the time given to the wrapper wins over the
// default, as does the time itself.
template <typename F, hpx::threads::task_size Default>
struct hpx::traits::get_task_size<hpx::threads::timed_function<F>, Default>
{
    static constexpr threads::task_size value = Default;

    [[nodiscard]] static constexpr threads::task_size call(
        hpx::threads::timed_function<F> const& f) noexcept
    {
        return f.timing.size != threads::task_size::unknown ? f.timing.size :
                                                              Default;
    }
};

template <typename F>
struct hpx::traits::get_task_time<hpx::threads::timed_function<F>>
{
    [[nodiscard]] static constexpr hpx::chrono::picoseconds call(
        hpx::threads::timed_function<F> const& f) noexcept
    {
        return f.timing.time;
    }
};

// The wrapped function keeps its stack size.
template <typename F, hpx::threads::thread_stacksize Default>
struct hpx::traits::get_stack_size<hpx::threads::timed_function<F>, Default>
  : get_stack_size<F, Default>
{
    [[nodiscard]] static constexpr threads::thread_stacksize call(
        hpx::threads::timed_function<F> const& f) noexcept
    {
        return get_stack_size<F, Default>::call(f.f);
    }
};

// The function keeps its own stack size, it does not take the one of the
// function it has the task size of.
template <typename F, hpx::threads::thread_stacksize Default>
struct hpx::traits::get_stack_size<hpx::threads::detail::size_of_function<F>,
    Default> : get_stack_size<F, Default>
{
    [[nodiscard]] static constexpr threads::thread_stacksize call(
        hpx::threads::detail::size_of_function<F> const& f) noexcept
    {
        return get_stack_size<F, Default>::call(f.f);
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
struct hpx::traits::get_function_address<hpx::threads::timed_function<F>>
{
    static constexpr std::size_t call(
        hpx::threads::timed_function<F> const& f) noexcept
    {
        return get_function_address<F>::call(f.f);
    }
};

template <typename F>
struct hpx::traits::get_function_annotation<hpx::threads::timed_function<F>>
{
    static constexpr char const* call(
        hpx::threads::timed_function<F> const& f) noexcept
    {
        return get_function_annotation<F>::call(f.f);
    }
};

template <typename F>
struct hpx::traits::get_function_annotation_tracing<
    hpx::threads::timed_function<F>>
{
    static hpx::tracing::annotation_handle call(
        hpx::threads::timed_function<F> const& f) noexcept
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
