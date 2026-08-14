//  Copyright (c) 2016-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)
#include <hpx/modules/datastructures.hpp>

#include <cstddef>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::parallel::traits {

    ///////////////////////////////////////////////////////////////////////////
    // exposition only
    HPX_CXX_CORE_EXPORT template <typename T, std::size_t N = 0,
        typename Abi = void>
    struct vector_pack_type;

    HPX_CXX_CORE_EXPORT template <typename T, std::size_t N = 0,
        typename Abi = void>
    using vector_pack_type_t = typename vector_pack_type<T, N, Abi>::type;

    // handle tuple<> transformations
    template <typename... T, std::size_t N, typename Abi>
    struct vector_pack_type<hpx::tuple<T...>, N, Abi>
    {
        using type = hpx::tuple<vector_pack_type_t<T, N, Abi>...>;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T, typename NewT>
    struct rebind_pack
    {
        using type = vector_pack_type_t<T>;
    };

    HPX_CXX_CORE_EXPORT template <typename T, typename NewT>
    using rebind_pack_t = typename rebind_pack<T, NewT>::type;

    ////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T, typename Enable = void>
    struct vector_pack_mask_type
    {
        using type = bool;
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    using vector_pack_mask_type_t = typename vector_pack_mask_type<T>::type;
}    // namespace hpx::parallel::traits

#if !defined(__CUDACC__)
#include <hpx/execution/traits/detail/eve/vector_pack_type.hpp>
#include <hpx/execution/traits/detail/simd/vector_pack_type.hpp>
#include <hpx/execution/traits/detail/vc/vector_pack_type.hpp>
#endif

#endif
