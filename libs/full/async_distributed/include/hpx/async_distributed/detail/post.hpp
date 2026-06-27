//  Copyright (c) 2007-2023 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/async_distributed/continuation.hpp>
#include <hpx/async_distributed/detail/post_implementations_fwd.hpp>
#include <hpx/async_distributed/put_parcel_fwd.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/async_local.hpp>
#include <hpx/modules/components.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/tracing.hpp>
#if defined(HPX_HAVE_THREAD_PARENT_REFERENCE)
#include <hpx/modules/runtime_local.hpp>
#endif

#include <type_traits>
#include <utility>

namespace hpx {

    ///////////////////////////////////////////////////////////////////////////
    // Invoked by a running HPX-thread to post() an action to any resource
    namespace detail {

#if defined(HPX_HAVE_NETWORKING)
        ///////////////////////////////////////////////////////////////////////
        template <typename Action>
        naming::address&& complement_addr(naming::address& addr)
        {
            if (to_int(hpx::components::component_enum_type::invalid) ==
                addr.type_)
            {
                addr.type_ = components::get_component_type<
                    typename Action::component_type>();
            }
            return HPX_MOVE(addr);
        }

        template <typename Action, typename... Ts>
        bool put_parcel(hpx::id_type const& id, naming::address&& addr,
            hpx::launch policy, Ts&&... vs)
        {
            using action_type = hpx::traits::extract_action<Action>::type;
            action_type act;

            parcelset::put_parcel(id, complement_addr<action_type>(addr), act,
                policy, HPX_FORWARD(Ts, vs)...);

            return false;    // destinations are remote
        }

        template <typename Action, typename Continuation, typename... Ts>
        bool put_parcel_cont(hpx::id_type const& id, naming::address&& addr,
            hpx::launch policy, Continuation&& cont, Ts&&... vs)
        {
            using action_type = hpx::traits::extract_action<Action>::type;
            action_type act;

            parcelset::put_parcel(id, complement_addr<action_type>(addr),
                HPX_FORWARD(Continuation, cont), act, policy,
                HPX_FORWARD(Ts, vs)...);

            return false;    // destinations are remote
        }

        template <typename Action, typename... Ts>
        bool put_parcel_cb(hpx::id_type const& id, naming::address&& addr,
            hpx::launch policy, parcelset::write_handler_type const& cb,
            Ts&&... vs)
        {
            using action_type = hpx::traits::extract_action<Action>::type;
            action_type act;

            parcelset::put_parcel_cb(cb, id, complement_addr<action_type>(addr),
                act, policy, HPX_FORWARD(Ts, vs)...);

            return false;    // destinations are remote
        }

        template <typename Action, typename... Ts>
        bool put_parcel_cb(hpx::id_type const& id, naming::address&& addr,
            hpx::launch policy, parcelset::write_handler_type&& cb, Ts&&... vs)
        {
            using action_type = hpx::traits::extract_action<Action>::type;
            action_type act;

            parcelset::put_parcel_cb(HPX_MOVE(cb), id,
                complement_addr<action_type>(addr), act, policy,
                HPX_FORWARD(Ts, vs)...);

            return false;    // destinations are remote
        }

        template <typename Action, typename Continuation, typename... Ts>
        bool put_parcel_cont_cb(hpx::id_type const& id, naming::address&& addr,
            hpx::launch policy, Continuation&& cont,
            parcelset::write_handler_type const& cb, Ts&&... vs)
        {
            using action_type = hpx::traits::extract_action<Action>::type;
            action_type act;

            parcelset::put_parcel_cb(cb, id, complement_addr<action_type>(addr),
                HPX_FORWARD(Continuation, cont), act, policy,
                HPX_FORWARD(Ts, vs)...);

            return false;    // destinations are remote
        }

        template <typename Action, typename Continuation, typename... Ts>
        bool put_parcel_cont_cb(hpx::id_type const& id, naming::address&& addr,
            hpx::launch policy, Continuation&& cont,
            parcelset::write_handler_type&& cb, Ts&&... vs)
        {
            using action_type = hpx::traits::extract_action<Action>::type;
            action_type act;

            parcelset::put_parcel_cb(HPX_MOVE(cb), id,
                complement_addr<action_type>(addr),
                HPX_FORWARD(Continuation, cont), act, policy,
                HPX_FORWARD(Ts, vs)...);

            return false;    // destinations are remote
        }

        // We know it is remote.
        template <typename Action, typename... Ts>
        bool post_r_p(naming::address&& addr, hpx::id_type const& id,
            hpx::launch policy, Ts&&... vs)
        {
            // If remote, create a new parcel to be sent to the destination
            // Create a new parcel with the gid, action, and arguments
            return detail::put_parcel<Action>(
                id, HPX_MOVE(addr), policy, HPX_FORWARD(Ts, vs)...);
        }

        template <typename Action, typename... Ts>
        bool post_r(naming::address&& addr, hpx::id_type const& gid, Ts&&... vs)
        {
            constexpr hpx::launch::async_policy policy(
                actions::action_priority<Action>(),
                actions::action_stacksize<Action>());
            return post_r_p<Action>(
                HPX_MOVE(addr), gid, policy, HPX_FORWARD(Ts, vs)...);
        }
#endif

        // We know it is local and has to be directly executed.
        template <typename Action, typename... Ts>
        bool post_l_p(hpx::id_type const& target, naming::address&& addr,
            hpx::launch policy, Ts&&... vs)
        {
            using action_type = hpx::traits::extract_action<Action>::type;

            HPX_ASSERT(traits::component_type_is_compatible<
                typename action_type::component_type>::call(addr));

            threads::thread_init_data data;
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            data.description = threads::thread_description(
                actions::detail::get_action_name<Action>(),
                actions::detail::get_action_name_tracing<Action>());
#endif
#if defined(HPX_HAVE_THREAD_PARENT_REFERENCE)
            data.parent_id = threads::get_self_id();
            data.parent_locality_id = get_locality_id();
#endif
            data.timer_data = threads::thread_init_data::setup_timer_data(data);
            data.priority = policy.priority();
            data.stacksize = policy.stacksize();

            post_helper<action_type>::call(HPX_MOVE(data), target,
                addr.address_, addr.type_, HPX_FORWARD(Ts, vs)...);
            return true;    // no parcel has been sent (dest is local)
        }

        // same as above, but taking all arguments by value
        template <typename Action, typename... Ts>
        bool post_l_p_val(hpx::id_type const& target, naming::address&& addr,
            hpx::launch policy, Ts... vs)
        {
            using action_type = hpx::traits::extract_action<Action>::type;

            HPX_ASSERT(traits::component_type_is_compatible<
                typename action_type::component_type>::call(addr));

            threads::thread_init_data data;
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            data.description = threads::thread_description(
                actions::detail::get_action_name<Action>(),
                actions::detail::get_action_name_tracing<Action>());
#endif
#if defined(HPX_HAVE_THREAD_PARENT_REFERENCE)
            data.parent_id = threads::get_self_id();
            data.parent_locality_id = get_locality_id();
#endif
            data.timer_data = threads::thread_init_data::setup_timer_data(data);
            data.priority = policy.priority();
            data.stacksize = policy.stacksize();

            post_helper<action_type>::call(HPX_MOVE(data), target,
                addr.address_, addr.type_, HPX_MOVE(vs)...);
            return true;    // no parcel has been sent (dest is local)
        }

        template <typename Action, typename... Ts>
        bool post_l(
            hpx::id_type const& target, naming::address&& addr, Ts&&... vs)
        {
            constexpr hpx::launch::async_policy policy(
                actions::action_priority<Action>(),
                actions::action_stacksize<Action>());
            return post_l_p<Action>(
                target, HPX_MOVE(addr), policy, HPX_FORWARD(Ts, vs)...);
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    bool post_p(hpx::id_type const& id, hpx::launch policy, Ts&&... vs)
    {
        return hpx::detail::post_impl<Action>(
            id, policy, HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Action, typename Client, typename Stub,
        typename Data, typename... Ts>
    bool post_p(components::client_base<Client, Stub, Data> const& c,
        hpx::launch policy, Ts&&... vs)
    {
        // make sure the action is compatible with the component type
        using component_type =
            components::client_base<Client, Stub, Data>::server_component_type;

        static_assert(traits::is_valid_action_v<Action, component_type>,
            "The action to invoke is not supported by the target");

        return hpx::detail::post_impl<Action>(
            c.get_id(), policy, HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Action, typename DistPolicy,
        typename... Ts>
        requires(traits::is_distribution_policy_v<DistPolicy>)
    bool post_p(DistPolicy const& policy, hpx::launch launch_policy, Ts&&... vs)
    {
        return policy.template apply<Action>(
            launch_policy, HPX_FORWARD(Ts, vs)...);
    }

    template <typename Action>
    struct detail::post_dispatch<Action,
        std::enable_if_t<traits::is_action<Action>::value>>
    {
        template <typename Component, typename Signature, typename Derived,
            typename... Ts>
        HPX_FORCEINLINE static bool call(
            hpx::actions::basic_action<Component, Signature, Derived>,
            hpx::id_type const& id, Ts&&... ts)
        {
            constexpr hpx::launch::async_policy policy(
                actions::action_priority<Action>(),
                actions::action_stacksize<Action>());
            return hpx::post_p<Derived>(id, policy, HPX_FORWARD(Ts, ts)...);
        }

        template <typename Component, typename Signature, typename Derived,
            typename Client, typename Stub, typename Data, typename... Ts>
        HPX_FORCEINLINE static bool call(
            hpx::actions::basic_action<Component, Signature, Derived>,
            components::client_base<Client, Stub, Data> const& c, Ts&&... ts)
        {
            // make sure the action is compatible with the component type
            using component_type = components::client_base<Client, Stub,
                Data>::server_component_type;

            static_assert(traits::is_valid_action_v<Derived, component_type>,
                "The action to invoke is not supported by the target");

            constexpr hpx::launch::async_policy policy(
                actions::action_priority<Action>(),
                actions::action_stacksize<Action>());
            return hpx::post_p<Derived>(
                c.get_id(), policy, HPX_FORWARD(Ts, ts)...);
        }

        template <typename Component, typename Signature, typename Derived,
            typename DistPolicy, typename... Ts>
        HPX_FORCEINLINE static std::enable_if_t<
            traits::is_distribution_policy_v<DistPolicy>, bool>
        call(hpx::actions::basic_action<Component, Signature, Derived>,
            DistPolicy const& policy, Ts&&... ts)
        {
            constexpr hpx::launch::async_policy launch_policy(
                actions::action_priority<Action>(),
                actions::action_stacksize<Action>());
            return hpx::post_p<Derived>(
                policy, launch_policy, HPX_FORWARD(Ts, ts)...);
        }
    };

    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    bool post(hpx::id_type const& id, Ts&&... vs)
    {
        constexpr hpx::launch::async_policy policy(
            actions::action_priority<Action>(),
            actions::action_stacksize<Action>());
        return hpx::post_p<Action>(id, policy, HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Action, typename Client, typename Stub,
        typename Data, typename... Ts>
    bool post(components::client_base<Client, Stub, Data> const& c, Ts&&... vs)
    {
        // make sure the action is compatible with the component type
        using component_type =
            components::client_base<Client, Stub, Data>::server_component_type;

        static_assert(traits::is_valid_action_v<Action, component_type>,
            "The action to invoke is not supported by the target");

        constexpr hpx::launch::async_policy policy(
            actions::action_priority<Action>(),
            actions::action_stacksize<Action>());
        return hpx::post_p<Action>(c.get_id(), policy, HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Action, typename DistPolicy,
        typename... Ts>
        requires(traits::is_distribution_policy_v<DistPolicy>)
    bool post(DistPolicy const& policy, Ts&&... vs)
    {
        constexpr hpx::launch::async_policy launch_policy(
            actions::action_priority<Action>(),
            actions::action_stacksize<Action>());
        return hpx::post_p<Action>(
            policy, launch_policy, HPX_FORWARD(Ts, vs)...);
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

#if defined(HPX_HAVE_NETWORKING)
        HPX_CXX_EXPORT template <typename Action, typename Continuation,
            typename... Ts>
        bool post_r_p(naming::address&& addr, Continuation&& c,
            hpx::id_type const& id, hpx::launch policy, Ts&&... vs)
        {
            // If remote, create a new parcel to be sent to the destination
            // Create a new parcel with the gid, action, and arguments
            return detail::put_parcel_cont<Action>(id, HPX_MOVE(addr), policy,
                HPX_FORWARD(Continuation, c), HPX_FORWARD(Ts, vs)...);
        }

        HPX_CXX_EXPORT template <typename Action, typename Continuation,
            typename... Ts>
            requires(traits::is_continuation_v<Continuation>)
        bool post_r(naming::address&& addr, Continuation&& c,
            hpx::id_type const& gid, Ts&&... vs)
        {
            constexpr hpx::launch::async_policy policy(
                actions::action_priority<Action>(),
                actions::action_stacksize<Action>());
            return post_r_p<Action>(HPX_MOVE(addr),
                HPX_FORWARD(Continuation, c), gid, policy,
                HPX_FORWARD(Ts, vs)...);
        }

        HPX_CXX_EXPORT template <typename Action>
        bool post_r_sync_p(
            naming::address&& addr, hpx::id_type const& id, hpx::launch policy)
        {
            using action_type_ = hpx::traits::extract_action<Action>::type;

            // If remote, create a new parcel to be sent to the destination
            // Create a new parcel with the gid, action, and arguments
            // Send the parcel through the parcel handler
            HPX_ASSERT(id.get_management_type() ==
                hpx::id_type::management_type::unmanaged);
            naming::gid_type gid = id.get_gid();
            parcelset::parcel p = parcelset::detail::create_parcel::call(
                HPX_MOVE(gid), complement_addr<action_type_>(addr),
                action_type_(), policy);

            parcelset::detail::parcel_await_apply(HPX_MOVE(p),
                parcelset::write_handler_type(), 0,
                [](parcelset::parcel&& p, parcelset::write_handler_type&&) {
                    hpx::parcelset::sync_put_parcel(HPX_MOVE(p));
                });

            return false;    // destination is remote
        }

        HPX_CXX_EXPORT template <typename Action>
        bool post_r_sync(naming::address&& addr, hpx::id_type const& gid)
        {
            constexpr hpx::launch::async_policy policy(
                actions::action_priority<Action>(),
                actions::action_stacksize<Action>());
            return post_r_sync_p<Action>(HPX_MOVE(addr), gid, policy);
        }
#endif

        // We know it is local and has to be directly executed.
        HPX_CXX_EXPORT template <typename Action, typename Continuation,
            typename... Ts>
        bool post_l_p(Continuation&& cont, hpx::id_type const& target,
            naming::address&& addr, hpx::launch policy, Ts&&... vs)
        {
            using action_type = hpx::traits::extract_action<Action>::type;

            HPX_ASSERT(traits::component_type_is_compatible<
                typename action_type::component_type>::call(addr));

            threads::thread_init_data data;
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            data.description = threads::thread_description(
                actions::detail::get_action_name<Action>(),
                actions::detail::get_action_name_tracing<Action>());
#endif
#if defined(HPX_HAVE_THREAD_PARENT_REFERENCE)
            data.parent_id = threads::get_self_id();
            data.parent_locality_id = get_locality_id();
#endif
            data.timer_data = threads::thread_init_data::setup_timer_data(data);
            data.priority = policy.priority();
            data.stacksize = policy.stacksize();

            post_helper<action_type>::call(HPX_MOVE(data),
                HPX_FORWARD(Continuation, cont), target, addr.address_,
                addr.type_, HPX_FORWARD(Ts, vs)...);
            return true;    // no parcel has been sent (dest is local)
        }

        HPX_CXX_EXPORT template <typename Action, typename Continuation,
            typename... Ts>
            requires(traits::is_continuation_v<Continuation>)
        bool post_l(Continuation&& c, hpx::id_type const& target,
            naming::address& addr, Ts&&... vs)
        {
            launch::async_policy policy(actions::action_priority<Action>(),
                actions::action_stacksize<Action>());
            return post_l_p<Action>(HPX_FORWARD(Continuation, c), target,
                HPX_MOVE(addr), policy, HPX_FORWARD(Ts, vs)...);
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename Continuation,
        typename... Ts>
        requires(traits::is_continuation_v<Continuation>)
    bool post_p(Continuation&& c, hpx::id_type const& gid, hpx::launch policy,
        Ts&&... vs)
    {
        return hpx::detail::post_impl<Action>(
            HPX_FORWARD(Continuation, c), gid, policy, HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Action, typename Continuation,
        typename Client, typename Stub, typename Data, typename... Ts>
        requires(traits::is_continuation_v<Continuation>)
    bool post_p(Continuation&& cont,
        components::client_base<Client, Stub, Data> const& c,
        hpx::launch policy, Ts&&... vs)
    {
        // make sure the action is compatible with the component type
        using component_type =
            components::client_base<Client, Stub, Data>::server_component_type;

        static_assert(traits::is_valid_action_v<Action, component_type>,
            "The action to invoke is not supported by the target");

        return hpx::detail::post_impl<Action>(HPX_FORWARD(Continuation, cont),
            c.get_id(), policy, HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Action, typename Continuation,
        typename DistPolicy, typename... Ts>
        requires(traits::is_continuation_v<Continuation> &&
            traits::is_distribution_policy_v<DistPolicy>)
    bool post_p(Continuation&& c, DistPolicy const& policy,
        hpx::launch launch_policy, Ts&&... vs)
    {
        return policy.template apply<Action>(HPX_FORWARD(Continuation, c),
            launch_policy, HPX_FORWARD(Ts, vs)...);
    }

    template <typename Continuation>
    struct detail::post_dispatch<Continuation,
        std::enable_if_t<traits::is_continuation<Continuation>::value>>
    {
        template <typename Component, typename Signature, typename Derived,
            typename... Ts>
        HPX_FORCEINLINE static bool call(Continuation&& c,
            hpx::actions::basic_action<Component, Signature, Derived>,
            hpx::id_type const& id, Ts&&... ts)
        {
            launch::async_policy policy(actions::action_priority<Derived>(),
                actions::action_stacksize<Derived>());
            return hpx::post_p<Derived>(HPX_FORWARD(Continuation, c), id,
                policy, HPX_FORWARD(Ts, ts)...);
        }

        template <typename Continuation_, typename Component,
            typename Signature, typename Derived, typename Client,
            typename Stub, typename Data, typename... Ts>
        HPX_FORCEINLINE static bool call(Continuation_&& cont,
            hpx::actions::basic_action<Component, Signature, Derived>,
            components::client_base<Client, Stub, Data> const& c, Ts&&... ts)
        {
            // make sure the action is compatible with the component type
            using component_type = components::client_base<Client, Stub,
                Data>::server_component_type;

            static_assert(traits::is_valid_action_v<Derived, component_type>,
                "The action to invoke is not supported by the target");

            launch::async_policy policy(actions::action_priority<Derived>(),
                actions::action_stacksize<Derived>());
            return hpx::post_p<Derived>(HPX_FORWARD(Continuation, cont),
                c.get_id(), policy, HPX_FORWARD(Ts, ts)...);
        }

        template <typename Component, typename Signature, typename Derived,
            typename DistPolicy, typename... Ts>
        HPX_FORCEINLINE static std::enable_if_t<
            traits::is_distribution_policy_v<DistPolicy>, bool>
        call(Continuation&& c,
            hpx::actions::basic_action<Component, Signature, Derived>,
            DistPolicy const& policy, Ts&&... ts)
        {
            launch::async_policy launch_policy(
                actions::action_priority<Derived>(),
                actions::action_stacksize<Derived>());
            return hpx::post_p<Derived>(HPX_FORWARD(Continuation, c), policy,
                launch_policy, HPX_FORWARD(Ts, ts)...);
        }
    };    // namespace detail

    HPX_CXX_EXPORT template <typename Action, typename Continuation,
        typename... Ts>
        requires(traits::is_continuation_v<Continuation>)
    bool post(Continuation&& c, hpx::id_type const& gid, Ts&&... vs)
    {
        launch::async_policy policy(actions::action_priority<Action>(),
            actions::action_stacksize<Action>());
        return hpx::post_p<Action>(
            HPX_FORWARD(Continuation, c), gid, policy, HPX_FORWARD(Ts, vs)...);
    }

    template <typename Action, typename Continuation, typename Client,
        typename Stub, typename Data, typename... Ts>
        requires(traits::is_continuation_v<Continuation>)
    bool post(Continuation&& cont,
        components::client_base<Client, Stub, Data> const& c, Ts&&... vs)
    {
        // make sure the action is compatible with the component type
        using component_type =
            components::client_base<Client, Stub, Data>::server_component_type;

        static_assert(traits::is_valid_action_v<Action, component_type>,
            "The action to invoke is not supported by the target");

        hpx::launch::async_policy policy(actions::action_priority<Action>(),
            actions::action_stacksize<Action>());
        return hpx::post_p<Action>(HPX_FORWARD(Continuation, cont), c.get_id(),
            policy, HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Action, typename Continuation,
        typename DistPolicy, typename... Ts>
        requires(traits::is_distribution_policy_v<DistPolicy> &&
            traits::is_continuation_v<Continuation>)
    bool post(Continuation&& c, DistPolicy const& policy, Ts&&... vs)
    {
        hpx::launch::async_policy launch_policy(
            actions::action_priority<Action>(),
            actions::action_stacksize<Action>());
        return hpx::post_p<Action>(HPX_FORWARD(Continuation, c), policy,
            launch_policy, HPX_FORWARD(Ts, vs)...);
    }

    ///////////////////////////////////////////////////////////////////////////
#if defined(HPX_HAVE_NETWORKING)
    namespace detail {

        HPX_CXX_EXPORT template <typename Action, typename... Ts>
        bool post_c_p(naming::address&& addr, hpx::id_type const& contgid,
            hpx::id_type const& gid, hpx::launch policy, Ts&&... vs)
        {
            using local_result_type =
                hpx::traits::extract_action<Action>::local_result_type;
            using remote_result_type =
                hpx::traits::extract_action<Action>::remote_result_type;

            return post_r_p<Action>(HPX_MOVE(addr),
                actions::typed_continuation<local_result_type,
                    remote_result_type>(contgid),
                gid, policy, HPX_FORWARD(Ts, vs)...);
        }

        HPX_CXX_EXPORT template <typename Action, typename... Ts>
        bool post_c(naming::address&& addr, hpx::id_type const& contgid,
            hpx::id_type const& gid, Ts&&... vs)
        {
            using local_result_type =
                hpx::traits::extract_action<Action>::local_result_type;
            using remote_result_type =
                hpx::traits::extract_action<Action>::remote_result_type;

            hpx::launch::async_policy policy(actions::action_priority<Action>(),
                actions::action_stacksize<Action>());
            return post_r_p<Action>(HPX_MOVE(addr),
                actions::typed_continuation<local_result_type,
                    remote_result_type>(contgid),
                gid, policy, HPX_FORWARD(Ts, vs)...);
        }
    }    // namespace detail
#endif

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    bool post_c_p(hpx::id_type const& contgid, hpx::id_type const& gid,
        hpx::launch policy, Ts&&... vs)
    {
        using local_result_type =
            hpx::traits::extract_action<Action>::local_result_type;
        using remote_result_type =
            hpx::traits::extract_action<Action>::remote_result_type;

        return hpx::post_p<Action>(
            actions::typed_continuation<local_result_type, remote_result_type>(
                contgid),
            gid, policy, HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    bool post_c(
        hpx::id_type const& contgid, hpx::id_type const& gid, Ts&&... vs)
    {
        using local_result_type =
            hpx::traits::extract_action<Action>::local_result_type;
        using remote_result_type =
            hpx::traits::extract_action<Action>::remote_result_type;

        hpx::launch::async_policy policy(actions::action_priority<Action>(),
            actions::action_stacksize<Action>());
        return hpx::post_p<Action>(
            actions::typed_continuation<local_result_type, remote_result_type>(
                contgid),
            gid, policy, HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Component, typename Signature,
        typename Derived, typename... Ts>
    bool post_c(
        hpx::actions::basic_action<Component, Signature, Derived> /*act*/,
        hpx::id_type const& contgid, hpx::id_type const& gid, Ts&&... vs)
    {
        using local_result_type =
            hpx::traits::extract_action<Derived>::local_result_type;
        using remote_result_type =
            hpx::traits::extract_action<Derived>::remote_result_type;

        hpx::launch::async_policy policy(actions::action_priority<Derived>(),
            actions::action_stacksize<Derived>());
        return hpx::post_p<Derived>(
            actions::typed_continuation<local_result_type, remote_result_type>(
                contgid),
            gid, policy, HPX_FORWARD(Ts, vs)...);
    }

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action, typename... Ts>
    HPX_DEPRECATED_V(1, 9, "hpx::apply is deprecated, use hpx::post instead")
    bool apply(Ts&&... ts)
    {
        return hpx::post<Action>(HPX_FORWARD(Ts, ts)...);
    }

    template <typename... Ts>
    HPX_DEPRECATED_V(1, 9, "hpx::apply is deprecated, use hpx::post instead")
    bool apply(Ts&&... ts)
    {
        return hpx::post(HPX_FORWARD(Ts, ts)...);
    }

    template <typename Action, typename... Ts>
    HPX_DEPRECATED_V(
        1, 9, "hpx::apply_p is deprecated, use hpx::post_p instead")
    bool apply_p(Ts&&... ts)
    {
        return hpx::post_p<Action>(HPX_FORWARD(Ts, ts)...);
    }

    template <typename... Ts>
    HPX_DEPRECATED_V(
        1, 9, "hpx::apply_p is deprecated, use hpx::post_p instead")
    bool apply_p(Ts&&... ts)
    {
        return hpx::post_p(HPX_FORWARD(Ts, ts)...);
    }

    template <typename Action, typename... Ts>
    HPX_DEPRECATED_V(
        1, 9, "hpx::apply_c is deprecated, use hpx::post_c instead")
    bool apply_c(Ts&&... ts)
    {
        return hpx::post_c<Action>(HPX_FORWARD(Ts, ts)...);
    }

    template <typename... Ts>
    HPX_DEPRECATED_V(
        1, 9, "hpx::apply_c is deprecated, use hpx::post_c instead")
    bool apply_c(Ts&&... ts)
    {
        return hpx::post_c(HPX_FORWARD(Ts, ts)...);
    }

    template <typename Action, typename... Ts>
    HPX_DEPRECATED_V(
        1, 9, "hpx::apply_c_p is deprecated, use hpx::post_c_p instead")
    bool apply_c_p(Ts&&... ts)
    {
        return hpx::post_c_p<Action>(HPX_FORWARD(Ts, ts)...);
    }

    template <typename... Ts>
    HPX_DEPRECATED_V(
        1, 9, "hpx::apply_c_p is deprecated, use hpx::post_c_p instead")
    bool apply_c_p(Ts&&... ts)
    {
        return hpx::post_c_p(HPX_FORWARD(Ts, ts)...);
    }
}    // namespace hpx

// these files are intentionally #included last as it refers to functions
// defined here
#include <hpx/async_distributed/detail/post_implementations.hpp>
#include <hpx/async_distributed/put_parcel.hpp>
