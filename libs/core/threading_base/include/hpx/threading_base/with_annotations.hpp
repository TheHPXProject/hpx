//  Copyright (c) 2026 K S Nithurshen
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \page hpx::threads::with_annotations
/// \headerfile hpx/functional.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/threading_base/sized_function.hpp>
#include <hpx/threading_base/stack_sized_function.hpp>

#include <type_traits>
#include <utility>

namespace hpx::threads {

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Returns a function that carries both the given task size and
    ///        the given stack size.
    ///
    /// The result is the same as with_task_size<Size>(
    /// with_stack_size<StackSize>(f)), so it adds no storage, and wrapping it
    /// again replaces only the annotation given: nano_task of it changes the
    /// task size only, no_stack of it the stack size only.
    ///
    /// \tparam Size      The expected run time of the task that runs \a f
    /// \tparam StackSize The stack size the thread that runs \a f needs, one
    ///                   of small_, medium, large, huge or nostack
    /// \param f Function to wrap
    HPX_CXX_CORE_EXPORT template <task_size Size, thread_stacksize StackSize,
        typename F>
    constexpr sized_function<stack_sized_function<std::decay_t<F>, StackSize>,
        Size>
    with_annotations(F&& f)
    {
        return with_task_size<Size>(
            with_stack_size<StackSize>(HPX_FORWARD(F, f)));
    }
}    // namespace hpx::threads

namespace hpx {

    HPX_CXX_CORE_EXPORT using threads::with_annotations;
}    // namespace hpx
