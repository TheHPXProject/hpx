//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//  Copyright (c) 2012-2026 Hartmut Kaiser
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
#include <hpx/performance_counters/counter_creators.hpp>
#include <hpx/performance_counters/counters.hpp>
#include <hpx/performance_counters/manage_counter_type.hpp>
#include <hpx/performance_counters/primary_namespace_counters.hpp>
#include <hpx/performance_counters/server/primary_namespace_counters.hpp>

#include <cstddef>
#include <cstdint>
#include <string>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::agas::server {

    // register all performance counter types exposed by this component
    void primary_namespace_register_counter_types(error_code& ec)
    {
        performance_counters::create_counter_func creator(
            hpx::bind_back(&performance_counters::agas_raw_counter_creator,
                agas::server::primary_namespace_service_name));

        for (std::size_t i = 0;
            i != agas::detail::num_primary_namespace_services; ++i)
        {
            // global counters are handled elsewhere
            if (agas::detail::primary_namespace_services[i].code_ ==
                primary_ns_statistics_counter)
            {
                continue;
            }

            std::string name(agas::detail::primary_namespace_services[i].name_);
            std::string help;
            performance_counters::counter_type type;
            std::string::size_type p = name.find_last_of('/');
            HPX_ASSERT(p != std::string::npos);

            if (agas::detail::primary_namespace_services[i].target_ ==
                agas::detail::counter_target_count)
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
                &performance_counters::locality_counter_discoverer,
                performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                agas::detail::primary_namespace_services[i].uom_, ec);
            if (ec)
            {
                return;
            }
        }
    }

    void primary_namespace_register_global_counter_types(error_code& ec)
    {
        performance_counters::create_counter_func creator(
            hpx::bind_back(&performance_counters::agas_raw_counter_creator,
                agas::server::primary_namespace_service_name));

        for (std::size_t i = 0;
            i != agas::detail::num_primary_namespace_services; ++i)
        {
            // local counters are handled elsewhere
            if (agas::detail::primary_namespace_services[i].code_ !=
                primary_ns_statistics_counter)
            {
                continue;
            }

            std::string help;
            performance_counters::counter_type type;
            if (agas::detail::primary_namespace_services[i].target_ ==
                agas::detail::counter_target_count)
            {
                help = "returns the overall number of invocations of all "
                       "primary AGAS services";
                type = performance_counters::counter_type::
                    monotonically_increasing;
            }
            else
            {
                help = "returns the overall execution time of all primary "
                       "AGAS services";
                type = performance_counters::counter_type::elapsed_time;
            }

            performance_counters::install_counter_type(
                std::string(agas::performance_counter_basename) +
                    agas::detail::primary_namespace_services[i].name_,
                type, help, creator,
                &performance_counters::locality_counter_discoverer,
                performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                agas::detail::primary_namespace_services[i].uom_, ec);
            if (ec)
            {
                return;
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    naming::gid_type primary_namespace_statistics_counter(
        primary_namespace& service, std::string const& name)
    {    // statistics_counter implementation
        LAGAS_(info).format("primary_namespace_statistics_counter");

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
                "primary_namespace::statistics_counter",
                "unknown performance counter (unrelated to AGAS)");
        }

        namespace_action_code code = invalid_request;
        detail::counter_target target = agas::detail::counter_target_invalid;
        for (std::size_t i = 0;
            i != agas::detail::num_primary_namespace_services; ++i)
        {
            if (p.countername_ ==
                agas::detail::primary_namespace_services[i].name_)
            {
                code = agas::detail::primary_namespace_services[i].code_;
                target = agas::detail::primary_namespace_services[i].target_;
                break;
            }
        }

        if (code == invalid_request ||
            target == agas::detail::counter_target_invalid)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "primary_namespace::statistics_counter",
                "unknown performance counter (unrelated to AGAS?)");
        }

        using cd = primary_namespace::counter_data;

        hpx::function<std::int64_t(bool)> get_data_func;
        if (target == agas::detail::counter_target_count)
        {
            switch (code)
            {
#if defined(HPX_HAVE_NETWORKING)
            case primary_ns_route:
                get_data_func = hpx::bind_front(
                    &cd::get_route_count, &service.counter_data_);
                service.counter_data_.route_.enabled_ = true;
                break;
#endif
            case primary_ns_bind_gid:
                get_data_func = hpx::bind_front(
                    &cd::get_bind_gid_count, &service.counter_data_);
                service.counter_data_.bind_gid_.enabled_ = true;
                break;
            case primary_ns_resolve_gid:
                get_data_func = hpx::bind_front(
                    &cd::get_resolve_gid_count, &service.counter_data_);
                service.counter_data_.resolve_gid_.enabled_ = true;
                break;
            case primary_ns_unbind_gid:
                get_data_func = hpx::bind_front(
                    &cd::get_unbind_gid_count, &service.counter_data_);
                service.counter_data_.unbind_gid_.enabled_ = true;
                break;
            case primary_ns_increment_credit:
                get_data_func = hpx::bind_front(
                    &cd::get_increment_credit_count, &service.counter_data_);
                service.counter_data_.increment_credit_.enabled_ = true;
                break;
            case primary_ns_decrement_credit:
                get_data_func = hpx::bind_front(
                    &cd::get_decrement_credit_count, &service.counter_data_);
                service.counter_data_.decrement_credit_.enabled_ = true;
                break;
            case primary_ns_allocate:
                get_data_func = hpx::bind_front(
                    &cd::get_allocate_count, &service.counter_data_);
                service.counter_data_.allocate_.enabled_ = true;
                break;
            case primary_ns_begin_migration:
                get_data_func = hpx::bind_front(
                    &cd::get_begin_migration_count, &service.counter_data_);
                service.counter_data_.begin_migration_.enabled_ = true;
                break;
            case primary_ns_end_migration:
                get_data_func = hpx::bind_front(
                    &cd::get_end_migration_count, &service.counter_data_);
                service.counter_data_.end_migration_.enabled_ = true;
                break;
            case primary_ns_statistics_counter:
                get_data_func = hpx::bind_front(
                    &cd::get_overall_count, &service.counter_data_);
                service.counter_data_.enable_all();
                break;
            default:
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "primary_namespace::statistics",
                    "bad action code while querying statistics");
            }
        }
        else
        {
            HPX_ASSERT(agas::detail::counter_target_time == target);
            switch (code)
            {
#if defined(HPX_HAVE_NETWORKING)
            case primary_ns_route:
                get_data_func = hpx::bind_front(
                    &cd::get_route_time, &service.counter_data_);
                service.counter_data_.route_.enabled_ = true;
                break;
#endif
            case primary_ns_bind_gid:
                get_data_func = hpx::bind_front(
                    &cd::get_bind_gid_time, &service.counter_data_);
                service.counter_data_.bind_gid_.enabled_ = true;
                break;
            case primary_ns_resolve_gid:
                get_data_func = hpx::bind_front(
                    &cd::get_resolve_gid_time, &service.counter_data_);
                service.counter_data_.resolve_gid_.enabled_ = true;
                break;
            case primary_ns_unbind_gid:
                get_data_func = hpx::bind_front(
                    &cd::get_unbind_gid_time, &service.counter_data_);
                service.counter_data_.unbind_gid_.enabled_ = true;
                break;
            case primary_ns_increment_credit:
                get_data_func = hpx::bind_front(
                    &cd::get_increment_credit_time, &service.counter_data_);
                service.counter_data_.increment_credit_.enabled_ = true;
                break;
            case primary_ns_decrement_credit:
                get_data_func = hpx::bind_front(
                    &cd::get_decrement_credit_time, &service.counter_data_);
                service.counter_data_.decrement_credit_.enabled_ = true;
                break;
            case primary_ns_allocate:
                get_data_func = hpx::bind_front(
                    &cd::get_allocate_time, &service.counter_data_);
                service.counter_data_.allocate_.enabled_ = true;
                break;
            case primary_ns_begin_migration:
                get_data_func = hpx::bind_front(
                    &cd::get_begin_migration_time, &service.counter_data_);
                service.counter_data_.begin_migration_.enabled_ = true;
                break;
            case primary_ns_end_migration:
                get_data_func = hpx::bind_front(
                    &cd::get_end_migration_time, &service.counter_data_);
                service.counter_data_.end_migration_.enabled_ = true;
                break;
            case primary_ns_statistics_counter:
                get_data_func = hpx::bind_front(
                    &cd::get_overall_time, &service.counter_data_);
                service.counter_data_.enable_all();
                break;
            default:
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "primary_namespace::statistics",
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
        naming::gid_type gid = create_raw_counter(info, get_data_func, ec);
        if (ec)
        {
            return naming::invalid_gid;
        }
        return naming::detail::strip_credits_from_gid(gid);
    }
}    // namespace hpx::agas::server

namespace hpx::agas {

    // register performance counters for primary_namespace service
    void primary_namespace_register_counter_types(error_code& ec)
    {
        server::primary_namespace_register_counter_types(ec);
        if (!ec)
        {
            server::primary_namespace_register_global_counter_types(ec);
        }
    }

    // statistics_counter implementation
    naming::gid_type primary_namespace_statistics_counter(
        std::string const& name)
    {
        return server::primary_namespace_statistics_counter(
            naming::get_agas_client().get_local_primary_namespace_service(),
            name);
    }
}    // namespace hpx::agas

HPX_REGISTER_ACTION_ID(hpx::agas::primary_namespace_statistics_counter_action,
    primary_namespace_statistics_counter_action,
    hpx::actions::primary_namespace_statistics_counter_action_id)
