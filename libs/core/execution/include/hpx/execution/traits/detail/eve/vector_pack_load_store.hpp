//  Copyright (c) 2022 Srinivas Yadav
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR_EVE)

#include <hpx/execution/traits/detail/eve/vector_pack_simd.hpp>

#include <eve/eve.hpp>
#include <eve/memory/aligned_ptr.hpp>
#include <eve/module/core.hpp>

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
                eve::as_aligned(std::addressof(*iter), eve::cardinal_t<V>{}));
        }

        template <typename Iter>
        HPX_HOST_DEVICE HPX_FORCEINLINE static V unaligned(Iter& iter)
        {
            // V can be a genuine SIMD pack or a scalar value_type (width-1
            // pack, see vector_pack_type<T, 1, Abi>). eve::wide's pointer
            // constructor only accepts actual SIMD types, so scalars take
            // the plain-load path instead.
            if constexpr (eve::is_simd_value<V>{})
            {
                return V(std::addressof(*iter));
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
            eve::store(value,
                eve::as_aligned(std::addressof(*iter), eve::cardinal_t<V>{}));
        }

        template <typename Iter>
        HPX_HOST_DEVICE HPX_FORCEINLINE static void unaligned(
            V& value, Iter& iter)
        {
            // See vector_pack_load::unaligned above: eve::store only
            // accepts genuine SIMD packs, so scalars take the plain-store
            // path instead.
            if constexpr (eve::is_simd_value<V>{})
            {
                eve::store(value, std::addressof(*iter));
            }
            else
            {
                *iter = value;
            }
        }
    };
}    // namespace hpx::parallel::traits

#endif
