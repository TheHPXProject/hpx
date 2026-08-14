//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/thread_support.hpp>

#include <hpx/performance_counters/counters.hpp>
#include <hpx/performance_counters/performance_counter_base.hpp>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::performance_counters::server {

    HPX_CXX_EXPORT class HPX_EXPORT base_performance_counter
      : public hpx::performance_counters::performance_counter_base
      , public hpx::traits::detail::component_tag
    {
    protected:
        // the following functions are not implemented by default, they will
        // just throw
        void reset_counter_value() override;
        void set_counter_value(counter_value const& /*value*/) override;
        counter_value get_counter_value(bool /*reset*/) override;
        counter_values_array get_counter_values_array(bool /*reset*/) override;
        bool start() override;
        bool stop() override;
        void reinit(bool /*reset*/) override;
        counter_info get_counter_info() const override;

    public:
        base_performance_counter();
        explicit base_performance_counter(counter_info const& info);

        // components must contain a typedef for wrapping_type defining the
        // component type used to encapsulate instances of this
        // component
        using wrapping_type = components::component<base_performance_counter>;
        using base_type_holder = base_performance_counter;

        // finalize() will be called just before the instance gets destructed
        static constexpr void finalize() noexcept {}

        static components::component_type get_component_type() noexcept
        {
            return components::get_component_type<wrapping_type>();
        }
        static void set_component_type(components::component_type t)
        {
            components::set_component_type<wrapping_type>(t);
        }

        ///////////////////////////////////////////////////////////////////////
        counter_info get_counter_info_nonvirt() const;
        counter_value get_counter_value_nonvirt(bool reset);
        counter_values_array get_counter_values_array_nonvirt(bool reset);
        void set_counter_value_nonvirt(counter_value const& info);
        void reset_counter_value_nonvirt();
        bool start_nonvirt();
        bool stop_nonvirt();
        void reinit_nonvirt(bool reset);

        // Each of the exposed functions needs to be encapsulated into an action
        // type, allowing to generate all required boilerplate code for threads,
        // serialization, etc.

        /// The \a get_counter_info_action retrieves a performance counters
        /// information.
        HPX_DEFINE_COMPONENT_ACTION(base_performance_counter,
            get_counter_info_nonvirt, get_counter_info_action)

        /// The \a get_counter_value_action queries the value of a performance
        /// counter.
        HPX_DEFINE_COMPONENT_ACTION(base_performance_counter,
            get_counter_value_nonvirt, get_counter_value_action)

        /// The \a get_counter_value_action queries the value of a performance
        /// counter.
        HPX_DEFINE_COMPONENT_ACTION(base_performance_counter,
            get_counter_values_array_nonvirt, get_counter_values_array_action)

        /// The \a set_counter_value_action
        HPX_DEFINE_COMPONENT_ACTION(base_performance_counter,
            set_counter_value_nonvirt, set_counter_value_action)

        /// The \a reset_counter_value_action
        HPX_DEFINE_COMPONENT_ACTION(base_performance_counter,
            reset_counter_value_nonvirt, reset_counter_value_action)

        /// The \a start_action
        HPX_DEFINE_COMPONENT_ACTION(
            base_performance_counter, start_nonvirt, start_action)

        /// The \a stop_action
        HPX_DEFINE_COMPONENT_ACTION(
            base_performance_counter, stop_nonvirt, stop_action)

        /// The \a reinit_action
        HPX_DEFINE_COMPONENT_ACTION(
            base_performance_counter, reinit_nonvirt, reinit_action)

    protected:
        hpx::performance_counters::counter_info info_;
        util::atomic_count invocation_count_;
    };
}    // namespace hpx::performance_counters::server

#include <hpx/config/warnings_suffix.hpp>

HPX_ACTION_HAS_CRITICAL_PRIORITY(hpx::performance_counters::server::
        base_performance_counter ::get_counter_info_action)
HPX_REGISTER_ACTION_DECLARATION(
    hpx::performance_counters::server::base_performance_counter ::
        get_counter_info_action,
    performance_counter_get_counter_info_action)

HPX_ACTION_HAS_CRITICAL_PRIORITY(hpx::performance_counters::server::
        base_performance_counter ::get_counter_value_action)
HPX_REGISTER_ACTION_DECLARATION(
    hpx::performance_counters::server::base_performance_counter ::
        get_counter_value_action,
    performance_counter_get_counter_value_action)

HPX_ACTION_HAS_CRITICAL_PRIORITY(hpx::performance_counters::server::
        base_performance_counter ::get_counter_values_array_action)
HPX_REGISTER_ACTION_DECLARATION(
    hpx::performance_counters::server::base_performance_counter ::
        get_counter_values_array_action,
    performance_counter_get_counter_values_array_action)

HPX_ACTION_HAS_CRITICAL_PRIORITY(hpx::performance_counters::server::
        base_performance_counter ::set_counter_value_action)
HPX_REGISTER_ACTION_DECLARATION(
    hpx::performance_counters::server::base_performance_counter ::
        set_counter_value_action,
    performance_counter_set_counter_value_action)

HPX_ACTION_HAS_CRITICAL_PRIORITY(hpx::performance_counters::server::
        base_performance_counter ::reset_counter_value_action)
HPX_REGISTER_ACTION_DECLARATION(
    hpx::performance_counters::server::base_performance_counter ::
        reset_counter_value_action,
    performance_counter_reset_counter_value_action)

HPX_ACTION_HAS_CRITICAL_PRIORITY(
    hpx::performance_counters::server::base_performance_counter::start_action)
HPX_REGISTER_ACTION_DECLARATION(
    hpx::performance_counters::server::base_performance_counter::start_action,
    performance_counter_start_action)

HPX_ACTION_HAS_CRITICAL_PRIORITY(
    hpx::performance_counters::server::base_performance_counter::stop_action)
HPX_REGISTER_ACTION_DECLARATION(
    hpx::performance_counters::server::base_performance_counter::stop_action,
    performance_counter_stop_action)

HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(
    hpx::performance_counters::counter_info, hpx_counter_info)
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(
    hpx::performance_counters::counter_value, hpx_counter_value)
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(
    hpx::performance_counters::counter_values_array, hpx_counter_values_array)
