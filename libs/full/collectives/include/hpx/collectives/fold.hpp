//  Copyright (c) 2013-2026 Hartmut Kaiser
//  Copyright (c) 2013 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file fold.hpp

#pragma once

#if defined(DOXYGEN)
namespace hpx { namespace lcos {

    /// \brief Perform a distributed fold operation
    ///
    /// The function hpx::lcos::fold performs a distributed folding
    /// operation over results returned from action invocations on a given set
    /// of global identifiers. The action can be either a plain action (in
    /// which case the global identifiers have to refer to localities) or a
    /// component action (in which case the global identifiers have to refer
    /// to instances of a component type which exposes the action.
    ///
    /// \param ids       [in] A list of global identifiers identifying the
    ///                  target objects for which the given action will be
    ///                  invoked.
    /// \param fold_op   [in] A binary function expecting two results as
    ///                  returned from the action invocations. The function
    ///                  (or function object) is expected to return the result
    ///                  of the folding operation performed on its arguments.
    /// \param init      [in] The initial value to be used for the folding
    ///                  operation
    /// \param argN      [in] Any number of arbitrary arguments (passed by
    ///                  value, by const reference or by rvalue reference)
    ///                  which will be forwarded to the action invocation.
    ///
    /// \note            The type of the initial value must be convertible to
    ///                  the result type returned from the invoked action.
    ///
    /// \returns         This function returns a future representing the result
    ///                  of the overall folding operation.
    ///
    template <typename Action, typename FoldOp, typename Init, typename ArgN,
        ...>
    hpx::future<decltype(Action(hpx::id_type, ArgN, ...))> fold(
        std::vector<hpx::id_type> const& ids, FoldOp&& fold_op, Init&& init,
        ArgN argN, ...);

    /// \brief Perform a distributed folding operation
    ///
    /// The function hpx::lcos::fold_with_index performs a distributed folding
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
    /// \param fold_op [in] A binary function expecting two results as
    ///                  returned from the action invocations. The function
    ///                  (or function object) is expected to return the result
    ///                  of the folding operation performed on its arguments.
    /// \param init      [in] The initial value to be used for the folding
    ///                  operation
    /// \param argN      [in] Any number of arbitrary arguments (passed by
    ///                  value, by const reference or by rvalue reference)
    ///                  which will be forwarded to the action invocation.
    ///
    /// \note            The type of the initial value must be convertible to
    ///                  the result type returned from the invoked action.
    ///
    /// \returns         This function returns a future representing the result
    ///                  of the overall folding operation.
    ///
    template <typename Action, typename FoldOp, typename Init, typename ArgN,
        ...>
    hpx::future<decltype(Action(hpx::id_type, ArgN, ..., std::size_t))>
    fold_with_index(std::vector<hpx::id_type> const& ids, FoldOp&& fold_op,
        Init&& init, ArgN argN, ...);

    /// \brief Perform a distributed inverse folding operation
    ///
    /// The function hpx::lcos::inverse_fold performs an inverse distributed folding
    /// operation over results returned from action invocations on a given set
    /// of global identifiers. The action can be either a plain action (in
    /// which case the global identifiers have to refer to localities) or a
    /// component action (in which case the global identifiers have to refer
    /// to instances of a component type which exposes the action.
    ///
    /// \param ids       [in] A list of global identifiers identifying the
    ///                  target objects for which the given action will be
    ///                  invoked.
    /// \param fold_op   [in] A binary function expecting two results as
    ///                  returned from the action invocations. The function
    ///                  (or function object) is expected to return the result
    ///                  of the folding operation performed on its arguments.
    /// \param init      [in] The initial value to be used for the folding
    ///                  operation
    /// \param argN      [in] Any number of arbitrary arguments (passed by
    ///                  value, by const reference or by rvalue reference)
    ///                  which will be forwarded to the action invocation.
    ///
    /// \note            The type of the initial value must be convertible to
    ///                  the result type returned from the invoked action.
    ///
    /// \returns         This function returns a future representing the result
    ///                  of the overall folding operation.
    ///
    template <typename Action, typename FoldOp, typename Init, typename ArgN,
        ...>
    hpx::future<decltype(Action(hpx::id_type, ArgN, ...))> inverse_fold(
        std::vector<hpx::id_type> const& ids, FoldOp&& fold_op, Init&& init,
        ArgN argN, ...);

    /// \brief Perform a distributed inverse folding operation
    ///
    /// The function hpx::lcos::inverse_fold_with_index performs an inverse
    /// distributed folding operation over results returned from action
    /// invocations on a given set of global identifiers. The action can be
    /// either plain action (in
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
    /// \param fold_op [in] A binary function expecting two results as
    ///                  returned from the action invocations. The function
    ///                  (or function object) is expected to return the result
    ///                  of the folding operation performed on its arguments.
    /// \param init      [in] The initial value to be used for the folding
    ///                  operation
    /// \param argN      [in] Any number of arbitrary arguments (passed by
    ///                  value, by const reference or by rvalue reference)
    ///                  which will be forwarded to the action invocation.
    ///
    /// \note            The type of the initial value must be convertible to
    ///                  the result type returned from the invoked action.
    ///
    /// \returns         This function returns a future representing the result
    ///                  of the overall folding operation.
    ///
    template <typename Action, typename FoldOp, typename Init, typename ArgN,
        ...>
    hpx::future<decltype(Action(hpx::id_type, ArgN, ..., std::size_t))>
    inverse_fold_with_index(std::vector<hpx::id_type> const& ids,
        FoldOp&& fold_op, Init&& init, ArgN argN, ...);
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

#include <hpx/collectives/macros.hpp>

#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::lcos {

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_EXPORT template <typename Action>
        struct fold_with_index
        {
            using arguments_type = typename Action::arguments_type;
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Action>
        struct fold_result
          : traits::promise_local_result<typename hpx::traits::extract_action<
                Action>::remote_result_type>
        {
        };

        template <typename Action>
        struct fold_result<fold_with_index<Action>> : fold_result<Action>
        {
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Action, typename FoldOp, typename... Ts>
        typename fold_result<Action>::type fold_impl(Action const& act,
            std::vector<hpx::id_type> const& ids, FoldOp&& fold_op,
            typename fold_result<Action>::type const& init, long global_idx,
            Ts const&... vs);

        ///////////////////////////////////////////////////////////////////////
        template <typename Action, typename Futures, typename... Ts>
        void fold_invoke(Action, Futures& futures, hpx::id_type const& id,
            std::size_t, Ts const&... vs)
        {
            futures.push_back(hpx::async<Action>(id, vs...));
        }

        template <typename Action, typename Futures, typename... Ts>
        void fold_invoke(fold_with_index<Action>, Futures& futures,
            hpx::id_type const& id, std::size_t global_idx, Ts const&... vs)
        {
            futures.push_back(hpx::async<Action>(id, vs..., global_idx));
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename Action, typename FoldOp, typename... Ts>
        struct fold_invoker
        {
            using result_type = typename fold_result<Action>::type;

            static result_type call(Action const& act,
                std::vector<hpx::id_type> const& ids, FoldOp const& fold_op,
                result_type const& init, long global_idx, Ts const&... vs)
            {
                return fold_impl(act, ids, fold_op, init, global_idx, vs...);
            }
        };

        ///////////////////////////////////////////////////////////////////////
        HPX_CXX_EXPORT template <typename Action, typename Is>
        struct make_fold_action_impl;

        template <typename Action, std::size_t... Is>
        struct make_fold_action_impl<Action, util::index_pack<Is...>>
        {
            using action_result = typename fold_result<Action>::type;

            template <typename FoldOp>
            struct fold_invoker
            {
                using fold_op_type = typename std::decay<FoldOp>::type;

                using fold_invoker_type =
                    detail::fold_invoker<Action, fold_op_type,
                        typename hpx::tuple_element<Is,
                            typename Action::arguments_type>::type...>;

                using type = typename HPX_MAKE_ACTION(
                    fold_invoker_type::call)::type;
            };
        };

        HPX_CXX_EXPORT template <typename Action>
        struct make_fold_action
          : make_fold_action_impl<Action,
                typename util::make_index_pack<Action::arity>::type>
        {
        };

        template <typename Action>
        struct make_fold_action<fold_with_index<Action>>
          : make_fold_action_impl<fold_with_index<Action>,
                typename util::make_index_pack<Action::arity - 1>::type>
        {
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Result, typename FoldOp>
        struct perform_folding
        {
            perform_folding(FoldOp const& fold_op, Result const& init)
              : fold_op_(fold_op)
              , init_(init)
            {
            }

            Result operator()(
                hpx::future<std::vector<hpx::future<Result>>> r) const
            {
                std::vector<hpx::future<Result>> fres = HPX_MOVE(r.get());
                HPX_ASSERT(!fres.empty());

                // we're at the beginning of the folding chain, incorporate the
                // initial value
                if (fres.size() == 1)
                    return fold_op_(init_, fres[0].get());

                // in the middle of the folding chain we simply apply the folding
                // operation to the two values we received.
                HPX_ASSERT(fres.size() == 2);
                return fold_op_(fres[0].get(), fres[1].get());
            }

            FoldOp const& fold_op_;
            Result const& init_;
        };

        ///////////////////////////////////////////////////////////////////////
        template <typename Action, typename FoldOp, typename... Ts>
        typename fold_result<Action>::type fold_impl(Action const& act,
            std::vector<hpx::id_type> const& ids, FoldOp&& fold_op,
            typename fold_result<Action>::type const& init, long global_idx,
            Ts const&... vs)
        {
            using result_type = typename fold_result<Action>::type;

            if (ids.empty())
                return result_type();

            std::vector<hpx::future<result_type>> fold_futures;
            fold_futures.reserve(2);

            // first kick off the possibly remote operation
            id_type id_first = ids.front();
            if (ids.size() > 1)
            {
                using fold_impl_action = typename detail::make_fold_action<
                    Action>::template fold_invoker<FoldOp>::type;

                std::vector<id_type> ids_next(ids.begin() + 1, ids.end());
                fold_futures.push_back(
                    hpx::detail::async_colocated<fold_impl_action>(
                        ids_next.front(), act, HPX_MOVE(ids_next), fold_op,
                        init, global_idx + 1, vs...));
            }

            // now perform the local operation
            fold_invoke(
                act, fold_futures, id_first, std::abs(global_idx), vs...);

            return hpx::when_all(fold_futures)
                .then(perform_folding<result_type, FoldOp>(fold_op, init))
                .get();
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename FoldOp, typename Init,
        typename... Ts>
    hpx::future<typename detail::fold_result<Action>::type> fold(
        std::vector<hpx::id_type> const& ids, FoldOp&& fold_op, Init&& init,
        Ts const&... vs)
    {
        using action_result = typename detail::fold_result<Action>::type;

        if (ids.empty())
        {
            return hpx::make_exceptional_future<action_result>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter, "hpx::lcos::fold",
                    "empty list of targets for fold operation"));
        }

        using fold_impl_action = typename detail::make_fold_action<
            Action>::template fold_invoker<FoldOp>::type;

        return hpx::detail::async_colocated<fold_impl_action>(ids[0], Action(),
            ids, HPX_FORWARD(FoldOp, fold_op), HPX_FORWARD(Init, init), 0,
            vs...);
    }

    HPX_CXX_EXPORT template <typename Component, typename Signature,
        typename Derived, typename FoldOp, typename Init, typename... Ts>
    hpx::future<typename detail::fold_result<Derived>::type> fold(
        hpx::actions::basic_action<Component, Signature, Derived> /* act */
        ,
        std::vector<hpx::id_type> const& ids, FoldOp&& fold_op, Init&& init,
        Ts const&... vs)
    {
        return fold<Derived>(
            ids, HPX_FORWARD(FoldOp, fold_op), HPX_FORWARD(Init, init), vs...);
    }

    HPX_CXX_EXPORT template <typename Action, typename FoldOp, typename Init,
        typename... Ts>
    hpx::future<typename detail::fold_result<Action>::type> fold_with_index(
        std::vector<hpx::id_type> const& ids, FoldOp&& fold_op, Init&& init,
        Ts const&... vs)
    {
        return fold<detail::fold_with_index<Action>>(
            ids, HPX_FORWARD(FoldOp, fold_op), HPX_FORWARD(Init, init), vs...);
    }

    HPX_CXX_EXPORT template <typename Component, typename Signature,
        typename Derived, typename FoldOp, typename Init, typename... Ts>
    hpx::future<typename detail::fold_result<Derived>::type> fold_with_index(
        hpx::actions::basic_action<Component, Signature, Derived> /* act */
        ,
        std::vector<hpx::id_type> const& ids, FoldOp&& fold_op, Init&& init,
        Ts const&... vs)
    {
        return fold<detail::fold_with_index<Derived>>(
            ids, HPX_FORWARD(FoldOp, fold_op), HPX_FORWARD(Init, init), vs...);
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT template <typename Action, typename FoldOp, typename Init,
        typename... Ts>
    hpx::future<typename detail::fold_result<Action>::type> inverse_fold(
        std::vector<hpx::id_type> const& ids, FoldOp&& fold_op, Init&& init,
        Ts const&... vs)
    {
        using action_result = typename detail::fold_result<Action>::type;

        if (ids.empty())
        {
            return hpx::make_exceptional_future<action_result>(
                HPX_GET_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::lcos::inverse_fold",
                    "empty list of targets for inverse_fold operation"));
        }

        std::vector<id_type> inverted_ids;
        std::reverse_copy(
            ids.begin(), ids.end(), std::back_inserter(inverted_ids));

        using fold_impl_action = typename detail::make_fold_action<
            Action>::template fold_invoker<FoldOp>::type;

        if (inverted_ids.empty())
        {
            return hpx::make_exceptional_future<action_result>(hpx::exception(
                hpx::error::bad_parameter, "array of targets is empty"));
        }

        return hpx::detail::async_colocated<fold_impl_action>(inverted_ids[0],
            Action(), inverted_ids, HPX_FORWARD(FoldOp, fold_op),
            HPX_FORWARD(Init, init),
            -static_cast<long>(inverted_ids.size() - 1), vs...);
    }

    HPX_CXX_EXPORT template <typename Component, typename Signature,
        typename Derived, typename FoldOp, typename Init, typename... Ts>
    hpx::future<typename detail::fold_result<Derived>::type> inverse_fold(
        hpx::actions::basic_action<Component, Signature, Derived> /* act */
        ,
        std::vector<hpx::id_type> const& ids, FoldOp&& fold_op, Init&& init,
        Ts const&... vs)
    {
        return inverse_fold<Derived>(
            ids, HPX_FORWARD(FoldOp, fold_op), HPX_FORWARD(Init, init), vs...);
    }

    HPX_CXX_EXPORT template <typename Action, typename FoldOp, typename Init,
        typename... Ts>
    hpx::future<typename detail::fold_result<Action>::type>
    inverse_fold_with_index(std::vector<hpx::id_type> const& ids,
        FoldOp&& fold_op, Init&& init, Ts const&... vs)
    {
        return inverse_fold<detail::fold_with_index<Action>>(
            ids, HPX_FORWARD(FoldOp, fold_op), HPX_FORWARD(Init, init), vs...);
    }

    HPX_CXX_EXPORT template <typename Component, typename Signature,
        typename Derived, typename FoldOp, typename Init, typename... Ts>
    hpx::future<typename detail::fold_result<Derived>::type>
    inverse_fold_with_index(
        hpx::actions::basic_action<Component, Signature, Derived> /* act */
        ,
        std::vector<hpx::id_type> const& ids, FoldOp&& fold_op, Init&& init,
        Ts const&... vs)
    {
        return inverse_fold<detail::fold_with_index<Derived>>(
            ids, HPX_FORWARD(FoldOp, fold_op), HPX_FORWARD(Init, init), vs...);
    }
}    // namespace hpx::lcos

#endif    // DOXYGEN
