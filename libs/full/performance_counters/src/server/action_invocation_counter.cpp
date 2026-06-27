//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>

#include <hpx/performance_counters/action_invocation_counter_discoverer.hpp>
#include <hpx/performance_counters/counter_creators.hpp>
#include <hpx/performance_counters/counters.hpp>

#include <cstdint>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::performance_counters {

    ///////////////////////////////////////////////////////////////////////////
    // Discoverer function for action invocation counters
    bool action_invocation_counter_discoverer(counter_info const& info,
        discover_counter_func const& f, discover_counters_mode mode,
        hpx::actions::detail::invocation_count_registry& registry,
        error_code& ec)
    {
        // compose the counter name templates
        performance_counters::counter_path_elements p;
        performance_counters::counter_status status =
            get_counter_path_elements(info.fullname_, p, ec);
        if (!status_is_valid(status))
            return false;

        bool result = action_invocation_counter_discoverer(
            registry, info, p, f, mode, ec);
        if (!result || ec)
            return false;

        if (&ec != &throws)
            ec = make_success_code();

        return true;
    }

    bool local_action_invocation_counter_discoverer(counter_info const& info,
        discover_counter_func const& f, discover_counters_mode mode,
        error_code& ec)
    {
        using hpx::actions::detail::invocation_count_registry;
        return action_invocation_counter_discoverer(
            info, f, mode, invocation_count_registry::local_instance(), ec);
    }

#if defined(HPX_HAVE_NETWORKING)
    bool remote_action_invocation_counter_discoverer(counter_info const& info,
        discover_counter_func const& f, discover_counters_mode mode,
        error_code& ec)
    {
        using hpx::actions::detail::invocation_count_registry;
        return action_invocation_counter_discoverer(
            info, f, mode, invocation_count_registry::remote_instance(), ec);
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    // Creation function for action invocation counter
    naming::gid_type action_invocation_counter_creator(counter_info const& info,
        hpx::actions::detail::invocation_count_registry& registry,
        error_code& ec)
    {
        if (info.type_ != counter_type::monotonically_increasing)
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "action_invocation_counter_creator",
                "invalid counter type requested");
            return naming::invalid_gid;
        }

        counter_path_elements paths;
        get_counter_path_elements(info.fullname_, paths, ec);
        if (ec)
            return naming::invalid_gid;

        if (paths.parentinstance_is_basename_)
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "action_invocation_counter_creator",
                "invalid action invocation counter name (instance name "
                "must not be a valid base counter name)");
            return naming::invalid_gid;
        }

        if (paths.parameters_.empty())
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "action_invocation_counter_creator",
                "invalid action invocation counter parameter: must "
                "specify an action type");
            return naming::invalid_gid;
        }

        // ask registry
        hpx::function<std::int64_t(bool)> f =
            registry.get_invocation_counter(paths.parameters_);

        return detail::create_raw_counter(info, HPX_MOVE(f), ec);
    }

    naming::gid_type local_action_invocation_counter_creator(
        counter_info const& info, error_code& ec)
    {
        using hpx::actions::detail::invocation_count_registry;
        return action_invocation_counter_creator(
            info, invocation_count_registry::local_instance(), ec);
    }

#if defined(HPX_HAVE_NETWORKING)
    naming::gid_type remote_action_invocation_counter_creator(
        counter_info const& info, error_code& ec)
    {
        using hpx::actions::detail::invocation_count_registry;
        return action_invocation_counter_creator(
            info, invocation_count_registry::remote_instance(), ec);
    }
#endif
}    // namespace hpx::performance_counters
