//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//  Copyright (c) 2007-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/components_base/agas_interface.hpp>
#include <hpx/components_base/detail/agas_interface_functions.hpp>
#include <hpx/components_base/pinned_ptr.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/parcelset_base.hpp>

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace hpx::agas {

    ///////////////////////////////////////////////////////////////////////////
    bool is_console()
    {
        return detail::is_console();
    }

    bool is_connecting(naming::gid_type const& locality)
    {
        return detail::is_connecting(locality);
    }

    ///////////////////////////////////////////////////////////////////////////
    bool register_name(launch::sync_policy, std::string const& name,
        naming::gid_type const& gid, error_code& ec)
    {
        return detail::register_name(name, gid, ec);
    }

    bool register_name(launch::sync_policy, std::string const& name,
        hpx::id_type const& id, error_code& ec)
    {
        return detail::register_name_id(name, id, ec);
    }

    hpx::future<bool> register_name(
        std::string const& name, hpx::id_type const& id)
    {
        return detail::register_name_async(name, id);
    }

    ///////////////////////////////////////////////////////////////////////////
    hpx::id_type unregister_name(
        launch::sync_policy, std::string const& name, error_code& ec)
    {
        return detail::unregister_name(name, ec);
    }

    hpx::future<hpx::id_type> unregister_name(std::string const& name)
    {
        return detail::unregister_name_async(name);
    }

    ///////////////////////////////////////////////////////////////////////////
    hpx::future<hpx::id_type> resolve_name(std::string const& name)
    {
        return detail::resolve_name_async(name);
    }

    hpx::id_type resolve_name(
        launch::sync_policy, std::string const& name, error_code& ec)
    {
        return detail::resolve_name(name, ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    hpx::future<std::uint32_t> get_num_localities(naming::component_type type)
    {
        return detail::get_num_localities_async(type);
    }

    std::uint32_t get_num_localities(
        launch::sync_policy, naming::component_type type, error_code& ec)
    {
        return detail::get_num_localities(type, ec);
    }

    hpx::future<std::vector<std::uint32_t>> get_num_threads()
    {
        return detail::get_num_threads_async();
    }

    std::vector<std::uint32_t> get_num_threads(
        launch::sync_policy, error_code& ec)
    {
        return detail::get_num_threads(ec);
    }

    hpx::future<std::uint32_t> get_num_overall_threads()
    {
        return detail::get_num_overall_threads_async();
    }

    std::uint32_t get_num_overall_threads(launch::sync_policy, error_code& ec)
    {
        return detail::get_num_overall_threads(ec);
    }

    std::string get_component_type_name(
        naming::component_type type, error_code& ec)
    {
        return detail::get_component_type_name(type, ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    bool is_local_address_cached(naming::gid_type const& gid, error_code& ec)
    {
        return detail::is_local_address_cached(gid, ec);
    }

    bool is_local_address_cached(
        naming::gid_type const& gid, naming::address& addr, error_code& ec)
    {
        return detail::is_local_address_cached_addr(gid, addr, ec);
    }

    bool is_local_address_cached(naming::gid_type const& gid,
        naming::address& addr, std::pair<bool, components::pinned_ptr>& r,
        hpx::move_only_function<std::pair<bool, components::pinned_ptr>(
            naming::address const&)>&& f,
        error_code& ec)
    {
        return detail::is_local_address_cached_addr_pinned_ptr(
            gid, addr, r, HPX_MOVE(f), ec);
    }

    void update_cache_entry(naming::gid_type const& gid,
        naming::address const& addr, std::uint64_t count, std::uint64_t offset,
        error_code& ec)
    {
        return detail::update_cache_entry(gid, addr, count, offset, ec);
    }

    bool is_local_lva_encoded_address(naming::gid_type const& gid)
    {
        return detail::is_local_lva_encoded_address(gid);
    }

    ///////////////////////////////////////////////////////////////////////////
    hpx::future_or_value<naming::address> resolve_async(hpx::id_type const& id)
    {
        return detail::resolve_async(id);
    }

    hpx::future<naming::address> resolve(hpx::id_type const& id)
    {
        auto result = detail::resolve_async(id);
        if (result.has_value())
        {
            return hpx::make_ready_future(HPX_MOVE(result).get_value());
        }

        auto f = HPX_MOVE(result).get_future();
        return f;
    }

    naming::address resolve(
        launch::sync_policy, hpx::id_type const& id, error_code& ec)
    {
        return detail::resolve(id, ec);
    }

    bool resolve_local(
        naming::gid_type const& gid, naming::address& addr, error_code& ec)
    {
        return detail::resolve_local(gid, addr, ec);
    }

    bool resolve_cached(naming::gid_type const& gid, naming::address& addr)
    {
        return detail::resolve_cached(gid, addr);
    }

    hpx::future<bool> bind(naming::gid_type const& gid,
        naming::address const& addr, std::uint32_t locality_id)
    {
        return detail::bind_async(gid, addr, locality_id);
    }

    bool bind(launch::sync_policy, naming::gid_type const& gid,
        naming::address const& addr, std::uint32_t locality_id, error_code& ec)
    {
        return detail::bind(gid, addr, locality_id, ec);
    }

    hpx::future<bool> bind(naming::gid_type const& gid,
        naming::address const& addr, naming::gid_type const& locality_)
    {
        return detail::bind_async_locality(gid, addr, locality_);
    }

    bool bind(launch::sync_policy, naming::gid_type const& gid,
        naming::address const& addr, naming::gid_type const& locality_,
        error_code& ec)
    {
        return detail::bind_locality(gid, addr, locality_, ec);
    }

    hpx::future<naming::address> unbind(
        naming::gid_type const& id, std::uint64_t t)
    {
        return detail::unbind_async(id, t);
    }

    naming::address unbind(launch::sync_policy, naming::gid_type const& id,
        std::uint64_t type, error_code& ec)
    {
        return detail::unbind(id, type, ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    bool bind_gid_local(naming::gid_type const& gid,
        naming::address const& addr, error_code& ec)
    {
        return detail::bind_gid_local(gid, addr, ec);
    }

    void unbind_gid_local(naming::gid_type const& gid, error_code& ec)
    {
        return detail::unbind_gid_local(gid, ec);
    }

    bool bind_range_local(naming::gid_type const& gid, std::size_t count,
        naming::address const& addr, std::size_t offset, error_code& ec)
    {
        return detail::bind_range_local(gid, count, addr, offset, ec);
    }

    void unbind_range_local(
        naming::gid_type const& gid, std::size_t count, error_code& ec)
    {
        return detail::unbind_range_local(gid, count, ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    void garbage_collect_non_blocking(error_code& ec)
    {
        detail::garbage_collect_non_blocking(ec);
    }

    void garbage_collect(error_code& ec)
    {
        detail::garbage_collect(ec);
    }

    /// \brief Invoke an asynchronous garbage collection step on the given target
    ///        locality.
    void garbage_collect_non_blocking(hpx::id_type const& id, error_code& ec)
    {
        detail::garbage_collect_non_blocking_id(id, ec);
    }

    /// \brief Invoke a synchronous garbage collection step on the given target
    ///        locality.
    void garbage_collect(hpx::id_type const& id, error_code& ec)
    {
        detail::garbage_collect_id(id, ec);
    }

    /// \brief Return an id_type referring to the console locality.
    hpx::id_type get_console_locality(error_code& ec)
    {
        return detail::get_console_locality(ec);
    }

    std::uint32_t get_locality_id(error_code& ec)
    {
        return detail::get_locality_id(ec);
    }

    std::vector<std::uint32_t> get_all_locality_ids(
        naming::component_type type, error_code& ec)
    {
        return detail::get_all_locality_ids(type, ec);
    }

    ///////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_NETWORKING)
    parcelset::endpoints_type const& resolve_locality(
        naming::gid_type const& gid, error_code& ec)
    {
        return detail::resolve_locality(gid, ec);
    }

    void remove_resolved_locality(naming::gid_type const& gid)
    {
        return detail::remove_resolved_locality(gid);
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    naming::gid_type get_next_id(std::size_t count, error_code& ec)
    {
        return detail::get_next_id(count, ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    void decref(
        naming::gid_type const& gid, std::int64_t credits, error_code& ec)
    {
        detail::decref(gid, credits, ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    hpx::future_or_value<std::int64_t> incref(naming::gid_type const& gid,
        std::int64_t credits, hpx::id_type const& keep_alive)
    {
        return detail::incref_async(gid, credits, keep_alive);
    }

    std::int64_t incref(launch::sync_policy, naming::gid_type const& gid,
        std::int64_t credits, hpx::id_type const& keep_alive, error_code& ec)
    {
        auto result = detail::incref_async(gid, credits, keep_alive);
        if (result.has_value())
        {
            return HPX_MOVE(result).get_value();
        }
        return result.get_future().get(ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    std::int64_t replenish_credits(naming::gid_type& gid)
    {
        return detail::replenish_credits(gid);
    }

    ///////////////////////////////////////////////////////////////////////////
    hpx::future_or_value<id_type> get_colocation_id(hpx::id_type const& id)
    {
        return detail::get_colocation_id_async(id);
    }

    hpx::id_type get_colocation_id(
        launch::sync_policy, hpx::id_type const& id, error_code& ec)
    {
        return detail::get_colocation_id(id, ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    hpx::future<hpx::id_type> on_symbol_namespace_event(
        std::string const& name, bool call_for_past_events)
    {
        return detail::on_symbol_namespace_event(name, call_for_past_events);
    }

    ///////////////////////////////////////////////////////////////////////////
    hpx::future<std::pair<hpx::id_type, naming::address>> begin_migration(
        hpx::id_type const& id)
    {
        return detail::begin_migration(id);
    }

    bool end_migration(hpx::id_type const& id)
    {
        return detail::end_migration(id);
    }

    hpx::future<void> mark_as_migrated(naming::gid_type const& gid,
        hpx::move_only_function<std::pair<bool, hpx::future<void>>()>&& f,
        bool expect_to_be_marked_as_migrating)
    {
        return detail::mark_as_migrated(
            gid, HPX_MOVE(f), expect_to_be_marked_as_migrating);
    }

    std::pair<bool, components::pinned_ptr> was_object_migrated(
        naming::gid_type const& gid,
        hpx::move_only_function<components::pinned_ptr()>&& f)
    {
        return detail::was_object_migrated(gid, HPX_MOVE(f));
    }

    void unmark_as_migrated(
        naming::gid_type const& gid, hpx::move_only_function<void()>&& f)
    {
        return detail::unmark_as_migrated(gid, HPX_MOVE(f));
    }

    hpx::future<std::map<std::string, hpx::id_type>> find_symbols(
        std::string const& pattern)
    {
        return detail::find_symbols_async(pattern);
    }

    std::map<std::string, hpx::id_type> find_symbols(
        hpx::launch::sync_policy, std::string const& pattern)
    {
        return detail::find_symbols(pattern);
    }

    ///////////////////////////////////////////////////////////////////////////
    naming::component_type register_factory(
        std::uint32_t prefix, std::string const& name, error_code& ec)
    {
        return detail::register_factory(prefix, name, ec);
    }

    naming::component_type get_component_id(
        std::string const& name, error_code& ec)
    {
        return detail::get_component_id(name, ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    void destroy_component(
        naming::gid_type const& gid, naming::address const& addr)
    {
        return detail::destroy_component(gid, addr);
    }

    ///////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_NETWORKING)
    void route(parcelset::parcel&& p,
        hpx::function<void(std::error_code const&, parcelset::parcel const&)>&&
            f,
        threads::thread_priority local_priority)
    {
        return detail::route(HPX_MOVE(p), HPX_MOVE(f), local_priority);
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    naming::address_type get_primary_ns_lva()
    {
        return detail::get_primary_ns_lva();
    }

    naming::address_type get_symbol_ns_lva()
    {
        return detail::get_symbol_ns_lva();
    }

    naming::address_type get_runtime_support_lva()
    {
        return detail::get_runtime_support_lva();
    }
}    // namespace hpx::agas
