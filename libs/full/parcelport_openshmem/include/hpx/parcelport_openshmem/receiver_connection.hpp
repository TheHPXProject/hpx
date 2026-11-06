//  Copyright (c) 2026 Christopher Taylor
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_OPENSHMEM)
#include <hpx/assert.hpp>
#include <hpx/parcelport_openshmem/mailbox_array.hpp>
#include <hpx/parcelset/decode_parcels.hpp>
#include <hpx/parcelset/parcel_buffer.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <cstring>
#include <utility>
#include <vector>

namespace hpx::parcelset::policies::openshmem {

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
            // Credits for pages consumed in this call are published lazily
            // (see mailbox_array::release_page()); make sure none is left
            // unpublished when we hand the stream back, or the sender could
            // wait on a credit nobody will send.
            struct credit_flush
            {
                mailbox_array& m;
                std::size_t src;
                ~credit_flush()
                {
                    m.flush_credits(src);
                }
            } flush{mailboxes_, static_cast<std::size_t>(src_)};

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
            // The page is read in place from the rx ring and the credit is
            // returned only after its payload has been copied out, so there
            // is no intermediate copy of the whole page.
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
            std::uint64_t const vt0 = stats_now();
            bool const page_ok = detail::validate_message_page(page,
                mailboxes_.mtu(), chunk_size, mailboxes_.checksum_enabled());
            stats_add(detail::g_io_stats.crc_ns, vt0);
            if (!page_ok)
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

            std::uint64_t const ct0 = stats_now();
            bool const ok = consume(page + header_size, chunk_size);
            stats_add(detail::g_io_stats.copy_ns, ct0);
            if (!ok)
            {
                mailboxes_.release_page(src_pe);
                return handle_malformed_header(header);
            }

            mailboxes_.release_page(src_pe);
            received_chunks_++;

            if (received_chunks_ == expected_chunks_)
            {
                // return the credits before dispatching: decoding and
                // running the handlers can take a while
                mailboxes_.flush_credits(static_cast<std::size_t>(src_));
                return finish_message();
            }

            state_ = connection_state::collecting;
            return collect_chunks();
        }

        static std::uint64_t stats_now() noexcept
        {
            return detail::g_io_stats_on.load(std::memory_order_relaxed) ?
                detail::ns_now() : 0;
        }

        static void stats_add(
            std::atomic<std::uint64_t>& counter, std::uint64_t t0) noexcept
        {
            if (t0 != 0)
                counter.fetch_add(
                    detail::ns_now() - t0, std::memory_order_relaxed);
        }

        bool decode_or_collect() noexcept
        {
            return collect_chunks();
        }

        bool collect_chunks() noexcept
        {
            std::size_t const src_pe = static_cast<std::size_t>(src_);

            // Drain every page that is already published instead of going
            // back through the receiver's connection queue after each one.
            for (;;)
            {
                unsigned char const* const page = mailboxes_.peek_page(src_pe);
                if (page == nullptr)
                {
                    return false;
                }

                detail::message_header header;
                std::memcpy(&header, page, sizeof(header));

                std::size_t chunk_size = 0;
                std::uint64_t const vt0 = stats_now();
                bool const page_ok = detail::validate_message_page(page,
                    mailboxes_.mtu(), chunk_size, mailboxes_.checksum_enabled());
                stats_add(detail::g_io_stats.crc_ns, vt0);
                if (!page_ok)
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

                std::uint64_t const ct1 = stats_now();
                bool const ok = consume(page + header_size, chunk_size);
                stats_add(detail::g_io_stats.copy_ns, ct1);
                if (!ok)
                {
                    mailboxes_.release_page(src_pe);
                    return handle_malformed_header(header);
                }
                mailboxes_.release_page(src_pe);
                received_chunks_++;

                if (received_chunks_ == expected_chunks_)
                {
                    // return the credits before dispatching: decoding and
                    // running the handlers can take a while
                    mailboxes_.flush_credits(static_cast<std::size_t>(src_));
                    return finish_message();
                }
            }
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
                "openshmem: PE %zu: dropping malformed message from src %d "
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

            state_ = connection_state::failed;
            return true;
        }

        // ---- message stream sink ------------------------------------------
        // The message is the byte stream [counts][transmission chunks]
        // [data_][zero-copy chunk 0]...  Every page payload is pushed through
        // consume(), which routes the bytes to their final destination
        // (descriptor, the parcel data buffer, or a zero-copy chunk buffer).
        enum class stream_stage : std::uint8_t
        {
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

            bool optimize = pp_->allow_zero_copy_receive_optimizations() &&
                !detail::g_discard_received.load(std::memory_order_acquire);
            if (optimize)
            {
                buffer_.chunks_.resize(nzc);
                for (std::size_t i = 0; i != nzc; ++i)
                {
                    buffer_.chunks_[i] = serialization::create_pointer_chunk(
                        nullptr, static_cast<std::size_t>(
                                     buffer_.transmission_chunks_[i].second));
                }
                try
                {
                    parcels_ = hpx::parcelset::decode_parcels_zero_copy(
                        *pp_, buffer_, static_cast<std::size_t>(-1));
                    // buffer_.chunks_ now holds entries for all chunks,
                    // including the non-zero-copy (index) ones
                    optimized_ = true;
                }
                catch (...)
                {
                    // Decoding schedules the actions, which throws once the
                    // runtime is shutting down.  Still consume the chunk
                    // bytes (into scratch buffers) so the stream stays in
                    // sync, and drop the message in finish_message().
                    parcels_.clear();
                    dropped_ = true;
                    optimize = false;
                }
            }
            if (!optimize)
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

        // All pages consumed: hand the message to HPX.  Runs on a transport
        // io thread, which is not an HPX worker: pass -1 ("unknown thread",
        // as the TCP parcelport does) so received actions are not scheduled
        // with a worker-thread hint they do not own.
        bool finish_message() noexcept
        {
            if (stage_ != stream_stage::done)
            {
                std::fprintf(stderr,
                    "openshmem: PE %zu: message from src %d ended early\n",
                    mailboxes_.my_pe(), src_);
                state_ = connection_state::failed;
                return true;
            }

            if (dropped_ ||
                detail::g_discard_received.load(std::memory_order_acquire))
            {
                parcels_.clear();
                buffer_ = buffer_type{};
                chunk_buffers_.clear();
                state_ = connection_state::decoded;
                return true;
            }

            std::uint64_t const dt0 =
                detail::g_io_stats_on.load(std::memory_order_relaxed) ?
                detail::ns_now() : 0;
            // A parcel can complete while the runtime is already shutting
            // down (e.g. a late AGAS credit decrement): scheduling its action
            // then throws "runtime is being shut down".  Letting that escape
            // this noexcept function would terminate the process, so drop the
            // message instead; during shutdown nobody can use it anyway.
            try
            {
                if (optimized_)
                {
                    hpx::parcelset::handle_received_parcels(
                        HPX_MOVE(parcels_), static_cast<std::size_t>(-1));
                    buffer_ = buffer_type{};
                }
                else
                {
                    std::vector<parcel> parcels =
                        hpx::parcelset::decode_parcels(*pp_, HPX_MOVE(buffer_),
                            static_cast<std::size_t>(-1));
                    hpx::parcelset::handle_received_parcels(
                        HPX_MOVE(parcels), static_cast<std::size_t>(-1));
                    chunk_buffers_.clear();
                }
            }
            catch (...)
            {
                parcels_.clear();
                chunk_buffers_.clear();
                if (std::getenv("OPENSHMEM_PP_VERBOSE") != nullptr)
                {
                    std::fprintf(stderr,
                        "openshmem: PE %zu: dropped a message from src %d "
                        "(exception while dispatching, runtime shutting "
                        "down?)\n",
                        mailboxes_.my_pe(), src_);
                }
            }
            if (dt0 != 0)
                detail::g_io_stats.decode_ns.fetch_add(
                    detail::ns_now() - dt0, std::memory_order_relaxed);

            state_ = connection_state::decoded;
            return true;
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
        bool dropped_ = false;
        std::vector<parcelset::parcel> parcels_;
        std::vector<std::unique_ptr<char[]>> chunk_buffers_;
        std::uint32_t expected_chunks_;
        std::uint32_t received_chunks_;

        // message_id of the message currently being collected (validated
        // against every continuation chunk).
        std::uint64_t expected_message_id_ = 0;

        // Pages left to skip while draining a discarded message.
        std::size_t to_drain_ = 0;

        std::vector<unsigned char> recv_buf_;
    };
}    // namespace hpx::parcelset::policies::openshmem

#endif
