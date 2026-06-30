//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2013-2014 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This is needed to make everything work with the Intel MPI library header
#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/io_service.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/threading.hpp>
#include <hpx/modules/tracing.hpp>
#include <hpx/modules/util.hpp>

#include <hpx/parcelset_base/parcelport.hpp>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>
#include <system_error>
#include <utility>

namespace hpx::parcelset {

    ///////////////////////////////////////////////////////////////////////////
    parcelport::parcelport(util::runtime_configuration const& ini,
        locality here, std::string const& type,
        std::size_t zero_copy_serialization_threshold)
      : num_parcel_destinations_(0)
      , here_(HPX_MOVE(here))
      , max_inbound_message_size_(0)
      , max_outbound_message_size_(0)
      , allow_array_optimizations_(true)
      , allow_zero_copy_optimizations_(true)
      , allow_zero_copy_receive_optimizations_(true)
      , async_serialization_(false)
      , priority_(hpx::util::get_entry_as<int>(
            ini, "hpx.parcel." + type + ".priority", 0))
      , type_(type)
      , zero_copy_serialization_threshold_(zero_copy_serialization_threshold)
    {
        std::string key("hpx.parcel.");
        key += type;

        // clang-format off
        max_inbound_message_size_ = static_cast<std::int64_t>(
            ini.get_max_inbound_message_size(type));
        max_outbound_message_size_ = static_cast<std::int64_t>(
            ini.get_max_outbound_message_size(type));
        // clang-format on

        if (hpx::util::get_entry_as<int>(ini, key + ".array_optimization", 1) ==
            0)
        {
            allow_array_optimizations_ = false;
            allow_zero_copy_optimizations_ = false;
            allow_zero_copy_receive_optimizations_ = false;
        }
        else if (hpx::util::get_entry_as<int>(
                     ini, key + ".zero_copy_optimization", 1) == 0)
        {
            allow_zero_copy_optimizations_ = false;
            allow_zero_copy_receive_optimizations_ = false;
        }
        else if (hpx::util::get_entry_as<int>(
                     ini, key + ".zero_copy_receive_optimization", 1) == 0)
        {
            allow_zero_copy_receive_optimizations_ = false;
        }

        if (hpx::util::get_entry_as<int>(
                ini, key + ".async_serialization", 0) != 0)
        {
            async_serialization_ = true;
        }
    }

    int parcelport::priority() const noexcept
    {
        return priority_;
    }

    std::string const& parcelport::type() const noexcept
    {
        return type_;
    }

    std::size_t parcelport::get_zero_copy_serialization_threshold()
        const noexcept
    {
        return zero_copy_serialization_threshold_;
    }

    locality const& parcelport::here() const noexcept
    {
        return here_;
    }

    void parcelport::initialized() {}

    bool parcelport::can_connect(
        locality const&, bool use_alternative_parcelport)
    {
        return use_alternative_parcelport || can_bootstrap();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Update performance counter data
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
    void parcelport::add_received_data(parcelset::data_point const& data)
    {
        parcels_received_.add_data(data);
    }

    void parcelport::add_sent_data(parcelset::data_point const& data)
    {
        parcels_sent_.add_data(data);
    }

    void parcelport::add_received_data(
        [[maybe_unused]] char const* action, parcelset::data_point const& data)
    {
#if defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS)
        if (action != nullptr)
        {
            action_parcels_received_.add_data(action, data);
            return;
        }
#endif
        add_received_data(data);
    }

    void parcelport::add_sent_data(
        [[maybe_unused]] char const* action, parcelset::data_point const& data)
    {
#if defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS)
        if (action != nullptr)
        {
            action_parcels_sent_.add_data(action, data);
            return;
        }
#endif
        add_sent_data(data);
    }

    ///////////////////////////////////////////////////////////////////////////
    // number of parcels sent
    std::int64_t parcelport::get_parcel_send_count(bool reset)
    {
        return parcels_sent_.num_parcels(reset);
    }

    // number of messages sent
    std::int64_t parcelport::get_message_send_count(bool reset)
    {
        return parcels_sent_.num_messages(reset);
    }

    // number of parcels received
    std::int64_t parcelport::get_parcel_receive_count(bool reset)
    {
        return parcels_received_.num_parcels(reset);
    }

    // number of messages received
    std::int64_t parcelport::get_message_receive_count(bool reset)
    {
        return parcels_received_.num_messages(reset);
    }

    // the total time it took for all sends, from async_write to the
    // completion handler (nanoseconds)
    std::int64_t parcelport::get_sending_time(bool reset)
    {
        return parcels_sent_.total_time(reset);
    }

    // the total time it took for all receives, from async_read to the
    // completion handler (nanoseconds)
    std::int64_t parcelport::get_receiving_time(bool reset)
    {
        return parcels_received_.total_time(reset);
    }

    // the total time it took for all sender-side serialization operations
    // (nanoseconds)
    std::int64_t parcelport::get_sending_serialization_time(bool reset)
    {
        return parcels_sent_.total_serialization_time(reset);
    }

    // the total time it took for all receiver-side serialization
    // operations (nanoseconds)
    std::int64_t parcelport::get_receiving_serialization_time(bool reset)
    {
        return parcels_received_.total_serialization_time(reset);
    }

    // total data sent (bytes)
    std::int64_t parcelport::get_data_sent(bool reset)
    {
        return parcels_sent_.total_bytes(reset);
    }

    // total data (uncompressed) sent (bytes)
    std::int64_t parcelport::get_raw_data_sent(bool reset)
    {
        return parcels_sent_.total_raw_bytes(reset);
    }

    // total data received (bytes)
    std::int64_t parcelport::get_data_received(bool reset)
    {
        return parcels_received_.total_bytes(reset);
    }

    // total data (uncompressed) received (bytes)
    std::int64_t parcelport::get_raw_data_received(bool reset)
    {
        return parcels_received_.total_raw_bytes(reset);
    }

    std::int64_t parcelport::get_buffer_allocate_time_sent(bool reset)
    {
        return parcels_sent_.total_buffer_allocate_time(reset);
    }

    std::int64_t parcelport::get_buffer_allocate_time_received(bool reset)
    {
        return parcels_received_.total_buffer_allocate_time(reset);
    }

    //// total zero-copy chunks sent
    std::int64_t parcelport::get_zchunks_send_count(bool reset)
    {
        return parcels_sent_.num_zchunks(reset);
    }

    //// total zero-copy chunks received
    std::int64_t parcelport::get_zchunks_recv_count(bool reset)
    {
        return parcels_received_.num_zchunks(reset);
    }

    //// the maximum number of zero-copy chunks per message sent
    std::int64_t parcelport::get_zchunks_send_per_msg_count_max(bool reset)
    {
        return parcels_sent_.num_zchunks_per_msg_max(reset);
    }

    //// the maximum number of zero-copy chunks per message received
    std::int64_t parcelport::get_zchunks_recv_per_msg_count_max(bool reset)
    {
        return parcels_received_.num_zchunks_per_msg_max(reset);
    }

    //// the size of zero-copy chunks per message sent
    std::int64_t parcelport::get_zchunks_send_size(bool reset)
    {
        return parcels_sent_.size_zchunks_total(reset);
    }

    //// the size of zero-copy chunks per message received
    std::int64_t parcelport::get_zchunks_recv_size(bool reset)
    {
        return parcels_received_.size_zchunks_total(reset);
    }

    //// the maximum size of zero-copy chunks per message sent
    std::int64_t parcelport::get_zchunks_send_size_max(bool reset)
    {
        return parcels_sent_.size_zchunks_max(reset);
    }

    //// the maximum size of zero-copy chunks per message received
    std::int64_t parcelport::get_zchunks_recv_size_max(bool reset)
    {
        return parcels_received_.size_zchunks_max(reset);
    }

    ///////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS)
    // same as above, just separated data for each action
    // number of parcels sent
    std::int64_t parcelport::get_action_parcel_send_count(
        std::string const& action, bool reset)
    {
        if (action.empty())
            return parcels_sent_.num_parcels(reset);
        return action_parcels_sent_.num_parcels(action, reset);
    }

    // number of parcels received
    std::int64_t parcelport::get_action_parcel_receive_count(
        std::string const& action, bool reset)
    {
        if (action.empty())
            return parcels_received_.num_parcels(reset);
        return action_parcels_received_.num_parcels(action, reset);
    }

    // the total time it took for all sender-side serialization operations
    // (nanoseconds)
    std::int64_t parcelport::get_action_sending_serialization_time(
        std::string const& action, bool reset)
    {
        if (action.empty())
            return parcels_sent_.total_serialization_time(reset);
        return action_parcels_sent_.total_serialization_time(action, reset);
    }

    // the total time it took for all receiver-side serialization
    // operations (nanoseconds)
    std::int64_t parcelport::get_action_receiving_serialization_time(
        std::string const& action, bool reset)
    {
        if (action.empty())
            return parcels_received_.total_serialization_time(reset);
        return action_parcels_received_.total_serialization_time(action, reset);
    }

    // total data sent (bytes)
    std::int64_t parcelport::get_action_data_sent(
        std::string const& action, bool reset)
    {
        if (action.empty())
            return parcels_sent_.total_bytes(reset);
        return action_parcels_sent_.total_bytes(action, reset);
    }

    // total data received (bytes)
    std::int64_t parcelport::get_action_data_received(
        std::string const& action, bool reset)
    {
        if (action.empty())
            return parcels_received_.total_bytes(reset);
        return action_parcels_received_.total_bytes(action, reset);
    }
#endif
#endif

    std::int64_t parcelport::get_pending_parcels_count(bool /*reset*/)
    {
        std::lock_guard<hpx::spinlock> l(mtx_);
        std::int64_t count = 0;
        for (auto&& p : pending_parcels_)
        {
            count += static_cast<std::int64_t>(hpx::get<0>(p.second).size());
            HPX_ASSERT(
                hpx::get<0>(p.second).size() == hpx::get<1>(p.second).size());
        }
        return count;
    }

    ///////////////////////////////////////////////////////////////////////////
    std::int64_t get_max_inbound_size(parcelport const& pp)
    {
        return pp.get_max_inbound_message_size();
    }

    std::int64_t parcelport::get_max_inbound_message_size() const noexcept
    {
        return max_inbound_message_size_;
    }

    std::int64_t parcelport::get_max_outbound_message_size() const noexcept
    {
        return max_outbound_message_size_;
    }

    bool parcelport::allow_array_optimizations() const noexcept
    {
        return allow_array_optimizations_;
    }

    bool parcelport::allow_zero_copy_optimizations() const noexcept
    {
        return allow_zero_copy_optimizations_;
    }

    bool parcelport::allow_zero_copy_receive_optimizations() const noexcept
    {
        return allow_zero_copy_receive_optimizations_;
    }

    bool parcelport::async_serialization() const noexcept
    {
        return async_serialization_;
    }

    ///////////////////////////////////////////////////////////////////////////
    // the code below is needed to bootstrap the parcel layer
    void parcelport::early_pending_parcel_handler(
        std::error_code const& ec, parcel const& p)
    {
        if (ec)
        {
            // all errors during early parcel handling are fatal
            std::exception_ptr const exception =
                HPX_GET_EXCEPTION(ec, "early_pending_parcel_handler",
                    "error while handling early parcel: " + ec.message() + "(" +
                        std::to_string(ec.value()) + ")" +
                        parcelset::dump_parcel(p));

            hpx::report_error(exception);
            return;
        }

#if defined(HPX_HAVE_PARCEL_PROFILING)
        hpx::tracing::send_parcel(
            p.parcel_id().get_lsb(), p.size(), p.destination_locality_id());
#endif
    }

}    // namespace hpx::parcelset

#endif
