//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//  Copyright (c) 2012-2026 Hartmut Kaiser
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
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/modules/synchronization.hpp>

#include <hpx/agas_base/agas_fwd.hpp>

#include <atomic>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::agas {

    HPX_CXX_EXPORT HPX_EXPORT naming::gid_type
    bootstrap_locality_namespace_gid();
    HPX_CXX_EXPORT HPX_EXPORT hpx::id_type bootstrap_locality_namespace_id();
}    // namespace hpx::agas

namespace hpx::agas::server {

    // Base name used to register the component
    HPX_CXX_EXPORT inline constexpr char const* const
        locality_namespace_service_name = "locality/";

    HPX_CXX_EXPORT struct HPX_EXPORT locality_namespace
      : components::fixed_component_base<locality_namespace>
    {
        using mutex_type = hpx::spinlock;
        using base_type = components::fixed_component_base<locality_namespace>;

        using component_type = std::int32_t;

        // stores the locality endpoints, and number of OS-threads running on
        // this locality
        using partition_type =
            hpx::tuple<parcelset::endpoints_type, std::uint32_t>;

        using partition_table_type = std::map<std::uint32_t, partition_type>;

    private:
        // REVIEW: Separate mutexes might reduce contention here. This has to be
        // investigated carefully.
        mutex_type mutex_ = mutex_type("locality_namespace");
        std::string instance_name_;

        partition_table_type partitions_;
        std::uint32_t prefix_counter_;
        primary_namespace* primary_;

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
            std::int64_t get_allocate_count(bool);
            std::int64_t get_resolve_locality_count(bool);
            std::int64_t get_free_count(bool);
            std::int64_t get_localities_count(bool);
            std::int64_t get_num_localities_count(bool);
            std::int64_t get_num_threads_count(bool);
            std::int64_t get_resolved_localities_count(bool);
            std::int64_t get_overall_count(bool);

            std::int64_t get_allocate_time(bool);
            std::int64_t get_resolve_locality_time(bool);
            std::int64_t get_free_time(bool);
            std::int64_t get_localities_time(bool);
            std::int64_t get_num_localities_time(bool);
            std::int64_t get_num_threads_time(bool);
            std::int64_t get_resolved_localities_time(bool);
            std::int64_t get_overall_time(bool);

            // increment counter values
            void increment_allocate_count();
            void increment_resolve_locality_count();
            void increment_free_count();
            void increment_localities_count();
            void increment_num_localities_count();
            void increment_num_threads_count();

            void enable_all();

            api_counter_data allocate_;    // locality_ns_allocate
            api_counter_data
                resolve_locality_;               // locality_ns_resolve_locality
            api_counter_data free_;              // locality_ns_free
            api_counter_data localities_;        // locality_ns_localities
            api_counter_data num_localities_;    // locality_ns_num_localities
            api_counter_data num_threads_;       // locality_ns_num_threads
        };

        counter_data counter_data_;

    public:
        locality_namespace(primary_namespace* primary)
          : base_type(agas::locality_ns_msb, agas::locality_ns_lsb)
          , prefix_counter_(agas::booststrap_prefix)
          , primary_(primary)
        {
        }

        void finalize();

        void register_server_instance(
            char const* servicename, error_code& ec = throws);

        void unregister_server_instance(error_code& ec = throws);

        std::uint32_t allocate(parcelset::endpoints_type const& endpoints,
            std::uint64_t count, std::uint32_t num_threads,
            naming::gid_type suggested_prefix);

        parcelset::endpoints_type resolve_locality(
            naming::gid_type const& locality);

        void free(naming::gid_type const& locality);

        std::vector<std::uint32_t> localities();

        std::uint32_t get_num_localities();

        std::vector<std::uint32_t> get_num_threads();

        std::uint32_t get_num_overall_threads();

    public:
        HPX_DEFINE_COMPONENT_ACTION(locality_namespace, allocate)
        HPX_DEFINE_COMPONENT_ACTION(locality_namespace, free)
        HPX_DEFINE_COMPONENT_ACTION(locality_namespace, localities)
        HPX_DEFINE_COMPONENT_ACTION(locality_namespace, resolve_locality)
        HPX_DEFINE_COMPONENT_ACTION(locality_namespace, get_num_localities)
        HPX_DEFINE_COMPONENT_ACTION(locality_namespace, get_num_threads)
        HPX_DEFINE_COMPONENT_ACTION(locality_namespace, get_num_overall_threads)
    };
}    // namespace hpx::agas::server

HPX_ACTION_USES_MEDIUM_STACK(
    hpx::agas::server::locality_namespace::allocate_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::locality_namespace::allocate_action,
    locality_namespace_allocate_action)

HPX_ACTION_USES_MEDIUM_STACK(hpx::agas::server::locality_namespace::free_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::locality_namespace::free_action,
    locality_namespace_allocate_action)

HPX_ACTION_USES_MEDIUM_STACK(
    hpx::agas::server::locality_namespace::localities_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::locality_namespace::localities_action,
    locality_namespace_localities_action)

HPX_ACTION_USES_MEDIUM_STACK(
    hpx::agas::server::locality_namespace::resolve_locality_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::locality_namespace::resolve_locality_action,
    locality_namespace_resolve_locality_action)

HPX_ACTION_USES_MEDIUM_STACK(
    hpx::agas::server::locality_namespace::get_num_localities_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::locality_namespace::get_num_localities_action,
    locality_namespace_get_num_localities_action)

HPX_ACTION_USES_MEDIUM_STACK(
    hpx::agas::server::locality_namespace::get_num_threads_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::locality_namespace::get_num_threads_action,
    locality_namespace_get_num_threads_action)

HPX_ACTION_USES_MEDIUM_STACK(
    hpx::agas::server::locality_namespace::get_num_overall_threads_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::locality_namespace::get_num_overall_threads_action,
    locality_namespace_get_num_overall_threads_action)

#include <hpx/config/warnings_suffix.hpp>
