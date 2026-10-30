//  Copyright (c) 2026 Christopher Taylor
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_GASNET)
#include <hpx/assert.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/parcelport_gasnet/mailbox_array.hpp>
#include <hpx/parcelport_gasnet/receiver_connection.hpp>

#include <atomic>
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <deque>
#include <memory>
#include <mutex>
#include <set>

namespace hpx::parcelset::policies::gasnet {

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

        constexpr void run() noexcept {}

        // True if there are still partially received connections being
        // processed (used by do_stop()). Called only from do_stop() (never
        // from the progress thread), so a blocking lock is safe and avoids
        // spurious "empty" results under contention.  A connection whose
        // message is in-flight stays in either connections_ (re-queued
        // between steps) or active_connections_ (its src is drained by an
        // in-progress connection), so both must be checked.
        void set_poll_interval(std::uint32_t n) noexcept
        {
            poll_interval_ = n == 0 ? 1 : n;
        }

        bool has_pending() noexcept
        {
            // Lock-free on purpose: do_stop() polls this every millisecond
            // from the main thread.  Taking connections_mtx_ here phase-locks
            // with an io thread that is backing off (nanosleep) on that very
            // mutex to requeue a connection, and can starve it for the whole
            // stop timeout.  Every queued connection is also in the active
            // set, so the active count covers both.
            return pending_.load(std::memory_order_acquire) > 0;
        }

        void dump_state(std::size_t my_pe) noexcept
        {
            std::unique_lock l1(connections_mtx_);
            std::unique_lock l2(active_mtx_);
            std::fprintf(stderr,
                "gasnet: PE %zu receiver: queued=%zu in_flight=%d active_src=",
                my_pe, connections_.size(), in_flight_.load());
            for (auto d : active_connections_)
                std::fprintf(stderr, "%zu ", static_cast<std::size_t>(d));
            std::fprintf(stderr, "\n");
        }

        bool background_work(std::size_t arena_idx, std::size_t arena_cnt) noexcept
        {
            bool has_work = false;

            // Keep the transport progressing even when this PE has nothing to
            // receive or send (required for AM-based RMA across nodes).  Every
            // poll_interval_-th pass only (1 = every pass): the call is a
            // contended entry into the conduit.
            if (poll_interval_ == 1 ||
                (poll_calls_.fetch_add(1, std::memory_order_relaxed) %
                    poll_interval_) == 0)
            {
                util::gasnet_environment::poll();
            }

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
            in_flight_.fetch_add(1, std::memory_order_relaxed);
            bool const done = connection->receive();
            in_flight_.fetch_sub(1, std::memory_order_relaxed);
            if (!done)
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
            // the reservation holder drains), otherwise the new connection
            // would wait forever for a page that is not coming, and
            // has_pending() would keep do_stop() waiting for it.
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
        std::atomic<int> in_flight_{0};
        std::atomic<int> pending_{0};
        std::uint32_t poll_interval_ = 1;
        std::atomic<std::uint32_t> poll_calls_{0};
    };
}    // namespace hpx::parcelset::policies::gasnet

#endif
