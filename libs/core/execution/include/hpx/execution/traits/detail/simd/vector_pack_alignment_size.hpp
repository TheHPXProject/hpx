//  Copyright (c) 2021 Srinivas Yadav
//  Copyright (c) 2016-2026 Hartmut Kaiser
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
    template <typename T>
    struct is_vector_pack<datapar::experimental::native_simd<T>>
      : std::true_type
    {
    };

    // With the SVE backend fixed_size_simd<T, N> is simd<T, sve_abi>, so N is
    // not deducible (and it duplicates the native_simd<T> specialization).
#if !defined(HPX_HAVE_DATAPAR_SVE)
    template <typename T, std::size_t N>
    struct is_vector_pack<datapar::experimental::fixed_size_simd<T, N>>
      : std::true_type
    {
    };
#endif

    template <typename T>
    struct is_vector_pack<T> : std::false_type
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename T>
    struct is_scalar_vector_pack<datapar::experimental::native_simd<T>>
      : std::false_type
    {
    };

#if !defined(HPX_HAVE_DATAPAR_SVE)
    template <typename T, std::size_t N>
    struct is_scalar_vector_pack<datapar::experimental::fixed_size_simd<T, N>>
      : std::false_type
    {
    };
#endif

    template <typename T>
    struct is_scalar_vector_pack<T> : std::true_type
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T, typename Enable>
    struct vector_pack_alignment
    {
        static constexpr std::size_t value = sizeof(T);
    };

    template <typename T, typename Abi>
    struct vector_pack_alignment<datapar::experimental::simd<T, Abi>>
    {
        static constexpr std::size_t value =
            datapar::experimental::memory_alignment_v<
                datapar::experimental::simd<T, Abi>>;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T, typename Enable>
    struct vector_pack_size
    {
        static constexpr std::size_t value = 1;
    };

    template <typename T, typename Abi>
    struct vector_pack_size<datapar::experimental::simd<T, Abi>>
    {
        static constexpr std::size_t value =
            datapar::experimental::simd<T, Abi>::size();
    };
}    // namespace hpx::parallel::traits

#endif
