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
#include <type_traits>

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
            if constexpr (std::is_class_v<V>)
            {
                return eve::load(eve::as_aligned(
                    std::addressof(*iter), eve::cardinal_t<V>{}));
            }
            else
            {
                return *iter;
            }
        }

        template <typename Iter>
        HPX_HOST_DEVICE HPX_FORCEINLINE static V unaligned(Iter& iter)
        {
            if constexpr (std::is_class_v<V>)
            {
                return eve::load(std::addressof(*iter));
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
            if constexpr (std::is_class_v<V>)
            {
                eve::store(value,
                    eve::as_aligned(
                        std::addressof(*iter), eve::cardinal_t<V>{}));
            }
            else
            {
                *iter = value;
            }
        }

        template <typename Iter>
        HPX_HOST_DEVICE HPX_FORCEINLINE static void unaligned(
            V& value, Iter& iter)
        {
            if constexpr (std::is_class_v<V>)
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
