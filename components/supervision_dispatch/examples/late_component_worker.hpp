//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// A minimal, self-contained HPX component shared by the supervision_dispatch
// "late component" example/demo binaries (late_component_worker.cpp,
// late_component_failing_worker.cpp, and the eventual root launcher). It models
// a worker-owned component created *after* the worker has joined the
// supervision-dispatch system ("late" relative to root's own startup), and
// exposes just enough of a get/set message interface for those binaries to
// demonstrate that a worker is alive and serving real component work, without
// pulling in any actual application logic.

#pragma once

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>

#include <string>
#include <utility>

namespace late_component {

    namespace server {

        /// Server-side component exposing a trivial get/set message interface.
        class test_client : public hpx::components::component_base<test_client>
        {
        public:
            test_client() = default;

            explicit test_client(std::string msg)
              : message_(std::move(msg))
            {
            }

            void set_message(std::string const& msg)
            {
                message_ = msg;
            }

            std::string get_message() const
            {
                return message_;
            }

            HPX_DEFINE_COMPONENT_ACTION(
                test_client, set_message, set_message_action)
            HPX_DEFINE_COMPONENT_ACTION(
                test_client, get_message, get_message_action)

        private:
            std::string message_;
        };
    }    // namespace server

    /// Client-side representation of \a server::test_client.
    class test_client
      : public hpx::components::client_base<test_client, server::test_client>
    {
        using base_type =
            hpx::components::client_base<test_client, server::test_client>;

    public:
        test_client() = default;

        explicit test_client(hpx::id_type const& id)
          : base_type(id)
        {
        }

        explicit test_client(hpx::future<hpx::id_type>&& id)
          : base_type(std::move(id))
        {
        }

        void set_message(std::string const& msg) const
        {
            using action_type = server::test_client::set_message_action;
            action_type()(this->get_id(), msg);
        }

        std::string get_message() const
        {
            using action_type = server::test_client::get_message_action;
            return action_type()(this->get_id());
        }
    };
}    // namespace late_component

using late_component_test_client_set_message_action =
    late_component::server::test_client::set_message_action;
using late_component_test_client_get_message_action =
    late_component::server::test_client::get_message_action;

HPX_REGISTER_ACTION_DECLARATION(late_component_test_client_set_message_action)
HPX_REGISTER_ACTION_DECLARATION(late_component_test_client_get_message_action)
HPX_REGISTER_ACTION(late_component_test_client_set_message_action)
HPX_REGISTER_ACTION(late_component_test_client_get_message_action)

using late_component_test_client_type =
    hpx::components::component<late_component::server::test_client>;
HPX_REGISTER_COMPONENT(
    late_component_test_client_type, late_component_test_client)

#endif
