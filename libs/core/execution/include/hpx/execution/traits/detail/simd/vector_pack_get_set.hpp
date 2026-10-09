//  Copyright (c) 2022 Srinivas Yadav
//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR_EXPERIMENTAL_SIMD)

#include <hpx/assert.hpp>
#include <hpx/execution/traits/detail/simd/vector_pack_simd.hpp>
#include <hpx/execution/traits/vector_pack_alignment_size.hpp>

#include <cstddef>

namespace hpx::parallel::traits {

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Vector>
        requires(is_vector_pack_v<Vector>)
    HPX_HOST_DEVICE HPX_FORCEINLINE auto get(
        Vector& vec, std::size_t index) noexcept
    {
        return vec[index];
    }

    HPX_CXX_CORE_EXPORT template <typename Scalar>
        requires(is_scalar_vector_pack_v<Scalar>)
    HPX_HOST_DEVICE HPX_FORCEINLINE auto get(
        Scalar& sc, [[maybe_unused]] std::size_t const index) noexcept
    {
        HPX_ASSERT(index == 0);
        return sc;
    }

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Vector, typename T>
        requires(is_vector_pack_v<Vector>)
    HPX_HOST_DEVICE HPX_FORCEINLINE auto set(
        Vector& vec, std::size_t index, T val) noexcept
    {
        hpx::datapar::experimental::set(vec, index, val);
    }

    HPX_CXX_CORE_EXPORT template <typename Scalar, typename T>
        requires(is_scalar_vector_pack_v<Scalar>)
    HPX_HOST_DEVICE HPX_FORCEINLINE auto set(
        Scalar& sc, [[maybe_unused]] std::size_t const index, T val) noexcept
    {
        HPX_ASSERT(index == 0);
        sc = val;
    }
}    // namespace hpx::parallel::traits

#endif
