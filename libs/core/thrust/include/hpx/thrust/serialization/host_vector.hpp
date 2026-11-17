//  Copyright (c) 2026 Ryan Thomas
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/thrust/serialization/detail/host_accessible.hpp>

#include <thrust/host_vector.h>

namespace hpx::serialization {

    // thrust::host_vector<T, Alloc>

    HPX_CXX_CORE_EXPORT template <typename T, typename Alloc>
    void serialize(
        output_archive& ar, ::thrust::host_vector<T, Alloc> const& v, unsigned)
    {
        detail::save_host_accessible(ar, v);
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Alloc>
    void serialize(
        input_archive& ar, ::thrust::host_vector<T, Alloc>& v, unsigned)
    {
        detail::load_host_accessible(ar, v);
    }
}    // namespace hpx::serialization
