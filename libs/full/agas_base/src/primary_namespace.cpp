////////////////////////////////////////////////////////////////////////////////
//  Copyright (c) 2016 Thomas Heller
//  Copyright (c) 2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////////////////////////////////////////////////////////////////////////////////

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/agas_base/primary_namespace.hpp>
#include <hpx/agas_base/server/primary_namespace.hpp>

#include <cstdint>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

using hpx::agas::server::primary_namespace;

HPX_DEFINE_COMPONENT_NAME(primary_namespace, hpx_primary_namespace)
HPX_DEFINE_GET_COMPONENT_TYPE_STATIC(primary_namespace,
    to_int(components::component_enum_type::agas_primary_namespace))

HPX_REGISTER_ACTION_ID(primary_namespace::allocate_action,
    primary_namespace_allocate_action,
    hpx::actions::primary_namespace_allocate_action_id)

HPX_REGISTER_ACTION_ID(primary_namespace::bind_gid_action,
    primary_namespace_bind_gid_action,
    hpx::actions::primary_namespace_bind_gid_action_id)

HPX_REGISTER_ACTION_ID(primary_namespace::begin_migration_action,
    primary_namespace_begin_migration_action,
    hpx::actions::primary_namespace_begin_migration_action_id)

HPX_REGISTER_ACTION_ID(primary_namespace::end_migration_action,
    primary_namespace_end_migration_action,
    hpx::actions::primary_namespace_end_migration_action_id)

HPX_REGISTER_ACTION_ID(primary_namespace::decrement_credit_action,
    primary_namespace_decrement_credit_action,
    hpx::actions::primary_namespace_decrement_credit_action_id)

HPX_REGISTER_ACTION_ID(primary_namespace::increment_credit_action,
    primary_namespace_increment_credit_action,
    hpx::actions::primary_namespace_increment_credit_action_id)

HPX_REGISTER_ACTION_ID(primary_namespace::resolve_gid_action,
    primary_namespace_resolve_gid_action,
    hpx::actions::primary_namespace_resolve_gid_action_id)

HPX_REGISTER_ACTION_ID(primary_namespace::colocate_action,
    primary_namespace_colocate_action,
    hpx::actions::primary_namespace_colocate_action_id)

HPX_REGISTER_ACTION_ID(primary_namespace::unbind_gid_action,
    primary_namespace_unbind_gid_action,
    hpx::actions::primary_namespace_unbind_gid_action_id)

#if defined(HPX_HAVE_NETWORKING)
HPX_REGISTER_ACTION_ID(primary_namespace::route_action,
    primary_namespace_route_action,
    hpx::actions::primary_namespace_route_action_id)
#endif

HPX_REGISTER_BASE_LCO_WITH_VALUE_ID(hpx::naming::address, naming_address,
    hpx::actions::base_lco_with_value_naming_address_get,
    hpx::actions::base_lco_with_value_naming_address_set)
HPX_REGISTER_BASE_LCO_WITH_VALUE_ID(gva_tuple_type, gva_tuple,
    hpx::actions::base_lco_with_value_gva_tuple_get,
    hpx::actions::base_lco_with_value_gva_tuple_set)
HPX_REGISTER_BASE_LCO_WITH_VALUE_ID(std_pair_address_id_type,
    std_pair_address_id_type,
    hpx::actions::base_lco_with_value_std_pair_address_id_type_get,
    hpx::actions::base_lco_with_value_std_pair_address_id_type_set)
HPX_REGISTER_BASE_LCO_WITH_VALUE_ID(std_pair_gid_type, std_pair_gid_type,
    hpx::actions::base_lco_with_value_std_pair_gid_type_get,
    hpx::actions::base_lco_with_value_std_pair_gid_type_set)
HPX_REGISTER_BASE_LCO_WITH_VALUE_ID(std::vector<std::int64_t>,
    vector_std_int64_type,
    hpx::actions::base_lco_with_value_vector_std_int64_get,
    hpx::actions::base_lco_with_value_vector_std_int64_set)

namespace hpx { namespace agas {

    naming::gid_type primary_namespace::get_service_instance(
        std::uint32_t service_locality_id)
    {
        naming::gid_type service(agas::primary_ns_msb, agas::primary_ns_lsb);
        return naming::replace_locality_id(service, service_locality_id);
    }

    naming::gid_type primary_namespace::get_service_instance(
        naming::gid_type const& dest, error_code& ec)
    {
        std::uint32_t service_locality_id =
            naming::get_locality_id_from_gid(dest);
        if (service_locality_id == naming::invalid_locality_id)
        {
            HPX_THROWS_IF(ec, hpx::error::bad_parameter,
                "primary_namespace::get_service_instance",
                "can't retrieve a valid locality id from global address "
                "({1}): ",
                dest);
            return naming::gid_type();
        }
        return get_service_instance(service_locality_id);
    }

    bool primary_namespace::is_service_instance(naming::gid_type const& gid)
    {
        return gid.get_lsb() == agas::primary_ns_lsb &&
            (gid.get_msb() & ~naming::gid_type::locality_id_mask) ==
            (agas::primary_ns_msb & ~naming::gid_type::locality_id_mask);
    }

    primary_namespace::primary_namespace()
      : server_(new server::primary_namespace())
    {
    }

    primary_namespace::~primary_namespace() = default;

    naming::address::address_type primary_namespace::ptr() const
    {
        return reinterpret_cast<naming::address::address_type>(server_.get());
    }

    naming::address primary_namespace::addr() const
    {
        return naming::address(agas::get_locality(),
            to_int(components::component_enum_type::agas_primary_namespace),
            this->ptr());
    }

    hpx::id_type primary_namespace::gid() const
    {
        return hpx::id_type(get_service_instance(agas::get_locality()),
            hpx::id_type::management_type::unmanaged);
    }

    hpx::future<std::pair<hpx::id_type, naming::address>>
    primary_namespace::begin_migration(naming::gid_type const& id)
    {
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        hpx::id_type dest = hpx::id_type(
            get_service_instance(id), hpx::id_type::management_type::unmanaged);

        if (naming::get_locality_id_from_gid(dest.get_gid()) ==
            agas::get_locality_id())
        {
            return hpx::make_ready_future(server_->begin_migration(id));
        }

        server::primary_namespace::begin_migration_action action;
        return hpx::async(action, HPX_MOVE(dest), id);
#else
        HPX_ASSERT(false);
        HPX_UNUSED(id);
        return hpx::make_ready_future(
            std::pair<hpx::id_type, naming::address>{});
#endif
    }
    bool primary_namespace::end_migration(naming::gid_type const& id)
    {
        HPX_ASSERT(naming::get_locality_id_from_gid(get_service_instance(id)) ==
            agas::get_locality_id());

        return server_->end_migration(id);
    }

    bool primary_namespace::bind_gid(gva const& g, naming::gid_type const& id,
        naming::gid_type const& locality)
    {
        return server_->bind_gid(g, id, locality);
    }

    future<bool> primary_namespace::bind_gid_async(
        gva g, naming::gid_type id, naming::gid_type locality)
    {
        hpx::id_type dest = hpx::id_type(
            get_service_instance(id), hpx::id_type::management_type::unmanaged);
        if (naming::get_locality_id_from_gid(dest.get_gid()) ==
            agas::get_locality_id())
        {
            return hpx::make_ready_future(server_->bind_gid(g, id, locality));
        }
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        server::primary_namespace::bind_gid_action action;
        return hpx::async(action, HPX_MOVE(dest), g, id, locality);
#else
        HPX_ASSERT(false);
        return hpx::make_ready_future(true);
#endif
    }

#if defined(HPX_HAVE_NETWORKING)
    void primary_namespace::route(parcelset::parcel&& p,
        hpx::function<void(std::error_code const&, parcelset::parcel const&)>&&
            f)
    {
        // compose request
        naming::gid_type const& id = p.destination();
        hpx::id_type dest = hpx::id_type(
            get_service_instance(id), hpx::id_type::management_type::unmanaged);
        if (naming::get_locality_id_from_gid(dest.get_gid()) ==
            agas::get_locality_id())
        {
            hpx::post(
                &server::primary_namespace::route, server_.get(), HPX_MOVE(p));
            f(std::error_code(), parcelset::parcel());
            return;
        }

#if !defined(HPX_COMPUTE_DEVICE_CODE)
        server::primary_namespace::route_action action;
        hpx::post_cb(action, HPX_MOVE(dest), HPX_MOVE(f), HPX_MOVE(p));
#else
        HPX_ASSERT(false);
#endif
    }
#endif

    primary_namespace::resolved_type primary_namespace::resolve_gid(
        naming::gid_type const& id)
    {
        return server_->resolve_gid(id);
    }

    hpx::future_or_value<primary_namespace::resolved_type>
    primary_namespace::resolve_full(naming::gid_type id)
    {
        hpx::id_type dest = hpx::id_type(
            get_service_instance(id), hpx::id_type::management_type::unmanaged);

        if (naming::get_locality_id_from_id(dest) == agas::get_locality_id())
        {
            return server_->resolve_gid(id);
        }

#if !defined(HPX_COMPUTE_DEVICE_CODE)
        server::primary_namespace::resolve_gid_action action;
        return hpx::async(action, HPX_MOVE(dest), id);
#else
        HPX_ASSERT(false);
        return primary_namespace::resolved_type{};
#endif
    }

    hpx::future_or_value<id_type> primary_namespace::colocate(
        naming::gid_type id)
    {
        hpx::id_type dest = hpx::id_type(
            get_service_instance(id), hpx::id_type::management_type::unmanaged);

        if (naming::get_locality_id_from_id(dest) == agas::get_locality_id())
        {
            return server_->colocate(id);
        }

#if !defined(HPX_COMPUTE_DEVICE_CODE)
        server::primary_namespace::colocate_action action;
        return hpx::async(action, HPX_MOVE(dest), id);
#else
        HPX_ASSERT(false);
        return hpx::invalid_id;
#endif
    }

    future<naming::address> primary_namespace::unbind_gid_async(
        std::uint64_t count, naming::gid_type const& id)
    {
        hpx::id_type dest = hpx::id_type(
            get_service_instance(id), hpx::id_type::management_type::unmanaged);
        naming::gid_type stripped_id = naming::detail::get_stripped_gid(id);

        if (naming::get_locality_id_from_id(dest) == agas::get_locality_id())
        {
            return hpx::make_ready_future(
                server_->unbind_gid(count, stripped_id));
        }
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        server::primary_namespace::unbind_gid_action action;
        return hpx::async(action, HPX_MOVE(dest), count, stripped_id);
#else
        HPX_ASSERT(false);
        return hpx::make_ready_future(naming::address{});
#endif
    }

    naming::address primary_namespace::unbind_gid(
        std::uint64_t count, naming::gid_type const& id)
    {
        hpx::id_type dest = hpx::id_type(
            get_service_instance(id), hpx::id_type::management_type::unmanaged);
        naming::gid_type stripped_id = naming::detail::get_stripped_gid(id);

        if (naming::get_locality_id_from_id(dest) == agas::get_locality_id())
        {
            return server_->unbind_gid(count, stripped_id);
        }
#if !defined(HPX_COMPUTE_DEVICE_CODE)
        server::primary_namespace::unbind_gid_action action;
        return action(HPX_MOVE(dest), count, stripped_id);
#else
        HPX_ASSERT(false);
        return naming::address{};
#endif
    }

    future_or_value<std::int64_t> primary_namespace::increment_credit(
        std::int64_t credits, naming::gid_type lower, naming::gid_type upper)
    {
        hpx::id_type dest = hpx::id_type(get_service_instance(lower),
            hpx::id_type::management_type::unmanaged);

        if (naming::get_locality_id_from_id(dest) == agas::get_locality_id())
        {
            return server_->increment_credit(credits, lower, upper);
        }

#if !defined(HPX_COMPUTE_DEVICE_CODE)
        server::primary_namespace::increment_credit_action action;
        return hpx::async(action, HPX_MOVE(dest), credits, lower, upper);
#else
        HPX_ASSERT(false);
        return std::int64_t(-1);
#endif
    }

    std::pair<naming::gid_type, naming::gid_type> primary_namespace::allocate(
        std::uint64_t count)
    {
        return server_->allocate(count);
    }

    void primary_namespace::set_local_locality(naming::gid_type const& g)
    {
        server_->set_local_locality(g);
    }

    void primary_namespace::register_server_instance(std::uint32_t locality_id)
    {
        std::string str("locality#" + std::to_string(locality_id) + "/");
        server_->register_server_instance(str.c_str(), locality_id);
    }

    void primary_namespace::unregister_server_instance(error_code& ec)
    {
        server_->unregister_server_instance(ec);
    }
}}    // namespace hpx::agas
