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

#include <hpx/performance_counters/agas_namespace_action_code.hpp>
#include <hpx/performance_counters/counter_creators.hpp>
#include <hpx/performance_counters/counters.hpp>
#include <hpx/performance_counters/manage_counter_type.hpp>
#include <hpx/performance_counters/server/symbol_namespace_counters.hpp>
#include <hpx/performance_counters/symbol_namespace_counters.hpp>

#include <cstdint>
#include <string>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::agas::server {

    // register all performance counter types exposed by this component
    void symbol_namespace_register_counter_types(error_code& ec)
    {
        performance_counters::create_counter_func const creator(
            hpx::bind_back(&performance_counters::agas_raw_counter_creator,
                agas::server::symbol_namespace_service_name));

        for (auto const& symbol_namespace_service :
            detail::symbol_namespace_services)
        {
            // global counters are handled elsewhere
            if (symbol_namespace_service.code_ == symbol_ns_statistics_counter)
            {
                continue;
            }

            std::string name(symbol_namespace_service.name_);
            std::string help;
            performance_counters::counter_type type;
            std::string::size_type const p = name.find_last_of('/');
            HPX_ASSERT(p != std::string::npos);

            if (symbol_namespace_service.target_ ==
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
                &performance_counters::locality_counter_discoverer,
                performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                symbol_namespace_service.uom_, ec);
            if (ec)
                return;
        }
    }

    void symbol_namespace_register_global_counter_types(error_code& ec)
    {
        performance_counters::create_counter_func const creator(
            hpx::bind_back(&performance_counters::agas_raw_counter_creator,
                agas::server::symbol_namespace_service_name));

        for (auto const& symbol_namespace_service :
            detail::symbol_namespace_services)
        {
            // local counters are handled elsewhere
            if (symbol_namespace_service.code_ != symbol_ns_statistics_counter)
            {
                continue;
            }

            std::string help;
            performance_counters::counter_type type;
            if (symbol_namespace_service.target_ ==
                detail::counter_target_count)
            {
                help = "returns the overall number of invocations of all "
                       "symbol AGAS services";
                type = performance_counters::counter_type::
                    monotonically_increasing;
            }
            else
            {
                help = "returns the overall execution time of all symbol AGAS "
                       "services";
                type = performance_counters::counter_type::elapsed_time;
            }

            performance_counters::install_counter_type(
                std::string(agas::performance_counter_basename) +
                    symbol_namespace_service.name_,
                type, help, creator,
                &performance_counters::locality_counter_discoverer,
                performance_counters::HPX_PERFORMANCE_COUNTER_V1,
                symbol_namespace_service.uom_, ec);
            if (ec)
            {
                return;
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    naming::gid_type symbol_namespace_statistics_counter(
        symbol_namespace& service, std::string const& name)
    {    // statistics_counter implementation
        LAGAS_(info).format("symbol_namespace::statistics_counter");

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
                "symbol_namespace::statistics_counter",
                "unknown performance counter (unrelated to AGAS)");
        }

        namespace_action_code code = invalid_request;
        detail::counter_target target = detail::counter_target_invalid;
        for (auto const& symbol_namespace_service :
            detail::symbol_namespace_services)
        {
            if (p.countername_ == name)
            {
                code = symbol_namespace_service.code_;
                target = symbol_namespace_service.target_;
                break;
            }
        }

        if (code == invalid_request || target == detail::counter_target_invalid)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "symbol_namespace::statistics_counter",
                "unknown performance counter (unrelated to AGAS)");
        }

        using cd = symbol_namespace::counter_data;

        hpx::function<std::int64_t(bool)> get_data_func;
        if (target == detail::counter_target_count)
        {
            switch (code)
            {
            case symbol_ns_bind:
                get_data_func = hpx::bind_front(
                    &cd::get_bind_count, &service.counter_data_);
                service.counter_data_.bind_.enabled_ = true;
                break;
            case symbol_ns_resolve:
                get_data_func = hpx::bind_front(
                    &cd::get_resolve_count, &service.counter_data_);
                service.counter_data_.resolve_.enabled_ = true;
                break;
            case symbol_ns_unbind:
                get_data_func = hpx::bind_front(
                    &cd::get_unbind_count, &service.counter_data_);
                service.counter_data_.unbind_.enabled_ = true;
                break;
            case symbol_ns_iterate_names:
                get_data_func = hpx::bind_front(
                    &cd::get_iterate_names_count, &service.counter_data_);
                service.counter_data_.iterate_names_.enabled_ = true;
                break;
            case symbol_ns_on_event:
                get_data_func = hpx::bind_front(
                    &cd::get_on_event_count, &service.counter_data_);
                service.counter_data_.on_event_.enabled_ = true;
                break;
            case symbol_ns_statistics_counter:
                get_data_func = hpx::bind_front(
                    &cd::get_overall_count, &service.counter_data_);
                service.counter_data_.enable_all();
                break;
            default:
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "symbol_namespace::statistics",
                    "bad action code while querying statistics");
            }
        }
        else
        {
            HPX_ASSERT(detail::counter_target_time == target);
            switch (code)
            {
            case symbol_ns_bind:
                get_data_func =
                    hpx::bind_front(&cd::get_bind_time, &service.counter_data_);
                service.counter_data_.bind_.enabled_ = true;
                break;
            case symbol_ns_resolve:
                get_data_func = hpx::bind_front(
                    &cd::get_resolve_time, &service.counter_data_);
                service.counter_data_.resolve_.enabled_ = true;
                break;
            case symbol_ns_unbind:
                get_data_func = hpx::bind_front(
                    &cd::get_unbind_time, &service.counter_data_);
                service.counter_data_.unbind_.enabled_ = true;
                break;
            case symbol_ns_iterate_names:
                get_data_func = hpx::bind_front(
                    &cd::get_iterate_names_time, &service.counter_data_);
                service.counter_data_.iterate_names_.enabled_ = true;
                break;
            case symbol_ns_on_event:
                get_data_func = hpx::bind_front(
                    &cd::get_on_event_time, &service.counter_data_);
                service.counter_data_.on_event_.enabled_ = true;
                break;
            case symbol_ns_statistics_counter:
                get_data_func = hpx::bind_front(
                    &cd::get_overall_time, &service.counter_data_);
                service.counter_data_.enable_all();
                break;
            default:
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "symbol_namespace::statistics",
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

    // register performance counters for symbol_namespace service
    void symbol_namespace_register_counter_types(error_code& ec)
    {
        server::symbol_namespace_register_counter_types(ec);
        if (!ec)
        {
            server::symbol_namespace_register_global_counter_types(ec);
        }
    }

    // statistics_counter implementation
    naming::gid_type symbol_namespace_statistics_counter(
        std::string const& name)
    {
        return server::symbol_namespace_statistics_counter(
            naming::get_agas_client().get_local_symbol_namespace_service(),
            name);
    }
}    // namespace hpx::agas

HPX_REGISTER_ACTION_ID(hpx::agas::symbol_namespace_statistics_counter_action,
    symbol_namespace_statistics_counter_action,
    hpx::actions::symbol_namespace_statistics_counter_action_id)
