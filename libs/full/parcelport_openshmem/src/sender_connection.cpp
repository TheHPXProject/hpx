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

        constexpr std::size_t header_size = sizeof(message_header);
    }    // namespace detail

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
            (total_data_size_ + available_payload_ - 1) /
            available_payload_);
        if (num_chunks_ == 0)
        {
            num_chunks_ = 1;
        }

        chunk_idx_ = 0;
    }

    // Stage one chunk: copy header + payload into the local buffer slot.
    // The actual shmem transfer is done by the caller via send().
    void sender_connection::stage_chunk() noexcept
    {
        auto& mailboxes = *mailboxes_;

        std::size_t const offset = chunk_idx_ * available_payload_;
        std::size_t const chunk_size =
            (offset + available_payload_ > total_data_size_) ?
            (total_data_size_ - offset) :
            available_payload_;

        unsigned char* buffer = mailboxes.get_buffer(
            static_cast<std::size_t>(dst_));

        detail::message_header header{buffer_.size_, buffer_.data_size_,
            num_chunks_, chunk_idx_,
            static_cast<std::uint32_t>(total_data_size_ & 0xFFFFFFFF),
            static_cast<std::uint32_t>(total_data_size_ >> 32)};

        std::memcpy(buffer, &header, sizeof(header));
        if (chunk_size > 0)
        {
            std::memcpy(buffer + sizeof(header), buffer_.data_.data() + offset,
                chunk_size);
        }
    }

    // Blocking send: stage each chunk and call mailboxes_.send() which
    // transfers the data and waits for the receiver's ack before returning.
    bool sender_connection::poll_send() noexcept
    {
        if (dst_ == static_cast<int>(mailboxes_->my_pe()))
        {
            handle_local_send();
            return true;
        }

        for (std::uint32_t i = 0; i < num_chunks_; ++i)
        {
            chunk_idx_ = i;
            stage_chunk();

            std::size_t const offset = i * available_payload_;
            std::size_t const chunk_size =
                (offset + available_payload_ > total_data_size_) ?
                (total_data_size_ - offset) :
                available_payload_;

            mailboxes_->send(
                static_cast<std::size_t>(dst_),
                detail::header_size + chunk_size);
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

    void sender_connection::handle_local_send() noexcept
    {
        auto& mailboxes = *mailboxes_;

        std::size_t const mtu = mailboxes.mtu();
        std::size_t const available_payload = mtu - detail::header_size;

        std::size_t const total_data_size = buffer_.size_;
        std::uint32_t const num_chunks = static_cast<std::uint32_t>(
            (total_data_size + available_payload - 1) / available_payload);

        std::size_t offset = 0;

        for (std::uint32_t chunk_idx = 0; chunk_idx < num_chunks; ++chunk_idx)
        {
            std::size_t const chunk_size =
                (offset + available_payload > total_data_size) ?
                (total_data_size - offset) :
                available_payload;

            unsigned char* buffer = mailboxes.get_buffer(
                static_cast<std::size_t>(dst_));

            detail::message_header header{buffer_.size_, buffer_.data_size_,
                num_chunks, chunk_idx,
                static_cast<std::uint32_t>(total_data_size & 0xFFFFFFFF),
                static_cast<std::uint32_t>(total_data_size >> 32)};

            std::memcpy(buffer, &header, sizeof(header));
            if (chunk_size > 0)
            {
                std::memcpy(buffer + sizeof(header),
                    buffer_.data_.data() + offset, chunk_size);
            }

            // publish this chunk into our own rx pool via the credit protocol
            mailboxes.send(static_cast<std::size_t>(dst_),
                detail::header_size + chunk_size);

            offset += chunk_size;
        }

        finish();
    }

}    // namespace hpx::parcelset::policies::openshmem

#endif
