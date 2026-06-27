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

#include <cstddef>
#include <type_traits>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parallel::traits {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        template <typename T, std::size_t N, typename Abi>
        struct vector_pack_type
        {
            using type = eve::wide<T, eve::fixed<N>>;
        };

        template <typename T, typename Abi>
        struct vector_pack_type<T, 0, Abi>
        {
            using abi_type = std::conditional_t<std::is_void_v<Abi>,
                eve::expected_cardinal_t<T>, Abi>;

            using type = eve::wide<T, abi_type>;
        };

        template <typename T, typename Abi>
        struct vector_pack_type<T, 1, Abi>
        {
            using type = T;
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T, std::size_t N, typename Abi>
    struct vector_pack_type : detail::vector_pack_type<T, N, Abi>
    {
    };

    ////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T>
    struct vector_pack_mask_type<T,
        typename std::enable_if_t<eve::is_simd_value<T>{}>>
    {
        using type = eve::logical<T>;
    };
}    // namespace hpx::parallel::traits

#endif
