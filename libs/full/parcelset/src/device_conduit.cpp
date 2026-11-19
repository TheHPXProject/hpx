//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/modules/plugin_factories.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/parcelset/device_conduit.hpp>
#include <hpx/parcelset/init_parcelports.hpp>

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace hpx::parcelset {

    namespace {

        // the selected conduit; set once at startup, before parcels carry
        // device data, reset at shutdown
        std::unique_ptr<device_conduit> conduit;
        std::atomic<device_conduit*> current_conduit = nullptr;
    }    // namespace

    device_conduit* get_device_conduit() noexcept
    {
        return current_conduit.load(std::memory_order_acquire);
    }

    namespace detail {

        std::vector<plugins::device_conduit_factory_base*>&
        get_device_conduit_factories()
        {
            // the application's factories registered themselves already
            static bool const static_factories_added = [] {
                if (init_static_device_conduit_factories != nullptr)
                {
                    init_static_device_conduit_factories(
                        plugins::get_device_conduit_factories());
                }
                return true;
            }();
            HPX_UNUSED(static_factories_added);
            return plugins::get_device_conduit_factories();
        }

        void init_device_conduit(util::runtime_configuration const& cfg)
        {
            std::string const name =
                cfg.get_entry("hpx.parcel.device_conduit", "none");
            if (name == "none")
            {
                return;
            }

            plugins::device_conduit_factory_base* factory = nullptr;
            for (auto* f : get_device_conduit_factories())
            {
                if (f->name() == name)
                {
                    factory = f;
                    break;
                }
            }
            if (factory == nullptr)
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::parcelset::detail::init_device_conduit",
                    "no device conduit '{}' is available (see "
                    "hpx.parcel.device_conduit)",
                    name);
            }

            HPX_ASSERT(!conduit);
            conduit = factory->create(cfg);
            current_conduit.store(conduit.get(), std::memory_order_release);
        }

        void reset_device_conduit() noexcept
        {
            current_conduit.store(nullptr, std::memory_order_release);
            conduit.reset();
        }

        std::uint32_t device_conduit_flags(
            std::uint32_t const locality_id) noexcept
        {
            device_conduit const* c = get_device_conduit();
            if (c == nullptr || !c->can_reach(locality_id))
            {
                return 0;
            }
            return static_cast<std::uint32_t>(
                serialization::archive_flags::enable_device_transport |
                serialization::archive_flags::enable_device_chunks);
        }

        void attach_device_conduit_sender(
            serialization::output_archive& archive,
            std::uint32_t const locality_id, std::shared_ptr<void> keep_alive)
        {
            if (!archive.enable_device_transport())
            {
                return;
            }

            device_conduit* c = get_device_conduit();
            HPX_ASSERT(c != nullptr && c->can_reach(locality_id));
            archive.get_extra_data<serialization::detail::device_transport>()
                .transport = c->sender(locality_id, HPX_MOVE(keep_alive));
        }

        void attach_device_conduit_receiver(
            serialization::input_archive& archive)
        {
            // only messages whose sender used its conduit need one
            if (!archive.enable_device_transport())
            {
                return;
            }
            if (device_conduit* c = get_device_conduit())
            {
                archive
                    .get_extra_data<serialization::detail::device_transport>()
                    .transport = c->receiver();
            }
        }
    }    // namespace detail
}    // namespace hpx::parcelset

#endif
