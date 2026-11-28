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

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace hpx::parcelset::device_conduits {

    /// What the receiver of device chunks tells their sender: the receives
    /// of round 'round' of the channel from GPU 'sender_device' of the
    /// sender to GPU 'receiver_device' of the receiver are (or will be)
    /// posted, in the order of 'tags', with the sizes in 'sizes'. The sender
    /// posts the matching sends in the same order, rounds strictly by
    /// number. 'id' is the ncclUniqueId of the channel's communicator (with
    /// the first round only).
    struct nccl_conduit_round
    {
        std::uint64_t round = 0;
        std::int32_t sender_device = -1;
        std::int32_t receiver_device = -1;
        std::vector<std::uint64_t> tags;
        std::vector<std::uint64_t> sizes;
        std::vector<char> id;

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            ar & round & sender_device & receiver_device & tags & sizes & id;
        }
    };

    /// A device conduit moving the zero-copy chunks of device memory of
    /// parcels with NCCL (or RCCL, in HIP builds) point-to-point operations.
    ///
    /// NCCL matches sends and receives between two ranks by the order they
    /// are issued in (it has no tags), a rank belongs to one GPU, its
    /// communicators aren't thread safe, and HPX decodes several parcels at
    /// once. So the conduit uses one communicator of two ranks per direction
    /// of a pair of localities and pair of GPUs (a channel: the GPU of the
    /// sent data and the one of the memory receiving it; the sender puts its
    /// GPU into the chunk's descriptor), and lets the receiver decide the
    /// order: it collects the
    /// receives of the chunks it has decoded into numbered rounds, sends the
    /// sender the tags of each round (an action), and posts the receives of
    /// the round as one NCCL group. The sender holds its chunks until their
    /// round arrives and posts the matching sends as one group, rounds by
    /// number. The receiver creates the communicator (it is rank 0) when it
    /// forms its first round, which carries the ncclUniqueId to the sender.
    ///
    /// The communicators are non-blocking: no thread waits in NCCL, the
    /// conduit polls the state of the communicators and the CUDA events of
    /// the rounds in its background work. With RCCL they are blocking, their
    /// blocking calls run on a thread of each channel. Communicators are
    /// created for the pairs of GPUs used only; localities on the same node
    /// must not use the same GPU (two ranks of a communicator can't share
    /// one).
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

        /// The defaults of [hpx.parcel.conduit.nccl]: stall_timeout (the
        /// seconds after which a round in flight is reported, 0: never),
        /// stall_action ('report' or 'fail'), test_hold_rounds (for tests).
        static void config_defaults(std::vector<std::string>& ini);

        /// The number of rounds reported as stalled so far (see
        /// hpx.parcel.conduit.nccl.stall_timeout).
        [[nodiscard]] std::uint64_t stalled_rounds() const noexcept
        {
            return stalled_rounds_.load(std::memory_order_relaxed);
        }

        [[nodiscard]] bool can_reach_impl(std::uint32_t) const noexcept
        {
            return true;
        }

        // sets d.extra to the GPU of the data
        void send_impl(std::uint32_t locality_id,
            serialization::detail::device_chunk_descriptor& d, void const* p,
            std::size_t size,
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

        /// The receiver 'receiver' won't ask for the chunks 'tags' this
        /// locality sent it (its channel failed, or its request for them
        /// couldn't be sent): they fail with 'why' (what the action calls).
        void chunks_dropped(std::uint32_t receiver,
            std::vector<std::uint64_t> const& tags, std::string const& why);

        struct channel;
        struct outbox;
        struct deferred;

    private:
        // a channel: the other locality, the GPU here, the GPU there
        using channel_key = std::tuple<std::uint32_t, int, int>;

        // the channel receiving from 'key', created the first time
        std::shared_ptr<channel> get_receiver(channel_key const& key);

        // the chunks held for sending to 'locality_id', created the first
        // time
        std::shared_ptr<outbox> get_outbox(std::uint32_t locality_id);

        // the channel sending to 'key', created the first time if chunks
        // were sent to the locality (also if it was removed since), nullptr
        // otherwise
        std::shared_ptr<channel> find_sender(channel_key const& key);

        // makes progress on one channel, returns whether it did anything
        bool progress(channel& c);

        // the request for a round of the receiving channel 'key' couldn't be
        // sent: fails the channel, tells the sender about the chunks 'tags'
        void round_not_sent(channel_key const& key,
            std::vector<std::uint64_t> tags, std::exception_ptr const& error);

        hpx::spinlock channels_mtx_;
        std::map<channel_key, std::shared_ptr<channel>> senders_;
        std::map<channel_key, std::shared_ptr<channel>> receivers_;
        std::map<std::uint32_t, std::shared_ptr<outbox>> outboxes_;

        // the channels and outboxes of removed localities, until they are
        // idle
        std::vector<std::shared_ptr<channel>> removed_;
        std::vector<std::shared_ptr<outbox>> removed_outboxes_;

        // the localities disconnected (removed by force), with the error
        // their sends fail with: a send that arrives after the removal gets
        // a new outbox, which fails it instead of holding the chunk
        std::map<std::uint32_t, std::exception_ptr> disconnected_;

        // completions waiting for a call of a failed channel's thread to
        // return (RCCL), run by the background work
        std::vector<std::shared_ptr<deferred>> deferred_;

        // keeps 'd' until its call has returned (nullptr: nothing to keep)
        void keep_deferred(std::shared_ptr<deferred> d);

        // a round in flight for longer than the stall timeout
        struct stalled_round
        {
            std::uint32_t peer;
            bool sending;
            int device;
            int remote_device;
            std::uint64_t number;
            std::uint64_t launch;
            std::chrono::steady_clock::duration age;
            std::vector<std::uint64_t> tags;
            std::vector<std::uint64_t> sizes;
        };

        // logs the stalled round 'r' of channel 'c' with the other rounds
        // in flight on its GPU
        void report_stall(stalled_round const& r, channel const* c);

        // see config_defaults
        std::chrono::steady_clock::duration stall_timeout_{};
        bool fail_stalled_ = false;
        std::chrono::milliseconds hold_rounds_{0};

        // numbers the launches of rounds of this process (in order)
        std::atomic<std::uint64_t> launches_{0};

        // the rounds reported as stalled
        std::atomic<std::uint64_t> stalled_rounds_{0};
    };
}    // namespace hpx::parcelset::device_conduits

#endif
