//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/async_distributed/detail/post.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>

namespace hpx {

#if defined(HPX_HAVE_NETWORKING)
    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_EXPORT template <typename Action, typename Callback,
            typename... Ts>
        bool post_r_p_cb(naming::address&& addr, hpx::id_type const& id,
            hpx::launch policy, Callback&& cb, Ts&&... vs)
        {
            // If remote, create a new parcel to be sent to the destination
            // Create a new parcel with the gid, action, and arguments
            return detail::put_parcel_cb<Action>(id, HPX_MOVE(addr), policy,
                HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
        }

        HPX_CXX_EXPORT template <typename Action, typename Callback,
            typename... Ts>
        bool post_r_cb(naming::address&& addr, hpx::id_type const& gid,
            Callback&& cb, Ts&&... vs)
        {
            constexpr launch::async_policy policy(
                actions::action_priority<Action>(),
                actions::action_stacksize<Action>());
            return post_r_p_cb<Action>(HPX_MOVE(addr), gid, policy,
                HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
        }
    }    // namespace detail
#endif

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename Callback, typename... Ts>
    bool post_p_cb(
        hpx::id_type const& gid, hpx::launch policy, Callback&& cb, Ts&&... vs)
    {
        return hpx::detail::post_cb_impl<Action>(
            gid, policy, HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Action, typename Callback, typename... Ts>
    bool post_cb(hpx::id_type const& gid, Callback&& cb, Ts&&... vs)
    {
        constexpr launch::async_policy policy(
            actions::action_priority<Action>(),
            actions::action_stacksize<Action>());
        return hpx::post_p_cb<Action>(
            gid, policy, HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Component, typename Signature,
        typename Derived, typename Callback, typename... Ts>
    bool post_cb(
        hpx::actions::basic_action<Component, Signature, Derived> /*act*/,
        hpx::id_type const& gid, Callback&& cb, Ts&&... vs)
    {
        constexpr launch::async_policy policy(
            actions::action_priority<Derived>(),
            actions::action_stacksize<Derived>());
        return hpx::post_p_cb<Derived>(
            gid, policy, HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Action, typename DistPolicy,
        typename Callback, typename... Ts>
        requires(traits::is_distribution_policy_v<DistPolicy>)
    bool post_p_cb(DistPolicy const& policy, hpx::launch launch_policy,
        Callback&& cb, Ts&&... vs)
    {
        return policy.template apply_cb<Action>(
            launch_policy, HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
    }

    template <typename Action, typename DistPolicy, typename Callback,
        typename... Ts>
        requires(traits::is_distribution_policy_v<DistPolicy>)
    bool post_cb(DistPolicy const& policy, Callback&& cb, Ts&&... vs)
    {
        constexpr launch::async_policy launch_policy(
            actions::action_priority<Action>(),
            actions::action_stacksize<Action>());
        return hpx::post_p_cb<Action>(policy, launch_policy,
            HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
    }

    template <typename Component, typename Signature, typename Derived,
        typename DistPolicy, typename Callback, typename... Ts>
        requires(traits::is_distribution_policy_v<DistPolicy>)
    bool post_cb(
        hpx::actions::basic_action<Component, Signature, Derived> /*act*/,
        DistPolicy const& policy, Callback&& cb, Ts&&... vs)
    {
        constexpr launch::async_policy launch_policy(
            actions::action_priority<Derived>(),
            actions::action_stacksize<Derived>());
        return hpx::post_p_cb<Derived>(policy, launch_policy,
            HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
    }

#if defined(HPX_HAVE_NETWORKING)
    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_EXPORT template <typename Action, typename Continuation,
            typename Callback, typename... Ts>
        bool post_r_p_cb(naming::address&& addr, Continuation&& c,
            hpx::id_type const& id, hpx::launch policy, Callback&& cb,
            Ts&&... vs)
        {
            // If remote, create a new parcel to be sent to the destination
            // Create a new parcel with the gid, action, and arguments
            return detail::put_parcel_cont_cb<Action>(id, HPX_MOVE(addr),
                policy, HPX_FORWARD(Continuation, c), HPX_FORWARD(Callback, cb),
                HPX_FORWARD(Ts, vs)...);
        }

        HPX_CXX_EXPORT template <typename Action, typename Continuation,
            typename Callback, typename... Ts>
        bool post_r_cb(naming::address&& addr, Continuation&& c,
            hpx::id_type const& gid, Callback&& cb, Ts&&... vs)
        {
            constexpr launch::async_policy policy(
                actions::action_priority<Action>(),
                actions::action_stacksize<Action>());
            return post_r_p_cb<Action>(HPX_MOVE(addr),
                HPX_FORWARD(Continuation, c), gid, policy,
                HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
        }
    }    // namespace detail
#endif

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename Continuation,
        typename Callback, typename... Ts>
    bool post_p_cb(Continuation&& c, naming::address&& addr,
        hpx::id_type const& gid, hpx::launch policy, Callback&& cb, Ts&&... vs)
    {
        if (!traits::action_is_target_valid<Action>::call(gid))
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter, "post_p_cb",
                "the target (destination) does not match the action type ({})",
                hpx::actions::detail::get_action_name<Action>());
        }

        // Determine whether the gid is local or remote
        if (addr &&
            naming::get_locality_id_from_gid(addr.locality_) ==
                agas::get_locality_id())
        {
            // apply locally
            bool const result =
                hpx::detail::post_l_p<Action>(HPX_FORWARD(Continuation, c), gid,
                    HPX_MOVE(addr), policy, HPX_FORWARD(Ts, vs)...);

            detail::invoke_callback(HPX_FORWARD(Callback, cb));
            return result;
        }

#if defined(HPX_HAVE_NETWORKING)
        // apply remotely
        return hpx::detail::post_r_p_cb<Action>(HPX_MOVE(addr),
            HPX_FORWARD(Continuation, c), gid, policy,
            HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
#else
        HPX_THROW_EXCEPTION(hpx::error::invalid_status, "hpx::post_cb",
            "unexpected attempt to send a parcel with networking disabled");
#endif
    }

    HPX_CXX_EXPORT template <typename Action, typename Continuation,
        typename Callback, typename... Ts>
    bool post_p_cb(Continuation&& c, hpx::id_type const& gid,
        hpx::launch policy, Callback&& cb, Ts&&... vs)
    {
        return hpx::detail::post_cb_impl<Action>(HPX_FORWARD(Continuation, c),
            gid, policy, HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Action, typename Continuation,
        typename Callback, typename... Ts>
    bool post_cb(
        Continuation&& c, hpx::id_type const& gid, Callback&& cb, Ts&&... vs)
    {
        constexpr launch::async_policy policy(
            actions::action_priority<Action>(),
            actions::action_stacksize<Action>());
        return hpx::post_p_cb<Action>(HPX_FORWARD(Continuation, c), gid, policy,
            HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Component, typename Continuation,
        typename Signature, typename Derived, typename Callback, typename... Ts>
    bool post_cb(Continuation&& c,
        hpx::actions::basic_action<Component, Signature, Derived> /*act*/,
        hpx::id_type const& gid, Callback&& cb, Ts&&... vs)
    {
        constexpr launch::async_policy policy(
            actions::action_priority<Derived>(),
            actions::action_stacksize<Derived>());
        return hpx::post_p<Derived>(HPX_FORWARD(Continuation, c), gid, policy,
            HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Action, typename Continuation,
        typename DistPolicy, typename Callback, typename... Ts>
        requires(traits::is_continuation<Continuation>::value &&
            traits::is_distribution_policy_v<DistPolicy>)
    bool post_p_cb(Continuation&& c, DistPolicy const& policy,
        hpx::launch launch_policy, Callback&& cb, Ts&&... vs)
    {
        return policy.template apply_cb<Action>(HPX_FORWARD(Continuation, c),
            launch_policy, HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Action, typename Continuation,
        typename DistPolicy, typename Callback, typename... Ts>
        requires(traits::is_continuation<Continuation>::value &&
            traits::is_distribution_policy_v<DistPolicy>)
    bool post_cb(
        Continuation&& c, DistPolicy const& policy, Callback&& cb, Ts&&... vs)
    {
        constexpr launch::async_policy launch_policy(
            actions::action_priority<Action>(),
            actions::action_stacksize<Action>());
        return hpx::post_p_cb<Action>(HPX_FORWARD(Continuation, c), policy,
            launch_policy, HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Component, typename Continuation,
        typename Signature, typename Derived, typename DistPolicy,
        typename Callback, typename... Ts>
        requires(traits::is_distribution_policy_v<DistPolicy>)
    bool post_cb(Continuation&& c,
        hpx::actions::basic_action<Component, Signature, Derived> /*act*/,
        DistPolicy const& policy, Callback&& cb, Ts&&... vs)
    {
        constexpr launch::async_policy launch_policy(
            actions::action_priority<Derived>(),
            actions::action_stacksize<Derived>());
        return hpx::post_p<Derived>(HPX_FORWARD(Continuation, c), policy,
            launch_policy, HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
    }

#if defined(HPX_HAVE_NETWORKING)
    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        HPX_CXX_EXPORT template <typename Action, typename Callback,
            typename... Ts>
        bool post_c_p_cb(naming::address&& addr, hpx::id_type const& contgid,
            hpx::id_type const& gid, hpx::launch policy, Callback&& cb,
            Ts&&... vs)
        {
            using remote_result_type = typename hpx::traits::extract_action<
                Action>::remote_result_type;
            using local_result_type =
                typename hpx::traits::extract_action<Action>::local_result_type;

            return post_r_p_cb<Action>(HPX_MOVE(addr),
                actions::typed_continuation<local_result_type,
                    remote_result_type>(contgid),
                gid, policy, HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
        }

        HPX_CXX_EXPORT template <typename Action, typename Callback,
            typename... Ts>
        bool post_c_cb(naming::address&& addr, hpx::id_type const& contgid,
            hpx::id_type const& gid, Callback&& cb, Ts&&... vs)
        {
            using remote_result_type = typename hpx::traits::extract_action<
                Action>::remote_result_type;
            using local_result_type =
                typename hpx::traits::extract_action<Action>::local_result_type;

            constexpr launch::async_policy policy(
                actions::action_priority<Action>(),
                actions::action_stacksize<Action>());
            return post_r_p_cb<Action>(HPX_MOVE(addr),
                actions::typed_continuation<local_result_type,
                    remote_result_type>(contgid),
                gid, policy, HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
        }
    }    // namespace detail
#endif

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename Callback, typename... Ts>
    bool post_c_p_cb(hpx::id_type const& contgid, hpx::id_type const& gid,
        hpx::launch policy, Callback&& cb, Ts&&... vs)
    {
        using remote_result_type =
            typename hpx::traits::extract_action<Action>::remote_result_type;
        using local_result_type =
            typename hpx::traits::extract_action<Action>::local_result_type;

        return hpx::post_p_cb<Action>(
            actions::typed_continuation<local_result_type, remote_result_type>(
                contgid),
            gid, policy, HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Action, typename Callback, typename... Ts>
    bool post_c_cb(hpx::id_type const& contgid, hpx::id_type const& gid,
        Callback&& cb, Ts&&... vs)
    {
        using remote_result_type =
            typename hpx::traits::extract_action<Action>::remote_result_type;
        using local_result_type =
            typename hpx::traits::extract_action<Action>::local_result_type;

        constexpr launch::async_policy policy(
            actions::action_priority<Action>(),
            actions::action_stacksize<Action>());
        return hpx::post_p_cb<Action>(
            actions::typed_continuation<local_result_type, remote_result_type>(
                contgid),
            gid, policy, HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Action, typename Callback, typename... Ts>
    bool post_c_p_cb(hpx::id_type const& contgid, naming::address&& addr,
        hpx::id_type const& gid, hpx::launch policy, Callback&& cb, Ts&&... vs)
    {
        using remote_result_type =
            typename hpx::traits::extract_action<Action>::remote_result_type;
        using local_result_type =
            typename hpx::traits::extract_action<Action>::local_result_type;

        return hpx::post_p_cb<Action>(
            actions::typed_continuation<local_result_type, remote_result_type>(
                contgid),
            HPX_MOVE(addr), gid, policy, HPX_FORWARD(Callback, cb),
            HPX_FORWARD(Ts, vs)...);
    }

    HPX_CXX_EXPORT template <typename Action, typename Callback, typename... Ts>
    bool post_c_cb(hpx::id_type const& contgid, naming::address&& addr,
        hpx::id_type const& gid, Callback&& cb, Ts&&... vs)
    {
        using remote_result_type =
            typename hpx::traits::extract_action<Action>::remote_result_type;
        using local_result_type =
            typename hpx::traits::extract_action<Action>::local_result_type;

        constexpr launch::async_policy policy(
            actions::action_priority<Action>(),
            actions::action_stacksize<Action>());
        return hpx::post_p_cb<Action>(
            actions::typed_continuation<local_result_type, remote_result_type>(
                contgid),
            HPX_MOVE(addr), gid, policy, HPX_FORWARD(Callback, cb),
            HPX_FORWARD(Ts, vs)...);
    }

    namespace functional {

        HPX_CXX_EXPORT template <typename Action, typename Callback,
            typename... Ts>
        struct post_c_p_cb_impl
        {
        public:
            using tuple_type = hpx::tuple<Ts...>;

            template <typename... Ts_>
            post_c_p_cb_impl(hpx::id_type const& contid, naming::address&& addr,
                hpx::id_type const& id, hpx::launch policy, Callback&& cb,
                Ts_&&... vs)
              : contid_(contid)
              , addr_(HPX_MOVE(addr))
              , id_(id)
              , policy_(policy)
              , cb_(HPX_MOVE(cb))
              , args_(HPX_FORWARD(Ts_, vs)...)
            {
            }

            post_c_p_cb_impl(post_c_p_cb_impl const& rhs) noexcept = delete;
            post_c_p_cb_impl(post_c_p_cb_impl&& rhs) noexcept = default;

            post_c_p_cb_impl& operator=(
                post_c_p_cb_impl const& rhs) noexcept = delete;
            post_c_p_cb_impl& operator=(
                post_c_p_cb_impl&& rhs) noexcept = default;

            ~post_c_p_cb_impl() = default;

            void operator()()
            {
                post_action(
                    typename util::make_index_pack<sizeof...(Ts)>::type());
            }

        protected:
            template <std::size_t... Is>
            void post_action(util::index_pack<Is...>)
            {
                if (addr_)
                {
                    hpx::post_c_p_cb<Action>(contid_, HPX_MOVE(addr_), id_,
                        policy_, HPX_MOVE(cb_),
                        hpx::get<Is>(HPX_FORWARD(tuple_type, args_))...);
                }
                else
                {
                    hpx::post_c_p_cb<Action>(contid_, id_, policy_,
                        HPX_MOVE(cb_),
                        hpx::get<Is>(HPX_FORWARD(tuple_type, args_))...);
                }
            }

        private:
            hpx::id_type contid_;
            naming::address addr_;
            hpx::id_type id_;
            hpx::launch policy_;
            Callback cb_;
            tuple_type args_;
        };

        template <typename Action, typename Callback, typename... Ts>
        post_c_p_cb_impl<Action, std::decay_t<Callback>, std::decay_t<Ts>...>
        post_c_p_cb(hpx::id_type const& contid, naming::address&& addr,
            hpx::id_type const& id, hpx::launch policy, Callback&& cb,
            Ts&&... vs)
        {
            using result_type = post_c_p_cb_impl<Action, std::decay_t<Callback>,
                std::decay_t<Ts>...>;

            return result_type(contid, HPX_MOVE(addr), id, policy,
                HPX_FORWARD(Callback, cb), HPX_FORWARD(Ts, vs)...);
        }
    }    // namespace functional

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action, typename... Ts>
    HPX_DEPRECATED_V(
        1, 9, "hpx::apply_p_cb is deprecated, use hpx::post_p_cb instead")
    bool apply_p_cb(Ts&&... ts)
    {
        return hpx::post_p_cb<Action>(HPX_FORWARD(Ts, ts)...);
    }

    template <typename... Ts>
    HPX_DEPRECATED_V(
        1, 9, "hpx::apply_p_cb is deprecated, use hpx::post_p_cb instead")
    bool apply_p_cb(Ts&&... ts)
    {
        return hpx::post_p_cb(HPX_FORWARD(Ts, ts)...);
    }

    template <typename Action, typename... Ts>
    HPX_DEPRECATED_V(
        1, 9, "hpx::apply_cb is deprecated, use hpx::post_cp instead")
    bool apply_cb(Ts&&... ts)
    {
        return hpx::post_cb<Action>(HPX_FORWARD(Ts, ts)...);
    }

    template <typename... Ts>
    HPX_DEPRECATED_V(
        1, 9, "hpx::apply_cb is deprecated, use hpx::post_cp instead")
    bool apply_cb(Ts&&... ts)
    {
        return hpx::post_cb(HPX_FORWARD(Ts, ts)...);
    }

    template <typename Action, typename... Ts>
    HPX_DEPRECATED_V(
        1, 9, "hpx::apply_c_p_cb is deprecated, use hpx::post_c_p_cb instead")
    bool apply_c_p_cb(Ts&&... ts)
    {
        return hpx::post_c_p_cb<Action>(HPX_FORWARD(Ts, ts)...);
    }

    template <typename Action, typename... Ts>
    HPX_DEPRECATED_V(
        1, 9, "hpx::apply_c_cb is deprecated, use hpx::post_c_cb instead")
    bool apply_c_cb(Ts&&... ts)
    {
        return hpx::post_c_cb<Action>(HPX_FORWARD(Ts, ts)...);
    }
}    // namespace hpx
