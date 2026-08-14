//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_PARCELPORT_COUNTERS) &&                                   \
    defined(HPX_HAVE_PARCELPORT_ACTION_COUNTERS) &&                            \
    defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/util.hpp>

#include <hpx/performance_counters/counter_creators.hpp>
#include <hpx/performance_counters/counters.hpp>
#include <hpx/performance_counters/per_action_data_counter_discoverer.hpp>
#include <hpx/performance_counters/registry.hpp>

#include <regex>
#include <string>
#include <utility>

namespace hpx::performance_counters {

    bool per_action_counter_counter_discoverer(
        hpx::actions::detail::per_action_data_counter_registry const& registry,
        performance_counters::counter_info const& info,
        performance_counters::counter_path_elements& p,
        performance_counters::discover_counter_func const& f,
        performance_counters::discover_counters_mode mode, error_code& ec)
    {
        using map_type =
            hpx::actions::detail::per_action_data_counter_registry::map_type;

        map_type const& map = registry.registered_counters();

        if (mode == performance_counters::discover_counters_mode::minimal ||
            p.parentinstancename_.empty() || p.instancename_.empty())
        {
            if (p.parentinstancename_.empty())
            {
                p.parentinstancename_ = "locality#*";
                p.parentinstanceindex_ = -1;
            }

            if (p.instancename_.empty())
            {
                p.instancename_ = "total";
                p.instanceindex_ = -1;
            }
        }

        if (p.parameters_.empty())
        {
            // if no parameters (action name) is given assume that this counter
            // should report the overall value for all actions
            return performance_counters::locality_counter_discoverer(
                info, f, mode, ec);
        }

        if (p.parameters_.find_first_of("*?[]") != std::string::npos)
        {
            std::string str_rx(util::regex_from_pattern(p.parameters_, ec));
            if (ec)
                return false;

            bool found_one = false;
            std::regex rx(str_rx);

            for (auto const& e : map)
            {
                if (!std::regex_match(e, rx))
                    continue;
                found_one = true;

                // propagate parameters
                std::string fullname;
                performance_counters::counter_path_elements cp = p;
                cp.parameters_ = e;

                performance_counters::get_counter_name(cp, fullname, ec);
                if (ec)
                    return false;

                performance_counters::counter_info cinfo = info;
                cinfo.fullname_ = HPX_MOVE(fullname);

                if (!f(cinfo, ec) || ec)
                    return false;
            }

            if (!found_one)
            {
                // compose a list of known action types
                std::string types;
                for (auto const& e : map)
                {
                    types += "  " + e + "\n";
                }

                HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                    "per_action_data_counter_registry::counter_discoverer",
                    "action type {} does not match any known type, "
                    "known action types: \n{}",
                    p.parameters_, types);
                return false;
            }

            if (&ec != &throws)
                ec = make_success_code();

            return true;
        }

        // use given action type directly
        map_type::const_iterator it = map.find(p.parameters_);
        if (it == map.end())
        {
            // compose a list of known action types
            std::string types;
            for (auto const& e : map)
            {
                types += "  " + e + "\n";
            }

            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "per_action_data_counter_registry::counter_discoverer",
                "action type {} does not match any known type, "
                "known action types: \n{}",
                p.parameters_, types);
            return false;
        }

        // propagate parameters
        std::string fullname;
        performance_counters::get_counter_name(p, fullname, ec);
        if (ec)
            return false;

        performance_counters::counter_info cinfo = info;
        cinfo.fullname_ = HPX_MOVE(fullname);

        if (!f(cinfo, ec) || ec)
            return false;

        if (&ec != &throws)
            ec = make_success_code();

        return true;
    }
}    // namespace hpx::performance_counters

#endif
