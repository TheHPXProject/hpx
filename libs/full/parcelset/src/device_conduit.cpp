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
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
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

            // the smallest host chunk the conduit moves (where enabled)
            if (conduit->moves_host_memory())
            {
                serialization::detail::set_host_transport_threshold(
                    hpx::util::get_entry_as<std::size_t>(cfg,
                        "hpx.parcel.conduit_host_threshold",
                        std::size_t(1) << 20));
            }
        }

        void reset_device_conduit() noexcept
        {
            current_conduit.store(nullptr, std::memory_order_release);
            if (!conduit)
            {
                return;
            }

            // A conduit may complete a send after the message carrying its
            // descriptor (the completion refers to the conduit): wait for the
            // transfers still in flight. If they don't finish, keep the
            // conduit (a late completion must not touch a destroyed one);
            // such a completion is best effort: it releases the message's
            // objects after the runtime has gone.
            if (!conduit->drain(std::chrono::seconds(10)))
            {
                [[maybe_unused]] auto* leaked = conduit.release();
                return;
            }
            conduit.reset();
        }

        namespace {

            // what the endpoints of the destinations can do, by locality and
            // parcelport (see record_destination_capabilities)
            struct destination_capabilities
            {
                std::shared_mutex mtx;
                std::map<std::pair<std::uint32_t, std::string>,
                    locality_capabilities>
                    entries;
            };

            destination_capabilities& destinations()
            {
                static destination_capabilities d;
                return d;
            }
        }    // namespace

        void record_destination_capabilities(
            std::uint32_t const locality_id, endpoints_type const& endpoints)
        {
            if (get_device_conduit() == nullptr || endpoints.empty())
            {
                return;
            }

            auto& d = destinations();
            {
                std::shared_lock<std::shared_mutex> l(d.mtx);
                if (d.entries.contains({locality_id, endpoints.begin()->first}))
                {
                    return;
                }
            }

            std::unique_lock<std::shared_mutex> l(d.mtx);
            for (auto const& [type, endpoint] : endpoints)
            {
                d.entries.insert_or_assign(
                    {locality_id, type}, endpoint.capabilities());
            }
        }

        void forget_destination_capabilities(
            std::uint32_t const locality_id) noexcept
        {
            auto& d = destinations();
            std::unique_lock<std::shared_mutex> l(d.mtx);
            std::erase_if(d.entries, [&](auto const& entry) {
                return entry.first.first == locality_id;
            });
        }

        std::uint32_t device_conduit_flags(
            parcelport const& pp, std::uint32_t const locality_id) noexcept
        {
            device_conduit* c = get_device_conduit();
            if (c == nullptr || !c->can_reach(locality_id))
            {
                return 0;
            }

            // the destination uses the same conduit (its endpoint names it);
            // otherwise, or if it isn't known (yet), the parcelport moves the
            // data
            bool host_chunks = false;
            {
                auto& d = destinations();
                std::shared_lock<std::shared_mutex> l(d.mtx);
                auto const it = d.entries.find({locality_id, pp.type()});
                if (it == d.entries.end() || it->second.conduit != c->name())
                {
                    return 0;
                }
                host_chunks =
                    it->second.has(locality_capabilities::conduit_host_chunks);
            }

            auto flags = serialization::archive_flags::enable_device_transport |
                serialization::archive_flags::enable_device_chunks;

            // and large host chunks, if this parcelport lets the conduit
            // move them, the destination receives them that way, and the
            // conduit is ready to move them there; if it isn't (it may check
            // first that it reaches the destination), it is told when a
            // message has such a chunk, which goes with the message
            // meanwhile
            if (host_chunks && c->moves_host_memory() &&
                pp.conduit_host_chunks())
            {
                flags = flags |
                    (c->host_chunks_ready(locality_id) ?
                            serialization::archive_flags::
                                enable_host_transport :
                            serialization::archive_flags::
                                host_transport_pending);
            }
            return static_cast<std::uint32_t>(flags);
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
