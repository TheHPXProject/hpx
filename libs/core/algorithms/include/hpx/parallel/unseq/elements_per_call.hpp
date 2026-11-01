//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/parallel/util/detail/elements_per_task_size.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>

namespace hpx::parallel::util::detail {

    // An unsequenced policy lets the compiler vectorize the loop. A loop body
    // of the smallest size (nano) on arithmetic elements is expected to be
    // vectorized, so a call handles as many elements as fit into a vector
    // register (HPX_LANE_SIZE bytes) in that time. Other loop bodies are
    // taken to handle one element.
    template <typename ExPolicy, typename Iter>
    struct policy_elements_per_call<ExPolicy, Iter,
        std::enable_if_t<hpx::is_unsequenced_execution_policy_v<ExPolicy> &&
            !hpx::is_vectorpack_execution_policy_v<ExPolicy>>>
    {
        [[nodiscard]] static constexpr std::size_t call(
            hpx::threads::task_size const size) noexcept
        {
            using iterator_type = std::decay_t<Iter>;
            if constexpr (std::random_access_iterator<iterator_type>)
            {
                // the elements of a zip iterator are tuples, its loops work
                // on the elements of the first iterator
                using value_type = std::iter_value_t<iterator_type>;
                using element_type = std::conditional_t<
                    hpx::traits::is_zip_iterator_v<iterator_type>,
                    hpx::tuple_element<0, value_type>,
                    std::type_identity<value_type>>::type;
                if constexpr (std::is_arithmetic_v<element_type>)
                {
                    if (size == hpx::threads::task_size::nano)
                    {
                        constexpr std::size_t lanes =
                            HPX_LANE_SIZE / sizeof(element_type);
                        return lanes == 0 ? 1 : lanes;
                    }
                }
            }
            return 1;
        }
    };
}    // namespace hpx::parallel::util::detail
