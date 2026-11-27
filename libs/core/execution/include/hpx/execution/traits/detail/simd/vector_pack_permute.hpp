//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR_STD_EXPERIMENTAL_SIMD) ||                         \
    defined(HPX_HAVE_DATAPAR_EMULATION)

#include <hpx/execution/traits/detail/simd/vector_pack_simd.hpp>
#include <hpx/execution/traits/vector_pack_alignment_size.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx::parallel::traits {

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Vector>
        requires(is_vector_pack_v<Vector>)
    HPX_HOST_DEVICE HPX_FORCEINLINE Vector vector_pack_min(
        Vector const& lhs, Vector const& rhs) noexcept
    {
#if defined(HPX_HAVE_DATAPAR_STD_EXPERIMENTAL_SIMD)
        return (std::experimental::min) (lhs, rhs);
#else
        return (hpx::datapar::experimental::min) (lhs, rhs);
#endif
    }

    HPX_CXX_CORE_EXPORT template <typename Vector>
        requires(is_vector_pack_v<Vector>)
    HPX_HOST_DEVICE HPX_FORCEINLINE Vector vector_pack_max(
        Vector const& lhs, Vector const& rhs) noexcept
    {
#if defined(HPX_HAVE_DATAPAR_STD_EXPERIMENTAL_SIMD)
        return (std::experimental::max) (lhs, rhs);
#else
        return (hpx::datapar::experimental::max) (lhs, rhs);
#endif
    }

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Vector, typename Generator>
        requires(is_vector_pack_v<Vector>)
    HPX_HOST_DEVICE HPX_FORCEINLINE Vector vector_pack_generate(
        Generator&& gen) noexcept
    {
        using value_type = typename Vector::value_type;
#if defined(HPX_HAVE_DATAPAR_STD_EXPERIMENTAL_SIMD)
        // the generator constructor; gen has to return values, not lane
        // references of another pack (libstdc++ converts those through an
        // integer for fixed_size_simd)
        return Vector([&](auto i) { return static_cast<value_type>(gen(i)); });
#else
        return []<std::size_t... I>(
                   Generator& g, std::index_sequence<I...>) constexpr {
            Vector result;
            (result.set(I,
                 static_cast<value_type>(
                     g(std::integral_constant<std::size_t, I>{}))),
                ...);
            return result;
        }(gen, std::make_index_sequence<vector_pack_size_v<Vector>>{});
#endif
    }
}    // namespace hpx::parallel::traits

#endif
