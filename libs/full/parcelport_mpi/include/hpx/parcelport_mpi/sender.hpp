//  Copyright (c) 2007-2024 Hartmut Kaiser
//  Copyright (c) 2014-2015 Thomas Heller
//  Copyright (c)      2023 Jiakun Yan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_MPI)
#include <hpx/assert.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/mpi_base.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/parcelport_mpi/sender_connection.hpp>
#include <hpx/parcelport_mpi/tag_provider.hpp>

#include <algorithm>
#include <iterator>
#include <list>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace hpx::parcelset::policies::mpi {

    struct sender
    {
        using connection_type = sender_connection;
        using connection_ptr = std::shared_ptr<connection_type>;
        using connection_list = std::deque<connection_ptr>;

        constexpr static void run() noexcept {}

        connection_ptr create_connection(
            int dest, parcelset::parcelport* pp, bool enable_ack_handshakes)
        {
            return std::make_shared<connection_type>(
                this, dest, pp, enable_ack_handshakes);
        }

        void add(connection_ptr const& ptr)
        {
            std::unique_lock l(connections_mtx_);
            connections_.push_back(ptr);
        }

        int acquire_tag() noexcept
        {
            return tag_provider_.get_next_tag();
        }

        void send_messages(connection_ptr connection)
        {
            // Check if sending has been completed....
            if (connection->send())
            {
                error_code const ec(throwmode::lightweight);
                hpx::move_only_function<void(error_code const&,
                    parcelset::locality const&, connection_ptr)>
                    postprocess_handler;
                std::swap(
                    postprocess_handler, connection->postprocess_handler_);
                if (postprocess_handler)
                    postprocess_handler(
                        ec, connection->destination(), connection);
            }
            else
            {
                std::unique_lock l(connections_mtx_);
                connections_.push_back(HPX_MOVE(connection));
            }
        }

        bool background_work() noexcept
        {
            connection_ptr connection;
            {
                std::unique_lock const l(connections_mtx_, std::try_to_lock);
                if (l && !connections_.empty())
                {
                    connection = HPX_MOVE(connections_.front());
                    connections_.pop_front();
                }
            }

            bool has_work = false;
            if (connection)
            {
                send_messages(HPX_MOVE(connection));
                has_work = true;
            }
            return has_work;
        }

        using parcel_buffer_type = parcel_buffer;
        using callback_fn_type =
            hpx::move_only_function<void(error_code const&)>;

        bool send_immediate(parcelset::parcelport* pp,
            parcelset::locality const& dest, parcel_buffer_type buffer,
            callback_fn_type&& callbackFn, bool enable_ack_handshakes)
        {
            int dest_rank = dest.get<locality>().rank();
            auto connection =
                create_connection(dest_rank, pp, enable_ack_handshakes);
            connection->buffer_ = HPX_MOVE(buffer);
            connection->async_write(HPX_MOVE(callbackFn), nullptr);
            return true;
        }

    private:
        tag_provider tag_provider_;
        hpx::spinlock connections_mtx_;
        connection_list connections_;
    };
}    // namespace hpx::parcelset::policies::mpi

#endif
