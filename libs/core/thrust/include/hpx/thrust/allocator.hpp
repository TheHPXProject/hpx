//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <hpx/thrust/memory.hpp>

#include <thrust/device_ptr.h>
#include <thrust/device_vector.h>
#include <thrust/mr/allocator.h>
#include <thrust/mr/memory_resource.h>

#include <cstddef>
#include <limits>
#include <new>

namespace hpx::thrust {

    // Memory for thrust::device_vector and the like from pools of device or
    // pinned host memory, see hpx::thrust::default_memory_kind for which one.
    // Allocating device memory or pinned host memory takes long (milliseconds
    // for a few MiB of pinned memory), and the network has to register each
    // new buffer before it can send it from or receive it into it; buffers
    // from the pools are allocated, and registered, once.
    //
    // Memory goes back to its pool when it's deallocated and is reused right
    // away: as with any allocator, no device work may still use it then
    // (cudaFree waits for all device work, the pool doesn't).
    HPX_CXX_CORE_EXPORT class pooled_memory_resource final
      : public ::thrust::mr::memory_resource<::thrust::device_ptr<void>>
    {
    public:
        pointer do_allocate(
            std::size_t const bytes, std::size_t const alignment) override
        {
            // the pools' memory is aligned to at least 256 bytes
            if (alignment > 256)
            {
                throw std::bad_alloc();
            }
            return pointer(detail::pool_allocate(bytes));
        }

        void do_deallocate(
            pointer p, std::size_t const bytes, std::size_t) override
        {
            detail::pool_deallocate(p.get(), bytes);
        }
    };

    // allocator of thrust::device_vector<T, hpx::thrust::pooled_allocator<T>>
    HPX_CXX_CORE_EXPORT template <typename T>
    class pooled_allocator
      : public ::thrust::mr::stateless_resource_allocator<T,
            pooled_memory_resource>
    {
        using base_type = ::thrust::mr::stateless_resource_allocator<T,
            pooled_memory_resource>;

    public:
        template <typename U>
        struct rebind
        {
            using other = pooled_allocator<U>;
        };

        using pointer = base_type::pointer;
        using size_type = base_type::size_type;

        pooled_allocator() = default;

        template <typename U>
        HPX_HOST_DEVICE pooled_allocator(pooled_allocator<U> const& other)
          : base_type(other)
        {
        }

        // thrust computes the number of bytes without checking for overflow
        pointer allocate(size_type const n)
        {
            if (n > (std::numeric_limits<std::size_t>::max)() / sizeof(T))
            {
                throw std::bad_alloc();
            }
            return base_type::allocate(n);
        }
    };

    // a thrust::device_vector of pooled memory
    HPX_CXX_CORE_EXPORT template <typename T>
    using device_vector = ::thrust::device_vector<T, pooled_allocator<T>>;
}    // namespace hpx::thrust
