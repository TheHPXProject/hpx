//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>

namespace hpx::serialization::detail {

    // What an archive stores in place of a chunk of device memory that a
    // transport moves (see device_chunk_transport); the transport defines
    // the meaning of the two values (e.g. the sending locality and a tag).
    HPX_CXX_CORE_EXPORT struct device_chunk_descriptor
    {
        std::uint64_t source = 0;
        std::uint64_t tag = 0;
    };

    // Moves the data of zero-copy chunks of device memory between the two
    // ends of an archive on its own, outside of the archive (e.g. over UCX or
    // NCCL), the archive carries only a descriptor for each of them. The
    // parcel layer attaches one to the archives of the parcels it sends to a
    // destination such a transport reaches, and to the archives it receives,
    // and sets archive_flags::enable_device_transport on the former. The
    // archives call it from save_binary_chunk and load_binary_chunk for
    // chunks marked as device memory (chunk_memory::device), never while
    // preprocessing.
    HPX_CXX_CORE_EXPORT class device_chunk_transport
    {
    public:
        device_chunk_transport() = default;
        device_chunk_transport(device_chunk_transport const&) = delete;
        device_chunk_transport& operator=(
            device_chunk_transport const&) = delete;
        virtual ~device_chunk_transport() = default;

        // Sending side: starts sending 'size' bytes of device memory at 'p'
        // to the destination of the archive and returns their descriptor.
        // The memory stays valid until the transport has sent the data (the
        // parcel layer keeps the sent objects alive through the transport).
        [[nodiscard]] virtual device_chunk_descriptor send(
            void const* p, std::size_t size) = 0;

        // Receiving side: receives the data described by 'descriptor' into
        // 'size' bytes of memory at 'p' and calls 'done' once it has arrived
        // (with the error if it failed). It is started as one of the steps of
        // the parcel that de-serialized the chunk
        // (post_receive_actions::add_transfer): the parcel's action runs
        // after 'done' has been called, not at all if it failed.
        virtual void recv(device_chunk_descriptor const& descriptor, void* p,
            std::size_t size, std::function<void(std::exception_ptr)> done) = 0;
    };

    // The transport of an archive (extra data); none if 'transport' is
    // empty.
    HPX_CXX_CORE_EXPORT struct device_transport
    {
        std::shared_ptr<device_chunk_transport> transport;
    };
}    // namespace hpx::serialization::detail

// This is explicitly instantiated to ensure that the id is stable across shared
// libraries.
template <>
struct hpx::util::extra_data_helper<
    hpx::serialization::detail::device_transport>
{
    HPX_CORE_EXPORT static extra_data_id_type id() noexcept;
    static void reset(serialization::detail::device_transport* p) noexcept
    {
        p->transport.reset();
    }
};
