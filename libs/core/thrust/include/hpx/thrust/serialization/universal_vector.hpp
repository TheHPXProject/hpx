//  Copyright (c) 2026 Ryan Thomas
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/thrust/serialization/detail/host_accessible.hpp>

#include <thrust/universal_vector.h>

#include <memory>
#include <type_traits>

namespace hpx::serialization::detail {

    // thrust::universal_vector<T, Alloc> and the device-only
    // thrust::cuda::vector<T, Alloc> are the same template
    // (thrust::detail::vector_base): only allocators of universal memory
    // (managed or pinned host memory) give memory the host can access
    HPX_CXX_CORE_EXPORT template <typename T, typename Alloc>
    inline constexpr bool is_universal_allocator_v =
        std::is_same_v<typename std::allocator_traits<Alloc>::pointer,
            ::thrust::universal_ptr<T>>;
}    // namespace hpx::serialization::detail

namespace hpx::serialization {

    // thrust::universal_vector<T, Alloc>

    HPX_CXX_CORE_EXPORT template <typename T, typename Alloc,
        typename = std::enable_if_t<detail::is_universal_allocator_v<T, Alloc>>>
    void serialize(output_archive& ar,
        ::thrust::universal_vector<T, Alloc> const& v, unsigned)
    {
        detail::save_host_accessible(ar, v);
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Alloc,
        typename = std::enable_if_t<detail::is_universal_allocator_v<T, Alloc>>>
    void serialize(
        input_archive& ar, ::thrust::universal_vector<T, Alloc>& v, unsigned)
    {
        detail::load_host_accessible(ar, v);
    }
}    // namespace hpx::serialization
