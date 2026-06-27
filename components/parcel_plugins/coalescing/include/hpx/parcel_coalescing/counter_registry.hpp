//  Copyright (c) 2016-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCEL_COALESCING)
#include <hpx/modules/functional.hpp>
#include <hpx/modules/hashing.hpp>
#include <hpx/modules/performance_counters.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::plugins::parcel {

    ///////////////////////////////////////////////////////////////////////////
    class coalescing_counter_registry
    {
        using mutex_type = hpx::spinlock;

    public:
        HPX_NON_COPYABLE(coalescing_counter_registry);

    public:
        coalescing_counter_registry() {}

        using get_counter_type = hpx::function<std::int64_t(bool)>;
        using get_counter_values_type =
            hpx::function<std::vector<std::int64_t>(bool)>;
        using get_counter_values_creator_type = hpx::function<void(std::int64_t,
            std::int64_t, std::int64_t, get_counter_values_type&)>;

        struct counter_functions
        {
            get_counter_type num_parcels;
            get_counter_type num_messages;
            get_counter_type num_parcels_per_message;
            get_counter_type average_time_between_parcels;
            get_counter_values_creator_type
                time_between_parcels_histogram_creator;
            std::int64_t min_boundary = 0, max_boundary = 0, num_buckets = 0;
        };

        using map_type = std::unordered_map<std::string, counter_functions,
            hpx::util::jenkins_hash>;

        static coalescing_counter_registry& instance();

        void register_action(std::string const& name);

        void register_action(std::string const& name,
            get_counter_type const& num_parcels,
            get_counter_type const& num_messages,
            get_counter_type const& time_between_parcels,
            get_counter_type const& average_time_between_parcels,
            get_counter_values_creator_type const&
                time_between_parcels_histogram_creator);

        get_counter_type get_parcels_counter(std::string const& name) const;
        get_counter_type get_messages_counter(std::string const& name) const;
        get_counter_type get_parcels_per_message_counter(
            std::string const& name) const;
        get_counter_type get_average_time_between_parcels_counter(
            std::string const& name) const;
        get_counter_values_type get_time_between_parcels_histogram_counter(
            std::string const& name, std::int64_t min_boundary,
            std::int64_t max_boundary, std::int64_t num_buckets);

        bool counter_discoverer(performance_counters::counter_info const& info,
            performance_counters::counter_path_elements& p,
            performance_counters::discover_counter_func const& f,
            performance_counters::discover_counters_mode mode, error_code& ec);

        static std::vector<std::int64_t> empty_histogram(bool)
        {
            std::vector<std::int64_t> result = {0, 0, 1, 0};
            return result;
        }

    private:
        struct tag
        {
        };

        friend struct hpx::util::static_<coalescing_counter_registry, tag>;

        mutable mutex_type mtx_;
        map_type map_;
    };
}    // namespace hpx::plugins::parcel

#endif
