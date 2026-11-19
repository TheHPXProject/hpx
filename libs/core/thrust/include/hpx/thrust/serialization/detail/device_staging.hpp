//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/serialization.hpp>

#include <cstddef>

// Moving device memory through the archives: pinned host staging buffers and
// asynchronous copies between them and the device, used by the serialization
// of thrust::device_vector (hpx/thrust/serialization/device_vector.hpp).
namespace hpx::serialization::detail {

    // Returns the pinned host copy of 'bytes' of device memory at
    // 'device_data' that the zero-copy chunk of the object being serialized
    // refers to, or nullptr if there is none: data below the zero-copy
    // threshold, which the archive copies anyway, or archives without
    // keep-alive data (preprocess_keep_alive, which only the owner of an
    // archive that keeps the buffers alive and waits for the futures
    // provides, e.g. a parcel). The preprocessing pass starts the copy and
    // makes the parcel wait for it, like for a future that isn't ready yet;
    // the parcel keeps the copy alive until it has been sent, and the real
    // pass takes it again. The copies are matched to the objects by the
    // order of the calls, so both passes must call this for the same objects
    // in the same order. The preprocessing archive has no chunk list and
    // always reports data chunking as disabled, so it can't tell whether the
    // real pass will use chunks: it always prepares the copy, and
    // save_binary_chunk copies it into the archive if chunking is off.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT keep_alive_buffer const*
    device_host_copy_of(
        output_archive& ar, void const* device_data, std::size_t bytes);

    // Zero-copy receive into device memory: the networking layer places the
    // data at the address handed to load_binary_chunk only after the load
    // returns, and it can't write to device memory. Returns a pinned staging
    // buffer of 'bytes' for that and registers the copy from there to
    // 'device_data', which runs once the data has arrived, before the
    // parcel's action is scheduled.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void* stage_device_receive(
        input_archive& ar, void* device_data, std::size_t bytes);

    // Debug builds call this before device data is serialized: warns (once)
    // if the device still has work queued on the default stream of this
    // thread or on the legacy default stream, i.e. the data may not be
    // complete yet. This catches common mistakes only; work on other streams,
    // or on the default stream of another thread, isn't visible here.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void warn_if_device_busy() noexcept;
}    // namespace hpx::serialization::detail
