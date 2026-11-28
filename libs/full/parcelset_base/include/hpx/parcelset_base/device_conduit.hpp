//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/serialization.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

namespace hpx::parcelset {

    /// What a device conduit has moved so far.
    HPX_CXX_EXPORT struct device_conduit_statistics
    {
        std::uint64_t chunks_sent = 0;
        std::uint64_t bytes_sent = 0;
        std::uint64_t chunks_received = 0;
        std::uint64_t bytes_received = 0;
    };

    /// A device conduit moves the data of zero-copy chunks of device memory
    /// (serialization_chunk::is_device_memory) between localities on its
    /// own, below the parcelports (e.g. over UCX or NCCL): the parcel carries
    /// only a descriptor for each such chunk, the parcelport sends host data
    /// only. Conduits are interchangeable like parcelports: they are compiled
    /// into HPX or loaded as plugins at startup (see
    /// hpx::plugins::device_conduit_factory), and the configuration entry
    /// hpx.parcel.device_conduit names the one to use ('none' by default: the
    /// parcelport sends device memory itself if it can, otherwise it is
    /// staged through host memory). Both ends of a message must use the same
    /// conduit. A locality uses one conduit at a time, chosen at startup, for
    /// all its parcelports and destinations (see get_device_conduit()).
    ///
    /// This is the interface the parcel layer uses, once per message;
    /// conduits implement it through hpx::parcelset::device_conduit_impl.
    HPX_CXX_EXPORT class HPX_EXPORT device_conduit
    {
    public:
        device_conduit() = default;
        device_conduit(device_conduit const&) = delete;
        device_conduit& operator=(device_conduit const&) = delete;
        virtual ~device_conduit();

        /// The name the conduit is registered under.
        [[nodiscard]] virtual std::string name() const = 0;

        /// Whether the conduit moves device memory between this locality
        /// and 'locality_id' (the other one uses the same conduit).
        [[nodiscard]] virtual bool can_reach(
            std::uint32_t locality_id) const = 0;

        /// Whether the conduit also moves host memory (the parcel layer
        /// then lets it move large host chunks, where the parcelports are
        /// configured to, see locality_capabilities::conduit_host_chunks).
        [[nodiscard]] virtual bool moves_host_memory() const = 0;

        /// Whether the conduit may move host chunks to 'locality_id' now
        /// (moves_host_memory is true): a conduit may first check that it
        /// can reach the destination (it starts the check when a message
        /// to it has a host chunk, see
        /// serialization::detail::device_chunk_transport::
        /// host_chunk_waiting), host chunks go through the parcelport until
        /// then, and for good if it can't. Called for every message to a
        /// destination that receives host chunks through the conduit, from
        /// any thread.
        [[nodiscard]] virtual bool host_chunks_ready(
            std::uint32_t locality_id) = 0;

        /// The transport for one message to 'locality_id' (can_reach is
        /// true). It keeps 'keep_alive' (the objects the message refers to,
        /// e.g. its parcels) until it has sent all data handed to it, which
        /// may be after the message itself has been sent.
        [[nodiscard]] virtual std::shared_ptr<
            serialization::detail::device_chunk_transport>
        sender(std::uint32_t locality_id, std::shared_ptr<void> keep_alive) = 0;

        /// The transport for one received message.
        [[nodiscard]] virtual std::shared_ptr<
            serialization::detail::device_chunk_transport>
        receiver() = 0;

        /// What the conduit has moved so far.
        [[nodiscard]] virtual device_conduit_statistics statistics() const = 0;

        /// Waits until the transfers the conduit has started have finished
        /// (their completions refer to the conduit), but not longer than
        /// 'timeout'. Returns whether they have. Called at shutdown, after
        /// the parcelports and the thread manager have stopped, on an OS
        /// thread, before the conduit is destroyed: a conduit must be able
        /// to finish its transfers without HPX threads (driving its own
        /// progress if needed). Errors of transfers finishing then are logged
        /// only, and messages whose data arrives then are dropped (their
        /// actions can't run any more).
        [[nodiscard]] virtual bool drain(
            std::chrono::steady_clock::duration timeout) noexcept = 0;

        /// Makes progress (called with the parcelports' background work),
        /// returns whether it did anything.
        virtual bool background_work() = 0;

        /// The locality 'locality_id' has left ('forced' is false) or was
        /// disconnected with hpx::force_disconnect ('forced' is true): the
        /// conduit releases what it holds for it (e.g. its connection). A
        /// locality leaving still receives what was sent to it before, a
        /// disconnected one need not. Sends to it that haven't finished may
        /// fail; like parcels to a disconnected locality, these failures are
        /// logged, not reported as errors.
        virtual void locality_removed(
            std::uint32_t locality_id, bool forced) noexcept = 0;
    };
}    // namespace hpx::parcelset

#endif
