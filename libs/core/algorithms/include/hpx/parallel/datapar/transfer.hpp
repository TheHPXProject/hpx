//  Copyright (c) 2021 Srinivas Yadav
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/parallel/datapar/transform_loop.hpp>
#include <hpx/parallel/util/result_types.hpp>
#include <hpx/parallel/util/transfer.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx::parallel::util {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename Iterator>
        struct datapar_copy_n
        {
            template <typename InIter, typename OutIter>
            HPX_HOST_DEVICE HPX_FORCEINLINE static typename std::enable_if<
                iterators_datapar_compatible_v<InIter, OutIter> &&
                    iterator_datapar_compatible_v<InIter> &&
                    iterator_datapar_compatible_v<OutIter>,
                in_out_result<InIter, OutIter>>::type
            call(InIter first, std::size_t count, OutIter dest)
            {
                auto ret =
                    util::transform_loop_n_ind<hpx::execution::simd_policy>(
                        first, count, dest, [](auto&& v) { return v; });

                return util::in_out_result<InIter, OutIter>{
                    HPX_MOVE(ret.first), HPX_MOVE(ret.second)};
            }

            template <typename InIter, typename OutIter>
            HPX_HOST_DEVICE HPX_FORCEINLINE static typename std::enable_if<
                !iterators_datapar_compatible_v<InIter, OutIter> ||
                    !iterator_datapar_compatible_v<InIter> ||
                    !iterator_datapar_compatible_v<OutIter>,
                in_out_result<InIter, OutIter>>::type
            call(InIter first, std::size_t count, OutIter dest)
            {
                return util::copy_n<hpx::execution::sequenced_policy>(
                    first, count, dest);
            }
        };
    }    // namespace detail

    HPX_CXX_CORE_EXPORT template <typename ExPolicy, typename InIter,
        typename OutIter>
    HPX_HOST_DEVICE HPX_FORCEINLINE
        typename std::enable_if<hpx::is_vectorpack_execution_policy_v<ExPolicy>,
            in_out_result<InIter, OutIter>>::type
        hpx_invoke(hpx::parallel::util::copy_n_t<ExPolicy>, InIter first,
            std::size_t count, OutIter dest)
    {
        return detail::datapar_copy_n<InIter>::call(first, count, dest);
    }
}    // namespace hpx::parallel::util

#endif
