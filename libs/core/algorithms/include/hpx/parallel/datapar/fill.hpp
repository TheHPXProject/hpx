//  Copyright (c) 2021 Srinivas Yadav
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)
#include <hpx/modules/execution.hpp>
#include <hpx/parallel/algorithms/detail/fill.hpp>
#include <hpx/parallel/datapar/handle_local_exceptions.hpp>
#include <hpx/parallel/datapar/loop.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct datapar_fill
    {
        template <typename ExPolicy, typename Iter, typename Sent, typename T>
        HPX_HOST_DEVICE HPX_FORCEINLINE static typename std::enable_if<
            util::detail::iterator_datapar_compatible_v<Iter>, Iter>::type
        call(ExPolicy&&, Iter first, Sent last, T const& val)
        {
            hpx::parallel::util::loop_ind<std::decay_t<ExPolicy>>(
                first, last, [&val](auto&& v) { v = val; });
            return first;
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter,
        typename Sent, typename T>
    // clang-format off
        requires (
            hpx::is_vectorpack_execution_policy_v<ExPolicy> &&
            hpx::parallel::util::detail::iterator_datapar_compatible<Iter>::value
        )
    // clang-format on
    HPX_HOST_DEVICE HPX_FORCEINLINE Iter hpx_invoke(sequential_fill_t,
        ExPolicy&& policy, Iter first, Sent last, T const& value)
    {
        return datapar_fill::call(
            HPX_FORWARD(ExPolicy, policy), first, last, value);
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct datapar_fill_n
    {
        template <typename ExPolicy, typename Iter, typename T>
        HPX_HOST_DEVICE HPX_FORCEINLINE static typename std::enable_if<
            util::detail::iterator_datapar_compatible_v<Iter>, Iter>::type
        call(ExPolicy&&, Iter first, std::size_t count, T const& val)
        {
            hpx::parallel::util::loop_n_ind<std::decay_t<ExPolicy>>(
                first, count, [&val](auto&& v) { v = val; });
            return first;
        }
    };

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename Iter, typename T>
    // clang-format off
        requires (
            hpx::is_vectorpack_execution_policy_v<ExPolicy> &&
            parallel::util::detail::iterator_datapar_compatible<Iter>::value
        )
    // clang-format on
    HPX_HOST_DEVICE HPX_FORCEINLINE Iter hpx_invoke(sequential_fill_n_t,
        ExPolicy&& policy, Iter first, std::size_t count, T const& value)
    {
        return datapar_fill_n::call(
            HPX_FORWARD(ExPolicy, policy), first, count, value);
    }
}    // namespace hpx::parallel::detail

#endif
