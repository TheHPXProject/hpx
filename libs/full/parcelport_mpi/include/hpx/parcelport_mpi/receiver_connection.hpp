//  Copyright (c) 2014-2015 Thomas Heller
//  Copyright (c) 2007-2024 Hartmut Kaiser
//  Copyright (c) 2023-2024 Jiakun Yan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_MPI)
#include <hpx/assert.hpp>
#include <hpx/modules/mpi_base.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/parcelport_mpi/header.hpp>
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
#include <hpx/modules/timing.hpp>
#endif

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

namespace hpx::parcelset::policies::mpi {

    template <typename Parcelport>
    struct receiver_connection
    {
    private:
        enum class connection_state : std::uint8_t
        {
            initialized = 1,
            rcvd_transmission_chunks = 2,
            rcvd_data = 3,
            rcvd_chunks = 4,

            acked_transmission_chunks = 5,
            acked_data = 6
        };

        using buffer_type = parcel_buffer;

        constexpr int ack_tag() const noexcept
        {
            return static_cast<int>(tag_ | util::mpi_environment::MPI_ACK_TAG);
        }

    public:
        receiver_connection(
            int src, std::vector<char> header_buffer, Parcelport& pp)
          : state_(connection_state::initialized)
          , src_(src)
          , request_(MPI_REQUEST_NULL)
          , request_ptr_(nullptr)
          , chunks_idx_(0)
          , zero_copy_chunks_idx_(0)
          , needs_ack_handshake_(false)
          , ack_(0)
          , pp_(pp)
        {
            header header_ = header(header_buffer.data());
            header_.assert_valid();

#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
            parcelset::data_point& data = buffer_.data_point_;
            data.time_ = timer_.elapsed_nanoseconds();
            data.bytes_ = static_cast<std::size_t>(header_.numbytes());
#endif
            tag_ = header_.get_tag();
            needs_ack_handshake_ = header_.get_ack_handshakes();

            // decode data
            buffer_.data_.resize(header_.numbytes_nonzero_copy());
            if (char* piggy_back_data = header_.piggy_back_data())
            {
                need_recv_data = false;
                memcpy(buffer_.data_.data(), piggy_back_data,
                    buffer_.data_.size());
            }
            else
            {
                need_recv_data = true;
            }

            need_recv_tchunks = false;
            if (header_.num_zero_copy_chunks() != 0)
            {
                // decode transmission chunk
                int const num_zero_copy_chunks = header_.num_zero_copy_chunks();
                int const num_non_zero_copy_chunks =
                    header_.num_non_zero_copy_chunks();
                buffer_.num_chunks_.first = num_zero_copy_chunks;
                buffer_.num_chunks_.second = num_non_zero_copy_chunks;

                auto& tchunks = buffer_.transmission_chunks_;
                tchunks.resize(num_zero_copy_chunks + num_non_zero_copy_chunks);
                if (char const* piggy_back_tchunk = header_.piggy_back_tchunk())
                {
#if defined(HPX_GCC_VERSION) && !defined(HPX_CLANG_VERSION)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif
                    int const tchunks_length = static_cast<int>(tchunks.size() *
                        sizeof(buffer_type::transmission_chunk_type));
                    memcpy(static_cast<void*>(tchunks.data()),
                        piggy_back_tchunk, tchunks_length);

#if defined(HPX_GCC_VERSION) && !defined(HPX_CLANG_VERSION)
#pragma GCC diagnostic pop
#endif
                }
                else
                {
                    need_recv_tchunks = true;
                }

                // zero-copy chunks
                buffer_.chunks_.resize(num_zero_copy_chunks);
                if (!pp_.allow_zero_copy_receive_optimizations())
                {
                    chunk_buffers_.resize(num_zero_copy_chunks);
                }
            }
        }

        bool receive(std::size_t num_thread = -1)
        {
            switch (state_)
            {
            case connection_state::initialized:
                return receive_transmission_chunks(num_thread);

            case connection_state::rcvd_transmission_chunks:
                return ack_transmission_chunks(num_thread);

            case connection_state::rcvd_data:
                return ack_data(num_thread);

            case connection_state::rcvd_chunks:
                return done(num_thread);

            case connection_state::acked_transmission_chunks:
                return receive_data(num_thread);

            case connection_state::acked_data:
                return receive_chunks(num_thread);

            default:
                HPX_ASSERT(false);
            }
            return false;
        }

        bool receive_transmission_chunks(std::size_t num_thread = -1)
        {
            if (need_recv_tchunks)
            {
                request_ = util::mpi_environment::irecv(
                    buffer_.transmission_chunks_.data(),
                    buffer_.transmission_chunks_.size() *
                        sizeof(buffer_type::transmission_chunk_type),
                    src_, tag_);
                request_ptr_ = &request_;

                state_ = connection_state::rcvd_transmission_chunks;
                return ack_transmission_chunks(num_thread);
            }

            // no need to acknowledge the transmission chunks
            state_ = connection_state::rcvd_transmission_chunks;
            return receive_data(num_thread);
        }

        constexpr bool need_ack_transmission_chunks() const noexcept
        {
            return needs_ack_handshake_ && need_recv_tchunks;
        }

        bool ack_transmission_chunks(std::size_t num_thread = -1)
        {
            if (!need_ack_transmission_chunks())
            {
                return receive_data(num_thread);
            }

            HPX_ASSERT(state_ == connection_state::rcvd_transmission_chunks);

            if (!request_done())
            {
                return false;
            }
            HPX_ASSERT(request_ptr_ == nullptr);

            {
                ack_ = static_cast<char>(
                    connection_state::acked_transmission_chunks);
                request_ = util::mpi_environment::isend(
                    &ack_, sizeof(ack_), src_, ack_tag());
                request_ptr_ = &request_;
            }

            state_ = connection_state::acked_transmission_chunks;
            return receive_data(num_thread);
        }

        constexpr bool need_ack_data() const noexcept
        {
            return needs_ack_handshake_ && need_recv_data;
        }

        bool receive_data(std::size_t num_thread = -1)
        {
            HPX_ASSERT(
                (!need_ack_transmission_chunks() &&
                    state_ == connection_state::rcvd_transmission_chunks) ||
                (need_ack_transmission_chunks() &&
                    state_ == connection_state::acked_transmission_chunks));

            if (!request_done())
            {
                return false;
            }
            HPX_ASSERT(request_ptr_ == nullptr);

            if (need_recv_data)
            {
                request_ = util::mpi_environment::irecv(
                    buffer_.data_.data(), buffer_.data_.size(), src_, tag_);
                request_ptr_ = &request_;

                state_ = connection_state::rcvd_data;
                return ack_data(num_thread);
            }

            // no need to acknowledge the data sent
            state_ = connection_state::rcvd_data;
            return receive_chunks(num_thread);
        }

        bool ack_data(std::size_t num_thread = -1)
        {
            if (!need_ack_data())
            {
                return receive_chunks(num_thread);
            }

            HPX_ASSERT(state_ == connection_state::rcvd_data);

            if (!request_done())
            {
                return false;
            }
            HPX_ASSERT(request_ptr_ == nullptr);

            {
                request_ = util::mpi_environment::isend(
                    &ack_, sizeof(ack_), src_, ack_tag());
                request_ptr_ = &request_;
            }

            state_ = connection_state::acked_data;
            return receive_chunks(num_thread);
        }

        bool receive_chunks(std::size_t num_thread = -1)
        {
            HPX_ASSERT(
                (!need_ack_data() && state_ == connection_state::rcvd_data) ||
                (need_ack_data() && state_ == connection_state::acked_data));

            if (pp_.allow_zero_copy_receive_optimizations())
            {
                if (!request_done())
                {
                    return false;
                }
                HPX_ASSERT(request_ptr_ == nullptr);

                // handle zero-copy receive, this should be done on the first entry
                // to receive_chunks only
                if (parcels_.empty())
                {
                    HPX_ASSERT(zero_copy_chunks_idx_ == 0);

                    auto const num_zero_copy_chunks =
                        static_cast<std::size_t>(buffer_.num_chunks_.first);
                    if (num_zero_copy_chunks != 0)
                    {
                        HPX_ASSERT(
                            buffer_.chunks_.size() == num_zero_copy_chunks);

                        // De-serialize the parcels such that all data but the
                        // zero-copy chunks are in place. This de-serialization
                        // also allocates all zero-chunk buffers and stores
                        // those in the chunks array for the subsequent
                        // networking to place the received data directly.
                        for (std::size_t i = 0; i != num_zero_copy_chunks; ++i)
                        {
                            auto const chunk_size = static_cast<std::size_t>(
                                buffer_.transmission_chunks_[i].second);
                            buffer_.chunks_[i] =
                                serialization::create_pointer_chunk(
                                    nullptr, chunk_size);
                        }

                        parcels_ =
                            decode_parcels_zero_copy(pp_, buffer_, num_thread);

                        // note that at this point, buffer_.chunks_ will have
                        // entries for all chunks, including the non-zero-copy ones
                    }

                    // we should have received at least one parcel if there are
                    // zero-copy chunks to be received
                    HPX_ASSERT(parcels_.empty() || !buffer_.chunks_.empty());
                }

                while (chunks_idx_ != buffer_.chunks_.size())
                {
                    if (!request_done())
                    {
                        return false;
                    }
                    HPX_ASSERT(request_ptr_ == nullptr);

                    auto& c = buffer_.chunks_[chunks_idx_++];
                    if (c.type_ == serialization::chunk_type::chunk_type_index)
                    {
                        continue;    // skip non-zero-copy chunks
                    }

                    // the zero-copy chunks come first in the transmission_chunks_
                    // array
                    auto const chunk_size =
                        buffer_.transmission_chunks_[zero_copy_chunks_idx_++]
                            .second;

                    // the parcel de-serialization above should have allocated the
                    // correct amount of memory
                    HPX_ASSERT_MSG(
                        c.data() != nullptr && c.size() == chunk_size,
                        "zero-copy chunk buffers should have been initialized "
                        "during de-serialization");

                    request_ = util::mpi_environment::irecv(
                        c.data(), chunk_size, src_, tag_);
                    request_ptr_ = &request_;
                }
                HPX_ASSERT_MSG(
                    zero_copy_chunks_idx_ == buffer_.num_chunks_.first,
                    "observed: {}, expected {}", zero_copy_chunks_idx_,
                    buffer_.num_chunks_.first);
            }
            else
            {
                HPX_ASSERT(chunk_buffers_.size() == buffer_.chunks_.size());
                while (chunks_idx_ < buffer_.chunks_.size())
                {
                    if (!request_done())
                    {
                        return false;
                    }
                    HPX_ASSERT(request_ptr_ == nullptr);

                    std::size_t const idx = chunks_idx_++;
                    std::size_t const chunk_size =
                        buffer_.transmission_chunks_[idx].second;

                    auto& c = chunk_buffers_[idx];
                    c.resize(chunk_size);

                    // store buffer for decode_parcels below
                    buffer_.chunks_[idx] = serialization::create_pointer_chunk(
                        c.data(), chunk_size);

                    {
                        request_ = util::mpi_environment::irecv(
                            c.data(), c.size(), src_, tag_);
                        request_ptr_ = &request_;
                    }
                }
            }

            state_ = connection_state::rcvd_chunks;
            return done(num_thread);
        }

        bool done(std::size_t num_thread = -1) noexcept
        {
            HPX_ASSERT(state_ == connection_state::rcvd_chunks);

            if (!request_done())
            {
                return false;
            }
            HPX_ASSERT(request_ptr_ == nullptr);

#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
            parcelset::data_point& data = buffer_.data_point_;
            data.time_ = timer_.elapsed_nanoseconds() - data.time_;
#endif
            if (parcels_.empty())
            {
                // decode and handle received data
                HPX_ASSERT(buffer_.num_chunks_.first == 0 ||
                    !pp_.allow_zero_copy_receive_optimizations());
                handle_received_parcels(
                    decode_parcels(pp_, HPX_MOVE(buffer_), num_thread),
                    num_thread);
                chunk_buffers_.clear();
            }
            else
            {
                // handle the received zero-copy parcels.
                HPX_ASSERT(buffer_.num_chunks_.first != 0 &&
                    pp_.allow_zero_copy_receive_optimizations());
                handle_received_parcels(HPX_MOVE(parcels_));
                buffer_ = buffer_type{};
            }
            return true;
        }

        bool request_done() noexcept
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

#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
        hpx::chrono::high_resolution_timer timer_;
#endif
        connection_state state_;

        int src_;
        int tag_;
        bool need_recv_data;
        bool need_recv_tchunks;
        buffer_type buffer_;

        MPI_Request request_;
        MPI_Request* request_ptr_;
        std::size_t chunks_idx_;
        std::size_t zero_copy_chunks_idx_;

        bool needs_ack_handshake_;
        char ack_;

        Parcelport& pp_;

        std::vector<parcelset::parcel> parcels_;
        std::vector<std::vector<char>> chunk_buffers_;
    };
}    // namespace hpx::parcelset::policies::mpi

#endif
