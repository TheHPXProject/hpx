//  Copyright (c) 2007-2024 Hartmut Kaiser
//  Copyright (c) 2014-2015 Thomas Heller
//  Copyright (c) 2023-2024 Jiakun Yan
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
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/parcelport_mpi/header.hpp>
#include <hpx/parcelport_mpi/locality.hpp>
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
#include <hpx/modules/timing.hpp>
#endif

#include <cstddef>
#include <cstdint>
#include <memory>
#include <system_error>
#include <utility>
#include <vector>

namespace hpx::parcelset::policies::mpi {

    struct sender;
    struct sender_connection;

    int acquire_tag(sender*) noexcept;
    void add_connection(sender*, std::shared_ptr<sender_connection> const&);

    struct sender_connection
      : parcelset::parcelport_connection<sender_connection>
    {
    private:
        using sender_type = sender;

        using write_handler_type =
            hpx::function<void(std::error_code const&, parcel const&)>;

        enum class connection_state : std::uint8_t
        {
            initialized = 0,
            sent_header = 1,
            sent_transmission_chunks = 2,
            sent_data = 3,
            sent_chunks = 4,

            acked_transmission_chunks = 5,
            acked_data = 6
        };

        using base_type = parcelset::parcelport_connection<sender_connection>;

    public:
        sender_connection(sender_type* s, int dst, parcelset::parcelport* pp,
            bool enable_ack_handshakes)
          : state_(connection_state::initialized)
          , sender_(s)
          , tag_(-1)
          , dst_(dst)
          , request_(MPI_REQUEST_NULL)
          , request_ptr_(nullptr)
          , chunks_idx_(0)
          , needs_ack_handshake_(enable_ack_handshakes)
          , ack_(0)
          , pp_(pp)
          , there_(parcelset::locality(locality(dst_)))
        {
        }

        constexpr parcelset::locality const& destination() const noexcept
        {
            return there_;
        }

        static constexpr void verify_(
            parcelset::locality const& /* parcel_locality_id */) noexcept
        {
        }

        constexpr int ack_tag() const noexcept
        {
            return static_cast<int>(tag_ | util::mpi_environment::MPI_ACK_TAG);
        }

        using handler_type = hpx::move_only_function<void(error_code const&)>;
        using post_handler_type = hpx::move_only_function<void(
            error_code const&, parcelset::locality const&,
            std::shared_ptr<sender_connection>)>;
        void async_write(
            handler_type&& handler, post_handler_type&& parcel_postprocess)
        {
            HPX_ASSERT(!handler_);
            HPX_ASSERT(!buffer_.data_.empty());

#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
            buffer_.data_point_.time_ = static_cast<std::int64_t>(
                hpx::chrono::high_resolution_clock::now());
#endif
            request_ptr_ = nullptr;
            chunks_idx_ = 0;
            tag_ = acquire_tag(sender_);
            header_buffer.resize(header::get_header_size(
                buffer_, pp_->get_zero_copy_serialization_threshold()));

            header_ =
                header(buffer_, header_buffer.data(), header_buffer.size());
            header_.set_tag(tag_);
            header_.set_ack_handshakes(needs_ack_handshake_);
            header_.assert_valid();

            state_ = connection_state::initialized;

            handler_ = HPX_MOVE(handler);

            if (!send())
            {
                postprocess_handler_ = HPX_MOVE(parcel_postprocess);
                add_connection(sender_, shared_from_this());
            }
            else
            {
                HPX_ASSERT(!handler_);
                error_code ec;
                if (parcel_postprocess)
                    parcel_postprocess(ec, there_, shared_from_this());
            }
        }

        bool send()
        {
            switch (state_)
            {
            case connection_state::initialized:
                return send_header();

            case connection_state::sent_header:
                return send_transmission_chunks();

            case connection_state::sent_transmission_chunks:
                return ack_transmission_chunks();

            case connection_state::sent_data:
                return ack_data();

            case connection_state::sent_chunks:
                return done();

            case connection_state::acked_transmission_chunks:
                return send_data();

            case connection_state::acked_data:
                return send_chunks();

            default:
                HPX_ASSERT(false);
            }
            return false;
        }

        bool send_header()
        {
            HPX_ASSERT(state_ == connection_state::initialized);
            HPX_ASSERT(request_ptr_ == nullptr);

            request_ = util::mpi_environment::isend(
                header_buffer.data(), header_buffer.size(), dst_, 0);
            request_ptr_ = &request_;

            state_ = connection_state::sent_header;
            return send_transmission_chunks();
        }

        bool send_transmission_chunks()
        {
            HPX_ASSERT(state_ == connection_state::sent_header);

            if (!request_done())
            {
                return false;
            }
            HPX_ASSERT(request_ptr_ == nullptr);

            auto const& chunks = buffer_.transmission_chunks_;
            if (!chunks.empty() && !header_.piggy_back_tchunk())
            {
                request_ = util::mpi_environment::isend(chunks.data(),
                    chunks.size() *
                        sizeof(parcel_buffer_type::transmission_chunk_type),
                    dst_, tag_);
                request_ptr_ = &request_;

                state_ = connection_state::sent_transmission_chunks;
                return ack_transmission_chunks();
            }

            // no need to acknowledge the transmission chunks
            state_ = connection_state::sent_transmission_chunks;
            return send_data();
        }

        constexpr bool need_ack_transmission_chunks() const noexcept
        {
            auto const& chunks = buffer_.transmission_chunks_;
            return needs_ack_handshake_ && !chunks.empty() &&
                !header_.piggy_back_tchunk();
        }

        bool ack_transmission_chunks()
        {
            if (!need_ack_transmission_chunks())
            {
                return send_data();
            }

            HPX_ASSERT(state_ == connection_state::sent_transmission_chunks);

            if (!request_done())
            {
                return false;
            }
            HPX_ASSERT(request_ptr_ == nullptr);

            {
                request_ = util::mpi_environment::irecv(
                    &ack_, sizeof(ack_), dst_, ack_tag());
                request_ptr_ = &request_;
            }

            state_ = connection_state::acked_transmission_chunks;
            return send_data();
        }

        bool send_data()
        {
            HPX_ASSERT(
                (need_ack_transmission_chunks() &&
                    state_ == connection_state::acked_transmission_chunks) ||
                (!need_ack_transmission_chunks() &&
                    state_ == connection_state::sent_transmission_chunks));

            if (!request_done())
            {
                return false;
            }
            HPX_ASSERT(request_ptr_ == nullptr);

            if (!header_.piggy_back_data())
            {
                request_ = util::mpi_environment::isend(
                    buffer_.data_.data(), buffer_.data_.size(), dst_, tag_);
                request_ptr_ = &request_;

                state_ = connection_state::sent_data;
                return ack_data();
            }

            // no need to acknowledge the data sent
            state_ = connection_state::sent_data;
            return send_chunks();
        }

        constexpr bool need_ack_data() const noexcept
        {
            return needs_ack_handshake_ && !header_.piggy_back_data();
        }

        bool ack_data()
        {
            if (!need_ack_data())
            {
                return send_chunks();
            }

            HPX_ASSERT(state_ == connection_state::sent_data);

            if (!request_done())
            {
                return false;
            }
            HPX_ASSERT(request_ptr_ == nullptr);

            {
                request_ = util::mpi_environment::irecv(
                    &ack_, sizeof(ack_), dst_, ack_tag());
                request_ptr_ = &request_;
            }

            state_ = connection_state::acked_data;
            return send_chunks();
        }

        bool send_chunks()
        {
            HPX_ASSERT(
                (!need_ack_data() && state_ == connection_state::sent_data) ||
                (need_ack_data() && state_ == connection_state::acked_data));

            while (chunks_idx_ < buffer_.chunks_.size())
            {
                auto const& c = buffer_.chunks_[chunks_idx_];
                if (c.type_ == serialization::chunk_type::chunk_type_pointer ||
                    c.type_ ==
                        serialization::chunk_type::chunk_type_const_pointer)
                {
                    if (!request_done())
                    {
                        return false;
                    }
                    HPX_ASSERT(request_ptr_ == nullptr);
                    request_ = util::mpi_environment::isend(
                        c.data(), c.size(), dst_, tag_);
                    request_ptr_ = &request_;
                }

                ++chunks_idx_;
            }

            state_ = connection_state::sent_chunks;
            return done();
        }

        bool done()
        {
            HPX_ASSERT(state_ == connection_state::sent_chunks);

            if (!request_done())
            {
                return false;
            }
            HPX_ASSERT(request_ptr_ == nullptr);

            error_code const ec(throwmode::lightweight);
            handler_(ec);
            handler_.reset();

#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
            buffer_.data_point_.time_ =
                static_cast<std::int64_t>(
                    hpx::chrono::high_resolution_clock::now()) -
                buffer_.data_point_.time_;
            pp_->add_sent_data(buffer_.data_point_);
#endif
            buffer_.clear();

            state_ = connection_state::initialized;
            return true;
        }

        bool request_done()
        {
            if (request_ptr_ == nullptr)
            {
                return true;
            }

            util::mpi_environment::scoped_try_lock l;
            if (!l.locked)
            {
                return false;
            }

            int completed = 0;
            int const ret =
                MPI_Test(request_ptr_, &completed, MPI_STATUS_IGNORE);
            util::mpi_environment::check_mpi_error(
                l, HPX_CURRENT_SOURCE_LOCATION(), ret);

            if (completed)
            {
                request_ptr_ = nullptr;
                return true;
            }
            return false;
        }

        connection_state state_;
        sender_type* sender_;
        int tag_;
        int dst_;

        handler_type handler_;
        post_handler_type postprocess_handler_;

        std::vector<char> header_buffer;
        header header_;

        MPI_Request request_;
        MPI_Request* request_ptr_;
        std::size_t chunks_idx_;

        bool needs_ack_handshake_;
        char ack_;

        parcelset::parcelport* pp_;

        parcelset::locality there_;
    };
}    // namespace hpx::parcelset::policies::mpi

#endif
