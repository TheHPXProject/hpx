//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/modules/runtime_configuration.hpp>

#include <memory>
#include <string>
#include <vector>

namespace hpx::plugins {

    ///////////////////////////////////////////////////////////////////////////
    /// The base of the factories of device conduits
    /// (hpx::parcelset::device_conduit), like parcelport_factory_base for
    /// parcelports: the conduits compiled into HPX are listed statically
    /// (HPX_REGISTER_DEVICE_CONDUIT), others register their factory when it
    /// is constructed (e.g. a static device_conduit_factory<> object in the
    /// application).
    HPX_CXX_EXPORT struct HPX_EXPORT device_conduit_factory_base
    {
        virtual ~device_conduit_factory_base() = default;

        /// The name the conduit is selected by (hpx.parcel.device_conduit).
        [[nodiscard]] virtual std::string name() const = 0;

        /// Adds the configuration defaults of the conduit.
        virtual void get_plugin_info(std::vector<std::string>& fillini) = 0;

        /// Creates the conduit.
        [[nodiscard]] virtual std::unique_ptr<parcelset::device_conduit> create(
            util::runtime_configuration const& cfg) = 0;
    };

    ///////////////////////////////////////////////////////////////////////////
    /// The factories of the available conduits.
    HPX_CXX_EXPORT HPX_EXPORT std::vector<device_conduit_factory_base*>&
    get_device_conduit_factories();
    HPX_CXX_EXPORT HPX_EXPORT void add_device_conduit_factory(
        device_conduit_factory_base* factory);
}    // namespace hpx::plugins

#endif
