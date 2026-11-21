//  Copyright (c) 2026 Christopher Taylor
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_OPENSHMEM)
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/parcelport_openshmem/sender_connection.hpp>
#include <hpx/parcelset/parcelset_fwd.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <deque>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace hpx::parcelset::policies::openshmem {

    struct sender_connection;

    struct sender
    {
        using connection_type = sender_connection;
        using connection_ptr = std::shared_ptr<connection_type>;
        using connection_list = std::deque<connection_ptr>;

        explicit sender(void* mailboxes) noexcept
          : mailboxes_(mailboxes)
        {
        }

        sender(sender const&) = delete;
        sender(sender&&) = delete;
        sender& operator=(sender const&) = delete;
        sender& operator=(sender&&) = delete;

        constexpr static void run() noexcept {}

        connection_ptr create_connection(int dest, void* pp)
        {
            auto* const mailboxes = static_cast<mailbox_array*>(pp);
            if (dest < 0 ||
                static_cast<std::size_t>(dest) >= mailboxes->num_pes())
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "openshmem::sender::create_connection",
                    "destination rank " + std::to_string(dest) +
                        " is outside the job [0, " +
                        std::to_string(mailboxes->num_pes()) + ")");
            }
            return std::make_shared<connection_type>(this, dest, mailboxes);
        }

        // Enqueue a connection to be driven by any progress thread.
        // Safe to call from any HPX thread.  A connection whose destination
        // is free takes its reservation right away and becomes steppable;
        // otherwise it waits in its destination's queue until the connection
        // holding the reservation hands it over (per-destination order is
        // kept, and background_work never scans).
        void add(connection_ptr const& ptr)
        {
            pending_.fetch_add(1, std::memory_order_acq_rel);
            std::unique_lock l(connections_mtx_);
            if (busy_dsts_.insert(ptr->dst()).second)
            {
                ptr->reserved_dst_ = true;
                connections_.push_back(ptr);
            }
            else
            {
                waiting_[ptr->dst()].push_back(ptr);
            }
        }

        // True while a transfer to the destination is in flight or waiting.
        bool busy(int dst)
        {
            std::unique_lock l(connections_mtx_);
            return busy_dsts_.count(dst) != 0;
        }

        // True if the progress thread still has queued work to empty (used
        // by do_stop()).  A connection that is in flight is always re-queued
        // after each poll, so the queue plus the set of destinations with an
        // in-flight (reserved) connection fully covers "work remaining".
        // Called only from do_stop() (never from the progress thread), so a
        // blocking lock is safe and avoids spurious "empty" results under
        // contention.
        bool has_pending() noexcept
        {
            // Lock-free, see receiver::has_pending().  Counts connections that
            // were added and have not finished yet (queued or in flight).
            return pending_.load(std::memory_order_acquire) > 0;
        }

        // Drive one connection by a single non-blocking step.  Callable from
        // any progress thread.  Every destination is single-flight: a
        // connection holds a reservation on its destination until its whole
        // transfer completes, so parcels to the same destination are
        // serialized (their chunks must not interleave in the shared
        // per-(sender,destination) rx ring).  Only connections that hold
        // their destination's reservation are in the queue; the others wait
        // per destination (see add()).  Does not block.
        bool background_work() noexcept
        {
            connection_ptr connection;
            {
                std::unique_lock l(connections_mtx_);
                if (connections_.empty())
                {
                    return false;
                }
                connection = HPX_MOVE(connections_.front());
                connections_.pop_front();
            }

            // poll_send() transfers as many chunks as credit allows in this
            // call and never blocks.  A finished connection hands its
            // destination reservation to the next connection waiting for that
            // destination (or releases it); one that still has chunks in
            // flight is re-queued with the reservation held.
            bool const finished = connection->poll_send();

            std::unique_lock l(connections_mtx_);
            if (finished)
            {
                int const dst = connection->dst();
                connection->reserved_dst_ = false;
                auto it = waiting_.find(dst);
                if (it != waiting_.end())
                {
                    connection_ptr next = HPX_MOVE(it->second.front());
                    it->second.pop_front();
                    if (it->second.empty())
                        waiting_.erase(it);
                    next->reserved_dst_ = true;
                    connections_.push_back(HPX_MOVE(next));
                }
                else
                {
                    busy_dsts_.erase(dst);
                }
                pending_.fetch_sub(1, std::memory_order_acq_rel);
                return true;
            }

            connections_.push_back(HPX_MOVE(connection));
            return true;
        }

        using parcel_buffer_type = parcel_buffer<>;
        using callback_fn_type =
            hpx::move_only_function<void(error_code const&)>;

        // Enqueue a parcel for sending; the actual transfer is driven by any
        // progress thread.  Returns immediately (non-blocking), which is
        // required since this may be called from any HPX thread.
        bool send_immediate(parcelset::locality const& dest,
            parcel_buffer_type buffer, callback_fn_type&& callbackFn)
        {
            int dest_rank = dest.get<locality>().rank();
            auto connection = create_connection(dest_rank, mailboxes_);
            connection->buffer_ = HPX_MOVE(buffer);
            connection->async_write(HPX_MOVE(callbackFn), nullptr);
            add(connection);
            return true;
        }

    private:
        void* mailboxes_;
        std::mutex connections_mtx_;

        // Connections waiting for their transfer to be driven.
        connection_list connections_;

        // Destinations currently reserved by an in-flight connection (part
        // of the single-flight protocol).  Guarded by connections_mtx_.
        std::set<int> busy_dsts_;

        // Connections waiting for their destination's reservation, per
        // destination, in order.  Guarded by connections_mtx_.
        std::map<int, connection_list> waiting_;
        std::atomic<int> pending_{0};
    };
}    // namespace hpx::parcelset::policies::openshmem

#endif
