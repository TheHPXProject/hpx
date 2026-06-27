//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/thread_support.hpp>

#include <hpx/performance_counters/counters.hpp>
#include <hpx/performance_counters/performance_counter_base.hpp>
#include <hpx/performance_counters/server/base_performance_counter.hpp>

HPX_DEFINE_GET_COMPONENT_TYPE(hpx::components::component<
    hpx::performance_counters::server::base_performance_counter>)

///////////////////////////////////////////////////////////////////////////////
namespace hpx::performance_counters::server {

    void base_performance_counter::reset_counter_value()
    {
        HPX_THROW_EXCEPTION(hpx::error::invalid_status, "reset_counter_value",
            "reset_counter_value is not implemented for this counter");
    }

    void base_performance_counter::set_counter_value(
        counter_value const& /*value*/)
    {
        HPX_THROW_EXCEPTION(hpx::error::invalid_status, "set_counter_value",
            "set_counter_value is not implemented for this counter");
    }

    counter_value base_performance_counter::get_counter_value(bool /*reset*/)
    {
        HPX_THROW_EXCEPTION(hpx::error::invalid_status, "get_counter_value",
            "get_counter_value is not implemented for this counter");
        return {};
    }

    counter_values_array base_performance_counter::get_counter_values_array(
        bool /*reset*/)
    {
        HPX_THROW_EXCEPTION(hpx::error::invalid_status,
            "get_counter_values_array",
            "get_counter_values_array is not implemented for this "
            "counter");
        return {};
    }

    bool base_performance_counter::start()
    {
        return false;    // nothing to do
    }

    bool base_performance_counter::stop()
    {
        return false;    // nothing to do
    }

    void base_performance_counter::reinit(bool /*reset*/) {}

    counter_info base_performance_counter::get_counter_info() const
    {
        return info_;
    }

    base_performance_counter::base_performance_counter()
      : invocation_count_(0)
    {
    }

    base_performance_counter::base_performance_counter(counter_info const& info)
      : info_(info)
      , invocation_count_(0)
    {
    }

    ///////////////////////////////////////////////////////////////////////
    counter_info base_performance_counter::get_counter_info_nonvirt() const
    {
        return this->get_counter_info();
    }

    counter_value base_performance_counter::get_counter_value_nonvirt(
        bool reset)
    {
        return this->get_counter_value(reset);
    }

    counter_values_array
    base_performance_counter::get_counter_values_array_nonvirt(bool reset)
    {
        return this->get_counter_values_array(reset);
    }

    void base_performance_counter::set_counter_value_nonvirt(
        counter_value const& info)
    {
        this->set_counter_value(info);
    }

    void base_performance_counter::reset_counter_value_nonvirt()
    {
        this->reset_counter_value();
    }

    bool base_performance_counter::start_nonvirt()
    {
        return this->start();
    }

    bool base_performance_counter::stop_nonvirt()
    {
        return this->stop();
    }

    void base_performance_counter::reinit_nonvirt(bool reset)
    {
        reinit(reset);
    }
}    // namespace hpx::performance_counters::server
