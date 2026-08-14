//  Copyright (c) 2018-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/resiliency/config.hpp>
#include <hpx/modules/async_local.hpp>

#include <utility>

namespace hpx::resiliency::experimental {

    ///////////////////////////////////////////////////////////////////////////
    // helper base classes implementing the CPO dispatch logic
    namespace detail {

        /// \brief Detects whether an ADL customization point
        ///   `hpx_invoke(tag, args...)` has been defined for the given CPO
        ///   \a Tag.
        ///
        /// \tparam Tag  The CPO tag type to check.
        /// \tparam Args The argument types forwarded to the hook.
        template <typename Tag, typename... Args>
        concept has_hpx_invoke = requires(Tag const& tag, Args&&... args) {
            hpx_invoke(tag, HPX_FORWARD(Args, args)...);
        };

        /// \brief CRTP mixin providing ADL-based dispatch for resiliency
        ///   CPOs.
        ///
        /// Implementations are supplied as free `hpx_invoke` ADL hooks in
        /// the same namespace as \a Tag. The CPO's `operator()` forwards
        /// to `hpx_invoke(tag, args...)`.
        ///
        /// \tparam Tag  The concrete CPO tag type (CRTP derived class).
        template <typename Tag>
        // NOLINTNEXTLINE(bugprone-crtp-constructor-accessibility)
        struct dispatch_cpo
        {
            template <typename... Args>
                requires(has_hpx_invoke<Tag, Args...>)
            decltype(auto) operator()(Args&&... args) const
            {
                Tag const& tag = static_cast<Tag const&>(*this);
                return hpx_invoke(tag, HPX_FORWARD(Args, args)...);
            }
        };

        // helper used by tag_deferred to unwrap the inner future returned by
        // hpx::dataflow (e.g. dataflow_replay produces a future<future<T>> from
        // the base async_replay CPO, which we want to return as future<T>).
        HPX_CXX_CORE_EXPORT template <typename Future>
        auto unwrap_dataflow_future(Future&& f)
        {
            using result_type =
                hpx::traits::future_traits_t<std::decay_t<Future>>;
            return result_type(HPX_FORWARD(Future, f));
        }

        // helper base class implementing the deferred dispatch logic for CPOs
        HPX_CXX_CORE_EXPORT template <typename Tag, typename BaseTag>
        // NOLINTNEXTLINE(bugprone-crtp-constructor-accessibility)
        struct tag_deferred
        {
            // force unwrapping of the inner future on return
            template <typename... Args>
            HPX_FORCEINLINE auto operator()(Args&&... args) const
            {
                auto f = hpx::dataflow(BaseTag{}, HPX_FORWARD(Args, args)...);
                return unwrap_dataflow_future(HPX_MOVE(f));
            }
        };
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // Replay customization points

    /// Customization point for asynchronously launching the given function \a
    /// f. repeatedly. Verify the result of those invocations using the given
    /// predicate \a pred. Repeat launching on error exactly \a n times (except
    /// if abort_replay_exception is thrown).
    HPX_CXX_CORE_EXPORT inline constexpr struct async_replay_validate_t final
      : detail::dispatch_cpo<async_replay_validate_t>
    {
    } async_replay_validate{};

    /// Customization point for asynchronously launching given function \a f
    /// repeatedly. Repeat launching on error exactly \a n times (except if
    /// abort_replay_exception is thrown).
    HPX_CXX_CORE_EXPORT inline constexpr struct async_replay_t final
      : detail::dispatch_cpo<async_replay_t>
    {
    } async_replay{};

    /// Customization point for asynchronously launching the given function \a
    /// f. repeatedly. Verify the result of those invocations using the given
    /// predicate \a pred. Repeat launching on error exactly \a n times.
    ///
    /// Delay the invocation of \a f if any of the arguments to \a f are
    /// futures.
    HPX_CXX_CORE_EXPORT inline constexpr struct dataflow_replay_validate_t final
      : detail::tag_deferred<dataflow_replay_validate_t,
            async_replay_validate_t>
    {
    } dataflow_replay_validate{};

    /// Customization point for asynchronously launching the given function \a
    /// f. repeatedly. Repeat launching on error exactly \a n times.
    ///
    /// Delay the invocation of \a f if any of the arguments to \a f are
    /// futures.
    HPX_CXX_CORE_EXPORT inline constexpr struct dataflow_replay_t final
      : detail::tag_deferred<dataflow_replay_t, async_replay_t>
    {
    } dataflow_replay{};

    ///////////////////////////////////////////////////////////////////////////
    // Replicate customization points

    /// Customization point for asynchronously launching the given function \a f
    /// exactly \a n times concurrently. Verify the result of those invocations
    /// using the given predicate \a pred. Run all the valid results against a
    /// user provided voting function. Return the valid output.
    HPX_CXX_CORE_EXPORT inline constexpr struct async_replicate_vote_validate_t
        final : detail::dispatch_cpo<async_replicate_vote_validate_t>
    {
    } async_replicate_vote_validate{};

    ///////////////////////////////////////////////////////////////////////////
    /// Customization point for asynchronously launching the given function \a f
    /// exactly \a n times concurrently. Verify the result of those invocations
    /// using the given predicate \a pred. Run all the valid results against a
    /// user provided voting function. Return the valid output.
    HPX_CXX_CORE_EXPORT inline constexpr struct async_replicate_vote_t final
      : detail::dispatch_cpo<async_replicate_vote_t>
    {
    } async_replicate_vote{};

    ///////////////////////////////////////////////////////////////////////////
    /// Customization point for asynchronously launching the given function \a f
    /// exactly \a n times concurrently. Verify the result of those invocations
    /// using the given predicate \a pred. Return the first valid result.
    HPX_CXX_CORE_EXPORT inline constexpr struct async_replicate_validate_t final
      : detail::dispatch_cpo<async_replicate_validate_t>
    {
    } async_replicate_validate{};

    ///////////////////////////////////////////////////////////////////////////
    /// Customization point for asynchronously launching the given function \a f
    /// exactly \a n times concurrently. Verify the result of those invocations
    /// by checking for exception. Return the first valid result.
    HPX_CXX_CORE_EXPORT inline constexpr struct async_replicate_t final
      : detail::dispatch_cpo<async_replicate_t>
    {
    } async_replicate{};

    /// Customization point for asynchronously launching the given function \a f
    /// exactly \a n times concurrently. Run all the valid results against a
    /// user provided voting function. Return the valid output.
    ///
    /// Delay the invocation of \a f if any of the arguments to \a f are
    /// futures.
    HPX_CXX_CORE_EXPORT inline constexpr struct
        dataflow_replicate_vote_validate_t final
      : detail::tag_deferred<dataflow_replicate_vote_validate_t,
            async_replicate_vote_validate_t>
    {
    } dataflow_replicate_vote_validate{};

    /// Customization point for asynchronously launching the given function \a f
    /// exactly \a n times concurrently. Run all the valid results against a
    /// user provided voting function. Return the valid output.
    ///
    /// Delay the invocation of \a f if any of the arguments to \a f are
    /// futures.
    HPX_CXX_CORE_EXPORT inline constexpr struct dataflow_replicate_vote_t final
      : detail::tag_deferred<dataflow_replicate_vote_t, async_replicate_vote_t>
    {
    } dataflow_replicate_vote{};

    /// Customization point for asynchronously launching the given function \a f
    /// exactly \a n times concurrently. Verify the result of those invocations
    /// using the given predicate \a pred. Return the first valid result.
    ///
    /// Delay the invocation of \a f if any of the arguments to \a f are
    /// futures.
    HPX_CXX_CORE_EXPORT inline constexpr struct dataflow_replicate_validate_t
        final
      : detail::tag_deferred<dataflow_replicate_validate_t,
            async_replicate_validate_t>
    {
    } dataflow_replicate_validate{};

    /// Customization point for asynchronously launching the given function \a f
    /// exactly \a n times concurrently. Return the first valid result.
    ///
    /// Delay the invocation of \a f if any of the arguments to \a f are
    /// futures.
    HPX_CXX_CORE_EXPORT inline constexpr struct dataflow_replicate_t final
      : detail::tag_deferred<dataflow_replicate_t, async_replicate_t>
    {
    } dataflow_replicate{};
}    // namespace hpx::resiliency::experimental
