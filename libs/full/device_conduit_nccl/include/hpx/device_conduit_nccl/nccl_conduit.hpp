//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_DEVICE_CONDUIT_NCCL)
#include <hpx/modules/functional.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/synchronization.hpp>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace hpx::parcelset::device_conduits {

    /// What the receiver of device chunks tells their sender: the receives
    /// of round 'round' are (or will be) posted, in the order of 'tags',
    /// with the sizes in 'sizes'. The sender posts the matching sends in the
    /// same order, rounds strictly by number. 'id' is the ncclUniqueId of
    /// the communicator of the two (with the first round only).
    struct nccl_conduit_round
    {
        std::uint64_t round = 0;
        std::vector<std::uint64_t> tags;
        std::vector<std::uint64_t> sizes;
        std::vector<char> id;

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            // clang-format off
            ar & round & tags & sizes & id;
            // clang-format on
        }
    };

    /// A device conduit moving the zero-copy chunks of device memory of
    /// parcels with NCCL (or RCCL, in HIP builds) point-to-point operations.
    ///
    /// NCCL matches sends and receives between two ranks by the order they
    /// are issued in (it has no tags), its communicators aren't thread safe,
    /// and HPX decodes several parcels at once. So the conduit uses one
    /// communicator of two ranks per direction of a pair of localities (a
    /// channel), and lets the receiver decide the order: it collects the
    /// receives of the chunks it has decoded into numbered rounds, sends the
    /// sender the tags of each round (an action), and posts the receives of
    /// the round as one NCCL group. The sender holds its chunks until their
    /// round arrives and posts the matching sends as one group, rounds by
    /// number. The receiver creates the communicator (it is rank 0) when it
    /// forms its first round, which carries the ncclUniqueId to the sender.
    ///
    /// The communicators are non-blocking: no thread waits in NCCL, the
    /// conduit polls the state of the communicators and the CUDA events of
    /// the rounds in its background work. A channel uses the GPU of the
    /// first chunk it moves (one GPU per locality).
    class HPX_EXPORT nccl_conduit : public device_conduit_impl<nccl_conduit>
    {
    public:
        explicit nccl_conduit(util::runtime_configuration const& cfg);
        ~nccl_conduit() override;

        nccl_conduit(nccl_conduit const&) = delete;
        nccl_conduit& operator=(nccl_conduit const&) = delete;

        [[nodiscard]] static std::string name_impl()
        {
            return "nccl";
        }

        [[nodiscard]] bool can_reach_impl(std::uint32_t) const noexcept
        {
            return true;
        }

        void send_impl(std::uint32_t locality_id,
            serialization::detail::device_chunk_descriptor const& d,
            void const* p, std::size_t size,
            hpx::move_only_function<void(std::exception_ptr)> sent);

        void recv_impl(serialization::detail::device_chunk_descriptor const& d,
            void* p, std::size_t size,
            std::function<void(std::exception_ptr)> done);

        bool background_work_impl();

        /// Fails what is held for a locality that was disconnected
        /// ('forced'); for a locality that has left, finishes the rounds it
        /// has asked for first. Its channels are destroyed once idle.
        void locality_removed_impl(
            std::uint32_t locality_id, bool forced) noexcept;

        /// The receiver 'receiver' has formed a round of the chunks this
        /// locality sent it (what the action calls).
        void round_ready(std::uint32_t receiver, nccl_conduit_round round);

        struct channel;

    private:
        // the channel for sending to (or receiving from) 'locality_id',
        // created the first time
        std::shared_ptr<channel> get_channel(
            std::uint32_t locality_id, bool sending);

        // the channel for sending to 'locality_id' (also if the locality was
        // removed), nullptr if there is none
        std::shared_ptr<channel> find_sender(std::uint32_t locality_id);

        // makes progress on one channel, returns whether it did anything
        bool progress(channel& c);

        hpx::spinlock channels_mtx_;
        std::map<std::uint32_t, std::shared_ptr<channel>> senders_;
        std::map<std::uint32_t, std::shared_ptr<channel>> receivers_;

        // the channels of removed localities, until they are idle
        std::vector<std::shared_ptr<channel>> removed_;
    };
}    // namespace hpx::parcelset::device_conduits

#endif
