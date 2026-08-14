//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/components_base.hpp>

#include <hpx/performance_counters/server/base_performance_counter.hpp>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::performance_counters::server {

    class elapsed_time_counter
      : public base_performance_counter
      , public components::component_base<elapsed_time_counter>
    {
        using base_type = components::component_base<elapsed_time_counter>;

    public:
        using type_holder = elapsed_time_counter;
        using base_type_holder = base_performance_counter;

        elapsed_time_counter();
        explicit elapsed_time_counter(counter_info const& info);

        hpx::performance_counters::counter_value get_counter_value(
            bool reset) override;
        void reset_counter_value() override;

        bool start() override;
        bool stop() override;

        // finalize() will be called just before the instance gets destructed
        static void finalize();

        naming::address get_current_address() const;
    };
}    // namespace hpx::performance_counters::server

#include <hpx/config/warnings_suffix.hpp>
