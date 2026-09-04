//  Copyright (c) 2025 Hartmut Kaiser
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
#include <deque>
#include <memory>
#include <set>

#include <shmem.h>

namespace hpx::parcelset::policies::openshmem {

#include <hpx/parcelport_openshmem/mailbox_array.hpp>

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
        // spurious "empty" results under contention.
        bool has_pending() noexcept
        {
            std::unique_lock l(connections_mtx_);
            return !connections_.empty();
        }

        bool background_work() noexcept
        {
            bool has_work = false;

            connection_ptr connection = accept();
            if (connection)
            {
                receive_messages(HPX_MOVE(connection));
                return true;
            }

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
            }
        }

        connection_ptr accept() noexcept
        {
            auto& mailboxes = pp_.get_mailboxes();

            int const pe = mailboxes.try_detect_pe_notification();
            if (pe < 0)
                return connection_ptr();

            std::size_t const src = static_cast<std::size_t>(pe);
            {
                std::unique_lock l(active_mtx_);
                if (active_connections_.count(src))
                    return connection_ptr();
                active_connections_.insert(src);
            }

            return std::make_shared<connection_type>(
                pe, mailboxes, &pp_);
        }

        Parcelport& pp_;

        hpx::spinlock connections_mtx_;
        connection_list connections_;

        hpx::spinlock active_mtx_;
        std::set<int> active_connections_;
    };
}    // namespace hpx::parcelset::policies::openshmem

#endif
