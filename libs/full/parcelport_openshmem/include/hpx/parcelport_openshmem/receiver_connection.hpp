//  Copyright (c) 2026 Christopher Taylor
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_OPENSHMEM)
#include <hpx/assert.hpp>
#include <hpx/parcelset/decode_parcels.hpp>
#include <hpx/parcelset/parcel_buffer.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

namespace hpx::parcelset::policies::openshmem {

    namespace detail {

        struct message_header
        {
            std::uint64_t size;
            std::uint64_t data_size;
            std::uint32_t num_chunks;
            std::uint32_t chunk_index;
            std::uint32_t total_size_low;
            std::uint32_t total_size_high;
        };

        static_assert(sizeof(message_header) % 8 == 0,
            "message_header must be 8-byte aligned");
    }    // namespace detail

#include <hpx/parcelport_openshmem/mailbox_array.hpp>
#include <hpx/parcelset/decode_parcels.hpp>

    template <typename Parcelport>
    struct receiver_connection
    {
    private:
        enum class connection_state : std::uint8_t
        {
            initialized = 1,
            rcvd_header = 2,
            collecting = 3,
            decoded = 4
        };

        using buffer_type = parcel_buffer<>;

    public:
        receiver_connection(
            int src, mailbox_array& mailboxes, Parcelport* pp)
          : state_(connection_state::initialized)
          , src_(src)
          , mailboxes_(mailboxes)
          , pp_(pp)
          , expected_chunks_(0)
          , received_chunks_(0)
          , recv_buf_(mailboxes.mtu())
        {
        }

        constexpr int src() const noexcept
        {
            return src_;
        }

        bool is_multi_chunk() const noexcept
        {
            return expected_chunks_ > 1;
        }

        bool receive() noexcept
        {
            switch (state_)
            {
            case connection_state::initialized:
                return receive_header();

            case connection_state::rcvd_header:
                return decode_or_collect();

            case connection_state::collecting:
                return collect_chunks();

            case connection_state::decoded:
                return true;

            default:
                return false;
            }
        }

    private:
        bool receive_header() noexcept
        {
            std::size_t const src_pe =
                static_cast<std::size_t>(src_);

            if (!mailboxes_.receive_(src_pe, recv_buf_.data(), mailboxes_.mtu()))
            {
                return false;
            }

            detail::message_header header;
            std::memcpy(&header, recv_buf_.data(), sizeof(header));

            if (header.num_chunks == 0)
            {
                return false;
            }

            buffer_.size_ = header.size;
            buffer_.data_size_ = header.data_size;
            buffer_.num_chunks_ = std::make_pair(0u, 0u);

            expected_chunks_ = header.num_chunks;
            received_chunks_ = 0;

            std::size_t const header_size = sizeof(detail::message_header);

            if (header.num_chunks == 1)
            {
                if (header.size > 0)
                {
                    buffer_.data_.resize(header.size);
                    std::memcpy(buffer_.data_.data(),
                        recv_buf_.data() + header_size, header.size);
                }

                state_ = connection_state::rcvd_header;
                return decode_parcels(0);
            }

            std::size_t const mtu = mailboxes_.mtu();
            std::size_t const payload_size = mtu - header_size;

            std::uint64_t const total_size =
                (static_cast<std::uint64_t>(header.total_size_high) << 32) |
                header.total_size_low;

            chunks_.clear();
            chunks_.resize(header.num_chunks);

            if (header.chunk_index >= chunks_.size())
            {
                return false;
            }

            std::size_t const chunk_size =
                (header.chunk_index == header.num_chunks - 1) ?
                (total_size - (header.chunk_index * payload_size)) :
                payload_size;

            chunks_[header.chunk_index].resize(chunk_size);
            std::memcpy(chunks_[header.chunk_index].data(),
                recv_buf_.data() + header_size, chunk_size);
            received_chunks_++;

            if (received_chunks_ == expected_chunks_)
            {
                return reassemble_and_decode();
            }

            state_ = connection_state::collecting;
            return false;
        }

        bool decode_or_collect() noexcept
        {
            if (expected_chunks_ > 1)
            {
                return collect_chunks();
            }
            return decode_parcels(0);
        }

        bool collect_chunks() noexcept
        {
            std::size_t const src_pe =
                static_cast<std::size_t>(src_);

            if (!mailboxes_.receive_(src_pe, recv_buf_.data(), mailboxes_.mtu()))
            {
                return false;
            }

            detail::message_header header;
            std::memcpy(&header, recv_buf_.data(), sizeof(header));

            std::size_t const header_size = sizeof(detail::message_header);
            std::size_t const mtu = mailboxes_.mtu();
            std::size_t const payload_size = mtu - header_size;

            std::uint64_t const total_size =
                (static_cast<std::uint64_t>(header.total_size_high) << 32) |
                header.total_size_low;

            if (chunks_.empty())
            {
                chunks_.resize(header.num_chunks);
                expected_chunks_ = header.num_chunks;
            }

            if (header.chunk_index < chunks_.size())
            {
                std::size_t const chunk_size =
                    (header.chunk_index == header.num_chunks - 1) ?
                    (total_size - (header.chunk_index * payload_size)) :
                    payload_size;

                if (chunks_[header.chunk_index].empty())
                {
                    chunks_[header.chunk_index].resize(chunk_size);
                    std::memcpy(chunks_[header.chunk_index].data(),
                        recv_buf_.data() + header_size, chunk_size);
                    received_chunks_++;
                }
            }

            if (received_chunks_ == expected_chunks_)
            {
                return reassemble_and_decode();
            }

            return false;
        }

        bool reassemble_and_decode() noexcept
        {
            std::size_t total_size = 0;
            for (auto const& chunk : chunks_)
            {
                total_size += chunk.size();
            }

            buffer_.data_.resize(total_size);
            std::size_t offset = 0;
            for (auto const& chunk : chunks_)
            {
                std::memcpy(
                    buffer_.data_.data() + offset, chunk.data(), chunk.size());
                offset += chunk.size();
            }

            chunks_.clear();

            return decode_parcels(0);
        }

        bool decode_parcels(std::size_t num_thread) noexcept
        {
            HPX_ASSERT(!buffer_.data_.empty());

            std::vector<parcel> parcels = hpx::parcelset::decode_parcels(
                *pp_, HPX_MOVE(buffer_), num_thread);

            hpx::parcelset::handle_received_parcels(
                HPX_MOVE(parcels), num_thread);

            state_ = connection_state::decoded;
            return true;
        }

        connection_state state_;
        int src_;
        mailbox_array& mailboxes_;
        Parcelport* pp_;

        buffer_type buffer_;
        std::vector<std::vector<char>> chunks_;
        std::uint32_t expected_chunks_;
        std::uint32_t received_chunks_;
        std::vector<unsigned char> recv_buf_;
    };
}    // namespace hpx::parcelset::policies::openshmem

#endif
