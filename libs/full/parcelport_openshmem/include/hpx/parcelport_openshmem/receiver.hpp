//  Copyright (c) 2026 Christopher Taylor
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_OPENSHMEM)
#include <hpx/assert.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/parcelport_openshmem/receiver_connection.hpp>

#include <cstddef>
#include <atomic>
#include <deque>
#include <memory>
#include <mutex>
#include <set>

#include <shmem.h>

#include <hpx/parcelport_openshmem/mailbox_array.hpp>

namespace hpx::parcelset::policies::openshmem {

    template <typename Parcelport>
    struct receiver
    {
        using connection_type = receiver_connection<Parcelport>;
        using connection_ptr = std::shared_ptr<connection_type>;
        using connection_list = std::deque<connection_ptr>;

        explicit constexpr receiver(Parcelport& pp) noexcept
          : pp_(pp)
        {
        }

        void run() noexcept {}

        // True if there are still partially received connections being
        // processed (used by do_stop()). Called only from do_stop() (never
        // from the progress thread), so a blocking lock is safe and avoids
        // spurious "empty" results under contention.  A connection whose
        // message is in-flight stays in either connections_ (re-queued
        // between steps) or active_connections_ (its src is drained by an
        // in-progress connection), so both must be checked.
        bool has_pending() noexcept
        {
            // Lock-free on purpose: do_stop() polls this every millisecond
            // from the main thread, and an unfair lock plus an io thread that
            // backs off on it can starve the io thread for the whole stop
            // timeout.  Every queued connection is also in the active set,
            // so the active count covers both.
            return pending_.load(std::memory_order_acquire) > 0;
        }

        bool background_work(std::size_t arena_idx, std::size_t arena_cnt) noexcept
        {
            bool has_work = false;

            connection_ptr connection = accept(arena_idx, arena_cnt);
            if (!connection)
            {
                std::unique_lock l(connections_mtx_, std::try_to_lock);
                if (l.owns_lock() && !connections_.empty())
                {
                    connection = HPX_MOVE(connections_.front());
                    connections_.pop_front();
                }
            }

            if (connection)
            {
                receive_messages(HPX_MOVE(connection));
                has_work = true;
            }

            return has_work;
        }

        void receive_messages(connection_ptr connection) noexcept
        {
            int const src = connection->src();
            if (!connection->receive())
            {
                std::unique_lock l(connections_mtx_);
                connections_.push_back(HPX_MOVE(connection));
            }
            else
            {
                std::unique_lock l(active_mtx_);
                active_connections_.erase(src);
                pending_.fetch_sub(1, std::memory_order_acq_rel);
            }
        }

        connection_ptr accept(std::size_t arena_idx, std::size_t arena_cnt) noexcept
        {
            auto& mailboxes = pp_.get_mailboxes();

            int const pe =
                mailboxes.try_detect_pe_notification(arena_idx, arena_cnt);
            if (pe < 0)
                return connection_ptr();

            std::size_t const src = static_cast<std::size_t>(pe);
            {
                std::unique_lock l(active_mtx_);
                if (active_connections_.count(src))
                    return connection_ptr();
                active_connections_.insert(src);
                pending_.fetch_add(1, std::memory_order_acq_rel);
            }

            // The page that triggered the detection may have been drained by
            // a connection that released this source between the probe and
            // the reservation above.  Re-check under the reservation (only
            // the reservation holder drains); otherwise the new connection
            // would wait for a page that is not coming and has_pending()
            // would keep do_stop() waiting for it.
            if (!mailboxes.page_ready(src))
            {
                std::unique_lock l(active_mtx_);
                active_connections_.erase(src);
                pending_.fetch_sub(1, std::memory_order_acq_rel);
                return connection_ptr();
            }

            return std::make_shared<connection_type>(pe, mailboxes, &pp_);
        }

        Parcelport& pp_;

        std::mutex connections_mtx_;
        connection_list connections_;

        std::mutex active_mtx_;
        std::set<int> active_connections_;
        std::atomic<int> pending_{0};
    };
}    // namespace hpx::parcelset::policies::openshmem

#endif
