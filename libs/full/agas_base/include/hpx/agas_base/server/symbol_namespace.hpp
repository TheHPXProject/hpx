//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//  Copyright (c) 2012-2023 Hartmut Kaiser
//  Copyright (c) 2016 Thomas Heller
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
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/synchronization.hpp>

#include <hpx/agas_base/agas_fwd.hpp>

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::agas {

    HPX_CXX_EXPORT HPX_EXPORT naming::gid_type bootstrap_symbol_namespace_gid();
    HPX_CXX_EXPORT HPX_EXPORT hpx::id_type bootstrap_symbol_namespace_id();
}    // namespace hpx::agas

namespace hpx::agas::server {

    // Base name used to register the component
    HPX_CXX_EXPORT inline constexpr char const* const
        symbol_namespace_service_name = "symbol/";

    HPX_CXX_EXPORT struct HPX_EXPORT symbol_namespace
      : components::fixed_component_base<symbol_namespace>
    {
        using mutex_type = hpx::spinlock;
        using base_type = components::fixed_component_base<symbol_namespace>;
        using iterate_names_return_type =
            std::map<std::string, naming::gid_type>;

        using gid_table_type =
            std::map<std::string, std::shared_ptr<naming::gid_type>>;

        using on_event_data_map_type = std::multimap<std::string, hpx::id_type>;

    private:
        mutex_type mutex_ = mutex_type("symbol_namespace");
        gid_table_type gids_;
        std::string instance_name_;
        on_event_data_map_type on_event_data_;

    public:
        // data structure holding all counters for the component_namespace component
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
            ~counter_data() = default;

        public:
            // access current counter values
            [[nodiscard]] std::int64_t get_bind_count(bool);
            [[nodiscard]] std::int64_t get_resolve_count(bool);
            [[nodiscard]] std::int64_t get_unbind_count(bool);
            [[nodiscard]] std::int64_t get_iterate_names_count(bool);
            [[nodiscard]] std::int64_t get_on_event_count(bool);
            [[nodiscard]] std::int64_t get_overall_count(bool);

            [[nodiscard]] std::int64_t get_bind_time(bool);
            [[nodiscard]] std::int64_t get_resolve_time(bool);
            [[nodiscard]] std::int64_t get_unbind_time(bool);
            [[nodiscard]] std::int64_t get_iterate_names_time(bool);
            [[nodiscard]] std::int64_t get_on_event_time(bool);
            [[nodiscard]] std::int64_t get_overall_time(bool);

            // increment counter values
            void increment_bind_count();
            void increment_resolve_count();
            void increment_unbind_count();
            void increment_iterate_names_count();
            void increment_on_event_count();

            void enable_all();

            api_counter_data bind_;             // symbol_ns_bind
            api_counter_data resolve_;          // symbol_ns_resolve
            api_counter_data unbind_;           // symbol_ns_unbind
            api_counter_data iterate_names_;    // symbol_ns_iterate_names
            api_counter_data on_event_;         // symbol_ns_on_event
        };

        counter_data counter_data_;

    public:
        symbol_namespace()
          : base_type(agas::symbol_ns_msb, agas::symbol_ns_lsb)
        {
        }

        void finalize() const;

        void register_server_instance(char const* servicename,
            std::uint32_t locality_id = naming::invalid_locality_id,
            error_code& ec = throws);

        void unregister_server_instance(error_code& ec = throws) const;

        bool bind(std::string const& key, naming::gid_type const& gid);

        naming::gid_type resolve(std::string const& key);

        naming::gid_type unbind(std::string const& key);

        iterate_names_return_type iterate(std::string const& pattern);

        bool on_event(std::string const& name, bool call_for_past_events,
            hpx::id_type const& lco);

        HPX_DEFINE_COMPONENT_ACTION(symbol_namespace, bind)
        HPX_DEFINE_COMPONENT_ACTION(symbol_namespace, resolve)
        HPX_DEFINE_COMPONENT_ACTION(symbol_namespace, unbind)
        HPX_DEFINE_COMPONENT_ACTION(symbol_namespace, iterate)
        HPX_DEFINE_COMPONENT_ACTION(symbol_namespace, on_event)
    };
}    // namespace hpx::agas::server

HPX_ACTION_USES_MEDIUM_STACK(hpx::agas::server::symbol_namespace::bind_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::symbol_namespace::bind_action,
    symbol_namespace_bind_action)

HPX_ACTION_USES_MEDIUM_STACK(
    hpx::agas::server::symbol_namespace::resolve_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::symbol_namespace::resolve_action,
    symbol_namespace_resolve_action)

HPX_ACTION_USES_MEDIUM_STACK(hpx::agas::server::symbol_namespace::unbind_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::symbol_namespace::unbind_action,
    symbol_namespace_unbind_action)

HPX_ACTION_USES_MEDIUM_STACK(
    hpx::agas::server::symbol_namespace::iterate_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::symbol_namespace::iterate_action,
    symbol_namespace_iterate_action)

HPX_ACTION_USES_MEDIUM_STACK(
    hpx::agas::server::symbol_namespace::on_event_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::symbol_namespace::on_event_action,
    symbol_namespace_on_event_action)

#include <hpx/config/warnings_suffix.hpp>
