//  Copyright (c) 2013-2025 Hartmut Kaiser
//  Copyright (c) 2013 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file lcos/reduce_direct.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx { namespace lcos {

    /// \brief Perform a distributed reduction operation
    ///
    /// The function hpx::lcos::reduce performs a distributed reduction
    /// operation over results returned from action invocations on a given set
    /// of global identifiers. The action can be either a plain action (in
    /// which case the global identifiers have to refer to localities) or a
    /// component action (in which case the global identifiers have to refer
    /// to instances of a component type which exposes the action.
    ///
    /// \param ids       [in] A list of global identifiers identifying the
    ///                  target objects for which the given action will be
    ///                  invoked.
    /// \param reduce_op [in] A binary function expecting two results as
    ///                  returned from the action invocations. The function
    ///                  (or function object) is expected to return the result
    ///                  of the reduction operation performed on its arguments.
    /// \param argN      [in] Any number of arbitrary arguments (passed by
    ///                  by const reference) which will be forwarded to the
    ///                  action invocation.
    ///
    /// \returns         This function returns a future representing the result
    ///                  of the overall reduction operation.
    ///
    template <typename Action, typename ReduceOp, typename ArgN, ...>
    hpx::future<decltype(Action(hpx::id_type, ArgN, ...))> reduce(
        std::vector<hpx::id_type> const& ids, ReduceOp&& reduce_op, ArgN argN,
        ...);

    /// \brief Perform a distributed reduction operation
    ///
    /// The function hpx::lcos::reduce_with_index performs a distributed reduction
    /// operation over results returned from action invocations on a given set
    /// of global identifiers. The action can be either plain action (in
    /// which case the global identifiers have to refer to localities) or a
    /// component action (in which case the global identifiers have to refer
    /// to instances of a component type which exposes the action.
    ///
    /// The function passes the index of the global identifier in the given
    /// list of identifiers as the last argument to the action.
    ///
    /// \param ids       [in] A list of global identifiers identifying the
    ///                  target objects for which the given action will be
    ///                  invoked.
    /// \param reduce_op [in] A binary function expecting two results as
    ///                  returned from the action invocations. The function
    ///                  (or function object) is expected to return the result
    ///                  of the reduction operation performed on its arguments.
    /// \param argN      [in] Any number of arbitrary arguments (passed by
    ///                  by const reference) which will be forwarded to the
    ///                  action invocation.
    ///
    /// \returns         This function returns a future representing the result
    ///                  of the overall reduction operation.
    ///
    template <typename Action, typename ReduceOp, typename ArgN, ...>
    hpx::future<decltype(Action(hpx::id_type, ArgN, ..., std::size_t))>
    reduce_with_index(std::vector<hpx::id_type> const& ids,
        ReduceOp&& reduce_op, ArgN argN, ...);
}}    // namespace hpx::lcos
#else

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/async_colocated.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/preprocessor.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/modules/util.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

#if !defined(HPX_REDUCE_FANOUT)
#define HPX_REDUCE_FANOUT 16
#endif

namespace hpx { namespace lcos {

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        template <typename Action>
        struct reduce_with_index
        {
            typedef typename Action::arguments_type arguments_type;
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Action>
        struct reduce_result
          : traits::promise_local_result<typename hpx::traits::extract_action<
                Action>::remote_result_type>
        {
        };

        template <typename Action>
        struct reduce_result<reduce_with_index<Action>> : reduce_result<Action>
        {
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Action, typename ReduceOp, typename... Ts>
        typename reduce_result<Action>::type reduce_impl(Action const& act,
            std::vector<hpx::id_type> const& ids, ReduceOp&& reduce_op,
            std::size_t global_idx, Ts const&... vs);

        ///////////////////////////////////////////////////////////////////////
        template <typename Action, typename Futures, typename... Ts>
        void reduce_invoke(Action, Futures& futures,    //-V524
            hpx::id_type const& id, std::size_t, Ts const&... vs)
        {
            futures.push_back(hpx::async<Action>(id, vs...));
        }

        template <typename Action, typename Futures, typename... Ts>
        void reduce_invoke(reduce_with_index<Action>, Futures& futures,
            hpx::id_type const& id, std::size_t global_idx, Ts const&... vs)
        {
            futures.push_back(hpx::async<Action>(id, vs..., global_idx));
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename Action, typename ReduceOp, typename... Ts>
        struct reduce_invoker
        {
            //static hpx::future<typename reduce_result<Action>::type>
            static typename reduce_result<Action>::type call(Action const& act,
                std::vector<hpx::id_type> const& ids, ReduceOp const& reduce_op,
                std::size_t global_idx, Ts const&... vs)
            {
                return reduce_impl(act, ids, reduce_op, global_idx, vs...);
            }
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Action, typename Is>
        struct make_reduce_action_impl;

        template <typename Action, std::size_t... Is>
        struct make_reduce_action_impl<Action, util::index_pack<Is...>>
        {
            typedef typename reduce_result<Action>::type action_result;

            template <typename ReduceOp>
            struct reduce_invoker_helper
            {
                typedef typename std::decay<ReduceOp>::type reduce_op_type;

                typedef detail::reduce_invoker<Action, reduce_op_type,
                    typename hpx::tuple_element<Is,
                        typename Action::arguments_type>::type...>
                    reduce_invoker_type;

                typedef typename HPX_MAKE_ACTION(
                    reduce_invoker_type::call)::type type;
            };
        };

        template <typename Action>
        struct make_reduce_action
          : make_reduce_action_impl<Action,
                typename util::make_index_pack<Action::arity>::type>
        {
        };

        template <typename Action>
        struct make_reduce_action<reduce_with_index<Action>>
          : make_reduce_action_impl<reduce_with_index<Action>,
                typename util::make_index_pack<Action::arity - 1>::type>
        {
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Result, typename ReduceOp>
        struct perform_reduction
        {
            perform_reduction(ReduceOp const& reduce_op)
              : reduce_op_(reduce_op)
            {
            }

            Result operator()(
                hpx::future<std::vector<hpx::future<Result>>> r) const
            {
                std::vector<hpx::future<Result>> fres = r.get();

                HPX_ASSERT(!fres.empty());

                if (fres.size() == 1)
                    return fres[0].get();

                Result res = reduce_op_(fres[0].get(), fres[1].get());
                for (std::size_t i = 2; i != fres.size(); ++i)
                    res = reduce_op_(res, fres[i].get());

                return res;
            }

            ReduceOp const& reduce_op_;
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Action, typename ReduceOp, typename... Ts>
        typename reduce_result<Action>::type reduce_impl(Action const& act,
            std::vector<hpx::id_type> const& ids, ReduceOp&& reduce_op,
            std::size_t global_idx, Ts const&... vs)
        {
            typedef typename reduce_result<Action>::type result_type;

            if (ids.empty())
                return result_type();

            std::size_t const local_fanout = HPX_REDUCE_FANOUT;
            std::size_t local_size = (std::min) (ids.size(), local_fanout);
            std::size_t fanout =
                util::calculate_fanout(ids.size(), local_fanout);

            std::vector<hpx::future<result_type>> reduce_futures;
            reduce_futures.reserve(local_size + (ids.size() / fanout) + 1);
            for (std::size_t i = 0; i != local_size; ++i)
            {
                reduce_invoke(
                    act, reduce_futures, ids[i], global_idx + i, vs...);
            }

            if (ids.size() > local_fanout)
            {
                std::size_t applied = local_fanout;
                std::vector<hpx::id_type>::const_iterator it =
                    ids.begin() + local_fanout;

                typedef typename detail::make_reduce_action<
                    Action>::template reduce_invoker_helper<ReduceOp>::type
                    reduce_impl_action;

                while (it != ids.end())
                {
                    HPX_ASSERT(ids.size() >= applied);

                    std::size_t next_fan =
                        (std::min) (fanout, ids.size() - applied);
                    std::vector<hpx::id_type> ids_next(
                        it, it + static_cast<std::ptrdiff_t>(next_fan));

                    hpx::id_type id(ids_next[0]);
                    reduce_futures.push_back(
                        hpx::detail::async_colocated<reduce_impl_action>(id,
                            act, HPX_MOVE(ids_next), reduce_op,
                            global_idx + applied, vs...));

                    applied += next_fan;
                    it += static_cast<std::ptrdiff_t>(next_fan);
                }
            }

            return hpx::when_all(reduce_futures)
                .then(perform_reduction<result_type, ReduceOp>(reduce_op))
                .get();
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    template <typename Action, typename ReduceOp, typename... Ts>
    hpx::future<typename detail::reduce_result<Action>::type> reduce(
        std::vector<hpx::id_type> const& ids, ReduceOp&& reduce_op,
        Ts const&... vs)
    {
        typedef typename detail::make_reduce_action<Action>::
            template reduce_invoker_helper<ReduceOp>::type reduce_impl_action;

        typedef typename detail::reduce_result<Action>::type action_result;

        if (ids.empty())
        {
            return hpx::make_exceptional_future<action_result>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::lcos::reduce",
                    "empty list of targets for reduce operation"));
        }

        return hpx::detail::async_colocated<reduce_impl_action>(
            ids[0], Action(), ids, HPX_FORWARD(ReduceOp, reduce_op), 0, vs...);
    }

    template <typename Component, typename Signature, typename Derived,
        typename ReduceOp, typename... Ts>
    hpx::future<typename detail::reduce_result<Derived>::type> reduce(
        hpx::actions::basic_action<Component, Signature, Derived> /* act */
        ,
        std::vector<hpx::id_type> const& ids, ReduceOp&& reduce_op,
        Ts const&... vs)
    {
        return reduce<Derived>(ids, HPX_FORWARD(ReduceOp, reduce_op), vs...);
    }

    template <typename Action, typename ReduceOp, typename... Ts>
    hpx::future<typename detail::reduce_result<Action>::type> reduce_with_index(
        std::vector<hpx::id_type> const& ids, ReduceOp&& reduce_op,
        Ts const&... vs)
    {
        return reduce<detail::reduce_with_index<Action>>(
            ids, HPX_FORWARD(ReduceOp, reduce_op), vs...);
    }

    template <typename Component, typename Signature, typename Derived,
        typename ReduceOp, typename... Ts>
    hpx::future<typename detail::reduce_result<Derived>::type>
    reduce_with_index(
        hpx::actions::basic_action<Component, Signature, Derived> /* act */
        ,
        std::vector<hpx::id_type> const& ids, ReduceOp&& reduce_op,
        Ts const&... vs)
    {
        return reduce<detail::reduce_with_index<Derived>>(
            ids, HPX_FORWARD(ReduceOp, reduce_op), vs...);
    }
}}    // namespace hpx::lcos

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_REDUCE_ACTION_DECLARATION(...)                            \
    HPX_REGISTER_REDUCE_ACTION_DECLARATION_(__VA_ARGS__)                       \
/**/
#define HPX_REGISTER_REDUCE_ACTION_DECLARATION_(...)                           \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_REDUCE_ACTION_DECLARATION_,          \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_REGISTER_REDUCE_ACTION_DECLARATION_2(Action, ReduceOp)             \
    HPX_REGISTER_ACTION_DECLARATION(                                           \
        ::hpx::lcos::detail::make_reduce_action<                               \
            Action>::reduce_invoker_helper<ReduceOp>::type,                    \
        HPX_PP_CAT(HPX_PP_CAT(reduce_, Action), ReduceOp))                     \
/**/
#define HPX_REGISTER_REDUCE_ACTION_DECLARATION_3(Action, ReduceOp, Name)       \
    HPX_REGISTER_ACTION_DECLARATION(                                           \
        ::hpx::lcos::detail::make_reduce_action<                               \
            Action>::reduce_invoker_helper<ReduceOp>::type,                    \
        HPX_PP_CAT(reduce_, Name))                                             \
/**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_REDUCE_ACTION(...)                                        \
    HPX_REGISTER_REDUCE_ACTION_(__VA_ARGS__)                                   \
/**/
#define HPX_REGISTER_REDUCE_ACTION_(...)                                       \
    HPX_PP_EXPAND(HPX_PP_CAT(                                                  \
        HPX_REGISTER_REDUCE_ACTION_, HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))  \
    /**/

#define HPX_REGISTER_REDUCE_ACTION_2(Action, ReduceOp)                         \
    HPX_REGISTER_ACTION(::hpx::lcos::detail::make_reduce_action<               \
                            Action>::reduce_invoker_helper<ReduceOp>::type,    \
        HPX_PP_CAT(HPX_PP_CAT(reduce_, Action), ReduceOp))                     \
/**/
#define HPX_REGISTER_REDUCE_ACTION_3(Action, ReduceOp, Name)                   \
    HPX_REGISTER_ACTION(::hpx::lcos::detail::make_reduce_action<               \
                            Action>::reduce_invoker_helper<ReduceOp>::type,    \
        HPX_PP_CAT(reduce_, Name))                                             \
/**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_REDUCE_WITH_INDEX_ACTION_DECLARATION(...)                 \
    HPX_REGISTER_REDUCE_WITH_INDEX_ACTION_DECLARATION_(__VA_ARGS__)            \
/**/
#define HPX_REGISTER_REDUCE_WITH_INDEX_ACTION_DECLARATION_(...)                \
    HPX_PP_EXPAND(                                                             \
        HPX_PP_CAT(HPX_REGISTER_REDUCE_WITH_INDEX_ACTION_DECLARATION_,         \
            HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                           \
    /**/

#define HPX_REGISTER_REDUCE_WITH_INDEX_ACTION_DECLARATION_2(Action, ReduceOp)  \
    HPX_REGISTER_ACTION_DECLARATION(                                           \
        ::hpx::lcos::detail::make_reduce_action<                               \
            ::hpx::lcos::detail::reduce_with_index<Action>>::                  \
            reduce_invoker_helper<ReduceOp>::type,                             \
        HPX_PP_CAT(HPX_PP_CAT(reduce_, Action), ReduceOp))                     \
/**/
#define HPX_REGISTER_REDUCE_WITH_INDEX_ACTION_DECLARATION_3(                   \
    Action, ReduceOp, Name)                                                    \
    HPX_REGISTER_ACTION_DECLARATION(                                           \
        ::hpx::lcos::detail::make_reduce_action<                               \
            ::hpx::lcos::detail::reduce_with_index<Action>>::                  \
            reduce_invoker_helper<ReduceOp>::type,                             \
        HPX_PP_CAT(reduce_, Name))                                             \
/**/

///////////////////////////////////////////////////////////////////////////////
#define HPX_REGISTER_REDUCE_WITH_INDEX_ACTION(...)                             \
    HPX_REGISTER_REDUCE_WITH_INDEX_ACTION_(__VA_ARGS__)                        \
/**/
#define HPX_REGISTER_REDUCE_WITH_INDEX_ACTION_(...)                            \
    HPX_PP_EXPAND(HPX_PP_CAT(HPX_REGISTER_REDUCE_WITH_INDEX_ACTION_,           \
        HPX_PP_NARGS(__VA_ARGS__))(__VA_ARGS__))                               \
    /**/

#define HPX_REGISTER_REDUCE_WITH_INDEX_ACTION_2(Action, ReduceOp)              \
    HPX_REGISTER_ACTION(::hpx::lcos::detail::make_reduce_action<               \
                            ::hpx::lcos::detail::reduce_with_index<Action>>::  \
                            reduce_invoker_helper<ReduceOp>::type,             \
        HPX_PP_CAT(HPX_PP_CAT(reduce_, Action), ReduceOp))                     \
/**/
#define HPX_REGISTER_REDUCE_WITH_INDEX_ACTION_3(Action, ReduceOp, Name)        \
    HPX_REGISTER_ACTION(::hpx::lcos::detail::make_reduce_action<               \
                            ::hpx::lcos::detail::reduce_with_index<Action>>::  \
                            reduce_invoker_helper<ReduceOp>::type,             \
        HPX_PP_CAT(reduce_, Name))                                             \
    /**/

#endif    // DOXYGEN
