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

#include <iterator>
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
            hpx::traits::get_task_time<F>::call(f)};
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
    [[nodiscard]] constexpr hpx::threads::task_size_and_time
    task_size_of_iteration(F const& f, Iters const&... its) noexcept
    {
        hpx::threads::task_size_and_time const call =
            task_size_of_call<Args...>(f);
        hpx::threads::task_size size = call.size;
        ((size = hpx::threads::combined_task_size(size,
              hpx::traits::iterator_dereference_task_size<Iters>::call(its))),
            ...);
        return {
            size, size == call.size ? call.time : hpx::chrono::picoseconds(0)};
    }
}    // namespace hpx::parallel::detail
