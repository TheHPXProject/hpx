//  Copyright (c) 2013-2014 Thomas Heller
//  Copyright (c) 2013-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file broadcast_direct.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx { namespace lcos {

    /// \brief Perform a distributed broadcast operation
    ///
    /// The function hpx::lcos::broadcast performs a distributed broadcast
    /// operation resulting in action invocations on a given set
    /// of global identifiers. The action can be either a plain action (in
    /// which case the global identifiers have to refer to localities) or a
    /// component action (in which case the global identifiers have to refer
    /// to instances of a component type which exposes the action.
    ///
    /// The given action is invoked asynchronously on all given identifiers,
    /// and the arguments ArgN are passed along to those invocations.
    ///
    /// \param ids       [in] A list of global identifiers identifying the
    ///                  target objects for which the given action will be
    ///                  invoked.
    /// \param argN      [in] Any number of arbitrary arguments (passed
    ///                  by const reference) which will be forwarded to the
    ///                  action invocation.
    ///
    /// \returns         This function returns a future representing the result
    ///                  of the overall reduction operation.
    ///
    /// \note            If decltype(Action(...)) is void, then the result of
    ///                  this function is future<void>.
    ///
    template <typename Action, typename ArgN, ...>
    hpx::future<std::vector<decltype(Action(hpx::id_type, ArgN, ...))>>
    broadcast(std::vector<hpx::id_type> const& ids, ArgN argN, ...);

    /// \brief Perform an asynchronous (fire&forget) distributed broadcast operation
    ///
    /// The function hpx::lcos::broadcast_post performs an asynchronous
    /// (fire&forget) distributed broadcast operation resulting in action
    /// invocations on a given set of global identifiers. The action can be
    /// either a plain action (in which case the global identifiers have to
    /// refer to localities) or a component action (in which case the global
    /// identifiers have to refer to instances of a component type which
    /// exposes the action.
    ///
    /// The given action is invoked asynchronously on all given identifiers,
    /// and the arguments ArgN are passed along to those invocations.
    ///
    /// \param ids       [in] A list of global identifiers identifying the
    ///                  target objects for which the given action will be
    ///                  invoked.
    /// \param argN      [in] Any number of arbitrary arguments (passed
    ///                  by const reference) which will be forwarded to the
    ///                  action invocation.
    ///
    template <typename Action, typename ArgN, ...>
    void broadcast_post(std::vector<hpx::id_type> const& ids, ArgN argN, ...);

    /// \brief Perform a distributed broadcast operation
    ///
    /// The function hpx::lcos::broadcast_with_index performs a distributed broadcast
    /// operation resulting in action invocations on a given set
    /// of global identifiers. The action can be either a plain action (in
    /// which case the global identifiers have to refer to localities) or a
    /// component action (in which case the global identifiers have to refer
    /// to instances of a component type which exposes the action.
    ///
    /// The given action is invoked asynchronously on all given identifiers,
    /// and the arguments ArgN are passed along to those invocations.
    ///
    /// The function passes the index of the global identifier in the given
    /// list of identifiers as the last argument to the action.
    ///
    /// \param ids       [in] A list of global identifiers identifying the
    ///                  target objects for which the given action will be
    ///                  invoked.
    /// \param argN      [in] Any number of arbitrary arguments (passed
    ///                  by const reference) which will be forwarded to the
    ///                  action invocation.
    ///
    /// \returns         This function returns a future representing the result
    ///                  of the overall reduction operation.
    ///
    /// \note            If decltype(Action(...)) is void, then the result of
    ///                  this function is future<void>.
    ///
    template <typename Action, typename ArgN, ...>
    hpx::future<
        std::vector<decltype(Action(hpx::id_type, ArgN, ..., std::size_t))>>
    broadcast_with_index(std::vector<hpx::id_type> const& ids, ArgN argN, ...);

    /// \brief Perform an asynchronous (fire&forget) distributed broadcast operation
    ///
    /// The function hpx::lcos::broadcast_post_with_index performs an asynchronous
    /// (fire&forget) distributed broadcast operation resulting in action
    /// invocations on a given set of global identifiers. The action can be
    /// either a plain action (in which case the global identifiers have to
    /// refer to localities) or a component action (in which case the global
    /// identifiers have to refer to instances of a component type which
    /// exposes the action.
    ///
    /// The given action is invoked asynchronously on all given identifiers,
    /// and the arguments ArgN are passed along to those invocations.
    ///
    /// The function passes the index of the global identifier in the given
    /// list of identifiers as the last argument to the action.
    ///
    /// \param ids       [in] A list of global identifiers identifying the
    ///                  target objects for which the given action will be
    ///                  invoked.
    /// \param argN      [in] Any number of arbitrary arguments (passed
    ///                  by const reference) which will be forwarded to the
    ///                  action invocation.
    ///
    template <typename Action, typename ArgN, ...>
    void broadcast_post_with_index(
        std::vector<hpx::id_type> const& ids, ArgN argN, ...);
}}    // namespace hpx::lcos

#else

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/assert.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_colocated.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/preprocessor.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/modules/util.hpp>

#include <hpx/collectives/macros.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

#if !defined(HPX_BROADCAST_FANOUT)
#define HPX_BROADCAST_FANOUT 16
#endif

namespace hpx::lcos {

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_EXPORT template <typename Action>
        struct broadcast_with_index
        {
            using arguments_type = Action::arguments_type;
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Action>
        struct broadcast_result
        {
            using action_result = traits::promise_local_result<typename hpx::
                    traits::extract_action<Action>::remote_result_type>::type;
            using type = std::conditional_t<std::is_same_v<void, action_result>,
                void, std::vector<action_result>>;
        };

        template <typename Action>
        struct broadcast_result<broadcast_with_index<Action>>
          : broadcast_result<Action>
        {
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Action, typename... Ts>
        //hpx::future<typename broadcast_result<Action>::type>
        broadcast_result<Action>::type broadcast_impl(Action const& act,
            std::vector<hpx::id_type> const& ids, std::size_t global_idx,
            std::false_type, Ts const&... vs);

        template <typename Action, typename... Ts>
        //hpx::future<void>
        void broadcast_impl(Action const& act,
            std::vector<hpx::id_type> const& ids, std::size_t global_idx,
            std::true_type, Ts const&... vs);

        template <typename Action, typename... Ts>
        void broadcast_post_impl(Action const& act,
            std::vector<hpx::id_type> const& ids, std::size_t global_idx,
            Ts const&... vs);

        ///////////////////////////////////////////////////////////////////////
        template <typename Action, typename Futures, typename... Ts>
        void broadcast_invoke(Action act, Futures& futures,
            hpx::id_type const& id, std::size_t, Ts const&... vs)
        {
            futures.push_back(hpx::async(act, id, vs...));
        }

        template <typename Action, typename Futures, typename... Ts>
        void broadcast_invoke(broadcast_with_index<Action>, Futures& futures,
            hpx::id_type const& id, std::size_t global_idx, Ts const&... vs)
        {
            futures.push_back(hpx::async(Action(), id, vs..., global_idx));
        }

        template <typename Action, typename Futures, typename Cont,
            typename... Ts>
        void broadcast_invoke(Action act, Futures& futures, Cont&& cont,
            hpx::id_type const& id, std::size_t, Ts const&... vs)
        {
            futures.push_back(
                hpx::async(act, id, vs...).then(HPX_FORWARD(Cont, cont)));
        }

        template <typename Action, typename Futures, typename Cont,
            typename... Ts>
        void broadcast_invoke(broadcast_with_index<Action>, Futures& futures,
            Cont&& cont, hpx::id_type const& id, std::size_t global_idx,
            Ts const&... vs)
        {
            futures.push_back(hpx::async(Action(), id, vs..., global_idx)
                    .then(HPX_FORWARD(Cont, cont)));
        }

        template <typename Action, typename... Ts>
        void broadcast_invoke_apply(
            Action act, hpx::id_type const& id, std::size_t, Ts const&... vs)
        {
            hpx::post(act, id, vs...);
        }

        template <typename Action, typename... Ts>
        void broadcast_invoke_apply(broadcast_with_index<Action>,
            hpx::id_type const& id, std::size_t global_idx, Ts const&... vs)
        {
            hpx::post(Action(), id, vs..., global_idx);
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename Action, typename IsVoid, typename... Ts>
        struct broadcast_invoker
        {
            //static hpx::future<typename broadcast_result<Action>::type>
            static broadcast_result<Action>::type call(Action const& act,
                std::vector<hpx::id_type> const& ids, std::size_t global_idx,
                IsVoid, Ts const&... vs)
            {
                return broadcast_impl(act, ids, global_idx, IsVoid(), vs...);
            }
        };

        template <typename Action, typename... Ts>
        struct broadcast_post_invoker
        {
            static void call(Action const& act,
                std::vector<hpx::id_type> const& ids, std::size_t global_idx,
                Ts const&... vs)
            {
                return broadcast_post_impl(act, ids, global_idx, vs...);
            }
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_EXPORT template <typename Action, typename Is>
        struct make_broadcast_action_impl;

        template <typename Action, std::size_t... Is>
        struct make_broadcast_action_impl<Action, util::index_pack<Is...>>
        {
            using action_result = broadcast_result<Action>::action_result;

            using broadcast_invoker_type = detail::broadcast_invoker<Action,
                typename std::is_void<action_result>::type,
                typename hpx::tuple_element<Is,
                    typename Action::arguments_type>::type...>;

            using type = HPX_MAKE_ACTION(broadcast_invoker_type::call)::type;
        };

        HPX_CXX_EXPORT template <typename Action>
        struct make_broadcast_action
          : make_broadcast_action_impl<Action,
                typename util::make_index_pack<Action::arity>::type>
        {
        };

        template <typename Action>
        struct make_broadcast_action<broadcast_with_index<Action>>
          : make_broadcast_action_impl<broadcast_with_index<Action>,
                typename util::make_index_pack<Action::arity - 1>::type>
        {
        };

        template <typename Action, typename Is>
        struct make_broadcast_post_action_impl;

        template <typename Action, std::size_t... Is>
        struct make_broadcast_post_action_impl<Action, util::index_pack<Is...>>
        {
            using action_result = broadcast_result<Action>::action_result;

            using broadcast_invoker_type =
                detail::broadcast_post_invoker<Action,
                    typename hpx::tuple_element<Is,
                        typename Action::arguments_type>::type...>;

            using type = HPX_MAKE_ACTION(broadcast_invoker_type::call)::type;
        };

        HPX_CXX_EXPORT template <typename Action>
        struct make_broadcast_post_action
          : make_broadcast_post_action_impl<Action,
                typename util::make_index_pack<Action::arity>::type>
        {
        };

        template <typename Action>
        struct make_broadcast_post_action<broadcast_with_index<Action>>
          : make_broadcast_post_action_impl<broadcast_with_index<Action>,
                typename util::make_index_pack<Action::arity - 1>::type>
        {
        };

        ///////////////////////////////////////////////////////////////////////
        inline void return_void(hpx::future<std::vector<hpx::future<void>>>)
        {
            // todo: verify validity of all futures in the vector
        }

        template <typename Result>
        std::vector<Result> wrap_into_vector(hpx::future<Result> r)
        {
            std::vector<Result> result;
            result.push_back(r.get());
            return result;
        }

        template <typename Result>
        std::vector<Result> return_result_type(
            hpx::future<std::vector<hpx::future<std::vector<Result>>>> r)
        {
            std::vector<Result> res;
            std::vector<hpx::future<std::vector<Result>>> fres = r.get();

            for (hpx::future<std::vector<Result>>& f : fres)
            {
                std::vector<Result> t = f.get();
                res.reserve(res.capacity() + t.size());
                std::move(t.begin(), t.end(), std::back_inserter(res));
            }

            return res;
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename Action, typename... Ts>
        //hpx::future<void>
        void broadcast_impl(Action const& act,
            std::vector<hpx::id_type> const& ids, std::size_t global_idx,
            std::true_type, Ts const&... vs)
        {
            if (ids.empty())
                return;    // hpx::make_ready_future();

            constexpr std::size_t local_fanout = HPX_BROADCAST_FANOUT;
            std::size_t const local_size =
                (std::min) (ids.size(), local_fanout);
            std::size_t const fanout =
                util::calculate_fanout(ids.size(), local_fanout);

            std::vector<hpx::future<void>> broadcast_futures;
            broadcast_futures.reserve(local_size + (ids.size() / fanout) + 1);
            for (std::size_t i = 0; i != local_size; ++i)
            {
                broadcast_invoke(
                    act, broadcast_futures, ids[i], global_idx + i, vs...);
            }

            if (ids.size() > local_fanout)
            {
                std::size_t applied = local_fanout;
                std::vector<hpx::id_type>::const_iterator it =
                    ids.begin() + local_fanout;

                using broadcast_impl_action =
                    detail::make_broadcast_action<Action>::type;

                while (it != ids.end())
                {
                    HPX_ASSERT(ids.size() >= applied);

                    std::size_t const next_fan =
                        (std::min) (fanout, ids.size() - applied);
                    std::vector<hpx::id_type> ids_next(
                        it, it + static_cast<std::ptrdiff_t>(next_fan));

                    hpx::id_type id(ids_next[0]);
                    broadcast_futures.push_back(
                        hpx::detail::async_colocated<broadcast_impl_action>(id,
                            act, HPX_MOVE(ids_next), global_idx + applied,
                            std::true_type(), vs...));

                    applied += next_fan;
                    it += static_cast<std::ptrdiff_t>(next_fan);
                }
            }

            //return hpx::when_all(broadcast_futures).then(&return_void);
            hpx::when_all(broadcast_futures).then(&return_void).get();
        }

        template <typename Action, typename... Ts>
        //hpx::future<typename broadcast_result<Action>::type>
        broadcast_result<Action>::type broadcast_impl(Action const& act,
            std::vector<hpx::id_type> const& ids, std::size_t global_idx,
            std::false_type, Ts const&... vs)
        {
            using action_result = broadcast_result<Action>::action_result;
            using result_type = broadcast_result<Action>::type;

            //if(ids.empty()) return hpx::make_ready_future(result_type());
            if (ids.empty())
                return result_type();

            constexpr std::size_t local_fanout = HPX_BROADCAST_FANOUT;
            std::size_t const local_size =
                (std::min) (ids.size(), local_fanout);
            std::size_t const fanout =
                util::calculate_fanout(ids.size(), local_fanout);

            std::vector<hpx::future<result_type>> broadcast_futures;
            broadcast_futures.reserve(local_size + (ids.size() / fanout) + 1);
            for (std::size_t i = 0; i != local_size; ++i)
            {
                broadcast_invoke(act, broadcast_futures,
                    &wrap_into_vector<action_result>, ids[i], global_idx + i,
                    vs...);
            }

            if (ids.size() > local_fanout)
            {
                std::size_t applied = local_fanout;
                std::vector<hpx::id_type>::const_iterator it =
                    ids.begin() + local_fanout;

                using broadcast_impl_action =
                    detail::make_broadcast_action<Action>::type;

                while (it != ids.end())
                {
                    HPX_ASSERT(ids.size() >= applied);

                    std::size_t const next_fan =
                        (std::min) (fanout, ids.size() - applied);
                    std::vector<hpx::id_type> ids_next(
                        it, it + static_cast<std::ptrdiff_t>(next_fan));

                    hpx::id_type id(ids_next[0]);
                    broadcast_futures.push_back(
                        hpx::detail::async_colocated<broadcast_impl_action>(id,
                            act, HPX_MOVE(ids_next), global_idx + applied,
                            std::false_type(), vs...));

                    applied += next_fan;
                    it += static_cast<std::ptrdiff_t>(next_fan);
                }
            }

            return hpx::when_all(broadcast_futures)
                .then(&return_result_type<action_result>)
                .get();
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename Action, typename... Ts>
        void broadcast_post_impl(Action const& act,
            std::vector<hpx::id_type> const& ids, std::size_t global_idx,
            Ts const&... vs)
        {
            if (ids.empty())
                return;

            constexpr std::size_t local_fanout = HPX_BROADCAST_FANOUT;
            std::size_t const local_size =
                (std::min) (ids.size(), local_fanout);

            for (std::size_t i = 0; i != local_size; ++i)
            {
                broadcast_invoke_apply(act, ids[i], global_idx + i, vs...);
            }

            if (ids.size() > local_fanout)
            {
                std::size_t applied = local_fanout;
                std::vector<hpx::id_type>::const_iterator it =
                    ids.begin() + local_fanout;

                using broadcast_impl_action =
                    detail::make_broadcast_post_action<Action>::type;

                std::size_t const fanout =
                    util::calculate_fanout(ids.size(), local_fanout);
                while (it != ids.end())
                {
                    HPX_ASSERT(ids.size() >= applied);

                    std::size_t const next_fan =
                        (std::min) (fanout, ids.size() - applied);
                    std::vector<hpx::id_type> ids_next(
                        it, it + static_cast<std::ptrdiff_t>(next_fan));

                    hpx::id_type id(ids_next[0]);
                    hpx::detail::post_colocated<broadcast_impl_action>(id, act,
                        HPX_MOVE(ids_next), global_idx + applied, vs...);

                    applied += next_fan;
                    it += static_cast<std::ptrdiff_t>(next_fan);
                }
            }
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    hpx::future<typename detail::broadcast_result<Action>::type> broadcast(
        std::vector<hpx::id_type> const& ids, Ts const&... vs)
    {
        using broadcast_impl_action =
            detail::make_broadcast_action<Action>::type;
        using action_result = detail::broadcast_result<Action>::action_result;

        if (ids.empty())
        {
            using result_type = detail::broadcast_result<Action>::type;

            return hpx::make_exceptional_future<result_type>(HPX_GET_EXCEPTION(
                hpx::error::bad_parameter, "hpx::lcos::broadcast",
                "empty list of targets for broadcast operation"));
        }

        return hpx::detail::async_colocated<broadcast_impl_action>(ids[0],
            Action(), ids, static_cast<std::size_t>(0),
            std::is_void<action_result>(), vs...);
    }

    HPX_CXX_EXPORT template <typename Component, typename Signature,
        typename Derived, typename... Ts>
    hpx::future<typename detail::broadcast_result<Derived>::type> broadcast(
        hpx::actions::basic_action<Component, Signature, Derived> /* act */
        ,
        std::vector<hpx::id_type> const& ids, Ts const&... vs)
    {
        return broadcast<Derived>(ids, vs...);
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    void broadcast_post(std::vector<hpx::id_type> const& ids, Ts const&... vs)
    {
        using broadcast_impl_action =
            detail::make_broadcast_post_action<Action>::type;

        if (ids.empty())
        {
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::lcos::broadcast_post",
                "empty list of targets for broadcast operation");
        }

        hpx::detail::post_colocated<broadcast_impl_action>(
            ids[0], Action(), ids, 0, vs...);
    }

    HPX_CXX_EXPORT template <typename Component, typename Signature,
        typename Derived, typename... Ts>
    void broadcast_post(
        hpx::actions::basic_action<Component, Signature, Derived> /* act */
        ,
        std::vector<hpx::id_type> const& ids, Ts const&... vs)
    {
        broadcast_post<Derived>(ids, vs...);
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    hpx::future<typename detail::broadcast_result<Action>::type>
    broadcast_with_index(std::vector<hpx::id_type> const& ids, Ts const&... vs)
    {
        return broadcast<detail::broadcast_with_index<Action>>(ids, vs...);
    }

    HPX_CXX_EXPORT template <typename Component, typename Signature,
        typename Derived, typename... Ts>
    hpx::future<typename detail::broadcast_result<Derived>::type>
    broadcast_with_index(
        hpx::actions::basic_action<Component, Signature, Derived> /* act */
        ,
        std::vector<hpx::id_type> const& ids, Ts const&... vs)
    {
        return broadcast<detail::broadcast_with_index<Derived>>(ids, vs...);
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename... Ts>
    void broadcast_post_with_index(
        std::vector<hpx::id_type> const& ids, Ts const&... vs)
    {
        broadcast_post<detail::broadcast_with_index<Action>>(ids, vs...);
    }

    HPX_CXX_EXPORT template <typename Component, typename Signature,
        typename Derived, typename... Ts>
    void broadcast_post_with_index(
        hpx::actions::basic_action<Component, Signature, Derived> /* act */
        ,
        std::vector<hpx::id_type> const& ids, Ts const&... vs)
    {
        broadcast_post<detail::broadcast_with_index<Derived>>(ids, vs...);
    }
}    // namespace hpx::lcos

#endif    //COMPUTE_DEVICE_CODE
#endif    // DOXYGEN
