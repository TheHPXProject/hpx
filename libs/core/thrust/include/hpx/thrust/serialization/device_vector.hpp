//  Copyright (c) 2026 Ryan Thomas
//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/serialization.hpp>

#include <hpx/thrust/memory.hpp>
#include <hpx/thrust/serialization/detail/device_staging.hpp>

#include <thrust/copy.h>
#include <thrust/device_vector.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>

namespace hpx::serialization::detail {

    // how a device_vector's elements are stored in the archive
    HPX_CXX_CORE_EXPORT enum class device_data_storage : std::uint8_t {
        in_archive = 0,     // copied into the archive
        host_chunk = 1,     // zero-copy chunk of a pinned host copy
        device_chunk = 2    // zero-copy chunk of the device memory
    };
}    // namespace hpx::serialization::detail

namespace hpx::serialization {

    // thrust::device_vector<T, Alloc>
    //
    // The data must be complete when the vector is serialized (sent): the
    // work that produces it has to have finished, as for any other argument
    // nobody may still write to while it is sent. Thrust's algorithms
    // synchronize already. Kernels launched otherwise must be waited for
    // first, e.g. by sending from the continuation of an HPX CUDA future on
    // their stream (hpx::cuda::experimental::get_future_with_event or
    // get_future_with_callback). Reading the device memory (a copy to the
    // host on a separate stream, or the parcelport itself) is not ordered
    // after that work (HPX threads move between OS threads, so not even
    // after the sending thread's default stream). Debug builds warn if a
    // default stream is still busy.
    //
    // Data above the zero-copy threshold goes as a zero-copy chunk: of the
    // vector's memory itself if that is pinned host memory (see
    // hpx::thrust::pooled_allocator) or if the parcelport sends device
    // memory (enable_device_chunks), else of a pinned host copy. Smaller data
    // is copied into the archive. The receiving side decides itself where
    // chunks arrive, whatever memory they were sent from: in the vector's
    // memory if that is pinned host memory or if the parcelport receives into
    // device memory (allow_device_receive), else in a pinned staging buffer.
    //
    // A chunk of the vector's own memory refers to it until the parcel has
    // been sent, like the zero-copy chunks of any other container (see
    // output_archive::save_binary_chunk): the vector must live that long.
    // The arguments of a parcel do. A temporary vector a serialize()
    // function creates needs an owner that keeps it alive until then
    // whenever the chunk refers to its memory: if that is pinned, or if it
    // is device memory and the parcelport moves device memory. Only a
    // temporary sent from a host copy (which the parcel keeps) needs none,
    // unless its allocator frees the memory asynchronously on another
    // stream (e.g. cudaFreeAsync) and doesn't come from HPX's pools: the
    // copy is made asynchronously, and only the pools make freeing wait for
    // it.

    HPX_CXX_CORE_EXPORT template <typename T, typename Alloc>
    void serialize(output_archive& ar,
        ::thrust::device_vector<T, Alloc> const& v, unsigned)
    {
        static_assert(std::is_trivially_copyable_v<T>,
            "device_vector elements are serialized as raw bytes");

        using detail::device_data_storage;

        std::size_t const size = v.size();
        if (size != 0 && ar.endianess_differs())
        {
            // the elements are sent as raw bytes, they can't be converted
            HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                "hpx::serialization::serialize(device_vector)",
                "the elements of a device_vector can't be sent in an archive "
                "with another byte order");
        }

        ar << size;
        if (size == 0)
            return;

        std::size_t const bytes = size * sizeof(T);
        T const* device_data = ::thrust::raw_pointer_cast(v.data());

        if (hpx::thrust::detail::is_pinned_host_memory(device_data))
        {
            // the network reads it in place, the parcel keeps the vector
            // alive until it has been sent
#if defined(HPX_DEBUG)
            if (!ar.is_preprocessing())
            {
                detail::warn_if_device_busy();
            }
#endif
            if (bytes >= ar.get_zero_copy_serialization_threshold())
            {
                ar << static_cast<std::uint8_t>(
                    device_data_storage::host_chunk);
                ar.save_binary_chunk(
                    detail::fundamental_types::none, device_data, bytes);
            }
            else
            {
                ar << static_cast<std::uint8_t>(
                    device_data_storage::in_archive);
                ar.save_binary(
                    detail::fundamental_types::none, device_data, bytes);
            }
            return;
        }

        // Device memory must never reach save_binary, which copies on the
        // host: an archive without chunks (e.g. with the array optimization
        // disabled) copies the data through the host instead. Preprocessing
        // archives report chunking as disabled but copy nothing.
        // A filter (e.g. compression) reads the data on the host: it is
        // copied into the archive below.
        if (ar.enable_device_chunks() && !ar.is_filtered() &&
            (ar.is_preprocessing() || !ar.disable_data_chunking()) &&
            bytes >= ar.get_zero_copy_serialization_threshold())
        {
            // the parcel keeps the vector alive until it has been sent
#if defined(HPX_DEBUG)
            if (!ar.is_preprocessing())
            {
                detail::warn_if_device_busy();
            }
#endif
            ar << static_cast<std::uint8_t>(device_data_storage::device_chunk);
            ar.save_binary_chunk(detail::fundamental_types::none, device_data,
                bytes, chunk_memory::device);
            return;
        }

        // A host copy prepared during preprocessing, for the archives that
        // don't take device memory. These must ask for the copy in both
        // passes or in neither: the copies are matched by the order of the
        // requests. (An archive taking device memory with chunking disabled
        // in the real pass copies the bytes into the archive below.)
        if (auto const* buffer = !ar.enable_device_chunks() ?
                detail::device_host_copy_of(ar, device_data, bytes) :
                nullptr)
        {
            ar << static_cast<std::uint8_t>(device_data_storage::host_chunk);
            ar.save_binary_chunk(
                detail::fundamental_types::none, buffer->data, bytes);
            return;
        }

        // a preprocessing pass that prepares no copy (e.g. the size
        // calculation of a checkpoint) only counts the bytes
        if (ar.is_preprocessing())
        {
            ar << static_cast<std::uint8_t>(device_data_storage::in_archive);
            ar.save_binary(detail::fundamental_types::none, device_data, bytes);
            return;
        }

        // no host copy prepared (small data, or no preprocessing pass): copy
        // the bytes into the archive before the temporary goes away
#if defined(HPX_DEBUG)
        detail::warn_if_device_busy();
#endif
        ar << static_cast<std::uint8_t>(device_data_storage::in_archive);
        // (not a std::vector<T>, which has no data() for T = bool; not
        // value-initialized, it's overwritten)
        std::unique_ptr<T[]> host(new T[size]);
        ::thrust::copy(v.begin(), v.end(), host.get());
        ar.save_binary(detail::fundamental_types::none, host.get(), bytes);
    }

    HPX_CXX_CORE_EXPORT template <typename T, typename Alloc>
    void serialize(
        input_archive& ar, ::thrust::device_vector<T, Alloc>& v, unsigned)
    {
        static_assert(std::is_trivially_copyable_v<T>,
            "device_vector elements are serialized as raw bytes");

        using detail::device_data_storage;

        std::size_t size = 0;
        ar >> size;
        if (size != 0 && ar.endianess_differs())
        {
            // the elements are sent as raw bytes, they can't be converted
            HPX_THROW_EXCEPTION(hpx::error::serialization_error,
                "hpx::serialization::serialize(device_vector)",
                "the elements of a device_vector can't be received from an "
                "archive with another byte order");
        }
        v.resize(size);
        if (size == 0)
            return;

        std::uint8_t stored = 0;
        ar >> stored;
        auto const storage = static_cast<device_data_storage>(stored);

        std::size_t const bytes = size * sizeof(T);
        T* device_data = ::thrust::raw_pointer_cast(v.data());

        // A device conduit moves the data (see device_chunk_transport): it
        // arrives in the vector's own memory, device or pinned host memory,
        // before the parcel's action runs, whatever the parcelport can do.
        if (storage != device_data_storage::in_archive &&
            ar.next_chunk_from_transport())
        {
            ar.load_binary_chunk(detail::fundamental_types::none, device_data,
                bytes, true,
                hpx::thrust::detail::is_pinned_host_memory(device_data) ?
                    chunk_memory::host :
                    chunk_memory::device);
            return;
        }

        // Zero-copy receive: the networking layer places the data at the
        // address handed over here only after this function returns. A
        // filter (e.g. decompression) writes all data on the host instead.
        bool const zero_copy_receive =
            storage != device_data_storage::in_archive && !ar.is_filtered() &&
            !ar.disable_receive_data_chunking() &&
            ar.try_get_extra_data<detail::allow_zero_copy_receive>() != nullptr;

        if (hpx::thrust::detail::is_pinned_host_memory(device_data))
        {
            // the network writes to pinned host memory directly
            if (storage == device_data_storage::in_archive)
            {
                ar.load_binary(
                    detail::fundamental_types::none, device_data, bytes);
            }
            else
            {
                ar.load_binary_chunk(detail::fundamental_types::none,
                    device_data, bytes, zero_copy_receive);
            }
            return;
        }

        if (zero_copy_receive &&
            ar.try_get_extra_data<detail::allow_device_receive>() != nullptr)
        {
            // the parcelport receives into device memory: it receives into v,
            // also data sent from host memory
            ar.load_binary_chunk(detail::fundamental_types::none, device_data,
                bytes, true, chunk_memory::device);
            return;
        }

        if (zero_copy_receive)
        {
            // The parcelport doesn't receive into device memory: hand it a
            // pinned staging buffer and copy to the device once the data has
            // arrived, before the parcel's action runs.
            void* staging =
                detail::stage_device_receive(ar, device_data, bytes);
            ar.load_binary_chunk(
                detail::fundamental_types::none, staging, bytes, true);
            return;
        }

        // the data is complete (in host memory) when the archive returns it
        std::unique_ptr<T[]> host(new T[size]);
        if (storage == device_data_storage::in_archive)
        {
            ar.load_binary(detail::fundamental_types::none, host.get(), bytes);
        }
        else
        {
            ar.load_binary_chunk(
                detail::fundamental_types::none, host.get(), bytes, false);
        }
        ::thrust::copy(host.get(), host.get() + size, v.begin());
    }
}    // namespace hpx::serialization
