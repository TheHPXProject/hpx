//  Copyright (c) 2022 Srinivas Yadav
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR)

namespace hpx::parallel::traits {

    ////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr T choose(
        bool msk, T v_true, T v_false) noexcept
    {
        return msk ? v_true : v_false;
    }

    ////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename T>
    HPX_HOST_DEVICE HPX_FORCEINLINE constexpr void mask_assign(
        bool msk, T& v, T val) noexcept
    {
        if (msk)
        {
            v = val;
        }
    }
}    // namespace hpx::parallel::traits

#if !defined(__CUDACC__)
#include <hpx/execution/traits/detail/eve/vector_pack_conditionals.hpp>
#include <hpx/execution/traits/detail/simd/vector_pack_conditionals.hpp>
#include <hpx/execution/traits/detail/vc/vector_pack_conditionals.hpp>
#endif

#endif
