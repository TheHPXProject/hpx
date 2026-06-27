//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/errors.hpp>

#include <hpx/performance_counters/counters.hpp>
#include <hpx/performance_counters/performance_counter.hpp>

#include <string>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::performance_counters {

    ///////////////////////////////////////////////////////////////////////////
    performance_counter::performance_counter(std::string const& name)
      : base_type(performance_counters::get_counter_async(name))
    {
    }

    performance_counter::performance_counter(
        std::string const& name, hpx::id_type const& locality)
    {
        HPX_ASSERT(naming::is_locality(locality));

        counter_path_elements p;
        get_counter_path_elements(name, p);

        std::string full_name;
        p.parentinstanceindex_ = naming::get_locality_id_from_id(locality);
        get_counter_name(p, full_name);

        this->base_type::reset(
            performance_counters::get_counter_async(full_name));
    }

    ///////////////////////////////////////////////////////////////////////////
    hpx::future<counter_info> performance_counter::get_info() const
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        using action_type =
            server::base_performance_counter::get_counter_info_action;
        return hpx::async<action_type>(get_id());
#else
        HPX_ASSERT(false);
        return hpx::make_ready_future(counter_info{});
#endif
    }
    counter_info performance_counter::get_info(
        launch::sync_policy, [[maybe_unused]] error_code& ec) const
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        hpx::id_type id = get_id(ec);
        if (ec)
            return counter_info{};
        using action_type =
            server::base_performance_counter::get_counter_info_action;
        return hpx::async(action_type(), HPX_MOVE(id)).get(ec);
#else
        HPX_ASSERT(false);
        return counter_info{};
#endif
    }

    hpx::future<counter_value> performance_counter::get_counter_value(
        [[maybe_unused]] bool reset) const
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        using action_type =
            server::base_performance_counter::get_counter_value_action;
        return hpx::async<action_type>(get_id(), reset);
#else
        HPX_ASSERT(false);
        return hpx::make_ready_future(counter_value{});
#endif
    }
    counter_value performance_counter::get_counter_value(launch::sync_policy,
        [[maybe_unused]] bool reset, [[maybe_unused]] error_code& ec) const
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        hpx::id_type id = get_id(ec);
        if (ec)
            return counter_value{};
        using action_type =
            server::base_performance_counter::get_counter_value_action;
        return hpx::async(action_type(), HPX_MOVE(id), reset).get(ec);
#else
        HPX_ASSERT(false);
        return counter_value{};
#endif
    }

    hpx::future<counter_value> performance_counter::get_counter_value() const
    {
        return get_counter_value(false);
    }
    counter_value performance_counter::get_counter_value(
        launch::sync_policy l, error_code& ec) const
    {
        return get_counter_value(l, false, ec);
    }

    hpx::future<counter_values_array>
    performance_counter::get_counter_values_array(
        [[maybe_unused]] bool reset) const
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        using action_type =
            server::base_performance_counter::get_counter_values_array_action;
        return hpx::async(action_type(), get_id(), reset);
#else
        HPX_ASSERT(false);
        return hpx::make_ready_future(counter_values_array{});
#endif
    }
    counter_values_array performance_counter::get_counter_values_array(
        launch::sync_policy, bool reset, error_code& ec) const
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        hpx::id_type id = get_id(ec);
        if (ec)
            return counter_values_array{};
        using action_type =
            server::base_performance_counter::get_counter_values_array_action;
        return hpx::async(action_type(), HPX_MOVE(id), reset).get(ec);
#else
        HPX_ASSERT(false);
        return counter_values_array{};
#endif
    }

    hpx::future<counter_values_array>
    performance_counter::get_counter_values_array() const
    {
        return get_counter_values_array(false);
    }
    counter_values_array performance_counter::get_counter_values_array(
        launch::sync_policy l, error_code& ec) const
    {
        return get_counter_values_array(l, false, ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    hpx::future<bool> performance_counter::start() const
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        using action_type = server::base_performance_counter::start_action;
        return hpx::async<action_type>(get_id());
#else
        HPX_ASSERT(false);
        return hpx::make_ready_future(true);
#endif
    }
    bool performance_counter::start(
        launch::sync_policy, [[maybe_unused]] error_code& ec) const
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        hpx::id_type id = get_id(ec);
        if (ec)
            return false;
        using action_type = server::base_performance_counter::start_action;
        return hpx::async(action_type(), HPX_MOVE(id)).get(ec);
#else
        HPX_ASSERT(false);
        return false;
#endif
    }

    hpx::future<bool> performance_counter::stop() const
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        using action_type = server::base_performance_counter::stop_action;
        return hpx::async<action_type>(get_id());
#else
        HPX_ASSERT(false);
        return hpx::make_ready_future(true);
#endif
    }

    bool performance_counter::stop(
        launch::sync_policy, [[maybe_unused]] error_code& ec) const
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        hpx::id_type id = get_id(ec);
        if (ec)
            return false;
        using action_type = server::base_performance_counter::stop_action;
        return hpx::async(action_type(), HPX_MOVE(id)).get(ec);
#else
        HPX_ASSERT(false);
        return false;
#endif
    }

    hpx::future<void> performance_counter::reset() const
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        using action_type =
            server::base_performance_counter::reset_counter_value_action;
        return hpx::async<action_type>(get_id());
#else
        HPX_ASSERT(false);
        return hpx::make_ready_future();
#endif
    }

    void performance_counter::reset(
        launch::sync_policy, [[maybe_unused]] error_code& ec) const
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        hpx::id_type id = get_id(ec);
        if (ec)
            return;
        using action_type =
            server::base_performance_counter::reset_counter_value_action;
        hpx::async(action_type(), HPX_MOVE(id)).get(ec);
#else
        HPX_ASSERT(false);
#endif
    }

    hpx::future<void> performance_counter::reinit(
        [[maybe_unused]] bool reset) const
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        using action_type = server::base_performance_counter::reinit_action;
        return hpx::async<action_type>(get_id(), reset);
#else
        HPX_ASSERT(false);
        return hpx::make_ready_future();
#endif
    }

    void performance_counter::reinit(
        launch::sync_policy, bool reset, error_code& ec) const
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        hpx::id_type id = get_id(ec);
        if (ec)
            return;
        using action_type = server::base_performance_counter::reinit_action;
        hpx::async(action_type(), HPX_MOVE(id), reset).get(ec);
#else
        HPX_ASSERT(false);
#endif
    }

    ///////////////////////////////////////////////////////////////////////////
    hpx::future<std::string> performance_counter::get_name() const
    {
        return hpx::make_future<std::string>(get_info(),
            [](counter_info&& info) -> std::string { return info.fullname_; });
    }

    std::string performance_counter::get_name(
        launch::sync_policy, error_code& ec) const
    {
        return get_name().get(ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    /// Return all counters matching the given name (with optional wild-cards).
    std::vector<performance_counter> discover_counters(
        std::string const& name, error_code& ec)
    {
        std::vector<performance_counter> counters;

        std::vector<counter_info> infos;
        counter_status const status = discover_counter_type(
            name, infos, discover_counters_mode::full, ec);
        if (!status_is_valid(status) || ec)
            return counters;

        counters.reserve(infos.size());
        for (counter_info const& info : infos)
        {
            try
            {
                performance_counter counter(info.fullname_);
                counters.push_back(counter);
            }
            catch (hpx::exception const& e)
            {
                HPX_RETHROWS_IF(ec, e, "discover_counters");
            }
        }

        return counters;
    }
}    // namespace hpx::performance_counters
