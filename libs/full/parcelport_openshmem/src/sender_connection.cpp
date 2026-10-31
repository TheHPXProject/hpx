//  Copyright (c) 2026 Christopher Taylor
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_OPENSHMEM)
#include <hpx/parcelport_openshmem/sender_connection.hpp>
#include <hpx/parcelset/parcel_buffer.hpp>
#include <hpx/serialization/serialization_chunk.hpp>

#include <algorithm>

#include <cstring>

#include <shmem.h>

namespace hpx::parcelset::policies::openshmem {

    void sender_connection::prepare() noexcept
    {
        auto& mailboxes = *mailboxes_;

        // Descriptor: chunk counts + transmission chunk table (the same
        // information the MPI parcelport sends in its header).
        using tchunk_type = parcel_buffer<>::transmission_chunk_type;
        auto const& tch = buffer_.transmission_chunks_;
        std::uint32_t const counts[3] = {buffer_.num_chunks_.first,
            buffer_.num_chunks_.second,
            static_cast<std::uint32_t>(tch.size())};
        desc_.resize(sizeof(counts) + tch.size() * sizeof(tchunk_type));
        std::memcpy(desc_.data(), counts, sizeof(counts));
        if (!tch.empty())
        {
            std::memcpy(desc_.data() + sizeof(counts), tch.data(),
                tch.size() * sizeof(tchunk_type));
        }

        segs_.clear();
        std::size_t pos = 0;
        auto add = [&](void const* p, std::size_t n) {
            if (n == 0)
                return;
            segs_.push_back({static_cast<unsigned char const*>(p), n, pos});
            pos += n;
        };
        add(desc_.data(), desc_.size());
        add(buffer_.data_.data(), buffer_.data_.size());
        for (auto const& c : buffer_.chunks_)
        {
            if (c.type_ == serialization::chunk_type::chunk_type_pointer ||
                c.type_ == serialization::chunk_type::chunk_type_const_pointer)
            {
                add(c.data(), c.size());
            }
        }
        seg_hint_ = 0;

        buffer_.size_ = buffer_.data_.size();
        buffer_.data_size_ = buffer_.data_.size();
        total_data_size_ = pos;
        available_payload_ = mailboxes.mtu() - detail::header_size;

        num_chunks_ = static_cast<std::uint32_t>(
            (total_data_size_ + available_payload_ - 1) / available_payload_);
        if (num_chunks_ == 0)
        {
            num_chunks_ = 1;
        }

        message_id_ = mailboxes.next_message_id();

        chunk_idx_ = 0;
    }

    // Stage one page: gather the stream bytes [offset, offset+chunk_size)
    // from the segments, then header + payload CRC.  The actual shmem
    // transfer is done by the caller.  Returns the number of payload bytes
    // staged (0 for an empty chunk).
    std::size_t sender_connection::stage_chunk(
        std::uint32_t chunk_idx, unsigned char* buffer) noexcept
    {
        auto& mailboxes = *mailboxes_;

        std::size_t const offset =
            static_cast<std::size_t>(chunk_idx) * available_payload_;
        std::size_t const chunk_size = detail::chunk_size_for(
            chunk_idx_, num_chunks_, total_data_size_, available_payload_);

        unsigned char* out = buffer + sizeof(detail::message_header);
        std::size_t done = 0;
        if (chunk_size > 0)
        {
            std::size_t si = seg_hint_;
            while (si > 0 && segs_[si].start > offset)
                --si;
            while (si + 1 < segs_.size() &&
                segs_[si].start + segs_[si].len <= offset)
                ++si;
            seg_hint_ = si;
            while (done < chunk_size)
            {
                auto const& sg = segs_[si];
                std::size_t const so = offset + done - sg.start;
                std::size_t const n = (std::min)(sg.len - so, chunk_size - done);
                std::memcpy(out + done, sg.ptr + so, n);
                done += n;
                if (done < chunk_size)
                    ++si;
            }
        }

        detail::message_header header{total_data_size_, total_data_size_,
            num_chunks_, chunk_idx,
            static_cast<std::uint32_t>(total_data_size_ & 0xFFFFFFFF),
            static_cast<std::uint32_t>(total_data_size_ >> 32), message_id_,
            mailboxes.checksum_enabled() ? detail::crc32(out, chunk_size) : 0};
        std::memcpy(buffer, &header, sizeof(header));

        return chunk_size;
    }

    bool sender_connection::poll_send() noexcept
    {
        auto& mailboxes = *mailboxes_;
        std::size_t const dst = static_cast<std::size_t>(dst_);

        while (chunk_idx_ < num_chunks_)
        {
            // Send as many pages as there is credit for in one batch: stage
            // them into consecutive tx slots, push them with non-blocking
            // puts and publish the counter once.  Staging (memcpy + checksum)
            // only happens when there is credit, so a poll that finds none
            // costs nothing.
            std::size_t const credits = mailboxes.send_credits(dst);
            if (credits == 0)
            {
                return false;    // no credit: requeue and retry later
            }

            std::size_t const n = (std::min)(
                credits, static_cast<std::size_t>(num_chunks_ - chunk_idx_));
            std::uint64_t const t0 =
                detail::g_io_stats_on.load(std::memory_order_relaxed) ?
                detail::ns_now() : 0;
            std::size_t bytes = 0;
            for (std::size_t k = 0; k < n; ++k)
            {
                std::size_t const chunk_size = stage_chunk(
                    chunk_idx_ + static_cast<std::uint32_t>(k),
                    mailboxes.tx_page_at(dst, k));
                std::size_t const transfer_size =
                    detail::header_size + chunk_size;
                mailboxes.put_page_nbi(dst, k, transfer_size);
                bytes += transfer_size;
            }
            mailboxes.publish_pages(dst, n, bytes);
            if (t0 != 0)
                detail::g_io_stats.put_ns.fetch_add(
                    detail::ns_now() - t0, std::memory_order_relaxed);

            chunk_idx_ += static_cast<std::uint32_t>(n);
        }

        finish();
        return true;
    }

    void sender_connection::finish() noexcept
    {
        error_code ec;
        handler_(ec);

        hpx::move_only_function<void(error_code const&,
            parcelset::locality const&, std::shared_ptr<sender_connection>)>
            postprocess_handler;
        std::swap(postprocess_handler, postprocess_handler_);
        if (postprocess_handler)
        {
            postprocess_handler(ec, there_, shared_from_this());
        }
    }

}    // namespace hpx::parcelset::policies::openshmem

#endif
