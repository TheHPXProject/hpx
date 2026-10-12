//  Copyright (c) 2026 Christopher Taylor
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_OPENSHMEM)
#include <hpx/modules/command_line_handling.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/parcelset_base.hpp>
#include <hpx/modules/plugin.hpp>
#include <hpx/modules/resource_partitioner.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/util.hpp>
#include <hpx/openshmem_base/openshmem_environment.hpp>
#include <hpx/parcelport_openshmem/locality.hpp>
#include <hpx/parcelport_openshmem/mailbox_array.hpp>
#include <hpx/parcelport_openshmem/receiver.hpp>
#include <hpx/parcelport_openshmem/sender.hpp>
#include <hpx/parcelport_openshmem/sender_connection.hpp>
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

#include <shmem.h>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::parcelset {

    namespace policies::openshmem {
        class HPX_EXPORT parcelport;
    }    // namespace policies::openshmem

    template <>
    struct connection_handler_traits<policies::openshmem::parcelport>
    {
        using connection_type = policies::openshmem::sender_connection;
        using send_early_parcel = std::true_type;
        using do_background_work = std::true_type;
        using send_immediate_parcels = std::true_type;
        using is_connectionless = std::true_type;

        static constexpr char const* type() noexcept
        {
            return "openshmem";
        }

        static constexpr char const* pool_name() noexcept
        {
            return "parcel-pool-openshmem";
        }

        static constexpr char const* pool_name_postfix() noexcept
        {
            return "-openshmem";
        }
    };

    namespace policies::openshmem {

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
                    ini, "hpx.parcel.openshmem.mtu", HPX_PARCEL_OPENSHMEM_MTU);
            }

            // In-flight chunks per (sender,receiver) pair.  Must be
            // identical on every PE (the rx ring modulo is part of the wire
            // protocol), same caveat as mtu.
            static std::size_t slots(util::runtime_configuration const& ini)
            {
                std::size_t const slots = hpx::util::get_entry_as<std::size_t>(
                    ini, "hpx.parcel.openshmem.slots",
                    mailbox_array::default_slots_per_dst);
                return slots == 0 ? 1 : slots;
            }

            // Expected size (bytes) of the symmetric data segment the job was
            // launched with; used only to warn about large mappings at mailbox
            // construction time (an advisory, not a hard failure).
            static std::size_t symmetric_memory_size(
                util::runtime_configuration const& ini)
            {
                return hpx::util::get_entry_as<std::size_t>(ini,
                    "hpx.parcel.openshmem.symmetric_memory_size",
                    mailbox_array::default_symmetric_memory_size);
            }

            // Bound (seconds) for the empty wait in do_stop(). Shutdown must
            // terminate even if a peer stops returning credit for its last
            // in-flight transfers; when the timeout fires the few stranded
            // parcels are dropped.  The time is measured as *zero transfer
            // activity*: an empty that keeps moving pages waits as long as it
            // needs (no premature message drops), only a genuinely stalled
            // empty aborts.  Override with hpx.parcel.openshmem.stop_timeout.
            static std::size_t stop_timeout(util::runtime_configuration const& ini)
            {
                return hpx::util::get_entry_as<std::size_t>(
                    ini, "hpx.parcel.openshmem.stop_timeout", 30);
            }

            // checksum guard over received pages.  Off is only slightly faster
            // on checksum-heavy workloads and removes the corruption
            // tripwire, so leave it on unless profiling says otherwise.
            // Must be identical on every PE (sender and receiver must agree).
            static bool checksum(util::runtime_configuration const& ini)
            {
                return hpx::util::get_entry_as<bool>(
                    ini, "hpx.parcel.openshmem.checksum", true);
            }

            // Max source PEs probed per receive-scan pass.  0 = full scan
            // every pass.  Each probe issues a delivery-push (a remote
            // shmem_uint32_atomic_fetch) before reading the source's produced
            // counter, so this chiefly bounds the per-pass remote round-trip
            // cost at large n (see mailbox_array::default_probe_window).
            static std::size_t scan_window(
                util::runtime_configuration const& ini)
            {
                return hpx::util::get_entry_as<std::size_t>(
                    ini, "hpx.parcel.openshmem.scan_window",
                    mailbox_array::default_probe_window);
            }

            // Cross-PE wire-protocol uniformity check.  DISABLED by default
            // pending validation of the symmetric-memory rework.
            //
            // History: the check used to obtain its scratch space with
            // shmem_malloc() (a *collective* call) from inside do_run().  PE 0
            // blocked there waiting for a peer that never arrived, so the job
            // hung before main() with no locality output: do_run() is not
            // reached by all PEs in lockstep, because rank 0 runs in
            // runtime_mode::console and the others in runtime_mode::worker.
            //
            // It now publishes into the mailbox_array's own symmetric carve
            // (see mailbox_array::config_words()), which is allocated and
            // published behind a barrier that already completes on every PE.
            // It stays off by default until that path is confirmed on a real
            // oshrun job.  When enabling it, make sure every PE is launched
            // with identical hpx.parcel.openshmem.mtu, .slots and .checksum
            // values.
            static bool uniform_check(
                util::runtime_configuration const& ini)
            {
                return hpx::util::get_entry_as<bool>(
                    ini, "hpx.parcel.openshmem.uniform_check", false);
            }

        public:
            using sender_type = sender;
            parcelport(util::runtime_configuration const& ini,
                threads::policies::callback_notifier const& notifier)
              : base_type(ini, here(0), notifier)
              , ini_(ini)
              , stopped_(false)
              , num_pes_(static_cast<std::size_t>(shmem_n_pes()))
              , my_pe_(static_cast<std::size_t>(shmem_my_pe()))
              , mtu_(mtu(ini))
              , slots_(slots(ini))
              , stop_timeout_(stop_timeout(ini))
              , mailboxes_(num_pes_, my_pe_, mtu_, slots_,
                symmetric_memory_size(ini), checksum(ini),
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
            // fingerprint of the trio into its own word of a collective
            // scratch allocation, then reads every PE's fingerprint and aborts
            // loudly on any mismatch.  Called from do_run() while the transport
            // is still single-threaded and no parcel traffic exists.
            //
            // The publish is a plain local store into symmetric memory:
            // OpenSHMEM peers address a common address space directly, so no
            // flush is needed to make the word visible.  The shmem_fence()
            // orders that store before the following shmem_barrier_all(), and
            // because barriers order prior operations, the remote
            // shmem_uint32_atomic_fetch() reads below are guaranteed to observe
            // every completed publish.
void check_protocol_config_uniformity() const
            {
                std::uint32_t const hash =
                    (static_cast<std::uint32_t>(mtu_) * 31u +
                        static_cast<std::uint32_t>(slots_)) *
                        31u +
                    (mailboxes_.checksum_enabled() ? 1u : 0u);

                // The fingerprint row lives in the mailbox_array's own
                // symmetric carve rather than a fresh shmem_malloc(), which is
                // collective: do_run() is not reached by every PE in
                // lockstep (rank 0 runs in runtime_mode::console, the others in
                // runtime_mode::worker), so the extra collective deadlocked the
                // job before main().  The carve is identical on every PE and was
                // already published behind the constructor's barrier.
                std::uint32_t* const words = mailboxes_.config_words();
                if (!words)
                {
                    HPX_THROW_EXCEPTION(hpx::error::out_of_memory,
                        "openshmem::parcelport:"
                        "check_protocol_config_uniformity",
                        "mailbox_array config fingerprint row is null on PE " +
                            std::to_string(my_pe_));
                }

                // Each PE publishes its fingerprint at its own index.  Every
                // PE then REMOTE-fetches each peer's word: a plain local load
                // would read OUR OWN element, so the cross-PE comparison has
                // to be an actual remote atomic read.  The atomic also acts as
                // the delivery-push that makes the peer's counter visible to
                // this PE under UCX's OpenSHMEM shim.
                words[my_pe_] = hash;
                shmem_fence();
                shmem_barrier_all();

                bool mismatch = false;
                for (std::size_t pe = 0; pe < num_pes_; ++pe)
                {
                    std::uint32_t const other_hash =
                        (pe == my_pe_) ?
                            words[pe] :
                            shmem_uint32_atomic_fetch(
                                &words[pe], static_cast<int>(pe));
                    if (other_hash != hash)
                    {
                        mismatch = true;
                    }
                }

                if (mismatch)
                {
                    // All PEs converge on the same mismatch, so every PE
                    // aborts here — no single-PE hang.
                    std::cerr << "openshmem: PE " << my_pe_
                              << ": wire-protocol parameter mismatch across "
                                 "PEs (mtu="
                              << mtu_ << " slots=" << slots_ << " checksum="
                              << (mailboxes_.checksum_enabled() ? 1 : 0)
                              << " fingerprint=0x" << std::hex << hash
                              << std::dec << ").  This PE must be launched "
                                 "with the same hpx.parcel.openshmem.mtu, "
                                 "hpx.parcel.openshmem.slots and "
                                 "hpx.parcel.openshmem.checksum values as "
                                 "every other PE.\n";
                    std::abort();
                }
            }

            bool do_run()
            {
                sender_.run();
                receiver_.run();

                // Every PE must run the same wire-protocol parameters; a
                // mismatch corrupts transfers silently.  Disable only via
                // hpx.parcel.openshmem.uniform_check=0.
                if (uniform_check(ini_))
                {
                    check_protocol_config_uniformity();
                }

                // Unlike the GASNet-EX port (whose teams are always
                // thread-multiple), an OpenSHMEM runtime may only provide
                // SHMEM_THREAD_SERIALIZED, in which case concurrent drivers
                // would be illegal.  Clamp to a single driver in that case so
                // the spin loop below stays valid under either thread model.
                arena_count_ = io_service_pool_.size();
                if (!util::openshmem_environment::thread_multiple())
                {
                    if (arena_count_ > 1)
                    {
                        std::cerr << "openshmem: PE " << my_pe_
                                  << ": OpenSHMEM runtime provided only "
                                     "SHMEM_THREAD_SERIALIZED (not "
                                     "SHMEM_THREAD_MULTIPLE), so concurrent "
                                     "drivers would be illegal; limiting "
                                     "hpx.parcel.openshmem.io_pool_size from "
                                  << arena_count_ << " to 1\n";
                    }
                    arena_count_ = 1;
                }

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
                // then stop them.  This thread must NOT touch shmem_* while
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
                // empty -- no page movement for stop_timeout_ seconds -- aborts
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
                        std::cerr << "openshmem: PE " << my_pe_
                                  << ": do_stop: no transport activity for "
                                  << stop_timeout_
                                  << " s; dropping pending transfers and "
                                     "proceeding with shutdown\n";
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }

                if (util::openshmem_environment::thread_multiple())
                {
                    // SHMEM_THREAD_MULTIPLE: the drivers may still be
                    // pumping a stalled peer's straggler while we quiesce;
                    // the barrier (legal while other threads run
                    // non-collective shmem_* calls under this model) orders
                    // every PE's stores issued before it (data pages via
                    // putmem, produced/consumed credits via atomic_set).
                    // When it returns, no remote write is outstanding
                    // anywhere in the job, so no peer can write into our
                    // symmetric heap once we stop empty.  'stopped_' is
                    // set only below, so no driver is signalled to leave in
                    // the middle of the quiesce.
                    shmem_barrier_all();
                }

                // The drivers exit only after observing 'stopped_' below.
                // Under SHMEM_THREAD_SERIALIZED nothing below may touch
                // shmem_* until the last driver has left (io_service_work
                // decrements drivers_running_), so wait for them here.
                stopped_.store(true, std::memory_order_release);
                while (drivers_running_.load(std::memory_order_acquire) != 0)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }

                if (!util::openshmem_environment::thread_multiple())
                {
                    // SHMEM_THREAD_SERIALIZED: now exclusively
                    // single-threaded, so the final barrier is safe.  It
                    // guarantees every peer has observed our stores (and we
                    // theirs) before the transport is finalized by destroy().
                    shmem_barrier_all();
                }
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
            // drivers and (under SHMEM_THREAD_MULTIPLE) from any HPX thread that
            // runs the background-works scheduler callback.  'arena_idx' selects
            // the receive arena (a disjoint range of source PEs) so concurrent
            // scanners do not duplicate each other's probes.  Never blocks.
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

            // All shmem_* calls happen here (and in the io_service drivers).
            // Under SHMEM_THREAD_MULTIPLE any HPX thread may drive progress;
            // under SHMEM_THREAD_SERIALIZED only the single driver installed by
            // do_run() may, so this path stays disabled and all progress
            // happens on that driver.  The thread processes the send queue and
            // the receive arena selected by its OS-thread identifier
            // (num_thread modulo the io pool size).
            bool background_work(
                std::size_t num_thread, parcelport_background_mode mode)
            {
                if (stopped_.load(std::memory_order_acquire))
                {
                    return false;
                }
                if (!util::openshmem_environment::thread_multiple())
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
                // + credit, both written into our symmetric heap) is picked up
                // promptly.  Yielding starves delivery and the sender window
                // fills.  Each driver only scans its own arena.
                //
                // Track the number of running drivers so do_stop() can wait
                // for the last of them to exit: under SHMEM_THREAD_SERIALIZED
                // this thread must be the only one touching shmem_* once the
                // empty completes.  The try/catch guarantees the count never
                // leaks if progress() throws.
                try
                {
                    drivers_running_.fetch_add(1, std::memory_order_relaxed);
                    while (!stopped_.load(std::memory_order_acquire))
                    {
                        progress(arena_idx, parcelport_background_mode::all);
                    }
                }
                catch (...)
                {
                    drivers_running_.fetch_sub(1, std::memory_order_relaxed);
                    throw;
                }
                drivers_running_.fetch_sub(1, std::memory_order_relaxed);
            }

            util::runtime_configuration const& ini_;

            std::atomic<bool> stopped_;
            std::atomic<std::size_t> drivers_running_{0};

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
    }    // namespace policies::openshmem
}    // namespace hpx::parcelset

#include <hpx/config/warnings_suffix.hpp>

template <>
struct hpx::traits::plugin_config_data<
    hpx::parcelset::policies::openshmem::parcelport>
{
    static constexpr char const* priority() noexcept
    {
        return "100";
    }

    static void init(int* argc, char*** argv, util::command_line_handling& cfg)
    {
        util::openshmem_environment::init(argc, argv, cfg.rtcfg_);
        cfg.num_localities_ =
            static_cast<std::size_t>(util::openshmem_environment::size());
        cfg.node_ =
            static_cast<std::size_t>(util::openshmem_environment::rank());
    }

    static constexpr void init(hpx::resource::partitioner&) noexcept {}

    static void destroy() noexcept
    {
        util::openshmem_environment::finalize();
    }

    static constexpr char const* call() noexcept
    {
        // The io pool is sized freely under SHMEM_THREAD_MULTIPLE: every io
        // driver drives progress on its own arena (a disjoint range of source
        // PEs).  The value below is only a default; do_run() clamps it to 1
        // when the OpenSHMEM runtime provides only SHMEM_THREAD_SERIALIZED.
        return "mtu = "
               "${HPX_HAVE_PARCELPORT_OPENSHMEM_MTU:65536}\n"
               // In-flight chunks per (sender,receiver) pair; raises
               // symmetric tx/rx memory linearly, so keep it small for very
               // large runs and increase it for high-latency links.
               "slots = ${HPX_HAVE_PARCELPORT_OPENSHMEM_SLOTS:8}\n"
               // Expected symmetric data segment size (bytes; see
               // mailbox_array::default_symmetric_memory_size).
               "symmetric_memory_size = 1073741824\n"
               // Seconds of zero page activity at shutdown before the empty wait
               // aborts and remaining in-flight transfers are dropped.  An
               // empty that keeps transferring pages waits as long as it needs;
               // only a genuine stall terminates early.
               "stop_timeout = 30\n"
               // CRC32 guard over received pages.  Leave on unless profiling
               // shows the pass; setting it to 0 removes the corruption
               // tripwire and must be identical on every PE.
               "checksum = 1\n"
// Cross-PE wire-protocol uniformity check at startup.  Off by
                // default until the symmetric-fingerprint rework is validated
                // on a real oshrun job: it now reuses the mailbox_array carve
                // (no collective shmem_malloc in do_run()), but the check still
                // costs one barrier on every launch.  Enable with
                // uniform_check=1 when launching a homogeneous job.
                "uniform_check = 0\n"
               // Max source PEs probed per receive-scan pass (0 = all).
               // Bounds the per-pass remote round-trip cost at large n.
               "scan_window = 8\n"
               // Number of io_service threads driving progress.  The base
               // parcelport reads hpx.parcel.<pp>.io_pool_size (default 2).
               "io_pool_size = 1\n";
    }
};

HPX_REGISTER_PARCELPORT(
    hpx::parcelset::policies::openshmem::parcelport, openshmem)

#endif
