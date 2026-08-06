//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \page hpx::supervision::supervision_manager
/// \file hpx/supervision/supervision_manager.hpp
/// \headerfile hpx/supervision.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/runtime_configuration.hpp>

#include <hpx/supervision/server/supervision_manager.hpp>
#include <hpx/supervision/supervision_fwd.hpp>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>

namespace hpx::supervision {

    ////////////////////////////////////////////////////////////////////////////
    class supervision_manager
    {
    public:
        explicit supervision_manager(util::runtime_configuration const& ini);

        // supervision API
        publish_result publish_event(hpx::id_type const& target, event ev,
            std::uint64_t epoch = 0, hpx::error_code& ec = throws) const;

        lifecycle_state query_state(
            hpx::id_type const& target, hpx::error_code& ec = throws) const;

        hpx::id_type register_observer(hpx::id_type const& target,
            hpx::id_type const& agent,
            std::uint64_t epoch_filter = static_cast<std::uint64_t>(-1),
            hpx::error_code& ec = throws) const;

        void unregister_observer(hpx::id_type const& observer_handle,
            hpx::error_code& ec = throws) const;

        /// \brief Clears all locally tracked state for \p target.
        ///
        /// \param target Target whose local supervision state is removed.
        /// \param ec Error code receiving the operation result.
        void remove_target(
            hpx::id_type const& target, hpx::error_code& ec = throws) const;

        // Register an agent to be notified of activation/deactivation
        // transitions across all targets tracked by this locality's supervision
        // manager. Unlike register_observer(), this is deliberately
        // locality-scoped rather than target-scoped: it takes no `target`
        // parameter by design. Registration will replay an `already_active`
        // notification for every currently-tracked active target. That replay
        // must appear to happen atomically with subscription, i.e. under the
        // same lock that guards the tracked-target set, so that no transition
        // is missed or duplicated across the replay/subscribe boundary.
        hpx::id_type register_activity_observer(hpx::id_type const& agent,
            std::uint64_t epoch_filter = static_cast<std::uint64_t>(-1),
            hpx::error_code& ec = throws) const;

        // Unregister a handle previously returned by
        // register_activity_observer(). As with unregister_observer(), no
        // orphaned callbacks fire after this call completes. `handle` must have
        // been obtained from register_activity_observer(), not from
        // register_observer(); passing a handle from the latter, or one that
        // was never returned by either registration API, is rejected with
        // hpx::error::bad_parameter.
        void unregister_activity_observer(hpx::id_type const& observer_handle,
            hpx::error_code& ec = throws) const;

        hpx::future<lifecycle_state> await_terminal(hpx::id_type const& target,
            std::uint64_t epoch = 0,
            std::chrono::steady_clock::duration timeout =
                (std::chrono::steady_clock::duration::max) (),
            hpx::error_code& ec = throws) const;

        dispatch_outcome check_admission(
            hpx::id_type const& target, std::uint64_t epoch = 0) const noexcept;

        // Helper functions
        void register_server_instance(error_code& ec = throws) const;
        void unregister_server_instance(error_code& ec = throws) const;

        static naming::gid_type get_service_instance(
            naming::gid_type const& dest, error_code& ec = throws);
        static naming::gid_type get_service_instance(
            std::uint32_t service_locality_id);

        static naming::gid_type get_service_instance(hpx::id_type const& dest)
        {
            return get_service_instance(dest.get_gid());
        }

    private:
        std::unique_ptr<server::supervision_manager> server_{};
    };

}    // namespace hpx::supervision
