//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
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

    // The size of a call of f with arguments of the types Args, to be read
    // before f is moved into the function that calls it.
    HPX_CXX_CORE_EXPORT template <typename... Args, typename F>
    [[nodiscard]] constexpr hpx::threads::task_size task_size_of_call(
        F const& f) noexcept
    {
        return hpx::traits::get_task_size_for<F, Args...>::call(f);
    }
}    // namespace hpx::parallel::detail
