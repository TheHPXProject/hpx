//  Copyright (c) 2016-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_PARCELPORT_COUNTERS) &&                                   \
    defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS) &&                            \
    defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>

#include <hpx/performance_counters/counter_creators.hpp>
#include <hpx/performance_counters/counters.hpp>
#include <hpx/performance_counters/per_action_data_counter_discoverer.hpp>

#include <cstdint>
#include <string>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::performance_counters {

    ///////////////////////////////////////////////////////////////////////////
    // Discoverer function for per-action parcel data counters
    bool per_action_data_counter_discoverer(counter_info const& info,
        discover_counter_func const& f, discover_counters_mode mode,
        hpx::actions::detail::per_action_data_counter_registry& registry,
        error_code& ec)
    {
        // compose the counter name templates
        performance_counters::counter_path_elements p;
        performance_counters::counter_status status =
            get_counter_path_elements(info.fullname_, p, ec);
        if (!status_is_valid(status))
            return false;

        bool result = per_action_counter_counter_discoverer(
            registry, info, p, f, mode, ec);
        if (!result || ec)
            return false;

        if (&ec != &throws)
            ec = make_success_code();

        return true;
    }

    bool per_action_data_counter_discoverer(counter_info const& info,
        discover_counter_func const& f, discover_counters_mode mode,
        error_code& ec)
    {
        using hpx::actions::detail::per_action_data_counter_registry;
        return per_action_data_counter_discoverer(
            info, f, mode, per_action_data_counter_registry::instance(), ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Creation function for per-action parcel data counters
    naming::gid_type per_action_data_counter_creator(counter_info const& info,
        hpx::actions::detail::per_action_data_counter_registry& registry,
        hpx::function<std::int64_t(std::string const&, bool)> const&
            counter_func,
        error_code& ec)
    {
        switch (info.type_)
        {
        case counter_type::elapsed_time:
            [[fallthrough]];
        case counter_type::monotonically_increasing:
        {
            counter_path_elements paths;
            get_counter_path_elements(info.fullname_, paths, ec);
            if (ec)
                return naming::invalid_gid;

            if (paths.parentinstance_is_basename_)
            {
                HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                    "per_action_data_counter_creator",
                    "invalid action invocation counter name (instance name "
                    "must not be a valid base counter name)");
                return naming::invalid_gid;
            }

            if (paths.parameters_.empty())
            {
                // if no parameters (action name) is given assume that this
                // counter should report the overall value for all actions
                auto const& f =
                    hpx::bind_front(counter_func, paths.parameters_);
                return performance_counters::locality_raw_counter_creator(
                    info, f, ec);
            }

            // ask registry
            hpx::function<std::int64_t(bool)> f =
                registry.get_counter(paths.parameters_, counter_func);

            return detail::create_raw_counter(info, HPX_MOVE(f), ec);
        }
        break;

        default:
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "per_action_data_counter_creator",
                "invalid counter type requested");
            return naming::invalid_gid;
        }
    }

    naming::gid_type per_action_data_counter_creator(counter_info const& info,
        hpx::function<std::int64_t(std::string const&, bool)> const& f,
        error_code& ec)
    {
        using hpx::actions::detail::per_action_data_counter_registry;
        return per_action_data_counter_creator(
            info, per_action_data_counter_registry::instance(), f, ec);
    }
}    // namespace hpx::performance_counters

#endif
