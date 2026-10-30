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
#include <exception>
#include <memory>
#include <algorithm>
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
            failed = 6,
            // The last page of the message has been consumed (its credit
            // returned via an NBI region) but that region is still in flight.
            // The connection re-queues here until it completes, keeping the
            // source single-flighted, then moves on to the terminal state
            // recorded in post_credit_state_ ('decoded' on success, 'failed'
            // after a malformed drain).
            reaping_credit = 7
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

            case connection_state::reaping_credit:
                return reap_credit();

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
            // The page is read in place from the rx ring and the credit is
            // returned only after its payload has been copied out.
            unsigned char const* const page = mailboxes_.peek_page(src_pe);
            if (page == nullptr)
            {
                return false;
            }

            detail::message_header header;
            std::memcpy(&header, page, sizeof(header));

            // The page is consumed on every path below (credit returned to
            // the sender), so a failure must not lead to 'retry this page' —
            // that would read the next page and silently lose the transfer.
            // Handle it as a terminal/loud failure instead.
            std::size_t chunk_size = 0;
            if (!detail::validate_message_page(page, mailboxes_.mtu(),
                    chunk_size, mailboxes_.checksum_enabled()))
            {
                mailboxes_.release_page(src_pe);
                return handle_malformed_header(header);
            }

            // A connection always starts at a message boundary; a first chunk
            // with an index > 0 means the stream lost earlier pages
            // (unrecoverable corruption).  Fail loudly rather than collecting
            // a partial message.
            if (header.chunk_index != 0)
            {
                mailboxes_.release_page(src_pe);
                return handle_malformed_header(header);
            }

            expected_message_id_ = header.message_id;
            buffer_.num_chunks_ = std::make_pair(0u, 0u);

            expected_chunks_ = header.num_chunks;
            received_chunks_ = 0;
            total_size_ = header.size;
            stage_ = stream_stage::counts;
            stage_got_ = 0;

            std::size_t const header_size = detail::header_size;

            if (!consume(page + header_size, chunk_size))
            {
                mailboxes_.release_page(src_pe);
                return handle_malformed_header(header);
            }

            mailboxes_.release_page(src_pe);
            received_chunks_++;

            if (received_chunks_ == expected_chunks_)
            {
                return finish_message();
            }

            state_ = connection_state::collecting;
            return false;
        }

        bool decode_or_collect() noexcept
        {
            return collect_chunks();
        }

        bool collect_chunks() noexcept
        {
            std::size_t const src_pe = static_cast<std::size_t>(src_);

            unsigned char const* const page = mailboxes_.peek_page(src_pe);
            if (page == nullptr)
            {
                return false;
            }

            detail::message_header header;
            std::memcpy(&header, page, sizeof(header));

            std::size_t chunk_size = 0;
            if (!detail::validate_message_page(page, mailboxes_.mtu(),
                    chunk_size, mailboxes_.checksum_enabled()))
            {
                mailboxes_.release_page(src_pe);
                return handle_malformed_header(header);
            }

            // Sequence cross-checks: every continuation chunk must belong to
            // the same message and arrive strictly in order.  Any mismatch
            // means the stream lost or reordered pages — fail loudly.
            if (header.message_id != expected_message_id_ ||
                header.num_chunks != expected_chunks_ ||
                header.chunk_index != received_chunks_)
            {
                mailboxes_.release_page(src_pe);
                return handle_malformed_header(header);
            }

            std::size_t const header_size = detail::header_size;

            if (!consume(page + header_size, chunk_size))
            {
                mailboxes_.release_page(src_pe);
                return handle_malformed_header(header);
            }
            mailboxes_.release_page(src_pe);
            received_chunks_++;

            if (received_chunks_ == expected_chunks_)
            {
                return finish_message();
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

            // The failed page was consumed (credit returned) before
            // validation; wait for that credit-return region to land before
            // reporting the terminal failure (see reap_credit()).
            post_credit_state_ = connection_state::failed;
            state_ = connection_state::reaping_credit;
            return false;
        }

        // Skip (consume + drop) the pages of the discarded message so the
        // stream realigns.  Non-blocking: if the sender has not published the
        // next page yet, re-queue and continue later.  The final drained page
        // returns its credit via an NBI region, so the connection then waits
        // in 'reaping_credit' before finishing.
        bool drain_malformed() noexcept
        {
            std::size_t const src_pe = static_cast<std::size_t>(src_);

            // Scratch page for skipping a discarded message; allocated only
            // on this error path (pages are normally read in place).
            if (recv_buf_.empty())
            {
                recv_buf_.resize(mailboxes_.mtu());
            }

            while (to_drain_ > 0)
            {
                if (!mailboxes_.try_receive_(
                        src_pe, recv_buf_.data(), mailboxes_.mtu()))
                {
                    return false;
                }
                --to_drain_;
            }

            post_credit_state_ = connection_state::failed;
            state_ = connection_state::reaping_credit;
            return false;
        }

        // Wait for the credit-return region of the connection's last consumed
        // page to complete before finishing.  mailbox_array::credit_ready()
        // polls only and consumes the event when done; while it is still in
        // flight the connection re-queues and the source stays reserved, so
        // no other connection can touch this source's event slot.  On
        // completion the connection moves to the terminal state recorded in
        // post_credit_state_.  Mirrors sender_connection::poll_send()'s final
        // send_ready() reap, closing the send-side/credit-side loop so that a
        // drained receiver queue really means no access region is left
        // outstanding: do_stop() derives quiescence from those queues, and
        // finishing early would let the transport tear down while a credit
        // put was still on the wire.
        bool reap_credit() noexcept
        {
            if (!mailboxes_.credit_ready(static_cast<std::size_t>(src_)))
            {
                return false;    // final credit-return region in flight
            }
            state_ = post_credit_state_;
            return true;
        }

        // ---- message stream sink ------------------------------------------
        // The message is the byte stream [counts][transmission chunks]
        // [data_][zero-copy chunk 0]...  Every page payload is pushed through
        // consume(), which routes the bytes to their final destination
        // (descriptor, the parcel data buffer, or a zero-copy chunk buffer).
        enum class stream_stage : std::uint8_t
            counts,
            tchunks,
            data,
            zchunk,
            done
        };

        // Returns false on an inconsistent descriptor.
        bool consume(unsigned char const* p, std::size_t n) noexcept
        {
            while (n > 0)
            {
                switch (stage_)
                {
                case stream_stage::counts:
                {
                    std::size_t const take =
                        (std::min)(sizeof(counts_) - stage_got_, n);
                    std::memcpy(
                        reinterpret_cast<unsigned char*>(counts_) + stage_got_,
                        p, take);
                    stage_got_ += take;
                    p += take;
                    n -= take;
                    if (stage_got_ < sizeof(counts_))
                        break;
                    stage_got_ = 0;
                    std::size_t const nt = counts_[2];
                    if (counts_[0] > nt)
                        return false;
                    if (nt > detail::max_message_chunks)
                        return false;
                    buffer_.transmission_chunks_.assign(
                        nt, buffer_type::transmission_chunk_type{});
                    stage_ = nt == 0 ? begin_data() : stream_stage::tchunks;
                    if (stage_ == stream_stage::done)
                        return false;
                    break;
                }
                case stream_stage::tchunks:
                {
                    std::size_t const total = buffer_.transmission_chunks_.size() *
                        sizeof(buffer_type::transmission_chunk_type);
                    std::size_t const take = (std::min)(total - stage_got_, n);
                    std::memcpy(reinterpret_cast<unsigned char*>(
                                    buffer_.transmission_chunks_.data()) +
                            stage_got_,
                        p, take);
                    stage_got_ += take;
                    p += take;
                    n -= take;
                    if (stage_got_ < total)
                        break;
                    stage_got_ = 0;
                    stage_ = begin_data();
                    if (stage_ == stream_stage::done)
                        return false;
                    break;
                }
                case stream_stage::data:
                {
                    std::size_t const take =
                        (std::min)(buffer_.data_.size() - stage_got_, n);
                    std::memcpy(buffer_.data_.data() + stage_got_, p, take);
                    stage_got_ += take;
                    p += take;
                    n -= take;
                    if (stage_got_ < buffer_.data_.size())
                        break;
                    stage_got_ = 0;
                    if (!data_complete())
                        return false;
                    break;
                }
                case stream_stage::zchunk:
                {
                    auto& c = buffer_.chunks_[zc_idx_];
                    std::size_t const total = c.size();
                    std::size_t const take = (std::min)(total - stage_got_, n);
                    std::memcpy(static_cast<unsigned char*>(c.data()) +
                            stage_got_,
                        p, take);
                    stage_got_ += take;
                    p += take;
                    n -= take;
                    if (stage_got_ < total)
                        break;
                    stage_got_ = 0;
                    if (!next_zero_copy_chunk(zc_idx_ + 1))
                        return false;
                    break;
                }
                case stream_stage::done:
                    return false;    // trailing bytes
                }
            }
            return true;
        }

        // Called when the descriptor is complete: derive the data_ size from
        // the stream length and set up the data buffer.
        stream_stage begin_data() noexcept
        {
            std::size_t const nzc = counts_[0];
            std::uint64_t zsum = 0;
            for (std::size_t i = 0; i < nzc; ++i)
                zsum += buffer_.transmission_chunks_[i].second;
            std::uint64_t const desc = sizeof(counts_) +
                buffer_.transmission_chunks_.size() *
                    sizeof(buffer_type::transmission_chunk_type);
            if (desc + zsum > total_size_)
                return stream_stage::done;    // inconsistent
            std::size_t const dsize = total_size_ - desc - zsum;
            buffer_.size_ = dsize;
            buffer_.data_size_ = dsize;
            buffer_.data_.resize(dsize);
            buffer_.num_chunks_ = std::make_pair(counts_[0], counts_[1]);
            if (dsize == 0)
            {
                // (decode requires a non-empty data buffer, so this is
                // malformed for HPX; treat it as such)
                return stream_stage::done;
            }

            return stream_stage::data;
        }


        // data_ is complete.  With zero-copy chunks: allocate their buffers
        // (directly in the deserialized parcels if the zero-copy receive
        // optimization is on) and continue with the chunk bytes.
        bool data_complete() noexcept
        {
            std::size_t const nzc = counts_[0];
            if (nzc == 0)
            {
                stage_ = stream_stage::done;
                return true;
            }

            if (pp_->allow_zero_copy_receive_optimizations())
            {
                buffer_.chunks_.resize(nzc);
                for (std::size_t i = 0; i != nzc; ++i)
                {
                    buffer_.chunks_[i] = serialization::create_pointer_chunk(
                        nullptr, static_cast<std::size_t>(
                                     buffer_.transmission_chunks_[i].second));
                }
                parcels_ = hpx::parcelset::decode_parcels_zero_copy(
                    *pp_, buffer_, static_cast<std::size_t>(-1));
                // buffer_.chunks_ now holds entries for all chunks, including
                // the non-zero-copy (index) ones
                optimized_ = true;
            }
            else
            {
                buffer_.chunks_.resize(nzc);
                chunk_buffers_.resize(nzc);
                for (std::size_t i = 0; i != nzc; ++i)
                {
                    std::size_t const sz = static_cast<std::size_t>(
                        buffer_.transmission_chunks_[i].second);
                    chunk_buffers_[i].reset(new char[sz == 0 ? 1 : sz]);
                    buffer_.chunks_[i] = serialization::create_pointer_chunk(
                        chunk_buffers_[i].get(), sz);
                }
            }
            zc_seen_ = 0;
            return next_zero_copy_chunk(0);
        }

        // Advance to the next zero-copy chunk at or after buffer_.chunks_[from]
        // (index-type chunks carry no bytes in the stream).
        bool next_zero_copy_chunk(std::size_t from) noexcept
        {
            while (from < buffer_.chunks_.size() &&
                buffer_.chunks_[from].type_ ==
                    serialization::chunk_type::chunk_type_index)
            {
                ++from;
            }
            if (from >= buffer_.chunks_.size())
            {
                stage_ = stream_stage::done;
                return true;
            }
            // chunks whose declared size is 0 carry no bytes
            zc_idx_ = from;
            if (buffer_.chunks_[from].size() == 0)
                return next_zero_copy_chunk(from + 1);
            ++zc_seen_;
            stage_ = stream_stage::zchunk;
            return true;
        }

        // All pages consumed: hand the message to HPX.
        bool finish_message() noexcept
        {
            if (stage_ != stream_stage::done)
            {
                // truncated message: report loudly, keep the stream aligned
                std::fprintf(stderr,
                    "gasnet: PE %zu: message from src %d ended early\n",
                    mailboxes_.my_pe(), src_);
                post_credit_state_ = connection_state::failed;
                state_ = connection_state::reaping_credit;
                return false;
            }

            try {
            if (optimized_)
            {
                hpx::parcelset::handle_received_parcels(
                    HPX_MOVE(parcels_), static_cast<std::size_t>(-1));
                buffer_ = buffer_type{};
            }
            else
            {
                std::vector<parcel> parcels = hpx::parcelset::decode_parcels(
                    *pp_, HPX_MOVE(buffer_), static_cast<std::size_t>(-1));
                hpx::parcelset::handle_received_parcels(
                    HPX_MOVE(parcels), static_cast<std::size_t>(-1));
                chunk_buffers_.clear();
            }
            } catch (std::exception const& e) {
                std::fprintf(stderr, "gasnet: PE %zu: decode failed: %s (size=%zu nzc=%u nnzc=%u tch=%zu data=%zu)\n",
                    mailboxes_.my_pe(), e.what(), total_size_, counts_[0], counts_[1],
                    buffer_.transmission_chunks_.size(), buffer_.data_.size());
                std::abort();
            }

            // The final page's credit-return region is still in flight; wait
            // for it before releasing the source reservation (see
            // reap_credit()), so a drained receiver queue implies no access
            // region is left outstanding when do_stop() shuts the transport
            // down.
            post_credit_state_ = connection_state::decoded;
            state_ = connection_state::reaping_credit;
            return false;
        }

        connection_state state_;
        int src_;
        mailbox_array& mailboxes_;
        Parcelport* pp_;

        buffer_type buffer_;
        std::uint32_t counts_[3] = {0, 0, 0};
        stream_stage stage_ = stream_stage::counts;
        std::size_t stage_got_ = 0;
        std::size_t total_size_ = 0;
        std::size_t zc_idx_ = 0;
        std::size_t zc_seen_ = 0;
        bool optimized_ = false;
        std::vector<parcelset::parcel> parcels_;
        std::vector<std::unique_ptr<char[]>> chunk_buffers_;
        std::uint32_t expected_chunks_;
        std::uint32_t received_chunks_;

        // message_id of the message currently being collected (validated
        // against every continuation chunk).
        std::uint64_t expected_message_id_ = 0;

        // Terminal state reached after the 'reaping_credit' wait completes:
        // 'decoded' for a successful transfer, 'failed' after a malformed
        // drain.  Set by the transition that enters 'reaping_credit'.
        connection_state post_credit_state_ = connection_state::decoded;

        // Pages left to skip while draining a discarded message.
        std::size_t to_drain_ = 0;

        std::vector<unsigned char> recv_buf_;
    };
}    // namespace hpx::parcelset::policies::gasnet

#endif
