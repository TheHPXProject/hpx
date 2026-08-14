//  Copyright (c) 2021 Srinivas Yadav
//  Copyright (c) 2016-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR_EXPERIMENTAL_SIMD)

#include <hpx/execution/traits/detail/simd/vector_pack_simd.hpp>

#include <cstddef>
#include <iterator>
#include <memory>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parallel::traits {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename V, typename ValueType,
        typename Enable>
    struct vector_pack_load
    {
        template <typename Iter>
        HPX_HOST_DEVICE HPX_FORCEINLINE static V aligned(Iter& iter)
        {
            return V(
                std::addressof(*iter), datapar::experimental::vector_aligned);
        }

        template <typename Iter>
        HPX_HOST_DEVICE HPX_FORCEINLINE static V unaligned(Iter& iter)
        {
            // V can be a genuine SIMD pack or a scalar value_type (width-1
            // pack, see vector_pack_type<T, 1, Abi>). std::experimental::simd's
            // pointer constructor only accepts actual SIMD types, so scalars
            // take the plain-load path instead.
            if constexpr (datapar::experimental::is_simd_v<V>)
            {
                return V(std::addressof(*iter),
                    datapar::experimental::element_aligned);
            }
            else
            {
                return *iter;
            }
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename V, typename ValueType,
        typename Enable>
    struct vector_pack_store
    {
        template <typename Iter>
        HPX_HOST_DEVICE HPX_FORCEINLINE static void aligned(
            V& value, Iter& iter)
        {
            value.copy_to(
                std::addressof(*iter), datapar::experimental::vector_aligned);
        }

        template <typename Iter>
        HPX_HOST_DEVICE HPX_FORCEINLINE static void unaligned(
            V& value, Iter& iter)
        {
            // See vector_pack_load::unaligned above: copy_to() is only
            // available on genuine SIMD packs, so scalars take the
            // plain-store path instead.
            if constexpr (datapar::experimental::is_simd_v<V>)
            {
                value.copy_to(std::addressof(*iter),
                    datapar::experimental::element_aligned);
            }
            else
            {
                *iter = value;
            }
        }
    };
}    // namespace hpx::parallel::traits

#endif
