//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file dispatch_work.hpp
/// \page hpx::supervision::dispatch_work
/// \headerfile hpx/supervision_dispatch.hpp
///
/// \brief Fenced action dispatch for the supervision subsystem.
///
/// This header provides the machinery to dispatch an HPX action to a target
/// component while enforcing supervision "fencing" semantics: once a target has
/// latched a terminal event for a given epoch, no further actions for that
/// epoch may execute against it. Dispatch performs a cheap, non-authoritative
/// admission check on the caller's locality and an authoritative re-check on
/// the target's own locality (via hpx::colocated), immediately before invoking
/// the wrapped action, closing the race between admission and invocation.

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/supervision.hpp>

#include <cstdint>

#include <hpx/config/warnings_prefix.hpp>

/// \namespace hpx::supervision
///
/// Types and functions implementing HPX's supervision and fenced dispatch
/// facilities.
namespace hpx::supervision {

    /// \brief Remote dispatch point that performs the authoritative admission
    ///        re-check and invokes the wrapped action.
    ///
    /// This is the function body that actually runs as the action on the
    /// *destination* locality (the target's own locality, reached via
    /// hpx::colocated in dispatch_work()). It is invoked by fenced_action once
    /// the action has been dispatched.
    ///
    /// The admission check performed here runs on the same locality/thread that
    /// owns the target's supervision_manager state, so there is no suspension
    /// point between this check and the actual invocation below. This is what
    /// closes the client-side/dispatch race: even if the client's own
    /// (non-authoritative) check_admission() in dispatch_work() returned
    /// `admitted`, the target may have latched a terminal event for this epoch
    /// in the meantime, and this re-check will catch it.
    ///
    /// \tparam Action  The wrapped HPX action type to invoke on admission.
    /// \tparam IdType  Type of the target identifier forwarded to \p act (this
    ///                 type is always `hpx::id_type`).
    /// \tparam Epoch   Type of the fencing epoch value (this type is always
    ///                 `std::uint64_t`).
    /// \tparam Ts      Types of the additional arguments forwarded to \p act.
    ///
    /// \param act    The action instance to invoke once admission succeeds.
    /// \param target Identifier of the component instance being dispatched
    ///               to; forwarded to \p act (the epoch is fencing metadata
    ///               only and is not part of \p act's argument list).
    /// \param epoch  The fencing epoch to check admission for.
    /// \param ts     Additional arguments forwarded to \p act.
    ///
    /// \return The result of invoking \p act, obtained via hpx::sync (run
    ///         directly/locally, without an extra parcel hop, since this
    ///         function already executes on the target's locality).
    ///
    /// \throws hpx::exception with hpx::error::target_fenced if the target has
    ///         latched a terminal event for \p epoch since the client's own
    ///         admission check, in which case the wrapped action is not
    ///         invoked.
    template <typename Action, typename IdType, typename Epoch, typename... Ts>
    decltype(auto) invoke_fenced_action(
        Action act, IdType target, Epoch const epoch, Ts... ts)
    {
        // Authoritative re-check: runs on the same locality/thread that owns
        // the target's supervision_manager state, so there is no suspension
        // point between this check and the actual invocation below -- this is
        // what closes the client-side/dispatch race.
        dispatch_outcome const outcome =
            hpx::supervision::check_admission(target, epoch);

        if (outcome == dispatch_outcome::rejected_fenced)
        {
            // Target latched a terminal event for this epoch after the client's
            // own (non-authoritative) check_admission() returned admitted.
            // Surface this as an exceptional future so callers can distinguish
            // "fenced" dispatch failures from other errors.

            HPX_THROW_EXCEPTION(hpx::error::target_fenced,
                "hpx::supervision::invoke_fenced_action",
                "target has latched a terminal event for this epoch "
                "since the client-side admission check; dispatch was "
                "fenced and the wrapped action was not invoked");
        }

        // Admitted: forward straight into the wrapped Action. Note only
        // `target` (not `epoch`) is passed on -- epoch is fencing metadata
        // consumed entirely by this wrapper, not part of Action's own
        // argument list. hpx::sync runs the action body directly/locally
        // (no extra parcel hop) since we're already on target's locality.
        return hpx::sync(act, HPX_MOVE(target), HPX_MOVE(ts)...);
    }

    /// \brief HPX action wrapper for invoke_fenced_action().
    ///
    /// Registers invoke_fenced_action<Action, IdType, Epoch, Ts...> as an HPX
    /// action so it can be dispatched with hpx::async/hpx::colocated (see
    /// dispatch_work()).
    ///
    /// \tparam Action  The wrapped HPX action type.
    /// \tparam IdType  Type of the target identifier.
    /// \tparam Epoch   Type of the fencing epoch value.
    /// \tparam Ts      Types of the additional action arguments.
    template <typename Action, typename IdType, typename Epoch, typename... Ts>
    struct fenced_action
      : hpx::actions::make_action_t<
            decltype(&invoke_fenced_action<Action, IdType, Epoch, Ts...>),
            &invoke_fenced_action<Action, IdType, Epoch, Ts...>,
            fenced_action<Action, IdType, Epoch, Ts...>>
    {
    };

    /// \brief Dispatch an action to \p target under supervision fencing, by
    ///        action type template argument.
    ///
    /// This is the local dispatch point invoked by callers; it runs on the
    /// *caller's* locality. It first performs a cheap, non-authoritative
    /// admission check to avoid a wasted round trip when the caller's local
    /// view already shows the target fenced. This early-out does NOT by itself
    /// close the race -- the authoritative re-check happens later, inside
    /// invoke_fenced_action() on the target's own locality.
    ///
    /// On admission, the action is wrapped in fenced_action and dispatched via
    /// hpx::async to run *on the target's own locality* (through
    /// hpx::colocated), so that the re-check performed in
    /// invoke_fenced_action() is authoritative and executes on the same thread
    /// as the wrapped action invocation.
    ///
    /// \tparam Action The HPX action type to dispatch.
    /// \tparam Ts     Types of the additional arguments forwarded to \p Action.
    ///
    /// \param target Identifier of the target component instance.
    /// \param epoch  The fencing epoch to check admission for and to
    ///               validate authoritatively on the target's locality.
    /// \param ts     Additional arguments forwarded to the action.
    ///
    /// \return A future holding the result of the dispatched action. If the
    ///         local admission check fails, an already-exceptional future is
    ///         returned immediately without an actual dispatch.
    ///
    /// \note If either the local or the remote admission check fails, the
    ///       returned future is set to an exceptional state carrying
    ///       `hpx::error::target_fenced` (same error code/message shape as the
    ///       server-side fence in invoke_fenced_action(), giving callers a
    ///       single exception type to handle regardless of which side detected
    ///       the fence), and the wrapped action is not invoked.
    template <typename Action, typename... Ts>
    decltype(auto) dispatch_work(
        hpx::id_type const& target, std::uint64_t const epoch, Ts&&... ts)
    {
        // Cheap, non-authoritative early-out: avoids a wasted round trip when
        // the caller's local view already shows the target fenced. This does
        // NOT by itself close the race -- the authoritative re-check happens
        // later, inside invoke_fenced_action on the target's own locality.
        dispatch_outcome const outcome =
            hpx::supervision::check_admission(target, epoch);
        if (outcome == dispatch_outcome::rejected_fenced)
        {
            // Same error code/message shape as the server-side fence, so
            // callers get one exception type to handle regardless of which side
            // detected the fence.
            return hpx::make_exceptional_future<typename Action::result_type>(
                HPX_GET_EXCEPTION(hpx::error::target_fenced,
                    "hpx::supervision::detail::dispatch",
                    "target has already latched a terminal event for this "
                    "epoch; dispatch was fenced and the wrapped action was "
                    "not invoked"));
        }

        using fenced_action =
            fenced_action<Action, hpx::id_type, std::uint64_t, Ts...>;

        // Dispatch fenced_action to run *on target's own locality*
        // (hpx::colocated), so invoke_fenced_action's re-check above is
        // authoritative and same-thread with the wrapped Action call.
        return hpx::async(fenced_action(), hpx::colocated(target), Action(),
            target, epoch, HPX_FORWARD(Ts, ts)...);
    }

    /// \brief Dispatch an action to \p target under supervision fencing, by
    ///        action instance argument.
    ///
    /// Convenience overload of dispatch_work() constrained to HPX action types
    /// (via `hpx::traits::is_action_v<Action>`) that accepts an action instance
    /// rather than requiring the caller to specify \p Action explicitly as a
    /// template argument. Forwards directly to the primary dispatch_work()
    /// overload.
    ///
    /// \tparam Action The HPX action type to dispatch (deduced from the
    ///                \p Action instance argument).
    /// \tparam Ts     Types of the additional arguments forwarded to \p Action.
    ///
    /// \param target Identifier of the target component instance.
    /// \param epoch  The fencing epoch to check admission for.
    /// \param ts     Additional arguments forwarded to the action.
    ///
    /// \return See:
    ///         dispatch_work(hpx::id_type const&, std::uint64_t const, Ts&&...)
    template <typename Action, typename... Ts>
        requires(hpx::traits::is_action_v<Action>)
    decltype(auto) dispatch_work(Action, hpx::id_type const& target,
        std::uint64_t const epoch, Ts&&... ts)
    {
        return dispatch_work<Action>(target, epoch, HPX_FORWARD(Ts, ts)...);
    }
}    // namespace hpx::supervision

#include <hpx/config/warnings_suffix.hpp>
