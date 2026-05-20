//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//  Copyright (c) 2012-2021 Hartmut Kaiser
//  Copyright (c) 2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/format.hpp>
#include <hpx/modules/agas.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/naming.hpp>

#include <hpx/performance_counters/agas_namespace_action_code.hpp>
#include <hpx/performance_counters/component_namespace_counters.hpp>
#include <hpx/performance_counters/counter_creators.hpp>
#include <hpx/performance_counters/counters.hpp>
#include <hpx/performance_counters/manage_counter_type.hpp>
#include <hpx/performance_counters/server/component_namespace_counters.hpp>

#include <cstddef>
#include <cstdint>
#include <string>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::agas::server {

    // register all performance counter types exposed by this component
    void component_namespace_register_counter_types(error_code& ec)
    {
        performance_counters::create_counter_func const creator(
            hpx::bind_back(&performance_counters::agas_raw_counter_creator,
                agas::server::component_namespace_service_name));

        for (auto const& component_namespace_service :
            detail::component_namespace_services)
        {
            // global counters are handled elsewhere
            if (component_namespace_service.code_ ==
                component_ns_statistics_counter)
            {
                continue;
            }

            std::string name(component_namespace_service.name_);
            std::string help;
            performance_counters::counter_type type;
            std::string::size_type const p = name.find_last_of('/');
            HPX_ASSERT(p != std::string::npos);

            if (component_namespace_service.target_ ==
                detail::counter_target_count)
            {
                help = hpx::util::format("returns the number of invocations "
                                         "of the AGAS service '{}'",
                    name.substr(p + 1));
                type = performance_counters::counter_type::
                    monotonically_increasing;
            }
            else
            {
                help = hpx::util::format("returns the overall execution "
                                         "time of the AGAS service '{}'",
                    name.substr(p + 1));
                type = performance_counters::counter_type::elapsed_time;
            }

            performance_counters::install_counter_type(
                agas::performance_counter_basename + name, type, help, creator,
                &performance_counters::locality0_counter_discoverer,
                HPX_PERFORMANCE_COUNTER_V1, component_namespace_service.uom_,
                ec);
            if (ec)
            {
                return;
            }
        }
    }

    void component_namespace_register_global_counter_types(error_code& ec)
    {
        performance_counters::create_counter_func const creator(
            hpx::bind_back(&performance_counters::agas_raw_counter_creator,
                agas::server::component_namespace_service_name));

        for (auto const& component_namespace_service :
            detail::component_namespace_services)
        {
            // local counters are handled elsewhere
            if (component_namespace_service.code_ !=
                component_ns_statistics_counter)
            {
                continue;
            }

            std::string help;
            performance_counters::counter_type type;
            if (component_namespace_service.target_ ==
                detail::counter_target_count)
            {
                help = "returns the overall number of invocations of all "
                       "component AGAS services";
                type = performance_counters::counter_type::
                    monotonically_increasing;
            }
            else
            {
                help = "returns the overall execution time of all "
                       "component AGAS services";
                type = performance_counters::counter_type::elapsed_time;
            }

            performance_counters::install_counter_type(
                std::string(agas::performance_counter_basename) +
                    component_namespace_service.name_,
                type, help, creator,
                &performance_counters::locality0_counter_discoverer,
                HPX_PERFORMANCE_COUNTER_V1, component_namespace_service.uom_,
                ec);
            if (ec)
            {
                return;
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    naming::gid_type component_namespace_statistics_counter(
        component_namespace& service, std::string const& name)
    {    // statistics_counter implementation
        LAGAS_(info).format("component_namespace::statistics_counter");

        hpx::error_code ec;

        performance_counters::counter_path_elements p;
        performance_counters::get_counter_path_elements(name, p, ec);
        if (ec)
        {
            return naming::invalid_gid;
        }

        if (p.objectname_ != "agas")
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "component_namespace::statistics_counter",
                "unknown performance counter (unrelated to AGAS)");
        }

        namespace_action_code code = invalid_request;
        detail::counter_target target = detail::counter_target_invalid;
        for (auto const& component_namespace_service :
            detail::component_namespace_services)
        {
            if (p.countername_ == component_namespace_service.name_)
            {
                code = component_namespace_service.code_;
                target = component_namespace_service.target_;
                break;
            }
        }

        if (code == invalid_request || target == detail::counter_target_invalid)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "component_namespace::statistics_counter",
                "unknown performance counter (unrelated to AGAS)");
        }

        using cd = component_namespace::counter_data;

        hpx::function<std::int64_t(bool)> get_data_func;
        if (target == detail::counter_target_count)
        {
            switch (code)
            {
            case component_ns_bind_prefix:
                get_data_func = hpx::bind_front(
                    &cd::get_bind_prefix_count, &service.counter_data_);
                service.counter_data_.bind_prefix_.enabled_ = true;
                break;
            case component_ns_bind_name:
                get_data_func = hpx::bind_front(
                    &cd::get_bind_name_count, &service.counter_data_);
                service.counter_data_.bind_name_.enabled_ = true;
                break;
            case component_ns_resolve_id:
                get_data_func = hpx::bind_front(
                    &cd::get_resolve_id_count, &service.counter_data_);
                service.counter_data_.resolve_id_.enabled_ = true;
                break;
            case component_ns_unbind_name:
                get_data_func = hpx::bind_front(
                    &cd::get_unbind_name_count, &service.counter_data_);
                service.counter_data_.unbind_name_.enabled_ = true;
                break;
            case component_ns_iterate_types:
                get_data_func = hpx::bind_front(
                    &cd::get_iterate_types_count, &service.counter_data_);
                service.counter_data_.iterate_types_.enabled_ = true;
                break;
            case component_ns_get_component_type_name:
                get_data_func = hpx::bind_front(
                    &cd::get_component_type_name_count, &service.counter_data_);
                service.counter_data_.get_component_type_name_.enabled_ = true;
                break;
            case component_ns_num_localities:
                get_data_func = hpx::bind_front(
                    &cd::get_num_localities_count, &service.counter_data_);
                service.counter_data_.num_localities_.enabled_ = true;
                break;
            case component_ns_statistics_counter:
                get_data_func = hpx::bind_front(
                    &cd::get_overall_count, &service.counter_data_);
                service.counter_data_.enable_all();
                break;
            default:
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "component_namespace::statistics",
                    "bad action code while querying statistics");
            }
        }
        else
        {
            HPX_ASSERT(detail::counter_target_time == target);
            switch (code)
            {
            case component_ns_bind_prefix:
                get_data_func = hpx::bind_front(
                    &cd::get_bind_prefix_time, &service.counter_data_);
                service.counter_data_.bind_prefix_.enabled_ = true;
                break;
            case component_ns_bind_name:
                get_data_func = hpx::bind_front(
                    &cd::get_bind_name_time, &service.counter_data_);
                service.counter_data_.bind_name_.enabled_ = true;
                break;
            case component_ns_resolve_id:
                get_data_func = hpx::bind_front(
                    &cd::get_resolve_id_time, &service.counter_data_);
                service.counter_data_.resolve_id_.enabled_ = true;
                break;
            case component_ns_unbind_name:
                get_data_func = hpx::bind_front(
                    &cd::get_unbind_name_time, &service.counter_data_);
                service.counter_data_.unbind_name_.enabled_ = true;
                break;
            case component_ns_iterate_types:
                get_data_func = hpx::bind_front(
                    &cd::get_iterate_types_time, &service.counter_data_);
                service.counter_data_.iterate_types_.enabled_ = true;
                break;
            case component_ns_get_component_type_name:
                get_data_func = hpx::bind_front(
                    &cd::get_component_type_name_time, &service.counter_data_);
                service.counter_data_.get_component_type_name_.enabled_ = true;
                break;
            case component_ns_num_localities:
                get_data_func = hpx::bind_front(
                    &cd::get_num_localities_time, &service.counter_data_);
                service.counter_data_.num_localities_.enabled_ = true;
                break;
            case component_ns_statistics_counter:
                get_data_func = hpx::bind_front(
                    &cd::get_overall_time, &service.counter_data_);
                service.counter_data_.enable_all();
                break;
            default:
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "component_namespace::statistics",
                    "bad action code while querying statistics");
            }
        }

        performance_counters::counter_info info;
        performance_counters::get_counter_type(name, info, ec);
        if (ec)
        {
            return naming::invalid_gid;
        }
        performance_counters::complement_counter_info(info, ec);
        if (ec)
        {
            return naming::invalid_gid;
        }
        using performance_counters::detail::create_raw_counter;
        naming::gid_type gid = create_raw_counter(
            info, hpx::function<std::int64_t(bool)>(get_data_func), ec);
        if (ec)
        {
            return naming::invalid_gid;
        }
        return naming::detail::strip_credits_from_gid(gid);
    }
}    // namespace hpx::agas::server

namespace hpx::agas {

    // register performance counters for component_namespace service
    void component_namespace_register_counter_types(error_code& ec)
    {
        server::component_namespace_register_counter_types(ec);
        if (!ec)
        {
            server::component_namespace_register_global_counter_types(ec);
        }
    }

    // statistics_counter implementation
    naming::gid_type component_namespace_statistics_counter(
        std::string const& name)
    {
        auto* component_service =
            naming::get_agas_client().get_local_component_namespace_service();
        if (component_service != nullptr)    //-V547
        {
            return server::component_namespace_statistics_counter(
                *component_service, name);
        }
        return naming::invalid_gid;
    }
}    // namespace hpx::agas

HPX_REGISTER_ACTION_ID(hpx::agas::component_namespace_statistics_counter_action,
    component_namespace_statistics_counter_action,
    hpx::actions::component_namespace_statistics_counter_action_id)
