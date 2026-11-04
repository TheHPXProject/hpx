//  Copyright (c) 2007-2018 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file runtime_local_fwd.hpp
/// \page hpx::register_thread, hpx::unregister_thread, hpx::get_os_thread_data, hpx::enumerate_os_threads, hpx::get_runtime_instance_number, hpx::register_on_exit, hpx::is_starting, hpx::tolerate_node_faults, hpx::is_running, hpx::is_stopped, hpx::is_stopped_or_shutting_down, hpx::get_num_worker_threads, hpx::get_system_uptime
/// \headerfile hpx/runtime.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/io_service.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/threadmanager.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/runtime_local/config_entry.hpp>
#include <hpx/runtime_local/detail/runtime_local_fwd.hpp>
#include <hpx/runtime_local/get_locality_id.hpp>
#include <hpx/runtime_local/get_num_all_localities.hpp>
#include <hpx/runtime_local/get_os_thread_count.hpp>
#include <hpx/runtime_local/get_thread_name.hpp>
#include <hpx/runtime_local/get_worker_thread_num.hpp>
#include <hpx/runtime_local/os_thread_type.hpp>
#include <hpx/runtime_local/report_error.hpp>
#include <hpx/runtime_local/shutdown_function.hpp>
#include <hpx/runtime_local/startup_function.hpp>
#include <hpx/runtime_local/thread_hooks.hpp>
#include <hpx/runtime_local/thread_pool_helpers.hpp>

#include <cstddef>
#include <cstdint>
#include <string>

namespace hpx {

    /// Register the current kernel thread with HPX, this should be done once
    /// for each external OS-thread intended to invoke HPX functionality.
    /// Calling this function more than once will return false.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool register_thread(
        runtime* rt, char const* name, error_code& ec = throws);

    /// Unregister the thread from HPX, this should be done once in
    /// the end before the external thread exists.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void unregister_thread(runtime* rt);

    /// Access data for a given OS thread that was previously registered by
    /// \a register_thread. This function must be called from a thread that was
    /// previously registered with the runtime.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT runtime_local::os_thread_data
    get_os_thread_data(std::string const& label);

    /// Enumerate all OS threads that have registered with the runtime.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool enumerate_os_threads(
        hpx::function<bool(os_thread_data const&)> const& f);

    /// Return the runtime instance number associated with the runtime instance
    /// the current thread is running in.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::size_t
    get_runtime_instance_number();

    /// Register a function to be called during system shutdown
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool register_on_exit(
        hpx::function<void()> const&);

    /// \cond NOINTERNAL
    namespace util {

        /// \brief Expand INI variables in a string
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::string expand(
            std::string const& expand);

        /// \brief Expand INI variables in a string
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void expand(std::string& expand);
    }    // namespace util

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Return whether NUMA-sensitive scheduling is configured.
    ///
    /// This returns true when the configured \c hpx.numa_sensitive level is
    /// nonzero.
    ///
    /// \throws hpx::exception with \a hpx::error::invalid_status if the runtime
    /// has not finished initialization.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool is_scheduler_numa_sensitive();

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT hpx::util::runtime_configuration const&
    get_config();

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT hpx::util::io_service_pool*
    get_thread_pool(char const* name, char const* pool_name_suffix = "");
    /// \endcond

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Test whether the runtime system is currently being started.
    ///
    /// This function returns whether the runtime system is currently being
    /// started or not, e.g. whether the current state of the runtime system is
    /// \a hpx::state::startup
    ///
    /// \note   This function needs to be executed on a HPX-thread. It will
    ///         return false otherwise.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool is_starting();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Test if HPX runs in fault-tolerant mode
    ///
    /// This function returns whether the runtime system is running
    /// in fault-tolerant mode
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool tolerate_node_faults();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Test whether the runtime system is currently running.
    ///
    /// This function returns whether the runtime system is currently running
    /// or not, e.g.  whether the current state of the runtime system is
    /// \a hpx::state::running
    ///
    /// \note   This function needs to be executed on a HPX-thread. It will
    ///         return false otherwise.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool is_running();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Test whether the runtime system is currently stopped.
    ///
    /// This function returns whether the runtime system is currently stopped
    /// or not, e.g.  whether the current state of the runtime system is
    /// \a hpx::state::stopped
    ///
    /// \note   This function needs to be executed on a HPX-thread. It will
    ///         return false otherwise.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool is_stopped();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Test whether the runtime system is currently being shut down.
    ///
    /// This function returns whether the runtime system is currently being
    /// shut down or not, e.g.  whether the current state of the runtime system
    /// is \a hpx::state::stopped or \a hpx::state::shutdown
    ///
    /// \note   This function needs to be executed on a HPX-thread. It will
    ///         return false otherwise.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT bool is_stopped_or_shutting_down();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Return the number of worker OS- threads used to execute HPX
    ///        threads
    ///
    /// This function returns the number of OS-threads used to execute HPX
    /// threads. If the function is called while no HPX runtime system is active,
    /// it will return zero.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::size_t get_num_worker_threads();

    ///////////////////////////////////////////////////////////////////////////
    /// \brief Return the system uptime measure on the thread executing this call.
    ///
    /// This function returns the system uptime measured in nanoseconds for the
    /// thread executing this call. If the function is called while no HPX
    /// runtime system is active, it will return zero.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT std::uint64_t get_system_uptime();

    namespace threads {

        /// \cond NOINTERNAL
        // The function get_thread_manager returns a reference to the
        // current thread manager.
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT threadmanager& get_thread_manager();
        /// \endcond

        /// \cond NOINTERNAL
        /// Reset internal (round-robin ) thread distribution scheme
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void reset_thread_distribution();

        /// Set the new scheduler mode
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void set_scheduler_mode(
            threads::policies::scheduler_mode new_mode);
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void set_scheduler_mode(
            threads::policies::scheduler_mode new_mode,
            hpx::threads::mask_cref_type pu_mask);

        /// Add the given flags to the scheduler mode
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void add_scheduler_mode(
            threads::policies::scheduler_mode to_add);
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void add_scheduler_mode(
            threads::policies::scheduler_mode to_add,
            hpx::threads::mask_cref_type pu_mask);

        /// Add/remove the given flags to the scheduler mode
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void add_remove_scheduler_mode(
            threads::policies::scheduler_mode to_add,
            threads::policies::scheduler_mode to_remove);
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void add_remove_scheduler_mode(
            threads::policies::scheduler_mode to_add,
            threads::policies::scheduler_mode to_remove,
            hpx::threads::mask_cref_type pu_mask);

        /// Remove the given flags from the scheduler mode
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void remove_scheduler_mode(
            threads::policies::scheduler_mode to_remove);
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void remove_scheduler_mode(
            threads::policies::scheduler_mode to_remove,
            hpx::threads::mask_cref_type pu_mask);
        /// \endcond
    }    // namespace threads

    namespace detail {

        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void on_exit() noexcept;
        HPX_CXX_CORE_EXPORT [[noreturn]] HPX_CORE_EXPORT void on_abort(
            int signal) noexcept;
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void handle_print_bind(
            std::size_t num_threads);
    }    // namespace detail
}    // namespace hpx
