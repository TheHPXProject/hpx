//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/performance_counters.hpp>
#include <hpx/modules/runtime_components.hpp>
#include <hpx/runtime_distributed/runtime_support.hpp>

#include <mutex>
#include <utility>

HPX_PLUGIN_EXPORT_LIST(HPX_PLUGIN_COMPONENT_PREFIX, factory)
HPX_REGISTER_REGISTRY_MODULE()

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::components::server {

    void runtime_support::add_pre_startup_function(startup_function_type f)
    {
        if (!f.empty())
        {
            std::lock_guard<hpx::spinlock> l(globals_mtx_);
            pre_startup_functions_.push_back(HPX_MOVE(f));
        }
    }

    void runtime_support::add_startup_function(startup_function_type f)
    {
        if (!f.empty())
        {
            std::lock_guard<hpx::spinlock> l(globals_mtx_);
            startup_functions_.push_back(HPX_MOVE(f));
        }
    }

    void runtime_support::add_pre_shutdown_function(shutdown_function_type f)
    {
        if (!f.empty())
        {
            std::lock_guard<hpx::spinlock> l(globals_mtx_);
            pre_shutdown_functions_.push_back(HPX_MOVE(f));
        }
    }

    void runtime_support::add_shutdown_function(shutdown_function_type f)
    {
        if (!f.empty())
        {
            std::lock_guard<hpx::spinlock> l(globals_mtx_);
            shutdown_functions_.push_back(HPX_MOVE(f));
        }
    }
}    // namespace hpx::components::server

namespace hpx::agas::detail::impl {

    /// \brief Invoke an asynchronous garbage collection step on the given target
    ///        locality.
    void garbage_collect_non_blocking_id(hpx::id_type const& id, error_code& ec)
    {
        try
        {
            components::stubs::runtime_support::garbage_collect_non_blocking(
                id);
        }
        catch (hpx::exception const& e)
        {
            if (&ec == &throws)
                throw;
            ec = make_error_code(e.get_error(), e.what());
        }
    }

    /// \brief Invoke a synchronous garbage collection step on the given target
    ///        locality.
    void garbage_collect_id(hpx::id_type const& id, error_code& ec)
    {
        try
        {
            components::stubs::runtime_support::garbage_collect(id);
        }
        catch (hpx::exception const& e)
        {
            if (&ec == &throws)
                throw;
            ec = make_error_code(e.get_error(), e.what());
        }
    }
}    // namespace hpx::agas::detail::impl

namespace hpx::agas {

    // initialize AGAS interface function pointers in components_base module
    struct HPX_EXPORT runtime_components_init_interface_functions
    {
        runtime_components_init_interface_functions()
        {
            detail::garbage_collect_non_blocking_id =
                &detail::impl::garbage_collect_non_blocking_id;
            detail::garbage_collect_id = &detail::impl::garbage_collect_id;
        }
    };

    runtime_components_init_interface_functions& runtime_components_init()
    {
        static runtime_components_init_interface_functions
            runtime_components_init_;
        return runtime_components_init_;
    }
}    // namespace hpx::agas

namespace hpx::components {

    // some compilers try to invoke this function, even if it's actually not
    // needed
    namespace commandline_options_provider {

        hpx::program_options::options_description add_commandline_options()
        {
            return {};
        }
    }    // namespace commandline_options_provider

    // initialize AGAS interface function pointers in components_base module
    struct HPX_EXPORT counter_interface_functions
    {
        counter_interface_functions()
        {
            performance_counters::detail::create_performance_counter_async =
                &stubs::runtime_support::create_performance_counter_async;
        }
    };

    counter_interface_functions& counter_init()
    {
        static counter_interface_functions counter_init_;
        return counter_init_;
    }
}    // namespace hpx::components

#include <hpx/config/warnings_suffix.hpp>
