//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCEL_COALESCING)

// MSVC generates a strange error in boost::accumulators if this is compiled
// with modules enables
#undef HPX_HAVE_CXX_MODULES

#include <boost/accumulators/accumulators.hpp>

#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/modules/plugin.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/statistics.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/util.hpp>

#include <hpx/modules/plugin_factories.hpp>
#include <hpx/parcel_coalescing/counter_registry.hpp>
#include <hpx/parcel_coalescing/message_handler.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::traits {

    // Inject additional configuration data into the factory registry for this
    // type. This information ends up in the systemwide configuration database
    // under the plugin specific section:
    //
    //      [hpx.plugins.coalescing_message_handler]
    //      ...
    //      num_messages = 50
    //      interval = 100
    //
    template <>
    struct plugin_config_data<hpx::plugins::parcel::coalescing_message_handler>
    {
        static constexpr char const* call() noexcept
        {
            return "num_messages = 50\n"
                   "interval = 100\n"
                   "allow_background_flush = 1";
        }
    };
}    // namespace hpx::traits

///////////////////////////////////////////////////////////////////////////////
HPX_REGISTER_PLUGIN_MODULE_DYNAMIC()
HPX_REGISTER_MESSAGE_HANDLER_FACTORY(
    hpx::plugins::parcel::coalescing_message_handler,
    coalescing_message_handler)

///////////////////////////////////////////////////////////////////////////////
namespace hpx::plugins::parcel {

    namespace detail { namespace {

        std::size_t get_num_messages(std::size_t num_messages)
        {
            return hpx::util::from_string<std::size_t>(hpx::get_config_entry(
                "hpx.plugins.coalescing_message_handler.num_messages",
                num_messages));
        }

        std::size_t get_interval(std::size_t interval)
        {
            return hpx::util::from_string<std::size_t>(hpx::get_config_entry(
                "hpx.plugins.coalescing_message_handler.interval", interval));
        }

        bool get_background_flush()
        {
            std::string const value = hpx::get_config_entry(
                "hpx.plugins.coalescing_message_handler.allow_background_flush",
                "1");
            return !value.empty() && value[0] != '0';
        }

        ////////////////////////////////////////////////////////////////////////
        struct histogram_collector final : histogram_collector_base
        {
            histogram_collector(std::int64_t min_boundary,
                std::int64_t max_boundary, std::int64_t num_buckets)
              : acc_(hpx::util::tag::histogram::num_bins =
                         static_cast<double>(num_buckets),
                    hpx::util::tag::histogram::min_range =
                        static_cast<double>(min_boundary),
                    hpx::util::tag::histogram::max_range =
                        static_cast<double>(max_boundary))
            {
            }

            void add_value(std::int64_t value) override
            {
                acc_(value);
            }

            // collects percentiles
            using histogram_collector_type =
                boost::accumulators::accumulator_set<double,
                    boost::accumulators::features<hpx::util::tag::histogram>>;
            histogram_collector_type acc_;
        };
    }}    // namespace detail

    void coalescing_message_handler::update_num_messages()
    {
        std::scoped_lock<mutex_type> l(mtx_);
        num_coalesced_parcels_ =
            detail::get_num_messages(num_coalesced_parcels_);
    }

    void coalescing_message_handler::update_interval()
    {
        std::scoped_lock<mutex_type> l(mtx_);
        interval_ = detail::get_interval(interval_);
    }

    coalescing_message_handler::coalescing_message_handler(
        char const* action_name, parcelset::parcelport* pp, std::size_t num,
        std::size_t interval)
      : pp_(pp)
      , num_coalesced_parcels_(detail::get_num_messages(num))
      , interval_(detail::get_interval(interval))
      , buffer_(num_coalesced_parcels_)
      , timer_(hpx::bind_back(&coalescing_message_handler::timer_flush, this),
            hpx::bind_back(&coalescing_message_handler::flush_terminate, this),
            std::string(action_name) + "_timer")
      , stopped_(false)
      , allow_background_flush_(detail::get_background_flush())
      , action_name_(action_name)
      , num_parcels_(0)
      , reset_num_parcels_(0)
      , reset_num_parcels_per_message_parcels_(0)
      , num_messages_(0)
      , reset_num_messages_(0)
      , reset_num_parcels_per_message_messages_(0)
      , started_at_(static_cast<std::int64_t>(
            hpx::chrono::high_resolution_clock::now()))
      , reset_time_num_parcels_(0)
      , last_parcel_time_(started_at_)
      , histogram_min_boundary_(-1)
      , histogram_max_boundary_(-1)
      , histogram_num_buckets_(-1)
    {
        // register performance counter functions
        coalescing_counter_registry::instance().register_action(action_name,
            hpx::bind_front(
                &coalescing_message_handler::get_parcels_count, this),
            hpx::bind_front(
                &coalescing_message_handler::get_messages_count, this),
            hpx::bind_front(
                &coalescing_message_handler::get_parcels_per_message_count,
                this),
            hpx::bind_front(
                &coalescing_message_handler::get_average_time_between_parcels,
                this),
            hpx::bind_front(&coalescing_message_handler::
                                get_time_between_parcels_histogram_creator,
                this));

        // register parameter update callbacks
        set_config_entry_callback(
            "hpx.plugins.coalescing_message_handler.num_messages",
            hpx::bind(&coalescing_message_handler::update_num_messages, this));
        set_config_entry_callback(
            "hpx.plugins.coalescing_message_handler.interval",
            hpx::bind(&coalescing_message_handler::update_interval, this));
    }

    void coalescing_message_handler::put_parcel(parcelset::locality const& dest,
        parcelset::parcel p, write_handler_type f)
    {
        std::unique_lock<mutex_type> l(mtx_);
        ++num_parcels_;

        // get time since last parcel
        auto const parcel_time = static_cast<std::int64_t>(
            hpx::chrono::high_resolution_clock::now());
        std::int64_t const time_since_last_parcel =
            parcel_time - last_parcel_time_;
        last_parcel_time_ = parcel_time;

        // collect data for time between parcels histogram
        if (time_between_parcels_)
            time_between_parcels_->add_value(time_since_last_parcel);

        std::chrono::microseconds const interval(interval_);

        // just send parcel if the coalescing was stopped or the buffer is
        // empty and time since last parcel is larger than coalescing interval.
        if (stopped_ ||
            (buffer_.empty() &&
                std::chrono::nanoseconds(time_since_last_parcel) > interval))
        {
            ++num_messages_;
            l.unlock();

            // this instance should not buffer parcels anymore
            pp_->put_parcel(dest, HPX_MOVE(p), HPX_MOVE(f));
            return;
        }

        detail::message_buffer::message_buffer_append_state const s =
            buffer_.append(dest, HPX_MOVE(p), HPX_MOVE(f));

        switch (s)
        {
        case detail::message_buffer::message_buffer_append_state::first_message:
            [[fallthrough]];
        case detail::message_buffer::message_buffer_append_state::normal:
            // start deadline timer to flush buffer
            {
                l.unlock();
                [[maybe_unused]] bool started = timer_.start(interval);
            }
            break;

        case detail::message_buffer::message_buffer_append_state::
            buffer_now_full:
            flush_locked(l,
                parcelset::policies::message_handler::flush_mode_buffer_full,
                false, true);
            break;

        case detail::message_buffer::message_buffer_append_state::
            singleton_buffer:
        default:
            l.unlock();
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "coalescing_message_handler::put_parcel",
                "unexpected return value from message_buffer::append");
        }
    }

    bool coalescing_message_handler::timer_flush()
    {
        // adjust timer if needed
        std::unique_lock<mutex_type> l(mtx_);
        if (!buffer_.empty())
        {
            flush_locked(l,
                parcelset::policies::message_handler::flush_mode_timer, false,
                false);
        }

        // do not restart timer for now, will be restarted on next parcel
        return false;
    }

    bool coalescing_message_handler::flush(
        parcelset::policies::message_handler::flush_mode mode,
        bool stop_buffering)
    {
        std::unique_lock<mutex_type> l(mtx_);
        return flush_locked(l, mode, stop_buffering, true);
    }

    void coalescing_message_handler::flush_terminate()
    {
        std::unique_lock<mutex_type> l(mtx_);
        flush_locked(l, parcelset::policies::message_handler::flush_mode_timer,
            true, true);
    }

    bool coalescing_message_handler::flush_locked(
        std::unique_lock<mutex_type>& l,
        parcelset::policies::message_handler::flush_mode mode,
        bool stop_buffering, bool cancel_timer)
    {
        HPX_ASSERT(l.owns_lock());

        // proceed with background work only if explicitly allowed
        if (!allow_background_flush_ &&
            mode ==
                parcelset::policies::message_handler::
                    flush_mode_background_work)
        {
            return false;
        }

        if (!stopped_ && stop_buffering)
        {
            stopped_ = true;
            {
                // interrupt timer
                hpx::unlock_guard<std::unique_lock<mutex_type>> ul(l);
                [[maybe_unused]] bool stopped = timer_.stop();
            }
        }
        else if (cancel_timer)
        {
            // interrupt timer
            hpx::unlock_guard<std::unique_lock<mutex_type>> ul(l);
            [[maybe_unused]] bool stopped = timer_.stop();
        }

        if (buffer_.empty())
            return false;

        detail::message_buffer buff(num_coalesced_parcels_);
        std::swap(buff, buffer_);

        ++num_messages_;

        // 26110: Caller failing to hold lock 'l'
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26110)
#endif
        l.unlock();
#if defined(HPX_MSVC)
#pragma warning(pop)
#endif

        HPX_ASSERT(nullptr != pp_);
        buff(pp_);    // 'invoke' the buffer

        return true;
    }

    // performance counter values
    std::int64_t coalescing_message_handler::get_average_time_between_parcels(
        bool reset)
    {
        std::scoped_lock<mutex_type> l(mtx_);
        auto const now = static_cast<std::int64_t>(
            hpx::chrono::high_resolution_clock::now());
        if (num_parcels_ == 0)
        {
            if (reset)
                started_at_ = now;
            return 0;
        }

        std::int64_t const num_parcels = num_parcels_ - reset_time_num_parcels_;
        if (num_parcels == 0)
        {
            if (reset)
                started_at_ = now;
            return 0;
        }

        HPX_ASSERT(now >= started_at_);
        std::int64_t const value = (now - started_at_) / num_parcels;

        if (reset)
        {
            started_at_ = now;
            reset_time_num_parcels_ = num_parcels_;
        }

        return value;
    }

    std::int64_t coalescing_message_handler::get_parcels_count(bool reset)
    {
        std::unique_lock<mutex_type> l(mtx_);
        std::int64_t const num_parcels = num_parcels_ - reset_num_parcels_;
        if (reset)
            reset_num_parcels_ = num_parcels_;
        return num_parcels;
    }

    std::int64_t coalescing_message_handler::get_parcels_per_message_count(
        bool reset)
    {
        std::unique_lock<mutex_type> l(mtx_);

        if (num_messages_ == 0)
        {
            if (reset)
            {
                reset_num_parcels_per_message_parcels_ = num_parcels_;
                reset_num_parcels_per_message_messages_ = num_messages_;
            }
            return 0;
        }

        std::int64_t const num_parcels =
            num_parcels_ - reset_num_parcels_per_message_parcels_;
        std::int64_t const num_messages =
            num_messages_ - reset_num_parcels_per_message_messages_;

        if (reset)
        {
            reset_num_parcels_per_message_parcels_ = num_parcels_;
            reset_num_parcels_per_message_messages_ = num_messages_;
        }

        if (num_messages == 0)
            return 0;

        return num_parcels / num_messages;
    }

    std::int64_t coalescing_message_handler::get_messages_count(bool reset)
    {
        std::unique_lock<mutex_type> l(mtx_);
        std::int64_t const num_messages = num_messages_ - reset_num_messages_;
        if (reset)
            reset_num_messages_ = num_messages_;
        return num_messages;
    }

    std::vector<std::int64_t>
    coalescing_message_handler::get_time_between_parcels_histogram(
        bool /* reset */)
    {
        std::vector<std::int64_t> result;

        std::unique_lock<mutex_type> l(mtx_);
        if (!time_between_parcels_)
        {
            l.unlock();
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "coalescing_message_handler::"
                "get_time_between_parcels_histogram",
                "parcel-arrival-histogram counter was not initialized for "
                "action type: {}",
                action_name_);
        }

        // first add histogram parameters
        result.push_back(histogram_min_boundary_);
        result.push_back(histogram_max_boundary_);
        result.push_back(histogram_num_buckets_);

        auto const data = hpx::util::histogram(
            static_cast<detail::histogram_collector&>(*time_between_parcels_)
                .acc_);

        for (auto const& item : data)
        {
            result.push_back(static_cast<std::int64_t>(item.second * 1000));
        }

        return result;
    }

    void coalescing_message_handler::get_time_between_parcels_histogram_creator(
        std::int64_t min_boundary, std::int64_t max_boundary,
        std::int64_t num_buckets,
        hpx::function<std::vector<std::int64_t>(bool)>& result)
    {
        std::scoped_lock<mutex_type> l(mtx_);
        if (time_between_parcels_)
        {
            result = hpx::bind_front(
                &coalescing_message_handler::get_time_between_parcels_histogram,
                this);
            return;
        }

        histogram_min_boundary_ = min_boundary;
        histogram_max_boundary_ = max_boundary;
        histogram_num_buckets_ = num_buckets;

        time_between_parcels_ = std::make_unique<detail::histogram_collector>(
            min_boundary, max_boundary, num_buckets);

        last_parcel_time_ = static_cast<std::int64_t>(
            hpx::chrono::high_resolution_clock::now());

        result = hpx::bind_front(
            &coalescing_message_handler::get_time_between_parcels_histogram,
            this);
    }

    ///////////////////////////////////////////////////////////////////////////
    // register the given action (called during startup)
    void coalescing_message_handler::register_action(
        char const* action, error_code& ec)
    {
        coalescing_counter_registry::instance().register_action(action);
        if (&ec != &throws)
            ec = make_success_code();
    }
}    // namespace hpx::plugins::parcel

#endif
