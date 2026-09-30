//  Copyright (c) 2026 Christopher Taylor
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_GASNET)
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/parcelport_gasnet/sender_connection.hpp>
#include <hpx/parcelset/parcelset_fwd.hpp>

#include <algorithm>
#include <cstddef>
#include <deque>
#include <iterator>
#include <list>
#include <memory>
#include <mutex>
#include <set>
#include <utility>
#include <vector>

namespace hpx::parcelset::policies::gasnet {

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
            return std::make_shared<connection_type>(
                this, dest, static_cast<mailbox_array*>(pp));
        }

        // Enqueue a connection to be driven by any progress thread.
        // Safe to call from any HPX thread.
        void add(connection_ptr const& ptr)
        {
            std::unique_lock l(connections_mtx_);
            connections_.push_back(ptr);
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
            std::unique_lock l(connections_mtx_);
            return !connections_.empty() || !busy_dsts_.empty();
        }

        // Drive one connection by a single non-blocking step.  Callable from
        // any progress thread.  Every destination is single-flight: a
        // connection holds a reservation on its destination until its whole
        // transfer completes, so parcels to the same destination are
        // serialized (their chunks must not interleave in the shared
        // per-(sender,destination) rx ring).  Connections whose destination
        // is already reserved by another in-flight connection are skipped
        // and re-queued — a caller whose destination cannot do work right
        // now moves on to the next connection. Does not block.
        bool background_work() noexcept
        {
            connection_ptr connection;
            {
                std::unique_lock l(connections_mtx_);

                std::size_t const n = connections_.size();
                for (std::size_t i = 0; i < n && !connection; ++i)
                {
                    connection_ptr c = HPX_MOVE(connections_.front());
                    connections_.pop_front();

                    // Steppable if it already owns its destination
                    // reservation, or its destination is not reserved by
                    // another, still in-flight connection.
                    if (c->reserved_dst_ || !busy_dsts_.count(c->dst()))
                    {
                        if (!c->reserved_dst_)
                        {
                            busy_dsts_.insert(c->dst());
                            c->reserved_dst_ = true;
                        }
                        connection = HPX_MOVE(c);
                    }
                    else
                    {
                        connections_.push_back(HPX_MOVE(c));
                    }
                }
            }

            if (!connection)
            {
                return false;
            }

            // poll_send() transfers as many chunks as the transport admits in
            // this call and never blocks (each chunk needs one region, and
            // only one region per destination may be in flight).  A finished
            // connection releases its destination reservation; one that still
            // has chunks in flight is re-queued with the reservation held.
            bool const finished = connection->poll_send();
            if (finished)
            {
                std::unique_lock l(connections_mtx_);
                busy_dsts_.erase(connection->dst());
                connection->reserved_dst_ = false;
                return true;
            }

            std::unique_lock l(connections_mtx_);
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
        hpx::spinlock connections_mtx_;

        // Connections waiting for their transfer to be driven.
        connection_list connections_;

        // Destinations currently reserved by an in-flight connection (part
        // of the single-flight protocol).  Guarded by connections_mtx_.
        std::set<int> busy_dsts_;
    };
}    // namespace hpx::parcelset::policies::gasnet

#endif
