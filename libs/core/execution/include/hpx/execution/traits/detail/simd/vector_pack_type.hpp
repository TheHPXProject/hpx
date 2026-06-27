//  Copyright (c) 2021 Srinivas Yadav
//  Copyright (c) 2016-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR_EXPERIMENTAL_SIMD)

#include <hpx/execution/traits/detail/simd/vector_pack_simd.hpp>

#include <cstddef>
#include <type_traits>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parallel::traits {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        template <typename T, std::size_t N, typename Abi>
        struct vector_pack_type
        {
            using type = datapar::experimental::fixed_size_simd<T, N>;
        };

        template <typename T, typename Abi>
        struct vector_pack_type<T, 0, Abi>
        {
            using abi_type = std::conditional_t<std::is_void_v<Abi>,
                datapar::experimental::native<T>, Abi>;

            using type = datapar::experimental::simd<T, abi_type>;
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
    template <typename T>
    struct vector_pack_mask_type<T,
        std::enable_if_t<datapar::experimental::is_simd_v<T>>>
    {
        using type = typename T::mask_type;
    };
}    // namespace hpx::parallel::traits

#endif
