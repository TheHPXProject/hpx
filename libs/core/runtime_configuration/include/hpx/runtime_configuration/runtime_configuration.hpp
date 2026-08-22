//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/filesystem.hpp>
#include <hpx/modules/ini.hpp>
#include <hpx/runtime_configuration/agas_service_mode.hpp>
#include <hpx/runtime_configuration/component_registry_base.hpp>
#include <hpx/runtime_configuration/plugin_registry_base.hpp>
#include <hpx/runtime_configuration/runtime_configuration_fwd.hpp>
#include <hpx/runtime_configuration/runtime_mode.hpp>
#include <hpx/runtime_configuration/static_factory_data.hpp>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::util {

    ///////////////////////////////////////////////////////////////////////////
    // The runtime_configuration class is a wrapper for the runtime
    // configuration data allowing to extract configuration information in a
    // more convenient way
    HPX_CXX_CORE_EXPORT class runtime_configuration : public section
    {
        std::string hpx_ini_file;
        std::vector<std::string> cmdline_ini_defs;
        std::vector<std::string> extra_static_ini_defs;

    public:
        // initialize and load configuration information
        runtime_configuration(char const* argv0, runtime_mode mode,
            std::vector<std::string> extra_static_ini_defs = {});

        // re-initialize all entries based on the additional information from
        // the given configuration file
        void reconfigure(std::string ini_file);

        // re-initialize all entries based on the additional information from
        // any explicit command line options
        void reconfigure(std::vector<std::string> ini_defs);

        std::vector<std::shared_ptr<plugins::plugin_registry_base>>
        load_modules(
            std::vector<std::shared_ptr<components::component_registry_base>>&
                component_registries);

        void load_components_static(
            std::vector<components::static_factory_load_data_type> const&
                static_modules);

        // Returns the AGAS mode of this locality, returns either hosted (for
        // localities connecting to a remote AGAS server) or bootstrap for the
        // locality hosting the AGAS server.
        agas::service_mode get_agas_service_mode() const;

        // initial number of localities
        std::uint32_t get_num_localities() const;
        void set_num_localities(std::uint32_t);

        // should networking be enabled
        bool enable_networking() const;

        // sequence number of first usable pu
        std::uint32_t get_first_used_core() const;
        void set_first_used_core(std::uint32_t);

        // Get the size of the ipc parcelport data buffer cache
        std::size_t get_ipc_data_buffer_cache_size() const;

        // Get AGAS client-side local cache size
        std::size_t get_agas_local_cache_size(
            std::size_t dflt = HPX_AGAS_LOCAL_CACHE_SIZE) const;

        bool get_agas_caching_mode() const;

        bool get_agas_range_caching_mode() const;

        // Get AGAS RPC timeout in milliseconds
        std::uint64_t get_agas_rpc_timeout(std::uint64_t dflt = 5000) const;

        std::size_t get_agas_max_pending_refcnt_requests() const;

        // Load application specific configuration and merge it with the
        // default configuration loaded from hpx.ini
        bool load_application_configuration(
            char const* filename, error_code& ec = throws);

        // Can be set to true if we want to use the ITT notify tools API.
        bool get_itt_notify_mode() const;

        // Enable lock detection during suspension
        bool enable_lock_detection() const;

        // Enable minimal deadlock detection for HPX threads
        bool enable_minimal_deadlock_detection() const;
        bool enable_spinlock_deadlock_detection() const;
        std::size_t get_spinlock_deadlock_detection_limit() const;

#if defined(__linux) || defined(linux) || defined(__linux__) ||                \
    defined(__FreeBSD__)
        bool use_stack_guard_pages() const;
#endif

        // return trace_depth for stack-backtraces
        std::size_t trace_depth() const;

        // Returns the number of OS threads this locality is running.
        std::size_t get_os_thread_count() const;

        // Returns the command line that this locality was invoked with.
        std::string get_cmd_line() const;

        // Will return the default stack size to use for all HPX-threads.
        std::ptrdiff_t get_default_stack_size() const
        {
            return small_stacksize;
        }

        // Will return the requested stack size to use for an HPX-threads.
        std::ptrdiff_t get_stack_size(
            threads::thread_stacksize stacksize) const;

        // Return the configured sizes of the known thread pools
        std::size_t get_thread_pool_size(char const* poolname) const;

        // Return the endianness to be used for out-serialization
        std::string get_endian_out() const;

        // Return maximally allowed message sizes
        std::uint64_t get_max_inbound_message_size(
            std::string const& type) const;
        std::uint64_t get_max_outbound_message_size(
            std::string const& type) const;

        std::map<std::string, hpx::util::plugin::dll>& modules()
        {
            return modules_;
        }

        /// \brief Check whether fault tolerance for node/locality faults is
        ///        enabled via the "enable_fault_tolerance" configuration
        ///        entry in the "hpx" section.
        ///
        /// \return true if fault tolerance is enabled, false otherwise.
        bool tolerate_node_faults();

        /// \brief Set whether this runtime instance should tolerate node
        ///        faults.
        ///
        /// This controls whether the runtime is configured to continue
        /// operating in the presence of locality failures (e.g. forced or
        /// unexpected disconnection of remote localities) rather than treating
        /// such failures as fatal errors.
        ///
        /// \param value  Set to `true` to enable fault tolerance for node
        ///               failures, or `false` to require all localities to
        ///               remain connected for normal operation.
        void tolerate_node_faults(bool const value) const
        {
            tolerate_node_faults_value = value;
            need_to_initialize_tolerate_node_faults = false;
        }

    private:
        std::ptrdiff_t init_stack_size(char const* entryname,
            char const* defaultvaluestr, std::ptrdiff_t defaultvalue) const;

        std::ptrdiff_t init_small_stack_size() const;
        std::ptrdiff_t init_medium_stack_size() const;
        std::ptrdiff_t init_large_stack_size() const;
        std::ptrdiff_t init_huge_stack_size() const;

        void pre_initialize_ini();
        void post_initialize_ini(std::string& hpx_ini_file,
            std::vector<std::string> const& cmdline_ini_defs);
        void pre_initialize_logging_ini();

        void reconfigure();

        void load_component_paths(
            std::vector<std::shared_ptr<plugins::plugin_registry_base>>&
                plugin_registries,
            std::vector<std::shared_ptr<components::component_registry_base>>&
                component_registries,
            std::string const& component_base_paths,
            std::string const& component_path_suffixes,
            std::set<std::string>& component_paths,
            std::map<std::string, filesystem::path>& basenames);

        void load_component_path(
            std::vector<std::shared_ptr<plugins::plugin_registry_base>>&
                plugin_registries,
            std::vector<std::shared_ptr<components::component_registry_base>>&
                component_registries,
            std::string const& path, std::set<std::string>& component_paths,
            std::map<std::string, filesystem::path>& basenames);

    public:
        runtime_mode mode_;

    private:
        mutable std::uint32_t num_localities;
        mutable std::uint32_t num_os_threads;
        std::ptrdiff_t small_stacksize;
        std::ptrdiff_t medium_stacksize;
        std::ptrdiff_t large_stacksize;
        std::ptrdiff_t huge_stacksize;
        bool need_to_call_pre_initialize;
        mutable bool need_to_initialize_tolerate_node_faults;
        mutable bool tolerate_node_faults_value;
#if defined(__linux) || defined(linux) || defined(__linux__)
        char const* argv0;
#endif

        std::map<std::string, hpx::util::plugin::dll> modules_;
    };
}    // namespace hpx::util

#include <hpx/config/warnings_suffix.hpp>
