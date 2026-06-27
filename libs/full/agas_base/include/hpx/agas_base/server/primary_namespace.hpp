////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//  Copyright (c) 2012-2026 Hartmut Kaiser
//  Copyright (c) 2016 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/allocator_support.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/modules/synchronization.hpp>

#include <hpx/agas_base/agas_fwd.hpp>
#include <hpx/agas_base/gva.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::agas {

    HPX_CXX_EXPORT HPX_EXPORT naming::gid_type
    bootstrap_primary_namespace_gid();
    HPX_CXX_EXPORT HPX_EXPORT hpx::id_type bootstrap_primary_namespace_id();
}    // namespace hpx::agas

/// \brief AGAS's primary namespace maps 128-bit global identifiers (GIDs) to
/// resolved addresses.
///
/// \note The layout of the address space is implementation defined, and
/// subject to change. Never write application code that relies on the internal
/// layout of GIDs. AGAS only guarantees that all assigned GIDs will be unique.
///
/// The following is the canonical description of the partitioning of AGAS's
/// primary namespace.
///
///     |-----MSB------||------LSB-----|
///     BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB
///     |prefix||RC||----identifier----|
///
///     MSB        - Most significant bits (bit 64 to bit 127)
///     LSB        - Least significant bits (bit 0 to bit 63)
///     prefix     - Highest 32 bits (bit 96 to bit 127) of the MSB. Each
///                  locality is assigned a prefix. This creates a 96-bit
///                  address space for each locality.
///     RC         - Bit 88 to bit 92 of the MSB. This is the log2 of the number
///                  of reference counting credits on the GID.
///                  Bit 93 is used by the locking scheme for gid_types.
///                  Bit 94 is a flag which is set if the credit value is valid.
///                  Bit 95 is a flag that is set if a GID's credit count is
///                  ever split (e.g. if the GID is ever passed to another
///                  locality).
///                - Bit 87 marks the gid such that it will not be stored in
///                  any of the AGAS caches. This is used mainly for ids
///                  which represent 'one-shot' objects (like promises).
///     identifier - Bit 64 to bit 86 of the MSB, and the entire LSB. The
///                  content of these bits depends on the component type of
///                  the underlying object. For all user-defined components,
///                  these bits contain a unique 88-bit number which is
///                  assigned sequentially for each locality. For
///                  \a hpx#components#component_enum_type#runtime_support
///                  the high 24 bits are zeroed and the low 64 bits hold the
///                  LVA of the component.
///
/// The following address ranges are reserved. Some are either explicitly or
/// implicitly protected by AGAS. The letter x represents a single-byte
/// wild card.
///
///     00000000xxxxxxxxxxxxxxxxxxxxxxxx
///         Historically unused address space reserved for future use.
///     xxxxxxxxxxxx0000xxxxxxxxxxxxxxxx
///         Address space for LVA-encoded GIDs.
///     00000001xxxxxxxxxxxxxxxxxxxxxxxx
///         Prefix of the bootstrap AGAS locality.
///     00000001000000010000000000000001
///         Address of the primary_namespace component on the bootstrap AGAS
///         locality.
///     00000001000000010000000000000002
///         Address of the component_namespace component on the bootstrap AGAS
///         locality.
///     00000001000000010000000000000003
///         Address of the symbol_namespace component on the bootstrap AGAS
///         locality.
///     00000001000000010000000000000004
///         Address of the locality_namespace component on the bootstrap AGAS
///         locality.
///

namespace hpx::agas::server {

    // Base name used to register the component
    HPX_CXX_EXPORT inline constexpr char const* const
        primary_namespace_service_name = "primary/";

    HPX_CXX_EXPORT struct HPX_EXPORT primary_namespace
      : components::fixed_component_base<primary_namespace>
    {
        using mutex_type = hpx::spinlock;
        using base_type = components::fixed_component_base<primary_namespace>;

        using component_type = std::int32_t;

        using gva_table_data_type = std::pair<gva, naming::gid_type>;
        using gva_table_type = std::map<naming::gid_type, gva_table_data_type>;
        using refcnt_table_type = std::map<naming::gid_type, std::int64_t>;

        using resolved_type =
            hpx::tuple<naming::gid_type, gva, naming::gid_type>;

        mutex_type& mutex()
        {
            return mutex_;
        }

    private:
        // REVIEW: Separate mutexes might reduce contention here. This has to be
        // investigated carefully.
        mutex_type mutex_ = mutex_type("primary_namespace");
        gva_table_type gvas_;
        refcnt_table_type refcnts_;

        using migration_table_type = std::map<naming::gid_type,
            hpx::tuple<bool, std::size_t,
                lcos::local::detail::condition_variable>>;

        std::string instance_name_;
        naming::gid_type next_id_;     // next available gid
        naming::gid_type locality_;    // our locality id
        migration_table_type migrating_objects_;

    public:
        // data structure holding all counters for the component_namespace
        // component
        struct counter_data
        {
        public:
            counter_data(counter_data const&) = delete;
            counter_data(counter_data&&) = delete;
            counter_data& operator=(counter_data const&) = delete;
            counter_data& operator=(counter_data&&) = delete;

        public:
            struct api_counter_data
            {
                std::atomic<std::int64_t> count_ = 0;
                std::atomic<std::int64_t> time_ = 0;
                bool enabled_ = false;
            };

            counter_data() = default;

        public:
            // access current counter values
            std::int64_t get_bind_gid_count(bool);
            std::int64_t get_resolve_gid_count(bool);
            std::int64_t get_unbind_gid_count(bool);
            std::int64_t get_increment_credit_count(bool);
            std::int64_t get_decrement_credit_count(bool);
            std::int64_t get_allocate_count(bool);
            std::int64_t get_begin_migration_count(bool);
            std::int64_t get_end_migration_count(bool);
            std::int64_t get_overall_count(bool);

            std::int64_t get_bind_gid_time(bool);
            std::int64_t get_resolve_gid_time(bool);
            std::int64_t get_unbind_gid_time(bool);
            std::int64_t get_increment_credit_time(bool);
            std::int64_t get_decrement_credit_time(bool);
            std::int64_t get_allocate_time(bool);
            std::int64_t get_begin_migration_time(bool);
            std::int64_t get_end_migration_time(bool);
            std::int64_t get_overall_time(bool);

            // increment counter values
            void increment_bind_gid_count();
            void increment_resolve_gid_count();
            void increment_unbind_gid_count();
            void increment_increment_credit_count();
            void increment_decrement_credit_count();
            void increment_allocate_count();
            void increment_begin_migration_count();
            void increment_end_migration_count();

            void enable_all();

#if defined(HPX_HAVE_NETWORKING)
            std::int64_t get_route_count(bool);
            std::int64_t get_route_time(bool);
            void increment_route_count();
            api_counter_data route_;    // primary_ns_
#endif

            // primary_ns_bind_gid
            api_counter_data bind_gid_;
            // primary_ns_resolve_gid
            api_counter_data resolve_gid_;
            // primary_ns_unbind_gid
            api_counter_data unbind_gid_;
            // primary_ns_increment_credit
            api_counter_data increment_credit_;
            // primary_ns_decrement_credit
            api_counter_data decrement_credit_;
            // primary_ns_allocate
            api_counter_data allocate_;
            // primary_ns_begin_migration
            api_counter_data begin_migration_;
            // primary_ns_end_migration
            api_counter_data end_migration_;
        };

        counter_data counter_data_;

    private:
#if defined(HPX_HAVE_AGAS_DUMP_REFCNT_ENTRIES)
        /// Dump the credit counts of all matching ranges. Expects that \p l
        /// is locked.
        void dump_refcnt_matches(refcnt_table_type::iterator lower_it,
            refcnt_table_type::iterator upper_it, naming::gid_type const& lower,
            naming::gid_type const& upper, std::unique_lock<mutex_type>& l,
            char const* func_name);
#endif

    public:
        // helper function
        void wait_for_migration_locked(std::unique_lock<mutex_type>& l,
            naming::gid_type const& id, error_code& ec);

    public:
        primary_namespace()
          : base_type(agas::primary_ns_msb, agas::primary_ns_lsb)
          , next_id_(naming::invalid_gid)
          , locality_(naming::invalid_gid)
        {
        }

        void finalize() const;

        void set_local_locality(naming::gid_type const& g)
        {
            locality_ = g;
            next_id_ = naming::gid_type(g.get_msb() + 1, 0x1000);
        }

        void register_server_instance(char const* servicename,
            std::uint32_t locality_id = naming::invalid_locality_id,
            error_code& ec = throws);

        void unregister_server_instance(error_code& ec = throws) const;

#if defined(HPX_HAVE_NETWORKING)
        void route(parcelset::parcel&& p);
#endif

        bool bind_gid(gva const& g, naming::gid_type id,
            naming::gid_type const& locality);

        // API
        std::pair<hpx::id_type, naming::address> begin_migration(
            naming::gid_type id);
        bool end_migration(naming::gid_type const& id);

        resolved_type resolve_gid(naming::gid_type const& id);

        hpx::id_type colocate(naming::gid_type const& id);

        naming::address unbind_gid(std::uint64_t count, naming::gid_type id);

        std::int64_t increment_credit(std::int64_t credits,
            naming::gid_type lower, naming::gid_type upper);

        std::vector<std::int64_t> decrement_credit(
            std::vector<hpx::tuple<std::int64_t, naming::gid_type,
                naming::gid_type>> const& requests);

        std::pair<naming::gid_type, naming::gid_type> allocate(
            std::uint64_t count);

        resolved_type resolve_gid_locked(std::unique_lock<mutex_type>& l,
            naming::gid_type const& gid, error_code& ec);

    private:
        resolved_type resolve_gid_locked_non_local(
            std::unique_lock<mutex_type>& l, naming::gid_type const& gid,
            error_code& ec);

        void increment(naming::gid_type const& lower,
            naming::gid_type const& upper, std::int64_t const& credits,
            error_code& ec);

        ///////////////////////////////////////////////////////////////////////////
        struct free_entry
        {
            free_entry(agas::gva const& gva, naming::gid_type const& gid,
                naming::gid_type const& loc)
              : gva_(gva)
              , gid_(gid)
              , locality_(loc)
            {
            }

            agas::gva gva_;
            naming::gid_type gid_;
            naming::gid_type locality_;
        };

        using free_entry_allocator_type = util::internal_allocator<free_entry>;
        using free_entry_list_type =
            std::list<free_entry, free_entry_allocator_type>;

        void resolve_free_list(std::unique_lock<mutex_type>& l,
            std::list<refcnt_table_type::iterator> const& free_list,
            free_entry_list_type& free_entry_list,
            naming::gid_type const& lower, naming::gid_type const& upper,
            error_code& ec);

        void decrement_sweep(free_entry_list_type& free_list,
            naming::gid_type const& lower, naming::gid_type const& upper,
            std::int64_t credits, error_code& ec);

        void free_components_sync(free_entry_list_type const& free_list,
            naming::gid_type const& lower, naming::gid_type const& upper,
            error_code& ec) const;

    public:
        HPX_DEFINE_COMPONENT_ACTION(primary_namespace, allocate)
        HPX_DEFINE_COMPONENT_ACTION(primary_namespace, bind_gid)
        HPX_DEFINE_COMPONENT_ACTION(primary_namespace, colocate)
        HPX_DEFINE_COMPONENT_ACTION(primary_namespace, begin_migration)
        HPX_DEFINE_COMPONENT_ACTION(primary_namespace, end_migration)
        HPX_DEFINE_COMPONENT_ACTION(primary_namespace, decrement_credit)
        HPX_DEFINE_COMPONENT_ACTION(primary_namespace, increment_credit)
        HPX_DEFINE_COMPONENT_ACTION(primary_namespace, resolve_gid)
        HPX_DEFINE_COMPONENT_ACTION(primary_namespace, unbind_gid)
#if defined(HPX_HAVE_NETWORKING)
        HPX_DEFINE_COMPONENT_ACTION(primary_namespace, route)
#endif
    };
}    // namespace hpx::agas::server

HPX_ACTION_USES_MEDIUM_STACK(
    hpx::agas::server::primary_namespace::allocate_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::primary_namespace::allocate_action,
    primary_namespace_allocate_action)

HPX_ACTION_USES_MEDIUM_STACK(
    hpx::agas::server::primary_namespace::bind_gid_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::primary_namespace::bind_gid_action,
    primary_namespace_bind_gid_action)

HPX_ACTION_USES_MEDIUM_STACK(
    hpx::agas::server::primary_namespace::begin_migration_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::primary_namespace::begin_migration_action,
    primary_namespace_begin_migration_action)

HPX_ACTION_USES_MEDIUM_STACK(
    hpx::agas::server::primary_namespace::end_migration_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::primary_namespace::end_migration_action,
    primary_namespace_end_migration_action)

HPX_ACTION_USES_MEDIUM_STACK(
    hpx::agas::server::primary_namespace::decrement_credit_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::primary_namespace::decrement_credit_action,
    primary_namespace_decrement_credit_action)

HPX_ACTION_USES_MEDIUM_STACK(
    hpx::agas::server::primary_namespace::increment_credit_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::primary_namespace::increment_credit_action,
    primary_namespace_increment_credit_action)

HPX_ACTION_USES_MEDIUM_STACK(
    hpx::agas::server::primary_namespace::resolve_gid_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::primary_namespace::resolve_gid_action,
    primary_namespace_resolve_gid_action)

HPX_ACTION_USES_MEDIUM_STACK(
    hpx::agas::server::primary_namespace::colocate_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::primary_namespace::colocate_action,
    primary_namespace_colocate_action)

HPX_ACTION_USES_MEDIUM_STACK(
    hpx::agas::server::primary_namespace::unbind_gid_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::primary_namespace::unbind_gid_action,
    primary_namespace_unbind_gid_action)

#if defined(HPX_HAVE_NETWORKING)
HPX_ACTION_USES_MEDIUM_STACK(hpx::agas::server::primary_namespace::route_action)

HPX_REGISTER_ACTION_DECLARATION(
    hpx::agas::server::primary_namespace::route_action,
    primary_namespace_route_action)
#endif

HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(
    hpx::naming::address, naming_address)
typedef hpx::tuple<hpx::naming::gid_type, hpx::agas::gva, hpx::naming::gid_type>
    gva_tuple_type;
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(gva_tuple_type, gva_tuple)
typedef std::pair<hpx::id_type, hpx::naming::address> std_pair_address_id_type;
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(
    std_pair_address_id_type, std_pair_address_id_type)
typedef std::pair<hpx::naming::gid_type, hpx::naming::gid_type>
    std_pair_gid_type;
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(
    std_pair_gid_type, std_pair_gid_type)
HPX_REGISTER_BASE_LCO_WITH_VALUE_DECLARATION(
    std::vector<std::int64_t>, vector_std_int64_type)

#if !defined(HPX_COMPUTE_DEVICE_CODE) && defined(HPX_HAVE_NETWORKING)
// Parcel routing forwards the binary filter request to the routed action
template <>
struct hpx::traits::action_get_embedded_parcel<
    hpx::agas::server::primary_namespace::route_action>
{
    static hpx::optional<hpx::parcelset::parcel> call(
        hpx::actions::transfer_base_action<
            hpx::agas::server::primary_namespace::route_action> const& act)
    {
        auto p = hpx::actions::get<0>(act);
        return hpx::optional<hpx::parcelset::parcel>(HPX_MOVE(p));
    }
};    // namespace hpx::traits
#endif

#include <hpx/config/warnings_suffix.hpp>
