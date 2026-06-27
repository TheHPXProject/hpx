//  Copyright (c) 2016-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_PARCELPORT_COUNTERS) &&                                   \
    defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS) &&                            \
    defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/functional.hpp>
#include <hpx/modules/hashing.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstdint>
#include <string>
#include <unordered_set>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::actions::detail {

    class HPX_EXPORT per_action_data_counter_registry
    {
    public:
        using counter_function_type = hpx::function<std::int64_t(bool)>;
        using map_type =
            std::unordered_set<std::string, hpx::util::jenkins_hash>;

        enum per_action_counter_type
        {
            num_parcels = 0,
            num_messages,
            total_time,
            total_serialization_time,
            total_bytes,
            total_raw_bytes,
            total_buffer_allocate_time
        };

        per_action_data_counter_registry() = default;

        per_action_data_counter_registry(
            per_action_data_counter_registry const&) = delete;
        per_action_data_counter_registry(
            per_action_data_counter_registry&&) = delete;
        per_action_data_counter_registry& operator=(
            per_action_data_counter_registry const&) = delete;
        per_action_data_counter_registry& operator=(
            per_action_data_counter_registry&&) = delete;

        ~per_action_data_counter_registry() = default;

        static per_action_data_counter_registry& instance();

        void register_class(std::string action);

        counter_function_type get_counter(std::string const& action,
            hpx::function<std::int64_t(std::string const&, bool)> const& f)
            const;

        map_type const& registered_counters() const
        {
            return map_;
        }

    private:
        struct tag;
        friend struct hpx::util::static_<per_action_data_counter_registry, tag>;

        map_type map_;
    };

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action>
    void register_per_action_data_counter_types(
        per_action_data_counter_registry& registry);

    template <typename Action>
    struct register_per_action_data_counters
    {
        register_per_action_data_counters()
        {
            register_per_action_data_counter_types<Action>(
                per_action_data_counter_registry::instance());
        }

        static register_per_action_data_counters instance;
    };

    template <typename Action>
    register_per_action_data_counters<Action>
        register_per_action_data_counters<Action>::instance;
}    // namespace hpx::actions::detail

#include <hpx/config/warnings_suffix.hpp>

#endif
