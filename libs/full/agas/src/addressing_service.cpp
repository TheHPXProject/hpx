//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//  Copyright (c) 2011-2025 Hartmut Kaiser
//  Copyright (c) 2016 Parsa Amini
//  Copyright (c) 2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/agas_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/lock_registration.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/naming.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/thread_support.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/modules/util.hpp>

#include <hpx/agas/addressing_service.hpp>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::agas {

    struct addressing_service::gva_cache_key
    {
    private:
        using key_type = std::pair<naming::gid_type, naming::gid_type>;

        key_type key_;

    public:
        gva_cache_key() = default;

        explicit gva_cache_key(
            naming::gid_type const& id, std::uint64_t count = 1)
          : key_(naming::detail::get_stripped_gid(id),
                naming::detail::get_stripped_gid(id) + (count - 1))
        {
            HPX_ASSERT(count);
        }

        naming::gid_type get_gid() const
        {
            return key_.first;
        }

        std::uint64_t get_count() const
        {
            naming::gid_type const size = key_.second - key_.first;
            HPX_ASSERT(size.get_msb() == 0);
            return size.get_lsb();
        }

        friend bool operator<(
            gva_cache_key const& lhs, gva_cache_key const& rhs)
        {
            return lhs.key_.second < rhs.key_.first;
        }

        friend bool operator==(
            gva_cache_key const& lhs, gva_cache_key const& rhs)
        {
            // Direct hit
            if (lhs.key_ == rhs.key_)
            {
                return true;
            }

            // Is lhs in rhs?
            if (1 == lhs.get_count() && 1 != rhs.get_count())
            {
                return rhs.key_.first <= lhs.key_.first &&
                    lhs.key_.second <= rhs.key_.second;
            }

            // Is rhs in lhs?
            if (1 != lhs.get_count() && 1 == rhs.get_count())
            {
                return lhs.key_.first <= rhs.key_.first &&
                    rhs.key_.second <= lhs.key_.second;
            }

            return false;
        }
    };

    addressing_service::addressing_service(
        util::runtime_configuration const& ini_)
      : gva_cache_(new gva_cache_type)
      , console_cache_(naming::invalid_locality_id)
      , max_refcnt_requests_(ini_.get_agas_max_pending_refcnt_requests())
      , refcnt_requests_count_(0)
      , enable_refcnt_caching_(true)
      , refcnt_requests_(new refcnt_requests_type)
      , service_type(ini_.get_agas_service_mode())
      , runtime_type(ini_.mode_)
      , caching_(ini_.get_agas_caching_mode())
      , range_caching_(caching_ ? ini_.get_agas_range_caching_mode() : false)
      , action_priority_(threads::thread_priority::boost)
      , rts_lva_(0)
      , state_(hpx::state::starting)
    {
        if (caching_)
            gva_cache_->reserve(ini_.get_agas_local_cache_size());
    }

    void addressing_service::bootstrap(
        parcelset::endpoints_type const& endpoints,
        util::runtime_configuration& rtcfg)
    {
        LPROGRESS_;

        HPX_ASSERT(is_bootstrap());
        launch_bootstrap(endpoints, rtcfg);
    }

    void addressing_service::initialize(std::uint64_t rts_lva)
    {
        rts_lva_ = rts_lva;
        set_status(hpx::state::running);
    }

    namespace detail {

        std::uint32_t get_number_of_pus_in_cores(std::uint32_t num_cores);
    }

    void addressing_service::launch_bootstrap(
        parcelset::endpoints_type const& endpoints,
        util::runtime_configuration& rtcfg)
    {
        component_ns_.reset(new detail::bootstrap_component_namespace);
        locality_ns_.reset(new detail::bootstrap_locality_namespace(
            static_cast<server::primary_namespace*>(primary_ns_.ptr())));

        naming::gid_type const here =
            naming::get_gid_from_locality_id(agas::booststrap_prefix);
        set_local_locality(here);

        rtcfg.parse("assigned locality",
            hpx::util::format(
                "hpx.locality!={1}", naming::get_locality_id_from_gid(here)));

        std::uint32_t const num_threads =
            hpx::util::get_entry_as<std::uint32_t>(rtcfg, "hpx.os_threads", 1u);
        locality_ns_->allocate(endpoints, 0, num_threads, naming::invalid_gid);

        register_name("/0/agas/locality#0", here);
        if (is_console())
        {
            register_name("/0/locality#console", here);
        }
    }

    void addressing_service::adjust_local_cache_size(
        std::size_t cache_size) const
    {
        // adjust the local AGAS cache size for the number of worker threads and
        // create the hierarchy based on the topology
        if (caching_)
        {
            std::size_t const previous = gva_cache_->size();
            gva_cache_->reserve(cache_size);

            LAGAS_(info).format(
                "addressing_service::adjust_local_cache_size, previous size: "
                "{1}, new size: {2}",
                previous, cache_size);
        }
    }

    void addressing_service::set_local_locality(naming::gid_type const& g)
    {
        locality_ = g;
        primary_ns_.set_local_locality(g);
    }

    bool addressing_service::register_locality(
        parcelset::endpoints_type const& endpoints, naming::gid_type& prefix,
        std::uint32_t num_threads, error_code& ec)
    {
        try
        {
            prefix = naming::get_gid_from_locality_id(
                locality_ns_->allocate(endpoints, 0, num_threads, prefix));

            {
                std::unique_lock<hpx::shared_mutex> l(resolved_localities_mtx_);
                std::pair<resolved_localities_type::iterator, bool> const res =
                    resolved_localities_.emplace(prefix, endpoints);

                if (!res.second)
                {
                    l.unlock();
                    HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                        "addressing_service::register_locality",
                        "locality insertion failed because of a duplicate");
                    return false;
                }
            }

            return true;
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "addressing_service::register_locality");
            return false;
        }
    }

    void addressing_service::register_console(
        parcelset::endpoints_type const& eps)
    {
        std::lock_guard<hpx::shared_mutex> l(resolved_localities_mtx_);
        [[maybe_unused]] std::pair<resolved_localities_type::iterator,
            bool> const res =
            resolved_localities_.emplace(
                naming::get_gid_from_locality_id(0), eps);
        HPX_ASSERT(res.second);
    }

    bool addressing_service::has_resolved_locality(naming::gid_type const& gid)
    {
        std::shared_lock<hpx::shared_mutex> l(resolved_localities_mtx_);
        return resolved_localities_.find(gid) != resolved_localities_.end();
    }

    void addressing_service::pre_cache_endpoints(
        std::vector<parcelset::endpoints_type> const& endpoints)
    {
        std::unique_lock<hpx::shared_mutex> l(resolved_localities_mtx_);
        std::uint32_t locality_id = 0;
        for (parcelset::endpoints_type const& endpoint : endpoints)
        {
            resolved_localities_.emplace(
                naming::get_gid_from_locality_id(locality_id), endpoint);
            ++locality_id;
        }
    }

    parcelset::endpoints_type const& addressing_service::resolve_locality(
        naming::gid_type const& gid, error_code& ec)
    {
        resolved_localities_type::iterator it;
        {
            std::shared_lock<hpx::shared_mutex> l(resolved_localities_mtx_);
            it = resolved_localities_.find(gid);
            if (it != resolved_localities_.end() && !it->second.empty())
            {
                return it->second;
            }
        }
        std::unique_lock<hpx::shared_mutex> l(resolved_localities_mtx_);
        // The locality hasn't been requested to be resolved yet. Do it now.
        parcelset::endpoints_type endpoints;
        {
            hpx::unlock_guard<std::unique_lock<hpx::shared_mutex>> ul(l);
            endpoints = locality_ns_->resolve_locality(gid);
            if (endpoints.empty())
            {
                HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                    "addressing_service::resolve_locality",
                    "couldn't resolve the given target locality ({})", gid);

                static parcelset::endpoints_type const empty_endpoints;
                return empty_endpoints;
            }
        }

        // Search again ... might have been added by a different thread
        // already
        it = resolved_localities_.find(gid);
        if (it == resolved_localities_.end())
        {
            if (HPX_UNLIKELY(!util::insert_checked(
                    resolved_localities_.emplace(gid, endpoints), it)))
            {
                l.unlock();

                HPX_THROWS_IF(ec, hpx::error::internal_server_error,
                    "addressing_service::resolve_locality",
                    "resolved locality insertion failed "
                    "due to a locking error or memory corruption");

                static parcelset::endpoints_type const empty_endpoints;
                return empty_endpoints;
            }
        }
        else if (it->second.empty() && !endpoints.empty())
        {
            resolved_localities_[gid] = HPX_MOVE(endpoints);
        }
        return it->second;
    }

    // TODO: We need to ensure that the locality isn't unbound while it still
    // holds referenced objects.
    bool addressing_service::unregister_locality(
        naming::gid_type const& gid, error_code& ec)
    {
        try
        {
            locality_ns_->free(gid);
            component_ns_->unregister_server_instance(ec);
            symbol_ns_.unregister_server_instance(ec);

            remove_resolved_locality(gid);
            return true;
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "addressing_service::unregister_locality");
            return false;
        }
    }

    void addressing_service::remove_resolved_locality(
        naming::gid_type const& gid)
    {
        std::unique_lock<hpx::shared_mutex> l(resolved_localities_mtx_);
        if (auto const it = resolved_localities_.find(gid);
            it != resolved_localities_.end())
        {
            resolved_localities_.erase(it);
        }
    }

    bool addressing_service::get_console_locality(
        naming::gid_type& prefix, error_code& ec)
    {
        try
        {
            if (get_status() != hpx::state::running)
            {
                if (&ec != &throws)
                    ec = make_success_code();
                return false;
            }

            if (is_console())
            {
                prefix = get_local_locality();
                if (&ec != &throws)
                    ec = make_success_code();
                return true;
            }

            {
                std::lock_guard<mutex_type> lock(console_cache_mtx_);

                if (console_cache_ != naming::invalid_locality_id)
                {
                    prefix = naming::get_gid_from_locality_id(console_cache_);
                    if (&ec != &throws)
                        ec = make_success_code();
                    return true;
                }
            }

            std::string const key("/0/locality#console");

            hpx::id_type resolved_prefix = resolve_name(key);
            if (resolved_prefix != hpx::invalid_id)
            {
                std::uint32_t const console =
                    naming::get_locality_id_from_id(resolved_prefix);
                prefix = resolved_prefix.get_gid();

                {
                    std::unique_lock<mutex_type> lock(console_cache_mtx_);
                    if (console_cache_ == naming::invalid_locality_id)
                    {
                        console_cache_ = console;
                    }
                    else
                    {
                        HPX_ASSERT_LOCKED(lock, console_cache_ == console);
                    }
                }

                LAGAS_(debug).format(
                    "addressing_server::get_console_locality, caching console "
                    "locality, prefix({1})",
                    console);

                return true;
            }

            return false;
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "addressing_service::get_console_locality");
            return false;
        }
    }

    bool addressing_service::get_localities(
        std::vector<naming::gid_type>& locality_ids,
        components::component_type type, error_code& ec) const
    {
        try
        {
            if (type != to_int(hpx::components::component_enum_type::invalid))
            {
                std::vector<std::uint32_t> const p =
                    component_ns_->resolve_id(type);

                if (p.empty())
                    return false;

                locality_ids.clear();
                for (unsigned int const i : p)
                {
                    locality_ids.emplace_back(
                        naming::get_gid_from_locality_id(i));
                }

                return true;
            }

            std::vector<std::uint32_t> const p = locality_ns_->localities();

            if (!p.size())
                return false;

            locality_ids.clear();
            for (unsigned int const i : p)
            {
                locality_ids.emplace_back(naming::get_gid_from_locality_id(i));
            }
            return true;
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "addressing_service::get_locality_ids");
            return false;
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    std::uint32_t addressing_service::get_num_localities(
        components::component_type type, error_code& ec) const
    {
        try
        {
            if (type == to_int(hpx::components::component_enum_type::invalid))
            {
                return locality_ns_->get_num_localities();
            }

            return component_ns_->get_num_localities(type).get();
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "addressing_service::get_num_localities");
        }
        return static_cast<std::uint32_t>(-1);
    }

    hpx::future<std::uint32_t> addressing_service::get_num_localities_async(
        components::component_type type) const
    {
        if (type == to_int(hpx::components::component_enum_type::invalid))
        {
            return locality_ns_->get_num_localities_async();
        }

        return component_ns_->get_num_localities(type);
    }

    ///////////////////////////////////////////////////////////////////////////
    std::uint32_t addressing_service::get_num_overall_threads(
        error_code& ec) const
    {
        try
        {
            return locality_ns_->get_num_overall_threads();
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(
                ec, e, "addressing_service::get_num_overall_threads");
        }
        return static_cast<std::uint32_t>(0);
    }

    hpx::future<std::uint32_t>
    addressing_service::get_num_overall_threads_async() const
    {
        return locality_ns_->get_num_overall_threads_async();
    }

    std::vector<std::uint32_t> addressing_service::get_num_threads(
        error_code& ec) const
    {
        try
        {
            return locality_ns_->get_num_threads();
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "addressing_service::get_num_threads");
        }
        return std::vector<std::uint32_t>();
    }

    hpx::future<std::vector<std::uint32_t>>
    addressing_service::get_num_threads_async() const
    {
        return locality_ns_->get_num_threads_async();
    }

    ///////////////////////////////////////////////////////////////////////////
    components::component_type addressing_service::get_component_id(
        std::string const& name, error_code& ec) const
    {
        try
        {
            return component_ns_->bind_name(name);
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "addressing_service::get_component_id");
            return to_int(hpx::components::component_enum_type::invalid);
        }
    }

    void addressing_service::iterate_types(
        iterate_types_function_type const& f, error_code& ec) const
    {
        try
        {
            return component_ns_->iterate_types(f);
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "addressing_service::iterate_types");
        }
    }

    std::string addressing_service::get_component_type_name(
        components::component_type id, error_code& ec) const
    {
        try
        {
            return component_ns_->get_component_type_name(id);
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "addressing_service::iterate_types");
        }
        return "<unknown>";
    }

    components::component_type addressing_service::register_factory(
        std::uint32_t prefix, std::string const& name, error_code& ec) const
    {
        try
        {
            return component_ns_->bind_prefix(name, prefix);
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "addressing_service::register_factory");
            return to_int(hpx::components::component_enum_type::invalid);
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    bool addressing_service::get_id_range(std::uint64_t count,
        naming::gid_type& lower_bound, naming::gid_type& upper_bound,
        error_code& ec)
    {
        try
        {
            // parcelset::endpoints_type() is an obsolete, dummy argument

            std::pair<naming::gid_type, naming::gid_type> const rep(
                primary_ns_.allocate(count));

            if (rep.first == naming::invalid_gid ||
                rep.second == naming::invalid_gid)
            {
                return false;
            }

            lower_bound = rep.first;
            upper_bound = rep.second;

            return true;
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "addressing_service::get_id_range");
            return false;
        }
    }

    bool addressing_service::bind_range_local(naming::gid_type const& lower_id,
        std::uint64_t count, naming::address const& baseaddr,
        std::uint64_t offset, error_code& ec)
    {
        try
        {
            naming::gid_type const& prefix = baseaddr.locality_;

            // Create a global virtual address from the legacy calling convention
            // parameters
            gva const g(
                prefix, baseaddr.type_, count, baseaddr.address_, offset);

            primary_ns_.bind_gid(
                g, lower_id, naming::get_locality_from_gid(lower_id));

            if (range_caching_)
            {
                // Put the range into the cache.
                update_cache_entry(lower_id, g, ec);
            }
            else
            {
                // Only put the first GID in the range into the cache
                gva const first_g = g.resolve(lower_id, lower_id);
                update_cache_entry(lower_id, first_g, ec);
            }

            if (ec)
                return false;

            return true;
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "addressing_service::bind_range_local");
            return false;
        }
    }

    bool addressing_service::bind_postproc(
        naming::gid_type const& lower_id, gva const& g, future<bool> f)
    {
        f.get();

        if (range_caching_)
        {
            // Put the range into the cache.
            update_cache_entry(lower_id, g);
        }
        else
        {
            // Only put the first GID in the range into the cache
            gva const first_g = g.resolve(lower_id, lower_id);
            update_cache_entry(lower_id, first_g);
        }

        return true;
    }

    hpx::future<bool> addressing_service::bind_range_async(
        naming::gid_type const& lower_id, std::uint64_t count,
        naming::address const& baseaddr, std::uint64_t offset,
        naming::gid_type const& locality)
    {
        // ask server
        naming::gid_type const& prefix = baseaddr.locality_;

        // Create a global virtual address from the legacy calling convention
        // parameters.
        gva const g(prefix, baseaddr.type_, count, baseaddr.address_, offset);

        naming::gid_type id(
            naming::detail::get_stripped_gid_except_dont_cache(lower_id));

        future<bool> f = primary_ns_.bind_gid_async(g, id, locality);

        return f.then(hpx::launch::sync,
            util::one_shot(hpx::bind_front(
                &addressing_service::bind_postproc, this, id, g)));
    }

    hpx::future<naming::address> addressing_service::unbind_range_async(
        naming::gid_type const& lower_id, std::uint64_t count)
    {
        return primary_ns_.unbind_gid_async(count, lower_id);
    }

    bool addressing_service::unbind_range_local(
        naming::gid_type const& lower_id, std::uint64_t count,
        naming::address& addr, error_code& ec)
    {
        try
        {
            addr = primary_ns_.unbind_gid(count, lower_id);

            return true;
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "addressing_service::unbind_range_local");
            return false;
        }
    }

    // This function will test whether the given address refers to an object
    // living on the locality of the caller. We rely completely on the local
    // AGAS cache and local AGAS instance, assuming that everything which is not
    // in the cache is not local.

    bool addressing_service::is_local_address_cached(
        naming::gid_type const& gid, naming::address& addr, error_code& ec)
    {
        // Assume non-local operation if the gid is known to have been migrated
        naming::gid_type const id(
            naming::detail::get_stripped_gid_except_dont_cache(gid));

#if defined(HPX_HAVE_NETWORKING)
        // migratable objects should be handled by the function below
        HPX_ASSERT(!naming::detail::is_migratable(gid));
#endif

        // Try to resolve the address of the GID from the locally available
        // information.

        // NOTE: We do not throw here for a reason; it is perfectly valid for the
        // GID to not be found in the cache.
        if (!resolve_cached(id, addr, ec) || ec)
        {
            if (ec)
                return false;

            // try also the local part of AGAS before giving up
            if (!resolve_full_local(id, addr, ec) || ec)
                return false;
        }

        return addr.locality_ == get_local_locality();
    }

    bool addressing_service::is_local_address_cached(
        naming::gid_type const& gid, naming::address& addr,
        [[maybe_unused]] std::pair<bool, components::pinned_ptr>& r,
        hpx::move_only_function<std::pair<bool, components::pinned_ptr>(
            naming::address const&)>&& f,
        error_code& ec)
    {
        if (!naming::detail::is_migratable(gid))
        {
            return is_local_address_cached(gid, addr, ec);
        }

#if defined(HPX_HAVE_NETWORKING)
        // Assume non-local operation if the gid is known to have been migrated
        naming::gid_type const id(
            naming::detail::get_stripped_gid_except_dont_cache(gid));

        {
            std::unique_lock lock(migrated_objects_mtx_);
            if (was_object_migrated_locked(gid))
            {
                r = std::make_pair(true, components::pinned_ptr());
                if (&ec != &throws)
                    ec = make_success_code();
                return false;
            }
        }

        // Try to resolve the address of the GID from the locally available
        // information.

        // NOTE: We do not throw here for a reason; it is perfectly valid for
        // the GID to not be found in the cache.
        if (!resolve_cached(id, addr, ec) || ec)
        {
            if (ec)
                return false;

            // try also the local part of AGAS before giving up
            if (!resolve_full_local(id, addr, ec) || ec)
                return false;
        }

        if (&ec != &throws)
            ec = make_success_code();

        if (addr.locality_ == get_local_locality())
        {
            // if the object is local, acquire the pin count for the migratable
            // object
            r = f(addr);
            HPX_ASSERT((r.first || r.second) && !(r.first && r.second));
            if (!r.first)
            {
                return true;    // object was not migrated and address is known
            }

            // fall through
        }

        // make sure to force re-resolving the address
        addr = naming::address();
        return false;
#else
        return is_local_address_cached(gid, addr, ec);
#endif
    }

    // Return true if at least one address is local.
    bool addressing_service::is_local_lva_encoded_address(
        std::uint64_t msb) const
    {
        // NOTE: This should still be migration safe.
        return naming::detail::strip_internal_bits_and_component_type_from_gid(
                   msb) ==
            naming::detail::strip_internal_bits_and_component_type_from_gid(
                get_local_locality().get_msb());
    }

    bool addressing_service::resolve_locally_known_addresses(
        naming::gid_type const& id, naming::address& addr) const
    {
        // LVA-encoded GIDs (located on this machine)
        std::uint64_t const lsb = id.get_lsb();
        std::uint64_t msb = id.get_msb();

        if (is_local_lva_encoded_address(msb))
        {
            addr.locality_ = get_local_locality();

            // An LSB of 0 references the runtime support component
            if (0 == lsb || lsb == rts_lva_)
            {
                HPX_ASSERT(rts_lva_);

                addr.type_ = to_int(
                    hpx::components::component_enum_type::runtime_support);
                addr.address_ =
                    reinterpret_cast<naming::address::address_type>(rts_lva_);
                return true;
            }

            if (naming::refers_to_local_lva(id))
            {
                // handle (non-migratable) components located on this locality first
                addr.type_ = static_cast<components::component_type>(
                    naming::detail::get_component_type_from_gid(msb));
                addr.address_ =
                    reinterpret_cast<naming::address::address_type>(lsb);
                return true;
            }
        }

        msb = naming::detail::strip_internal_bits_from_gid(msb);

        // explicitly resolve localities
        if (naming::is_locality(id))
        {
            addr.locality_ = id;
            addr.type_ =
                to_int(hpx::components::component_enum_type::runtime_support);
            // addr.address_ will be supplied on the target locality
            return true;
        }

        // authoritative AGAS component address resolution
        if (agas::locality_ns_msb == msb && agas::locality_ns_lsb == lsb)
        {
            addr = locality_ns_->addr();
            return true;
        }
        if (agas::component_ns_msb == msb && agas::component_ns_lsb == lsb)
        {
            addr = component_ns_->addr();
            return true;
        }

        naming::gid_type const dest = naming::get_locality_from_gid(id);
        if (agas::primary_ns_lsb == lsb)
        {
            // primary AGAS service on locality 0?
            if (dest == get_local_locality())
            {
                addr = primary_ns_.addr();
            }
            // primary AGAS service on any locality
            else
            {
                addr.locality_ = dest;
                addr.type_ = to_int(
                    components::component_enum_type::agas_primary_namespace);
                // addr.address_ will be supplied on the target locality
            }
            return true;
        }

        if (agas::symbol_ns_lsb == lsb)
        {
            // symbol AGAS service on this locality?
            if (dest == get_local_locality())
            {
                addr = symbol_ns_.addr();
            }
            // symbol AGAS service on any locality
            else
            {
                addr.locality_ = dest;
                addr.type_ = to_int(
                    components::component_enum_type::agas_symbol_namespace);
                // addr.address_ will be supplied on the target locality
            }
            return true;
        }

        return false;
    }

    bool addressing_service::resolve_full_local(
        naming::gid_type const& id, naming::address& addr, error_code& ec)
    {
        try
        {
            auto rep = primary_ns_.resolve_gid(id);

            using hpx::get;

            if (get<0>(rep) == naming::invalid_gid ||
                get<2>(rep) == naming::invalid_gid)
                return false;

            // Resolve the gva to the real resolved address (which is just a gva
            // with as fully resolved LVA and an offset of zero).
            naming::gid_type const base_gid = get<0>(rep);
            gva const base_gva = get<1>(rep);

            gva const g = base_gva.resolve(id, base_gid);

            addr.locality_ = g.prefix;
            addr.type_ = g.type;
            addr.address_ = g.lva();

            if (naming::detail::store_in_cache(id))
            {
                HPX_ASSERT(addr.address_);
                if (range_caching_)
                {
                    // Put the range into the cache.
                    update_cache_entry(base_gid, base_gva, ec);
                }
                else
                {
                    // Put the fully resolved gva into the cache.
                    update_cache_entry(id, g, ec);
                }
            }

            if (ec)
                return false;

            if (&ec != &throws)
                ec = make_success_code();

            return true;
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "addressing_service::resolve_full_local");
            return false;
        }
    }

    bool addressing_service::resolve_cached(
        naming::gid_type const& gid, naming::address& addr, error_code& ec)
    {
        naming::gid_type const id =
            naming::detail::get_stripped_gid_except_dont_cache(gid);

        // special cases
        if (resolve_locally_known_addresses(id, addr))
        {
            if (&ec != &throws)
                ec = make_success_code();
            return true;
        }

        // If caching is disabled, bail
        if (!caching_)
        {
            if (&ec != &throws)
                ec = make_success_code();
            return false;
        }

        // don't look at cache if id is marked as non-cache-able
        if (!naming::detail::store_in_cache(id) || naming::is_locality(id))
        {
            if (&ec != &throws)
                ec = make_success_code();
            return false;
        }

        // don't look at the cache if the id is locally managed
        if (naming::get_locality_id_from_gid(id) ==
            naming::get_locality_id_from_gid(locality_))
        {
            if (&ec != &throws)
                ec = make_success_code();
            return false;
        }

        // force routing if target object was migrated
        if (naming::detail::is_migratable(id))
        {
            std::lock_guard<mutex_type> lock(migrated_objects_mtx_);
            if (was_object_migrated_locked(id))
            {
                if (&ec != &throws)
                    ec = make_success_code();
                return false;
            }
        }

        // first look up the requested item in the cache
        gva g;
        if (naming::gid_type idbase; get_cache_entry(id, g, idbase, ec))
        {
            addr.locality_ = g.prefix;
            addr.type_ = g.type;
            addr.address_ = g.lva(id, idbase);

            if (&ec != &throws)
                ec = make_success_code();

            return true;
        }

        if (&ec != &throws)
            ec = make_success_code();

        LAGAS_(debug).format(
            "addressing_service::resolve_cached, cache miss for address {1}",
            id);

        return false;
    }

    hpx::future_or_value<naming::address> addressing_service::resolve_async(
        naming::gid_type const& gid)
    {
        if (!gid)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "addressing_service::resolve_async", "invalid reference id");
            return naming::address();
        }

        // Try the cache.
        if (caching_)
        {
            naming::address addr;
            error_code ec;
            if (resolve_cached(gid, addr, ec))
            {
                return addr;
            }

            if (ec)
            {
                return hpx::make_exceptional_future<naming::address>(
                    hpx::detail::access_exception(ec));
            }
        }

        // now try the AGAS service
        return resolve_full_async(gid);
    }

    hpx::future_or_value<id_type> addressing_service::get_colocation_id_async(
        hpx::id_type const& id)
    {
        if (!id)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "addressing_service::get_colocation_id_async",
                "invalid reference id");
            return hpx::invalid_id;
        }

        return primary_ns_.colocate(id.get_gid());
    }

    ///////////////////////////////////////////////////////////////////////////
    naming::address addressing_service::resolve_full_postproc(
        naming::gid_type const& id, primary_namespace::resolved_type const& rep)
    {
        naming::address addr;

        if (hpx::get<0>(rep) == naming::invalid_gid ||
            hpx::get<2>(rep) == naming::invalid_gid)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "addressing_service::resolve_full_postproc",
                "could no resolve global id");
        }

        // Resolve the gva to the real resolved address (which is just a gva
        // with as fully resolved LVA and offset of zero).
        naming::gid_type const base_gid = hpx::get<0>(rep);
        gva const base_gva = hpx::get<1>(rep);

        gva const g = base_gva.resolve(id, base_gid);

        addr.locality_ = g.prefix;
        addr.type_ = g.type;
        addr.address_ = g.lva();

        if (naming::detail::store_in_cache(id))
        {
            if (range_caching_)
            {
                // Put the range into the cache.
                update_cache_entry(base_gid, base_gva);
            }
            else
            {
                // Put the fully resolved gva into the cache.
                update_cache_entry(id, g);
            }
        }

        return addr;
    }

    hpx::future_or_value<naming::address>
    addressing_service::resolve_full_async(naming::gid_type const& gid)
    {
        if (!gid)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "addressing_service::resolve_full_async",
                "invalid reference id");
            return naming::address();
        }

        // ask server
        auto result = primary_ns_.resolve_full(gid);

        if (result.has_value())
        {
            try
            {
                return resolve_full_postproc(gid, result.get_value());
            }
            catch (...)
            {
                return hpx::make_exceptional_future<naming::address>(
                    std::current_exception());
            }
        }

        return result.get_future().then(
            hpx::launch::sync, [this, gid](auto&& f) {
                return resolve_full_postproc(gid, f.get());
            });
    }

    ///////////////////////////////////////////////////////////////////////////
    bool addressing_service::resolve_full_local(naming::gid_type const* gids,
        naming::address* addrs, std::size_t count,
        hpx::detail::dynamic_bitset<>& locals, error_code& ec)
    {
        locals.resize(count);

        try
        {
            using hpx::get;

            // special cases
            for (std::size_t i = 0; i != count; ++i)
            {
                if (addrs[i])
                {
                    locals.set(i, true);
                    continue;
                }

                HPX_ASSERT(!locals.test(i));

                if (!addrs[i] && !locals.test(i))
                {
                    auto rep = primary_ns_.resolve_gid(gids[i]);

                    if (get<0>(rep) == naming::invalid_gid ||
                        get<2>(rep) == naming::invalid_gid)
                        return false;

                    // Resolve the gva to the real resolved address (which is
                    // just a gva with as fully resolved LVA and offset of
                    // zero).
                    naming::gid_type base_gid = get<0>(rep);
                    gva const base_gva = get<1>(rep);

                    gva const g = base_gva.resolve(gids[i], base_gid);

                    naming::address& addr = addrs[i];
                    addr.locality_ = g.prefix;
                    addr.type_ = g.type;
                    addr.address_ = g.lva();

                    if (naming::detail::store_in_cache(gids[i]))
                    {
                        if (range_caching_)
                        {
                            // Put the range into the cache.
                            update_cache_entry(base_gid, base_gva, ec);
                        }
                        else
                        {
                            // Put the fully resolved gva into the cache.
                            update_cache_entry(gids[i], g, ec);
                        }
                    }

                    if (ec)
                        return false;
                }
            }

            return true;
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "addressing_service::resolve_full");
            return false;
        }
    }

    bool addressing_service::resolve_cached(naming::gid_type const* gids,
        naming::address* addrs, std::size_t count,
        hpx::detail::dynamic_bitset<>& locals, error_code& ec)
    {
        locals.resize(count);

        std::size_t resolved = 0;
        for (std::size_t i = 0; i != count; ++i)
        {
            if (!addrs[i] && !locals.test(i))
            {
                bool const was_resolved = resolve_cached(gids[i], addrs[i], ec);
                if (ec)
                    return false;
                if (was_resolved)
                    ++resolved;

                if (addrs[i].locality_ == get_local_locality())
                    locals.set(i, true);
            }

            else if (addrs[i].locality_ == get_local_locality())
            {
                ++resolved;
                locals.set(i, true);
            }
        }

        return resolved == count;    // returns whether all have been resolved
    }

#if defined(HPX_HAVE_NETWORKING)
    ///////////////////////////////////////////////////////////////////////////
    void addressing_service::route(parcelset::parcel p,
        hpx::function<void(std::error_code const&, parcelset::parcel const&)>&&
            f,
        threads::thread_priority local_priority)
    {
        if (HPX_UNLIKELY(nullptr == threads::get_self_ptr()))
        {
            // reschedule this call as an HPX thread
            void (addressing_service::*route_ptr)(parcelset::parcel,
                hpx::function<void(
                    std::error_code const&, parcelset::parcel const&)>&&,
                threads::thread_priority) = &addressing_service::route;

            threads::thread_init_data data(
                threads::make_thread_function_nullary(
                    route_ptr, this, HPX_MOVE(p), HPX_MOVE(f), local_priority),
                "addressing_service::route", threads::thread_priority::normal,
                threads::thread_schedule_hint(),
                threads::thread_stacksize::default_,
                threads::thread_schedule_state::pending, true);
            threads::register_thread(data);
            return;
        }

        primary_ns_.route(HPX_MOVE(p), HPX_MOVE(f));
    }
#endif

    ///////////////////////////////////////////////////////////////////////////
    // The parameter 'compensated_credit' holds the amount of credits to be
    // added to the acknowledged number of credits. The compensated credits are
    // non-zero if there was a pending decref request at the point when the
    // incref was sent. The pending decref was subtracted from the amount of
    // credits to incref.
    std::int64_t addressing_service::synchronize_with_async_incref(
        std::int64_t old_credit, hpx::id_type const&,
        std::int64_t compensated_credit)
    {
        return old_credit + compensated_credit;
    }

    std::int64_t addressing_service::incref_async_helper(
        naming::gid_type const& id, std::int64_t credit,
        hpx::id_type const& keep_alive)
    {
        auto result = incref_async(id, credit, keep_alive);
        if (result.has_value())
        {
            return HPX_MOVE(result).get_value();
        }
        return result.get_future().get();
    }

    hpx::future_or_value<std::int64_t> addressing_service::incref_async(
        naming::gid_type const& id, std::int64_t credit,
        hpx::id_type const& keep_alive)
    {    // {{{ incref implementation
        naming::gid_type raw(naming::detail::get_stripped_gid(id));

        if (HPX_UNLIKELY(nullptr == threads::get_self_ptr()))
        {
            // reschedule this call as an HPX thread
            std::int64_t (addressing_service::*incref_async_ptr)(
                naming::gid_type const&, std::int64_t, hpx::id_type const&) =
                &addressing_service::incref_async_helper;

            return hpx::async(incref_async_ptr, this, raw, credit, keep_alive);
        }

        if (HPX_UNLIKELY(0 >= credit))
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "addressing_service::incref_async",
                "invalid credit count of {1}", credit);
            return std::int64_t(-1);
        }

        HPX_ASSERT(keep_alive != hpx::invalid_id);

        using mapping = refcnt_requests_type::value_type;

        // Some examples of calculating the compensated credits below
        //
        //  case   pending   credits   remaining   sent to   compensated
        //  no     decref              decrefs     AGAS      credits
        // ------+---------+---------+------------+--------+-------------
        //   1         0        10        0           0        10
        //   2        10         9        1           0        10
        //   3        10        10        0           0        10
        //   4        10        11        0           1        10

        std::pair<naming::gid_type, std::int64_t> pending_incref;
        bool has_pending_incref = false;
        std::int64_t pending_decrefs = 0;

        {
            std::lock_guard<mutex_type> l(refcnt_requests_mtx_);

            if (auto const matches = refcnt_requests_->find(raw);
                matches != refcnt_requests_->end())
            {
                pending_decrefs = matches->second;
                matches->second += credit;

                // Increment requests need to be handled immediately.

                // If the given incref was fully compensated by a pending decref
                // (i.e. match_data is less than 0) then there is no need to do
                // anything more.
                if (matches->second > 0)
                {
                    // credit > decrefs (case no 4): store the remaining incref
                    // to be handled below.
                    pending_incref = mapping(matches->first, matches->second);
                    has_pending_incref = true;

                    refcnt_requests_->erase(matches);
                }
                else if (matches->second == 0)
                {
                    // credit == decref (case no. 3): if the incref offsets any
                    // pending decref, just remove the pending decref request.
                    refcnt_requests_->erase(matches);
                }
                else
                {
                    // credit < decref (case no. 2): do nothing
                }
            }
            else
            {
                // case no. 1
                pending_incref = mapping(raw, credit);
                has_pending_incref = true;
            }
        }

        // no need to talk to AGAS, acknowledge the incref immediately
        if (!has_pending_incref)
        {
            return pending_decrefs;
        }

        naming::gid_type const e_lower = pending_incref.first;
        auto result = primary_ns_.increment_credit(
            pending_incref.second, e_lower, e_lower);

        // pass the amount of compensated decrefs to the callback
        if (result.has_value())
        {
            return synchronize_with_async_incref(
                result.get_value(), keep_alive, pending_decrefs);
        }

        return result.get_future().then(
            hpx::launch::sync, [keep_alive, pending_decrefs](auto&& f) {
                return synchronize_with_async_incref(
                    f.get(), keep_alive, pending_decrefs);
            });
    }    // }}}

    ///////////////////////////////////////////////////////////////////////////
    void addressing_service::decref(
        naming::gid_type const& gid, std::int64_t credit, error_code& ec)
    {
        naming::gid_type raw(naming::detail::get_stripped_gid(gid));

        if (HPX_UNLIKELY(nullptr == threads::get_self_ptr()))
        {
            // reschedule this call as an HPX thread
            threads::thread_init_data data(
                threads::make_thread_function_nullary([=, this]() -> void {
                    return decref(raw, credit, throws);
                }),
                "addressing_service::decref", threads::thread_priority::normal,
                threads::thread_schedule_hint(),
                threads::thread_stacksize::default_,
                threads::thread_schedule_state::pending, true);
            threads::register_thread(data, ec);
            return;
        }

        if (HPX_UNLIKELY(credit <= 0))
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "addressing_service::decref", "invalid credit count of {1}",
                credit);
            return;
        }

        try
        {
            std::unique_lock<mutex_type> l(refcnt_requests_mtx_);

            // Match the decref request with entries in the incref table
            if (auto const matches = refcnt_requests_->find(raw);
                matches != refcnt_requests_->end())
            {
                matches->second -= credit;
            }
            else
            {
                using iterator = refcnt_requests_type::iterator;
                using mapping = refcnt_requests_type::value_type;

                std::pair<iterator, bool> const p =
                    refcnt_requests_->insert(mapping(raw, -credit));

                if (HPX_UNLIKELY(!p.second))
                {
                    l.unlock();

                    HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                        "addressing_service::decref",
                        "couldn't insert decref request for {1} ({2})", raw,
                        credit);
                    return;
                }
            }

            send_refcnt_requests(l, ec);
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "addressing_service::decref");
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    static bool correct_credit_on_failure(future<bool> f, hpx::id_type id,
        std::int64_t mutable_gid_credit, std::int64_t new_gid_credit)
    {
        // Return the credit to the GID if the operation failed
        if ((f.has_exception() && mutable_gid_credit != 0) || !f.get())
        {
            naming::detail::add_credit_to_gid(id.get_gid(), new_gid_credit);
            return false;
        }
        return true;
    }

    bool addressing_service::register_name(std::string const& name,
        naming::gid_type const& id, error_code& ec) const
    {
        try
        {
            return symbol_ns_.bind(name, naming::detail::get_stripped_gid(id));
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "addressing_service::register_name");
        }
        return false;
    }

    bool addressing_service::register_name(
        std::string const& name, hpx::id_type const& id, error_code& ec) const
    {
        // We need to modify the reference count.
        naming::gid_type& mutable_gid = const_cast<hpx::id_type&>(id).get_gid();
        naming::gid_type const new_gid =
            naming::detail::split_gid_if_needed(hpx::launch::sync, mutable_gid);

        std::int64_t const new_credit =
            naming::detail::get_credit_from_gid(new_gid);

        try
        {
            return symbol_ns_.bind(name, new_gid);
        }
        catch (hpx::exception const& e)
        {
            if (new_credit != 0)
            {
                naming::detail::add_credit_to_gid(mutable_gid, new_credit);
            }
            HPX_RETHROWS_IF(ec, e, "addressing_service::register_name");
        }
        return false;
    }

    hpx::future<bool> addressing_service::register_name_async(
        std::string const& name, hpx::id_type const& id) const
    {
        // We need to modify the reference count.
        naming::gid_type& mutable_gid = const_cast<hpx::id_type&>(id).get_gid();
        naming::gid_type const new_gid =
            naming::detail::split_gid_if_needed(hpx::launch::sync, mutable_gid);

        std::int64_t new_credit = naming::detail::get_credit_from_gid(new_gid);

        future<bool> f = symbol_ns_.bind_async(name, new_gid);

        if (new_credit != 0)
        {
            return f.then(hpx::launch::sync,
                util::one_shot(hpx::bind_back(&correct_credit_on_failure, id,
                    HPX_GLOBALCREDIT_INITIAL, new_credit)));
        }

        return f;
    }

    ///////////////////////////////////////////////////////////////////////////
    hpx::id_type addressing_service::unregister_name(
        std::string const& name, error_code& ec) const
    {
        try
        {
            return symbol_ns_.unbind(name);
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "addressing_service::unregister_name");
            return hpx::invalid_id;
        }
    }

    hpx::future<hpx::id_type> addressing_service::unregister_name_async(
        std::string const& name) const
    {
        return symbol_ns_.unbind_async(name);
    }

    ///////////////////////////////////////////////////////////////////////////
    hpx::id_type addressing_service::resolve_name(
        std::string const& name, error_code& ec) const
    {
        try
        {
            return symbol_ns_.resolve(name);
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "addressing_service::resolve_name");
            return hpx::invalid_id;
        }
    }

    hpx::future<hpx::id_type> addressing_service::resolve_name_async(
        std::string const& name) const
    {
        return symbol_ns_.resolve_async(name);
    }

    namespace detail {

        hpx::future<hpx::id_type> on_register_event(
            hpx::future<bool> f, hpx::future<hpx::id_type> result_f)
        {
            if (!f.get())
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_request,
                    "hpx::agas::detail::on_register_event",
                    "request 'symbol_ns_on_event' failed");
            }
            return result_f;
        }
    }    // namespace detail

    future<hpx::id_type> addressing_service::on_symbol_namespace_event(
        std::string const& name, bool call_for_past_events) const
    {
        hpx::distributed::promise<hpx::id_type, naming::gid_type> p;
        auto result_f = p.get_future();

        hpx::future<bool> f =
            symbol_ns_.on_event(name, call_for_past_events, p.get_id());

        return f.then(hpx::launch::sync,
            util::one_shot(hpx::bind_back(
                &detail::on_register_event, HPX_MOVE(result_f))));
    }

    // Return all matching entries in the symbol namespace
    hpx::future<addressing_service::iterate_names_return_type>
    addressing_service::iterate_ids(std::string const& pattern) const
    {
        return symbol_ns_.iterate_async(pattern);
    }

    // This function has to return false if the key is already in the cache (true
    // means go ahead with the cache update).
    bool check_for_collisions(addressing_service::gva_cache_key const& new_key,
        addressing_service::gva_cache_key const& old_key)
    {
        return (new_key.get_gid() != old_key.get_gid()) ||
            (new_key.get_count() != old_key.get_count());
    }

    void addressing_service::update_cache_entry(
        naming::gid_type const& id, gva const& g, error_code& ec)
    {
        if (!caching_)
        {
            // If caching is disabled, we silently pretend success.
            if (&ec != &throws)
                ec = make_success_code();
            return;
        }

        // don't look at cache if id is marked as non-cache-able
        if (!naming::detail::store_in_cache(id) || naming::is_locality(id))
        {
            if (&ec != &throws)
                ec = make_success_code();
            return;
        }

        naming::gid_type const gid = naming::detail::get_stripped_gid(id);

        // don't look at the cache if the id is locally managed
        if (naming::get_locality_id_from_gid(gid) ==
            naming::get_locality_id_from_gid(locality_))
        {
            if (&ec != &throws)
                ec = make_success_code();
            return;
        }

        if (hpx::threads::get_self_ptr() == nullptr)
        {
            // Don't update the cache while HPX is starting up ...
            if (hpx::is_starting())
            {
                return;
            }

            threads::thread_init_data data(
                threads::make_thread_function_nullary([=, this]() -> void {
                    return update_cache_entry(id, g, throws);
                }),
                "addressing_service::update_cache_entry",
                threads::thread_priority::normal,
                threads::thread_schedule_hint(),
                threads::thread_stacksize::default_,
                threads::thread_schedule_state::pending, true);
            threads::register_thread(data, ec);
        }

        try
        {
            // The entry in AGAS for a locality's RTS component has a count of
            // 0, so we convert it to 1 here so that the cache doesn't break.
            std::uint64_t const count = (g.count ? g.count : 1);

            LAGAS_(debug).format(
                "addressing_service::update_cache_entry, gid({1}), count({2})",
                gid, count);

            gva_cache_key const key(gid, count);

            {
                std::unique_lock<hpx::shared_mutex> lock(gva_cache_mtx_);
                if (!gva_cache_->update_if(key, g, check_for_collisions))
                {
                    if (LAGAS_ENABLED(warning))
                    {
                        // Figure out who we collided with.
                        addressing_service::gva_cache_key idbase;
                        addressing_service::gva_cache_type::entry_type e;

                        if (!gva_cache_->get_entry(key, idbase, e))
                        {
                            // This is impossible under sane conditions.
                            lock.unlock();
                            HPX_THROWS_IF(ec, hpx::error::invalid_data,
                                "addressing_service::update_cache_entry",
                                "data corruption or lock error occurred in "
                                "cache");
                            return;
                        }

                        LAGAS_(warning).format(
                            "addressing_service::update_cache_entry, aborting "
                            "update due to key collision in cache, "
                            "new_gid({1}), new_count({2}), old_gid({3}), "
                            "old_count({4})",
                            gid, count, idbase.get_gid(), idbase.get_count());
                    }
                }
            }

            if (&ec != &throws)
                ec = make_success_code();
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "addressing_service::update_cache_entry");
        }
    }

    bool addressing_service::get_cache_entry(naming::gid_type const& gid,
        gva& gva, naming::gid_type& idbase, error_code& ec) const
    {
        // Don't use the cache while HPX is starting up
        if (hpx::is_starting())
        {
            return false;
        }

        // don't look at cache if gid is marked as non-cache-able
        HPX_ASSERT(naming::detail::store_in_cache(gid));

        gva_cache_key const k(gid);

        std::unique_lock<hpx::shared_mutex> lock(gva_cache_mtx_);
        if (gva_cache_key idbase_key; gva_cache_->get_entry(k, idbase_key, gva))
        {
            std::uint64_t const id_msb =
                naming::detail::strip_internal_bits_from_gid(gid.get_msb());

            if (HPX_UNLIKELY(id_msb != idbase_key.get_gid().get_msb()))
            {
                lock.unlock();
                HPX_THROWS_IF(ec, hpx::error::internal_server_error,
                    "addressing_service::get_cache_entry",
                    "bad entry in cache, MSBs of GID base and GID do not "
                    "match");
                return false;
            }

            idbase = idbase_key.get_gid();
            return true;
        }

        return false;
    }

    void addressing_service::clear_cache(error_code& ec) const
    {
        if (!caching_)
        {
            // If caching is disabled, we silently pretend success.
            if (&ec != &throws)
                ec = make_success_code();
            return;
        }

        // 26115: Failing to release lock 'this->gva_cache_mtx_'
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26115)
#endif
        try
        {
            LAGAS_(warning).format(
                "addressing_service::clear_cache, clearing cache");

            std::unique_lock<hpx::shared_mutex> lock(gva_cache_mtx_);

            gva_cache_->clear();

            if (&ec != &throws)
                ec = make_success_code();
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "addressing_service::clear_cache");
        }
    }
#if defined(HPX_MSVC)
#pragma warning(pop)
#endif

    void addressing_service::remove_cache_entry(
        naming::gid_type const& id, error_code& ec) const
    {
        // If caching is disabled, we silently pretend success.
        if (!caching_)
        {
            if (&ec != &throws)
                ec = make_success_code();
            return;
        }

        // don't look at cache if id is marked as non-cache-able
        if (!naming::detail::store_in_cache(id) || naming::is_locality(id))
        {
            if (&ec != &throws)
                ec = make_success_code();
            return;
        }

        // don't look at the cache if the id is locally managed
        if (naming::get_locality_id_from_gid(id) ==
            naming::get_locality_id_from_gid(locality_))
        {
            if (&ec != &throws)
                ec = make_success_code();
            return;
        }

        naming::gid_type gid = naming::detail::get_stripped_gid(id);
        try
        {
            LAGAS_(warning).format("addressing_service::remove_cache_entry");

            std::unique_lock<hpx::shared_mutex> lock(gva_cache_mtx_);

            gva_cache_->erase([&gid](std::pair<gva_cache_key, gva> const& p) {
                return gid == p.first.get_gid();
            });

            if (&ec != &throws)
                ec = make_success_code();
        }
        catch (hpx::exception const& e)
        {
            HPX_RETHROWS_IF(ec, e, "addressing_service::clear_cache");
        }
    }

    // Disable refcnt caching during shutdown
    void addressing_service::start_shutdown(error_code& ec)
    {
        // If caching is disabled, we silently pretend success.
        if (!caching_)
            return;

        std::unique_lock<mutex_type> l(refcnt_requests_mtx_);
        enable_refcnt_caching_ = false;
        send_refcnt_requests_sync(l, ec);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Helper functions to access the current cache statistics
    std::uint64_t addressing_service::get_cache_entries(bool /* reset */) const
    {
        std::shared_lock<hpx::shared_mutex> lock(gva_cache_mtx_);
        return gva_cache_->size();
    }

    std::uint64_t addressing_service::get_cache_hits(bool reset) const
    {
        std::shared_lock<hpx::shared_mutex> lock(gva_cache_mtx_);
        return gva_cache_->get_statistics().hits(reset);
    }

    std::uint64_t addressing_service::get_cache_misses(bool reset) const
    {
        std::shared_lock<hpx::shared_mutex> lock(gva_cache_mtx_);
        return gva_cache_->get_statistics().misses(reset);
    }

    std::uint64_t addressing_service::get_cache_evictions(bool reset) const
    {
        std::shared_lock<hpx::shared_mutex> lock(gva_cache_mtx_);
        return gva_cache_->get_statistics().evictions(reset);
    }

    std::uint64_t addressing_service::get_cache_insertions(bool reset) const
    {
        std::shared_lock<hpx::shared_mutex> lock(gva_cache_mtx_);
        return gva_cache_->get_statistics().insertions(reset);
    }

    ///////////////////////////////////////////////////////////////////////////
    std::uint64_t addressing_service::get_cache_get_entry_count(
        bool reset) const
    {
        std::shared_lock<hpx::shared_mutex> lock(gva_cache_mtx_);
        return gva_cache_->get_statistics().get_get_entry_count(reset);
    }

    std::uint64_t addressing_service::get_cache_insertion_entry_count(
        bool reset) const
    {
        std::shared_lock<hpx::shared_mutex> lock(gva_cache_mtx_);
        return gva_cache_->get_statistics().get_insert_entry_count(reset);
    }

    std::uint64_t addressing_service::get_cache_update_entry_count(
        bool reset) const
    {
        std::shared_lock<hpx::shared_mutex> lock(gva_cache_mtx_);
        return gva_cache_->get_statistics().get_update_entry_count(reset);
    }

    std::uint64_t addressing_service::get_cache_erase_entry_count(
        bool reset) const
    {
        std::shared_lock<hpx::shared_mutex> lock(gva_cache_mtx_);
        return gva_cache_->get_statistics().get_erase_entry_count(reset);
    }

    std::uint64_t addressing_service::get_cache_get_entry_time(bool reset) const
    {
        std::shared_lock<hpx::shared_mutex> lock(gva_cache_mtx_);
        return gva_cache_->get_statistics().get_get_entry_time(reset);
    }

    std::uint64_t addressing_service::get_cache_insertion_entry_time(
        bool reset) const
    {
        std::shared_lock<hpx::shared_mutex> lock(gva_cache_mtx_);
        return gva_cache_->get_statistics().get_insert_entry_time(reset);
    }

    std::uint64_t addressing_service::get_cache_update_entry_time(
        bool reset) const
    {
        std::shared_lock<hpx::shared_mutex> lock(gva_cache_mtx_);
        return gva_cache_->get_statistics().get_update_entry_time(reset);
    }

    std::uint64_t addressing_service::get_cache_erase_entry_time(
        bool reset) const
    {
        std::shared_lock<hpx::shared_mutex> lock(gva_cache_mtx_);
        return gva_cache_->get_statistics().get_erase_entry_time(reset);
    }

    void addressing_service::register_server_instances()
    {
        // register root server
        std::uint32_t const locality_id =
            naming::get_locality_id_from_gid(get_local_locality());
        locality_ns_->register_server_instance(locality_id);
        primary_ns_.register_server_instance(locality_id);
        component_ns_->register_server_instance(locality_id);
        symbol_ns_.register_server_instance(locality_id);
    }

    void addressing_service::unregister_server_instances(hpx::error_code& ec)
    {
        // unregister root server
        hpx::error_code first_ec;

        hpx::error_code local_ec;
        symbol_ns_.unregister_server_instance(local_ec);
        if (local_ec && !first_ec)
            first_ec = local_ec;

        local_ec = hpx::error_code();
        component_ns_->unregister_server_instance(local_ec);
        if (local_ec && !first_ec)
            first_ec = local_ec;

        local_ec = hpx::error_code();
        primary_ns_.unregister_server_instance(local_ec);
        if (local_ec && !first_ec)
            first_ec = local_ec;

        local_ec = hpx::error_code();
        locality_ns_->unregister_server_instance(local_ec);
        if (local_ec && !first_ec)
            first_ec = local_ec;

        if (first_ec)
        {
            HPX_THROWS_IF(ec, hpx::error::internal_server_error,
                "addressing_service::unregister_server_instances",
                first_ec.message());
        }
        else if (&ec != &hpx::throws)
        {
            ec = hpx::make_success_code();
        }
    }

    void addressing_service::garbage_collect_non_blocking(error_code& ec)
    {
        std::unique_lock<mutex_type> l(refcnt_requests_mtx_, std::try_to_lock);
        if (!l.owns_lock())
            return;    // no need to compete for garbage collection

        send_refcnt_requests_non_blocking(l, ec);
    }

    void addressing_service::garbage_collect(error_code& ec)
    {
        std::unique_lock<mutex_type> l(refcnt_requests_mtx_, std::try_to_lock);
        if (!l.owns_lock())
            return;    // no need to compete for garbage collection

        send_refcnt_requests_sync(l, ec);
    }

    void addressing_service::send_refcnt_requests(
        std::unique_lock<addressing_service::mutex_type>& l, error_code& ec)
    {
        if (!l.owns_lock())
        {
            HPX_THROWS_IF(ec, hpx::error::lock_error,
                "addressing_service::send_refcnt_requests",
                "mutex is not locked");
            return;
        }

        if (!enable_refcnt_caching_ ||
            max_refcnt_requests_ == ++refcnt_requests_count_)
            send_refcnt_requests_non_blocking(l, ec);

        else if (&ec != &throws)
            ec = make_success_code();
    }

#if defined(HPX_HAVE_AGAS_DUMP_REFCNT_ENTRIES)
    void dump_refcnt_requests(
        std::unique_lock<addressing_service::mutex_type>& l,
        addressing_service::refcnt_requests_type const& requests,
        char const* func_name)
    {
        HPX_ASSERT(l.owns_lock());

        std::stringstream ss;
        hpx::util::format_to(ss,
            "{1}, dumping client-side refcnt table, requests({2}):", func_name,
            requests.size());

        typedef addressing_service::refcnt_requests_type::const_reference
            const_reference;

        for (const_reference e : requests)
        {
            // The [client] tag is in there to make it easier to filter
            // through the logs.
            hpx::util::format_to(
                ss, "\n  [client] gid({1}), credits({2})", e.first, e.second);
        }

        LAGAS_(debug) << ss.str();
    }
#endif

    // 26110: Caller failing to hold lock 'l' before calling function
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26110)
#endif
    void addressing_service::send_refcnt_requests_non_blocking(
        [[maybe_unused]] std::unique_lock<addressing_service::mutex_type>& l,
        [[maybe_unused]] error_code& ec)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        HPX_ASSERT_OWNS_LOCK(l);

        try
        {
            if (refcnt_requests_->empty())
            {
                l.unlock();
                return;
            }

            auto p = std::make_shared<refcnt_requests_type>();

            p.swap(refcnt_requests_);
            refcnt_requests_count_ = 0;

            l.unlock();

            LAGAS_(info).format("addressing_service::send_refcnt_requests_non_"
                                "blocking, requests({1})",
                p->size());

#if defined(HPX_HAVE_AGAS_DUMP_REFCNT_ENTRIES)
            if (LAGAS_ENABLED(debug))
                dump_refcnt_requests(l, *p,
                    "addressing_service::send_refcnt_requests_non_blocking");
#endif

            // collect all requests for each locality
            using requests_type = std::map<hpx::id_type,
                std::vector<hpx::tuple<std::int64_t, naming::gid_type,
                    naming::gid_type>>>;
            requests_type requests;

            for (refcnt_requests_type::const_reference e : *p)
            {
                HPX_ASSERT(e.second < 0);

                naming::gid_type raw(e.first);

                hpx::id_type target(
                    primary_namespace::get_service_instance(raw),
                    hpx::id_type::management_type::unmanaged);

                requests[target].emplace_back(e.second, raw, raw);
            }

            // send requests to all locality
            auto const end = requests.end();
            for (auto it = requests.begin(); it != end; ++it)
            {
                server::primary_namespace::decrement_credit_action action;
                hpx::post(action, it->first, HPX_MOVE(it->second));
            }

            if (&ec != &throws)
                ec = make_success_code();
        }
        catch (hpx::exception const& e)
        {
            l.unlock();
            HPX_RETHROWS_IF(
                ec, e, "addressing_service::send_refcnt_requests_non_blocking");
        }
#else
        HPX_ASSERT(false);
#endif
    }

    std::vector<hpx::future<std::vector<std::int64_t>>>
    addressing_service::send_refcnt_requests_async(
        [[maybe_unused]] std::unique_lock<addressing_service::mutex_type>& l)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        HPX_ASSERT_OWNS_LOCK(l);

        if (refcnt_requests_->empty())
        {
            l.unlock();
            return std::vector<hpx::future<std::vector<std::int64_t>>>();
        }

        auto p = std::make_shared<refcnt_requests_type>();

        p.swap(refcnt_requests_);
        refcnt_requests_count_ = 0;

        l.unlock();

        LAGAS_(info).format(
            "addressing_service::send_refcnt_requests_async, requests({1})",
            p->size());

#if defined(HPX_HAVE_AGAS_DUMP_REFCNT_ENTRIES)
        if (LAGAS_ENABLED(debug))
            dump_refcnt_requests(
                l, *p, "addressing_service::send_refcnt_requests_sync");
#endif

        // collect all requests for each locality
        using requests_type = std::map<hpx::id_type,
            std::vector<
                hpx::tuple<std::int64_t, naming::gid_type, naming::gid_type>>>;
        requests_type requests;

        std::vector<hpx::future<std::vector<std::int64_t>>> lazy_results;
        for (refcnt_requests_type::const_reference e : *p)
        {
            HPX_ASSERT(e.second < 0);

            naming::gid_type raw(e.first);

            hpx::id_type target(primary_namespace::get_service_instance(raw),
                hpx::id_type::management_type::unmanaged);

            requests[target].emplace_back(e.second, raw, raw);
        }

        // send requests to all locality
        auto const end = requests.end();
        for (auto it = requests.begin(); it != end; ++it)
        {
            server::primary_namespace::decrement_credit_action action;
            lazy_results.push_back(
                hpx::async(action, it->first, HPX_MOVE(it->second)));
        }

        return lazy_results;
#else
        HPX_ASSERT(false);
        std::vector<hpx::future<std::vector<std::int64_t>>> lazy_results;
        return lazy_results;
#endif
    }
#if defined(HPX_MSVC)
#pragma warning(pop)
#endif

    void addressing_service::send_refcnt_requests_sync(
        std::unique_lock<addressing_service::mutex_type>& l, error_code& ec)
    {
        std::vector<hpx::future<std::vector<std::int64_t>>> lazy_results =
            send_refcnt_requests_async(l);

        // re throw possible errors
        hpx::wait_all(lazy_results);

        if (&ec != &throws)
            ec = make_success_code();
    }

    ///////////////////////////////////////////////////////////////////////////
    hpx::future<void> addressing_service::mark_as_migrated(
        naming::gid_type const& gid_,
        hpx::move_only_function<std::pair<bool, hpx::future<void>>()>&& f,
        [[maybe_unused]] bool expect_to_be_marked_as_migrating)
    {
        if (!gid_)
        {
            return hpx::make_exceptional_future<void>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "addressing_service::mark_as_migrated",
                    "invalid reference gid"));
        }

        HPX_ASSERT(naming::detail::is_migratable(gid_));

        // Always first grab the AGAS lock before invoking the user supplied
        // function. The user supplied code will grab another lock. Both locks have
        // to be acquired and always in the same sequence.
        // The AGAS lock needs to be acquired first as the migrated object might
        // not exist on this locality, in which case it should not be accessed
        // anymore. The only way to determine whether the object still exists on
        // this locality is to query the migrated objects table in AGAS.
        using lock_type = std::unique_lock<mutex_type>;

        lock_type lock(migrated_objects_mtx_);
        [[maybe_unused]] util::ignore_while_checking const ignore(&lock);

        // call the user code for the component instance to be migrated, the
        // returned future becomes ready whenever the component instance can be
        // migrated (no threads are pending/active any more)
        std::pair<bool, hpx::future<void>> result = f();

        // mark the gid as 'migrated' right away - the worst what can happen is
        // that a parcel which comes in for this object is bouncing between this
        // locality and the locality managing the address resolution for the object
        if (result.first)
        {
            naming::gid_type const gid(naming::detail::get_stripped_gid(gid_));

            // insert the object into the map of migrated objects
            if (auto const it = migrated_objects_table_.find(gid);
                it == migrated_objects_table_.end())
            {
                HPX_ASSERT(!expect_to_be_marked_as_migrating);
                migrated_objects_table_.insert(gid);
            }
            else
            {
                HPX_ASSERT(expect_to_be_marked_as_migrating);
            }

            // avoid interactions with the locking in the cache
            lock.unlock();

            // remove entry from cache
            remove_cache_entry(gid_);
        }

        return HPX_MOVE(result.second);
    }

    void addressing_service::unmark_as_migrated(
        naming::gid_type const& gid_, hpx::move_only_function<void()>&& f)
    {
        if (!gid_)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "addressing_service::unmark_as_migrated",
                "invalid reference gid");
        }

        HPX_ASSERT(naming::detail::is_migratable(gid_));

        naming::gid_type const gid(naming::detail::get_stripped_gid(gid_));

        std::unique_lock<mutex_type> lock(migrated_objects_mtx_);

        // remove the object from the map of migrated objects
        bool remove_from_cache = false;
        if (auto const it = migrated_objects_table_.find(gid);
            it != migrated_objects_table_.end())
        {
            migrated_objects_table_.erase(it);

            // remove entry from cache
            if (caching_ && naming::detail::store_in_cache(gid_))
            {
                remove_from_cache = true;
            }
        }

        f();    // call the user code for the component instance to be migrated

        if (remove_from_cache)
        {
            // avoid interactions with the locking in the cache
            lock.unlock();

            // remove entry from cache
            remove_cache_entry(gid_);
        }
    }

    hpx::future<std::pair<hpx::id_type, naming::address>>
    addressing_service::begin_migration(hpx::id_type const& id)
    {
        if (!id)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "addressing_service::begin_migration", "invalid reference id");
        }

        HPX_ASSERT(naming::detail::is_migratable(id.get_gid()));

        naming::gid_type const gid(
            naming::detail::get_stripped_gid(id.get_gid()));

        return primary_ns_.begin_migration(gid);
    }

    bool addressing_service::end_migration(hpx::id_type const& id)
    {
        if (!id)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "addressing_service::end_migration", "invalid reference id");
        }

        HPX_ASSERT(naming::detail::is_migratable(id.get_gid()));

        naming::gid_type const gid(
            naming::detail::get_stripped_gid(id.get_gid()));

        return primary_ns_.end_migration(gid);
    }

    bool addressing_service::was_object_migrated_locked(
        naming::gid_type const& gid_)
    {
        naming::gid_type const gid(naming::detail::get_stripped_gid(gid_));

        return migrated_objects_table_.find(gid) !=
            migrated_objects_table_.end();
    }

    std::pair<bool, components::pinned_ptr>
    addressing_service::was_object_migrated(naming::gid_type const& gid,
        hpx::move_only_function<components::pinned_ptr()>&& f    //-V669
    )
    {
        if (!gid)
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "addressing_service::was_object_migrated",
                "invalid reference gid");
        }

        // Always first grab the AGAS lock before invoking the user supplied
        // function. The user supplied code will grab another lock. Both locks
        // have to be acquired and always in the same sequence. The AGAS lock
        // needs to be acquired first as the migrated object might not exist on
        // this locality, in which case it should not be accessed anymore. The
        // only way to determine whether the object still exists on this
        // locality is to query the migrated objects table in AGAS.
        using lock_type = std::unique_lock<mutex_type>;

        lock_type const lock(migrated_objects_mtx_);

        if (!naming::detail::is_migratable(gid))
        {
            return std::make_pair(false, f());
        }

        if (was_object_migrated_locked(gid))
            return std::make_pair(true, components::pinned_ptr());

        [[maybe_unused]] util::ignore_while_checking const ignore(&lock);

        return std::make_pair(false, f());
    }
}    // namespace hpx::agas
