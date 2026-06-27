//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/runtime_components.hpp>
#include <hpx/modules/runtime_local.hpp>

#include <hpx/performance_counters/counters.hpp>
#include <hpx/performance_counters/server/raw_counter.hpp>

#include <cstdint>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
using raw_counter_type =
    hpx::components::component<hpx::performance_counters::server::raw_counter>;

HPX_REGISTER_DERIVED_COMPONENT_FACTORY(raw_counter_type, raw_counter,
    "base_performance_counter", hpx::components::factory_state::enabled)
HPX_DEFINE_GET_COMPONENT_TYPE(hpx::performance_counters::server::raw_counter)

///////////////////////////////////////////////////////////////////////////////
namespace hpx::performance_counters::server {

    raw_counter::raw_counter()
      : reset_(false)
    {
    }

    raw_counter::raw_counter(
        counter_info const& info, hpx::function<std::int64_t(bool)> f)
      : base_type_holder(info)
      , f_(HPX_MOVE(f))
      , reset_(false)
    {
        if (info.type_ != counter_type::raw &&
            info.type_ != counter_type::elapsed_time &&
            info.type_ != counter_type::aggregating &&
            info.type_ != counter_type::monotonically_increasing &&
            info.type_ != counter_type::average_count &&
            info.type_ != counter_type::average_timer)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "raw_counter::raw_counter",
                "unexpected counter type specified for raw_counter");
        }
    }

    hpx::performance_counters::counter_value raw_counter::get_counter_value(
        bool reset)
    {
        hpx::performance_counters::counter_value value;
        value.value_ = f_(reset);    // gather the current value
        reset_ = false;
        value.scaling_ = 1;
        value.scale_inverse_ = false;
        value.status_ = counter_status::new_data;
        value.time_ = static_cast<std::int64_t>(hpx::get_system_uptime());
        value.count_ = ++invocation_count_;
        return value;
    }

    void raw_counter::reset_counter_value()
    {
        [[maybe_unused]] auto _ = f_(true);
    }

    void raw_counter::finalize()
    {
        base_performance_counter::finalize();
        base_type::finalize();
    }

    naming::address raw_counter::get_current_address() const
    {
        return naming::address(
            naming::get_gid_from_locality_id(agas::get_locality_id()),
            components::get_component_type<raw_counter>(),
            const_cast<raw_counter*>(this));
    }
}    // namespace hpx::performance_counters::server
