//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file new.hpp
/// \page hpx::new_
/// \headerfile hpx/components.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/components.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/runtime_components/create_component_helpers.hpp>

#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(DOXYGEN)
namespace hpx {
    /// \brief Create one or more new instances of the given Component type
    /// on the specified locality.
    ///
    /// This function creates one or more new instances of the given Component
    /// type on the specified locality and returns a future object for the
    /// global address which can be used to reference the new component
    /// instance.
    ///
    /// \param locality  [in] The global address of the locality where the
    ///                  new instance should be created on.
    /// \param vs        [in] Any number of arbitrary arguments (passed by
    ///                  value, by const reference or by rvalue reference)
    ///                  which will be forwarded to the constructor of
    ///                  the created component instance.
    ///
    /// \note    This function requires to specify an explicit template
    ///          argument which will define what type of component(s) to
    ///          create, for instance:
    ///          \code
    ///              hpx::future<hpx::id_type> f =
    ///                 hpx::new_<some_component>(hpx::find_here(), ...);
    ///              hpx::id_type id = f.get();
    ///          \endcode
    ///
    /// \returns The function returns different types depending on its use:\n
    ///          * If the explicit template argument \a Component represents a
    ///          component type (<code>traits::is_component<Component>::value</code>
    ///          evaluates to true), the function will return an \a hpx::future
    ///          object instance which can be used to retrieve the global
    ///          address of the newly created component.
    ///          * If the explicit template argument \a Component represents a
    ///          client side object (<code>traits::is_client<Component>::value</code>
    ///          evaluates to true), the function will return a new instance
    ///          of that type which can be used to refer to the newly created
    ///          component instance.
    ///
    template <typename Component, typename... Ts>
    auto new_(id_type const& locality, Ts&&... vs);

    /// \brief Create one new instance of the given Component type on the
    /// current locality.
    ///
    /// This function creates one new instance of the given Component
    /// type on the current locality and returns a future object for the
    /// global address which can be used to reference the new component
    /// instance.
    ///
    /// \param vs        [in] Any number of arbitrary arguments (passed by
    ///                  value, by const reference or by rvalue reference)
    ///                  which will be forwarded to the constructor of
    ///                  the created component instance.
    ///
    /// \note    This function requires to specify an explicit template
    ///          argument which will define what type of component(s) to
    ///          create, for instance:
    ///          \code
    ///              hpx::future<hpx::id_type> f =
    ///                 hpx::local_new<some_component>(...);
    ///              hpx::id_type id = f.get();
    ///          \endcode
    ///
    /// \returns The function returns different types depending on its use:\n
    ///          * If the explicit template argument \a Component represents a
    ///          component type (<code>traits::is_component<Component>::value</code>
    ///          evaluates to true), the function will return an \a hpx::future
    ///          object instance which can be used to retrieve the global
    ///          address of the newly created component. If the first argument
    ///          is `hpx::launch::sync` the function will directly return an
    ///          `hpx::id_type`.
    ///          * If the explicit template argument \a Component represents a
    ///          client side object (<code>traits::is_client<Component>::value</code>
    ///          evaluates to true), the function will return a new instance
    ///          of that type which can be used to refer to the newly created
    ///          component instance.
    ///
    /// \note    The difference of this function to \a hpx::new_ is that it can
    ///          be used in cases where the supplied arguments are non-copyable
    ///          and non-movable. All operations are guaranteed to be local
    ///          only.
    ///
    template <typename Component, typename... Ts>
    auto local_new(Ts&&... vs);

    /// \brief Create multiple new instances of the given Component type on the
    /// specified locality.
    ///
    /// This function creates multiple new instances of the given Component type
    /// on the specified locality and returns a future object for the
    /// global address which can be used to reference the new component
    /// instance.
    ///
    /// \param locality  [in] The global address of the locality where the
    ///                  new instance should be created on.
    /// \param count     [in] The number of component instances to create
    /// \param vs        [in] Any number of arbitrary arguments (passed by
    ///                  value, by const reference or by rvalue reference)
    ///                  which will be forwarded to the constructor of
    ///                  the created component instance.
    ///
    /// \note    This function requires to specify an explicit template
    ///          argument which will define what type of component(s) to
    ///          create, for instance:
    ///          \code
    ///              hpx::future<std::vector<hpx::id_type> > f =
    ///                 hpx::new_<some_component[]>(hpx::find_here(), 10, ...);
    ///              hpx::id_type id = f.get();
    ///          \endcode
    ///
    /// \returns The function returns different types depending on its use:\n
    ///          * If the explicit template argument \a Component
    ///          represents an array of a component type (i.e. \a Component[],
    ///          where <code>traits::is_component<Component>::value</code>
    ///          evaluates to true), the function will return an \a hpx::future
    ///          object instance which holds a std::vector<hpx::id_type>, where
    ///          each of the items in this vector is a global address of one
    ///          of the newly created components.
    ///          * If the explicit template argument \a Component
    ///          represents an array of a client side object type (i.e. \a Component[],
    ///          where <code>traits::is_client<Component>::value</code>
    ///          evaluates to true), the function will return an \a hpx::future
    ///          object instance which holds a std::vector<hpx::id_type>, where
    ///          each of the items in this vector is a client side instance of
    ///          the given type, each representing one of the newly created
    ///          components.
    ///
    template <typename Component, typename... Ts>
    auto new_(id_type const& locality, std::size_t count, Ts&&... vs);

    /// \brief Create one or more new instances of the given Component type
    /// based on the given distribution policy.
    ///
    /// This function creates one or more new instances of the given Component
    /// type on the localities defined by the given distribution policy and
    /// returns a future object for global address which can be used to reference
    /// the new component instance(s).
    ///
    /// \param policy    [in] The distribution policy used to decide where to
    ///                  place the newly created.
    /// \param vs        [in] Any number of arbitrary arguments (passed by
    ///                  value, by const reference or by rvalue reference)
    ///                  which will be forwarded to the constructor of
    ///                  the created component instance.
    ///
    /// \note    This function requires to specify an explicit template
    ///          argument which will define what type of component(s) to
    ///          create, for instance:
    ///          \code
    ///              hpx::future<hpx::id_type> f =
    ///                 hpx::new_<some_component>(hpx::default_layout, ...);
    ///              hpx::id_type id = f.get();
    ///          \endcode
    ///
    /// \returns The function returns different types depending on its use:\n
    ///          * If the explicit template argument \a Component represents a
    ///          component type (<code>traits::is_component<Component>::value</code>
    ///          evaluates to true), the function will return an \a hpx::future
    ///          object instance which can be used to retrieve the global
    ///          address of the newly created component.
    ///          * If the explicit template argument \a Component represents a
    ///          client side object (<code>traits::is_client<Component>::value</code>
    ///          evaluates to true), the function will return a new instance
    ///          of that type which can be used to refer to the newly created
    ///          component instance.
    ///
    template <typename Component, typename DistPolicy, typename... Ts>
    auto new_(DistPolicy const& policy, Ts&&... vs);

    /// \brief Create multiple new instances of the given Component type on the
    /// localities as defined by the given distribution policy.
    ///
    /// This function creates multiple new instances of the given Component type
    /// on the localities defined by the given distribution policy and returns
    /// a future object for the global address which can be used to reference
    /// the new component instance.
    ///
    /// \param policy    [in] The distribution policy used to decide where to
    ///                  place the newly created.
    /// \param count     [in] The number of component instances to create
    /// \param vs        [in] Any number of arbitrary arguments (passed by
    ///                  value, by const reference or by rvalue reference)
    ///                  which will be forwarded to the constructor of
    ///                  the created component instance.
    ///
    /// \note    This function requires to specify an explicit template
    ///          argument which will define what type of component(s) to
    ///          create, for instance:
    ///          \code
    ///              hpx::future<std::vector<hpx::id_type> > f =
    ///                 hpx::new_<some_component[]>(hpx::default_layout, 10, ...);
    ///              hpx::id_type id = f.get();
    ///          \endcode
    ///
    /// \returns The function returns different types depending on its use:\n
    ///          * If the explicit template argument \a Component
    ///          represents an array of a component type (i.e. \a Component[],
    ///          where <code>traits::is_component<Component>::value</code>
    ///          evaluates to true), the function will return an \a hpx::future
    ///          object instance which holds a std::vector<hpx::id_type>, where
    ///          each of the items in this vector is a global address of one
    ///          of the newly created components.\n
    ///          * If the explicit template argument \a Component
    ///          represents an array of a client side object type (i.e. \a Component[],
    ///          where <code>traits::is_client<Component>::value</code>
    ///          evaluates to true), the function will return an \a hpx::future
    ///          object instance which holds a std::vector<hpx::id_type>, where
    ///          each of the items in this vector is a client side instance of
    ///          the given type, each representing one of the newly created
    ///          components.
    ///
    template <typename Component, typename DistPolicy, typename... Ts>
    auto new_(DistPolicy const& policy, std::size_t count, Ts&&... vs);
}    // namespace hpx

#else

namespace hpx::components {

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        // create a single instance of a component
        template <typename Component>
        struct new_component
        {
            using type = hpx::future<hpx::id_type>;

            template <typename... Ts>
            static type call(hpx::id_type const& locality, Ts&&... vs)
            {
                return components::create_async<Component>(
                    locality, HPX_FORWARD(Ts, vs)...);
            }

            template <typename DistPolicy, typename... Ts>
            static type call(DistPolicy const& policy, Ts&&... vs)
            {
                return policy.template create<Component>(
                    HPX_FORWARD(Ts, vs)...);
            }
        };

        // create multiple component instances
        template <typename Component>
        struct new_component<Component[]>
        {
            using type = hpx::future<std::vector<hpx::id_type>>;

            template <typename... Ts>
            static type call(
                hpx::id_type const& locality, std::size_t count, Ts&&... vs)
            {
                return components::bulk_create_async<false, Component>(
                    locality, count, HPX_FORWARD(Ts, vs)...);
            }

            template <typename DistPolicy, typename... Ts>
            static type call(
                DistPolicy const& policy, std::size_t count, Ts&&... vs)
            {
                using bulk_locality_result =
                    typename DistPolicy::bulk_locality_result;

                hpx::future<std::vector<bulk_locality_result>> f =
                    policy.template bulk_create<false, Component>(
                        count, HPX_FORWARD(Ts, vs)...);

                return f.then(launch::sync,
                    [count](hpx::future<std::vector<bulk_locality_result>>&& f)
                        -> std::vector<hpx::id_type> {
                        std::vector<hpx::id_type> result;
                        result.reserve(count);

                        for (bulk_locality_result& r : f.get())
                        {
                            std::move(r.second.begin(), r.second.end(),
                                std::back_inserter(result));
                        }
                        return result;
                    });
            }
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Component>
        struct local_new_component
        {
            using type = hpx::future<hpx::id_type>;

            template <typename... Ts>
            static type call(Ts&&... ts)
            {
                using component_type = Component::wrapping_type;

                hpx::id_type id(components::server::create<component_type>(
                                    HPX_FORWARD(Ts, ts)...),
                    hpx::id_type::management_type::managed);
                return hpx::make_ready_future(HPX_MOVE(id));
            }
        };

        template <typename Component>
        struct local_new_component<Component[]>
        {
            using type = hpx::future<std::vector<hpx::id_type>>;

            template <typename... Ts>
            static type call(std::size_t count, Ts&&... ts)
            {
                using component_type = Component::wrapping_type;

                std::vector<hpx::id_type> result;
                result.reserve(count);

                for (std::size_t i = 0; i != count; ++i)
                {
                    result.emplace_back(
                        components::server::create<component_type>(ts...),
                        hpx::id_type::management_type::managed);
                }

                return hpx::make_ready_future(result);
            }
        };

        ///////////////////////////////////////////////////////////////////////
        // same as above, just fully synchronous
        template <typename Component>
        struct local_new_component_sync
        {
            using type = hpx::id_type;

            template <typename... Ts>
            static type call(Ts&&... ts)
            {
                using component_type = Component::wrapping_type;

                hpx::id_type id(components::server::create<component_type>(
                                    HPX_FORWARD(Ts, ts)...),
                    hpx::id_type::management_type::managed);

                return id;
            }
        };

        template <typename Component>
        struct local_new_component_sync<Component[]>
        {
            using type = std::vector<hpx::id_type>;

            template <typename... Ts>
            static type call(std::size_t count, Ts&&... ts)
            {
                using component_type = Component::wrapping_type;

                std::vector<hpx::id_type> result;
                result.reserve(count);

                for (std::size_t i = 0; i != count; ++i)
                {
                    result.emplace_back(
                        components::server::create<component_type>(ts...),
                        hpx::id_type::management_type::managed);
                }

                return result;
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Component, typename... Ts>
        requires(traits::is_component_or_component_array_v<Component>)
    decltype(auto) new_(id_type const& locality, Ts&&... vs)
    {
        if (naming::get_locality_id_from_id(locality) ==
            agas::get_locality_id())
        {
            return detail::local_new_component<Component>::call(
                HPX_FORWARD(Ts, vs)...);
        }

        return detail::new_component<Component>::call(
            locality, HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Component, typename DistPolicy,
        typename... Ts>
        requires(traits::is_component_or_component_array_v<Component> &&
            traits::is_distribution_policy_v<DistPolicy>)
    decltype(auto) new_(DistPolicy const& policy, Ts&&... vs)
    {
        return detail::new_component<Component>::call(
            policy, HPX_FORWARD(Ts, vs)...);
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        // create a single instance of a component
        template <typename Client>
        struct new_client
        {
            using type = Client;
            using component_type = Client::server_component_type;

            template <typename... Ts>
            static type call(hpx::id_type const& locality, Ts&&... vs)
            {
                return make_client<Client>(
                    components::create_async<component_type>(
                        locality, HPX_FORWARD(Ts, vs)...));
            }

            template <typename DistPolicy, typename... Ts>
            static type call(DistPolicy const& policy, Ts&&... vs)
            {
                return make_client<Client>(
                    policy.template create<component_type>(
                        HPX_FORWARD(Ts, vs)...));
            }
        };

        // create multiple client instances
        template <typename Client>
        struct new_client<Client[]>
        {
            using type = hpx::future<std::vector<Client>>;
            using component_type = Client::server_component_type;

            template <typename... Ts>
            static type call(Ts&&... vs)
            {
                return new_component<component_type[]>::call(
                    HPX_FORWARD(Ts, vs)...)
                    .then(hpx::launch::sync,
                        [](hpx::future<std::vector<hpx::id_type>>&& v)
                            -> std::vector<Client> {
                            return make_clients<Client>(v.get());
                        });
            }
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Client>
        struct local_new_client
        {
            using type = Client;
            using component_type = Client::server_component_type::wrapping_type;

            template <typename... Ts>
            static type call(Ts&&... ts)
            {
                hpx::id_type id(components::server::create<component_type>(
                                    HPX_FORWARD(Ts, ts)...),
                    hpx::id_type::management_type::managed);
                return make_client<Client>(HPX_MOVE(id));
            }
        };

        template <typename Client>
        struct local_new_client<Client[]>
        {
            using type = hpx::future<std::vector<Client>>;
            using component_type = Client::server_component_type::wrapping_type;

            template <typename... Ts>
            static type call(Ts&&... ts)
            {
                return local_new_component<component_type[]>::call(
                    HPX_FORWARD(Ts, ts)...)
                    .then(hpx::launch::sync,
                        [](hpx::future<std::vector<hpx::id_type>>&& v)
                            -> std::vector<Client> {
                            return make_clients<Client>(v.get());
                        });
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Client, typename... Ts>
        requires(traits::is_client_or_client_array_v<Client>)
    decltype(auto) new_(id_type const& locality, Ts&&... vs)
    {
        if (naming::get_locality_id_from_id(locality) ==
            agas::get_locality_id())
        {
            return detail::local_new_client<Client>::call(
                HPX_FORWARD(Ts, vs)...);
        }

        return detail::new_client<Client>::call(
            locality, HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Client, typename DistPolicy,
        typename... Ts>
        requires(traits::is_client_or_client_array_v<Client> &&
            traits::is_distribution_policy_v<DistPolicy>)
    decltype(auto) new_(DistPolicy const& policy, Ts&&... vs)
    {
        return detail::new_client<Client>::call(policy, HPX_FORWARD(Ts, vs)...);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Same as above, but just on this locality. This does not go through an
    // action, that means that the constructor arguments can be non-copyable and
    // non-movable.
    HPX_CXX_EXPORT template <typename Component>
        requires(traits::is_component_or_component_array_v<Component>)
    decltype(auto) local_new()
    {
        return detail::local_new_component<Component>::call();
    }

    HPX_CXX_EXPORT template <typename Component, typename T1, typename... Ts>
        requires(traits::is_component_or_component_array_v<Component> &&
            !std::is_same_v<std::decay_t<T1>, launch::sync_policy>)
    decltype(auto) local_new(T1&& t1, Ts&&... ts)
    {
        return detail::local_new_component<Component>::call(
            HPX_FORWARD(T1, t1), HPX_FORWARD(Ts, ts)...);
    }

    HPX_CXX_EXPORT template <typename Component, typename... Ts>
        requires(traits::is_component_or_component_array_v<Component>)
    decltype(auto) local_new(launch::sync_policy, Ts&&... ts)
    {
        return detail::local_new_component_sync<Component>::call(
            HPX_FORWARD(Ts, ts)...);
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Client, typename... Ts>
        requires(traits::is_client_or_client_array_v<Client>)
    decltype(auto) local_new(Ts&&... ts)
    {
        return detail::local_new_client<Client>::call(HPX_FORWARD(Ts, ts)...);
    }
}    // namespace hpx::components

namespace hpx {

    HPX_CXX_EXPORT using hpx::components::local_new;
    HPX_CXX_EXPORT using hpx::components::new_;
}    // namespace hpx

#endif
