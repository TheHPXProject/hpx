//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/components_base.hpp>

#include <hpx/performance_counters/performance_counter_set.hpp>
#include <hpx/performance_counters/server/base_performance_counter.hpp>

#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::performance_counters::server {

    ///////////////////////////////////////////////////////////////////////////
    // This counter exposes the result of an arithmetic operation The counter
    // relies on querying two base counters.
    template <typename Statistic>
    class arithmetics_counter_extended
      : public base_performance_counter
      , public components::component_base<
            arithmetics_counter_extended<Statistic>>
    {
        using base_type =
            components::component_base<arithmetics_counter_extended<Statistic>>;

    public:
        using type_holder = arithmetics_counter_extended;
        using base_type_holder = base_performance_counter;

        arithmetics_counter_extended();

        arithmetics_counter_extended(counter_info const& info,
            std::vector<std::string> const& base_counter_names);

        /// Overloads from the base_counter base class.
        hpx::performance_counters::counter_value get_counter_value(
            bool reset = false) override;

        bool start() override;
        bool stop() override;
        void reset_counter_value() override;

        void finalize();

        naming::address get_current_address() const;

    private:
        // base counters to be queried
        performance_counter_set counters_;
    };
}    // namespace hpx::performance_counters::server

#include <hpx/config/warnings_suffix.hpp>
