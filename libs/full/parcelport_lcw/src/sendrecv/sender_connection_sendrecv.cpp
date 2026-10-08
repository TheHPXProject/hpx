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

#include <hpx/modules/lcw_base.hpp>
#include <hpx/parcelport_lcw/header.hpp>
#include <hpx/parcelport_lcw/locality.hpp>
#include <hpx/parcelport_lcw/parcelport_lcw.hpp>
#include <hpx/parcelport_lcw/receiver_base.hpp>
#include <hpx/parcelport_lcw/sendrecv/sender_connection_sendrecv.hpp>

#include <hpx/assert.hpp>

#include <atomic>
#include <memory>
#include <utility>
#include <vector>

namespace hpx::parcelset::policies::lcw {

    std::atomic<int> sender_connection_sendrecv::next_tag = 0;

    void sender_connection_sendrecv::load(
        sender_connection_sendrecv::handler_type&& handler,
        sender_connection_sendrecv::postprocess_handler_type&&
            parcel_postprocess)
    {
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
        timer_.restart();
#endif
        conn_start_time = util::lcw_environment::pcounter_now();
        HPX_ASSERT(!handler_);
        HPX_ASSERT(!postprocess_handler_);
        HPX_ASSERT(!buffer_.data_.empty());
        handler_ = HPX_MOVE(handler);
        postprocess_handler_ = HPX_MOVE(parcel_postprocess);

        // build header
        header_buffer.resize(header::get_header_size(
            buffer_, pp_->get_zero_copy_serialization_threshold()));
        header_ =
            header(buffer_, (char*) header_buffer.data(), header_buffer.size());
        HPX_ASSERT((header_.num_zero_copy_chunks() == 0) ==
            buffer_.transmission_chunks_.empty());
        need_send_data = false;
        need_send_tchunks = false;
        // Calculate how many sends we need to make
        int num_send = 0;    // header doesn't need to do tag matching
        if (header_.piggy_back_data() == nullptr)
        {
            need_send_data = true;
            ++num_send;
        }
        if (header_.num_zero_copy_chunks() != 0)
        {
            if (header_.piggy_back_tchunk() == nullptr)
            {
                need_send_tchunks = true;
                ++num_send;
            }
            num_send += header_.num_zero_copy_chunks();
        }
        tag = 0;    // If no need to post send, then tag can be ignored.
        sharedPtr_p = new std::shared_ptr<sender_connection_sendrecv>(
            std::dynamic_pointer_cast<sender_connection_sendrecv>(
                shared_from_this()));
        if (num_send > 0)
        {
            tag = next_tag.fetch_add(num_send) %
                ::lcw::get_max_tag(device_p->device);
            if ((int) tag <= ::lcw::get_max_tag(device_p->device) &&
                (int) tag + num_send > ::lcw::get_max_tag(device_p->device))
                util::lcw_environment::log(
                    util::lcw_environment::log_level_t::debug, "tag",
                    "Rank %d Wrap around!\n", ::lcw::get_rank());
        }
        header_.set_device_idx(device_p->idx);
        header_.set_tag(tag);
        send_chunks_idx = 0;
        // set state
        profile_start_hook(header_);
        state.store(connection_state::initialized, std::memory_order_release);
        original_tag = tag;
        util::lcw_environment::log(util::lcw_environment::log_level_t::debug,
            "send",
            "send connection (%d, %d, %d, %d) tchunks "
            "%d data %d chunks %d start!\n",
            ::lcw::get_rank(), dst_rank, original_tag, num_send,
            header_.piggy_back_tchunk() != nullptr,
            header_.piggy_back_data() != nullptr,
            header_.num_zero_copy_chunks());
        was_waiting = false;
    }

    sender_connection_sendrecv::return_t sender_connection_sendrecv::send_nb()
    {
        while (
            state.load(std::memory_order_acquire) == connection_state::locked)
        {
            continue;
        }
        if (was_waiting)
        {
            was_waiting = false;
            util::lcw_environment::pcounter_add(
                util::lcw_environment::send_conn_wait_timer,
                util::lcw_environment::pcounter_since(wait_start_time));
        }

        switch (state.load(std::memory_order_acquire))
        {
        case connection_state::initialized:
            return send_header();

        case connection_state::sent_header:
            return send_transmission_chunks();

        case connection_state::sent_transmission_chunks:
            return send_data();

        case connection_state::sent_data:
            return send_chunks();

        case connection_state::sent_chunks:
            return {return_status_t::done, nullptr};

        default:
            throw std::runtime_error("Unexpected send state!");
        }
    }

    sender_connection_sendrecv::return_t
    sender_connection_sendrecv::send_header()
    {
        auto const current_state = connection_state::initialized;
        auto const next_state = connection_state::sent_header;
        HPX_ASSERT(state.load(std::memory_order_acquire) == current_state);
        HPX_UNUSED(current_state);
        auto completion =
            device_p->completion_manager_p->send->alloc_completion();
        state.store(connection_state::locked, std::memory_order_relaxed);
        bool ret = ::lcw::put(device_p->device, dst_rank, header_buffer.data(),
            static_cast<int64_t>(header_buffer.size()), completion,
            sharedPtr_p);
        if (ret)
        {
            util::lcw_environment::log(
                util::lcw_environment::log_level_t::debug, "send",
                "put(%d, %d, %d) length %lu\n", ::lcw::get_rank(), dst_rank,
                tag, header_buffer.size());
            wait_start_time = util::lcw_environment::pcounter_now();
            was_waiting = true;
            state.store(next_state, std::memory_order_release);
            return {return_status_t::wait, completion};
        }
        else
        {
            state.store(current_state, std::memory_order_release);
            return {return_status_t::retry, nullptr};
        }
    }

    sender_connection_sendrecv::return_t
    sender_connection_sendrecv::unified_followup_send(
        void* address, size_t length)
    {
        auto completion =
            device_p->completion_manager_p->send->alloc_completion();
        bool ret = ::lcw::send(device_p->device, dst_rank, tag, address, length,
            completion, sharedPtr_p);
        if (ret)
        {
            util::lcw_environment::log(
                util::lcw_environment::log_level_t::debug, "send",
                "send(%d, %d, %d) device %d, tag %d size %d\n",
                ::lcw::get_rank(), dst_rank, original_tag, device_p->idx, tag,
                length);
            tag = (tag + 1) % ::lcw::get_max_tag(device_p->device);
            return {return_status_t::wait, completion};
        }
        else
        {
            return {return_status_t::retry, nullptr};
        }
    }

    sender_connection_sendrecv::return_t
    sender_connection_sendrecv::send_transmission_chunks()
    {
        auto const current_state = connection_state::sent_header;
        auto const next_state = connection_state::sent_transmission_chunks;
        HPX_ASSERT(state.load(std::memory_order_acquire) == current_state);
        if (!need_send_tchunks)
        {
            state.store(next_state, std::memory_order_release);
            return send_data();
        }

        std::vector<typename parcel_buffer_type::transmission_chunk_type>&
            tchunks = buffer_.transmission_chunks_;
        size_t tchunks_size = tchunks.size() *
            sizeof(parcel_buffer_type::transmission_chunk_type);
        state.store(connection_state::locked, std::memory_order_relaxed);
        auto ret = unified_followup_send(tchunks.data(), tchunks_size);
        if (ret.status == return_status_t::done)
        {
            state.store(next_state, std::memory_order_release);
            return send_data();
        }
        else if (ret.status == return_status_t::retry)
        {
            state.store(current_state, std::memory_order_release);
            return ret;
        }
        else
        {
            state.store(next_state, std::memory_order_release);
            return ret;
        }
    }

    sender_connection_sendrecv::return_t sender_connection_sendrecv::send_data()
    {
        auto const current_state = connection_state::sent_transmission_chunks;
        auto const next_state = connection_state::sent_data;
        HPX_ASSERT(state.load(std::memory_order_acquire) == current_state);
        if (!need_send_data)
        {
            state.store(next_state, std::memory_order_release);
            return send_chunks();
        }
        state.store(connection_state::locked, std::memory_order_relaxed);
        auto ret =
            unified_followup_send(buffer_.data_.data(), buffer_.data_.size());
        if (ret.status == return_status_t::done)
        {
            state.store(next_state, std::memory_order_release);
            return send_chunks();
        }
        else if (ret.status == return_status_t::retry)
        {
            state.store(current_state, std::memory_order_release);
            return ret;
        }
        else
        {
            state.store(next_state, std::memory_order_release);
            return ret;
        }
    }

    sender_connection_sendrecv::return_t
    sender_connection_sendrecv::send_chunks()
    {
        auto const current_state = connection_state::sent_data;
        auto const next_state = connection_state::sent_chunks;
        HPX_ASSERT(state.load(std::memory_order_acquire) == current_state);

        while (send_chunks_idx < buffer_.chunks_.size())
        {
            serialization::serialization_chunk& chunk =
                buffer_.chunks_[send_chunks_idx];
            // Accept both pointer chunk kinds (save uses const_pointer; #812).
            if (chunk.type_ == serialization::chunk_type::chunk_type_pointer ||
                chunk.type_ ==
                    serialization::chunk_type::chunk_type_const_pointer)
            {
                state.store(
                    connection_state::locked, std::memory_order_relaxed);
                auto ret = unified_followup_send(
                    const_cast<void*>(std::as_const(chunk).data()),
                    chunk.size_);
                if (ret.status == return_status_t::done)
                {
                    ++send_chunks_idx;
                    state.store(current_state, std::memory_order_release);
                    continue;
                }
                else if (ret.status == return_status_t::retry)
                {
                    state.store(current_state, std::memory_order_release);
                    return ret;
                }
                else
                {
                    ++send_chunks_idx;
                    state.store(current_state, std::memory_order_release);
                    return ret;
                }
            }
            else
            {
                ++send_chunks_idx;
            }
        }

        state.store(next_state, std::memory_order_release);
        return {return_status_t::done, nullptr};
    }

    void sender_connection_sendrecv::done()
    {
        util::lcw_environment::log(util::lcw_environment::log_level_t::debug,
            "send", "send connection (%d, %d, %d, %d) done!\n",
            ::lcw::get_rank(), dst_rank, original_tag, tag - original_tag + 1);
        profile_end_hook();
        error_code ec;
        handler_(ec);
        handler_.reset();
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
        data_point_.time_ = timer_.elapsed_nanoseconds();
        pp_->add_sent_data(data_point_);
#endif
        buffer_.clear();
        if (tag == original_tag)
        {
            util::lcw_environment::pcounter_add(
                util::lcw_environment::send_eager_conn_timer,
                util::lcw_environment::pcounter_since(conn_start_time));
        }
        else
        {
            util::lcw_environment::pcounter_add(
                util::lcw_environment::send_rdv_conn_timer,
                util::lcw_environment::pcounter_since(conn_start_time));
        }

        if (postprocess_handler_)
        {
            // Return this connection to the connection cache.
            // After postprocess_handler is invoked, this connection can be
            // obtained by another thread.
            // so make sure to call this at the very end.
            hpx::move_only_function<void(error_code const&,
                parcelset::locality const&,
                std::shared_ptr<sender_connection_base>)>
                postprocess_handler;
            std::swap(postprocess_handler, postprocess_handler_);
            error_code ec2;
            postprocess_handler(ec2, there_, shared_from_this());
        }
    }

    bool sender_connection_sendrecv::tryMerge(
        std::shared_ptr<sender_connection_base> const& other_base)
    {
        // We cannot merge any message here.
        HPX_UNUSED(other_base);
        return false;
    }
}    // namespace hpx::parcelset::policies::lcw

#endif
