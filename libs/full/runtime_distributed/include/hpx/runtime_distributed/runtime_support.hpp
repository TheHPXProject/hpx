//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/runtime_distributed/applier.hpp>
#include <hpx/runtime_distributed/stubs/runtime_support.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace hpx::components {

    ///////////////////////////////////////////////////////////////////////////
    /// The \a runtime_support class is the client side representation of a
    /// \a server#runtime_support component
    HPX_CXX_EXPORT class HPX_EXPORT runtime_support
      : public stubs::runtime_support
    {
    private:
        using base_type = stubs::runtime_support;

    public:
        /// Create a client side representation for the existing
        /// \a server#runtime_support instance with the given global id \a gid.
        runtime_support(hpx::id_type const& gid = hpx::invalid_id)
          : gid_(hpx::invalid_id == gid ?
                    hpx::id_type(
                        applier::get_applier().get_runtime_support_raw_gid(),
                        hpx::id_type::management_type::unmanaged) :
                    gid)
        {
        }

        ///////////////////////////////////////////////////////////////////////
        // exposed functionality of this component

        /// Create a new component type using the runtime_support
        template <typename Component, typename... Ts>
        hpx::id_type create_component(Ts&&... vs)
        {
            return this->base_type::template create_component<Component>(
                gid_, HPX_FORWARD(Ts, vs)...);
        }

        /// Asynchronously create a new component using the runtime_support
        template <typename Component, typename... Ts>
        hpx::future<hpx::id_type> create_component_async(Ts&&... vs)
        {
            return this->base_type::template create_component_async<Component>(
                gid_, HPX_FORWARD(Ts, vs)...);
        }

        /// Asynchronously create N new default constructed components using
        /// the runtime_support
        template <bool WithCount, typename Component, typename... Ts>
        std::vector<hpx::id_type> bulk_create_component(
            std::size_t count, Ts&&... vs)
        {
            return this->base_type::bulk_create_component<WithCount, Component>(
                gid_, count, HPX_FORWARD(Ts, vs)...);
        }

        /// Asynchronously create a new component using the runtime_support
        template <bool WithCount, typename Component, typename... Ts>
        hpx::future<std::vector<hpx::id_type>> bulk_create_components_async(
            std::size_t count, Ts&&... vs)
        {
            return this->base_type::bulk_create_component<WithCount, Component>(
                gid_, count, HPX_FORWARD(Ts, vs)...);
        }

        ///////////////////////////////////////////////////////////////////////
        hpx::future<int> load_components_async() const
        {
            return this->base_type::load_components_async(gid_);
        }

        int load_components() const
        {
            return this->base_type::load_components(gid_);
        }

        hpx::future<void> call_startup_functions_async(bool pre_startup) const
        {
            return this->base_type::call_startup_functions_async(
                gid_, pre_startup);
        }

        void call_startup_functions(bool pre_startup) const
        {
            this->base_type::call_startup_functions(gid_, pre_startup);
        }

        /// \brief Shutdown the given runtime system
        hpx::future<void> shutdown_async(double timeout = -1) const
        {
            return this->base_type::shutdown_async(gid_, timeout);
        }

        void shutdown(double timeout = -1) const
        {
            this->base_type::shutdown(gid_, timeout);
        }

        /// \brief Shutdown the runtime systems of all localities
        void shutdown_all(double timeout = -1) const
        {
            this->base_type::shutdown_all(gid_, timeout);
        }

        /// \brief Terminate the given runtime system
        hpx::future<void> terminate_async() const
        {
            return this->base_type::terminate_async(gid_);
        }

        void terminate() const
        {
            this->base_type::terminate(gid_);
        }

        /// \brief Terminate the runtime systems of all localities
        void terminate_all() const
        {
            this->base_type::terminate_all(gid_);
        }

        /// \brief Retrieve configuration information
        void get_config(util::section& ini) const
        {
            this->base_type::get_config(gid_, ini);
        }

        ///////////////////////////////////////////////////////////////////////
        hpx::id_type const& get_id() const
        {
            return gid_;
        }

        naming::gid_type const& get_raw_gid() const
        {
            return gid_.get_gid();
        }

    private:
        hpx::id_type gid_;
    };
}    // namespace hpx::components

///////////////////////////////////////////////////////////////////////////////
// initialize runtime interface function wrappers
namespace hpx::agas {

    HPX_CXX_EXPORT struct runtime_components_init_interface_functions&
    runtime_components_init();
}

namespace hpx::components {

    HPX_CXX_EXPORT struct counter_interface_functions& counter_init();
}    // namespace hpx::components
