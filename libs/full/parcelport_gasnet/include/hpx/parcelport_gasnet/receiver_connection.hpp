//  Copyright (c) 2026 Christopher Taylor
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_GASNET)
#include <hpx/assert.hpp>
#include <hpx/parcelport_gasnet/mailbox_array.hpp>
#include <hpx/parcelset/decode_parcels.hpp>
#include <hpx/parcelset/parcel_buffer.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <utility>
#include <vector>

namespace hpx::parcelset::policies::gasnet {

    template <typename Parcelport>
    struct receiver_connection
    {
    private:
        enum class connection_state : std::uint8_t
        {
            initialized = 1,
            rcvd_header = 2,
            collecting = 3,
            decoded = 4,
            // A consumed page failed validation.  The remaining pages of the
            // (discarded) message are skipped so the stream realigns at the
            // next message boundary.
            draining_malformed = 5,
            // Terminal: the connection produced no usable parcel.  Distinct
            // from 'decoded' so the caller knows a transfer was lost.
            failed = 6
        };

        using buffer_type = parcel_buffer<>;

    public:
        receiver_connection(int src, mailbox_array& mailboxes, Parcelport* pp)
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

            case connection_state::draining_malformed:
                return drain_malformed();

            case connection_state::decoded:
            case connection_state::failed:
                return true;

            default:
                return false;
            }
        }

    private:
        bool receive_header() noexcept
        {
            std::size_t const src_pe = static_cast<std::size_t>(src_);

            // Non-blocking: if the page is not published yet the connection
            // is re-queued unchanged and no page has been consumed.
            if (!mailboxes_.try_receive_(
                    src_pe, recv_buf_.data(), mailboxes_.mtu()))
            {
                return false;
            }

            detail::message_header header;
            std::memcpy(&header, recv_buf_.data(), sizeof(header));

            // The page is now consumed (credit returned to the sender), so a
            // failure must not lead to 'retry this page' — that would read
            // the next page and silently lose the transfer.  Handle it as a
            // terminal/loud failure instead.
            std::size_t chunk_size = 0;
            if (!detail::validate_message_page(recv_buf_.data(),
                    mailboxes_.mtu(), chunk_size,
                    mailboxes_.checksum_enabled()))
            {
                return handle_malformed_header(header);
            }

            // A connection always starts at a message boundary; a first chunk
            // with an index > 0 means the stream lost earlier pages
            // (unrecoverable corruption).  Fail loudly rather than collecting
            // a partial message.
            if (header.chunk_index != 0)
            {
                return handle_malformed_header(header);
            }

            expected_message_id_ = header.message_id;
            buffer_.size_ = header.size;
            buffer_.data_size_ = header.data_size;
            buffer_.num_chunks_ = std::make_pair(0u, 0u);

            expected_chunks_ = header.num_chunks;
            received_chunks_ = 0;

            std::size_t const header_size = detail::header_size;

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

            chunks_.clear();
            chunks_.resize(header.num_chunks);

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
            std::size_t const src_pe = static_cast<std::size_t>(src_);

            if (!mailboxes_.try_receive_(
                    src_pe, recv_buf_.data(), mailboxes_.mtu()))
            {
                return false;
            }

            detail::message_header header;
            std::memcpy(&header, recv_buf_.data(), sizeof(header));

            std::size_t chunk_size = 0;
            if (!detail::validate_message_page(recv_buf_.data(),
                    mailboxes_.mtu(), chunk_size,
                    mailboxes_.checksum_enabled()))
            {
                return handle_malformed_header(header);
            }

            // Sequence cross-checks: every continuation chunk must belong to
            // the same message and arrive strictly in order.  Any mismatch
            // means the stream lost or reordered pages — fail loudly.
            if (header.message_id != expected_message_id_ ||
                header.num_chunks != expected_chunks_ ||
                header.chunk_index != received_chunks_)
            {
                return handle_malformed_header(header);
            }

            std::size_t const header_size = detail::header_size;

            chunks_[header.chunk_index].resize(chunk_size);
            std::memcpy(chunks_[header.chunk_index].data(),
                recv_buf_.data() + header_size, chunk_size);
            received_chunks_++;

            if (received_chunks_ == expected_chunks_)
            {
                return reassemble_and_decode();
            }

            return false;
        }

        // A page was already consumed (credit returned to the sender) and its
        // header failed validation, or a first/continuation chunk broke
        // ordering guarantees.  The transfer cannot be salvaged: report it
        // loudly and, whenever the message extent is still parseable, drain
        // the message's remaining pages so the per-src stream realigns at the
        // next message boundary.  Never re-enters the failed state silently.
        bool handle_malformed_header(
            detail::message_header const& header) noexcept
        {
            std::size_t remaining = 0;
            if (detail::message_header_geometry_valid(
                    header, mailboxes_.mtu()) &&
                header.chunk_index + 1 < header.num_chunks)
            {
                remaining = header.num_chunks - header.chunk_index - 1;
            }

            std::fprintf(stderr,
                "gasnet: PE %zu: dropping malformed message from src %d "
                "(num_chunks=%u chunk_index=%u message_id=%llu "
                "size=%llu checksum=%08x, draining %zu remaining page(s))\n",
                mailboxes_.my_pe(), src_, header.num_chunks,
                header.chunk_index,
                static_cast<unsigned long long>(header.message_id),
                static_cast<unsigned long long>(header.size), header.checksum,
                remaining);

            if (remaining > 0)
            {
                to_drain_ = remaining;
                state_ = connection_state::draining_malformed;
                return false;    // re-queue; the pages are skipped below
            }

            state_ = connection_state::failed;
            return true;    // terminal; no parcel is decoded for this connection
        }

        // Skip (consume + drop) the pages of the discarded message so the
        // stream realigns.  Non-blocking: if the sender has not published the
        // next page yet, re-queue and continue later.
        bool drain_malformed() noexcept
        {
            std::size_t const src_pe = static_cast<std::size_t>(src_);

            while (to_drain_ > 0)
            {
                if (!mailboxes_.try_receive_(
                        src_pe, recv_buf_.data(), mailboxes_.mtu()))
                {
                    return false;
                }
                --to_drain_;
            }

            state_ = connection_state::failed;
            return true;
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

        // message_id of the message currently being collected (validated
        // against every continuation chunk).
        std::uint64_t expected_message_id_ = 0;

        // Pages left to skip while draining a discarded message.
        std::size_t to_drain_ = 0;

        std::vector<unsigned char> recv_buf_;
    };
}    // namespace hpx::parcelset::policies::gasnet

#endif