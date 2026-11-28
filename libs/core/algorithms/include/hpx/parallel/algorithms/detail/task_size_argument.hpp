//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/util/detail/transfer_task_size.hpp>

#include <cstddef>
#include <iterator>
#include <ratio>
#include <type_traits>

namespace hpx::parallel::detail {

    // The type of an argument whose size is not known, for an element that
    // is passed through a projection, which may do any amount of work (see
    // traits::get_task_size_for).
    HPX_CXX_CORE_EXPORT struct unknown_task_size_argument
    {
    };

    // The type of the argument an algorithm calls its function with for an
    // element of Iter that is passed through the projection Proj.
    HPX_CXX_CORE_EXPORT template <typename Iter, typename Proj = hpx::identity>
    using task_size_argument_t =
        std::conditional_t<std::is_same_v<std::decay_t<Proj>, hpx::identity>,
            std::iter_reference_t<Iter>, unknown_task_size_argument>;

    // The size of a call of f with arguments of the types Args, with its run
    // time if f gives one (see hpx::timed_task), to be read before f is moved
    // into the function that calls it.
    HPX_CXX_CORE_EXPORT template <typename... Args, typename F>
    [[nodiscard]] constexpr hpx::threads::task_size_and_time task_size_of_call(
        F const& f) noexcept
    {
        return {hpx::traits::get_task_size_for<F, Args...>::call(f),
            hpx::traits::get_task_time<F>::call(f),
            hpx::traits::get_task_bytes<F>::call(f)};
    }

    // The bytes of an element of Iter, 0 if it isn't of a builtin type or
    // if Iter is void (no element written). A class template, so that
    // std::iter_value_t is named only for the iterators it is defined for.
    HPX_CXX_CORE_EXPORT template <typename Iter, typename Enable = void>
    struct task_size_element_bytes_of : std::integral_constant<std::size_t, 0>
    {
    };

    template <typename Iter>
        requires(!std::is_void_v<Iter> &&
            requires { typename std::iter_value_t<Iter>; } &&
            hpx::traits::detail::builtin_operand<std::iter_value_t<Iter>>)
    struct task_size_element_bytes_of<Iter>
      : std::integral_constant<std::size_t, sizeof(std::iter_value_t<Iter>)>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Iter>
    inline constexpr std::size_t task_size_element_bytes =
        task_size_element_bytes_of<Iter>::value;

    // The size of an iteration that dereferences the iterators its, calls f
    // with arguments of the types Args and, unless OutIter is void, writes
    // the result to an element of OutIter, as task_size_of_iteration
    // describes. A builtin arithmetic operation or comparison on builtin
    // elements (see traits::is_memory_bound_builtin_operation) costs about
    // what its bytes cost to read and write: the iteration gets the run time
    // of the elements of its and of the element it writes (see
    // memory_task_size),
    // times Weight (a std::ratio) for an iteration that calls f more or less
    // than once per element.
    HPX_CXX_CORE_EXPORT template <typename OutIter, typename Weight,
        typename... Args, typename F, typename... Iters>
    [[nodiscard]] hpx::threads::task_size_and_time task_size_of_iteration_with(
        F const& f, Iters const&... its) noexcept
    {
        hpx::threads::task_size_and_time const call =
            task_size_of_call<Args...>(f);
        hpx::threads::task_size size = call.size;
        ((size = hpx::threads::combined_task_size(size,
              hpx::traits::iterator_dereference_task_size<Iters>::call(its))),
            ...);
        if (size != call.size)
        {
            return {size, hpx::chrono::picoseconds(0)};
        }
        if (call.time.count() > 0)
        {
            return call;
        }

        if constexpr (hpx::traits::is_memory_bound_builtin_operation<
                          std::decay_t<F>>::value &&
            sizeof...(Iters) != 0 &&
            ((task_size_element_bytes<Iters> != 0) && ...) &&
            (std::is_void_v<OutIter> || task_size_element_bytes<OutIter> != 0))
        {
            if (size != hpx::threads::task_size::unknown)
            {
                constexpr std::size_t bytes =
                    (task_size_element_bytes<Iters> + ...) +
                    task_size_element_bytes<OutIter>;
                return util::detail::memory_task_size(
                    (bytes * Weight::num + Weight::den - 1) / Weight::den);
            }
        }
        return {size, hpx::chrono::picoseconds(0)};
    }

    // The size of an iteration that calls f once per element.
    HPX_CXX_CORE_EXPORT template <typename OutIter, typename... Args,
        typename F, typename... Iters>
    [[nodiscard]] hpx::threads::task_size_and_time
    task_size_of_iteration_writing(F const& f, Iters const&... its) noexcept
    {
        return task_size_of_iteration_with<OutIter, std::ratio<1>, Args...>(
            f, its...);
    }

    // The size of an iteration that dereferences the iterators its and calls
    // f with arguments of the types Args, to be read before f is moved into
    // the function that calls it. Dereferencing an iterator that computes
    // its element can take any amount of time, which makes the size of the
    // iteration unknown (see traits::iterator_dereference_task_size).
    //
    // Dereferencing an iterator that refers to an existing element takes no
    // longer than the smallest size: an iteration whose size is still that
    // of f keeps the run time of f.
    HPX_CXX_CORE_EXPORT template <typename... Args, typename F,
        typename... Iters>
    [[nodiscard]] hpx::threads::task_size_and_time task_size_of_iteration(
        F const& f, Iters const&... its) noexcept
    {
        return task_size_of_iteration_writing<void, Args...>(f, its...);
    }
}    // namespace hpx::parallel::detail
