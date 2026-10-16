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

        explicit receiver(Parcelport& pp) noexcept
          : pp_(pp)
        {
        }

        void run() noexcept
        {
            std::size_t num_slots = pp_.get_max_header_receives();
            if (num_slots == 0)
            {
                std::size_t const env_size = util::mpi_environment::size() > 0 ?
                    static_cast<std::size_t>(util::mpi_environment::size()) :
                    1;
                num_slots = (std::min) (static_cast<std::size_t>(64),
                    (std::max) (static_cast<std::size_t>(16), env_size * 2));
            }

            std::size_t const buf_size =
                pp_.get_zero_copy_serialization_threshold();

            hdr_requests_.assign(num_slots, MPI_REQUEST_NULL);
            header_buffers_.resize(num_slots);
            for (auto& buf : header_buffers_)
            {
                buf.resize(buf_size);
            }

            util::mpi_environment::scoped_lock l;
            for (std::size_t i = 0; i < num_slots; ++i)
            {
                post_header(l, i);
            }
            hdr_posted_ = true;
        }

        void stop() noexcept
        {
            // Cancel the permanently re-posted wildcard header receives.
            // Leaving them pending would let MPI_Finalize run with active
            // requests on the communicator, which the MPI standard does not
            // allow and which leaves live receive state for the MPI library
            // to trip over during teardown. Errors are ignored: this runs
            // directly before MPI is finalized anyway.
            int is_finalized = 0;
            MPI_Finalized(&is_finalized);
            if (is_finalized)
            {
                return;
            }

            // Serialize against any late header polling. accept_locked()
            // and its re-post of the header receive run under headers_mtx_,
            // so holding it here gives stop() exclusive ownership of
            // hdr_requests_ and hdr_posted_ in all MPI threading modes (the
            // mpi_environment lock alone is a no-op for multithreaded MPI).
            // Lock order matches accept(): headers_mtx_ first, then the
            // MPI serialization lock.
            std::unique_lock const header_lock(headers_mtx_);

            util::mpi_environment::scoped_lock l;
            if (hdr_posted_)
            {
                hdr_posted_ = false;
                for (auto& req : hdr_requests_)
                {
                    if (req != MPI_REQUEST_NULL)
                    {
                        MPI_Cancel(&req);
                        MPI_Wait(&req, MPI_STATUS_IGNORE);
                        req = MPI_REQUEST_NULL;
                    }
                }
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

            if (hdr_requests_.empty())
            {
                return {};
            }

            util::mpi_environment::scoped_try_lock l;

            // Caller failing to hold lock 'header_lock' before calling function
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26110)
#endif

            if (l.locked)
            {
                int index = MPI_UNDEFINED;
                int completed = 0;
                MPI_Status status;
                // Multiple wildcard receives may complete out-of-order with
                // respect to their sending order when polled via MPI_Testany.
                // HPX parcels are independent, asynchronous messages that do
                // not require per-sender FIFO delivery at the parcelport layer;
                // each header carries a distinct sender-allocated tag and
                // self-contained metadata to create its receiver_connection.
                int const ret =
                    MPI_Testany(static_cast<int>(hdr_requests_.size()),
                        hdr_requests_.data(), &index, &completed, &status);
                util::mpi_environment::check_mpi_error(
                    l, HPX_CURRENT_SOURCE_LOCATION(), ret);

                if (completed && index != MPI_UNDEFINED)
                {
                    int recv_size = 0;
                    int const count_ret =
                        MPI_Get_count(&status, MPI_CHAR, &recv_size);
                    util::mpi_environment::check_mpi_error(
                        l, HPX_CURRENT_SOURCE_LOCATION(), count_ret);

                    std::vector<char> recv_header(
                        header_buffers_[index].begin(),
                        header_buffers_[index].begin() + recv_size);

                    post_header(l, static_cast<std::size_t>(index));

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
        void post_header([[maybe_unused]] Lock& l, std::size_t idx) noexcept
        {
            HPX_ASSERT_OWNS_LOCK(l);
            HPX_ASSERT(idx < hdr_requests_.size());
            int const ret = MPI_Irecv(header_buffers_[idx].data(),
                static_cast<int>(header_buffers_[idx].size()), MPI_BYTE,
                MPI_ANY_SOURCE, 0, util::mpi_environment::communicator(),
                &hdr_requests_[idx]);
            util::mpi_environment::check_mpi_error(
                l, HPX_CURRENT_SOURCE_LOCATION(), ret);
        }

        Parcelport& pp_;

        hpx::spinlock headers_mtx_;
        std::vector<MPI_Request> hdr_requests_;
        bool hdr_posted_ = false;
        std::vector<std::vector<char>> header_buffers_;

        hpx::spinlock connections_mtx_;
        connection_list connections_;
    };
}    // namespace hpx::parcelset::policies::mpi

#endif
