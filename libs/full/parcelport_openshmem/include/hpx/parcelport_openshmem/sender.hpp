//  Copyright (c) 2025 Hartmut Kaiser
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
#include <cstddef>
#include <deque>
#include <iterator>
#include <list>
#include <memory>
#include <mutex>
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
            return std::make_shared<connection_type>(
                this, dest, static_cast<mailbox_array*>(pp));
        }

        // Enqueue a connection to be driven by the single progress thread.
        // Safe to call from any HPX thread.
        void add(connection_ptr const& ptr)
        {
            std::unique_lock l(connections_mtx_);
            connections_.push_back(ptr);
        }

        // True if the progress thread still has queued work to drain (used
        // by do_stop()). A connection that is in flight is always re-queued
        // after each poll, so checking the queue is sufficient. Called only
        // from do_stop() (never from the progress thread), so a blocking
        // lock is safe and avoids spurious "empty" results under contention.
        bool has_pending() noexcept
        {
            std::unique_lock l(connections_mtx_);
            return !connections_.empty();
        }

        // Drive one connection to completion.  Called only from the single
        // progress thread.  poll_send() blocks until all chunks are sent
        // and acked (the shmem_memcpy protocol), then delivers the
        // completion callback.  Returns true if any progress was made.
        bool background_work() noexcept
        {
            connection_ptr connection;
            {
                std::unique_lock l(connections_mtx_, std::try_to_lock);
                if (l && !connections_.empty())
                {
                    connection = HPX_MOVE(connections_.front());
                    connections_.pop_front();
                }
            }

            if (!connection)
            {
                return false;
            }

            // poll_send() blocks until the entire multi-chunk transfer
            // completes (each chunk: putmem + signal + wait ack).
            connection->poll_send();
            return true;
        }

        using parcel_buffer_type = parcel_buffer<>;
        using callback_fn_type =
            hpx::move_only_function<void(error_code const&)>;

        // Enqueue a parcel for sending; the actual transfer happens on the
        // single progress thread. Returns immediately (non-blocking), which
        // is required since this may be called from any HPX thread while
        // all shmem_* calls must stay on the progress thread.
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
        connection_list connections_;
    };
}    // namespace hpx::parcelset::policies::openshmem

#endif
