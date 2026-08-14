//  Copyright (c) 2023      Christopher Taylor
//  Copyright (c) 2014-2015 Thomas Heller
//  Copyright (c) 2007-2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_GASNET)
#include <hpx/assert.hpp>
#include <hpx/modules/gasnet_base.hpp>
#include <hpx/modules/timing.hpp>

#include <hpx/modules/parcelset.hpp>
#include <hpx/parcelport_gasnet/header.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

#include <gasnet.h>

namespace hpx::parcelset::policies::gasnet {

    template <typename Parcelport>
    struct receiver_connection
    {
    private:
        enum connection_state
        {
            initialized,
            rcvd_transmission_chunks,
            rcvd_data,
            rcvd_chunks,
            sent_release_tag
        };

        using data_type = std::vector<char>;
        using buffer_type = parcel_buffer<data_type, data_type>;

    public:
        receiver_connection(int src, header h, Parcelport& pp) noexcept
          : state_(initialized)
          , src_(src)
          , tag_(h.tag())
          , header_(h)
          , request_ptr_(false)
          , chunks_idx_(0)
          , pp_(pp)
        {
            header_.assert_valid();
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
            parcelset::data_point& data = buffer_.data_point_;
            data.time_ = timer_.elapsed_nanoseconds();
            data.bytes_ = static_cast<std::size_t>(header_.numbytes());
#endif
            buffer_.data_.resize(static_cast<std::size_t>(header_.size()));
            buffer_.num_chunks_ = header_.num_chunks();
        }

        bool receive(std::size_t num_thread = -1)
        {
            switch (state_)
            {
            case initialized:
                return receive_transmission_chunks(num_thread);

            case rcvd_transmission_chunks:
                return receive_data(num_thread);

            case rcvd_data:
                return receive_chunks(num_thread);

            case rcvd_chunks:
                return send_release_tag(num_thread);

            case sent_release_tag:
                return done();

            default:
                HPX_ASSERT(false);
            }
            return false;
        }

        bool receive_transmission_chunks(std::size_t num_thread = -1)
        {
            auto self_ = hpx::util::gasnet_environment::rank();

            // determine the size of the chunk buffer
            std::size_t num_zero_copy_chunks = static_cast<std::size_t>(
                static_cast<std::uint32_t>(buffer_.num_chunks_.first));
            std::size_t num_non_zero_copy_chunks = static_cast<std::size_t>(
                static_cast<std::uint32_t>(buffer_.num_chunks_.second));
            buffer_.transmission_chunks_.resize(
                num_zero_copy_chunks + num_non_zero_copy_chunks);
            if (num_zero_copy_chunks != 0)
            {
                buffer_.chunks_.resize(num_zero_copy_chunks);
                {
                    hpx::util::gasnet_environment::scoped_lock l;
                    unsigned long elem[2] = {0, 0};
                    std::memcpy(elem,
                        hpx::util::gasnet_environment::segments[self_].addr,
                        static_cast<int>(buffer_.transmission_chunks_.size() *
                            sizeof(buffer_type::transmission_chunk_type)));
                    buffer_.transmission_chunks_.data()->first = elem[0];
                    buffer_.transmission_chunks_.data()->second = elem[1];
                    request_ptr_ = true;
                }
            }

            state_ = rcvd_transmission_chunks;

            return receive_data(num_thread);
        }

        bool receive_data(std::size_t num_thread = -1)
        {
            if (!request_done())
            {
                return false;
            }

            char* piggy_back = header_.piggy_back();
            if (piggy_back)
            {
                std::memcpy(
                    &buffer_.data_[0], piggy_back, buffer_.data_.size());
            }
            else
            {
                auto self_ = hpx::util::gasnet_environment::rank();
                hpx::util::gasnet_environment::scoped_lock l;
                std::memcpy(buffer_.data_.data(),
                    hpx::util::gasnet_environment::segments[self_].addr,
                    buffer_.data_.size());
                request_ptr_ = true;
            }

            state_ = rcvd_data;

            return receive_chunks(num_thread);
        }

        bool receive_chunks(std::size_t num_thread = -1)
        {
            while (chunks_idx_ < buffer_.chunks_.size())
            {
                if (!request_done())
                {
                    return false;
                }

                std::size_t idx = chunks_idx_++;
                std::size_t chunk_size =
                    buffer_.transmission_chunks_[idx].second;

                data_type& c = buffer_.chunks_[idx];
                c.resize(chunk_size);
                {
                    auto self_ = hpx::util::gasnet_environment::rank();
                    hpx::util::gasnet_environment::scoped_lock l;
                    std::memcpy(c.data(),
                        hpx::util::gasnet_environment::segments[self_].addr,
                        c.size());
                    request_ptr_ = true;
                }
            }

            state_ = rcvd_chunks;

            return send_release_tag(num_thread);
        }

        bool send_release_tag(std::size_t num_thread = -1)
        {
            if (!request_done())
            {
                return false;
            }
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
            parcelset::data_point& data = buffer_.data_point_;
            data.time_ = timer_.elapsed_nanoseconds() - data.time_;
#endif
            {
                auto self_ = hpx::util::gasnet_environment::rank();
                hpx::util::gasnet_environment::scoped_lock l;
                std::memcpy(&tag_,
                    hpx::util::gasnet_environment::segments[self_].addr,
                    sizeof(int));
                request_ptr_ = true;
            }

            decode_parcels(pp_, HPX_MOVE(buffer_), num_thread);

            state_ = sent_release_tag;

            return done();
        }

        bool done() noexcept
        {
            return request_done();
        }

        bool request_done() noexcept
        {
            hpx::util::gasnet_environment::scoped_try_lock l;
            if (!l.locked)
            {
                return false;
            }

            return request_ptr_;
        }

#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
        hpx::chrono::high_resolution_timer timer_;
#endif
        connection_state state_;

        int src_;
        int tag_;
        header header_;
        buffer_type buffer_;

        bool request_ptr_;
        std::size_t chunks_idx_;

        Parcelport& pp_;
    };
}    // namespace hpx::parcelset::policies::gasnet

#endif
