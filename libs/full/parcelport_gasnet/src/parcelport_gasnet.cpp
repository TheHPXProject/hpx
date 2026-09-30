//  Copyright (c) 2023-2026 Christopher Taylor
//  Copyright (c) 2019-2021 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_GASNET)
#include <hpx/modules/command_line_handling.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/gasnet_base.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/modules/plugin.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/util.hpp>
#include <hpx/parcelport_gasnet/locality.hpp>
#include <hpx/parcelport_gasnet/mailbox_array.hpp>
#include <hpx/parcelport_gasnet/receiver.hpp>
#include <hpx/parcelport_gasnet/sender.hpp>
#include <hpx/parcelport_gasnet/sender_connection.hpp>
#include <hpx/parcelset/parcelport_impl.hpp>
#include <hpx/plugin_factories/parcelport_factory.hpp>

#include <asio/io_context.hpp>
#include <asio/post.hpp>
#include <asio/version.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::parcelset {

    namespace policies::gasnet {
        class HPX_EXPORT parcelport;
    }    // namespace policies::gasnet

    template <>
    struct connection_handler_traits<policies::gasnet::parcelport>
    {
        using connection_type = policies::gasnet::sender_connection;
        using send_early_parcel = std::true_type;
        using do_background_work = std::true_type;
        using send_immediate_parcels = std::true_type;
        using is_connectionless = std::true_type;

        static constexpr char const* type() noexcept
        {
            return "gasnet";
        }

        static constexpr char const* pool_name() noexcept
        {
            return "parcel-pool-gasnet";
        }

        static constexpr char const* pool_name_postfix() noexcept
        {
            return "-gasnet";
        }
    };

    namespace policies::gasnet {

        class HPX_EXPORT parcelport : public parcelport_impl<parcelport>
        {
            using base_type = parcelport_impl<parcelport>;

            static parcelset::locality here(std::size_t my_pe)
            {
                return parcelset::locality(
                    locality(static_cast<std::int32_t>(my_pe)));
            }

            static std::size_t mtu(util::runtime_configuration const& ini)
            {
                return hpx::util::get_entry_as<std::size_t>(
                    ini, "hpx.parcel.gasnet.mtu", HPX_PARCEL_GASNET_MTU);
            }

            // In-flight chunks per (sender,receiver) pair.  Must be
            // identical on every PE (the rx ring modulo is part of the wire
            // protocol), same caveat as mtu.
            static std::size_t slots(util::runtime_configuration const& ini)
            {
                std::size_t const slots = hpx::util::get_entry_as<std::size_t>(
                    ini, "hpx.parcel.gasnet.slots",
                    mailbox_array::default_slots_per_dst);
                return slots == 0 ? 1 : slots;
            }

            // Bound (seconds) for the empty wait in do_stop(). Shutdown 
            // terminates even if a peer stops returning credit for its last
            // in-flight transfers; when the timeout fires the few stranded
            // parcels are dropped.  The time is measured as *zero transfer
            // activity*: an empty that keeps moving pages waits as long as it
            // needs (no premature message drops), only a genuinely stalled
            // empty aborts. override with hpx.parcel.gasnet.stop_timeout.
            static std::size_t stop_timeout(util::runtime_configuration const& ini)
            {
                return hpx::util::get_entry_as<std::size_t>(
                    ini, "hpx.parcel.gasnet.stop_timeout", 30);
            }

            // checksum guard over received pages. Off is only slightly faster
            // on checksum-heavy workloads and removes the corruption
            // tripwire, so leave it on unless profiling says otherwise.
            // Must be identical on every PE (sender and receiver must agree).
            static bool checksum(util::runtime_configuration const& ini)
            {
                return hpx::util::get_entry_as<bool>(
                    ini, "hpx.parcel.gasnet.checksum", true);
            }

            // Max source PEs probed per receive-scan pass.  0 = full scan
            // every pass.  Each probe is one local load in the GASNet port,
            // so this chiefly bounds the per-pass loop length at large n (see
            // mailbox_array::default_probe_window).
            static std::size_t scan_window(util::runtime_configuration const& ini)
            {
                return hpx::util::get_entry_as<std::size_t>(
                    ini, "hpx.parcel.gasnet.scan_window",
                    mailbox_array::default_probe_window);
            }

            // Cross-PE wire-protocol uniformity check.  ENABLED by default: a
            // mismatch in mtu/slots/checksum silently corrupts transfers, so
            // the affordable startup-time collective runs unless explicitly
            // turned off via hpx.parcel.gasnet.uniform_check=0.
            static bool uniform_check(
                util::runtime_configuration const& ini)
            {
                return hpx::util::get_entry_as<bool>(
                    ini, "hpx.parcel.gasnet.uniform_check", true);
            }

        public:
            using sender_type = sender;
            parcelport(util::runtime_configuration const& ini,
                threads::policies::callback_notifier const& notifier)
              : base_type(ini, here(0), notifier)
              , ini_(ini)
              , stopped_(false)
              , num_pes_(static_cast<std::size_t>(
                    util::gasnet_environment::size()))
              , my_pe_(static_cast<std::size_t>(
                    util::gasnet_environment::rank()))
              , mtu_(mtu(ini))
              , slots_(slots(ini))
              , stop_timeout_(stop_timeout(ini))
              , mailboxes_(num_pes_, my_pe_, mtu_, slots_, checksum(ini),
                    scan_window(ini))
              , sender_(&mailboxes_)
              , receiver_(*this)
            {
                here_ = here(my_pe_);
            }

            parcelport(parcelport const&) = delete;
            parcelport(parcelport&&) = delete;
            parcelport& operator=(parcelport const&) = delete;
            parcelport& operator=(parcelport&&) = delete;

            ~parcelport() override = default;

            // Every PE must run the same wire-protocol parameters (mtu, slots,
            // checksum): the rx-ring modulo, the page geometry and the
            // checksum agreement are all part of the wire format, so a
            // mismatch corrupts transfers silently.  Each PE publishes a
            // fingerprint of the trio into its own word of the collective
            // scratch region carved out of the registered segments, then reads
            // every PE's fingerprint and aborts loudly on any mismatch.
            // Called from do_run() while the transport is still
            // single-threaded and no parcel traffic exists.
            //
            // The publish is routed through GASNet as a self-RMA put rather
            // than a plain local store: GASNet's memory model only guarantees
            // that an RMA read of a peer's segment observes the peer's writes
            // which that peer has flushed to the fabric, and an actual RMA
            // operation is the unambiguous way to make GASNet flush it.  The
            // subsequent team barrier then synchronizes every PE's publish
            // (barriers order RMA operations, this one included), so the
            // remote gets below are guaranteed to observe every completed
            // publish.
            void check_protocol_config_uniformity() const
            {
                std::uint32_t const hash =
                    (static_cast<std::uint32_t>(mtu_) * 31u +
                        static_cast<std::uint32_t>(slots_)) *
                        31u +
                    (mailboxes_.checksum_enabled() ? 1u : 0u);

                auto* const words = reinterpret_cast<std::uint32_t*>(
                    mailboxes_.scratch_beg());

                // Publish my fingerprint into my own scratch word via GASNet
                // (self-RMA).  mailbox_array's constructor zeroed the whole
                // carve, so before this put the word reads as 0 on every peer.
                void* const my_word =
                    mailboxes_.remote_ptr(my_pe_, &words[my_pe_]);
                util::gasnet_environment::put_uint32(
                    static_cast<int>(my_pe_), my_word, hash);

                // All publishes are now RMA operations; this barrier makes
                // each one performed at its target before any peer returns.
                util::gasnet_environment::barrier();

                bool mismatch = false;
                for (std::size_t pe = 0; pe < num_pes_; ++pe)
                {
                    std::uint32_t other_hash = 0;
                    if (pe == my_pe_)
                    {
                        // Own word: the blocking self-put above implies local
                        // completion, so a plain load sees it.
                        other_hash = words[pe];
                    }
                    else
                    {
                        // Remote fetch of peer pe's fingerprint.  A plain
                        // local load of words[pe] would read OUR OWN element
                        // (there is no symmetric shared memory, only RMA), so
                        // the cross-PE comparison has to be an actual get.
                        void* const remote_word =
                            mailboxes_.remote_ptr(pe, &words[pe]);
                        util::gasnet_environment::get(
                            static_cast<int>(pe), &other_hash, remote_word,
                            sizeof(other_hash));
                    }
                    if (other_hash != hash)
                    {
                        mismatch = true;
                    }
                }

                if (mismatch)
                {
                    // All PEs converge on the same mismatch, so every PE
                    // aborts here — no single-PE hang.
                    std::cerr << "gasnet: PE " << my_pe_
                              << ": wire-protocol parameter mismatch across "
                                 "PEs (mtu="
                              << mtu_ << " slots=" << slots_ << " checksum="
                              << (mailboxes_.checksum_enabled() ? 1 : 0)
                              << " fingerprint=0x" << std::hex << hash
                              << std::dec << ").  This PE must be launched "
                                 "with the same hpx.parcel.gasnet.mtu, "
                                 "hpx.parcel.gasnet.slots and "
                                 "hpx.parcel.gasnet.checksum values as "
                                 "every other PE.\n";
                    std::abort();
                }
            }

            bool do_run()
            {
                sender_.run();
                receiver_.run();

                // Collective rendezvous: every PE must have attached its
                // registered segment, built the remote-segment base table
                // (gasnet_environment::init) and zeroed its mailbox carve
                // (mailbox_array's constructor) before any parcel traffic is
                // allowed to start.  C++ construction is not collective in
                // GASNet, so this barrier replaces the shmem_barrier_all()
                // the openshmem counterpart placed in its constructor.
                util::gasnet_environment::barrier();

                // Every PE must run the same wire-protocol parameters; a
                // mismatch corrupts transfers silently.  Disable only via
                // hpx.parcel.gasnet.uniform_check=0 (not recommended).
                if (uniform_check(ini_))
                {
                    check_protocol_config_uniformity();
                }

                // GASNet-EX teams are always thread-multiple: any number of
                // threads may call gex_RMA_* concurrently on the same team,
                // so each io_service drives its own non-blocking progress
                // loop and scans only its arena (a disjoint range of source
                // PEs).  No clamping to a single driver is needed (unlike the
                // OpenSHMEM port under SHMEM_THREAD_SERIALIZED).
                arena_count_ = io_service_pool_.size();

                for (std::size_t i = 0; i < arena_count_; ++i)
                {
#if ASIO_VERSION >= 103400
                    ::asio::post(
                        io_service_pool_.get_io_service(static_cast<int>(i)),
                        hpx::bind(&parcelport::io_service_work, this, i));
#else
                    io_service_pool_.get_io_service(static_cast<int>(i))
                        .post(hpx::bind(&parcelport::io_service_work, this, i));
#endif
                }
                return true;
            }

            void do_stop()
            {
                // Wait for the io_service drivers to empty all queued work,
                // then stop them.  This thread must NOT touch GASNet while
                // the drivers may still be running (they exit only once
                // 'stopped_' is set below), so it waits passively instead of
                // driving progress itself.  The wait deliberately uses no HPX
                // thread suspension: nothing wakes a suspended thread, and
                // the loop must not depend on the scheduler.
                //
                // The abort condition is *stall*, not a wall-clock budget: an
                // empty that keeps transferring pages is allowed to take as
                // long as it needs (otherwise a legitimately slow-but-moving
                // empty would drop messages), and only a genuinely stalled
                // empty — no page movement for stop_timeout_ seconds — aborts
                // so shutdown still terminates when a peer never returns
                // credit (or never publishes) again.  Transfer activity is
                // measured via mailbox_array::transfer_count(), which changes
                // exactly as often as pages move.
                auto last_change = std::chrono::steady_clock::now();
                std::uint64_t last_count = mailboxes_.transfer_count();

                while (sender_.has_pending() || receiver_.has_pending())
                {
                    std::uint64_t const count = mailboxes_.transfer_count();
                    if (count != last_count)
                    {
                        last_count = count;
                        last_change = std::chrono::steady_clock::now();
                    }
                    else if (std::chrono::steady_clock::now() - last_change >=
                        std::chrono::seconds(stop_timeout_))
                    {
                        std::cerr << "gasnet: PE " << my_pe_
                                  << ": do_stop: no transport activity for "
                                  << stop_timeout_
                                  << " s; dropping pending transfers and "
                                     "proceeding with shutdown\n";
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }

                stopped_.store(true, std::memory_order_release);
            }

            std::string get_locality_name() const override
            {
                return std::to_string(my_pe_);
            }

            std::shared_ptr<sender_connection> create_connection(
                parcelset::locality const& l, error_code&)
            {
                int const dest_rank = l.get<locality>().rank();
                return sender_.create_connection(dest_rank, &mailboxes_);
            }

            parcelset::locality agas_locality(
                util::runtime_configuration const&) const override
            {
                return parcelset::locality(locality(0));
            }

            parcelset::locality create_locality() const override
            {
                return parcelset::locality(locality());
            }

            // Drive one step of transport progress.  Called from the io_service
            // drivers and from any HPX thread that runs the background-works
            // scheduler callback (GASNet-EX is thread-multiple, so concurrent
            // drivers are always legal).  'arena_idx' selects the receive
            // arena (a disjoint range of source PEs) so concurrent scanners
            // do not duplicate each other's probes.  Never blocks.
            bool progress(
                std::size_t arena_idx, parcelport_background_mode mode)
            {
                bool has_work = false;
                if (mode & parcelport_background_mode::send)
                {
                    has_work = sender_.background_work();
                }
                if (mode & parcelport_background_mode::receive)
                {
                    has_work =
                        receiver_.background_work(arena_idx, arena_count_) ||
                        has_work;
                }
                return has_work;
            }

            // All GASNet calls happen here (and in the io_service drivers).
            // Every HPX thread may drive progress: GASNet-EX teams are
            // thread-multiple, so there is no serialization gate (unlike the
            // OpenSHMEM port).  The thread processes the send queue and the
            // receive arena selected by its OS-thread identifier (num_thread
            // modulo the io pool size).
            bool background_work(
                std::size_t num_thread, parcelport_background_mode mode)
            {
                if (stopped_.load(std::memory_order_acquire))
                {
                    return false;
                }

                std::size_t const arena_idx =
                    arena_count_ == 0 ? 0 : num_thread % arena_count_;
                return progress(arena_idx, mode);
            }

            constexpr bool can_send_immediate() const noexcept
            {
                return true;
            }

            mailbox_array const& get_mailboxes() const noexcept
            {
                return mailboxes_;
            }

            mailbox_array& get_mailboxes() noexcept
            {
                return mailboxes_;
            }

            constexpr std::size_t num_pes() const noexcept
            {
                return num_pes_;
            }

            constexpr std::size_t my_pe() const noexcept
            {
                return my_pe_;
            }

            constexpr std::size_t mtu() const noexcept
            {
                return mtu_;
            }

            bool send_immediate(parcelset::parcelport* pp,
                parcelset::locality const& dest,
                sender::parcel_buffer_type buffer,
                sender::callback_fn_type&& callbackFn)
            {
                (void) pp;
                return sender_.send_immediate(
                    dest, HPX_MOVE(buffer), HPX_MOVE(callbackFn));
            }

        private:
            void io_service_work(std::size_t arena_idx)
            {
                // Deliberate hot spin (no OS yield).  The receiver arena scan
                // continuously pumps the transport so a published page (data
                // + credit, both RMA-written into our registered segment) is
                // picked up promptly.  Yielding starves delivery and the
                // sender window fills.  Each driver only scans its own arena.
                while (!stopped_.load(std::memory_order_acquire))
                {
                    progress(arena_idx, parcelport_background_mode::all);
                }
            }

            util::runtime_configuration const& ini_;

            std::atomic<bool> stopped_;

            std::size_t num_pes_;
            std::size_t my_pe_;
            std::size_t mtu_;
            std::size_t slots_;
            std::size_t stop_timeout_ = 30;
            std::size_t arena_count_ = 1;
            mailbox_array mailboxes_;

            sender sender_;
            receiver<parcelport> receiver_;
        };
    }    // namespace policies::gasnet
}    // namespace hpx::parcelset

#include <hpx/config/warnings_suffix.hpp>

template <>
struct hpx::traits::plugin_config_data<
    hpx::parcelset::policies::gasnet::parcelport>
{
    static constexpr char const* priority() noexcept
    {
        return "100";
    }

    static void init(int* argc, char*** argv, util::command_line_handling& cfg)
    {
        util::gasnet_environment::init(argc, argv, cfg.rtcfg_);
        cfg.num_localities_ =
            static_cast<std::size_t>(util::gasnet_environment::size());
        cfg.node_ = static_cast<std::size_t>(util::gasnet_environment::rank());
    }

    static constexpr void init(hpx::resource::partitioner&) noexcept {}

    static void destroy() noexcept
    {
        util::gasnet_environment::finalize();
    }

    static constexpr char const* call() noexcept
    {
        // The io pool is sized freely: GASNet-EX teams are always
        // thread-multiple, so every io driver drives progress on its own
        // arena (a disjoint range of source PEs) and the base parcelport
        // default io_pool_size applies (no clamp to one driver is needed).
        return "mtu = "
               "${HPX_HAVE_PARCELPORT_GASNET_MTU:65536}\n"
               // In-flight chunks per (sender,receiver) pair; raises
               // registered tx/rx memory linearly, so keep it small for
               // very large runs and increase it for high-latency links.
               "slots = ${HPX_HAVE_PARCELPORT_GASNET_SLOTS:8}\n"
               // Seconds of zero page activity at shutdown before the empty
               // wait aborts and remaining in-flight transfers are dropped.
               // An empty that keeps transferring pages waits as long as it
               // needs; only a genuine stall terminates early.
               "stop_timeout = 30\n"
               // CRC32 guard over received pages.  Leave on unless profiling
               // shows the pass; setting it to 0 removes the corruption
               // tripwire and must be identical on every PE.
               "checksum = 1\n"
               // Cross-PE wire-protocol uniformity check at startup, enforced by
               // default: identical mtu/slots/checksum on every PE are part
               // of the wire format, and a mismatch corrupts transfers
               // silently (aborts on mismatch).  Set to 0 only to skip it.
               "uniform_check = 1\n"
               // Max source PEs probed per receive-scan pass (0 = all).
               // Each probe is a single local load in the GASNet port; the
               // window bounds the per-pass loop length at large n.
               "scan_window = 8\n";
    }
};

HPX_REGISTER_PARCELPORT(
    hpx::parcelset::policies::gasnet::parcelport, gasnet)

#endif
