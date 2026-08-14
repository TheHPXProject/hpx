//  Copyright (c) 2007-2021 Hartmut Kaiser
//  Copyright (c) 2014-2015 Thomas Heller
//  Copyright (c) 2023 Christopher Taylor
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_GASNET)
#include <hpx/assert.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/gasnet_base.hpp>
#include <hpx/modules/timing.hpp>

#include <hpx/gasnet_base/gasnet_environment.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/parcelport_gasnet/header.hpp>
#include <hpx/parcelport_gasnet/locality.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <system_error>
#include <utility>
#include <vector>

namespace hpx::parcelset::policies::gasnet {

    struct sender;
    struct sender_connection;

    int acquire_tag(sender*) noexcept;
    void add_connection(sender*, std::shared_ptr<sender_connection> const&);

    struct sender_connection
      : parcelset::parcelport_connection<sender_connection>
    {
    private:
        using sender_type = sender;

        using write_handler_type =
            hpx::function<void(std::error_code const&, parcel const&)>;

        enum connection_state
        {
            initialized,
            sent_header,
            sent_transmission_chunks,
            sent_data,
            sent_chunks
        };

        using base_type = parcelset::parcelport_connection<sender_connection>;

    public:
        sender_connection(sender_type* s, int dst, parcelset::parcelport* pp)
          : state_(initialized)
          , sender_(s)
          , tag_(-1)
          , dst_(dst)
          , chunks_idx_(0)
          , ack_(0)
          , pp_(pp)
          , there_(parcelset::locality(locality(dst_)))
        {
        }

        parcelset::locality const& destination() const noexcept
        {
            return there_;
        }

        constexpr void verify_(
            parcelset::locality const& /* parcel_locality_id */) const noexcept
        {
        }

        template <typename Handler, typename ParcelPostprocess>
        void async_write(
            Handler&& handler, ParcelPostprocess&& parcel_postprocess)
        {
            HPX_ASSERT(!handler_);
            HPX_ASSERT(!postprocess_handler_);
            HPX_ASSERT(!buffer_.data_.empty());

#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
            buffer_.data_point_.time_ =
                hpx::chrono::high_resolution_clock::now();
#endif
            chunks_idx_ = 0;
            tag_ = acquire_tag(sender_);
            header_ = header(buffer_, tag_);
            header_.assert_valid();

            state_ = initialized;

            handler_ = HPX_FORWARD(Handler, handler);

            if (!send())
            {
                postprocess_handler_ =
                    HPX_FORWARD(ParcelPostprocess, parcel_postprocess);
                add_connection(sender_, shared_from_this());
            }
            else
            {
                HPX_ASSERT(!handler_);
                error_code ec;
                parcel_postprocess(ec, there_, shared_from_this());
            }
        }

        bool send()
        {
            switch (state_)
            {
            case initialized:
                return send_header();

            case sent_header:
                return send_transmission_chunks();

            case sent_transmission_chunks:
                return send_data();

            case sent_data:
                return send_chunks();

            case sent_chunks:
                return done();

            default:
                HPX_ASSERT(false);
            }
            return false;
        }

        bool send_header()
        {
            {
                hpx::util::gasnet_environment::scoped_lock l;
                HPX_ASSERT(state_ == initialized);

                // compute + send the number of GASNET_PAGEs to send and the
                // remainder number of bytes to a GASNET_PAGE
                //
                std::size_t const chunks[] = {
                    static_cast<size_t>(header_.data_size_ / GASNET_PAGESIZE),
                    static_cast<size_t>(header_.data_size_ % GASNET_PAGESIZE)};
                std::size_t const sizeof_chunks = sizeof(chunks);
                // clang-format off
                std::memcpy(hpx::util::gasnet_environment::segments
                    [hpx::util::gasnet_environment::rank()].addr,
                    chunks, sizeof_chunks);
                // clang-format on

                // put from this localities gasnet shared memory segment
                // into the remote locality (dst_)'s shared memory segment
                //
                // clang-format off
                hpx::util::gasnet_environment::put(
                    static_cast<std::uint8_t*>(hpx::util::gasnet_environment::
                        segments[hpx::util::gasnet_environment::rank()].addr),
                    dst_,
                    static_cast<std::uint8_t*>(
                        hpx::util::gasnet_environment::segments[dst_].addr),
                    sizeof_chunks);
                // clang-format on
            }

            state_ = sent_header;
            return send_transmission_chunks();
        }

        bool send_transmission_chunks()
        {
            HPX_ASSERT(state_ == sent_header);
            if (!request_done())
            {
                return false;
            }

            std::vector<typename parcel_buffer_type::transmission_chunk_type>&
                chunks = buffer_.transmission_chunks_;
            if (!chunks.empty())
            {
                hpx::util::gasnet_environment::scoped_lock l;
                // clang-format off
                std::memcpy(hpx::util::gasnet_environment::segments
                    [hpx::util::gasnet_environment::rank()].addr,
                    chunks.data(),
                    static_cast<int>(chunks.size() *
                        sizeof(parcel_buffer_type::transmission_chunk_type)));

                gasnet_put_bulk(dst_,
                    static_cast<std::uint8_t*>(
                        hpx::util::gasnet_environment::segments[dst_].addr),
                    static_cast<std::uint8_t*>(hpx::util::gasnet_environment::
                        segments[hpx::util::gasnet_environment::rank()].addr),
                    static_cast<int>(chunks.size() *
                        sizeof(parcel_buffer_type::transmission_chunk_type)));
                // clang-format on
            }

            state_ = sent_transmission_chunks;
            return send_data();
        }

        bool send_data()
        {
            HPX_ASSERT(state_ == sent_transmission_chunks);
            if (!request_done())
            {
                return false;
            }

            if (!header_.piggy_back())
            {
                hpx::util::gasnet_environment::scoped_lock l;
                // clang-format off
                std::memcpy(hpx::util::gasnet_environment::segments
                    [hpx::util::gasnet_environment::rank()].addr,
                    buffer_.data_.data(), buffer_.data_.size());

                hpx::util::gasnet_environment::put(
                    static_cast<std::uint8_t*>(hpx::util::gasnet_environment::
                        segments[hpx::util::gasnet_environment::rank()].addr),
                    dst_,
                    static_cast<std::uint8_t*>(
                        hpx::util::gasnet_environment::segments[dst_].addr),
                    buffer_.data_.size());
                // clang-format on
            }
            state_ = sent_data;

            return send_chunks();
        }

        bool send_chunks()
        {
            HPX_ASSERT(state_ == sent_data);

            while (chunks_idx_ < buffer_.chunks_.size())
            {
                serialization::serialization_chunk& c =
                    buffer_.chunks_[chunks_idx_];
                if (c.type_ == serialization::chunk_type::chunk_type_pointer ||
                    c.type_ ==
                        serialization::chunk_type::chunk_type_const_pointer)
                {
                    if (!request_done())
                    {
                        return false;
                    }

                    hpx::util::gasnet_environment::scoped_lock l;

                    // clang-format off
                    std::memcpy(hpx::util::gasnet_environment::segments
                                    [hpx::util::gasnet_environment::rank()]
                                        .addr,
                        c.data_.cpos_, static_cast<int>(c.size_));

                    hpx::util::gasnet_environment::put(
                        static_cast<std::uint8_t*>(
                            hpx::util::gasnet_environment::segments
                                [hpx::util::gasnet_environment::rank()]
                                    .addr),
                        dst_,
                        static_cast<std::uint8_t*>(
                            hpx::util::gasnet_environment::segments[dst_].addr),
                        static_cast<int>(c.size_));
                    // clang-format on
                }

                ++chunks_idx_;
            }

            state_ = sent_chunks;

            return done();
        }

        bool done()
        {
            if (!request_done())
            {
                return false;
            }

            error_code ec(throwmode::lightweight);
            handler_(ec);
            handler_.reset();
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
            buffer_.data_point_.time_ =
                hpx::chrono::high_resolution_clock::now() -
                buffer_.data_point_.time_;
            pp_->add_sent_data(buffer_.data_point_);
#endif
            buffer_.clear();

            state_ = initialized;

            return true;
        }

        bool request_done()
        {
            hpx::util::gasnet_environment::scoped_try_lock l;
            if (!l.locked)
            {
                return false;
            }

            return true;
        }

        connection_state state_;
        sender_type* sender_;
        int tag_;
        int dst_;

        using handler_type = hpx::move_only_function<void(error_code const&)>;
        handler_type handler_;

        using post_handler_type = hpx::move_only_function<void(
            error_code const&, parcelset::locality const&,
            std::shared_ptr<sender_connection>)>;
        post_handler_type postprocess_handler_;

        header header_;

        std::size_t chunks_idx_;
        char ack_;

        parcelset::parcelport* pp_;

        parcelset::locality there_;
    };
}    // namespace hpx::parcelset::policies::gasnet

#endif
