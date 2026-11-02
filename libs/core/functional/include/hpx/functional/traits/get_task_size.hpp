//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <type_traits>

namespace hpx::threads {

    ///////////////////////////////////////////////////////////////////////////
    /// \enum task_size
    ///
    /// The expected run time of a task, a hint the scheduler may use. A task
    /// with an unknown size is scheduled as it would be without the hint. The
    /// sizes are ordered by run time, see task_size_duration.
    HPX_CXX_CORE_EXPORT enum class task_size : std::uint8_t {
        unknown = 0,
        nano = 1,      ///< about a nanosecond, mostly for loop bodies
        small_ = 2,    ///< the underscore is to work around 'small' being
                       ///  defined to char on Windows
        medium = 3,
        large = 4
    };

    /// The run time a task of the given \p size stands for. Each size stands
    /// for a hundred times the run time of the one below it: nano about 1 ns,
    /// small about 100 ns, medium about 10 us, large about 1 ms or more.
    /// An unknown size gives a duration of zero. Callers should rely on
    /// std::chrono arithmetic only, not on the unit or the count of the
    /// returned duration, which may change once tasks can carry real run
    /// times.
    HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr std::chrono::nanoseconds
    task_size_duration(task_size const size) noexcept
    {
        using namespace std::chrono_literals;
        switch (size)
        {
        case task_size::nano:
            return 1ns;
        case task_size::small_:
            return 100ns;
        case task_size::medium:
            return 10us;
        case task_size::large:
            return 1ms;
        default:
            return 0ns;
        }
    }

    /// The size of a task that calls a function of the given \p size \p calls
    /// times, for instance a hundred nano calls make a small task, a hundred
    /// small calls a medium one and a hundred medium calls a large one. The
    /// size of a task calling a function of unknown size is unknown.
    HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr task_size bulk_task_size(
        task_size const size, std::size_t const calls) noexcept
    {
        if (size == task_size::large)
            return task_size::large;

        auto const per_call = task_size_duration(size);
        if (per_call == per_call.zero())
            return task_size::unknown;

        // the run time of all calls, limited to avoid overflow
        auto const limit = task_size_duration(task_size::large);
        auto const max_calls = limit / per_call;
        auto const total = calls > static_cast<std::size_t>(max_calls) ?
            limit :
            per_call * static_cast<std::chrono::nanoseconds::rep>(calls);

        for (task_size const s :
            {task_size::large, task_size::medium, task_size::small_})
        {
            if (total >= task_size_duration(s))
                return s;
        }
        return task_size::nano;
    }
}    // namespace hpx::threads

namespace hpx {

    HPX_CXX_CORE_EXPORT using threads::task_size;
    HPX_CXX_CORE_EXPORT using threads::task_size_duration;
    HPX_CXX_CORE_EXPORT using threads::bulk_task_size;
}    // namespace hpx

namespace hpx::traits {

    // By default, we don't know how long the function runs. Size stays
    // unknown for callers; it is an extension point for specializations,
    // which pass it on to the function they wrap.
    //
    // value is the size known from the type of F. call(f) returns the size of
    // the function object f, which can be known only at run time, for
    // instance for a function stored in an hpx::function. Specializations for
    // wrappers forward call to the function they wrap, the same way as the
    // ones of get_function_annotation do.
    HPX_CXX_CORE_EXPORT template <typename F,
        threads::task_size Size = threads::task_size::unknown,
        typename Enable = void>
    struct get_task_size
    {
        static constexpr threads::task_size value = Size;

        [[nodiscard]] static constexpr threads::task_size call(
            F const& /*f*/) noexcept
        {
            return Size;
        }
    };

    // A qualified function has the size of the unqualified one.
    template <typename F, threads::task_size Size>
        requires(!std::is_same_v<F, std::remove_cvref_t<F>>)
    struct get_task_size<F, Size> : get_task_size<std::remove_cvref_t<F>, Size>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename F,
        threads::task_size Size = threads::task_size::unknown>
    inline constexpr threads::task_size get_task_size_v =
        get_task_size<F, Size>::value;

    // The size of a call of f with arguments of the types Args. Unless it is
    // specialized, it is the size of f itself (get_task_size), so that a size
    // given to f (nano_task(f), with_task_size<Size>(f), ...) always wins.
    HPX_CXX_CORE_EXPORT template <typename F, typename... Args>
    struct get_task_size_for
    {
        [[nodiscard]] static constexpr threads::task_size call(
            F const& f) noexcept
        {
            return get_task_size<F>::call(f);
        }
    };

    // A qualified function has the size of the unqualified one.
    template <typename F, typename... Args>
        requires(!std::is_same_v<F, std::remove_cvref_t<F>>)
    struct get_task_size_for<F, Args...>
      : get_task_size_for<std::remove_cvref_t<F>, Args...>
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    // Function objects of a builtin operation, which runs for about a
    // nanosecond on builtin types.
    //
    // A transparent function object, which doesn't name the type of its
    // operands (for instance std::less<>), specializes
    // is_transparent_builtin_operation, its size depends on the arguments it
    // is called with.
    HPX_CXX_CORE_EXPORT template <typename F>
    struct is_transparent_builtin_operation : std::false_type
    {
    };

    // A typed function object, which names the type of its operands (for
    // instance std::less<int>), specializes typed_builtin_operation_operand,
    // its size depends on that type.
    HPX_CXX_CORE_EXPORT template <typename F>
    struct typed_builtin_operation_operand
    {
    };

    namespace detail {

        // The types builtin operations work on.
        HPX_CXX_CORE_EXPORT template <typename T>
        concept builtin_operand = std::is_arithmetic_v<std::decay_t<T>> ||
            std::is_enum_v<std::decay_t<T>> ||
            std::is_pointer_v<std::decay_t<T>>;

        HPX_CXX_CORE_EXPORT template <typename F>
        concept typed_builtin_operation = requires {
            typename typed_builtin_operation_operand<F>::type;
        } && builtin_operand<typename typed_builtin_operation_operand<F>::type>;
    }    // namespace detail

    // A builtin operation on a builtin type is a nano task.
    template <typename F, threads::task_size Size>
        requires(detail::typed_builtin_operation<F>)
    struct get_task_size<F, Size>
    {
        static constexpr threads::task_size value = threads::task_size::nano;

        [[nodiscard]] static constexpr threads::task_size call(
            F const&) noexcept
        {
            return value;
        }
    };

    // A builtin operation called with builtin types is a nano task. Without
    // arguments, the size stays unknown.
    template <typename F, typename... Args>
        requires(
            is_transparent_builtin_operation<F>::value && sizeof...(Args) != 0)
    struct get_task_size_for<F, Args...>
    {
        [[nodiscard]] static constexpr threads::task_size call(
            F const&) noexcept
        {
            return (detail::builtin_operand<Args> && ...) ?
                threads::task_size::nano :
                threads::task_size::unknown;
        }
    };

    // The function objects of the standard library.
#define HPX_TASK_SIZE_BUILTIN_OPERATION(op)                                    \
    template <>                                                                \
    struct is_transparent_builtin_operation<std::op<>> : std::true_type        \
    {                                                                          \
    };                                                                         \
    template <typename T>                                                      \
        requires(!std::is_void_v<T>)                                           \
    struct typed_builtin_operation_operand<std::op<T>>                         \
    {                                                                          \
        using type = T;                                                        \
    } /**/

    HPX_TASK_SIZE_BUILTIN_OPERATION(plus);
    HPX_TASK_SIZE_BUILTIN_OPERATION(minus);
    HPX_TASK_SIZE_BUILTIN_OPERATION(multiplies);
    HPX_TASK_SIZE_BUILTIN_OPERATION(divides);
    HPX_TASK_SIZE_BUILTIN_OPERATION(modulus);
    HPX_TASK_SIZE_BUILTIN_OPERATION(negate);
    HPX_TASK_SIZE_BUILTIN_OPERATION(equal_to);
    HPX_TASK_SIZE_BUILTIN_OPERATION(not_equal_to);
    HPX_TASK_SIZE_BUILTIN_OPERATION(less);
    HPX_TASK_SIZE_BUILTIN_OPERATION(greater);
    HPX_TASK_SIZE_BUILTIN_OPERATION(less_equal);
    HPX_TASK_SIZE_BUILTIN_OPERATION(greater_equal);
    HPX_TASK_SIZE_BUILTIN_OPERATION(logical_and);
    HPX_TASK_SIZE_BUILTIN_OPERATION(logical_or);
    HPX_TASK_SIZE_BUILTIN_OPERATION(logical_not);
    HPX_TASK_SIZE_BUILTIN_OPERATION(bit_and);
    HPX_TASK_SIZE_BUILTIN_OPERATION(bit_or);
    HPX_TASK_SIZE_BUILTIN_OPERATION(bit_xor);
    HPX_TASK_SIZE_BUILTIN_OPERATION(bit_not);

#undef HPX_TASK_SIZE_BUILTIN_OPERATION

    // std::ranges has transparent comparisons only.
#define HPX_TASK_SIZE_BUILTIN_RANGES_OPERATION(op)                             \
    template <>                                                                \
    struct is_transparent_builtin_operation<std::ranges::op> : std::true_type  \
    {                                                                          \
    } /**/

    HPX_TASK_SIZE_BUILTIN_RANGES_OPERATION(equal_to);
    HPX_TASK_SIZE_BUILTIN_RANGES_OPERATION(not_equal_to);
    HPX_TASK_SIZE_BUILTIN_RANGES_OPERATION(less);
    HPX_TASK_SIZE_BUILTIN_RANGES_OPERATION(greater);
    HPX_TASK_SIZE_BUILTIN_RANGES_OPERATION(less_equal);
    HPX_TASK_SIZE_BUILTIN_RANGES_OPERATION(greater_equal);

#undef HPX_TASK_SIZE_BUILTIN_RANGES_OPERATION
}    // namespace hpx::traits
