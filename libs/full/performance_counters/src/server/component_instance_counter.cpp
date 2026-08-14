//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>

#include <hpx/performance_counters/counter_creators.hpp>
#include <hpx/performance_counters/counters.hpp>

#include <cstdint>
#include <sstream>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::performance_counters::detail {

    ///////////////////////////////////////////////////////////////////////////
    // Extract the current number of instances for the given component type
    static std::int64_t get_instance_count(components::component_type type)
    {
        return hpx::components::instance_count(type);
    }

    ///////////////////////////////////////////////////////////////////////////
    /// Creation function for instance counter
    naming::gid_type component_instance_counter_creator(
        counter_info const& info, error_code& ec)
    {
        if (info.type_ != counter_type::raw)
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "component_instance_counter_creator",
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
                "component_instance_counter_creator",
                "invalid instance counter name (instance name must not "
                "be a valid base counter name)");
            return naming::invalid_gid;
        }

        if (paths.parameters_.empty())
        {
            std::stringstream strm;
            strm << "invalid instance counter parameter: must specify "
                    "a component type\n"
                    "known component types:\n";

            components::enumerate_instance_counts(
                [&strm](components::component_type type) -> bool {
                    strm << "  " << agas::get_component_type_name(type) << "\n";
                    return true;
                });

            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "component_instance_counter_creator", strm.str());

            return naming::invalid_gid;
        }

        // ask AGAS to resolve the component type
        components::component_type type =
            agas::get_component_id(paths.parameters_);

        if (type == to_int(hpx::components::component_enum_type::invalid))
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "component_instance_counter_creator",
                "invalid component type as counter parameter: {}",
                paths.parameters_);
            return naming::invalid_gid;
        }

        hpx::function<std::int64_t()> f =
            hpx::bind_front(&get_instance_count, type);
        return create_raw_counter(info, HPX_MOVE(f), ec);
    }
}    // namespace hpx::performance_counters::detail
