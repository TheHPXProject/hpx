//  Copyright (c) 2007-2021 Hartmut Kaiser
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
#include <hpx/modules/mpi_base.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/parcelport_mpi/header.hpp>
#include <hpx/parcelport_mpi/receiver_connection.hpp>

#include <algorithm>
#include <deque>
#include <memory>
#include <mutex>
#include <set>
#include <utility>
#include <vector>

namespace hpx::parcelset::policies::mpi {

    template <typename Parcelport>
    struct receiver
    {
        using connection_type = receiver_connection<Parcelport>;
        using connection_ptr = std::shared_ptr<connection_type>;
        using connection_list = std::deque<connection_ptr>;

        explicit constexpr receiver(Parcelport& pp) noexcept
          : pp_(pp)
          , hdr_request_(0)
          , header_buffer_(pp.get_zero_copy_serialization_threshold())
        {
        }

        void run() noexcept
        {
            util::mpi_environment::scoped_lock l;
            post_new_header(l);
        }

        void stop() noexcept
        {
            // Cancel the permanently re-posted wildcard header receive.
            // Leaving it pending would let MPI_Finalize run with an active
            // request on the communicator, which the MPI standard does not
            // allow and which leaves live receive state for the MPI library
            // to trip over during teardown. Errors are ignored: this runs
            // directly before MPI is finalized anyway.
            int is_finalized = 0;
            MPI_Finalized(&is_finalized);
            if (is_finalized)
            {
                return;
            }

            util::mpi_environment::scoped_lock l;
            if (hdr_posted_)
            {
                hdr_posted_ = false;
                MPI_Cancel(&hdr_request_);
                MPI_Wait(&hdr_request_, MPI_STATUS_IGNORE);
            }
        }

        bool background_work() noexcept
        {
            // We first try to accept a new connection
            connection_ptr connection = accept();

            // If we don't have a new connection, try to handle one of the
            // already accepted ones.
            if (!connection)
            {
                std::unique_lock const l(connections_mtx_, std::try_to_lock);
                if (l.owns_lock() && !connections_.empty())
                {
                    connection = HPX_MOVE(connections_.front());
                    connections_.pop_front();
                }
            }

            if (connection)
            {
                receive_messages(HPX_MOVE(connection));
                return true;
            }

            return false;
        }

        void receive_messages(connection_ptr connection) noexcept
        {
            if (!connection->receive())
            {
                std::unique_lock const l(connections_mtx_);
                connections_.push_back(HPX_MOVE(connection));
            }
        }

        connection_ptr accept() noexcept
        {
            std::unique_lock l(headers_mtx_, std::try_to_lock);
            if (l.owns_lock())
            {
                return accept_locked(l);
            }
            return connection_ptr();
        }

        template <typename Lock>
        connection_ptr accept_locked(Lock& header_lock)
        {
            HPX_ASSERT_OWNS_LOCK(header_lock);

            util::mpi_environment::scoped_try_lock l;

            // Caller failing to hold lock 'header_lock' before calling function
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26110)
#endif

            if (l.locked)
            {
                MPI_Status status;
                if (request_done_locked(l, hdr_request_, &status))
                {
                    int recv_size = 0;
                    int const ret =
                        MPI_Get_count(&status, MPI_CHAR, &recv_size);
                    util::mpi_environment::check_mpi_error(
                        l, HPX_CURRENT_SOURCE_LOCATION(), ret);

                    std::vector<char> recv_header(header_buffer_.begin(),
                        header_buffer_.begin() + recv_size);

                    post_new_header(l);

                    l.unlock();
                    header_lock.unlock();

                    return std::make_shared<connection_type>(
                        status.MPI_SOURCE, HPX_MOVE(recv_header), pp_);
                }
            }

#if defined(HPX_MSVC)
#pragma warning(pop)
#endif

            return {};
        }

        template <typename Lock>
        void post_new_header([[maybe_unused]] Lock& l) noexcept
        {
            HPX_ASSERT_OWNS_LOCK(l);
            int const ret = MPI_Irecv(header_buffer_.data(),
                static_cast<int>(header_buffer_.size()), MPI_BYTE,
                MPI_ANY_SOURCE, 0, util::mpi_environment::communicator(),
                &hdr_request_);
            util::mpi_environment::check_mpi_error(
                l, HPX_CURRENT_SOURCE_LOCATION(), ret);
            hdr_posted_ = true;
        }

        Parcelport& pp_;

        hpx::spinlock headers_mtx_;
        MPI_Request hdr_request_;
        bool hdr_posted_ = false;
        std::vector<char> header_buffer_;

        hpx::spinlock connections_mtx_;
        connection_list connections_;

        template <typename Lock>
        static bool request_done_locked(
            Lock& l, MPI_Request& r, MPI_Status* status) noexcept
        {
            HPX_ASSERT_OWNS_LOCK(l);

            int completed = 0;
            int const ret = MPI_Test(&r, &completed, status);
            util::mpi_environment::check_mpi_error(
                l, HPX_CURRENT_SOURCE_LOCATION(), ret);

            return completed ? true : false;
        }
    };
}    // namespace hpx::parcelset::policies::mpi

#endif
