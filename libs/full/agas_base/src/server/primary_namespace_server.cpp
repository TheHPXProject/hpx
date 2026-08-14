//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//  Copyright (c) 2012-2025 Hartmut Kaiser
//  Copyright (c) 2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/format.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/lock_registration.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/modules/util.hpp>

#include <hpx/agas_base/route.hpp>
#include <hpx/agas_base/server/primary_namespace.hpp>

#include <atomic>
#include <cstdint>
#include <list>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace hpx::agas {

    naming::gid_type bootstrap_primary_namespace_gid()
    {
        return naming::gid_type(agas::primary_ns_msb, agas::primary_ns_lsb);
    }

    hpx::id_type bootstrap_primary_namespace_id()
    {
        return {agas::primary_ns_msb, agas::primary_ns_lsb,
            hpx::id_type::management_type::unmanaged};
    }
}    // namespace hpx::agas

namespace hpx::agas::server {

    void primary_namespace::register_server_instance(
        char const* servicename, std::uint32_t locality_id, error_code& ec)
    {
        // set locality_id for this component
        if (locality_id == naming::invalid_locality_id)
            locality_id = 0;    // if not given, we're on the root

        this->base_type::set_locality_id(locality_id);

        // now register this AGAS instance with AGAS :-P
        instance_name_ = agas::service_name;
        instance_name_ += servicename;
        instance_name_ += agas::server::primary_namespace_service_name;

        // register a gid (not the id) to avoid AGAS holding a reference to this
        // component
        agas::register_name(
            launch::sync, instance_name_, get_unmanaged_id().get_gid(), ec);
    }

    void primary_namespace::unregister_server_instance(error_code& ec) const
    {
        agas::unregister_name(launch::sync, instance_name_, ec);
        this->base_type::finalize();
    }

    void primary_namespace::finalize() const
    {
        if (!instance_name_.empty())
        {
            error_code ec(throwmode::lightweight);
            agas::unregister_name(launch::sync, instance_name_, ec);
        }
    }

    // start migration of the given object
    std::pair<hpx::id_type, naming::address> primary_namespace::begin_migration(
        naming::gid_type id)
    {
        util::scoped_timer<std::atomic<std::int64_t>> update(
            counter_data_.begin_migration_.time_,
            counter_data_.begin_migration_.enabled_);
        counter_data_.increment_begin_migration_count();
        using hpx::get;

        std::unique_lock<mutex_type> l(mutex_);

        wait_for_migration_locked(l, id, hpx::throws);
        resolved_type r = resolve_gid_locked_non_local(l, id, hpx::throws);
        if (get<0>(r) == naming::invalid_gid)
        {
            l.unlock();

            LAGAS_(info).format("primary_namespace::begin_migration, gid({1}), "
                                "response(no_success)",
                id);

            return std::make_pair(hpx::invalid_id, naming::address());
        }

        auto it = migrating_objects_.find(id);
        if (it == migrating_objects_.end())
        {
            std::pair<migration_table_type::iterator, bool> const p =
                migrating_objects_.emplace(std::piecewise_construct,
                    std::forward_as_tuple(id), std::forward_as_tuple());
            HPX_ASSERT(p.second);
            it = p.first;
        }
        else
        {
            HPX_ASSERT(!hpx::get<0>(it->second));
        }

        // flag this id as being migrated
        hpx::get<0>(it->second) = true;    //-V601

        gva const& g(hpx::get<1>(r));
        naming::address addr(g.prefix, g.type, g.lva());
        hpx::id_type loc(
            hpx::get<2>(r), hpx::id_type::management_type::unmanaged);
        return std::make_pair(loc, addr);
    }

    // migration of the given object is complete
    // 26115: Failing to release lock 'this->mutex_' in function
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26115)
#endif
    bool primary_namespace::end_migration(naming::gid_type const& id)
    {
        util::scoped_timer<std::atomic<std::int64_t>> update(
            counter_data_.end_migration_.time_,
            counter_data_.end_migration_.enabled_);
        counter_data_.increment_end_migration_count();

        std::unique_lock<mutex_type> l(mutex_);

        using hpx::get;

        if (auto const it = migrating_objects_.find(id);
            it != migrating_objects_.end())
        {
            // flag this id as not being migrated anymore
            get<0>(it->second) = false;
            if (get<1>(it->second) != 0)
            {
                get<2>(it->second).notify_all(HPX_MOVE(l), hpx::throws);
            }
            else
            {
                migrating_objects_.erase(it);
            }
        }

        return true;
    }
#if defined(HPX_MSVC)
#pragma warning(pop)
#endif

    // wait if given object is currently being migrated
    void primary_namespace::wait_for_migration_locked(
        std::unique_lock<mutex_type>& l, naming::gid_type const& id,
        error_code& ec)
    {
        HPX_ASSERT_OWNS_LOCK(l);

        using hpx::get;

        if (auto const it = migrating_objects_.find(id);
            it != migrating_objects_.end())
        {
            if (get<0>(it->second))
            {
                ++get<1>(it->second);

                get<2>(it->second).wait(l, ec);

                if (--get<1>(it->second) == 0)    //-V516
                {
                    migrating_objects_.erase(it);
                }
            }
            else
            {
                if (get<1>(it->second) == 0)
                {
                    migrating_objects_.erase(it);
                }
            }
        }
    }

    bool primary_namespace::bind_gid(
        gva const& g, naming::gid_type id, naming::gid_type const& locality)
    {    // {{{ bind_gid implementation
        util::scoped_timer<std::atomic<std::int64_t>> update(
            counter_data_.bind_gid_.time_, counter_data_.bind_gid_.enabled_);
        counter_data_.increment_bind_gid_count();
        using hpx::get;

        naming::gid_type const gid = id;
        naming::detail::strip_internal_bits_from_gid(id);

        std::unique_lock<mutex_type> l(mutex_);

        auto const begin = gvas_.begin();
        auto const end = gvas_.end();

        if (auto it = gvas_.lower_bound(id); it != end)
        {
            // If we got an exact match, this is a request to update an existing
            // binding (e.g. move semantics).
            if (it->first == id)
            {
                // non-migratable gids can't be rebound
                if (naming::refers_to_local_lva(gid) &&
                    !naming::refers_to_virtual_memory(gid))
                {
                    l.unlock();

                    HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                        "primary_namespace::bind_gid",
                        "cannot rebind gids for non-migratable objects");
                }

                gva& gaddr = it->second.first;
                naming::gid_type& loc = it->second.second;

                // Check for count mismatch (we can't change block sizes of
                // existing bindings).
                if (HPX_UNLIKELY(gaddr.count != g.count))
                {
                    // REVIEW: Is this the right error code to use?
                    l.unlock();

                    HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                        "primary_namespace::bind_gid",
                        "cannot change block size of existing binding");
                }

                if (HPX_UNLIKELY(
                        to_int(hpx::components::component_enum_type::invalid) ==
                        g.type))
                {
                    l.unlock();

                    HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                        "primary_namespace::bind_gid",
                        "attempt to update a GVA with an invalid type, "
                        "gid({1}), gva({2}), locality({3})",
                        id, g, locality);
                }

                if (HPX_UNLIKELY(!locality))
                {
                    l.unlock();

                    HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                        "primary_namespace::bind_gid",
                        "attempt to update a GVA with an invalid "
                        "locality id, "
                        "gid({1}), gva({2}), locality({3})",
                        id, g, locality);
                }

                // Store the new endpoint and offset
                gaddr.prefix = g.prefix;
                gaddr.type = g.type;
                gaddr.lva(g.lva());
                gaddr.offset = g.offset;
                loc = locality;

                l.unlock();

                LAGAS_(info).format(
                    "primary_namespace::bind_gid, gid({1}), gva({2}), "
                    "locality({3}), response(repeated_request)",
                    id, g, locality);

                return false;
            }

            // We're about to decrement the iterator it - first, we
            // check that it's safe to do this.
            else if (it != begin)
            {
                --it;

                // Check that a previous range doesn't cover the new id.
                if (HPX_UNLIKELY((it->first + it->second.first.count) > id))
                {
                    // REVIEW: Is this the right error code to use?
                    l.unlock();

                    HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                        "primary_namespace::bind_gid",
                        "the new GID is contained in an existing range");
                }
            }
        }

        else if (HPX_LIKELY(!gvas_.empty()))
        {
            --it;

            // Check that a previous range doesn't cover the new id.
            if ((it->first + it->second.first.count) > id)
            {
                // REVIEW: Is this the right error code to use?
                l.unlock();

                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "primary_namespace::bind_gid",
                    "the new GID is contained in an existing range");
            }
        }

        // non-migratable gids don't need to be bound
        if (naming::refers_to_local_lva(gid) &&
            !naming::refers_to_virtual_memory(gid))
        {
            LAGAS_(info).format(
                "primary_namespace::bind_gid, gid({1}), gva({2}), "
                "locality({3})",
                gid, g, locality);

            return true;
        }

        naming::gid_type const upper_bound(id + (g.count - 1));

        if (HPX_UNLIKELY(id.get_msb() != upper_bound.get_msb()))
        {
            l.unlock();

            HPX_THROW_EXCEPTION(hpx::error::internal_server_error,
                "primary_namespace::bind_gid",
                "MSBs of lower and upper range bound do not match");
        }

        if (HPX_UNLIKELY(
                to_int(hpx::components::component_enum_type::invalid) ==
                g.type))
        {
            l.unlock();

            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "primary_namespace::bind_gid",
                "attempt to insert a GVA with an invalid type, "
                "gid({1}), gva({2}), locality({3})",
                id, g, locality);
        }

        // Insert a GID -> GVA entry into the GVA table.
        if (HPX_UNLIKELY(!util::insert_checked(
                gvas_.emplace(id, std::make_pair(g, locality)))))
        {
            l.unlock();

            HPX_THROW_EXCEPTION(hpx::error::lock_error,
                "primary_namespace::bind_gid",
                "GVA table insertion failed due to a locking error or "
                "memory corruption, gid({1}), gva({2}), locality({3})",
                id, g, locality);
        }

        l.unlock();

        LAGAS_(info).format(
            "primary_namespace::bind_gid, gid({1}), gva({2}), locality({3})",
            id, g, locality);

        return true;
    }    // }}}

    inline primary_namespace::resolved_type resolve_local_id(
        naming::gid_type const& id) noexcept
    {
        naming::gid_type locality = naming::get_locality_from_gid(id);
        gva addr(locality,
            static_cast<components::component_type>(
                naming::detail::get_component_type_from_gid(id.get_msb())),
            1, id.get_lsb());
        return primary_namespace::resolved_type(id, addr, locality);
    }

    primary_namespace::resolved_type primary_namespace::resolve_gid(
        naming::gid_type const& id)
    {    // {{{ resolve_gid implementation
        util::scoped_timer<std::atomic<std::int64_t>> update(
            counter_data_.resolve_gid_.time_,
            counter_data_.resolve_gid_.enabled_);
        counter_data_.increment_resolve_gid_count();
        using hpx::get;

        resolved_type r;

        // handle (non-migratable) components located on this locality first
        if (naming::refers_to_local_lva(id) &&
            !naming::refers_to_virtual_memory(id))
        {
            r = resolve_local_id(id);
        }
        else
        {
            std::unique_lock<mutex_type> l(mutex_);

            // wait for any migration to be completed
            if (naming::detail::is_migratable(id))
            {
                wait_for_migration_locked(l, id, hpx::throws);
            }

            // now, resolve the id
            r = resolve_gid_locked_non_local(l, id, hpx::throws);
        }

        if (get<0>(r) == naming::invalid_gid)
        {
            LAGAS_(info).format("primary_namespace::resolve_gid, gid({1}), "
                                "response(no_success)",
                id);

            return resolved_type(
                naming::invalid_gid, gva(), naming::invalid_gid);
        }

        LAGAS_(info).format(
            "primary_namespace::resolve_gid, gid({1}), base({2}), gva({3}), "
            "locality_id({4})",
            id, get<0>(r), get<1>(r), get<2>(r));

        return r;
    }    // }}}

    hpx::id_type primary_namespace::colocate(naming::gid_type const& id)
    {
        return {hpx::get<2>(resolve_gid(id)),
            hpx::id_type::management_type::unmanaged};
    }

    naming::address primary_namespace::unbind_gid(
        std::uint64_t count, naming::gid_type id)
    {    // {{{ unbind_gid implementation
        util::scoped_timer<std::atomic<std::int64_t>> update(
            counter_data_.unbind_gid_.time_,
            counter_data_.unbind_gid_.enabled_);
        counter_data_.increment_unbind_gid_count();

        naming::detail::strip_internal_bits_from_gid(id);

        std::unique_lock<mutex_type> l(mutex_);

        auto const it = gvas_.find(id);
        if (auto const end = gvas_.end(); it != end)
        {
            if (HPX_UNLIKELY(it->second.first.count != count))
            {
                l.unlock();

                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "primary_namespace::unbind_gid", "block sizes must match");
            }

            gva_table_data_type const data = it->second;

            gvas_.erase(it);

            l.unlock();
            LAGAS_(info).format(
                "primary_namespace::unbind_gid, gid({1}), count({2}), "
                "gva({3}), locality_id({4})",
                id, count, data.first, data.second);

            gva g = data.first;
            return {g.prefix, g.type, g.lva()};
        }

        // non-migratable gids are not bound
        if (naming::refers_to_local_lva(id) &&
            !naming::refers_to_virtual_memory(id))
        {
            naming::gid_type const locality = naming::get_locality_from_gid(id);
            gva g(locality,
                static_cast<components::component_type>(
                    naming::detail::get_component_type_from_gid(id.get_msb())),
                0, id.get_lsb());

            LAGAS_(info).format(
                "primary_namespace::unbind_gid, gid({1}), count({2}), "
                "gva({3}), locality({4})",
                id, count, g, g.prefix);

            return {g.prefix, g.type, g.lva()};
        }

        l.unlock();

        LAGAS_(info).format(
            "primary_namespace::unbind_gid, gid({1}), count({2}), "
            "response(no_success)",
            id, count);

        return {};
    }    // }}}

    std::int64_t primary_namespace::increment_credit(
        std::int64_t credits, naming::gid_type lower, naming::gid_type upper)
    {    // increment_credit implementation
        util::scoped_timer<std::atomic<std::int64_t>> update(
            counter_data_.increment_credit_.time_,
            counter_data_.increment_credit_.enabled_);
        counter_data_.increment_increment_credit_count();

        naming::detail::strip_internal_bits_from_gid(lower);
        naming::detail::strip_internal_bits_from_gid(upper);

        if (lower == upper)
            ++upper;

        // Increment.
        if (credits > 0)
        {
            increment(lower, upper, credits, hpx::throws);
            return credits;
        }

        HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
            "primary_namespace::increment_credit",
            "invalid credit count of {1}", credits);
    }

    std::vector<std::int64_t> primary_namespace::decrement_credit(
        std::vector<hpx::tuple<std::int64_t, naming::gid_type,
            naming::gid_type>> const& requests)
    {    // decrement_credit implementation
        util::scoped_timer<std::atomic<std::int64_t>> update(
            counter_data_.decrement_credit_.time_,
            counter_data_.decrement_credit_.enabled_);
        counter_data_.increment_decrement_credit_count();

        std::vector<int64_t> res_credits;
        res_credits.reserve(requests.size());

        for (auto& req : requests)
        {
            std::int64_t credits = hpx::get<0>(req);
            naming::gid_type lower = hpx::get<1>(req);
            naming::gid_type upper = hpx::get<1>(req);

            naming::detail::strip_internal_bits_from_gid(lower);
            naming::detail::strip_internal_bits_from_gid(upper);

            if (lower == upper)
                ++upper;

            // Decrement.
            if (credits < 0)
            {
                free_entry_list_type free_list;
                decrement_sweep(free_list, lower, upper, -credits, hpx::throws);

                free_components_sync(free_list, lower, upper, hpx::throws);
            }
            else
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "primary_namespace::decrement_credit",
                    "invalid credit count of {1}", credits);
            }
            res_credits.push_back(credits);
        }

        return res_credits;
    }

    std::pair<naming::gid_type, naming::gid_type> primary_namespace::allocate(
        std::uint64_t count)
    {    // {{{ allocate implementation
        util::scoped_timer<std::atomic<std::int64_t>> update(
            counter_data_.allocate_.time_, counter_data_.allocate_.enabled_);
        counter_data_.increment_allocate_count();

        // Just return the prefix
        if (count == 0)
        {
            LAGAS_(info).format(
                "primary_namespace::allocate, count({1}), lower({1}), "
                "upper({3}), prefix({4}), response(repeated_request)",
                count, next_id_, next_id_,
                naming::get_locality_id_from_gid(next_id_));

            return std::make_pair(next_id_, next_id_);
        }

        std::uint64_t const real_count = count - 1;

        // Compute the new allocation.
        naming::gid_type lower(next_id_ + 1);
        naming::gid_type upper(lower + real_count);

        // Check for overflow.
        if (upper.get_msb() != lower.get_msb())
        {
            // Check for address space exhaustion (we currently use 86 bits of
            // the gid for the actual id)
            if (HPX_UNLIKELY(
                    (lower.get_msb() & naming::gid_type::virtual_memory_mask) ==
                    naming::gid_type::virtual_memory_mask))
            {
                HPX_THROW_EXCEPTION(hpx::error::internal_server_error,
                    "locality_namespace::allocate",
                    "primary namespace has been exhausted");
            }

            // Otherwise, correct
            lower = naming::gid_type(upper.get_msb(), nullptr);
            upper = lower + real_count;
        }

        // Store the new upper bound.
        next_id_ = upper;

        // Set the initial credit count.
        naming::detail::set_credit_for_gid(
            lower, static_cast<std::int64_t>(HPX_GLOBALCREDIT_INITIAL));
        naming::detail::set_credit_for_gid(
            upper, static_cast<std::int64_t>(HPX_GLOBALCREDIT_INITIAL));

        LAGAS_(info).format(
            "primary_namespace::allocate, count({1}), lower({2}), upper({3}), "
            "response(success)",
            count, lower, upper);

        return std::make_pair(lower, upper);
    }    // }}}

#if defined(HPX_HAVE_AGAS_DUMP_REFCNT_ENTRIES)
    void primary_namespace::dump_refcnt_matches(
        refcnt_table_type::iterator lower_it,
        refcnt_table_type::iterator upper_it, naming::gid_type const& lower,
        naming::gid_type const& upper, std::unique_lock<mutex_type>& l,
        char const* func_name)
    {
        // dump_refcnt_matches implementation
        HPX_ASSERT(l.owns_lock());

        if (lower_it == refcnts_.end() && upper_it == refcnts_.end())
            // We got nothing, bail - our caller is probably about to throw.
            return;

        std::stringstream ss;
        hpx::util::format_to(ss,
            "{1}, dumping server-side refcnt table matches, lower({2}), "
            "upper({3}):",
            func_name, lower, upper);

        for (/**/; lower_it != upper_it; ++lower_it)
        {
            // The [server] tag is in there to make it easier to filter
            // through the logs.
            hpx::util::format_to(ss, "\n  [server] lower({1}), credits({2})",
                lower_it->first, lower_it->second);
        }

        LAGAS_(debug) << ss.str();
    }    // dump_refcnt_matches implementation
#endif

    ///////////////////////////////////////////////////////////////////////////////
    void primary_namespace::increment(naming::gid_type const& lower,
        naming::gid_type const& upper, std::int64_t const& credits,
        error_code& ec)
    {    // {{{ increment implementation
        std::unique_lock<mutex_type> l(mutex_);

#if defined(HPX_HAVE_AGAS_DUMP_REFCNT_ENTRIES)
        if (LAGAS_ENABLED(debug))
        {
            typedef refcnt_table_type::iterator iterator;

            // Find the mappings that we're about to touch.
            refcnt_table_type::iterator lower_it = refcnts_.find(lower);
            refcnt_table_type::iterator upper_it;
            if (lower != upper)
            {
                upper_it = refcnts_.find(upper);
            }
            else
            {
                upper_it = lower_it;
                ++upper_it;
            }

            dump_refcnt_matches(lower_it, upper_it, lower, upper, l,
                "primary_namespace::increment");
        }
#endif

        // TODO: Whine loudly if a reference count overflows. We reserve ~0 for
        // internal bookkeeping in the decrement algorithm, so the maximum global
        // reference count is 2^64 - 2. The maximum number of credits a single GID
        // can hold, however, is limited to 2^32 - 1.

        // The third parameter we pass here is the default data to use in case the
        // key is not mapped. We don't insert GIDs into the refcnt table when we
        // allocate/bind them, so if a GID is not in the refcnt table, we know that
        // it's global reference count is the initial global reference count.

        for (naming::gid_type raw = lower; raw != upper; ++raw)
        {
            auto it = refcnts_.find(raw);
            if (it == refcnts_.end())
            {
                std::int64_t count =
                    static_cast<std::int64_t>(HPX_GLOBALCREDIT_INITIAL) +
                    credits;

                std::pair<refcnt_table_type::iterator, bool> const p =
                    refcnts_.insert(refcnt_table_type::value_type(raw, count));
                if (!p.second)
                {
                    l.unlock();

                    HPX_THROWS_IF(ec, hpx::error::invalid_data,
                        "primary_namespace::increment",
                        "couldn't create entry in reference count table, "
                        "raw({1}), ref-count({2})",
                        raw, count);
                    return;
                }

                it = p.first;
            }
            else
            {
                it->second += credits;
            }

            LAGAS_(info).format(
                "primary_namespace::increment, raw({1}), refcnt({2})", lower,
                it->second);
        }

        if (&ec != &throws)
            ec = make_success_code();
    }    // }}}

    ///////////////////////////////////////////////////////////////////////////////
    // 26110: Caller failing to hold lock 'l' before calling function
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26110)
#endif
    void primary_namespace::resolve_free_list(std::unique_lock<mutex_type>& l,
        std::list<refcnt_table_type::iterator> const& free_list,
        free_entry_list_type& free_entry_list,
        naming::gid_type const& /* lower */,
        naming::gid_type const& /* upper */, error_code& ec)
    {
        HPX_ASSERT_OWNS_LOCK(l);

        using hpx::get;

        using iterator = refcnt_table_type::iterator;

        for (iterator const& it : free_list)
        {
            using key_type = refcnt_table_type::key_type;

            // The mapping's key space.
            key_type gid = it->first;

            if (naming::detail::is_migratable(gid))
            {
                // wait for any migration to be completed
                wait_for_migration_locked(l, gid, ec);
            }

            // Resolve the query GID.
            resolved_type r = resolve_gid_locked(l, gid, ec);
            if (ec)
                return;

            naming::gid_type& raw = get<0>(r);
            if (raw == naming::invalid_gid)
            {
                l.unlock();

                HPX_THROWS_IF(ec, hpx::error::internal_server_error,
                    "primary_namespace::resolve_free_list",
                    "primary_namespace::resolve_free_list, failed to resolve "
                    "gid, gid({1})",
                    gid);
                return;    // couldn't resolve this one
            }

            // Make sure the GVA is valid.
            gva& g = get<1>(r);

            // REVIEW: Should we do more to make sure the GVA is valid?
            if (HPX_UNLIKELY(
                    to_int(hpx::components::component_enum_type::invalid) ==
                    g.type))
            {
                l.unlock();

                HPX_THROWS_IF(ec, hpx::error::internal_server_error,
                    "primary_namespace::resolve_free_list",
                    "encountered a GVA with an invalid type while performing a "
                    "decrement, gid({1}), gva({2})",
                    gid, g);
                return;
            }
            else if (HPX_UNLIKELY(0 == g.count))
            {
                l.unlock();

                HPX_THROWS_IF(ec, hpx::error::internal_server_error,
                    "primary_namespace::resolve_free_list",
                    "encountered a GVA with a count of zero while performing a "
                    "decrement, gid({1}), gva({2})",
                    gid, g);
                return;
            }

            LAGAS_(info).format(
                "primary_namespace::resolve_free_list, resolved match, "
                "gid({1}), gva({2})",
                gid, g);

            // Fully resolve the range.
            gva const resolved = g.resolve(gid, raw);

            // Add the information needed to destroy these components to the
            // free list.
            free_entry_list.emplace_back(resolved, gid, get<2>(r));

            // remove this entry from the refcnt table
            refcnts_.erase(it);
        }
    }
#if defined(HPX_MSVC)
#pragma warning(pop)
#endif

    ///////////////////////////////////////////////////////////////////////////////
    void primary_namespace::decrement_sweep(
        free_entry_list_type& free_entry_list, naming::gid_type const& lower,
        naming::gid_type const& upper, std::int64_t credits, error_code& ec)
    {    // {{{ decrement_sweep implementation
        LAGAS_(info).format(
            "primary_namespace::decrement_sweep, lower({1}), upper({2}), "
            "credits({3})",
            lower, upper, credits);

        free_entry_list.clear();

        {
            std::unique_lock<mutex_type> l(mutex_);

#if defined(HPX_HAVE_AGAS_DUMP_REFCNT_ENTRIES)
            if (LAGAS_ENABLED(debug))
            {
                typedef refcnt_table_type::iterator iterator;

                // Find the mappings that we just added or modified.
                refcnt_table_type::iterator lower_it = refcnts_.find(lower);
                refcnt_table_type::iterator upper_it;
                if (lower != upper)
                {
                    upper_it = refcnts_.find(upper);
                }
                else
                {
                    upper_it = lower_it;
                    ++upper_it;
                }

                dump_refcnt_matches(lower_it, upper_it, lower, upper, l,
                    "primary_namespace::decrement_sweep");
            }
#endif

            ///////////////////////////////////////////////////////////////////////
            // Apply the decrement across the entire key space (e.g. [lower, upper]).

            // The third parameter we pass here is the default data to use in case
            // the key is not mapped. We don't insert GIDs into the refcnt table
            // when we allocate/bind them, so if a GID is not in the refcnt table,
            // we know that it's global reference count is the initial global
            // reference count.

            std::list<refcnt_table_type::iterator> free_list;    //-V826
            for (naming::gid_type raw = lower; raw != upper; ++raw)
            {
                auto it = refcnts_.find(raw);
                if (it == refcnts_.end())
                {
                    if (credits >
                        static_cast<std::int64_t>(HPX_GLOBALCREDIT_INITIAL))
                    {
                        l.unlock();

                        HPX_THROWS_IF(ec, hpx::error::invalid_data,
                            "primary_namespace::decrement_sweep",
                            "negative entry in reference count table, "
                            "raw({1}), refcount({2})",
                            raw,
                            static_cast<std::int64_t>(
                                HPX_GLOBALCREDIT_INITIAL) -
                                credits);
                        return;
                    }

                    std::int64_t count =
                        static_cast<std::int64_t>(HPX_GLOBALCREDIT_INITIAL) -
                        credits;

                    std::pair<refcnt_table_type::iterator, bool> const p =
                        refcnts_.emplace(raw, count);
                    if (!p.second)
                    {
                        l.unlock();

                        HPX_THROWS_IF(ec, hpx::error::invalid_data,
                            "primary_namespace::decrement_sweep",
                            "couldn't create entry in reference count table, "
                            "raw({1}), ref-count({2})",
                            raw, count);
                        return;
                    }

                    it = p.first;
                }
                else
                {
                    it->second -= credits;
                }

                // Sanity check.
                if (it->second < 0)
                {
                    l.unlock();

                    HPX_THROWS_IF(ec, hpx::error::invalid_data,
                        "primary_namespace::decrement_sweep",
                        "negative entry in reference count table, raw({1}), "
                        "refcount({2})",
                        raw, it->second);
                    return;
                }

                // this objects needs to be deleted
                if (it->second == 0)
                    free_list.push_back(it);
            }

            // Resolve the objects which have to be deleted.
            resolve_free_list(l, free_list, free_entry_list, lower, upper, ec);

        }    // Unlock the mutex.

        if (&ec != &throws)
            ec = make_success_code();
    }

    ///////////////////////////////////////////////////////////////////////////////
    void primary_namespace::free_components_sync(
        free_entry_list_type const& free_list, naming::gid_type const& lower,
        naming::gid_type const& upper, error_code& ec) const
    {
        using hpx::get;

        ///////////////////////////////////////////////////////////////////////////
        // Delete the objects on the free list.
        for (free_entry const& e : free_list)
        {
            // Bail if we're in late shutdown and non-local.
            if (HPX_UNLIKELY(!threads::threadmanager_is(hpx::state::running)) &&
                e.locality_ != locality_)
            {
                LAGAS_(info).format(
                    "primary_namespace::free_components_sync, cancelling free "
                    "operation because the threadmanager is down, lower({1}), "
                    "upper({2}), base({3}), gva({4}), locality({5})",
                    lower, upper, e.gid_, e.gva_, e.locality_);
                continue;
            }

            LAGAS_(info).format(
                "primary_namespace::free_components_sync, freeing component, "
                "lower({1}), upper({2}), base({3}), gva({4}), locality({5})",
                lower, upper, e.gid_, e.gva_, e.locality_);

            // Destroy the component.
            HPX_ASSERT(e.locality_ == e.gva_.prefix);
            naming::address addr(e.locality_, e.gva_.type, e.gva_.lva());
            if (e.locality_ == locality_)
            {
                auto const deleter = components::deleter(e.gva_.type);
                if (deleter == nullptr)
                {
                    HPX_THROWS_IF(ec, hpx::error::internal_server_error,
                        "primary_namespace::free_components_sync",
                        "Attempting to delete object of unknown component "
                        "type: " +
                            std::to_string(e.gva_.type));
                    return;
                }
                deleter(e.gid_, HPX_MOVE(addr));
            }
            else
            {
                agas::destroy_component(e.gid_, addr);
            }
        }

        if (&ec != &throws)
            ec = make_success_code();
    }

    primary_namespace::resolved_type primary_namespace::resolve_gid_locked(
        std::unique_lock<mutex_type>& l, naming::gid_type const& gid,
        error_code& ec)
    {
        HPX_ASSERT_OWNS_LOCK(l);

        // handle (non-migratable) components located on this locality first
        if (naming::refers_to_local_lva(gid) &&
            !naming::refers_to_virtual_memory(gid))
        {
            if (&ec != &throws)
                ec = make_success_code();

            return resolve_local_id(gid);
        }

        return resolve_gid_locked_non_local(l, gid, ec);
    }

    // 26110: Caller failing to hold lock 'l' before calling function
#if defined(HPX_MSVC)
#pragma warning(push)
#pragma warning(disable : 26110)
#endif
    primary_namespace::resolved_type
    primary_namespace::resolve_gid_locked_non_local(
        std::unique_lock<mutex_type>& l, naming::gid_type const& gid,
        error_code& ec)
    {
        HPX_ASSERT_OWNS_LOCK(l);

        HPX_ASSERT(!(naming::refers_to_local_lva(gid) &&
            !naming::refers_to_virtual_memory(gid)));

        // parameters
        naming::gid_type id = gid;
        naming::detail::strip_internal_bits_from_gid(id);

        gva_table_type::const_iterator it = gvas_.lower_bound(id);
        gva_table_type::const_iterator const begin = gvas_.begin();

        if (gva_table_type::const_iterator const end = gvas_.end(); it != end)
        {
            // Check for exact match
            if (it->first == id)
            {
                if (&ec != &throws)
                    ec = make_success_code();

                gva_table_data_type const& data = it->second;
                return resolved_type(it->first, data.first, data.second);
            }

            // We need to decrement the iterator, first we check that it's safe
            // to do this.
            if (it != begin)
            {
                --it;

                // Found the GID in a range
                gva_table_data_type const& data = it->second;
                if ((it->first + data.first.count) > id)
                {
                    if (HPX_UNLIKELY(id.get_msb() != it->first.get_msb()))
                    {
                        l.unlock();

                        HPX_THROWS_IF(ec, hpx::error::internal_server_error,
                            "primary_namespace::resolve_gid_locked",
                            "MSBs of lower and upper range bound do not "
                            "match");
                        return resolved_type(
                            naming::invalid_gid, gva(), naming::invalid_gid);
                    }

                    if (&ec != &throws)
                        ec = make_success_code();

                    return resolved_type(it->first, data.first, data.second);
                }
            }
        }

        else if (HPX_LIKELY(!gvas_.empty()))
        {
            --it;

            // Found the GID in a range
            gva_table_data_type const& data = it->second;
            if ((it->first + data.first.count) > id)
            {
                if (HPX_UNLIKELY(id.get_msb() != it->first.get_msb()))
                {
                    l.unlock();

                    HPX_THROWS_IF(ec, hpx::error::internal_server_error,
                        "primary_namespace::resolve_gid_locked",
                        "MSBs of lower and upper range bound do not match");
                    return resolved_type(
                        naming::invalid_gid, gva(), naming::invalid_gid);
                }

                if (&ec != &throws)
                    ec = make_success_code();

                return resolved_type(it->first, data.first, data.second);
            }
        }

        if (&ec != &throws)
            ec = make_success_code();

        return resolved_type(naming::invalid_gid, gva(), naming::invalid_gid);
    }
#if defined(HPX_MSVC)
#pragma warning(pop)
#endif

#if defined(HPX_HAVE_NETWORKING)
    void (*route)(primary_namespace& server, parcelset::parcel&& p) = nullptr;

    void primary_namespace::route(parcelset::parcel&& p)
    {
        util::scoped_timer<std::atomic<std::int64_t>> update(
            counter_data_.route_.time_, counter_data_.route_.enabled_);
        counter_data_.increment_route_count();

        (*server::route)(*this, HPX_MOVE(p));
    }
#endif

    // access current counter values
    std::int64_t primary_namespace::counter_data::get_bind_gid_count(bool reset)
    {
        return util::get_and_reset_value(bind_gid_.count_, reset);
    }

    std::int64_t primary_namespace::counter_data::get_resolve_gid_count(
        bool reset)
    {
        return util::get_and_reset_value(resolve_gid_.count_, reset);
    }

    std::int64_t primary_namespace::counter_data::get_unbind_gid_count(
        bool reset)
    {
        return util::get_and_reset_value(unbind_gid_.count_, reset);
    }

    std::int64_t primary_namespace::counter_data::get_increment_credit_count(
        bool reset)
    {
        return util::get_and_reset_value(increment_credit_.count_, reset);
    }

    std::int64_t primary_namespace::counter_data::get_decrement_credit_count(
        bool reset)
    {
        return util::get_and_reset_value(decrement_credit_.count_, reset);
    }

    std::int64_t primary_namespace::counter_data::get_allocate_count(bool reset)
    {
        return util::get_and_reset_value(allocate_.count_, reset);
    }

    std::int64_t primary_namespace::counter_data::get_begin_migration_count(
        bool reset)
    {
        return util::get_and_reset_value(begin_migration_.count_, reset);
    }

    std::int64_t primary_namespace::counter_data::get_end_migration_count(
        bool reset)
    {
        return util::get_and_reset_value(end_migration_.count_, reset);
    }

    std::int64_t primary_namespace::counter_data::get_overall_count(bool reset)
    {
        return
#if defined(HPX_HAVE_NETWORKING)
            util::get_and_reset_value(route_.count_, reset) +
#endif
            util::get_and_reset_value(bind_gid_.count_, reset) +
            util::get_and_reset_value(resolve_gid_.count_, reset) +
            util::get_and_reset_value(unbind_gid_.count_, reset) +
            util::get_and_reset_value(increment_credit_.count_, reset) +
            util::get_and_reset_value(decrement_credit_.count_, reset) +
            util::get_and_reset_value(allocate_.count_, reset) +
            util::get_and_reset_value(begin_migration_.count_, reset) +
            util::get_and_reset_value(end_migration_.count_, reset);
    }

    // access execution time counters
    std::int64_t primary_namespace::counter_data::get_bind_gid_time(bool reset)
    {
        return util::get_and_reset_value(bind_gid_.time_, reset);
    }

    std::int64_t primary_namespace::counter_data::get_resolve_gid_time(
        bool reset)
    {
        return util::get_and_reset_value(resolve_gid_.time_, reset);
    }

    std::int64_t primary_namespace::counter_data::get_unbind_gid_time(
        bool reset)
    {
        return util::get_and_reset_value(unbind_gid_.time_, reset);
    }

    std::int64_t primary_namespace::counter_data::get_increment_credit_time(
        bool reset)
    {
        return util::get_and_reset_value(increment_credit_.time_, reset);
    }

    std::int64_t primary_namespace::counter_data::get_decrement_credit_time(
        bool reset)
    {
        return util::get_and_reset_value(decrement_credit_.time_, reset);
    }

    std::int64_t primary_namespace::counter_data::get_allocate_time(bool reset)
    {
        return util::get_and_reset_value(allocate_.time_, reset);
    }

    std::int64_t primary_namespace::counter_data::get_begin_migration_time(
        bool reset)
    {
        return util::get_and_reset_value(begin_migration_.time_, reset);
    }

    std::int64_t primary_namespace::counter_data::get_end_migration_time(
        bool reset)
    {
        return util::get_and_reset_value(end_migration_.time_, reset);
    }

    std::int64_t primary_namespace::counter_data::get_overall_time(bool reset)
    {
        return
#if defined(HPX_HAVE_NETWORKING)
            util::get_and_reset_value(route_.time_, reset) +
#endif
            util::get_and_reset_value(bind_gid_.time_, reset) +
            util::get_and_reset_value(resolve_gid_.time_, reset) +
            util::get_and_reset_value(unbind_gid_.time_, reset) +
            util::get_and_reset_value(increment_credit_.time_, reset) +
            util::get_and_reset_value(decrement_credit_.time_, reset) +
            util::get_and_reset_value(allocate_.time_, reset) +
            util::get_and_reset_value(begin_migration_.time_, reset) +
            util::get_and_reset_value(end_migration_.time_, reset);
    }

    void primary_namespace::counter_data::enable_all()
    {
#if defined(HPX_HAVE_NETWORKING)
        route_.enabled_ = true;
#endif
        bind_gid_.enabled_ = true;
        resolve_gid_.enabled_ = true;
        unbind_gid_.enabled_ = true;
        increment_credit_.enabled_ = true;
        decrement_credit_.enabled_ = true;
        allocate_.enabled_ = true;
        begin_migration_.enabled_ = true;
        end_migration_.enabled_ = true;
    }

    // increment counter values
    void primary_namespace::counter_data::increment_bind_gid_count()
    {
        if (bind_gid_.enabled_)
        {
            ++bind_gid_.count_;
        }
    }

    void primary_namespace::counter_data::increment_resolve_gid_count()
    {
        if (resolve_gid_.enabled_)
        {
            ++resolve_gid_.count_;
        }
    }

    void primary_namespace::counter_data::increment_unbind_gid_count()
    {
        if (unbind_gid_.enabled_)
        {
            ++unbind_gid_.count_;
        }
    }

    void primary_namespace::counter_data::increment_increment_credit_count()
    {
        if (increment_credit_.enabled_)
        {
            ++increment_credit_.count_;
        }
    }

    void primary_namespace::counter_data::increment_decrement_credit_count()
    {
        if (decrement_credit_.enabled_)
        {
            ++decrement_credit_.count_;
        }
    }

    void primary_namespace::counter_data::increment_allocate_count()
    {
        if (allocate_.enabled_)
        {
            ++allocate_.count_;
        }
    }

    void primary_namespace::counter_data::increment_begin_migration_count()
    {
        if (begin_migration_.enabled_)
        {
            ++begin_migration_.count_;
        }
    }

    void primary_namespace::counter_data::increment_end_migration_count()
    {
        if (end_migration_.enabled_)
        {
            ++end_migration_.count_;
        }
    }

#if defined(HPX_HAVE_NETWORKING)
    std::int64_t primary_namespace::counter_data::get_route_count(bool reset)
    {
        return util::get_and_reset_value(route_.count_, reset);
    }

    std::int64_t primary_namespace::counter_data::get_route_time(bool reset)
    {
        return util::get_and_reset_value(route_.time_, reset);
    }

    void primary_namespace::counter_data::increment_route_count()
    {
        if (route_.enabled_)
        {
            ++route_.count_;
        }
    }
#endif
}    // namespace hpx::agas::server
