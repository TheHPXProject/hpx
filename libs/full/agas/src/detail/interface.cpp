//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/agas/addressing_service.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/runtime_local.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace hpx::agas::detail::impl {

    ///////////////////////////////////////////////////////////////////////////
    bool is_console()
    {
        return naming::get_agas_client().is_console();
    }

    ///////////////////////////////////////////////////////////////////////////
    bool register_name(
        std::string const& name, naming::gid_type const& gid, error_code&)
    {
        return naming::get_agas_client().register_name(name, gid);
    }

    future<bool> register_name_async(
        std::string const& name, hpx::id_type const& id)
    {
        return naming::get_agas_client().register_name_async(name, id);
    }

    bool register_name_id(
        std::string const& name, hpx::id_type const& id, error_code& ec)
    {
        if (&ec == &throws)
        {
            return naming::get_agas_client().register_name(name, id);
        }
        return register_name_async(name, id).get(ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    hpx::id_type unregister_name(std::string const& name, error_code&)
    {
        if (!hpx::is_stopped())
        {
            return naming::get_agas_client().unregister_name(name);
        }
        return hpx::invalid_id;
    }

    future<hpx::id_type> unregister_name_async(std::string const& name)
    {
        return naming::get_agas_client().unregister_name_async(name);
    }

    ///////////////////////////////////////////////////////////////////////////
    future<hpx::id_type> resolve_name_async(std::string const& name)
    {
        return naming::get_agas_client().resolve_name_async(name);
    }

    hpx::id_type resolve_name(std::string const& name, error_code& ec)
    {
        return naming::get_agas_client().resolve_name(name, ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    future<std::uint32_t> get_num_localities_async(components::component_type)
    {
        return naming::get_agas_client().get_num_localities_async();
    }

    std::uint32_t get_num_localities(
        components::component_type type, error_code& ec)
    {
        return naming::get_agas_client().get_num_localities(type, ec);
    }

    future<std::vector<std::uint32_t>> get_num_threads_async()
    {
        return naming::get_agas_client().get_num_threads_async();
    }

    std::vector<std::uint32_t> get_num_threads(error_code& ec)
    {
        return naming::get_agas_client().get_num_threads(ec);
    }

    future<std::uint32_t> get_num_overall_threads_async()
    {
        return naming::get_agas_client().get_num_overall_threads_async();
    }

    std::uint32_t get_num_overall_threads(error_code& ec)
    {
        return naming::get_agas_client().get_num_overall_threads(ec);
    }

    std::string get_component_type_name(
        components::component_type type, error_code& ec)
    {
        return naming::get_agas_client().get_component_type_name(type, ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    bool is_local_address_cached(naming::gid_type const& gid, error_code& ec)
    {
        return naming::get_agas_client().is_local_address_cached(gid, ec);
    }

    bool is_local_address_cached_addr(
        naming::gid_type const& gid, naming::address& addr, error_code& ec)
    {
        return naming::get_agas_client().is_local_address_cached(gid, addr, ec);
    }

    bool is_local_address_cached_addr_pinned_ptr(naming::gid_type const& gid,
        naming::address& addr, std::pair<bool, components::pinned_ptr>& r,
        hpx::move_only_function<std::pair<bool, components::pinned_ptr>(
            naming::address const&)>&& f,
        error_code& ec)
    {
        return naming::get_agas_client().is_local_address_cached(
            gid, addr, r, HPX_MOVE(f), ec);
    }

    void update_cache_entry(naming::gid_type const& gid,
        naming::address const& addr, std::uint64_t count, std::uint64_t offset,
        error_code& ec)
    {
        return naming::get_agas_client().update_cache_entry(
            gid, addr, count, offset, ec);
    }

    bool is_local_lva_encoded_address(naming::gid_type const& gid)
    {
        return naming::get_agas_client().is_local_lva_encoded_address(
            gid.get_msb());
    }

    ///////////////////////////////////////////////////////////////////////////
    hpx::future_or_value<naming::address> resolve_async(hpx::id_type const& id)
    {
        return naming::get_agas_client().resolve_async(id);
    }

    naming::address resolve(hpx::id_type const& id, error_code& ec)
    {
        auto result = naming::get_agas_client().resolve_async(id);
        if (result.has_value())
        {
            return HPX_MOVE(result).get_value();
        }
        return result.get_future().get(ec);
    }

    bool resolve_local(
        naming::gid_type const& gid, naming::address& addr, error_code& ec)
    {
        agas::addressing_service* agas_ = naming::get_agas_client_ptr();
        return (agas_ != nullptr) ? agas_->resolve_local(gid, addr, ec) : false;
    }

    bool resolve_cached(naming::gid_type const& gid, naming::address& addr)
    {
        agas::addressing_service* agas_ = naming::get_agas_client_ptr();
        return (agas_ != nullptr) ? agas_->resolve_cached(gid, addr) : false;
    }

    hpx::future<bool> bind_async(naming::gid_type const& gid,
        naming::address const& addr, std::uint32_t locality_id)
    {
        return naming::get_agas_client().bind_async(gid, addr, locality_id);
    }

    bool bind(naming::gid_type const& gid, naming::address const& addr,
        std::uint32_t locality_id, error_code& ec)
    {
        return naming::get_agas_client()
            .bind_async(gid, addr, locality_id)
            .get(ec);
    }

    hpx::future<bool> bind_async_locality(naming::gid_type const& gid,
        naming::address const& addr, naming::gid_type const& locality_)
    {
        return naming::get_agas_client().bind_async(gid, addr, locality_);
    }

    bool bind_locality(naming::gid_type const& gid, naming::address const& addr,
        naming::gid_type const& locality_, error_code& ec)
    {
        return naming::get_agas_client()
            .bind_async(gid, addr, locality_)
            .get(ec);
    }

    hpx::future<naming::address> unbind_async(
        naming::gid_type const& id, std::uint64_t)
    {
        return naming::get_agas_client().unbind_range_async(id);
    }

    naming::address unbind(
        naming::gid_type const& id, std::uint64_t, error_code& ec)
    {
        return naming::get_agas_client().unbind_range_async(id).get(ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    // helper functions allowing to bind and unbind a GID to a given address
    // without having to directly refer to the agas::addressing_service
    bool bind_gid_local(naming::gid_type const& gid_,
        naming::address const& addr, error_code& ec)
    {
        auto* client = naming::get_agas_client_ptr();
        if (nullptr == client)
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "agas::bind_gid_local", "addressing_service is not valid");
            return false;
        }
        return client->bind_local(gid_, addr, ec);
    }

    void unbind_gid_local(naming::gid_type const& gid, error_code& ec)
    {
        if (gid)
        {
            auto* client = naming::get_agas_client_ptr();
            if (nullptr == client)
            {
                HPX_THROWS_IF(ec, hpx::error::invalid_status,
                    "agas::unbind_gid_local",
                    "addressing_service is not valid");
            }
            else
            {
                client->unbind_local(gid, ec);
            }
        }
        else
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter, "agas::unbind_gid",
                "cannot dereference invalid GID");
        }
    }

    bool bind_range_local(naming::gid_type const& gid, std::size_t count,
        naming::address const& addr, std::size_t offset, error_code& ec)
    {
        auto* client = naming::get_agas_client_ptr();
        if (nullptr == client)
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "agas::bind_range_local", "addressing_service is not valid");
            return false;
        }
        return client->bind_range_local(gid, count, addr, offset, ec);
    }

    void unbind_range_local(
        naming::gid_type const& gid, std::size_t count, error_code& ec)
    {
        auto* client = naming::get_agas_client_ptr();
        if (nullptr == client)
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status,
                "agas::unbind_range_local", "addressing_service is not valid");
        }
        else
        {
            client->unbind_range_local(gid, count, ec);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    void garbage_collect_non_blocking(error_code& ec)
    {
        naming::get_agas_client().garbage_collect_non_blocking(ec);
    }

    void garbage_collect(error_code& ec)
    {
        naming::get_agas_client().garbage_collect(ec);
    }

    /// \brief Return an id_type referring to the console locality.
    hpx::id_type get_console_locality(error_code& ec)
    {
        runtime const* rt = get_runtime_ptr();
        if (rt == nullptr || rt->get_state() == state::invalid)
        {
            return hpx::invalid_id;
        }

        naming::gid_type console;
        naming::get_agas_client().get_console_locality(console, ec);
        if (ec)
        {
            return hpx::invalid_id;
        }

        return hpx::id_type(console, hpx::id_type::management_type::unmanaged);
    }

    std::uint32_t get_locality_id(error_code& ec)
    {
        runtime const* rt = get_runtime_ptr();
        if (rt == nullptr || rt->get_state() == state::invalid)
        {
            return naming::invalid_locality_id;
        }

        naming::gid_type const l =
            naming::get_agas_client().get_local_locality(ec);
        return l ? naming::get_locality_id_from_gid(l) :
                   naming::invalid_locality_id;
    }

    std::vector<std::uint32_t> get_all_locality_ids(
        naming::component_type type, error_code& ec)
    {
        std::vector<std::uint32_t> result;

        std::vector<naming::gid_type> localities;
        if (!naming::get_agas_client().get_localities(localities, type, ec))
        {
            return result;
        }

        result.reserve(localities.size());
        for (auto const& gid : localities)
        {
            result.push_back(naming::get_locality_id_from_gid(gid));
        }
        return result;
    }

    ///////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_NETWORKING)
    parcelset::endpoints_type const& resolve_locality(
        naming::gid_type const& gid, error_code& ec)
    {
        return naming::get_agas_client().resolve_locality(gid, ec);
    }

    void remove_resolved_locality(naming::gid_type const& gid)
    {
        naming::get_agas_client().remove_resolved_locality(gid);
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    naming::gid_type get_next_id(std::size_t count, error_code& ec)
    {
        runtime const* rt = get_runtime_ptr();
        if (rt == nullptr)
        {
            HPX_THROWS_IF(ec, hpx::error::invalid_status, "get_next_id",
                "the runtime system has not been started yet.");
            return naming::invalid_gid;
        }

        // during bootstrap, we use the id pool
        if (rt->get_state() == state::invalid)
        {
            return hpx::detail::get_next_id(count);
        }

        naming::gid_type lower_bound, upper_bound;
        naming::get_agas_client().get_id_range(
            count, lower_bound, upper_bound, ec);
        if (ec)
        {
            return naming::invalid_gid;
        }
        return lower_bound;
    }

    ///////////////////////////////////////////////////////////////////////////
    void decref(
        naming::gid_type const& gid, std::int64_t credits, error_code& ec)
    {
        naming::get_agas_client().decref(gid, credits, ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    hpx::future_or_value<std::int64_t> incref_async(naming::gid_type const& gid,
        std::int64_t credits, hpx::id_type const& keep_alive_)
    {
        HPX_ASSERT(!naming::detail::is_locked(gid));

        agas::addressing_service& resolver = naming::get_agas_client();
        if (keep_alive_)
            return resolver.incref_async(gid, credits, keep_alive_);

        hpx::id_type const keep_alive =
            hpx::id_type(gid, hpx::id_type::management_type::unmanaged);
        return resolver.incref_async(gid, credits, keep_alive);
    }

    ///////////////////////////////////////////////////////////////////////////
    hpx::future_or_value<id_type> get_colocation_id_async(
        hpx::id_type const& id)
    {
        return naming::get_agas_client().get_colocation_id_async(id);
    }

    hpx::id_type get_colocation_id(hpx::id_type const& id, error_code& ec)
    {
        auto result = get_colocation_id_async(id);
        if (result.has_value())
        {
            return HPX_MOVE(result).get_value();
        }
        return result.get_future().get(ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    hpx::future<hpx::id_type> on_symbol_namespace_event(
        std::string const& name, bool call_for_past_events)
    {
        return naming::get_agas_client().on_symbol_namespace_event(
            name, call_for_past_events);
    }

    ///////////////////////////////////////////////////////////////////////////
    hpx::future<std::pair<hpx::id_type, naming::address>> begin_migration(
        hpx::id_type const& id)
    {
        return naming::get_agas_client().begin_migration(id);
    }

    bool end_migration(hpx::id_type const& id)
    {
        return naming::get_agas_client().end_migration(id);
    }

    hpx::future<void> mark_as_migrated(naming::gid_type const& gid,
        hpx::move_only_function<std::pair<bool, hpx::future<void>>()>&& f,
        bool expect_to_be_marked_as_migrating)
    {
        return naming::get_agas_client().mark_as_migrated(
            gid, HPX_MOVE(f), expect_to_be_marked_as_migrating);
    }

    std::pair<bool, components::pinned_ptr> was_object_migrated(
        naming::gid_type const& gid,
        hpx::move_only_function<components::pinned_ptr()>&& f)
    {
        return naming::get_agas_client().was_object_migrated(gid, HPX_MOVE(f));
    }

    void unmark_as_migrated(
        naming::gid_type const& gid, hpx::move_only_function<void()>&& f)
    {
        return naming::get_agas_client().unmark_as_migrated(gid, HPX_MOVE(f));
    }

    hpx::future<symbol_namespace::iterate_names_return_type> find_symbols_async(
        std::string const& pattern)
    {
        return naming::get_agas_client().iterate_ids(pattern);
    }

    symbol_namespace::iterate_names_return_type find_symbols(
        std::string const& pattern)
    {
        return find_symbols_async(pattern).get();
    }

    ///////////////////////////////////////////////////////////////////////////
    naming::component_type register_factory(
        std::uint32_t prefix, std::string const& name, error_code& ec)
    {
        return naming::get_agas_client().register_factory(prefix, name, ec);
    }

    naming::component_type get_component_id(
        std::string const& name, error_code& ec)
    {
        return naming::get_agas_client().get_component_id(name, ec);
    }

#if defined(HPX_HAVE_NETWORKING)
    ///////////////////////////////////////////////////////////////////////////
    void route(parcelset::parcel&& p,
        hpx::function<void(std::error_code const&, parcelset::parcel const&)>&&
            f,
        threads::thread_priority local_priority)
    {
        return naming::get_agas_client().route(
            HPX_MOVE(p), HPX_MOVE(f), local_priority);
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    naming::address_type get_primary_ns_lva()
    {
        return naming::get_agas_client().get_primary_ns_lva();
    }

    naming::address_type get_symbol_ns_lva()
    {
        return naming::get_agas_client().get_symbol_ns_lva();
    }

    naming::address_type get_runtime_support_lva()
    {
        return naming::get_agas_client().get_runtime_support_lva();
    }
}    // namespace hpx::agas::detail::impl

namespace hpx::agas {

    // initialize AGAS interface function pointers in components_base module
    struct HPX_EXPORT agas_interface_functions
    {
        agas_interface_functions()
        {
            detail::is_console = &detail::impl::is_console;

            detail::register_name = &detail::impl::register_name;
            detail::register_name_async = &detail::impl::register_name_async;
            detail::register_name_id = &detail::impl::register_name_id;

            detail::unregister_name_async =
                &detail::impl::unregister_name_async;
            detail::unregister_name = &detail::impl::unregister_name;

            detail::resolve_name_async = &detail::impl::resolve_name_async;
            detail::resolve_name = &detail::impl::resolve_name;

            detail::get_num_localities_async =
                &detail::impl::get_num_localities_async;
            detail::get_num_localities = &detail::impl::get_num_localities;

            detail::get_num_threads_async =
                &detail::impl::get_num_threads_async;
            detail::get_num_threads = &detail::impl::get_num_threads;

            detail::get_num_overall_threads_async =
                &detail::impl::get_num_overall_threads_async;
            detail::get_num_overall_threads =
                &detail::impl::get_num_overall_threads;

            detail::get_component_type_name =
                &detail::impl::get_component_type_name;

            detail::is_local_address_cached =
                &detail::impl::is_local_address_cached;
            detail::is_local_address_cached_addr =
                &detail::impl::is_local_address_cached_addr;
            detail::is_local_address_cached_addr_pinned_ptr =
                &detail::impl::is_local_address_cached_addr_pinned_ptr;
            detail::update_cache_entry = &detail::impl::update_cache_entry;

            detail::is_local_lva_encoded_address =
                &detail::impl::is_local_lva_encoded_address;

            detail::resolve_async = &detail::impl::resolve_async;
            detail::resolve = &detail::impl::resolve;
            detail::resolve_cached = &detail::impl::resolve_cached;
            detail::resolve_local = &detail::impl::resolve_local;

            detail::bind_async = &detail::impl::bind_async;
            detail::bind = &detail::impl::bind;
            detail::bind_async_locality = &detail::impl::bind_async_locality;
            detail::bind_locality = &detail::impl::bind_locality;

            detail::unbind_async = &detail::impl::unbind_async;
            detail::unbind = &detail::impl::unbind;

            detail::bind_gid_local = &detail::impl::bind_gid_local;
            detail::unbind_gid_local = &detail::impl::unbind_gid_local;
            detail::bind_range_local = &detail::impl::bind_range_local;
            detail::unbind_range_local = &detail::impl::unbind_range_local;

            detail::garbage_collect_non_blocking =
                &detail::impl::garbage_collect_non_blocking;
            detail::garbage_collect = &detail::impl::garbage_collect;

            detail::get_console_locality = &detail::impl::get_console_locality;
            detail::get_locality_id = &detail::impl::get_locality_id;
            detail::get_all_locality_ids = &detail::impl::get_all_locality_ids;

#if defined(HPX_HAVE_NETWORKING)
            detail::resolve_locality = &detail::impl::resolve_locality;
            detail::remove_resolved_locality =
                &detail::impl::remove_resolved_locality;
#endif

            detail::get_next_id = &detail::impl::get_next_id;

            detail::decref = &detail::impl::decref;
            detail::incref_async = &detail::impl::incref_async;

            detail::get_colocation_id_async =
                &detail::impl::get_colocation_id_async;
            detail::get_colocation_id = &detail::impl::get_colocation_id;

            detail::on_symbol_namespace_event =
                &detail::impl::on_symbol_namespace_event;

            detail::begin_migration = &detail::impl::begin_migration;
            detail::end_migration = &detail::impl::end_migration;
            detail::mark_as_migrated = &detail::impl::mark_as_migrated;
            detail::was_object_migrated = &detail::impl::was_object_migrated;
            detail::unmark_as_migrated = &detail::impl::unmark_as_migrated;

            detail::find_symbols_async = &detail::impl::find_symbols_async;
            detail::find_symbols = &detail::impl::find_symbols;

            detail::register_factory = &detail::impl::register_factory;
            detail::get_component_id = &detail::impl::get_component_id;

#if defined(HPX_HAVE_NETWORKING)
            detail::route = &detail::impl::route;
#endif

            detail::get_primary_ns_lva = &detail::impl::get_primary_ns_lva;
            detail::get_symbol_ns_lva = &detail::impl::get_symbol_ns_lva;
            detail::get_runtime_support_lva =
                &detail::impl::get_runtime_support_lva;
        }
    };

    agas_interface_functions& agas_init()
    {
        static agas_interface_functions agas_init_;
        return agas_init_;
    }
}    // namespace hpx::agas
