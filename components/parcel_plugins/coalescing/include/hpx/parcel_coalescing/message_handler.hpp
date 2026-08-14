//  Copyright (c) 2007-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCEL_COALESCING)
#include <hpx/modules/functional.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/synchronization.hpp>

#include <hpx/parcel_coalescing/message_buffer.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::plugins::parcel {

    namespace detail {

        struct histogram_collector_base
        {
            virtual ~histogram_collector_base() {}
            virtual void add_value(std::int64_t value) = 0;
        };
    }    // namespace detail

    struct HPX_LIBRARY_EXPORT coalescing_message_handler
      : parcelset::policies::message_handler
    {
        using mutex_type = hpx::spinlock;

    public:
        using write_handler_type =
            parcelset::policies::message_handler::write_handler_type;

        coalescing_message_handler(char const* action_name,
            parcelset::parcelport* pp,
            std::size_t num = static_cast<std::size_t>(-1),
            std::size_t interval = static_cast<std::size_t>(-1));

        void put_parcel(parcelset::locality const& dest, parcelset::parcel p,
            write_handler_type f) override;

        bool flush(parcelset::policies::message_handler::flush_mode mode,
            bool stop_buffering = false) override;

        void flush_terminate();

        // access performance counter data
        std::int64_t get_parcels_count(bool reset);
        std::int64_t get_messages_count(bool reset);
        std::int64_t get_parcels_per_message_count(bool reset);
        std::int64_t get_average_time_between_parcels(bool reset);
        std::vector<std::int64_t> get_time_between_parcels_histogram(
            bool reset);
        void get_time_between_parcels_histogram_creator(
            std::int64_t min_boundary, std::int64_t max_boundary,
            std::int64_t num_buckets,
            hpx::function<std::vector<std::int64_t>(bool)>& result);

        // register the given action
        static void register_action(char const* action, error_code& ec);

    protected:
        bool timer_flush();
        bool flush_locked(std::unique_lock<mutex_type>& l,
            parcelset::policies::message_handler::flush_mode mode,
            bool stop_buffering, bool cancel_timer);

        void update_num_messages();
        void update_interval();

    private:
        mutable mutex_type mtx_;
        parcelset::parcelport* pp_;
        std::size_t num_coalesced_parcels_;
        std::size_t interval_;
        detail::message_buffer buffer_;
        util::pool_timer timer_;
        bool stopped_;
        bool allow_background_flush_;
        std::string action_name_;

        // performance counter data
        std::int64_t num_parcels_;
        std::int64_t reset_num_parcels_;
        std::int64_t reset_num_parcels_per_message_parcels_;
        std::int64_t num_messages_;
        std::int64_t reset_num_messages_;
        std::int64_t reset_num_parcels_per_message_messages_;
        std::int64_t started_at_;
        std::int64_t reset_time_num_parcels_;
        std::int64_t last_parcel_time_;

        // collects percentiles
        std::unique_ptr<detail::histogram_collector_base> time_between_parcels_;
        std::int64_t histogram_min_boundary_;
        std::int64_t histogram_max_boundary_;
        std::int64_t histogram_num_buckets_;
    };
}    // namespace hpx::plugins::parcel

#include <hpx/config/warnings_suffix.hpp>

#endif
