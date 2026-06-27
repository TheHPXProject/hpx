//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//  Copyright (c) 2012-2026 Hartmut Kaiser
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
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/synchronization.hpp>

#include <hpx/agas_base/agas_fwd.hpp>

#include <atomic>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::agas {

    HPX_CXX_EXPORT HPX_EXPORT naming::gid_type
    bootstrap_component_namespace_gid();
    HPX_CXX_EXPORT HPX_EXPORT hpx::id_type bootstrap_component_namespace_id();
}    // namespace hpx::agas

namespace hpx::agas::server {

    // Base name used to register the component
    HPX_CXX_EXPORT inline constexpr char const* const
        component_namespace_service_name = "component/";

    HPX_CXX_EXPORT struct HPX_EXPORT component_namespace
      : components::fixed_component_base<component_namespace>
    {
        using mutex_type = hpx::spinlock;
        using base_type = components::fixed_component_base<component_namespace>;

        using component_id_type = components::component_type;

        using prefixes_type = std::set<std::uint32_t>;

        using component_id_table_type =
            std::unordered_map<std::string, component_id_type>;

        using factory_table_type = std::map<component_id_type, prefixes_type>;

    private:
        // REVIEW: Separate mutexes might reduce contention here. This has to be
        // investigated carefully.
        mutex_type mutex_ = mutex_type("component_namespace");
        component_id_table_type component_ids_;
        factory_table_type factories_;
        component_id_type type_counter;
        std::string instance_name_;

    public:
        // data structure holding all counters for the component_namespace
        // component
        struct counter_data
        {
        public:
            counter_data(counter_data const&) = delete;
            counter_data(counter_data&&) = delete;
            counter_data& operator=(counter_data const&) = delete;
            counter_data& operator=(counter_data&&) = delete;

        public:
            using mutex_type = hpx::spinlock;

            struct api_counter_data
            {
                std::atomic<std::int64_t> count_ = 0;
                std::atomic<std::int64_t> time_ = 0;
                bool enabled_ = false;
            };

            counter_data() = default;

        public:
            // access current counter values
            std::int64_t get_bind_prefix_count(bool);
            std::int64_t get_bind_name_count(bool);
            std::int64_t get_resolve_id_count(bool);
            std::int64_t get_unbind_name_count(bool);
            std::int64_t get_iterate_types_count(bool);
            std::int64_t get_component_type_name_count(bool);
            std::int64_t get_num_localities_count(bool);
            std::int64_t get_overall_count(bool);

            std::int64_t get_bind_prefix_time(bool);
            std::int64_t get_bind_name_time(bool);
            std::int64_t get_resolve_id_time(bool);
            std::int64_t get_unbind_name_time(bool);
            std::int64_t get_iterate_types_time(bool);
            std::int64_t get_component_type_name_time(bool);
            std::int64_t get_num_localities_time(bool);
            std::int64_t get_overall_time(bool);

            // increment counter values
            void increment_bind_prefix_count();
            void increment_bind_name_count();
            void increment_resolve_id_count();
            void increment_unbind_name_count();
            void increment_iterate_types_count();
            void increment_get_component_type_name_count();
            void increment_num_localities_count();

            void enable_all();

            api_counter_data bind_prefix_;      // component_ns_bind_prefix
            api_counter_data bind_name_;        // component_ns_bind_name
            api_counter_data resolve_id_;       // component_ns_resolve_id
            api_counter_data unbind_name_;      // component_ns_unbind_name
            api_counter_data iterate_types_;    // component_ns_iterate_types
            api_counter_data get_component_type_name_;
            // component_ns_get_component_type_name
            api_counter_data num_localities_;    // component_ns_num_localities
        };

        counter_data counter_data_;

        component_namespace()
          : base_type(agas::component_ns_msb, agas::component_ns_lsb)
          , type_counter(to_int(components::component_enum_type::first_dynamic))
        {
        }

        void finalize();

        void register_server_instance(
            char const* servicename, error_code& ec = throws);

        void unregister_server_instance(error_code& ec = throws);

        components::component_type bind_prefix(
            std::string const& key, std::uint32_t prefix);

        components::component_type bind_name(std::string const& name);

        std::vector<std::uint32_t> resolve_id(components::component_type key);

        bool unbind(std::string const& key);

        void iterate_types(iterate_types_function_type const& f);

        std::string get_component_type_name(components::component_type type);

        std::uint32_t get_num_localities(components::component_type type);

        HPX_DEFINE_COMPONENT_ACTION(component_namespace, bind_prefix)
        HPX_DEFINE_COMPONENT_ACTION(component_namespace, bind_name)
        HPX_DEFINE_COMPONENT_ACTION(component_namespace, resolve_id)
        HPX_DEFINE_COMPONENT_ACTION(component_namespace, unbind)
        HPX_DEFINE_COMPONENT_ACTION(component_namespace, iterate_types)
        HPX_DEFINE_COMPONENT_ACTION(
            component_namespace, get_component_type_name)
        HPX_DEFINE_COMPONENT_ACTION(component_namespace, get_num_localities)
    };
}    // namespace hpx::agas::server

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::component_namespace::bind_prefix_action,
    component_namespace_bind_prefix_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::component_namespace::bind_name_action,
    component_namespace_bind_name_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::component_namespace::resolve_id_action,
    component_namespace_resolve_id_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::component_namespace::unbind_action,
    component_namespace_unbind_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::component_namespace::iterate_types_action,
    component_namespace_iterate_types_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::component_namespace::get_component_type_name_action,
    component_namespace_get_component_type_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::component_namespace::get_num_localities_action,
    component_namespace_get_num_localities_action)

#include <hpx/config/warnings_suffix.hpp>
