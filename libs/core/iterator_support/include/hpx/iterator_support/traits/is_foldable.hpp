//  Copyright (c) 2026 Hartmut Kaiser
//  Copyright (c) 2026 Mamidi Surya Teja
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>


#include <concepts>
#include <iterator>
#include <type_traits>

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    template <typename F, typename T, typename I, typename U>
    concept is_indirect_binary_left_foldable_impl =
        std::movable<T> && std::movable<U> && std::convertible_to<T, U> &&
        std::invocable<F&, U, std::iter_reference_t<I>> &&
        std::assignable_from<U&,
            std::invoke_result_t<F&, U, std::iter_reference_t<I>>>;

    template <typename F, typename T, typename I>
    concept is_indirectly_binary_left_foldable =
        std::copy_constructible<F> &&
        std::indirectly_readable<I> &&
        std::invocable<F&, T, std::iter_reference_t<I>> &&
        std::convertible_to<
            std::invoke_result_t<F&, T, std::iter_reference_t<I>>,
            std::decay_t<
                std::invoke_result_t<F&, T, std::iter_reference_t<I>>>> &&
        is_indirect_binary_left_foldable_impl<F, T, I,
            std::decay_t<
                std::invoke_result_t<F&, T, std::iter_reference_t<I>>>>;

    ///////////////////////////////////////////////////////////////////////////////
    template <typename F, typename T, typename I, typename U>
    concept is_indirect_binary_right_foldable_impl =
        std::movable<T> && std::movable<U> && std::convertible_to<T, U> &&
        std::invocable<F&, std::iter_reference_t<I>, U> &&
        std::assignable_from<U&,
            std::invoke_result_t<F&, std::iter_reference_t<I>, U>>;

    template <typename F, typename T, typename I>
    concept is_indirectly_binary_right_foldable =
        std::copy_constructible<F> &&
        std::indirectly_readable<I> &&
        std::invocable<F&, std::iter_reference_t<I>, T> &&
        std::convertible_to<
            std::invoke_result_t<F&, std::iter_reference_t<I>, T>,
            std::decay_t<
                std::invoke_result_t<F&, std::iter_reference_t<I>, T>>> &&
        is_indirect_binary_right_foldable_impl<F, T, I,
            std::decay_t<
                std::invoke_result_t<F&, std::iter_reference_t<I>, T>>>;
}    // namespace hpx
