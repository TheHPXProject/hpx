//  Copyright (c)      2017 Shoshana Jakobovits
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/ini.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/resource_partitioner/detail/create_partitioner.hpp>
#include <hpx/resource_partitioner/detail/partitioner.hpp>
#include <hpx/resource_partitioner/partitioner_fwd.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace hpx::resource {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT class pu
    {
        static constexpr std::size_t const invalid_pu_id =
            static_cast<std::size_t>(-1);

    public:
        explicit constexpr pu(std::size_t const id = invalid_pu_id,
            core* core = nullptr,
            std::size_t const thread_occupancy = 0) noexcept
          : id_(id)
          , core_(core)
          , thread_occupancy_(thread_occupancy)
          , thread_occupancy_count_(0)
        {
        }

        constexpr std::size_t id() const noexcept
        {
            return id_;
        }

    private:
        friend class core;
        friend class numa_domain;
        friend class resource::detail::partitioner;

        std::vector<pu> pus_sharing_core();
        std::vector<pu> pus_sharing_numa_domain();

        std::size_t id_;
        core* core_;

        // indicates the number of threads that should run on this PU
        //  0: this PU is not exposed by the affinity bindings
        //  1: normal occupancy
        // >1: oversubscription
        std::size_t thread_occupancy_;

        // counts number of threads bound to this PU
        mutable std::size_t thread_occupancy_count_;
    };

    HPX_CXX_CORE_EXPORT class core
    {
        static constexpr std::size_t const invalid_core_id =
            static_cast<std::size_t>(-1);

    public:
        explicit core(std::size_t const id = invalid_core_id,
            numa_domain* domain = nullptr) noexcept
          : id_(id)
          , domain_(domain)
        {
        }

        constexpr std::vector<pu> const& pus() const noexcept
        {
            return pus_;
        }
        constexpr std::size_t id() const noexcept
        {
            return id_;
        }

    private:
        std::vector<core> cores_sharing_numa_domain();

        friend class pu;
        friend class numa_domain;
        friend class resource::detail::partitioner;

        std::size_t id_;
        numa_domain* domain_;
        std::vector<pu> pus_;
    };

    HPX_CXX_CORE_EXPORT class numa_domain
    {
        static constexpr std::size_t const invalid_numa_domain_id =
            static_cast<std::size_t>(-1);

    public:
        explicit numa_domain(
            std::size_t const id = invalid_numa_domain_id) noexcept
          : id_(id)
        {
        }

        constexpr std::vector<core> const& cores() const noexcept
        {
            return cores_;
        }
        constexpr std::size_t id() const noexcept
        {
            return id_;
        }

    private:
        friend class pu;
        friend class core;
        friend class resource::detail::partitioner;

        std::size_t id_;
        std::vector<core> cores_;
    };

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_CORE_EXPORT inline ::hpx::resource::partitioner
        make_partitioner(resource::partitioner_mode rpmode,
            hpx::util::section const& rtcfg,
            hpx::threads::policies::detail::affinity_data const& affinity_data);
    }

    HPX_CXX_CORE_EXPORT class partitioner
    {
    private:
        friend ::hpx::resource::partitioner detail::make_partitioner(
            resource::partitioner_mode rpmode, hpx::util::section const& rtcfg,
            hpx::threads::policies::detail::affinity_data const& affinity_data);

        partitioner(resource::partitioner_mode const rpmode,
            hpx::util::section const& rtcfg,
            hpx::threads::policies::detail::affinity_data const& affinity_data)
          : partitioner_(
                detail::create_partitioner(rpmode, rtcfg, affinity_data))
        {
        }

    public:
        ///////////////////////////////////////////////////////////////////////
        // Create one of the predefined thread pools
        HPX_CORE_EXPORT void create_thread_pool(std::string const& name,
            scheduling_policy sched = scheduling_policy::unspecified,
            hpx::threads::policies::scheduler_mode =
                hpx::threads::policies::scheduler_mode::default_,
            background_work_function func = background_work_function());

        // Create a custom thread pool with a callback function
        HPX_CORE_EXPORT void create_thread_pool(std::string const& name,
            scheduler_function scheduler_creation,
            background_work_function func = background_work_function());

        // allow the default pool to be renamed to something else
        HPX_CORE_EXPORT void set_default_pool_name(std::string const& name);

        HPX_CORE_EXPORT std::string const& get_default_pool_name() const;

        ///////////////////////////////////////////////////////////////////////
        // Functions to add processing units to thread pools via the
        // pu/core/numa_domain API
        void add_resource(hpx::resource::pu const& p,
            std::string const& pool_name, std::size_t const num_threads = 1)
        {
            add_resource(p, pool_name, true, num_threads);
        }

        HPX_CORE_EXPORT void add_resource(hpx::resource::pu const& p,
            std::string const& pool_name, bool exclusive,
            std::size_t num_threads = 1);
        HPX_CORE_EXPORT void add_resource(
            std::vector<hpx::resource::pu> const& pv,
            std::string const& pool_name, bool exclusive = true);
        HPX_CORE_EXPORT void add_resource(hpx::resource::core const& c,
            std::string const& pool_name, bool exclusive = true);
        HPX_CORE_EXPORT void add_resource(std::vector<hpx::resource::core>& cv,
            std::string const& pool_name, bool exclusive = true);
        HPX_CORE_EXPORT void add_resource(hpx::resource::numa_domain const& nd,
            std::string const& pool_name, bool exclusive = true);
        HPX_CORE_EXPORT void add_resource(
            std::vector<hpx::resource::numa_domain> const& ndv,
            std::string const& pool_name, bool exclusive = true);

        // Access all available NUMA domains
        HPX_CORE_EXPORT std::vector<numa_domain> const& numa_domains() const;

        // Returns the threads requested at startup --hpx:threads=cores
        // for example will return the number actually created
        HPX_CORE_EXPORT std::size_t get_number_requested_threads();

        // return the topology object managed by the internal partitioner
        HPX_CORE_EXPORT hpx::threads::topology const& get_topology() const;

        // Does initialization of all resources and internal data of the
        // resource partitioner called in hpx_init
        HPX_CORE_EXPORT void configure_pools();

        // Local runtime only: initialize first_core and pus_needed
        HPX_CORE_EXPORT void assign_cores(std::size_t first_core);

    private:
        detail::partitioner& partitioner_;
    };

    namespace detail {

        HPX_CXX_CORE_EXPORT ::hpx::resource::partitioner make_partitioner(
            resource::partitioner_mode const rpmode,
            hpx::util::section const& rtcfg,
            hpx::threads::policies::detail::affinity_data const& affinity_data)
        {
            return ::hpx::resource::partitioner(rpmode, rtcfg, affinity_data);
        }
    }    // namespace detail
}    // namespace hpx::resource
