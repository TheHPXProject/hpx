//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>

#if defined(HPX_HAVE_NETWORKING)
#include <hpx/modules/errors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/logging.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/modules/plugin_factories.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/serialization.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace hpx::parcelset {

    /// The base of the device conduit implementations (CRTP, like
    /// parcelport_impl for parcelports): it implements the device_conduit
    /// interface and the transports the archives call, and forwards to the
    /// conduit without virtual calls. 'Derived' is constructible from the
    /// runtime configuration and provides
    ///
    ///  - static std::string name_impl()
    ///  - bool can_reach_impl(std::uint32_t locality_id) const
    ///  - void send_impl(std::uint32_t locality_id,
    ///        serialization::detail::device_chunk_descriptor const& d,
    ///        void const* p, std::size_t size,
    ///        hpx::move_only_function<void(std::exception_ptr)> sent)
    ///    starts sending the data, calls 'sent' once the memory may be
    ///    reused (the message's objects are kept alive until then), with
    ///    the error if the data couldn't be sent: the parcel has left
    ///    already, so the error is reported, and the send doesn't count in
    ///    the statistics
    ///  - void recv_impl(
    ///        serialization::detail::device_chunk_descriptor const& d,
    ///        void* p, std::size_t size,
    ///        std::function<void(std::exception_ptr)> done)
    ///    receives the data 'd' describes (d.source is the sending
    ///    locality), calls 'done' once it has arrived (with the error if it
    ///    failed: the parcel isn't run then)
    ///
    /// and optionally
    ///
    ///  - bool background_work_impl()
    ///    makes progress, called with the parcelports' background work
    ///  - static void config_defaults(std::vector<std::string>& ini)
    ///    adds the defaults of its configuration entries
    ///
    /// The descriptor of a chunk holds the sending locality and a tag unique
    /// for it. Conduits are registered with HPX_REGISTER_DEVICE_CONDUIT
    /// (compiled into HPX) or a static hpx::plugins::device_conduit_factory<>
    /// object (e.g. in the application).
    HPX_CXX_EXPORT template <typename Derived>
    class device_conduit_impl : public device_conduit
    {
    public:
        [[nodiscard]] std::string name() const override
        {
            return Derived::name_impl();
        }

        [[nodiscard]] bool can_reach(
            std::uint32_t const locality_id) const override
        {
            return derived().can_reach_impl(locality_id);
        }

        [[nodiscard]] std::shared_ptr<
            serialization::detail::device_chunk_transport>
        sender(std::uint32_t const locality_id,
            std::shared_ptr<void> keep_alive) override
        {
            return std::make_shared<sending_transport>(
                derived(), locality_id, HPX_MOVE(keep_alive));
        }

        [[nodiscard]] std::shared_ptr<
            serialization::detail::device_chunk_transport>
        receiver() override
        {
            return std::make_shared<receiving_transport>(derived());
        }

        [[nodiscard]] bool drain(
            std::chrono::steady_clock::duration const timeout) noexcept override
        {
            // the runtime is going away: errors of transfers that finish
            // now are logged, not reported (peers may have exited already)
            draining_.store(true, std::memory_order_release);

            auto const deadline = std::chrono::steady_clock::now() + timeout;
            while (in_flight_.load(std::memory_order_acquire) != 0)
            {
                if (std::chrono::steady_clock::now() >= deadline)
                {
                    return false;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            return true;
        }

        [[nodiscard]] device_conduit_statistics statistics() const override
        {
            device_conduit_statistics s;
            s.chunks_sent = chunks_sent_.load(std::memory_order_relaxed);
            s.bytes_sent = bytes_sent_.load(std::memory_order_relaxed);
            s.chunks_received =
                chunks_received_.load(std::memory_order_relaxed);
            s.bytes_received = bytes_received_.load(std::memory_order_relaxed);
            return s;
        }

        bool background_work() override
        {
            if constexpr (requires(Derived& d) { d.background_work_impl(); })
            {
                return derived().background_work_impl();
            }
            else
            {
                return false;
            }
        }

    private:
        Derived& derived() noexcept
        {
            return static_cast<Derived&>(*this);
        }
        Derived const& derived() const noexcept
        {
            return static_cast<Derived const&>(*this);
        }

        static device_conduit_impl& base(Derived& conduit) noexcept
        {
            return static_cast<device_conduit_impl&>(conduit);
        }

        // the transport of one message to 'locality_id'
        class sending_transport final
          : public serialization::detail::device_chunk_transport
        {
        public:
            sending_transport(Derived& conduit, std::uint32_t const locality_id,
                std::shared_ptr<void> keep_alive) noexcept
              : conduit_(conduit)
              , locality_id_(locality_id)
              , keep_alive_(HPX_MOVE(keep_alive))
            {
            }

            [[nodiscard]] serialization::detail::device_chunk_descriptor send(
                void const* p, std::size_t const size) override
            {
                device_conduit_impl& b = base(conduit_);
                serialization::detail::device_chunk_descriptor const d{
                    hpx::get_locality_id(),
                    b.next_tag_.fetch_add(1, std::memory_order_relaxed)};

                // the message's objects stay alive until the data is sent;
                // the transfer is counted until it finishes, once: through
                // its callback (which send_impl may call before it throws)
                // or because send_impl threw
                auto const once = std::make_shared<std::atomic<bool>>(false);
                b.in_flight_.fetch_add(1, std::memory_order_relaxed);
                try
                {
                    conduit_.send_impl(locality_id_, d, p, size,
                        [&b, size, once, keep_alive = keep_alive_](
                            std::exception_ptr error) mutable {
                            if (once->exchange(true))
                            {
                                return;
                            }
                            auto const finished =
                                hpx::experimental::scope_exit([&b] {
                                    b.in_flight_.fetch_sub(
                                        1, std::memory_order_release);
                                });

                            // the parcel referring to the data has left
                            keep_alive.reset();
                            if (error)
                            {
                                if (b.draining_.load(std::memory_order_acquire))
                                {
                                    LPT_(warning).format(
                                        "device conduit: a send failed during "
                                        "shutdown: {}",
                                        hpx::get_error_what(error));
                                }
                                else
                                {
                                    hpx::report_error(HPX_MOVE(error));
                                }
                            }
                            else
                            {
                                b.chunks_sent_.fetch_add(
                                    1, std::memory_order_relaxed);
                                b.bytes_sent_.fetch_add(
                                    size, std::memory_order_relaxed);
                            }
                        });
                }
                catch (...)
                {
                    if (!once->exchange(true))
                    {
                        b.in_flight_.fetch_sub(1, std::memory_order_release);
                    }
                    throw;
                }
                return d;
            }

            void recv(serialization::detail::device_chunk_descriptor const&,
                void*, std::size_t,
                std::function<void(std::exception_ptr)>) override
            {
                HPX_ASSERT_MSG(false, "a sending transport doesn't receive");
            }

        private:
            Derived& conduit_;
            std::uint32_t locality_id_;
            std::shared_ptr<void> keep_alive_;
        };

        // the transport of one received message
        class receiving_transport final
          : public serialization::detail::device_chunk_transport
        {
        public:
            explicit receiving_transport(Derived& conduit) noexcept
              : conduit_(conduit)
            {
            }

            [[nodiscard]] serialization::detail::device_chunk_descriptor send(
                void const*, std::size_t) override
            {
                HPX_ASSERT_MSG(false, "a receiving transport doesn't send");
                return {};
            }

            void recv(serialization::detail::device_chunk_descriptor const& d,
                void* p, std::size_t const size,
                std::function<void(std::exception_ptr)> done) override
            {
                device_conduit_impl& b = base(conduit_);
                // counted until it finishes, once (see sending_transport)
                auto const once = std::make_shared<std::atomic<bool>>(false);
                b.in_flight_.fetch_add(1, std::memory_order_relaxed);
                try
                {
                    conduit_.recv_impl(d, p, size,
                        [&b, size, once, done = HPX_MOVE(done)](
                            std::exception_ptr error) {
                            if (once->exchange(true))
                            {
                                return;
                            }
                            auto const finished =
                                hpx::experimental::scope_exit([&b] {
                                    b.in_flight_.fetch_sub(
                                        1, std::memory_order_release);
                                });
                            if (!error)
                            {
                                b.chunks_received_.fetch_add(
                                    1, std::memory_order_relaxed);
                                b.bytes_received_.fetch_add(
                                    size, std::memory_order_relaxed);
                            }
                            else if (b.draining_.load(
                                         std::memory_order_acquire))
                            {
                                // the runtime has gone: the message is
                                // dropped, its error can't be reported
                                LPT_(warning).format(
                                    "device conduit: a receive failed during "
                                    "shutdown: {}",
                                    hpx::get_error_what(error));
                                return;
                            }
                            done(HPX_MOVE(error));
                        });
                }
                catch (...)
                {
                    if (!once->exchange(true))
                    {
                        b.in_flight_.fetch_sub(1, std::memory_order_release);
                    }
                    throw;
                }
            }

        private:
            Derived& conduit_;
        };

        // tags of the chunks this locality sends, unique for it
        std::atomic<std::uint64_t> next_tag_ = 1;

        // set once drain() has started (at shutdown)
        std::atomic<bool> draining_ = false;

        // the sends and receives whose completion hasn't run yet
        std::atomic<std::size_t> in_flight_ = 0;

        std::atomic<std::uint64_t> chunks_sent_ = 0;
        std::atomic<std::uint64_t> bytes_sent_ = 0;
        std::atomic<std::uint64_t> chunks_received_ = 0;
        std::atomic<std::uint64_t> bytes_received_ = 0;
    };

    /// The conduit hpx.parcel.device_conduit names, nullptr for 'none' and
    /// before the parcel handler has created it (with the parcelports).
    HPX_CXX_EXPORT HPX_EXPORT device_conduit* get_device_conduit() noexcept;

    namespace detail {

        /// Creates the conduit hpx.parcel.device_conduit names (the parcel
        /// handler does, with the parcelports). Throws if there is no conduit
        /// of that name.
        HPX_CXX_EXPORT HPX_EXPORT void init_device_conduit(
            util::runtime_configuration const& cfg);

        /// Destroys the conduit (the parcel handler does, at shutdown).
        HPX_CXX_EXPORT HPX_EXPORT void reset_device_conduit() noexcept;

        /// The factories of the available conduits: the ones compiled into
        /// HPX and the ones registered by the application.
        HPX_CXX_EXPORT HPX_EXPORT
            std::vector<plugins::device_conduit_factory_base*>&
            get_device_conduit_factories();

        /// The archive flags to add to the parcelport's flags for a message
        /// to 'locality_id': enable_device_transport and enable_device_chunks
        /// if the conduit reaches the destination, none otherwise. The
        /// preprocessing pass of a parcel and its real serialization must use
        /// the same flags.
        HPX_CXX_EXPORT HPX_EXPORT std::uint32_t device_conduit_flags(
            std::uint32_t locality_id) noexcept;

        /// Attaches the sending transport for a message to 'locality_id' to
        /// 'archive' if its flags ask for one (see device_conduit_flags).
        HPX_CXX_EXPORT HPX_EXPORT void attach_device_conduit_sender(
            serialization::output_archive& archive, std::uint32_t locality_id,
            std::shared_ptr<void> keep_alive);

        /// Attaches the receiving transport to 'archive' if a conduit is
        /// selected.
        HPX_CXX_EXPORT HPX_EXPORT void attach_device_conduit_receiver(
            serialization::input_archive& archive);
    }    // namespace detail
}    // namespace hpx::parcelset

#endif
