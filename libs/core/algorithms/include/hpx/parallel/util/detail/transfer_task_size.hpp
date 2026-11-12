//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/pointer_category.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/parallel/util/detail/elements_per_task_size.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>

namespace hpx::parallel::util::detail {

    // The size of an operation on one element of each of the iterator types
    // Iters that calls no user function, for instance an assignment of a
    // value or a swap: about a nanosecond for elements of builtin types, not
    // known for others.
    HPX_CXX_CORE_EXPORT template <typename... Iters>
    [[nodiscard]] constexpr hpx::threads::task_size
    element_operation_size() noexcept
    {
        return (hpx::traits::detail::builtin_operand<
                    std::iter_value_t<Iters>> &&
                   ...) ?
            hpx::threads::task_size::nano :
            hpx::threads::task_size::unknown;
    }

    // The task size of copying or moving an element from InIter to OutIter
    // with the pointer category Category (see util::copy_n and util::move_n),
    // and the number of elements a copy handles in the time of that size.
    HPX_CXX_CORE_EXPORT struct transfer_task_size
    {
        hpx::threads::task_size size;
        std::size_t elements;
    };

    // Trivially copyable elements are copied with memmove, at about
    // copy_bytes_per_ns bytes in a nanosecond (see
    // task_size_chunk_size_defaults), so a nano task copies that many bytes.
    // Other elements of builtin types are copied in about a nanosecond each,
    // the size of copying others is not known.
    HPX_CXX_CORE_EXPORT template <typename Category, typename InIter,
        typename OutIter>
    [[nodiscard]] transfer_task_size get_transfer_task_size() noexcept
    {
        if constexpr (std::is_same_v<Category,
                          hpx::traits::trivially_copyable_pointer_tag>)
        {
            using value_type = std::iter_value_t<InIter>;
            std::size_t const bytes = hpx::execution::experimental::detail::
                                          get_task_size_chunk_size_defaults()
                                              .copy_bytes_per_ns;
            if (sizeof(value_type) <= bytes)
            {
                return {
                    hpx::threads::task_size::nano, bytes / sizeof(value_type)};
            }
            return {hpx::threads::bulk_task_size(hpx::threads::task_size::nano,
                        (sizeof(value_type) + bytes - 1) / bytes),
                1};
        }
        else
        {
            return {element_operation_size<InIter, OutIter>(), 1};
        }
    }

    // The number of elements a call of a loop body that copies or moves
    // elements of the zip iterator Iter handles in the time of its size. A
    // datapar policy copies the elements it puts into packs (see
    // datapar_copy_n), all other elements are copied as for the other
    // policies (see get_transfer_task_size).
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Category,
        typename InIter, typename OutIter, typename Iter>
    [[nodiscard]] std::size_t transfer_elements_per_call(
        hpx::threads::task_size const size) noexcept
    {
        if constexpr (hpx::is_vectorpack_execution_policy_v<ExPolicy>)
        {
            std::size_t const elements =
                policy_elements_per_call<ExPolicy, Iter>::call(size);
            if (elements > 1)
            {
                return elements;
            }
        }
        return get_transfer_task_size<Category, InIter, OutIter>().elements;
    }
}    // namespace hpx::parallel::util::detail
