//  Copyright (c) 2026 Ryan Thomas
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/serialization.hpp>

#include <thrust/memory.h>

#include <cstddef>

namespace hpx::serialization::detail {

    // host_vector and universal_vector own host-accessible memory, which
    // lives in the parcel as long as the vector does: serialize it like a
    // std::vector, zero-copy chunks included.
    HPX_CXX_CORE_EXPORT template <typename Vector>
    void save_host_accessible(output_archive& ar, Vector const& v)
    {
        std::size_t const size = v.size();
        ar << size;
        if (size != 0)
        {
            ar << hpx::serialization::make_array(
                ::thrust::raw_pointer_cast(v.data()), size);
        }
    }

    HPX_CXX_CORE_EXPORT template <typename Vector>
    void load_host_accessible(input_archive& ar, Vector& v)
    {
        std::size_t size = 0;
        ar >> size;
        v.resize(size);
        if (size != 0)
        {
            ar >> hpx::serialization::make_array(
                      ::thrust::raw_pointer_cast(v.data()), size);
        }
    }
}    // namespace hpx::serialization::detail
