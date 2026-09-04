//  Copyright (c) 2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_OPENSHMEM)
#include <hpx/assert.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/parcelport_openshmem/locality.hpp>
#include <hpx/parcelset/parcelport_connection.hpp>
#include <hpx/parcelset/parcelset_fwd.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <system_error>
#include <utility>
#include <vector>

#include <hpx/parcelport_openshmem/mailbox_array.hpp>

namespace hpx::parcelset::policies::openshmem {

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
        // The actual sending is driven by poll_send() from the single
        // progress thread, which keeps all shmem_* calls on that thread
        // (required by SHMEM_THREAD_SERIALIZED).
        void async_write(handler_type&& handler,
            post_handler_type&& parcel_postprocess) noexcept
        {
            HPX_ASSERT(!handler_);
            HPX_ASSERT(!buffer_.data_.empty());

            handler_ = HPX_MOVE(handler);
            postprocess_handler_ = HPX_MOVE(parcel_postprocess);

            prepare();
        }

        // Blocking send driver.  Sends all chunks (each via the blocking
        // mailbox_array::send()) and returns true when the connection is
        // complete.  Must be called from the single progress thread only.
        bool poll_send() noexcept;

    private:
        void prepare() noexcept;
        void stage_chunk() noexcept;
        void finish() noexcept;
        void handle_local_send() noexcept;

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
    };
}    // namespace hpx::parcelset::policies::openshmem

#endif
