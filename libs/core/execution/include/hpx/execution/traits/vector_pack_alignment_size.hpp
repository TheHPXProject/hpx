//  Copyright (c) 2016-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <type_traits>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parallel::traits {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct is_vector_pack : std::false_type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_vector_pack_v = is_vector_pack<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T>
    using is_vector_pack_t = typename is_vector_pack<T>::type;

    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct is_scalar_vector_pack;

    HPX_CXX_CORE_EXPORT template <typename T, typename Enable>
    struct is_scalar_vector_pack : std::false_type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_scalar_vector_pack_v =
        is_scalar_vector_pack<T>::value;

    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct is_non_scalar_vector_pack : std::false_type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_non_scalar_vector_pack_v =
        is_non_scalar_vector_pack<T>::value;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct vector_pack_alignment;

    template <typename... Vector>
    struct vector_pack_alignment<hpx::tuple<Vector...>,
        std::enable_if_t<hpx::util::all_of_v<is_vector_pack_t<Vector>...>>>
    {
        using pack_type =
            typename hpx::tuple_element<0, hpx::tuple<Vector...>>::type;

        static constexpr std::size_t const value =
            vector_pack_alignment<pack_type>::value;
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr std::size_t vector_pack_alignment_v =
        vector_pack_alignment<T>::value;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct vector_pack_size;

    template <typename... Vector>
    struct vector_pack_size<hpx::tuple<Vector...>,
        std::enable_if_t<hpx::util::all_of_v<is_vector_pack_t<Vector>...>>>
    {
        using pack_type =
            typename hpx::tuple_element<0, hpx::tuple<Vector...>>::type;

        static constexpr std::size_t const value =
            vector_pack_size<pack_type>::value;
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr std::size_t vector_pack_size_v =
        vector_pack_size<T>::value;

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Pack,
        typename Enable = std::enable_if_t < is_vector_pack_v<Pack> ||
            is_scalar_vector_pack_v<Pack> >>
                constexpr std::size_t size(Pack) noexcept
    {
        return vector_pack_size_v<Pack>;
    }
}    // namespace hpx::parallel::traits

#if !defined(__CUDACC__)
#include <hpx/execution/traits/detail/eve/vector_pack_alignment_size.hpp>
#include <hpx/execution/traits/detail/simd/vector_pack_alignment_size.hpp>
#include <hpx/execution/traits/detail/vc/vector_pack_alignment_size.hpp>
#endif

#endif
