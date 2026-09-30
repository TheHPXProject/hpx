//  Copyright (c) 2026 Christopher Taylor
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_GASNET)
#include <hpx/assert.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/parcelport_gasnet/locality.hpp>
#include <hpx/parcelset/parcelport_connection.hpp>
#include <hpx/parcelset/parcelset_fwd.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <system_error>
#include <utility>
#include <vector>

#include <hpx/parcelport_gasnet/mailbox_array.hpp>

namespace hpx::parcelset::policies::gasnet {

    struct sender;

    struct sender_connection
      : parcelset::parcelport_connection<sender_connection>
    {
    private:
        using sender_type = sender;

        using base_type = parcelset::parcelport_connection<sender_connection>;

    public:
        sender_connection(sender_type* s, int dst, void* mailboxes)
          : sender_(s)
          , dst_(dst)
          , mailboxes_(static_cast<mailbox_array*>(mailboxes))
          , there_(parcelset::locality(locality(dst_)))
        {
        }

        constexpr parcelset::locality const& destination() const noexcept
        {
            return there_;
        }

        constexpr int dst() const noexcept
        {
            return dst_;
        }

        static constexpr void verify_(
            parcelset::locality const& /* parcel_locality_id */) noexcept
        {
        }

        using handler_type = hpx::move_only_function<void(error_code const&)>;
        using post_handler_type = hpx::move_only_function<void(
            error_code const&, parcelset::locality const&,
            std::shared_ptr<sender_connection>)>;

        // Store the completion handlers and prepare the parcel for sending.
        // The actual sending is driven by poll_send() from one progress
        // thread at a time under the sender's per-destination reservation
        // (each destination is single-flight).
        void async_write(handler_type&& handler,
            post_handler_type&& parcel_postprocess) noexcept
        {
            HPX_ASSERT(!handler_);
            HPX_ASSERT(!buffer_.data_.empty());

            handler_ = HPX_MOVE(handler);
            postprocess_handler_ = HPX_MOVE(parcel_postprocess);

            prepare();
        }

        // Non-blocking send driver.  Sends as many chunks as the transport
        // admits in this call (each via the non-blocking
        // mailbox_array::try_send(), gated by send_ready() so the single
        // outstanding access region to the destination serializes chunk
        // ordering) and returns true when the connection is complete, false
        // when one or more chunks remain but either no credit is left or the
        // previous region is still in flight, in which case the connection
        // must be re-queued.  The sender holds this connection's destination
        // reservation for the whole transfer, so poll_send() never
        // interleaves with another concurrent connection to the same
        // destination.
        bool poll_send() noexcept;

    private:
        void prepare() noexcept;
        std::size_t stage_chunk() noexcept;
        void finish() noexcept;

        friend struct sender;

        sender_type* sender_;
        int dst_;
        mailbox_array* mailboxes_;

        parcelset::locality there_;

        handler_type handler_;
        post_handler_type postprocess_handler_;

        std::uint32_t num_chunks_ = 0;
        std::uint32_t chunk_idx_ = 0;
        std::size_t total_data_size_ = 0;
        std::size_t available_payload_ = 0;
        std::uint64_t message_id_ = 0;

        // True while this connection holds the single-flight reservation on
        // its destination (see sender::busy_dsts_).  Only manipulated by the
        // sender under connections_mtx_.
        bool reserved_dst_ = false;
    };
}    // namespace hpx::parcelset::policies::gasnet

#endif
