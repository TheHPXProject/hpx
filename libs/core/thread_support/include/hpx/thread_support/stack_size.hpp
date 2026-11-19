//  Copyright (c) 2026 K S Nithurshen
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/thread_support/thread_enums.hpp>

#include <type_traits>

namespace hpx::traits {

    ///////////////////////////////////////////////////////////////////////////
    /// \brief The stack size the thread that runs a function of type \a F
    ///        needs.
    ///
    /// By default, it is not known which stack the function needs, and
    /// \a Default is reported. \a Default stays unknown for callers; it is an
    /// extension point for specializations, which pass it on to the function
    /// they wrap.
    ///
    /// \a value is the stack size known from the type of \a F. \a call
    /// returns the stack size of a function object, which specializations for
    /// wrappers forward to the function they wrap, the same way as the ones
    /// of \a get_task_size do.
    ///
    /// A qualified type has the stack size of the unqualified one. As for
    /// \a get_task_size, a function or wrapper specializes the trait for its
    /// unqualified type only; a specialization that also matches qualified
    /// types would be ambiguous with the one that forwards them.
    ///
    /// \tparam F       The type of the function.
    /// \tparam Default The stack size reported for a function that has none.
    /// \tparam Enable  Allows specializations to be constrained.
    HPX_CXX_CORE_EXPORT template <typename F,
        threads::thread_stacksize Default = threads::thread_stacksize::unknown,
        typename Enable = void>
    struct get_stack_size
    {
        static constexpr threads::thread_stacksize value = Default;

        [[nodiscard]] static constexpr threads::thread_stacksize call(
            F const& /*f*/) noexcept
        {
            return value;
        }
    };

    // A qualified function has the stack size of the unqualified one.
    template <typename F, threads::thread_stacksize Default>
        requires(!std::is_same_v<F, std::remove_cvref_t<F>>)
    struct get_stack_size<F, Default>
      : get_stack_size<std::remove_cvref_t<F>, Default>
    {
    };

    /// \brief The stack size known from the type \a F, see \a get_stack_size.
    ///
    /// \tparam F       The type of the function.
    /// \tparam Default The stack size reported for a function that has none.
    HPX_CXX_CORE_EXPORT template <typename F,
        threads::thread_stacksize Default = threads::thread_stacksize::unknown>
    inline constexpr threads::thread_stacksize get_stack_size_v =
        get_stack_size<F, Default>::value;
}    // namespace hpx::traits
