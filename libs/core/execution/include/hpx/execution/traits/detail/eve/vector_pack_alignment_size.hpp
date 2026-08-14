//  Copyright (c) 2022 Srinivas Yadav
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR_EVE)

#include <hpx/execution/traits/detail/eve/vector_pack_simd.hpp>

#include <cstddef>
#include <type_traits>

#include <eve/eve.hpp>
#include <eve/memory/aligned_ptr.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parallel::traits {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_vector_pack<eve::wide<T, eve::expected_cardinal_t<T>>>
      : std::true_type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_vector_pack<T> : std::false_type
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_scalar_vector_pack<eve::wide<T, eve::expected_cardinal_t<T>>>
      : std::false_type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    struct is_scalar_vector_pack<T> : std::true_type
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T, typename Enable>
    struct vector_pack_alignment
    {
        static constexpr std::size_t const value = sizeof(T);
    };

    HPX_CXX_CORE_EXPORT template <typename T, typename Abi>
    struct vector_pack_alignment<eve::wide<T, Abi>>
    {
        static constexpr std::size_t const value =
            eve::alignment_v<eve::wide<T, Abi>>;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T, typename Enable>
    struct vector_pack_size
    {
        static constexpr std::size_t const value = 1;
    };

    HPX_CXX_CORE_EXPORT template <typename T, typename Abi>
    struct vector_pack_size<eve::wide<T, Abi>>
    {
        static constexpr std::size_t const value = eve::wide<T, Abi>::size();
    };
}    // namespace hpx::parallel::traits

#endif
