//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//  Copyright (c) 2012-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>

#include <cstddef>
#include <string>

namespace hpx::agas {

    // Base name used to register AGAS service instances
    inline constexpr char const* const performance_counter_basename = "/agas/";

    enum namespace_action_code
    {
        invalid_request = 0,

        locality_ns_service = 0b1100000,
        locality_ns_bulk_service = 0b1100001,
        locality_ns_allocate = 0b1100010,
        locality_ns_free = 0b1100011,
        locality_ns_localities = 0b1100100,
        locality_ns_num_localities = 0b1100101,
        locality_ns_num_threads = 0b1100110,
        locality_ns_statistics_counter = 0b1100111,
        locality_ns_resolve_locality = 0b1101000,

        primary_ns_service = 0b1000000,
        primary_ns_bulk_service = 0b1000001,
        primary_ns_route = 0b1000010,
        primary_ns_bind_gid = 0b1000011,
        primary_ns_resolve_gid = 0b1000100,
        primary_ns_unbind_gid = 0b1000101,
        primary_ns_increment_credit = 0b1000110,
        primary_ns_decrement_credit = 0b1000111,
        primary_ns_allocate = 0b1001000,
        primary_ns_begin_migration = 0b1001001,
        primary_ns_end_migration = 0b1001010,
        primary_ns_statistics_counter = 0b1001011,

        component_ns_service = 0b0100000,
        component_ns_bulk_service = 0b0100001,
        component_ns_bind_prefix = 0b0100010,
        component_ns_bind_name = 0b0100011,
        component_ns_resolve_id = 0b0100100,
        component_ns_unbind_name = 0b0100101,
        component_ns_iterate_types = 0b0100110,
        component_ns_get_component_type_name = 0b0100111,
        component_ns_num_localities = 0b0101000,
        component_ns_statistics_counter = 0b0101001,

        symbol_ns_service = 0b0010000,
        symbol_ns_bulk_service = 0b0010001,
        symbol_ns_bind = 0b0010010,
        symbol_ns_resolve = 0b0010011,
        symbol_ns_unbind = 0b0010100,
        symbol_ns_iterate_names = 0b0010101,
        symbol_ns_on_event = 0b0010110,
        symbol_ns_statistics_counter = 0b0010111
    };
}    // namespace hpx::agas

namespace hpx::agas::detail {

    enum counter_target
    {
        counter_target_invalid = -1,
        counter_target_count = 0,
        counter_target_time = 1
    };

    struct counter_service_data
    {
        char const* const name_;    // name of performance counter
        char const* const uom_;     // unit of measure of performance counter
        counter_target target_;     // target type of the counter
        namespace_action_code code_;            // corresponding AGAS service
        namespace_action_code service_code_;    // ... and AGAS component
    };

    // counter description data for component namespace components
    inline constexpr counter_service_data component_namespace_services[] = {
        // counters exposing overall API invocation count and timings
        {"component/count", "", counter_target_count,
            component_ns_statistics_counter, component_ns_statistics_counter},
        {"component/time", "ns", counter_target_time,
            component_ns_statistics_counter, component_ns_statistics_counter},
        // counters exposing API invocation counts
        {"count/bind_prefix", "", counter_target_count,
            component_ns_bind_prefix, component_ns_statistics_counter},
        {"count/bind_name", "", counter_target_count, component_ns_bind_name,
            component_ns_statistics_counter},
        {"count/resolve_id", "", counter_target_count, component_ns_resolve_id,
            component_ns_statistics_counter},
        {"count/unbind_name", "", counter_target_count,
            component_ns_unbind_name, component_ns_statistics_counter},
        {"count/iterate_types", "", counter_target_count,
            component_ns_iterate_types, component_ns_statistics_counter},
        {"count/get_component_typename", "", counter_target_count,
            component_ns_get_component_type_name,
            component_ns_statistics_counter},
        {"count/num_localities_type", "", counter_target_time,
            component_ns_num_localities, component_ns_statistics_counter},
        // counters exposing API timings
        {"time/bind_prefix", "ns", counter_target_time,
            component_ns_bind_prefix, component_ns_statistics_counter},
        {"time/bind_name", "ns", counter_target_time, component_ns_bind_name,
            component_ns_statistics_counter},
        {"time/resolve_id", "ns", counter_target_time, component_ns_resolve_id,
            component_ns_statistics_counter},
        {"time/unbind_name", "ns", counter_target_time,
            component_ns_unbind_name, component_ns_statistics_counter},
        {"time/iterate_types", "ns", counter_target_time,
            component_ns_iterate_types, component_ns_statistics_counter},
        {"time/get_component_typename", "ns", counter_target_time,
            component_ns_get_component_type_name,
            component_ns_statistics_counter},
        {"time/num_localities_type", "ns", counter_target_time,
            component_ns_num_localities, component_ns_statistics_counter}};

    // counter description data for localities namespace components
    inline constexpr counter_service_data locality_namespace_services[] = {
        // counters exposing overall API invocation count and timings
        {"locality/count", "", counter_target_count,
            locality_ns_statistics_counter, locality_ns_statistics_counter},
        {"locality/time", "ns", counter_target_time,
            locality_ns_statistics_counter, locality_ns_statistics_counter},
        // counters exposing API invocation counts
        {"count/free", "", counter_target_count, locality_ns_free,
            locality_ns_statistics_counter},
        {"count/localities", "", counter_target_count, locality_ns_localities,
            locality_ns_statistics_counter},
        {"count/num_localities", "", counter_target_count,
            locality_ns_num_localities, locality_ns_statistics_counter},
        {"count/num_threads", "", counter_target_count, locality_ns_num_threads,
            locality_ns_statistics_counter},
        {"count/resolve_locality", "", counter_target_count,
            locality_ns_resolve_locality, locality_ns_statistics_counter},
        // counters exposing API timings
        {"time/free", "ns", counter_target_time, locality_ns_free,
            locality_ns_statistics_counter},
        {"time/localities", "ns", counter_target_time, locality_ns_localities,
            locality_ns_statistics_counter},
        {"time/num_localities", "ns", counter_target_time,
            locality_ns_num_localities, locality_ns_statistics_counter},
        {"time/num_threads", "ns", counter_target_time, locality_ns_num_threads,
            locality_ns_statistics_counter},
        {"time/resolve_locality", "ns", counter_target_time,
            locality_ns_resolve_locality, locality_ns_statistics_counter}};

    inline constexpr std::size_t num_locality_namespace_services =
        sizeof(locality_namespace_services) /
        sizeof(locality_namespace_services[0]);

    // counter description data for primary namespace components
    inline constexpr counter_service_data primary_namespace_services[] = {
        // counters exposing overall API invocation count and timings
        {"primary/count", "", counter_target_count,
            primary_ns_statistics_counter, primary_ns_statistics_counter},
        {"primary/time", "ns", counter_target_time,
            primary_ns_statistics_counter, primary_ns_statistics_counter},
        // counters exposing API invocation counts
        {"count/bind_gid", "", counter_target_count, primary_ns_bind_gid,
            primary_ns_statistics_counter},
        {"count/resolve_gid", "", counter_target_count, primary_ns_resolve_gid,
            primary_ns_statistics_counter},
        {"count/unbind_gid", "", counter_target_count, primary_ns_unbind_gid,
            primary_ns_statistics_counter},
        {"count/increment_credit", "", counter_target_count,
            primary_ns_increment_credit, primary_ns_statistics_counter},
        {"count/decrement_credit", "", counter_target_count,
            primary_ns_decrement_credit, primary_ns_statistics_counter},
        {"count/allocate", "", counter_target_count, primary_ns_allocate,
            primary_ns_statistics_counter},
        {"count/begin_migration", "", counter_target_count,
            primary_ns_begin_migration, primary_ns_statistics_counter},
        {"count/end_migration", "", counter_target_count,
            primary_ns_end_migration, primary_ns_statistics_counter},
#if defined(HPX_HAVE_NETWORKING)
        {"count/route", "", counter_target_count, primary_ns_route,
            primary_ns_statistics_counter},
#endif
        // counters exposing API timings
        {"time/bind_gid", "ns", counter_target_time, primary_ns_bind_gid,
            primary_ns_statistics_counter},
        {"time/resolve_gid", "ns", counter_target_time, primary_ns_resolve_gid,
            primary_ns_statistics_counter},
        {"time/unbind_gid", "ns", counter_target_time, primary_ns_unbind_gid,
            primary_ns_statistics_counter},
        {"time/increment_credit", "ns", counter_target_time,
            primary_ns_increment_credit, primary_ns_statistics_counter},
        {"time/decrement_credit", "ns", counter_target_time,
            primary_ns_decrement_credit, primary_ns_statistics_counter},
        {"time/allocate", "ns", counter_target_time, primary_ns_allocate,
            primary_ns_statistics_counter},
        {"time/begin_migration", "ns", counter_target_time,
            primary_ns_begin_migration, primary_ns_statistics_counter},
        {"time/end_migration", "ns", counter_target_time,
            primary_ns_end_migration, primary_ns_statistics_counter},
#if defined(HPX_HAVE_NETWORKING)
        {"time/route", "ns", counter_target_time, primary_ns_route,
            primary_ns_statistics_counter}
#endif
    };

    inline constexpr std::size_t num_primary_namespace_services =
        sizeof(primary_namespace_services) /
        sizeof(primary_namespace_services[0]);

    // counter description data for symbol namespace components
    inline constexpr counter_service_data const symbol_namespace_services[] = {
        // counters exposing overall API invocation count and timings
        {"symbol/count", "", counter_target_count, symbol_ns_statistics_counter,
            symbol_ns_statistics_counter},
        {"symbol/time", "ns", counter_target_time, symbol_ns_statistics_counter,
            symbol_ns_statistics_counter},
        // counters exposing API invocation counts
        {"count/bind", "", counter_target_count, symbol_ns_bind,
            symbol_ns_statistics_counter},
        {"count/resolve", "", counter_target_count, symbol_ns_resolve,
            symbol_ns_statistics_counter},
        {"count/unbind", "", counter_target_count, symbol_ns_unbind,
            symbol_ns_statistics_counter},
        {"count/iterate_names", "", counter_target_count,
            symbol_ns_iterate_names, symbol_ns_statistics_counter},
        {"count/on_symbol_namespace_event", "", counter_target_count,
            symbol_ns_on_event, symbol_ns_statistics_counter},
        // counters exposing API timings
        {"time/bind", "ns", counter_target_time, symbol_ns_bind,
            symbol_ns_statistics_counter},
        {"time/resolve", "ns", counter_target_time, symbol_ns_resolve,
            symbol_ns_statistics_counter},
        {"time/unbind", "ns", counter_target_time, symbol_ns_unbind,
            symbol_ns_statistics_counter},
        {"time/iterate_names", "ns", counter_target_time,
            symbol_ns_iterate_names, symbol_ns_statistics_counter},
        {"time/on_symbol_namespace_event", "ns", counter_target_time,
            symbol_ns_on_event, symbol_ns_statistics_counter}};

    ///////////////////////////////////////////////////////////////////////////
    // get action code from counter type
    namespace_action_code retrieve_action_code(
        std::string const& name, error_code& ec = throws);

    // get service action code from counter type
    namespace_action_code retrieve_action_service_code(
        std::string const& name, error_code& ec = throws);
}    // namespace hpx::agas::detail
