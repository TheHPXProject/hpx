//  Copyright (c) 2022 Srinivas Yadav
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR_EXPERIMENTAL_SIMD)

#include <cstddef>

#if defined(HPX_HAVE_DATAPAR_EMULATION)
#include <hpx/experimental/emulate_simd.hpp>

namespace hpx::datapar::experimental {

    HPX_CXX_CORE_EXPORT template <typename Vector, typename T>
    HPX_HOST_DEVICE HPX_FORCEINLINE auto set(
        Vector& vec, std::size_t index, T val) noexcept
    {
        vec.set(static_cast<int>(index), val);
    }

    HPX_CXX_CORE_EXPORT using simd_abi::native;
}    // namespace hpx::datapar::experimental

#endif

#if defined(HPX_HAVE_DATAPAR_STD_EXPERIMENTAL_SIMD)
#include <experimental/simd>

namespace hpx::datapar::experimental {

    HPX_CXX_CORE_EXPORT using std::experimental::fixed_size_simd;
    HPX_CXX_CORE_EXPORT using std::experimental::is_simd_v;
    HPX_CXX_CORE_EXPORT using std::experimental::native_simd;
    HPX_CXX_CORE_EXPORT using std::experimental::simd;
    HPX_CXX_CORE_EXPORT using std::experimental::simd_mask;

    HPX_CXX_CORE_EXPORT using std::experimental::simd_abi::native;

    HPX_CXX_CORE_EXPORT using std::experimental::memory_alignment_v;
    HPX_CXX_CORE_EXPORT using std::experimental::vector_aligned;

    HPX_CXX_CORE_EXPORT using std::experimental::all_of;
    HPX_CXX_CORE_EXPORT using std::experimental::any_of;
    HPX_CXX_CORE_EXPORT using std::experimental::find_first_set;
    HPX_CXX_CORE_EXPORT using std::experimental::none_of;
    HPX_CXX_CORE_EXPORT using std::experimental::popcount;
    HPX_CXX_CORE_EXPORT using std::experimental::reduce;

    HPX_CXX_CORE_EXPORT template <typename T, typename Abi>
    HPX_HOST_DEVICE HPX_FORCEINLINE auto choose(
        std::experimental::simd_mask<T, Abi> const& msk,
        std::experimental::simd<T, Abi> const& v_true,
        std::experimental::simd<T, Abi> const& v_false) noexcept
    {
#if defined(HPX_GCC_VERSION)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif

        std::experimental::simd<T, Abi> v;
        where(msk, v) = v_true;
        where(!msk, v) = v_false;

#if defined(HPX_GCC_VERSION)
#pragma GCC diagnostic pop
#endif
        return v;
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Abi>
    HPX_HOST_DEVICE HPX_FORCEINLINE void mask_assign(
        std::experimental::simd_mask<T, Abi> const& msk,
        std::experimental::simd<T, Abi>& v,
        std::experimental::simd<T, Abi> const& val) noexcept
    {
        where(msk, v) = val;
    }

    HPX_CXX_CORE_EXPORT template <typename Vector, typename T>
    HPX_HOST_DEVICE HPX_FORCEINLINE auto set(
        Vector& vec, std::size_t index, T val) noexcept
    {
        vec[index] = val;
    }
}    // namespace hpx::datapar::experimental
#endif

#if defined(HPX_HAVE_DATAPAR_SVE)
#include <sve/sve.hpp>

namespace hpx::datapar::experimental {

    HPX_CXX_CORE_EXPORT using namespace sve::experimental;
    HPX_CXX_CORE_EXPORT using simd_abi::native;

    HPX_CXX_CORE_EXPORT template <typename Vector, typename T>
    HPX_HOST_DEVICE HPX_FORCEINLINE auto set(
        Vector& vec, std::size_t index, T val) noexcept
    {
        vec.set(index, val);
    }
}    // namespace hpx::datapar::experimental

#endif

#endif
