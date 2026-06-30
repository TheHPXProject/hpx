//  Copyright (c) 2025 Jiakun Yan
//  Copyright (c) 2007-2013 Hartmut Kaiser
//  Copyright (c) 2014-2015 Thomas Heller
//  Copyright (c)      2020 Google
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_LCW)

#include <hpx/assert.hpp>

#include <hpx/modules/lcw_base.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/parcelport_lcw/sendrecv/receiver_connection_sendrecv.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace hpx::parcelset::policies::lcw {
    receiver_connection_sendrecv::receiver_connection_sendrecv(
        int dst, parcelset::parcelport* pp)
      : dst_rank(dst)
      , pp_((lcw::parcelport*) pp)
    {
    }

    void receiver_connection_sendrecv::load(char* header_buffer)
    {
        conn_start_time = util::lcw_environment::pcounter_now();
        util::lcw_environment::pcounter_add(
            util::lcw_environment::recv_conn_start, 1);
        header header_ = header(header_buffer);
        header_.assert_valid();
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
        buffer.data_point_.bytes_ =
            static_cast<std::size_t>(header_.numbytes());
        timer_.restart();
#endif
        device_p = &pp_->devices[header_.get_device_idx()];
        tag = header_.get_tag();
        // decode data
        buffer.data_.resize(header_.numbytes_nonzero_copy());
        char* piggy_back_data = header_.piggy_back_data();
        if (piggy_back_data)
        {
            need_recv_data = false;
            memcpy(buffer.data_.data(), piggy_back_data, buffer.data_.size());
        }
        else
        {
            need_recv_data = true;
        }
        need_recv_tchunks = false;
        if (header_.num_zero_copy_chunks() != 0)
        {
            // decode transmission chunk
            int num_zero_copy_chunks = header_.num_zero_copy_chunks();
            int num_non_zero_copy_chunks = header_.num_non_zero_copy_chunks();
            buffer.num_chunks_.first = num_zero_copy_chunks;
            buffer.num_chunks_.second = num_non_zero_copy_chunks;
            auto& tchunks = buffer.transmission_chunks_;
            tchunks.resize(num_zero_copy_chunks + num_non_zero_copy_chunks);
            size_t tchunks_length = tchunks.size() *
                sizeof(receiver_base::buffer_type::transmission_chunk_type);
            char* piggy_back_tchunk = header_.piggy_back_tchunk();
            if (piggy_back_tchunk)
            {
                memcpy(
                    (void*) tchunks.data(), piggy_back_tchunk, tchunks_length);
            }
            else
            {
                need_recv_tchunks = true;
            }
            // zero-copy chunks
            buffer.chunks_.resize(num_zero_copy_chunks);
            if (!pp_->allow_zero_copy_receive_optimizations())
            {
                chunk_buffers_.resize(num_zero_copy_chunks);
            }
        }
        // Calculate how many recvs we need to make
        int num_recv =
            need_recv_data + need_recv_tchunks + header_.num_zero_copy_chunks();
        sharedPtr_p = nullptr;
        if (num_recv > 0)
        {
            sharedPtr_p = new std::shared_ptr<receiver_connection_sendrecv>(
                shared_from_this());
        }
        // set state
        recv_chunks_idx = 0;
        recv_zero_copy_chunks_idx = 0;
        original_tag = tag;
        state.store(connection_state::initialized, std::memory_order_release);
        util::lcw_environment::log(util::lcw_environment::log_level_t::debug,
            "recv", "recv connection (%d, %d, %d) start!\n", dst_rank,
            ::lcw::get_rank(), tag);
    }

    receiver_connection_sendrecv::return_t
    receiver_connection_sendrecv::receive()
    {
        while (
            state.load(std::memory_order_acquire) == connection_state::locked)
        {
            continue;
        }

        switch (state.load(std::memory_order_acquire))
        {
        case connection_state::initialized:
            return receive_transmission_chunks();

        case connection_state::rcvd_transmission_chunks:
            return receive_data();

        case connection_state::rcvd_data:
            return receive_chunks();

        case connection_state::rcvd_chunks:
            return {true, nullptr};

        default:
            throw std::runtime_error("Unexpected recv state!");
        }
    }

    ::lcw::comp_t receiver_connection_sendrecv::unified_recv(
        void* address, size_t length)
    {
        ::lcw::comp_t completion =
            device_p->completion_manager_p->recv_followup->alloc_completion();
        bool ret = ::lcw::recv(device_p->device, dst_rank, tag, address, length,
            completion, sharedPtr_p);
        HPX_ASSERT(ret);
        HPX_UNUSED(ret);
        util::lcw_environment::log(util::lcw_environment::log_level_t::debug,
            "recv", "recvl (%d, %d, %d) device %d tag %d size %d\n", dst_rank,
            ::lcw::get_rank(), original_tag, device_p->idx, tag, length);
        tag = (tag + 1) % ::lcw::get_max_tag(device_p->device);
        return completion;
    }

    receiver_connection_sendrecv::return_t
    receiver_connection_sendrecv::receive_transmission_chunks()
    {
        auto const current_state = connection_state::initialized;
        auto const next_state = connection_state::rcvd_transmission_chunks;
        HPX_ASSERT(state.load(std::memory_order_acquire) == current_state);
        HPX_UNUSED(current_state);
        if (need_recv_tchunks)
        {
            auto& tchunks = buffer.transmission_chunks_;
            size_t tchunk_length = tchunks.size() *
                sizeof(receiver_base::buffer_type::transmission_chunk_type);
            state.store(connection_state::locked, std::memory_order_relaxed);
            ::lcw::comp_t completion =
                unified_recv(tchunks.data(), tchunk_length);
            state.store(next_state, std::memory_order_release);
            return {false, completion};
        }
        else
        {
            state.store(next_state, std::memory_order_release);
            return receive_data();
        }
    }

    receiver_connection_sendrecv::return_t
    receiver_connection_sendrecv::receive_data()
    {
        auto const current_state = connection_state::rcvd_transmission_chunks;
        auto const next_state = connection_state::rcvd_data;
        HPX_ASSERT(state.load(std::memory_order_acquire) == current_state);
        HPX_UNUSED(current_state);
        if (need_recv_data)
        {
            state.store(connection_state::locked, std::memory_order_relaxed);
            ::lcw::comp_t completion =
                unified_recv(buffer.data_.data(), buffer.data_.size());
            state.store(next_state, std::memory_order_release);
            return {false, completion};
        }
        else
        {
            state.store(next_state, std::memory_order_release);
            return receive_chunks();
        }
    }

    receiver_connection_sendrecv::return_t
    receiver_connection_sendrecv::receive_chunks()
    {
        if (pp_->allow_zero_copy_receive_optimizations())
        {
            return receive_chunks_zc();
        }
        else
        {
            return receive_chunks_nzc();
        }
    }

    void receiver_connection_sendrecv::receive_chunks_zc_preprocess()
    {
        HPX_ASSERT(recv_chunks_idx == 0);

        auto const num_zero_copy_chunks = static_cast<std::size_t>(
            static_cast<std::uint32_t>(buffer.num_chunks_.first));
        if (num_zero_copy_chunks != 0)
        {
            HPX_ASSERT(buffer.chunks_.size() == num_zero_copy_chunks);

            // De-serialize the parcels such that all data but the
            // zero-copy chunks are in place. This de-serialization also
            // allocates all zero-chunk buffers and stores those in the
            // chunks array for the subsequent networking to place the
            // received data directly.
            for (std::size_t i = 0; i != num_zero_copy_chunks; ++i)
            {
                auto const chunk_size = static_cast<std::size_t>(
                    buffer.transmission_chunks_[i].second);
                buffer.chunks_[i] =
                    serialization::create_pointer_chunk(nullptr, chunk_size);
            }

            parcels_ = decode_parcels_zero_copy(*pp_, buffer);

            // note that at this point, buffer_.chunks_ will have
            // entries for all chunks, including the non-zero-copy ones
        }

        // we should have received at least one parcel if there are
        // zero-copy chunks to be received
        HPX_ASSERT(parcels_.empty() || !buffer.chunks_.empty());
    }

    receiver_connection_sendrecv::return_t
    receiver_connection_sendrecv::receive_chunks_zc()
    {
        auto const current_state = connection_state::rcvd_data;
        auto const next_state = connection_state::rcvd_chunks;
        HPX_ASSERT(state.load(std::memory_order_acquire) == current_state);
        HPX_UNUSED(current_state);
        // handle zero-copy receive, this should be done on the first entry
        // to receive_chunks only
        if (parcels_.empty())
        {
            receive_chunks_zc_preprocess();
        }

        while (recv_chunks_idx != buffer.chunks_.size())
        {
            auto& chunk = buffer.chunks_[recv_chunks_idx++];
            if (chunk.type_ == serialization::chunk_type::chunk_type_index)
            {
                continue;    // skip non-zero-copy chunks
            }

            // the zero-copy chunks come first in the transmission_chunks_
            // array
            std::size_t chunk_size =
                buffer.transmission_chunks_[recv_zero_copy_chunks_idx++].second;

            // the parcel de-serialization above should have allocated the
            // correct amount of memory
            HPX_ASSERT_MSG(
                chunk.data() != nullptr && chunk.size() == chunk_size,
                "zero-copy chunk buffers should have been initialized "
                "during de-serialization");
            HPX_UNUSED(chunk_size);

            state.store(connection_state::locked, std::memory_order_relaxed);
            ::lcw::comp_t completion = unified_recv(chunk.data(), chunk.size());
            state.store(current_state, std::memory_order_release);
            return {false, completion};
        }
        HPX_ASSERT_MSG(recv_zero_copy_chunks_idx == buffer.num_chunks_.first,
            "observed: {}, expected {}", recv_zero_copy_chunks_idx,
            buffer.num_chunks_.first);
        state.store(next_state, std::memory_order_release);
        return {true, nullptr};
    }

    receiver_connection_sendrecv::return_t
    receiver_connection_sendrecv::receive_chunks_nzc()
    {
        auto const current_state = connection_state::rcvd_data;
        auto const next_state = connection_state::rcvd_chunks;
        HPX_ASSERT(state.load(std::memory_order_acquire) == current_state);
        HPX_UNUSED(current_state);
        while (recv_chunks_idx < buffer.chunks_.size())
        {
            std::size_t idx = recv_chunks_idx++;
            std::size_t chunk_size = buffer.transmission_chunks_[idx].second;
            auto& chunk = chunk_buffers_[idx];
            chunk.resize(chunk_size);
            buffer.chunks_[idx] =
                serialization::create_pointer_chunk(chunk.data(), chunk.size());
            state.store(connection_state::locked, std::memory_order_relaxed);
            ::lcw::comp_t completion = unified_recv(chunk.data(), chunk.size());
            state.store(current_state, std::memory_order_release);
            return {false, completion};
        }
        state.store(next_state, std::memory_order_release);
        return {true, nullptr};
    }

    void receiver_connection_sendrecv::done()
    {
        util::lcw_environment::pcounter_add(
            util::lcw_environment::recv_conn_end, 1);
        util::lcw_environment::log(util::lcw_environment::log_level_t::debug,
            "recv", "recv connection (%d, %d, %d, %d) done!\n", dst_rank,
            ::lcw::get_rank(), original_tag, tag - original_tag + 1);
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
        buffer.data_point_.time_ = timer_.elapsed_nanoseconds();
#endif
        if (tag == original_tag)
        {
            util::lcw_environment::pcounter_add(
                util::lcw_environment::recv_eager_conn_timer,
                util::lcw_environment::pcounter_since(conn_start_time));
        }
        else
        {
            util::lcw_environment::pcounter_add(
                util::lcw_environment::recv_rdv_conn_timer,
                util::lcw_environment::pcounter_since(conn_start_time));
        }
        auto handle_parcels_start_time = util::lcw_environment::pcounter_now();
        if (parcels_.empty())
        {
            // decode and handle received data
            HPX_ASSERT(buffer.num_chunks_.first == 0 ||
                !pp_->allow_zero_copy_receive_optimizations());
            handle_received_parcels(decode_parcels(*pp_, HPX_MOVE(buffer)));
            chunk_buffers_.clear();
        }
        else
        {
            // handle the received zero-copy parcels.
            HPX_ASSERT(buffer.num_chunks_.first != 0 &&
                pp_->allow_zero_copy_receive_optimizations());
            handle_received_parcels(HPX_MOVE(parcels_));
        }
        util::lcw_environment::pcounter_add(
            util::lcw_environment::handle_parcels,
            util::lcw_environment::pcounter_since(handle_parcels_start_time));
        buffer.data_.clear();
        parcels_.clear();
    }
}    // namespace hpx::parcelset::policies::lcw

#endif
