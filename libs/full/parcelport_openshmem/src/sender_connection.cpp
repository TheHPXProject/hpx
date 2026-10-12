//  Copyright (c) 2026 Christopher Taylor
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_OPENSHMEM)
#include <hpx/parcelport_openshmem/sender_connection.hpp>

#include <cstring>

#include <shmem.h>

namespace hpx::parcelset::policies::openshmem {

    void sender_connection::prepare() noexcept
    {
        auto& mailboxes = *mailboxes_;

        buffer_.size_ = buffer_.data_.size();
        buffer_.data_size_ = buffer_.data_.size();
        buffer_.num_chunks_ =
            std::make_pair(static_cast<std::uint32_t>(buffer_.chunks_.size()),
                static_cast<std::uint32_t>(buffer_.chunks_.size()));

        buffer_.transmission_chunks_.clear();

        total_data_size_ = buffer_.size_;
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

    // Stage one chunk: copy header + payload into the local buffer slot.
    // The actual shmem transfer is done by the caller via try_send().
    // Returns the number of payload bytes staged (0 for an empty chunk).
    std::size_t sender_connection::stage_chunk() noexcept
    {
        auto& mailboxes = *mailboxes_;

        std::size_t const offset = chunk_idx_ * available_payload_;
        std::size_t const chunk_size = detail::chunk_size_for(
            chunk_idx_, num_chunks_, total_data_size_, available_payload_);

        unsigned char* buffer =
            mailboxes.get_buffer(static_cast<std::size_t>(dst_));

        // Protect the payload when checksumming is enabled; the receiver
        // recomputes crc32 over the exact same transferred region.  Header
        // integrity is guaranteed by the receiver's structural/geometry
        // validation instead.  Both sides must agree on the setting (it must
        // be identical on every PE via hpx.parcel.openshmem.checksum).
        detail::message_header header{buffer_.size_, buffer_.data_size_,
            num_chunks_, chunk_idx_,
            static_cast<std::uint32_t>(total_data_size_ & 0xFFFFFFFF),
            static_cast<std::uint32_t>(total_data_size_ >> 32), message_id_,
            mailboxes.checksum_enabled() ?
                detail::crc32(reinterpret_cast<unsigned char const*>(
                                  buffer_.data_.data() + offset),
                    chunk_size) :
                0};

        std::memcpy(buffer, &header, sizeof(header));
        if (chunk_size > 0)
        {
            std::memcpy(buffer + sizeof(header), buffer_.data_.data() + offset,
                chunk_size);
        }

        return chunk_size;
    }

    // Non-blocking send driver: stage and transfer as many chunks as the
    // credit word allows in this call via mailbox_array::try_send()
    // (which polls credit once).  Returns true when the entire
    // multi-chunk transfer is complete, false while chunks remain but no
    // credit is left, in which case the caller must re-queue the
    // connection.  Never blocks.  The sender holds this connection's
    // destination reservation for the whole transfer, so this never
    // interleaves with another concurrent connection to the same
    // destination.
    bool sender_connection::poll_send() noexcept
    {
        while (chunk_idx_ < num_chunks_)
        {
            std::size_t const chunk_size = stage_chunk();
            std::size_t const transfer_size =
                detail::header_size + chunk_size;

            if (!mailboxes_->try_send(static_cast<std::size_t>(dst_),
                    transfer_size))
            {
                return false;    // no credit: requeue and retry later
            }

            ++chunk_idx_;
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
