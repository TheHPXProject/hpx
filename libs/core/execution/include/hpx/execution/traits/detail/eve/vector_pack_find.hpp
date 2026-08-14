//  Copyright (c) 2022 Srinivas Yadav
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_DATAPAR_EVE)

#include <hpx/execution/traits/detail/eve/vector_pack_simd.hpp>

#include <eve/module/core.hpp>

namespace hpx::parallel::traits {

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Mask>
    HPX_HOST_DEVICE HPX_FORCEINLINE int find_first_of(Mask const& msk) noexcept
    {
        auto tmp = eve::first_true(msk);
        if (tmp.has_value())
        {
            return tmp.value();
        }
        return -1;
    }
}    // namespace hpx::parallel::traits

#endif
