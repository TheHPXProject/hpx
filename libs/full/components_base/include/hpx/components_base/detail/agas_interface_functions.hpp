//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/components_base/pinned_ptr.hpp>
#include <hpx/modules/coroutines.hpp>
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

namespace hpx::agas::detail {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT extern HPX_EXPORT bool (*is_console)();

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT extern HPX_EXPORT bool (*register_name)(
        std::string const& name, naming::gid_type const& gid, error_code& ec);

    HPX_CXX_EXPORT extern HPX_EXPORT bool (*register_name_id)(
        std::string const& name, hpx::id_type const& id, error_code& ec);

    HPX_CXX_EXPORT extern HPX_EXPORT future<bool> (*register_name_async)(
        std::string const& name, hpx::id_type const& id);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT extern HPX_EXPORT hpx::id_type (*unregister_name)(
        std::string const& name, error_code& ec);

    HPX_CXX_EXPORT extern HPX_EXPORT future<hpx::id_type> (
        *unregister_name_async)(std::string const& name);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT extern HPX_EXPORT hpx::id_type (*resolve_name)(
        std::string const& name, error_code& ec);

    HPX_CXX_EXPORT extern HPX_EXPORT future<hpx::id_type> (*resolve_name_async)(
        std::string const& name);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT extern HPX_EXPORT future<std::uint32_t> (
        *get_num_localities_async)(naming::component_type type);

    HPX_CXX_EXPORT extern HPX_EXPORT std::uint32_t (*get_num_localities)(
        naming::component_type type, error_code& ec);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT extern HPX_EXPORT std::string (*get_component_type_name)(
        naming::component_type type, error_code& ec);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT extern HPX_EXPORT future<std::vector<std::uint32_t>> (
        *get_num_threads_async)();

    HPX_CXX_EXPORT extern HPX_EXPORT std::vector<std::uint32_t> (
        *get_num_threads)(error_code& ec);

    HPX_CXX_EXPORT    ///////////////////////////////////////////////////////////////////////////
        extern HPX_EXPORT future<std::uint32_t> (
            *get_num_overall_threads_async)();

    HPX_CXX_EXPORT extern HPX_EXPORT std::uint32_t (*get_num_overall_threads)(
        error_code& ec);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT extern HPX_EXPORT std::uint32_t (*get_locality_id)(
        error_code& ec);

    HPX_CXX_EXPORT extern HPX_EXPORT std::vector<std::uint32_t> (
        *get_all_locality_ids)(naming::component_type type, error_code& ec);

    ///////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_NETWORKING)
    HPX_CXX_EXPORT extern HPX_EXPORT parcelset::endpoints_type const& (
        *resolve_locality)(naming::gid_type const& gid, error_code& ec);

    HPX_CXX_EXPORT extern HPX_EXPORT void (*remove_resolved_locality)(
        naming::gid_type const& gid);
#endif

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT extern HPX_EXPORT bool (*is_local_address_cached)(
        naming::gid_type const& gid, error_code& ec);

    HPX_CXX_EXPORT extern HPX_EXPORT bool (*is_local_address_cached_addr)(
        naming::gid_type const& gid, naming::address& addr, error_code& ec);

    HPX_CXX_EXPORT extern HPX_EXPORT bool (
        *is_local_address_cached_addr_pinned_ptr)(naming::gid_type const& gid,
        naming::address& addr, std::pair<bool, components::pinned_ptr>& r,
        hpx::move_only_function<std::pair<bool, components::pinned_ptr>(
            naming::address const&)>&& f,
        error_code& ec);

    HPX_CXX_EXPORT extern HPX_EXPORT void (*update_cache_entry)(
        naming::gid_type const& gid, naming::address const& addr,
        std::uint64_t count, std::uint64_t offset, error_code& ec);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT extern HPX_EXPORT bool (*is_local_lva_encoded_address)(
        naming::gid_type const& gid);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT extern HPX_EXPORT hpx::future_or_value<naming::address> (
        *resolve_async)(hpx::id_type const& id);

    HPX_CXX_EXPORT extern HPX_EXPORT naming::address (*resolve)(
        hpx::id_type const& id, error_code& ec);

    HPX_CXX_EXPORT extern HPX_EXPORT bool (*resolve_local)(
        naming::gid_type const& gid, naming::address& addr, error_code& ec);

    HPX_CXX_EXPORT extern HPX_EXPORT bool (*resolve_cached)(
        naming::gid_type const& gid, naming::address& addr);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT extern HPX_EXPORT hpx::future<bool> (*bind_async)(
        naming::gid_type const& gid, naming::address const& addr,
        std::uint32_t locality_id);

    HPX_CXX_EXPORT extern HPX_EXPORT bool (*bind)(naming::gid_type const& gid,
        naming::address const& addr, std::uint32_t locality_id, error_code& ec);

    HPX_CXX_EXPORT extern HPX_EXPORT hpx::future<bool> (*bind_async_locality)(
        naming::gid_type const& gid, naming::address const& addr,
        naming::gid_type const& locality_);

    HPX_CXX_EXPORT extern HPX_EXPORT bool (*bind_locality)(
        naming::gid_type const& gid, naming::address const& addr,
        naming::gid_type const& locality_, error_code& ec);

    HPX_CXX_EXPORT extern HPX_EXPORT hpx::future<naming::address> (
        *unbind_async)(naming::gid_type const& gid, std::uint64_t count);

    HPX_CXX_EXPORT extern HPX_EXPORT naming::address (*unbind)(
        naming::gid_type const& gid, std::uint64_t count, error_code& ec);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT extern HPX_EXPORT bool (*bind_gid_local)(
        naming::gid_type const& gid, naming::address const& addr,
        error_code& ec);
    HPX_CXX_EXPORT extern HPX_EXPORT void (*unbind_gid_local)(
        naming::gid_type const& gid, error_code& ec);

    HPX_CXX_EXPORT extern HPX_EXPORT bool (*bind_range_local)(
        naming::gid_type const& gid, std::size_t count,
        naming::address const& addr, std::size_t offset, error_code& ec);
    HPX_CXX_EXPORT extern HPX_EXPORT void (*unbind_range_local)(
        naming::gid_type const& gid, std::size_t count, error_code& ec);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT extern HPX_EXPORT void (*garbage_collect_non_blocking)(
        error_code& ec);

    HPX_CXX_EXPORT extern HPX_EXPORT void (*garbage_collect)(error_code& ec);

    HPX_CXX_EXPORT extern HPX_EXPORT void (*garbage_collect_non_blocking_id)(
        hpx::id_type const& id, error_code& ec);

    HPX_CXX_EXPORT extern HPX_EXPORT void (*garbage_collect_id)(
        hpx::id_type const& id, error_code& ec);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT extern HPX_EXPORT hpx::id_type (*get_console_locality)(
        error_code& ec);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT extern HPX_EXPORT naming::gid_type (*get_next_id)(
        std::size_t count, error_code& ec);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT extern HPX_EXPORT void (*decref)(
        naming::gid_type const& id, std::int64_t credits, error_code& ec);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT extern HPX_EXPORT hpx::future_or_value<std::int64_t> (
        *incref_async)(naming::gid_type const& gid, std::int64_t credits,
        hpx::id_type const& keep_alive);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT extern HPX_EXPORT std::int64_t (*replenish_credits)(
        naming::gid_type& gid);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT extern HPX_EXPORT hpx::future_or_value<id_type> (
        *get_colocation_id_async)(hpx::id_type const& id);

    HPX_CXX_EXPORT extern HPX_EXPORT hpx::id_type (*get_colocation_id)(
        hpx::id_type const& id, error_code& ec);

    HPX_CXX_EXPORT    ///////////////////////////////////////////////////////////////////////////
        extern HPX_EXPORT hpx::future<hpx::id_type> (
            *on_symbol_namespace_event)(
            std::string const& name, bool call_for_past_events);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT extern HPX_EXPORT
        hpx::future<std::pair<hpx::id_type, naming::address>> (
            *begin_migration)(hpx::id_type const& id);

    HPX_CXX_EXPORT extern HPX_EXPORT bool (*end_migration)(
        hpx::id_type const& id);

    HPX_CXX_EXPORT extern HPX_EXPORT hpx::future<void> (*mark_as_migrated)(
        naming::gid_type const& gid,
        hpx::move_only_function<std::pair<bool, hpx::future<void>>()>&& f,
        bool expect_to_be_marked_as_migrating);

    HPX_CXX_EXPORT extern HPX_EXPORT std::pair<bool, components::pinned_ptr> (
        *was_object_migrated)(naming::gid_type const& gid,
        hpx::move_only_function<components::pinned_ptr()>&& f);

    HPX_CXX_EXPORT extern HPX_EXPORT void (*unmark_as_migrated)(
        naming::gid_type const& gid, hpx::move_only_function<void()>&& f);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT extern HPX_EXPORT
        hpx::future<std::map<std::string, hpx::id_type>> (*find_symbols_async)(
            std::string const& pattern);

    HPX_CXX_EXPORT extern HPX_EXPORT std::map<std::string, hpx::id_type> (
        *find_symbols)(std::string const& pattern);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT extern HPX_EXPORT naming::component_type (*register_factory)(
        std::uint32_t prefix, std::string const& name, error_code& ec);

    HPX_CXX_EXPORT extern HPX_EXPORT naming::component_type (*get_component_id)(
        std::string const& name, error_code& ec);

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT extern HPX_EXPORT void (*destroy_component)(
        naming::gid_type const& gid, naming::address const& addr);

    ///////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_NETWORKING)
    HPX_CXX_EXPORT extern HPX_EXPORT void (*route)(parcelset::parcel&& p,
        hpx::function<void(std::error_code const&, parcelset::parcel const&)>&&,
        threads::thread_priority local_priority);
#endif

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT extern HPX_EXPORT naming::address_type (
        *get_primary_ns_lva)();
    HPX_CXX_EXPORT extern HPX_EXPORT naming::address_type (
        *get_symbol_ns_lva)();
    HPX_CXX_EXPORT extern HPX_EXPORT naming::address_type (
        *get_runtime_support_lva)();
}    // namespace hpx::agas::detail
