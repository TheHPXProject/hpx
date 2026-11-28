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

#include <algorithm>
#include <cstddef>
#include <cstdint>
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

    // The run time of an operation on an element that reads and writes the
    // given number of bytes and does little else: copy_bytes_per_ns (see
    // task_size_chunk_size_defaults) is the number of bytes a core copies in
    // a nanosecond, reading and writing each, so an operation that moves
    // twice that many bytes takes a nanosecond. With the default (16 bytes
    // per ns) copying a double takes 0.5 ns, adding two doubles into a third
    // 0.75 ns.
    HPX_CXX_CORE_EXPORT [[nodiscard]] inline hpx::threads::task_size_and_time
    memory_task_size(std::size_t const bytes) noexcept
    {
        std::size_t const rate =
            (std::max) (hpx::execution::experimental::detail::
                            get_task_size_chunk_size_defaults()
                                .copy_bytes_per_ns,
                std::size_t(1));
        hpx::chrono::picoseconds const time(
            static_cast<std::int64_t>((bytes * 1000 + rate) / (2 * rate)));
        return {hpx::threads::task_size_of_duration(time), time, bytes};
    }

    // The size of an operation that reads and writes each element of the
    // iterator types Iters Accesses times and calls no user function, for
    // instance an assignment (once) or a swap (twice, for both elements): the
    // run time of its bytes for elements of builtin types (see
    // memory_task_size), not known for others.
    HPX_CXX_CORE_EXPORT template <std::size_t Accesses, typename... Iters>
    [[nodiscard]] hpx::threads::task_size_and_time
    element_access_size() noexcept
    {
        if constexpr ((hpx::traits::detail::builtin_operand<
                           std::iter_value_t<Iters>> &&
                          ...))
        {
            return memory_task_size(
                Accesses * (sizeof(std::iter_value_t<Iters>) + ...));
        }
        else
        {
            return {
                hpx::threads::task_size::unknown, hpx::chrono::picoseconds(0)};
        }
    }

    // The size of copying or moving an element from InIter to OutIter with
    // the pointer category Category (see util::copy_n and util::move_n):
    // trivially copyable elements are copied with memmove, which reads and
    // writes their bytes (see memory_task_size), other elements of builtin
    // types take a nanosecond each, the size of copying others is not
    // known.
    HPX_CXX_CORE_EXPORT template <typename Category, typename InIter,
        typename OutIter>
    [[nodiscard]] hpx::threads::task_size_and_time
    get_transfer_task_size() noexcept
    {
        if constexpr (std::is_same_v<Category,
                          hpx::traits::trivially_copyable_pointer_tag>)
        {
            return memory_task_size(2 * sizeof(std::iter_value_t<InIter>));
        }
        else
        {
            return {element_operation_size<InIter, OutIter>(),
                hpx::chrono::picoseconds(0)};
        }
    }

    // The number of elements a call of a loop body that copies elements from
    // InIter to OutIter handles in the time of its size, for the elements
    // whose copy has no run time of its own (see get_transfer_task_size): a
    // policy that copies the elements in packs copies a pack in that time
    // (see copy_pack_size), the others one element.
    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Category,
        typename InIter, typename OutIter>
    [[nodiscard]] constexpr std::size_t transfer_elements_per_call() noexcept
    {
        constexpr std::size_t pack_size =
            copy_pack_size<ExPolicy, InIter, OutIter>::value;
        return pack_size > 1 ? pack_size : 1;
    }
}    // namespace hpx::parallel::util::detail
