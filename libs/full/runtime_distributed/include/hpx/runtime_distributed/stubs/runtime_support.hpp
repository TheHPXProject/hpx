//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/async_colocated.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/ini.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/runtime_distributed/server/runtime_support.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::components::stubs {

    ///////////////////////////////////////////////////////////////////////////
    // The \a runtime_support class is the client side representation of a
    // \a server#runtime_support component
    HPX_CXX_EXPORT struct HPX_EXPORT runtime_support
    {
        ///////////////////////////////////////////////////////////////////////
        /// Create a new component \a type using the runtime_support with the
        /// given \a targetgid. This is a non-blocking call. The caller needs
        /// to call \a future#get on the result of this function
        /// to obtain the global id of the newly created object.
        template <typename Component, typename... Ts>
        static hpx::future<hpx::id_type> create_component_async(
            hpx::id_type const& gid, Ts&&... vs)
        {
            if (!naming::is_locality(gid))
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "stubs::runtime_support::create_component_async",
                    "The id passed as the first argument is not representing"
                    " a locality");
            }

            using action_type =
                server::create_component_action<Component, std::decay_t<Ts>...>;
            return hpx::async<action_type>(gid, HPX_FORWARD(Ts, vs)...);
        }

        /// Create a new component \a type using the runtime_support with the
        /// given \a targetgid. Block for the creation to finish.
        template <typename Component, typename... Ts>
        static hpx::id_type create_component(
            hpx::id_type const& gid, Ts&&... vs)
        {
            return create_component_async<Component>(
                gid, HPX_FORWARD(Ts, vs)...)
                .get();
        }

        /// Create multiple new components \a type using the runtime_support
        /// colocated with the given \a targetgid. This is a
        /// non-blocking call.
        template <bool WithCount, typename Component, typename... Ts>
        static hpx::future<std::vector<hpx::id_type>>
        bulk_create_component_colocated_async(
            hpx::id_type const& gid, std::size_t count, Ts&&... vs)
        {
            using action_type = server::bulk_create_component_action<WithCount,
                Component, std::decay_t<Ts>...>;

            return hpx::detail::async_colocated<action_type>(
                gid, count, HPX_FORWARD(Ts, vs)...);
        }

        /// Create multiple new components \a type using the runtime_support
        /// colocated with the given \a targetgid. Block for the
        /// creation to finish.
        template <bool WithCount, typename Component, typename... Ts>
        static std::vector<hpx::id_type> bulk_create_component_colocated(
            hpx::id_type const& gid, std::size_t count, Ts&&... vs)
        {
            return bulk_create_component_colocated_async<WithCount, Component>(
                gid, count, HPX_FORWARD(Ts, vs)...)
                .get();
        }

        /// Create multiple new components \a type using the runtime_support
        /// on the given locality. This is a  non-blocking call.
        template <bool WithCount, typename Component, typename... Ts>
        static hpx::future<std::vector<hpx::id_type>>
        bulk_create_component_async(
            hpx::id_type const& gid, std::size_t count, Ts&&... vs)
        {
            if (!naming::is_locality(gid))
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "stubs::runtime_support::bulk_create_component_async",
                    "The id passed as the first argument is not representing"
                    " a locality");
            }

            using action_type = server::bulk_create_component_action<WithCount,
                Component, std::decay_t<Ts>...>;
            return hpx::async<action_type>(gid, count, HPX_FORWARD(Ts, vs)...);
        }

        /// Create multiple new components \a type using the runtime_support
        /// on the given locality. Block for the creation to finish.
        template <bool WithCount, typename Component, typename... Ts>
        static std::vector<hpx::id_type> bulk_create_component(
            hpx::id_type const& gid, std::size_t count, Ts&&... vs)
        {
            return bulk_create_component_async<WithCount, Component>(
                gid, count, HPX_FORWARD(Ts, vs)...)
                .get();
        }

        /// Create a new component \a type using the runtime_support with the
        /// given \a targetgid. This is a non-blocking call. The caller needs
        /// to call \a future#get on the result of this function
        /// to obtain the global id of the newly created object.
        template <typename Component, typename... Ts>
        static hpx::future<hpx::id_type> create_component_colocated_async(
            hpx::id_type const& gid, Ts&&... vs)
        {
            using action_type =
                server::create_component_action<Component, std::decay_t<Ts>...>;
            return hpx::detail::async_colocated<action_type>(
                gid, HPX_FORWARD(Ts, vs)...);
        }

        /// Create a new component \a type using the runtime_support with the
        /// given \a targetgid. Block for the creation to finish.
        template <typename Component, typename... Ts>
        static hpx::id_type create_component_colocated(
            hpx::id_type const& gid, Ts&&... vs)
        {
            return create_component_colocated_async<Component>(
                gid, HPX_FORWARD(Ts, vs)...)
                .get();
        }

        ///////////////////////////////////////////////////////////////////////
        // copy construct a component
        template <typename Component>
        static hpx::future<hpx::id_type> copy_create_component_async(
            hpx::id_type const& gid, std::shared_ptr<Component> const& p,
            bool local_op)
        {
            if (!naming::is_locality(gid))
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "stubs::runtime_support::copy_create_component_async",
                    "The id passed as the first argument is not representing"
                    " a locality");
            }

            using action_type = server::copy_create_component_action<Component>;
            return hpx::async<action_type>(gid, p, local_op);
        }

        template <typename Component>
        static hpx::id_type copy_create_component(hpx::id_type const& gid,
            std::shared_ptr<Component> const& p, bool local_op)
        {
            // The following get yields control while the action above
            // is executed and the result is returned to the future
            return copy_create_component_async<Component>(gid, p, local_op)
                .get();
        }

        ///////////////////////////////////////////////////////////////////////
        // copy construct a component
        template <typename Component>
        static hpx::future<hpx::id_type> migrate_component_async(
            hpx::id_type const& target_locality,
            std::shared_ptr<Component> const& p, hpx::id_type const& to_migrate)
        {
            if (!naming::is_locality(target_locality))
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "stubs::runtime_support::migrate_component_async",
                    "The id passed as the first argument is not representing"
                    " a locality");
            }

            using action_type =
                server::migrate_component_here_action<Component>;
            return hpx::async<action_type>(target_locality, p, to_migrate);
        }

        template <typename Component, typename DistPolicy>
        static hpx::future<hpx::id_type> migrate_component_async(
            DistPolicy const& policy, std::shared_ptr<Component> const& p,
            hpx::id_type const& to_migrate)
        {
            using action_type =
                server::migrate_component_here_action<Component>;
            return hpx::async<action_type>(policy, p, to_migrate);
        }

        template <typename Component, typename Target>
        static hpx::id_type migrate_component(Target const& target,
            hpx::id_type const& to_migrate, std::shared_ptr<Component> const& p)
        {
            // The following get yields control while the action above
            // is executed and the result is returned to the future
            return migrate_component_async<Component>(target, p, to_migrate)
                .get();
        }

        ///////////////////////////////////////////////////////////////////////
        static hpx::future<int> load_components_async(hpx::id_type const& gid);
        static int load_components(hpx::id_type const& gid);

        static hpx::future<void> call_startup_functions_async(
            hpx::id_type const& gid, bool pre_startup);
        static void call_startup_functions(
            hpx::id_type const& gid, bool pre_startup);

        /// \brief Shutdown the given runtime system
        static hpx::future<void> shutdown_async(
            hpx::id_type const& targetgid, double timeout = -1);
        static void shutdown(
            hpx::id_type const& targetgid, double timeout = -1);

        /// \brief Shutdown the runtime systems of all localities
        static void shutdown_all(
            hpx::id_type const& targetgid, double timeout = -1);

        static void shutdown_all(double timeout = -1);

        ///////////////////////////////////////////////////////////////////////
        /// \brief Retrieve configuration information
        /// \brief Terminate the given runtime system
        static hpx::future<void> terminate_async(hpx::id_type const& targetgid);

        static void terminate(hpx::id_type const& targetgid);

        /// \brief Terminate the runtime systems of all localities
        static void terminate_all(hpx::id_type const& targetgid);

        static void terminate_all();

        ///////////////////////////////////////////////////////////////////////
        static void garbage_collect_non_blocking(hpx::id_type const& targetgid);

        static hpx::future<void> garbage_collect_async(
            hpx::id_type const& targetgid);

        static void garbage_collect(hpx::id_type const& targetgid);

        ///////////////////////////////////////////////////////////////////////
        static hpx::future<hpx::id_type> create_performance_counter_async(
            hpx::id_type const& targetgid,
            performance_counters::counter_info const& info);
        static hpx::id_type create_performance_counter(
            hpx::id_type const& targetgid,
            performance_counters::counter_info const& info,
            error_code& ec = throws);

        ///////////////////////////////////////////////////////////////////////
        /// \brief Retrieve configuration information
        static hpx::future<util::section> get_config_async(
            hpx::id_type const& targetgid);
        static void get_config(
            hpx::id_type const& targetgid, util::section& ini);

        ///////////////////////////////////////////////////////////////////////
        static void remove_from_connection_cache_async(
            hpx::id_type const& target, naming::gid_type const& gid,
            parcelset::endpoints_type const& endpoints);
    };
}    // namespace hpx::components::stubs
