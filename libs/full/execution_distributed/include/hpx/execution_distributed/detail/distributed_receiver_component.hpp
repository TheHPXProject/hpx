//  Copyright (c) 2026 Shivansh Singh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

/// \file distributed_receiver_component.hpp
/// \brief HPX component that bridges the P2300 receiver protocol across
///        locality boundaries.
///
/// Architecture Overview (nvexec-style boundary crossing):
///
///   LOCAL LOCALITY                     REMOTE LOCALITY
///   ==============                     ===============
///
///   upstream_sender                    [work executes here]
///        |                                    |
///        v                                    |
///   continues_on(sched)                       |
///        |                                    |
///        v                                    |
///   distributed_op_state ---- parcel ---->  remote work
///        |                                    |
///        |  <---- set_value_action(T) -----   |
///        |  <---- set_error_action(eptr) --   |
///        |  <---- set_stopped_action() ----   |
///        v                                    |
///   downstream_receiver                       |
///
/// The component is created on the LOCAL locality (where the downstream
/// receiver lives). When the remote locality completes its work, it
/// invokes one of the three component actions to signal completion
/// back across the network boundary. The component then forwards the
/// signal to the type-erased downstream receiver.
///
/// This is analogous to how nvexec bridges GPU/CPU boundaries: the
/// "device" (remote locality) signals completion to the "host" (local
/// locality) through a well-defined, serializable interface.
///
/// Serialization Boundary:
///   - set_value_action: The value type T is expected to be a
///     hpx::tuple<Ts...> packing the multi-value result (following the
///     standard sender/receiver contract). T must be serializable via
///     HPX's serialization framework.
///   - set_error_action: Uses std::exception_ptr, which HPX already
///     knows how to serialize across localities.
///   - set_stopped_action: No payload, always serializable.

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)

#include <hpx/assert.hpp>
#include <hpx/modules/actions_base.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/naming_base.hpp>

#include <cstddef>
#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

namespace hpx::distributed::experimental::detail {

    ///////////////////////////////////////////////////////////////////////////
    /// Type-erased receiver wrapper.
    ///
    /// The downstream receiver is type-erased behind this virtual interface,
    /// parameterized on the value type T. T is expected to be a
    /// hpx::tuple<Ts...> representing the packed multi-value result.
    /// For void execution (no values), T is hpx::tuple<> (empty tuple).
    ///
    /// All completion methods are noexcept, conforming to the standard
    /// sender/receiver protocol (P2300 section 5.5).
    template <typename T>
    struct receiver_bridge_base
    {
        virtual ~receiver_bridge_base() = default;
        virtual void complete_value(T val) noexcept = 0;
        virtual void complete_error(std::exception_ptr ep) noexcept = 0;
        virtual void complete_stopped() noexcept = 0;
    };

    ///////////////////////////////////////////////////////////////////////////
    /// Concrete receiver bridge that wraps a real P2300 receiver.
    ///
    /// Captures the downstream receiver by value and forwards completion
    /// signals to it using the P2300 protocol. Handles void (empty tuple)
    /// and multi-value (non-empty tuple) cases via if constexpr.
    template <typename Receiver, typename T>
    struct receiver_bridge final : receiver_bridge_base<T>
    {
        using receiver_type = std::decay_t<Receiver>;

        explicit receiver_bridge(Receiver&& rcvr) noexcept
          : receiver_(HPX_MOVE(rcvr))
        {
        }

        void complete_value(T val) noexcept override
        {
            if constexpr (hpx::tuple_size<T>::value == 0)
            {
                // Void case: empty tuple, call set_value with no value args.
                hpx::execution::experimental::set_value(HPX_MOVE(receiver_));
            }
            else
            {
                // Forward the packed tuple directly to set_value.
                hpx::execution::experimental::set_value(
                    HPX_MOVE(receiver_), HPX_MOVE(val));
            }
        }

        void complete_error(std::exception_ptr ep) noexcept override
        {
            hpx::execution::experimental::set_error(
                HPX_MOVE(receiver_), HPX_MOVE(ep));
        }

        void complete_stopped() noexcept override
        {
            hpx::execution::experimental::set_stopped(HPX_MOVE(receiver_));
        }

    private:
        receiver_type receiver_;
    };

    ///////////////////////////////////////////////////////////////////////////
    /// HPX Component: distributed_receiver_component<T>
    ///
    /// This component lives on the LOCAL locality. It holds a type-erased
    /// receiver bridge and exposes three component actions that the REMOTE
    /// locality can invoke to signal completion.
    ///
    /// Template parameter T is the packed value type (typically a
    /// hpx::tuple<Ts...>). For void execution, use T = hpx::tuple<>.
    ///
    /// Lifecycle:
    ///   1. Created by the local operation_state during connect().
    ///   2. Its GID is sent to the remote locality as part of the
    ///      remote dispatch parcel.
    ///   3. The remote locality invokes set_value_action / set_error_action /
    ///      set_stopped_action on this GID when work completes.
    ///   4. The action handler forwards to the type-erased receiver bridge.
    ///   5. The component is destroyed when the operation_state is destroyed.
    template <typename T>
    class distributed_receiver_component
      : public hpx::components::component_base<
            distributed_receiver_component<T>>
    {
    public:
        using base_type =
            hpx::components::component_base<distributed_receiver_component<T>>;

        distributed_receiver_component() = default;

        template <typename Receiver>
        explicit distributed_receiver_component(Receiver&& rcvr)
          : bridge_(
                std::make_unique<receiver_bridge<Receiver, T>>(HPX_MOVE(rcvr)))
        {
        }

        // --- Action entry points ---
        // These are the non-virtual functions that HPX_DEFINE_COMPONENT_ACTION
        // wraps into serializable actions.

        void on_set_value(T val)
        {
            HPX_ASSERT(bridge_);
            bridge_->complete_value(HPX_MOVE(val));
        }

        void on_set_error(std::exception_ptr ep)
        {
            HPX_ASSERT(bridge_);
            bridge_->complete_error(HPX_MOVE(ep));
        }

        void on_set_stopped()
        {
            HPX_ASSERT(bridge_);
            bridge_->complete_stopped();
        }

        HPX_DEFINE_COMPONENT_ACTION(
            distributed_receiver_component, on_set_value, set_value_action)
        HPX_DEFINE_COMPONENT_ACTION(
            distributed_receiver_component, on_set_error, set_error_action)
        HPX_DEFINE_COMPONENT_ACTION(
            distributed_receiver_component, on_set_stopped, set_stopped_action)

    private:
        std::unique_ptr<receiver_bridge_base<T>> bridge_;
    };

}    // namespace hpx::distributed::experimental::detail

#endif    // HPX_HAVE_NETWORKING
