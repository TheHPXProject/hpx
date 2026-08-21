//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/components_base/agas_interface.hpp>
#include <hpx/components_base/detail/agas_interface_functions.hpp>
#include <hpx/components_base/pinned_ptr.hpp>
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

namespace hpx::agas::detail {

    ///////////////////////////////////////////////////////////////////////////
    bool (*is_console)() = nullptr;
    bool (*is_connecting)(naming::gid_type const&) = nullptr;

    ///////////////////////////////////////////////////////////////////////////
    bool (*register_name)(std::string const& name, naming::gid_type const& gid,
        error_code& ec) = nullptr;

    bool (*register_name_id)(std::string const& name, hpx::id_type const& id,
        error_code& ec) = nullptr;

    future<bool> (*register_name_async)(
        std::string const& name, hpx::id_type const& id) = nullptr;

    ///////////////////////////////////////////////////////////////////////////
    hpx::id_type (*unregister_name)(
        std::string const& name, error_code& ec) = nullptr;

    future<hpx::id_type> (*unregister_name_async)(
        std::string const& name) = nullptr;

    ///////////////////////////////////////////////////////////////////////////
    hpx::id_type (*resolve_name)(
        std::string const& name, error_code& ec) = nullptr;

    future<hpx::id_type> (*resolve_name_async)(
        std::string const& name) = nullptr;

    ///////////////////////////////////////////////////////////////////////////
    future<std::uint32_t> (*get_num_localities_async)(
        naming::component_type type) = nullptr;

    std::uint32_t (*get_num_localities)(
        naming::component_type type, error_code& ec) = nullptr;

    ///////////////////////////////////////////////////////////////////////////
    std::string (*get_component_type_name)(
        naming::component_type type, error_code& ec) = nullptr;

    ///////////////////////////////////////////////////////////////////////////
    future<std::vector<std::uint32_t>> (*get_num_threads_async)() = nullptr;

    std::vector<std::uint32_t> (*get_num_threads)(error_code& ec) = nullptr;

    ///////////////////////////////////////////////////////////////////////////
    future<std::uint32_t> (*get_num_overall_threads_async)() = nullptr;

    std::uint32_t (*get_num_overall_threads)(error_code& ec) = nullptr;

    ///////////////////////////////////////////////////////////////////////////
    std::uint32_t (*get_locality_id)(error_code& ec) = nullptr;

    std::vector<std::uint32_t> (*get_all_locality_ids)(
        naming::component_type type, error_code& ec) = nullptr;

    ///////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_NETWORKING)
    parcelset::endpoints_type const& (*resolve_locality)(
        naming::gid_type const& gid, error_code& ec) = nullptr;

    void (*remove_resolved_locality)(naming::gid_type const& gid) = nullptr;
#endif

    ///////////////////////////////////////////////////////////////////////////
    bool (*is_local_address_cached)(
        naming::gid_type const& gid, error_code& ec) = nullptr;

    bool (*is_local_address_cached_addr)(naming::gid_type const& gid,
        naming::address& addr, error_code& ec) = nullptr;

    bool (*is_local_address_cached_addr_pinned_ptr)(naming::gid_type const& gid,
        naming::address& addr, std::pair<bool, components::pinned_ptr>& r,
        hpx::move_only_function<std::pair<bool, components::pinned_ptr>(
            naming::address const&)>&& f,
        error_code& ec) = nullptr;

    void (*update_cache_entry)(naming::gid_type const& gid,
        naming::address const& addr, std::uint64_t count, std::uint64_t offset,
        error_code& ec) = nullptr;

    ///////////////////////////////////////////////////////////////////////////
    bool (*is_local_lva_encoded_address)(naming::gid_type const& gid) = nullptr;

    ///////////////////////////////////////////////////////////////////////////
    hpx::future_or_value<naming::address> (*resolve_async)(
        hpx::id_type const& id) = nullptr;

    naming::address (*resolve)(
        hpx::id_type const& id, error_code& ec) = nullptr;

    bool (*resolve_local)(naming::gid_type const& gid, naming::address& addr,
        error_code& ec) = nullptr;

    bool (*resolve_cached)(
        naming::gid_type const& gid, naming::address& addr) = nullptr;

    ///////////////////////////////////////////////////////////////////////////
    hpx::future<bool> (*bind_async)(naming::gid_type const& gid,
        naming::address const& addr, std::uint32_t locality_id) = nullptr;

    bool (*bind)(naming::gid_type const& gid, naming::address const& addr,
        std::uint32_t locality_id, error_code& ec) = nullptr;

    hpx::future<bool> (*bind_async_locality)(naming::gid_type const& gid,
        naming::address const& addr,
        naming::gid_type const& locality_) = nullptr;

    bool (*bind_locality)(naming::gid_type const& gid,
        naming::address const& addr, naming::gid_type const& locality_,
        error_code& ec) = nullptr;

    hpx::future<naming::address> (*unbind_async)(
        naming::gid_type const& gid, std::uint64_t count) = nullptr;

    naming::address (*unbind)(naming::gid_type const& gid, std::uint64_t count,
        error_code& ec) = nullptr;

    ///////////////////////////////////////////////////////////////////////////
    bool (*bind_gid_local)(naming::gid_type const& gid,
        naming::address const& addr, error_code& ec) = nullptr;
    void (*unbind_gid_local)(
        naming::gid_type const& gid, error_code& ec) = nullptr;

    bool (*bind_range_local)(naming::gid_type const& gid, std::size_t count,
        naming::address const& addr, std::size_t offset,
        error_code& ec) = nullptr;
    void (*unbind_range_local)(naming::gid_type const& gid, std::size_t count,
        error_code& ec) = nullptr;

    ///////////////////////////////////////////////////////////////////////////
    void (*garbage_collect_non_blocking)(error_code& ec) = nullptr;

    void (*garbage_collect)(error_code& ec) = nullptr;

    void (*garbage_collect_non_blocking_id)(
        hpx::id_type const& id, error_code& ec) = nullptr;

    void (*garbage_collect_id)(
        hpx::id_type const& id, error_code& ec) = nullptr;

    ///////////////////////////////////////////////////////////////////////////
    hpx::id_type (*get_console_locality)(error_code& ec) = nullptr;

    ///////////////////////////////////////////////////////////////////////////
    naming::gid_type (*get_next_id)(
        std::size_t count, error_code& ec) = nullptr;

    ///////////////////////////////////////////////////////////////////////////
    void (*decref)(naming::gid_type const& id, std::int64_t credits,
        error_code& ec) = nullptr;

    ///////////////////////////////////////////////////////////////////////////
    hpx::future_or_value<std::int64_t> (*incref_async)(
        naming::gid_type const& gid, std::int64_t credits,
        hpx::id_type const& keep_alive) = nullptr;

    ///////////////////////////////////////////////////////////////////////////
    std::int64_t (*replenish_credits)(naming::gid_type& gid) = nullptr;

    ///////////////////////////////////////////////////////////////////////////
    hpx::future_or_value<id_type> (*get_colocation_id_async)(
        hpx::id_type const& id) = nullptr;

    hpx::id_type (*get_colocation_id)(
        hpx::id_type const& id, error_code& ec) = nullptr;

    ///////////////////////////////////////////////////////////////////////////
    hpx::future<hpx::id_type> (*on_symbol_namespace_event)(
        std::string const& name, bool call_for_past_events) = nullptr;

    ///////////////////////////////////////////////////////////////////////////
    hpx::future<std::pair<hpx::id_type, naming::address>> (*begin_migration)(
        hpx::id_type const& id) = nullptr;

    bool (*end_migration)(hpx::id_type const& id) = nullptr;

    hpx::future<void> (*mark_as_migrated)(naming::gid_type const& gid,
        hpx::move_only_function<std::pair<bool, hpx::future<void>>()>&& f,
        bool expect_to_be_marked_as_migrating) = nullptr;

    std::pair<bool, components::pinned_ptr> (*was_object_migrated)(
        naming::gid_type const& gid,
        hpx::move_only_function<components::pinned_ptr()>&& f) = nullptr;

    void (*unmark_as_migrated)(naming::gid_type const& gid,
        hpx::move_only_function<void()>&& f) = nullptr;

    ///////////////////////////////////////////////////////////////////////////
    hpx::future<std::map<std::string, hpx::id_type>> (*find_symbols_async)(
        std::string const& pattern) = nullptr;

    std::map<std::string, hpx::id_type> (*find_symbols)(
        std::string const& pattern) = nullptr;

    ///////////////////////////////////////////////////////////////////////////
    naming::component_type (*register_factory)(std::uint32_t prefix,
        std::string const& name, error_code& ec) = nullptr;

    naming::component_type (*get_component_id)(
        std::string const& name, error_code& ec) = nullptr;

    ///////////////////////////////////////////////////////////////////////////
    void (*destroy_component)(
        naming::gid_type const& gid, naming::address const& addr) = nullptr;

    ///////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_NETWORKING)
    void (*route)(parcelset::parcel&& p,
        hpx::function<void(std::error_code const&, parcelset::parcel const&)>&&,
        threads::thread_priority local_priority) = nullptr;
#endif

    ///////////////////////////////////////////////////////////////////////////
    naming::address_type (*get_primary_ns_lva)() = nullptr;
    naming::address_type (*get_symbol_ns_lva)() = nullptr;
    naming::address_type (*get_runtime_support_lva)() = nullptr;
}    // namespace hpx::agas::detail
