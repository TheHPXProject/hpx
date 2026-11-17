//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)
#include <hpx/modules/execution.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/parallel/datapar/iterator_helpers.hpp>
#include <hpx/parallel/datapar/zip_iterator.hpp>
#include <hpx/parallel/util/detail/elements_per_task_size.hpp>
#include <hpx/parallel/util/detail/transfer_task_size.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>

namespace hpx::parallel::util::detail {

    // A datapar policy calls the loop body with packs of elements: the width
    // of the packs the datapar loops use (see datapar_loop), which the policy
    // may set (policy_traits<ExPolicy>::num_lanes), or one for elements the
    // loops don't put into packs. The packs of a zip iterator have the width
    // of the pack of its first iterator (see vector_pack_size).
    template <typename ExPolicy, typename Iter>
    struct policy_elements_per_call<ExPolicy, Iter,
        std::enable_if_t<hpx::is_vectorpack_execution_policy_v<ExPolicy>>>
    {
        [[nodiscard]] static constexpr std::size_t call(
            hpx::threads::task_size) noexcept
        {
            using iterator_type = std::decay_t<Iter>;
            if constexpr (iterator_datapar_compatible_v<iterator_type>)
            {
                constexpr std::size_t num_lanes =
                    hpx::execution::policy_traits<ExPolicy>::num_lanes;
                using pack_type = hpx::parallel::traits::vector_pack_type_t<
                    std::iter_value_t<iterator_type>, num_lanes>;
                return hpx::parallel::traits::vector_pack_size_v<pack_type>;
            }
            else
            {
                return 1;
            }
        }
    };

    // A datapar policy copies elements in packs if both iterators can be put
    // into packs and their packs have the same width (see datapar_copy_n);
    // all other elements are copied one by one.
    template <typename ExPolicy, typename InIter, typename OutIter>
    struct policy_copies_packs<ExPolicy, InIter, OutIter,
        std::enable_if_t<hpx::is_vectorpack_execution_policy_v<ExPolicy>>>
      : std::bool_constant<iterators_datapar_compatible_v<InIter, OutIter> &&
            iterator_datapar_compatible_v<InIter> &&
            iterator_datapar_compatible_v<OutIter>>
    {
    };
}    // namespace hpx::parallel::util::detail

#endif
