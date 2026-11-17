//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <cstddef>
#include <cstdint>

// Pools of device and pinned host memory, used by hpx::thrust::pooled_allocator
// (hpx/thrust/allocator.hpp) and for the staging buffers of the device_vector
// serialization.
namespace hpx::thrust {

    // where hpx::thrust::pooled_allocator places data
    HPX_CXX_CORE_EXPORT enum class memory_kind : std::uint8_t {
        device = 0,    // device memory (cudaMalloc, hipMalloc)
        pinned = 1    // page-locked host memory (cudaMallocHost, hipHostMalloc)
    };

    // The memory hpx::thrust::pooled_allocator uses, from the configuration
    // entry hpx.thrust.memory: device (the default) or pinned. Device kernels
    // work on device memory faster, also on integrated GPUs (measured on an
    // NVIDIA GB10); the network sends pinned host memory from and receives
    // it into its place without staging copies. Before the runtime exists,
    // the configuration isn't complete yet: it's read again on every call
    // until then.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT memory_kind default_memory_kind();

    // Frees the memory the pools keep for reuse. The pools free it themselves
    // (and try again) if an allocation runs out of memory.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void release_cached_memory() noexcept;
}    // namespace hpx::thrust

namespace hpx::thrust::detail {

    // Returns 'bytes' of memory of the default kind for the current device,
    // or of the given kind (device memory of the current device), from the
    // pools. Throws std::bad_alloc if there's not enough memory.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void* pool_allocate(std::size_t bytes);
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void* pool_allocate(
        std::size_t bytes, memory_kind kind);

    // Gives memory from pool_allocate back to its pool. The memory is reused
    // right away: no device work may use it anymore.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void pool_deallocate(
        void* p, std::size_t bytes) noexcept;

    // whether p points to pinned host memory, which the host and the network
    // can access directly
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool is_pinned_host_memory(
        void const* p) noexcept;
}    // namespace hpx::thrust::detail
