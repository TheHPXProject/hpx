//  Copyright (c) 2026 Christopher Taylor
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_OPENSHMEM)
#include <hpx/assert.hpp>
#include <hpx/modules/errors.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#if defined(__x86_64__)
#include <immintrin.h>
#endif
#include <chrono>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>

#include <shmem.h>

namespace hpx::parcelset::policies::openshmem {

    // TX/RX page-arena + per-pair 64-bit credit-word protocol
    //
    namespace detail {

        // Wire header of a chunk, shared by sender_connection (writer) and
        // receiver_connection (reader). Each page carries the full header.
        struct message_header
        {
            std::uint64_t size;
            std::uint64_t data_size;
            std::uint32_t num_chunks;
            std::uint32_t chunk_index;
            std::uint32_t total_size_low;
            std::uint32_t total_size_high;
            std::uint64_t message_id;
            std::uint32_t checksum;
        };

        static_assert(sizeof(message_header) % 8 == 0,
            "message_header must be 8-byte aligned");

        constexpr std::size_t header_size = sizeof(message_header);

        // Bounds a corrupt num_chunks field so downstream chunk-vector
        // resizes stay sane during validation.
        constexpr std::size_t max_message_chunks = 1u << 20;

        // checksum over the payload bytes only.
        constexpr std::array<std::uint32_t, 256> make_crc32_table()
        {
            std::array<std::uint32_t, 256> table{};
            for (std::size_t i = 0; i < table.size(); ++i)
            {
                std::uint32_t c = static_cast<std::uint32_t>(i);
                for (int k = 0; k < 8; ++k)
                {
                    c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
                }
                table[i] = c;
            }
            return table;
        }

        // CRC-32C (Castagnoli, reflected polynomial 0x82F63B78) page checksum.
        // The checksum covers every transferred byte on both sender and
        // receiver, so on a hub rank it was the dominant per-byte cost.  The
        // x86 CRC32 instruction computes exactly this polynomial at several
        // GB/s; a slicing-by-8 table version gives identical values on CPUs
        // without it (and is used by the hardware path for the tail).
        constexpr std::array<std::array<std::uint32_t, 256>, 8>
        make_crc32c_tables()
        {
            std::array<std::array<std::uint32_t, 256>, 8> t{};
            for (std::size_t i = 0; i < 256; ++i)
            {
                std::uint32_t c = static_cast<std::uint32_t>(i);
                for (int k = 0; k < 8; ++k)
                {
                    c = (c & 1u) ? (0x82F63B78u ^ (c >> 1)) : (c >> 1);
                }
                t[0][i] = c;
            }
            for (std::size_t k = 1; k < 8; ++k)
            {
                for (std::size_t i = 0; i < 256; ++i)
                {
                    std::uint32_t const prev = t[k - 1][i];
                    t[k][i] = (prev >> 8) ^ t[0][prev & 0xFFu];
                }
            }
            return t;
        }

        inline std::uint32_t crc32c_sw(std::uint32_t crc,
            unsigned char const* data, std::size_t size) noexcept
        {
            static constexpr auto t = make_crc32c_tables();
            if constexpr (std::endian::native == std::endian::little)
            {
                while (size >= 8)
                {
                    std::uint32_t lo;
                    std::uint32_t hi;
                    std::memcpy(&lo, data, 4);
                    std::memcpy(&hi, data + 4, 4);
                    lo ^= crc;
                    crc = t[7][lo & 0xFFu] ^ t[6][(lo >> 8) & 0xFFu] ^
                        t[5][(lo >> 16) & 0xFFu] ^ t[4][lo >> 24] ^
                        t[3][hi & 0xFFu] ^ t[2][(hi >> 8) & 0xFFu] ^
                        t[1][(hi >> 16) & 0xFFu] ^ t[0][hi >> 24];
                    data += 8;
                    size -= 8;
                }
            }

            for (std::size_t i = 0; i < size; ++i)
            {
               crc = t[0][(crc ^ data[i]) & 0xFFu] ^ (crc >> 8);
            }
            return crc;
        }

        // One 8-byte step of CRC-32C (the same update the x86 CRC32
        // instruction performs), table version.
        inline std::uint32_t crc32c_word_sw(
            std::uint32_t crc, std::uint64_t w) noexcept
        {
            static constexpr auto t = make_crc32c_tables();
            std::uint32_t lo = static_cast<std::uint32_t>(w) ^ crc;
            std::uint32_t hi = static_cast<std::uint32_t>(w >> 32);
            return t[7][lo & 0xFFu] ^ t[6][(lo >> 8) & 0xFFu] ^
                t[5][(lo >> 16) & 0xFFu] ^ t[4][lo >> 24] ^ t[3][hi & 0xFFu] ^
                t[2][(hi >> 8) & 0xFFu] ^ t[1][(hi >> 16) & 0xFFu] ^
                t[0][hi >> 24];
        }

        constexpr std::uint32_t rotl32(std::uint32_t v, int r) noexcept
        {
            return (v << r) | (v >> (32 - r));
        }

        // Page checksum.  Three independent CRC-32C streams over every third
        // 8-byte word (so the CRC instruction's 3-cycle latency is hidden and
        // it runs at ~1 word/cycle), combined as a ^ rotl(b, 11) ^ rotl(c, 22);
        // trailing words/bytes feed stream a.  This is an integrity tripwire,
        // not the standard CRC-32C of the byte string; sender and receiver
        // compute exactly the same function on every CPU (the table path
        // below gives bit-identical results to the instruction path).
        template <bool Hw, typename Step>
        inline std::uint32_t crc_pages_impl(unsigned char const* data,
            std::size_t size, Step step, std::uint32_t (*byte_step)(
                                              std::uint32_t, unsigned char))
        {
            std::uint32_t a = 0xFFFFFFFFu, b = 0xFFFFFFFFu, c = 0xFFFFFFFFu;
            while (size >= 24)
            {
                std::uint64_t wa, wb, wc;
                std::memcpy(&wa, data, 8);
                std::memcpy(&wb, data + 8, 8);
                std::memcpy(&wc, data + 16, 8);
                a = step(a, wa);
                b = step(b, wb);
                c = step(c, wc);
                data += 24;
                size -= 24;
            }
            while (size >= 8)
            {
                std::uint64_t w;
                std::memcpy(&w, data, 8);
                a = step(a, w);
                data += 8;
                size -= 8;
            }
            while (size > 0)
            {
                a = byte_step(a, *data);
                ++data;
                --size;
            }
            return (a ^ rotl32(b, 11) ^ rotl32(c, 22)) ^ 0xFFFFFFFFu;
        }

        inline std::uint32_t crc32c_byte_sw(
            std::uint32_t crc, unsigned char byte) noexcept
        {
            static constexpr auto t = make_crc32c_tables();
            return t[0][(crc ^ byte) & 0xFFu] ^ (crc >> 8);
        }

        inline std::uint32_t page_checksum_sw(
            unsigned char const* data, std::size_t size) noexcept
        {
            return crc_pages_impl<false>(
                data, size,
                [](std::uint32_t crc, std::uint64_t w) {
                    return crc32c_word_sw(crc, w);
                },
                &crc32c_byte_sw);
        }

#if defined(__x86_64__) && defined(__GNUC__)
        __attribute__((target("sse4.2"))) inline std::uint32_t page_checksum_hw(
            unsigned char const* data, std::size_t size) noexcept
        {
            std::uint64_t a = 0xFFFFFFFFu, b = 0xFFFFFFFFu, c = 0xFFFFFFFFu;
            while (size >= 24)
            {
                std::uint64_t wa, wb, wc;
                std::memcpy(&wa, data, 8);
                std::memcpy(&wb, data + 8, 8);
                std::memcpy(&wc, data + 16, 8);
                a = _mm_crc32_u64(a, wa);
                b = _mm_crc32_u64(b, wb);
                c = _mm_crc32_u64(c, wc);
                data += 24;
                size -= 24;
            }
            while (size >= 8)
            {
                std::uint64_t w;
                std::memcpy(&w, data, 8);
                a = _mm_crc32_u64(a, w);
                data += 8;
                size -= 8;
            }
            std::uint32_t a32 = static_cast<std::uint32_t>(a);
            while (size > 0)
            {
                a32 = _mm_crc32_u8(a32, *data);
                ++data;
                --size;
            }

            return (a32 ^ rotl32(static_cast<std::uint32_t>(b), 11) ^
                       rotl32(static_cast<std::uint32_t>(c), 22)) ^
                0xFFFFFFFFu;
        }
#endif

        inline std::uint32_t crc32(
            unsigned char const* data, std::size_t size) noexcept
        {
#if defined(__x86_64__) && defined(__GNUC__)
            static bool const has_hw = __builtin_cpu_supports("sse4.2");
            if (has_hw)
            {
                return page_checksum_hw(data, size);
            }
#endif
            return page_checksum_sw(data, size);
        }

        inline std::uint64_t message_total_size(message_header const& h) noexcept
        {
            return (std::uint64_t(h.total_size_high) << 32) | h.total_size_low;
        }

        // Payload size (bytes) of the chunk at 'chunk_index' of a message of
        // 'total_size' bytes split into 'num_chunks' chunks of at most
        inline std::size_t chunk_size_for(std::uint64_t chunk_index,
            std::uint64_t num_chunks, std::uint64_t total_size,
            std::size_t payload_size) noexcept
        {
            std::uint64_t const offset = chunk_index * payload_size;
            return static_cast<std::size_t>((chunk_index == num_chunks - 1) ?
                    (total_size - offset) :
                    payload_size);
        }

        // Structural sanity of one page header.
        inline bool message_header_geometry_valid(
            message_header const& h, std::size_t mtu) noexcept
        {
            if (header_size >= mtu)
                return false;
            std::size_t const payload_size = mtu - header_size;
            if (payload_size == 0)
                return false;

            if (h.message_id == 0)
                return false;
            if (h.num_chunks == 0 || h.num_chunks > max_message_chunks)
                return false;
            if (h.chunk_index >= h.num_chunks)
                return false;

            std::uint64_t const total_size = message_total_size(h);
            if (h.size != total_size || h.data_size != total_size)
                return false;
            if (total_size > max_message_chunks * (std::uint64_t) payload_size)
                return false;

            std::uint64_t const offset =
                std::uint64_t(h.chunk_index) * payload_size;
            if (offset > total_size)
                return false;

            if (chunk_size_for(
                    h.chunk_index, h.num_chunks, total_size, payload_size) >
                payload_size)
                return false;

            std::uint64_t const expected_chunks =
                (total_size + payload_size - 1) / payload_size;
            if (h.num_chunks != (total_size == 0 ? 1 : expected_chunks))
                return false;

            return true;
        }

        // Extract + fully validate a received page (geometry + payload
        // checksum).
        inline bool validate_message_page(unsigned char const* page,
            std::size_t mtu, std::size_t& chunk_size,
            bool use_checksum = true) noexcept
        {
            message_header header;
            std::memcpy(&header, page, sizeof(header));

            if (!message_header_geometry_valid(header, mtu))
                return false;

            std::size_t const payload_size = mtu - header_size;
            std::uint64_t const total_size = message_total_size(header);
            chunk_size = chunk_size_for(header.chunk_index, header.num_chunks,
                total_size, payload_size);

            return !use_checksum || header.checksum ==
                crc32(page + header_size, chunk_size);
        }
    }    // namespace detail

    namespace detail {
        // Optional io-thread accounting (hpx.parcel.openshmem.stats=1), printed
        // when the parcelport stops.  Diagnostics only.
        struct io_stats
        {
            std::atomic<std::uint64_t> send_ns{0}, recv_ns{0}, decode_ns{0},
                stage_ns{0}, put_ns{0}, pages_sent{0}, pages_recv{0},
                nocredit{0}, send_calls{0}, send_work{0}, recv_calls{0},
                recv_work{0}, bytes_sent{0}, prog_ns{0}, crc_ns{0}, copy_ns{0},
                rel_ns{0}, resize_ns{0}, peek_ns{0};
        };
        inline io_stats g_io_stats;
        inline std::atomic<bool> g_io_stats_on{false};

        // Set by parcelport::do_stop(): by then the HPX scheduler and AGAS
        // are gone, so a parcel that is still completing on an io driver can
        // no longer be decoded or dispatched (doing so dereferences the
        // destroyed AGAS).  The receiver keeps draining pages (so credits
        // keep flowing and peers can finish) but discards the messages.
        inline std::atomic<bool> g_discard_received{false};
        inline std::uint64_t ns_now() noexcept
        {
            return static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                    .count());
        }
    }    // namespace detail

    class HPX_EXPORT mailbox_array
    {
    public:
        // Default number of in-flight pages (chunks) allowed per
        // (sender,receiver) pair before the sender must wait for consumed slots
        static constexpr std::size_t default_slots_per_dst = 8;

        // Max number of source PEs probed per receive-scan pass. The scan
        // window slides over the arena (one source per call)
        static constexpr std::size_t default_probe_window = 8;

        // Expected size (bytes) of the OpenSHMEM symmetric data segment the
        // job was launched with.
        static constexpr std::size_t default_symmetric_memory_size = 1u << 28;

        // Per-PE symmetric-memory footprint in bytes for a given geometry
        // (identical on every PE by construction): two O(npes*slots*mtu)
        // tx/rx arenas, two O(npes^2) credit matrices, plus one O(npes)
        // fingerprint row that the parcelport's startup uniformity check
        // publishes into (see config_words()).  The fingerprint row lives in
        // this allocation rather than a separate shmem_malloc() because that
        // call is collective and do_run() is not reached by every PE in
        // lockstep (rank 0 runs in runtime_mode::console, the others in
        // runtime_mode::worker).
        static constexpr std::size_t symmetric_bytes(
            std::size_t npes, std::size_t slots, std::size_t mtu) noexcept
        {
            return 2 * npes * slots * mtu +
                (2 * npes * npes + npes) * sizeof(std::uint32_t);
        }

        mailbox_array() = default;

        mailbox_array(std::size_t num_pes, std::size_t my_pe,
            std::size_t mtu, std::size_t slots_per_dst,
            std::size_t symmetric_memory_size = default_symmetric_memory_size,
            bool checksum = true,
            std::size_t probe_window = default_probe_window)
          : num_pes_(num_pes)
          , my_pe_(my_pe)
          , mtu_(mtu)
          , slots_per_dst_(slots_per_dst)
          , symmetric_memory_size_(symmetric_memory_size)
          , checksum_(checksum)
          , probe_window_(probe_window)
        {
            HPX_ASSERT(slots_per_dst_ > 0);

            std::size_t const pages = num_pes_ * slots_per_dst_ * mtu_;

            // one zeroed allocation; zeroed produced/consumed = correct
            // monotonic start state (0 pages produced/consumed).
            std::size_t const bytes =
                symmetric_bytes(num_pes_, slots_per_dst_, mtu_);

            // The runtime's real symmetric heap size (SMA_SYMMETRIC_SIZE, as
            // understood by OpenMPI's oshmem) takes precedence over the
            // configured expectation.  Overflowing it is fatal inside
            // shmem_calloc, so reject the geometry up front, identically on
            // every PE (the inputs are identical), before any collective call.
            if (char const* env = std::getenv("SMA_SYMMETRIC_SIZE"))
            {
                char* end = nullptr;
                double v = std::strtod(env, &end);
                if (end != env && v > 0)
                {
                    switch (end && *end ? *end : 0)
                    {
                    case 'k': case 'K': v *= 1024.0; break;
                    case 'm': case 'M': v *= 1024.0 * 1024.0; break;
                    case 'g': case 'G': v *= 1024.0 * 1024.0 * 1024.0; break;
                    case 't': case 'T': v *= 1099511627776.0; break;
                    default: break;
                    }
                    symmetric_memory_size_ = static_cast<std::size_t>(v);
                }
            }
            if (symmetric_memory_size_ > 0 && bytes > symmetric_memory_size_)
            {
                HPX_THROW_EXCEPTION(hpx::error::out_of_memory,
                    "openshmem::mailbox_array::mailbox_array",
                    "mailbox geometry needs " + std::to_string(bytes) +
                        " bytes of symmetric memory (npes=" +
                        std::to_string(num_pes_) + " slots=" +
                        std::to_string(slots_per_dst_) + " mtu=" +
                        std::to_string(mtu_) +
                        ") but the symmetric heap is only " +
                        std::to_string(symmetric_memory_size_) +
                        " bytes; lower hpx.parcel.openshmem.slots/mtu or "
                        "raise it with SMA_SYMMETRIC_SIZE");
            }

            // Advisory warning (PE 0 only) when the mapping grows large
            // relative to the symmetric segment the runtime was actually
            if (my_pe_ == 0 && symmetric_memory_size_ > 0 &&
                bytes > symmetric_memory_size_ / 4 * 3)
            {
                std::fprintf(stderr,
                    "openshmem: warning: symmetric memory mapping is %zu "
                    "bytes (2 * npes * slots * mtu + (2 * npes^2 + npes) * 4 "
                    "with "
                    "npes=%zu slots=%zu mtu=%zu), more than three quarters of the "
                    "configured symmetric segment (%zu bytes).  Lower "
                    "hpx.parcel.openshmem.slots (build-time "
                    "HPX_WITH_PARCELPORT_OPENSHMEM_SLOTS), run on fewer PEs, "
                    "or raise the symmetric heap at launch (e.g. oshrun -x "
                    "SMA_SYMMETRIC_SIZE=8G).\n",
                    bytes, num_pes_, slots_per_dst_, mtu_,
                    symmetric_memory_size_);
            }

            heap_ = shmem_calloc(bytes, 1);
            if (!heap_)
            {
                // Report a catchable failure so the parcelport (and the
                // application) can shut down gracefully
                HPX_THROW_EXCEPTION(hpx::error::out_of_memory,
                    "openshmem::mailbox_array::mailbox_array",
                    "shmem_calloc(" + std::to_string(bytes) +
                        ") failed on PE " + std::to_string(my_pe_));
            }

            unsigned char* base = static_cast<unsigned char*>(heap_);
            tx_beg_ = base;
            tx_end_ = base + pages;
            rx_beg_ = tx_end_;
            rx_end_ = rx_beg_ + pages;
            produced_beg_ = reinterpret_cast<std::uint32_t*>(rx_end_);
            produced_end_ = produced_beg_ + num_pes_ * num_pes_;
            consumed_beg_ = produced_end_;
            consumed_end_ = consumed_beg_ + num_pes_ * num_pes_;
            config_beg_ = reinterpret_cast<std::uint32_t*>(consumed_end_);
            config_end_ = config_beg_ + num_pes_;

            // Publish the zeroed state to all PEs before any peer drives
            // progress against us.
            shmem_barrier_all();

            // Local-only mirror counters (never symmetric, never remote).
            // produced_locals_[dst] = how many pages I have published to dst
            produced_locals_ = new std::atomic<std::uint32_t>[num_pes_];
            consumed_locals_ = new std::atomic<std::uint32_t>[num_pes_];
            published_locals_ = new std::uint32_t[num_pes_]();
            scan_rotate_ = new std::atomic<std::uint32_t>[num_pes_];
            for (std::size_t i = 0; i < num_pes_; ++i)
            {
                produced_locals_[i].store(0, std::memory_order_relaxed);
                consumed_locals_[i].store(0, std::memory_order_relaxed);
                scan_rotate_[i].store(0, std::memory_order_relaxed);
            }
        }

        ~mailbox_array()
        {
            delete[] produced_locals_;
            delete[] consumed_locals_;
            delete[] published_locals_;
            delete[] scan_rotate_;
            if (heap_)
            {
                shmem_free(heap_);
            }
        }

        mailbox_array(mailbox_array const&) = delete;
        mailbox_array& operator=(mailbox_array const&) = delete;

        mailbox_array(mailbox_array&& other) noexcept
          : heap_(other.heap_)
          , tx_beg_(other.tx_beg_)
          , tx_end_(other.tx_end_)
          , rx_beg_(other.rx_beg_)
          , rx_end_(other.rx_end_)
          , produced_beg_(other.produced_beg_)
          , produced_end_(other.produced_end_)
          , consumed_beg_(other.consumed_beg_)
          , consumed_end_(other.consumed_end_)
          , config_beg_(other.config_beg_)
          , config_end_(other.config_end_)
          , produced_locals_(other.produced_locals_)
          , consumed_locals_(other.consumed_locals_)
          , published_locals_(other.published_locals_)
          , scan_rotate_(other.scan_rotate_)
          , num_pes_(other.num_pes_)
          , my_pe_(other.my_pe_)
          , mtu_(other.mtu_)
          , slots_per_dst_(other.slots_per_dst_)
          , symmetric_memory_size_(other.symmetric_memory_size_)
          , checksum_(other.checksum_)
          , probe_window_(other.probe_window_)
          , remote_delivery_(other.remote_delivery_)
          , progress_interval_(other.progress_interval_)
        {
            other.heap_ = nullptr;
            other.tx_beg_ = nullptr;
            other.tx_end_ = nullptr;
            other.rx_beg_ = nullptr;
            other.rx_end_ = nullptr;
            other.produced_beg_ = nullptr;
            other.produced_end_ = nullptr;
            other.consumed_beg_ = nullptr;
            other.consumed_end_ = nullptr;
            other.config_beg_ = nullptr;
            other.config_end_ = nullptr;
            other.produced_locals_ = nullptr;
            other.consumed_locals_ = nullptr;
            other.published_locals_ = nullptr;
            other.scan_rotate_ = nullptr;
        }

        mailbox_array& operator=(mailbox_array&& other) noexcept
        {
            if (this != &other)
            {
                if (heap_)
                {
                    shmem_free(heap_);
                }
                delete[] produced_locals_;
                delete[] consumed_locals_;
                delete[] published_locals_;
                heap_ = other.heap_;
                tx_beg_ = other.tx_beg_;
                tx_end_ = other.tx_end_;
                rx_beg_ = other.rx_beg_;
                rx_end_ = other.rx_end_;
                produced_beg_ = other.produced_beg_;
                produced_end_ = other.produced_end_;
                consumed_beg_ = other.consumed_beg_;
                consumed_end_ = other.consumed_end_;
                config_beg_ = other.config_beg_;
                config_end_ = other.config_end_;
                produced_locals_ = other.produced_locals_;
                consumed_locals_ = other.consumed_locals_;
                published_locals_ = other.published_locals_;
                scan_rotate_ = other.scan_rotate_;
                num_pes_ = other.num_pes_;
                my_pe_ = other.my_pe_;
                mtu_ = other.mtu_;
                slots_per_dst_ = other.slots_per_dst_;
                symmetric_memory_size_ = other.symmetric_memory_size_;
                checksum_ = other.checksum_;
                probe_window_ = other.probe_window_;
                remote_delivery_ = other.remote_delivery_;
                progress_interval_ = other.progress_interval_;
                other.heap_ = nullptr;
                other.tx_beg_ = nullptr;
                other.tx_end_ = nullptr;
                other.rx_beg_ = nullptr;
                other.rx_end_ = nullptr;
                other.produced_beg_ = nullptr;
                other.produced_end_ = nullptr;
                other.consumed_beg_ = nullptr;
                other.consumed_end_ = nullptr;
                other.config_beg_ = nullptr;
                other.config_end_ = nullptr;
                other.produced_locals_ = nullptr;
                other.consumed_locals_ = nullptr;
                other.published_locals_ = nullptr;
                other.scan_rotate_ = nullptr;
            }
            return *this;
        }

        // Return the symmetric scratch page that a sender uses to stage the
        // chunk for the current credit slot of (dst_pe).
        unsigned char* tx_page(std::size_t dst_pe) const
        {
            std::size_t const slot =
                produced_locals_[dst_pe].load(std::memory_order_relaxed) %
                slots_per_dst_;
            return tx_beg_ + (dst_pe * slots_per_dst_ + slot) * mtu_;
        }

        unsigned char* get_buffer(std::size_t pe) const
        {
            return tx_page(pe);
        }

        // checksum + sequence guard over received pages.  Disable only when the
        // checksum pass shows up in profiling
        bool checksum_enabled() const noexcept
        {
            return checksum_;
        }

        // Total number of pages transferred so far (published + consumed),
        // summed over the local mirrors.  Only used by do_stop()
        std::uint64_t transfer_count() const noexcept
        {
            std::uint64_t total = 0;
            for (std::size_t i = 0; i < num_pes_; ++i)
            {
                total += produced_locals_[i].load(std::memory_order_relaxed);
                total += consumed_locals_[i].load(std::memory_order_relaxed);
            }
            return total;
        }

        // Non-blocking scan of the source arena owned by 'arena_idx'.  With
        // 'arena_cnt' concurrent scanners each thread polls only its own slots
        int try_detect_pe_notification(std::size_t arena_idx = 0,
            std::size_t arena_cnt = 0) const noexcept
        {
            if (arena_cnt == 0)
            {
                arena_cnt = 1;
            }

            std::size_t const lo = (num_pes_ * arena_idx) / arena_cnt;
            std::size_t const hi = (num_pes_ * (arena_idx + 1)) / arena_cnt;
            std::size_t const width = hi - lo;
            if (width == 0)
            {
                return -1;
            }

            std::size_t const window = probe_window_ == 0 ?
                width :
                (std::min)(width, probe_window_);

            // Rotate the scan start point so no single (lowest-index) ready
            // source can hog this arena.
            std::size_t const slot =
                arena_idx < num_pes_ ? arena_idx : num_pes_ - 1;
            std::size_t start =
                lo + (scan_rotate_[slot].fetch_add(1,
                         std::memory_order_relaxed) %
                    width);

            if (!remote_delivery_)
            {
                maybe_local_progress();
            }

            for (std::size_t k = 0; k < window; ++k)
            {
                std::size_t const src = lo + ((start - lo + k) % width);

                if (src == my_pe_)
                {
                    continue;
                }
                std::size_t const w = src * num_pes_ + my_pe_;

                // Legacy mode only: pull delivery of src's inbound stores
                // (putmem + produced atomic_set) into local symmetric memory.
                if (remote_delivery_)
                {
                    remote_progress_to(src);
                }
                std::uint32_t const produced = produced_beg_[w];
                std::uint32_t const consumed =
                    consumed_locals_[src].load(std::memory_order_relaxed);
                if (produced > consumed)
                {
                    return static_cast<int>(src);
                }
            }
            return -1;
        }

        // True while src has published a page that we have not drained yet.
        // Only stable for a caller holding the per-source receive
        // reservation, since draining is what advances 'consumed'.
        bool page_ready(std::size_t const src) const noexcept
        {
            progress_to(src);
            std::size_t const w = src * num_pes_ + my_pe_;
            return produced_beg_[w] >
                consumed_locals_[src].load(std::memory_order_relaxed);
        }

        // Legacy delivery push: a remote atomic_fetch to the peer "pushes" the
        // peer's inbound stores into our local symmetric memory.  Each call is
        // two blocking network round trips, issued per probed peer on every
        // scan pass; on a busy node that dominated the io threads.
        void remote_progress_to(std::size_t const peer) const noexcept
        {
            std::size_t const w = my_pe_ * num_pes_ + peer;
            shmem_uint32_atomic_fetch(
                &produced_beg_[w], static_cast<int>(peer));
            shmem_uint32_atomic_fetch(
                &consumed_beg_[w], static_cast<int>(peer));
        }

        // Cheap local progress: an atomic fetch on our own PE drives the
        // transport's progress engine (so inbound puts/atomics are completed
        // and become visible in local symmetric memory) without a network
        // round trip.  The counters we poll are written into our memory by
        // the peers, so reading local memory after this is enough.
        void local_progress() const noexcept
        {
            std::uint64_t const t0 =
                detail::g_io_stats_on.load(std::memory_order_relaxed) ?
                detail::ns_now() : 0;
            shmem_uint32_atomic_fetch(
                &produced_beg_[my_pe_ * num_pes_ + my_pe_],
                static_cast<int>(my_pe_));
            if (t0 != 0)
                detail::g_io_stats.prog_ns.fetch_add(
                    detail::ns_now() - t0, std::memory_order_relaxed);
        }

        // Local progress, throttled: every progress_interval_-th call only
        // (1 = every call, 0 = never).  The call is a blocking transport entry
        // that cost ~5 us on a busy hub rank, and one io thread spins on it.
        void maybe_local_progress() const noexcept
        {
            std::uint32_t const interval = progress_interval_;
            if (interval == 0)
            {
                return;
            }
            if (interval == 1 ||
                (progress_calls_.fetch_add(1, std::memory_order_relaxed) %
                    interval) == 0)
            {
                local_progress();
            }
        }

        void progress_to(std::size_t const peer) const noexcept
        {
            if (remote_delivery_)
            {
                remote_progress_to(peer);
            }
            else
            {
                maybe_local_progress();
            }
        }

        void set_progress_interval(std::uint32_t n) noexcept
        {
            progress_interval_ = n;
        }

        // hpx.parcel.openshmem.remote_delivery (default 0): use the legacy
        // per-peer remote fetches instead of local progress.
        void set_remote_delivery(bool on) noexcept
        {
            remote_delivery_ = on;
        }

        // ---- batched send -------------------------------------------------
        // Pages that can be sent to dst_pe right now (free credit), 0 if none.
        // A batch of k pages is staged into the k consecutive tx slots
        // following the current 'produced' (tx_page_at), pushed with
        // non-blocking puts (put_page_nbi) and made visible to the receiver
        // with a single counter update (publish_pages).  Reusing a tx slot is
        // safe without a quiet: a slot is only reused once the receiver has
        // consumed the page previously sent from it.
        std::size_t send_credits(std::size_t const dst_pe) noexcept
        {
            std::size_t const w = my_pe_ * num_pes_ + dst_pe;
            std::uint32_t const produced =
                produced_locals_[dst_pe].load(std::memory_order_relaxed);
            std::uint32_t consumed = (dst_pe == my_pe_) ?
                consumed_locals_[dst_pe].load(std::memory_order_relaxed) :
                consumed_beg_[w];
            if (static_cast<int>(produced - consumed) >=
                static_cast<int>(slots_per_dst_))
            {
                if (dst_pe != my_pe_)
                {
                    progress_to(dst_pe);    // deliver dst's consumed
                    consumed = consumed_beg_[w];
                }
                else
                {
                    consumed = consumed_locals_[dst_pe].load(
                        std::memory_order_relaxed);
                }
                if (static_cast<int>(produced - consumed) >=
                    static_cast<int>(slots_per_dst_))
                {
                    if (detail::g_io_stats_on.load(std::memory_order_relaxed))
                        detail::g_io_stats.nocredit.fetch_add(
                            1, std::memory_order_relaxed);
                    return 0;
                }
            }
            return slots_per_dst_ - static_cast<std::size_t>(produced - consumed);
        }

        unsigned char* tx_page_at(
            std::size_t const dst_pe, std::size_t const k) const noexcept
        {
            std::size_t const slot =
                (produced_locals_[dst_pe].load(std::memory_order_relaxed) + k) %
                slots_per_dst_;
            return tx_beg_ + (dst_pe * slots_per_dst_ + slot) * mtu_;
        }

        void put_page_nbi(std::size_t const dst_pe, std::size_t const k,
            std::size_t const count) noexcept
        {
            HPX_ASSERT(count <= mtu_);
            std::size_t const slot =
                (produced_locals_[dst_pe].load(std::memory_order_relaxed) + k) %
                slots_per_dst_;
            unsigned char* const src =
                tx_beg_ + (dst_pe * slots_per_dst_ + slot) * mtu_;
            unsigned char* const dst =
                rx_beg_ + (my_pe_ * slots_per_dst_ + slot) * mtu_;
            if (dst_pe == my_pe_)
            {
                shmem_putmem(dst, src, count, static_cast<int>(dst_pe));
            }
            else
            {
                shmem_putmem_nbi(dst, src, count, static_cast<int>(dst_pe));
            }
        }

        void publish_pages(std::size_t const dst_pe, std::size_t const n,
            std::size_t const bytes) noexcept
        {
            std::size_t const w = my_pe_ * num_pes_ + dst_pe;
            std::uint32_t const produced =
                produced_locals_[dst_pe].load(std::memory_order_relaxed) +
                static_cast<std::uint32_t>(n);

            // order all pages of the batch before the produced counter
            shmem_fence();
            shmem_uint32_atomic_set(
                &produced_beg_[w], produced, static_cast<int>(dst_pe));
            produced_locals_[dst_pe].store(produced, std::memory_order_relaxed);
            shmem_fence();

            if (dst_pe != my_pe_)
            {
                progress_to(dst_pe);
            }
            if (detail::g_io_stats_on.load(std::memory_order_relaxed))
            {
                detail::g_io_stats.pages_sent.fetch_add(
                    n, std::memory_order_relaxed);
                detail::g_io_stats.bytes_sent.fetch_add(
                    bytes, std::memory_order_relaxed);
            }
        }

        // Non-blocking send of one mtu-sized page (chunk) to dst_pe.
        // Copies 'count' bytes from our staging page (get_buffer())
        bool try_send(std::size_t const dst_pe,
            std::size_t const count) noexcept
        {
            HPX_ASSERT(count <= mtu_);

            // Works for both remote (dst != me) and local (dst == me, a
            // self-putmem into our own rx pool) destinations.
            std::size_t const w = my_pe_ * num_pes_ + dst_pe;

            // mirrors of the two halves
            std::uint32_t produced =
                produced_locals_[dst_pe].load(std::memory_order_relaxed);

            // dst's consumed counter: for a local send it is our own drain
            // mirror (same thread); otherwise the delivered remote copy.
            std::uint32_t consumed = (dst_pe == my_pe_) ?
                consumed_locals_[dst_pe].load(std::memory_order_relaxed) :
                consumed_beg_[w];

            // Poll the credit (max slots_per_dst_ pages in flight).  Deliver
            // dst's consumed counter exactly once; if still no credit, return
            // false to retry
            if (static_cast<int>(produced - consumed) >=
                static_cast<int>(slots_per_dst_))
            {
                if (dst_pe != my_pe_)
                {
                    progress_to(dst_pe);            // deliver dst's consumed
                    consumed = consumed_beg_[w];    // read local (delivered)
                }
                else
                {
                    consumed =
                        consumed_locals_[dst_pe].load(std::memory_order_relaxed);
                }

                if (static_cast<int>(produced - consumed) >=
                    static_cast<int>(slots_per_dst_))
                {
                    if (detail::g_io_stats_on.load(std::memory_order_relaxed))
                        detail::g_io_stats.nocredit.fetch_add(1, std::memory_order_relaxed);
                    return false;    // no credit: requeue and retry later
                }
            }

            std::size_t const slot = produced % slots_per_dst_;

            // Staging source: my per-dst staging page (tx mirror of the rx
            // slot), keyed by dst_pe
            std::size_t const stage_slot =
                (dst_pe * slots_per_dst_ + slot) * mtu_;

            // Landing target: dst's shared rx pool, keyed by self PE (the
            // sender), so each sender has its own disjoint per-src ring
            std::size_t const rx_slot = (my_pe_ * slots_per_dst_ + slot) * mtu_;

            // data: blocking putmem staging -> dst's rx page.
            std::uint64_t const st_t0 =
                detail::g_io_stats_on.load(std::memory_order_relaxed) ?
                detail::ns_now() : 0;

            shmem_putmem(rx_beg_ + rx_slot, tx_beg_ + stage_slot, count,
                static_cast<int>(dst_pe));

            // shmem_putmem() returns once the source is reusable, not once
            // the bytes have landed at dst.  Order the page before the
            // produced counter, otherwise the receiver can observe the new
            // counter and read a stale/partial page.
            shmem_fence();

            // publish produced (low-32; self owns it)
            produced = produced + 1;
            shmem_uint32_atomic_set(
                &produced_beg_[w], produced, static_cast<int>(dst_pe));
            produced_locals_[dst_pe].store(
                produced, std::memory_order_relaxed);    // update our mirror
            shmem_fence();

            // delivery (push): issue a remote atomic_fetch to the peer right
            // after publishing produced, so our inbound stores.
            if (dst_pe != my_pe_)
            {
                progress_to(dst_pe);
            }

            if (st_t0 != 0)
            {
                detail::g_io_stats.put_ns.fetch_add(
                    detail::ns_now() - st_t0, std::memory_order_relaxed);
                detail::g_io_stats.pages_sent.fetch_add(1, std::memory_order_relaxed);
                detail::g_io_stats.bytes_sent.fetch_add(count, std::memory_order_relaxed);
            }

            return true;
        }

        // Zero-copy receive, step 1: if src has published its next page,
        // return a pointer to it in our symmetric rx ring (valid until
        // release_page(src) is called, because the sender may only reuse the
        // slot after the credit is returned), else nullptr.  Lets the
        // receiver validate and copy the payload straight to its final place
        // instead of staging the whole page in a scratch buffer first.
        unsigned char const* peek_page(std::size_t const src_pe) noexcept
        {
            std::size_t const w = src_pe * num_pes_ + my_pe_;

            std::uint32_t const consumed =
                consumed_locals_[src_pe].load(std::memory_order_relaxed);

            std::uint32_t produced = produced_beg_[w];

            // Poll for src to publish page 'consumed' (data ready).  Deliver
            // src's produced exactly once; if still not ready, requeue.
            if (produced <= consumed)
            {
                progress_to(src_pe);    // deliver src's produced
                produced = produced_beg_[w];
                if (produced <= consumed)
                {
                    return nullptr;    // not ready: requeue and retry later
                }
            }

            std::size_t const slot = consumed % slots_per_dst_;
            // src's per-src ring in our shared rx pool
            // ((src_pe * slots + slot) * mtu_).
            return rx_beg_ + (src_pe * slots_per_dst_ + slot) * mtu_;
        }

        // Zero-copy receive, step 2: give the page returned by peek_page()
        // back.  The local consumed counter advances immediately (so the next
        // peek_page() sees the next slot), but the credit is published to
        // src lazily: every slots/2 pages and whenever the caller invokes
        // flush_credits() (end of a scan, before a message is dispatched).
        // Publishing is an atomic_set plus a fence, which used to be paid per
        // page.  The page must not be touched afterwards.
        void release_page(std::size_t const src_pe) noexcept
        {
            std::uint32_t const consumed =
                consumed_locals_[src_pe].load(std::memory_order_relaxed) + 1;
            consumed_locals_[src_pe].store(
                consumed, std::memory_order_relaxed);    // update mirror
            if (detail::g_io_stats_on.load(std::memory_order_relaxed))
                detail::g_io_stats.pages_recv.fetch_add(
                    1, std::memory_order_relaxed);
            std::uint32_t const unpublished =
                consumed - published_locals_[src_pe];
            std::size_t const batch =
                slots_per_dst_ > 1 ? slots_per_dst_ / 2 : 1;
            if (unpublished >= batch)
            {
                flush_credits(src_pe);
            }
        }

        // Publish the consumed count for src (no-op if nothing is pending).
        void flush_credits(std::size_t const src_pe) noexcept
        {
            std::uint32_t const consumed =
                consumed_locals_[src_pe].load(std::memory_order_relaxed);
            if (consumed == published_locals_[src_pe])
            {
                return;
            }
            std::uint64_t const rt0 =
                detail::g_io_stats_on.load(std::memory_order_relaxed) ?
                detail::ns_now() : 0;
            std::size_t const w = src_pe * num_pes_ + my_pe_;
            shmem_uint32_atomic_set(
                &consumed_beg_[w], consumed, static_cast<int>(src_pe));
            published_locals_[src_pe] = consumed;
            shmem_fence();
            if (rt0 != 0)
                detail::g_io_stats.rel_ns.fetch_add(
                    detail::ns_now() - rt0, std::memory_order_relaxed);
        }

        // Non-blocking receive of one page (chunk) from src_pe.  Copies
        // 'count' bytes off our shared rx page into out_buf, then returns
        // the credit (publishes consumed) so src may reuse the slot.  If the
        // page is not yet published, returns false; the caller must requeue
        // and retry later.  Never blocks.
        bool try_receive_(std::size_t const src_pe,
            unsigned char* const out_buf, std::size_t const count) noexcept
        {
            HPX_ASSERT(count <= mtu_);

            unsigned char const* const page = peek_page(src_pe);
            if (page == nullptr)
            {
                return false;
            }
            std::memcpy(out_buf, page, count);
            release_page(src_pe);
            flush_credits(src_pe);
            return true;
        }

        bool receive(
            unsigned char* const output, std::size_t const count) noexcept
        {
            int const pe = try_detect_pe_notification();
            if (pe < 0)
            {
                return false;
            }
            return try_receive_(static_cast<std::size_t>(pe), output, count);
        }

        constexpr std::size_t mtu() const noexcept
        {
            return mtu_;
        }

        // The startup wire-protocol uniformity check in the parcelport needs
        // one symmetric word per PE to publish its fingerprint into.  Hand out
        // the row carved from this allocation instead of a separate
        // shmem_malloc(): that call is collective, and do_run() is not reached
        // by every PE in lockstep (rank 0 runs in runtime_mode::console, the
        // others in runtime_mode::worker), so allocating there deadlocked the
        // job before main().  This carve is identical on every PE and was
        // already published behind the constructor's barrier.
        // const: the caller (check_protocol_config_uniformity) is a const
        // member of the parcelport and so sees a const mailbox_array.  The
        // returned pointer is a copy, so the pointee stays writable; only the
        // member pointer itself is const through this overload.
        std::uint32_t* config_words() const noexcept
        {
            return config_beg_;
        }

        constexpr std::size_t num_pes() const noexcept
        {
            return num_pes_;
        }

        constexpr std::size_t my_pe() const noexcept
        {
            return my_pe_;
        }

        // Monotonically increasing per-PE message identifier.  Every parcel
        // (message) gets a fresh id, which the receiver uses to cross-check
        std::uint64_t next_message_id() noexcept
        {
            return message_sequence_.fetch_add(1, std::memory_order_relaxed) + 1;
        }

    private:
        void* heap_ = nullptr;

        unsigned char* tx_beg_ = nullptr;
        unsigned char* tx_end_ = nullptr;
        unsigned char* rx_beg_ = nullptr;
        unsigned char* rx_end_ = nullptr;
        std::uint32_t* produced_beg_ = nullptr;
        std::uint32_t* produced_end_ = nullptr;
        std::uint32_t* consumed_beg_ = nullptr;
        std::uint32_t* consumed_end_ = nullptr;
        std::uint32_t* config_beg_ = nullptr;
        std::uint32_t* config_end_ = nullptr;
        std::atomic<std::uint32_t>* produced_locals_ = nullptr;
        std::atomic<std::uint32_t>* consumed_locals_ = nullptr;
        // Last consumed count actually published to each source PE (<=
        // consumed_locals_).  Only touched by the thread that currently owns
        // the source's receive stream (see receiver::accept()).
        std::uint32_t* published_locals_ = nullptr;
        mutable std::atomic<std::uint32_t>* scan_rotate_ = nullptr;

        std::size_t num_pes_ = 0;
        std::size_t my_pe_ = 0;
        std::size_t mtu_ = 0;
        std::size_t slots_per_dst_ = 0;
        std::size_t symmetric_memory_size_ = 0;
        bool checksum_ = true;
        std::size_t probe_window_ = default_probe_window;
        bool remote_delivery_ = false;
        std::uint32_t progress_interval_ = 1;
        mutable std::atomic<std::uint32_t> progress_calls_{0};

        std::atomic<std::uint64_t> message_sequence_{0};
    };
}    // namespace hpx::parcelset::policies::openshmem

#endif
