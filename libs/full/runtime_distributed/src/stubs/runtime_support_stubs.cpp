//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_colocated.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/ini.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/performance_counters.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/runtime_distributed/applier.hpp>
#include <hpx/runtime_distributed/stubs/runtime_support.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>

namespace hpx::components::stubs {

    template <typename Policy>
    auto disable_run_as_child(Policy&& p)
    {
        auto policy = p;
        auto hint = policy.get_hint();
        hint.runs_as_child_mode(hpx::threads::thread_execution_hint::none);
        policy.set_hint(hint);
        return policy;
    }

    hpx::future<int> runtime_support::load_components_async(
        [[maybe_unused]] hpx::id_type const& gid)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        using action_type = server::runtime_support::load_components_action;
        return hpx::async<action_type>(
            disable_run_as_child(hpx::launch::async), gid);
#else
        HPX_ASSERT(false);
        return hpx::make_ready_future(0);
#endif
    }

    int runtime_support::load_components(hpx::id_type const& gid)
    {
        return load_components_async(gid).get();
    }

    hpx::future<void> runtime_support::call_startup_functions_async(
        [[maybe_unused]] hpx::id_type const& gid,
        [[maybe_unused]] bool pre_startup)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        using action_type =
            server::runtime_support::call_startup_functions_action;

        return hpx::async<action_type>(
            disable_run_as_child(hpx::launch::async), gid, pre_startup);
#else
        HPX_ASSERT(false);
        return ::hpx::make_ready_future();
#endif
    }

    void runtime_support::call_startup_functions(
        hpx::id_type const& gid, bool pre_startup)
    {
        call_startup_functions_async(gid, pre_startup).get();
    }

    /// \brief Shutdown the given runtime system
    hpx::future<void> runtime_support::shutdown_async(
        [[maybe_unused]] hpx::id_type const& targetgid,
        [[maybe_unused]] double timeout)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        // Create a promise directly and execute the required action.
        // This action has implemented special response handling as the
        // back-parcel is sent explicitly (and synchronously).
        using action_type = server::runtime_support::shutdown_action;

        hpx::distributed::promise<void> value;
        auto f = value.get_future();

        // We need to make it unmanaged to avoid late refcnt requests
        id_type gid(
            value.get_id().get_gid(), id_type::management_type::unmanaged);
        hpx::post<action_type>(targetgid, timeout, gid);

        return f;
#else
        HPX_ASSERT(false);
        return ::hpx::make_ready_future();
#endif
    }

    void runtime_support::shutdown(
        hpx::id_type const& targetgid, double timeout)
    {
        // The following get yields control while the action above
        // is executed and the result is returned to the future
        shutdown_async(targetgid, timeout).get();
    }

    /// \brief Shutdown the runtime systems of all localities
    void runtime_support::shutdown_all(
        [[maybe_unused]] hpx::id_type const& targetgid,
        [[maybe_unused]] double timeout)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        hpx::post<server::runtime_support::shutdown_all_action>(
            targetgid, timeout);
#else
        HPX_ASSERT(false);
#endif
    }

    void runtime_support::shutdown_all([[maybe_unused]] double timeout)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        hpx::post<server::runtime_support::shutdown_all_action>(
            hpx::id_type(
                hpx::applier::get_applier().get_runtime_support_raw_gid(),
                hpx::id_type::management_type::unmanaged),
            timeout);
#else
        HPX_ASSERT(false);
#endif
    }

    ///////////////////////////////////////////////////////////////////////
    /// \brief Retrieve configuration information
    /// \brief Terminate the given runtime system
    hpx::future<void> runtime_support::terminate_async(
        [[maybe_unused]] hpx::id_type const& targetgid)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        // Create a future directly and execute the required action.
        // This action has implemented special response handling as the
        // back-parcel is sent explicitly (and synchronously).
        using action_type = server::runtime_support::terminate_action;

        hpx::distributed::promise<void> value;
        auto f = value.get_future();

        hpx::post<action_type>(targetgid, value.get_id());
        return f;
#else
        HPX_ASSERT(false);
        return ::hpx::make_ready_future();
#endif
    }

    void runtime_support::terminate(hpx::id_type const& targetgid)
    {
        // The following get yields control while the action above
        // is executed and the result is returned to the future
        terminate_async(targetgid).get();
    }

    /// \brief Terminate the runtime systems of all localities
    void runtime_support::terminate_all(
        [[maybe_unused]] hpx::id_type const& targetgid)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        hpx::post<server::runtime_support::terminate_all_action>(targetgid);
#else
        HPX_ASSERT(false);
#endif
    }

    void runtime_support::terminate_all()
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        hpx::post<server::runtime_support::terminate_all_action>(hpx::id_type(
            hpx::applier::get_applier().get_runtime_support_raw_gid(),
            hpx::id_type::management_type::unmanaged));
#else
        HPX_ASSERT(false);
#endif
    }

    ///////////////////////////////////////////////////////////////////////
    void runtime_support::garbage_collect_non_blocking(
        [[maybe_unused]] hpx::id_type const& targetgid)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        using action_type = server::runtime_support::garbage_collect_action;
        hpx::post<action_type>(targetgid);
#else
        HPX_ASSERT(false);
#endif
    }

    hpx::future<void> runtime_support::garbage_collect_async(
        [[maybe_unused]] hpx::id_type const& targetgid)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        using action_type = server::runtime_support::garbage_collect_action;
        return hpx::async<action_type>(
            disable_run_as_child(hpx::launch::async), targetgid);
#else
        HPX_ASSERT(false);
        return ::hpx::make_ready_future();
#endif
    }

    void runtime_support::garbage_collect(
        [[maybe_unused]] hpx::id_type const& targetgid)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        using action_type = server::runtime_support::garbage_collect_action;
        hpx::async<action_type>(
            disable_run_as_child(hpx::launch::async), targetgid)
            .get();
#else
        HPX_ASSERT(false);
#endif
    }

    ///////////////////////////////////////////////////////////////////////
    hpx::future<hpx::id_type> runtime_support::create_performance_counter_async(
        [[maybe_unused]] hpx::id_type const& targetgid,
        [[maybe_unused]] performance_counters::counter_info const& info)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        if (!naming::is_locality(targetgid))
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "stubs::runtime_support::create_performance_counter_async",
                "The id passed as the first argument is not representing"
                " a locality");
        }

        using action_type =
            server::runtime_support::create_performance_counter_action;
        return hpx::async<action_type>(
            disable_run_as_child(hpx::launch::async), targetgid, info);
#else
        HPX_ASSERT(false);
        return ::hpx::make_ready_future(hpx::invalid_id);
#endif
    }

    hpx::id_type runtime_support::create_performance_counter(
        hpx::id_type const& targetgid,
        performance_counters::counter_info const& info, error_code& ec)
    {
        return create_performance_counter_async(targetgid, info).get(ec);
    }

    ///////////////////////////////////////////////////////////////////////
    /// \brief Retrieve configuration information
    hpx::future<util::section> runtime_support::get_config_async(
        [[maybe_unused]] hpx::id_type const& targetgid)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        // Create a future, execute the required action,
        // we simply return the initialized future, the caller needs
        // to call get() on the return value to obtain the result
        using action_type = server::runtime_support::get_config_action;
        return hpx::async<action_type>(
            disable_run_as_child(hpx::launch::async), targetgid);
#else
        HPX_ASSERT(false);
        return ::hpx::make_ready_future(util::section{});
#endif
    }

    void runtime_support::get_config(
        hpx::id_type const& targetgid, util::section& ini)
    {
        // The following get yields control while the action above
        // is executed and the result is returned to the future
        ini = get_config_async(targetgid).get();
    }

    ///////////////////////////////////////////////////////////////////////
    void runtime_support::remove_from_connection_cache_async(
        [[maybe_unused]] hpx::id_type const& target,
        [[maybe_unused]] naming::gid_type const& gid,
        [[maybe_unused]] parcelset::endpoints_type const& endpoints)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        using action_type =
            server::runtime_support::remove_from_connection_cache_action;
        hpx::post<action_type>(target, gid, endpoints);
#else
        HPX_ASSERT(false);
#endif
    }
}    // namespace hpx::components::stubs
