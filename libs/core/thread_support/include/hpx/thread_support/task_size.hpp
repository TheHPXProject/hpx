//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/timing.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <type_traits>

#if defined(HPX_HAVE_CXX26_REFLECTION) &&                                      \
    defined(HPX_HAVE_CXX26_REFLECTION_ANNOTATIONS)
#include <meta>
#endif

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

    /// The name of the given task \p size, as used in the configuration:
    /// "unknown", "nano", "small", "medium" or "large".
    HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr char const* get_task_size_name(
        task_size const size) noexcept
    {
        switch (size)
        {
        case task_size::nano:
            return "nano";
        case task_size::small_:
            return "small";
        case task_size::medium:
            return "medium";
        case task_size::large:
            return "large";
        default:
            return "unknown";
        }
    }

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

    /// The size of a task that does work of the size \p first and then work of
    /// the size \p second, for instance dereferencing an iterator and calling
    /// a function with the element. It is unknown if either size is unknown,
    /// otherwise it is the larger one: the sizes are a hundred times apart, so
    /// the smaller one hardly adds to the larger one.
    HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr task_size combined_task_size(
        task_size const first, task_size const second) noexcept
    {
        if (first == task_size::unknown || second == task_size::unknown)
            return task_size::unknown;
        return first < second ? second : first;
    }

    /// The size of a function together with the run time of one of its
    /// calls, for a function whose run time is known more precisely than its
    /// size (see hpx::timed_task). time is zero if the function says nothing
    /// finer than its size.
    HPX_CXX_CORE_EXPORT struct task_size_and_time
    {
        task_size size = task_size::unknown;
        hpx::chrono::picoseconds time = hpx::chrono::picoseconds(0);

        // where only the size is needed
        constexpr operator task_size() const noexcept
        {
            return size;
        }
    };

    /// The size of a function whose calls each run for about \p time: the
    /// largest size whose run time it reaches, as for bulk_task_size, and
    /// nano below a nanosecond (a call of 0.25 ns or of 30 ns is a nano call,
    /// one of 100 ns a small one). A time of zero or less has no size.
    HPX_CXX_CORE_EXPORT [[nodiscard]] constexpr task_size task_size_of_duration(
        hpx::chrono::picoseconds const time) noexcept
    {
        if (time.count() <= 0)
        {
            return task_size::unknown;
        }
        task_size result = task_size::nano;
        for (task_size const size :
            {task_size::small_, task_size::medium, task_size::large})
        {
            if (time >= hpx::chrono::picoseconds(task_size_duration(size)))
            {
                result = size;
            }
        }
        return result;
    }
}    // namespace hpx::threads

namespace hpx {

    HPX_CXX_CORE_EXPORT using threads::task_size;
    HPX_CXX_CORE_EXPORT using threads::get_task_size_name;
    HPX_CXX_CORE_EXPORT using threads::task_size_duration;
    HPX_CXX_CORE_EXPORT using threads::bulk_task_size;
    HPX_CXX_CORE_EXPORT using threads::combined_task_size;
    HPX_CXX_CORE_EXPORT using threads::task_size_and_time;
    HPX_CXX_CORE_EXPORT using threads::task_size_of_duration;
}    // namespace hpx

namespace hpx::traits {

#if defined(HPX_HAVE_CXX26_REFLECTION) &&                                      \
    defined(HPX_HAVE_CXX26_REFLECTION_ANNOTATIONS)
    namespace detail {

        HPX_CXX_CORE_EXPORT struct task_size_annotation
        {
            bool found = false;
            bool conflicting = false;
            threads::task_size size = threads::task_size::unknown;
        };

        // Only functions and class types are looked at. Annotations are not
        // inherited, so a derived type has to be annotated itself and the base
        // classes are not scanned. An unknown size counts as no annotation.
        HPX_CXX_CORE_EXPORT consteval task_size_annotation
        find_task_size_annotation(std::meta::info const entity)
        {
            task_size_annotation result;
            if (!std::meta::is_function(entity) &&
                !std::meta::is_class_type(entity))
            {
                return result;
            }

            for (std::meta::info const a : std::meta::annotations_of(entity))
            {
                if (std::meta::remove_const(std::meta::type_of(a)) !=
                    ^^threads::task_size)
                {
                    continue;
                }

                auto const size = std::meta::extract<threads::task_size>(a);
                if (size == threads::task_size::unknown)
                {
                    continue;
                }

                if (result.found && size != result.size)
                    result.conflicting = true;
                result.found = true;
                result.size = size;
            }
            return result;
        }

        // An alias does not have the annotations of the type it stands for, and
        // finding none is not an error: the type would silently keep the
        // default size. dealias makes sure that the type itself is looked at.
        HPX_CXX_CORE_EXPORT template <typename F, threads::task_size Size>
        consteval threads::task_size annotated_task_size()
        {
            constexpr task_size_annotation annotation =
                find_task_size_annotation(std::meta::dealias(^^F));

            static_assert(!annotation.conflicting,
                "a type must not have two different hpx::task_size "
                "annotations");

            return annotation.found ? annotation.size : Size;
        }

        // The size a function is annotated with, unknown if it has none. Fn
        // reflects the function rather than its type, because functions of
        // the same type can have different sizes.
        HPX_CXX_CORE_EXPORT template <std::meta::info Fn>
        consteval threads::task_size annotated_function_task_size()
        {
            constexpr task_size_annotation annotation =
                find_task_size_annotation(Fn);

            static_assert(!annotation.conflicting,
                "a function must not have two different hpx::task_size "
                "annotations");

            return annotation.size;
        }
    }    // namespace detail
#endif

    // By default, we don't know how long the function runs. Size stays
    // unknown for callers; it is an extension point for specializations,
    // which pass it on to the function they wrap. A class type can give its
    // size with an annotation if the compiler supports them, which wins over
    // Size but not over a specialization.
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
#if defined(HPX_HAVE_CXX26_REFLECTION) &&                                      \
    defined(HPX_HAVE_CXX26_REFLECTION_ANNOTATIONS)
        static constexpr threads::task_size value =
            detail::annotated_task_size<F, Size>();
#else
        static constexpr threads::task_size value = Size;
#endif

        [[nodiscard]] static constexpr threads::task_size call(
            F const& /*f*/) noexcept
        {
            return value;
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

    // The run time of one call of F, for a function whose run time was given
    // as a time (see hpx::timed_task), zero for a function that says nothing
    // finer than its size. For a loop body it is the time of one element of
    // the loop, whatever the execution policy. Wrappers forward it as they
    // forward get_task_size. task_size_chunk_size chunks a loop by it (see
    // measure_iteration).
    HPX_CXX_CORE_EXPORT template <typename F, typename Enable = void>
    struct get_task_time
    {
        [[nodiscard]] static constexpr hpx::chrono::picoseconds call(
            F const&) noexcept
        {
            return hpx::chrono::picoseconds(0);
        }
    };

    // A qualified function has the time of the unqualified one.
    template <typename F>
        requires(!std::is_same_v<F, std::remove_cvref_t<F>>)
    struct get_task_time<F> : get_task_time<std::remove_cvref_t<F>>
    {
    };

#if defined(HPX_HAVE_CXX26_REFLECTION) &&                                      \
    defined(HPX_HAVE_CXX26_REFLECTION_ANNOTATIONS)
    namespace detail {

        // A function pointer given as a template argument leads back to the
        // function it points to, whose declaration can carry an annotation.
        HPX_CXX_CORE_EXPORT template <auto F>
        consteval threads::task_size annotated_function_pointer_task_size()
        {
            if constexpr (std::is_pointer_v<decltype(F)> &&
                std::is_function_v<std::remove_pointer_t<decltype(F)>>)
            {
                return annotated_function_task_size<std::meta::reflect_function(
                    *F)>();
            }
            else
            {
                return threads::task_size::unknown;
            }
        }
    }    // namespace detail
#endif

    // The size of a local call of the function F points to, for a function
    // given as a template argument to hpx::post<&f> or hpx::async<&f>. A
    // specialization wins over an annotation of the function, as
    // HPX_ACTION_HAS_TASK_SIZE does for actions.
    HPX_CXX_CORE_EXPORT template <auto F>
    struct task_size_of
    {
#if defined(HPX_HAVE_CXX26_REFLECTION) &&                                      \
    defined(HPX_HAVE_CXX26_REFLECTION_ANNOTATIONS)
        static constexpr threads::task_size value =
            detail::annotated_function_pointer_task_size<F>();
#else
        static constexpr threads::task_size value = threads::task_size::unknown;
#endif
    };

    HPX_CXX_CORE_EXPORT template <auto F>
    inline constexpr threads::task_size task_size_of_v = task_size_of<F>::value;

    // The size of a call of f with arguments of the types Args. Unless it is
    // specialized, it is the size of f itself (get_task_size), so that a size
    // given to f (nano_task(f), with_task_size<Size>(f), ...) always wins.
    //
    // As for get_task_size, value is the size known from the types, call(f)
    // the size for the function object f.
    HPX_CXX_CORE_EXPORT template <typename F, typename... Args>
    struct get_task_size_for
    {
        static constexpr threads::task_size value = get_task_size<F>::value;

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
    // Their size depends on the types they are called with, they are sized
    // only for a call with arguments (get_task_size_for). A transparent
    // function object, which doesn't name the type of its operands (for
    // instance std::less<>), specializes is_transparent_builtin_operation.
    // A typed one (for instance std::less<int>) specializes
    // is_typed_builtin_operation if the type it names is builtin, it then
    // converts builtin arguments only, which is as fast as the operation.
    //
    // The work that produces the arguments is not part of the call: an
    // algorithm adds the size of dereferencing its iterators (see
    // hpx::traits::iterator_dereference_task_size).
    HPX_CXX_CORE_EXPORT template <typename F>
    struct is_transparent_builtin_operation : std::false_type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename F>
    struct is_typed_builtin_operation : std::false_type
    {
    };

    namespace detail {

        // The types builtin operations work on.
        HPX_CXX_CORE_EXPORT template <typename T>
        concept builtin_operand = std::is_arithmetic_v<std::decay_t<T>> ||
            std::is_enum_v<std::decay_t<T>> ||
            std::is_pointer_v<std::decay_t<T>>;
    }    // namespace detail

    // A builtin operation called with builtin types is a nano task. Without
    // arguments, the size stays unknown.
    template <typename F, typename... Args>
        requires((is_transparent_builtin_operation<F>::value ||
                     is_typed_builtin_operation<F>::value) &&
            sizeof...(Args) != 0)
    struct get_task_size_for<F, Args...>
    {
        static constexpr threads::task_size value =
            (detail::builtin_operand<Args> && ...) ?
            threads::task_size::nano :
            threads::task_size::unknown;

        [[nodiscard]] static constexpr threads::task_size call(
            F const&) noexcept
        {
            return value;
        }
    };

    // The function objects of the standard library: std::op<> is the
    // transparent one, std::op<T> a typed one (std::op<> is std::op<void>,
    // which is not builtin).
#define HPX_TASK_SIZE_BUILTIN_OPERATION(op)                                    \
    template <>                                                                \
    struct is_transparent_builtin_operation<std::op<>> : std::true_type        \
    {                                                                          \
    };                                                                         \
                                                                               \
    template <typename T>                                                      \
    struct is_typed_builtin_operation<std::op<T>>                              \
      : std::bool_constant<detail::builtin_operand<T>>                         \
    {                                                                          \
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
