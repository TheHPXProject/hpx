//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/components/get_ptr.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/components/client_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>

#include <memory>

///////////////////////////////////////////////////////////////////////////////
namespace hpx {

    /// \cond NOINTERNAL
    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_EXPORT struct get_ptr_deleter
        {
            explicit get_ptr_deleter(hpx::id_type id) noexcept
              : id_(HPX_MOVE(id))
            {
            }

            template <typename Component>
            void operator()(Component* p)
            {
                id_ = hpx::invalid_id;    // release component
                traits::component_pin_support<Component>::unpin(p);
            }

            hpx::id_type id_;    // holds component alive
        };

        HPX_CXX_EXPORT struct get_ptr_no_unpin_deleter
        {
            explicit get_ptr_no_unpin_deleter(hpx::id_type id) noexcept
              : id_(HPX_MOVE(id))
            {
            }

            template <typename Component>
            void operator()(Component*)
            {
                id_ = hpx::invalid_id;    // release component
            }

            hpx::id_type id_;    // holds component alive
        };

        HPX_CXX_EXPORT struct get_ptr_for_migration_deleter
        {
            explicit get_ptr_for_migration_deleter(hpx::id_type id) noexcept
              : id_(HPX_MOVE(id))
            {
            }

            template <typename Component>
            void operator()(Component* p)
            {
                // delete the object once the shared_ptr goes out of scope
                auto const pin_count =
                    traits::component_pin_support<Component>::pin_count(p);
                if (pin_count == ~0x0u)
                {
                    components::component_type const type =
                        components::get_component_type<Component>();
                    components::deleter(type)(id_.get_gid(),
                        naming::address(naming::get_gid_from_locality_id(
                                            agas::get_locality_id()),
                            type, p));
                }
                id_ = hpx::invalid_id;    // release credits
            }

            hpx::id_type id_;    // holds component alive
        };

        HPX_CXX_EXPORT template <typename Component, typename Deleter>
        std::shared_ptr<Component> get_ptr_postproc(naming::address const& addr,
            hpx::id_type const& id, bool pin_object = true)
        {
            if (agas::get_locality_id() !=
                naming::get_locality_id_from_gid(addr.locality_))
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::get_ptr_postproc<Component, Deleter>",
                    "the given component id does not belong to a local object");
            }

            if (!traits::component_type_is_compatible<Component>::call(addr))
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_component_type,
                    "hpx::get_ptr_postproc<Component, Deleter>",
                    "requested component type does not match the given "
                    "component id");
            }

            Component* p = get_lva<Component>::call(addr.address_);
            std::shared_ptr<Component> ptr(p, Deleter(id));

            // the shared_ptr pins the component, if requested
            if (pin_object)
            {
                traits::component_pin_support<Component>::pin(ptr.get());
            }
            return ptr;
        }

        ///////////////////////////////////////////////////////////////////////
        // This is similar to get_ptr<> below, except that the shared_ptr will
        // delete the local instance when it goes out of scope.
        HPX_CXX_EXPORT template <typename Component>
        std::shared_ptr<Component> get_ptr_for_migration(
            naming::address const& addr, hpx::id_type const& id)
        {
            // do not pin/unpin the object
            return get_ptr_postproc<Component, get_ptr_for_migration_deleter>(
                addr, id, false);
        }

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_EXPORT template <typename Component>
        std::shared_ptr<Component> resolve_get_ptr_local(hpx::id_type const& id)
        {
            return std::shared_ptr<Component>(
                get_lva<Component>::call(
                    reinterpret_cast<hpx::naming::address_type>(id.get_lsb())),
                detail::get_ptr_no_unpin_deleter(id));
        }

        HPX_CXX_EXPORT template <typename Component>
        hpx::future_or_value<std::shared_ptr<Component>> get_ptr(
            hpx::id_type const& id)
        {
            hpx::error_code ec(hpx::throwmode::lightweight);

            // shortcut for local, non-migratable objects
            naming::gid_type const gid = id.get_gid();
            if (naming::refers_to_local_lva(gid) &&
                naming::get_locality_id_from_gid(gid) ==
                    agas::get_locality_id(ec) &&
                !ec)
            {
                return resolve_get_ptr_local<Component>(id);
            }

            if (ec)
            {
                return hpx::make_exceptional_future<std::shared_ptr<Component>>(
                    hpx::detail::access_exception(ec));
            }

            auto result = agas::resolve_async(id);
            if (result.has_value())
            {
                try
                {
                    return get_ptr_postproc<Component, get_ptr_deleter>(
                        HPX_MOVE(result).get_value(), id);
                }
                catch (...)
                {
                    return hpx::make_exceptional_future<
                        std::shared_ptr<Component>>(std::current_exception());
                }
            }

            return result.get_future().then(hpx::launch::sync,
                [=](hpx::future<naming::address>&& f)
                    -> std::shared_ptr<Component> {
                    return get_ptr_postproc<Component, get_ptr_deleter>(
                        f.get(), id);
                });
        }
    }    // namespace detail
    /// \endcond

    /// \brief Returns a future referring to the pointer to the
    ///  underlying memory of a component
    ///
    /// The function hpx::get_ptr can be used to extract a future
    /// referring to the pointer to the underlying memory of a given component.
    ///
    /// \param id  [in] The global id of the component for which the pointer
    ///            to the underlying memory should be retrieved.
    ///
    /// \tparam    Component The type of the server side component.
    ///
    /// \returns   This function returns a future representing the pointer to
    ///            the underlying memory for the component instance with the
    ///            given \a id.
    ///
    /// \note      This function will successfully return the requested result
    ///            only if the given component is currently located on the
    ///            calling locality. Otherwise, the function will raise an
    ///            error.
    ///
    /// \note      The component instance the returned pointer refers to can
    ///            not be migrated as long as there is at least one copy of the
    ///            returned shared_ptr alive.
    ///
    HPX_CXX_EXPORT template <typename Component>
    hpx::future<std::shared_ptr<Component>> get_ptr(hpx::id_type const& id)
    {
        auto result = detail::get_ptr<Component>(id);
        if (result.has_value())
        {
            return hpx::make_ready_future(HPX_MOVE(result).get_value());
        }

        auto f = HPX_MOVE(result).get_future();
        return f;
    }

    /// \brief Returns a future referring to the pointer to the
    ///  underlying memory of a component
    ///
    /// The function hpx::get_ptr can be used to extract a future
    /// referring to the pointer to the underlying memory of a given component.
    ///
    /// \param c   [in] A client side representation of the component for which
    ///            the pointer to the underlying memory should be retrieved.
    ///
    /// \returns   This function returns a future representing the pointer to
    ///            the underlying memory for the component instance with the
    ///            given \a id.
    ///
    /// \note      This function will successfully return the requested result
    ///            only if the given component is currently located on the
    ///            calling locality. Otherwise, the function will raise an
    ///            error.
    ///
    /// \note      The component instance the returned pointer refers to can
    ///            not be migrated as long as there is at least one copy of the
    ///            returned shared_ptr alive.
    ///
    HPX_CXX_EXPORT template <typename Derived, typename Stub, typename Data>
    hpx::future<std::shared_ptr<typename components::client_base<Derived, Stub,
        Data>::server_component_type>>
    get_ptr(components::client_base<Derived, Stub, Data> const& c)
    {
        using component_type = typename components::client_base<Derived, Stub,
            Data>::server_component_type;

        return get_ptr<component_type>(c.get_id());
    }

    /// \brief Returns the pointer to the underlying memory of a component
    ///
    /// The function hpx::get_ptr_sync can be used to extract the pointer to
    /// the underlying memory of a given component.
    ///
    /// \param p   [in] The parameter \a p represents a placeholder type to
    ///            turn make the call synchronous.
    /// \param id  [in] The global id of the component for which the pointer
    ///            to the underlying memory should be retrieved.
    /// \param ec  [in,out] this represents the error status on exit, if this
    ///            is pre-initialized to \a hpx#throws the function will throw
    ///            on error instead.
    ///
    /// \tparam    Component The only template parameter has to be the type of
    ///            the server side component.
    ///
    /// \returns   This function returns the pointer to the underlying memory
    ///            for the component instance with the given \a id.
    ///
    /// \note      This function will successfully return the requested result
    ///            only if the given component is currently located on the
    ///            requesting locality. Otherwise, the function will raise and
    ///            error.
    ///
    /// \note      The component instance the returned pointer refers to can
    ///            not be migrated as long as there is at least one copy of the
    ///            returned shared_ptr alive.
    ///
    /// \note      As long as \a ec is not pre-initialized to \a hpx::throws this
    ///            function doesn't throw but returns the result code using the
    ///            parameter \a ec. Otherwise, it throws an instance of
    ///            hpx::exception.
    ///
#if defined(DOXYGEN)
    template <typename Component>
    std::shared_ptr<Component> get_ptr(
        launch::sync_policy p, hpx::id_type const& id, error_code& ec = throws);
#else
    HPX_CXX_EXPORT template <typename Component>
    std::shared_ptr<Component> get_ptr(
        launch::sync_policy, hpx::id_type const& id, error_code& ec = throws)
    {
        auto result = detail::get_ptr<Component>(id);
        if (result.has_value())
        {
            return HPX_MOVE(result).get_value();
        }
        return result.get_future().get(ec);
    }
#endif

    /// \brief Returns the pointer to the underlying memory of a component
    ///
    /// The function hpx::get_ptr_sync can be used to extract the pointer to
    /// the underlying memory of a given component.
    ///
    /// \param p   [in] The parameter \a p represents a placeholder type to
    ///            turn make the call synchronous.
    /// \param c   [in] A client side representation of the component for which
    ///            the pointer to the underlying memory should be retrieved.
    /// \param ec  [in,out] this represents the error status on exit, if this
    ///            is pre-initialized to \a hpx#throws the function will throw
    ///            on error instead.
    ///
    /// \returns   This function returns the pointer to the underlying memory
    ///            for the component instance with the given \a id.
    ///
    /// \note      This function will successfully return the requested result
    ///            only if the given component is currently located on the
    ///            requesting locality. Otherwise, the function will raise and
    ///            error.
    ///
    /// \note      The component instance the returned pointer refers to can
    ///            not be migrated as long as there is at least one copy of the
    ///            returned shared_ptr alive.
    ///
    /// \note      As long as \a ec is not pre-initialized to \a hpx::throws this
    ///            function doesn't throw but returns the result code using the
    ///            parameter \a ec. Otherwise, it throws an instance of
    ///            hpx::exception.
    ///
    HPX_CXX_EXPORT template <typename Derived, typename Stub, typename Data>
    std::shared_ptr<typename components::client_base<Derived, Stub,
        Data>::server_component_type>
    get_ptr(launch::sync_policy p,
        components::client_base<Derived, Stub, Data> const& c,
        error_code& ec = throws)
    {
        using component_type = typename components::client_base<Derived, Stub,
            Data>::server_component_type;

        return get_ptr<component_type>(p, c.get_id(), ec);
    }
}    // namespace hpx
