//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/preprocessor.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/plugin_factories/device_conduit_factory_base.hpp>
#include <hpx/plugin_factories/macros.hpp>

#include <memory>
#include <string>
#include <vector>

namespace hpx::plugins {

    ///////////////////////////////////////////////////////////////////////////
    /// The factory of the device conduit 'Conduit' (derived from
    /// hpx::parcelset::device_conduit_impl<Conduit>, constructible from the
    /// runtime configuration, with a static name_impl()).
    HPX_CXX_EXPORT template <typename Conduit>
    struct device_conduit_factory : device_conduit_factory_base
    {
        /// Registers the factory (e.g. a static object in the application);
        /// it must exist before the runtime starts.
        device_conduit_factory()
        {
            add_device_conduit_factory(this);
        }

        /// A conduit compiled into HPX (see HPX_REGISTER_DEVICE_CONDUIT in
        /// hpx/plugin_factories/macros.hpp).
        explicit device_conduit_factory(
            std::vector<device_conduit_factory_base*>& factories)
        {
            factories.push_back(this);
        }

        device_conduit_factory(device_conduit_factory const&) = delete;
        device_conduit_factory(device_conduit_factory&&) = delete;
        device_conduit_factory& operator=(
            device_conduit_factory const&) = delete;
        device_conduit_factory& operator=(device_conduit_factory&&) = delete;

        ~device_conduit_factory() override = default;

        [[nodiscard]] std::string name() const override
        {
            return Conduit::name_impl();
        }

        void get_plugin_info(std::vector<std::string>& fillini) override
        {
            if constexpr (requires { Conduit::config_defaults(fillini); })
            {
                Conduit::config_defaults(fillini);
            }
        }

        [[nodiscard]] std::unique_ptr<parcelset::device_conduit> create(
            util::runtime_configuration const& cfg) override
        {
            return std::make_unique<Conduit>(cfg);
        }
    };
}    // namespace hpx::plugins

#endif
