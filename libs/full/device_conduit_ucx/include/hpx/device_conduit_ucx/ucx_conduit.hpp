//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_DEVICE_CONDUIT_UCX)
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/synchronization.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <map>
#include <string>
#include <vector>

// the UCX handle types (ucp_context_h etc. are pointers to these)
struct ucp_context;
struct ucp_worker;
struct ucp_ep;

namespace hpx::parcelset::device_conduits {

    /// A device conduit moving the zero-copy chunks of device memory of
    /// parcels with UCX tagged messages: a context and a worker of its own
    /// per locality (also next to a UCX based MPI library, which uses the
    /// same UCX library), one endpoint per destination, created on the first
    /// message to it from the destination's worker address, which an action
    /// fetches. The UCX tag of a chunk is made of the sending locality and
    /// the descriptor's tag.
    class HPX_EXPORT ucx_conduit : public device_conduit_impl<ucx_conduit>
    {
    public:
        explicit ucx_conduit(util::runtime_configuration const& cfg);
        ~ucx_conduit() override;

        ucx_conduit(ucx_conduit const&) = delete;
        ucx_conduit& operator=(ucx_conduit const&) = delete;

        [[nodiscard]] static std::string name_impl()
        {
            return "ucx";
        }

        static void config_defaults(std::vector<std::string>& ini);

        [[nodiscard]] bool can_reach_impl(std::uint32_t) const noexcept
        {
            return true;
        }

        void send_impl(std::uint32_t locality_id,
            serialization::detail::device_chunk_descriptor const& d,
            void const* p, std::size_t size,
            hpx::move_only_function<void(std::exception_ptr)> sent);

        void recv_impl(serialization::detail::device_chunk_descriptor const& d,
            void* p, std::size_t size,
            std::function<void(std::exception_ptr)> done);

        bool background_work_impl();

        /// Closes the endpoint to a locality that has left, after the sends
        /// in flight to it have finished (or failed, with
        /// hpx.parcel.conduit.ucx.error_handling 'peer'), or that was
        /// disconnected ('forced'), cancelling them (with 'peer'; with 'none'
        /// UCX allows only to flush the endpoint).
        void locality_removed_impl(
            std::uint32_t locality_id, bool forced) noexcept;

        /// The address of this locality's worker (what the action returns).
        [[nodiscard]] std::vector<char> const& worker_address() const noexcept
        {
            return address_;
        }

    private:
        // an endpoint, and which one of those to the same locality (a
        // removed locality's endpoint is closed, a later message to that
        // locality creates a new one)
        struct endpoint_entry
        {
            hpx::shared_future<ucp_ep*> ep;
            std::uint64_t generation = 0;
        };

        // the endpoint to 'locality_id', fetches the address and creates it
        // the first time (on an HPX thread)
        endpoint_entry endpoint(std::uint32_t locality_id);

        // starts closing an endpoint that is no longer in endpoints_ (or at
        // destruction), without holding endpoints_mtx_: UCX may complete the
        // sends on it right away, calling their callbacks
        void close_endpoint(ucp_ep* ep, std::uint32_t locality_id, bool force);

        // makes sure the OS thread has a CUDA context for the GPU of the
        // memory at 'p' (if it is device memory), remembers the GPU for
        // progress
        void note_device(void const* p) noexcept;

    public:
        // A completion of a send or receive, run outside of UCX (see
        // run_completions): UCX calls the callbacks of requests while it
        // holds its worker lock, and a completion may suspend its HPX thread
        // (it may send parcels, e.g. when the sent parcels are released),
        // which would leave the lock taken.
        void defer(hpx::move_only_function<void()> completion);

    private:
        // runs the deferred completions (no lock held)
        bool run_completions();

        // sends unless the endpoint was closed since (its locality was
        // removed)
        void start_send(std::uint32_t locality_id, std::uint64_t generation,
            ucp_ep* ep, std::uint64_t tag, void const* p, std::size_t size,
            hpx::move_only_function<void(std::exception_ptr)> sent);

        ucp_context* context_ = nullptr;
        ucp_worker* worker_ = nullptr;
        std::vector<char> address_;

        // UCP_ERR_HANDLING_MODE_PEER for the endpoints
        // (hpx.parcel.conduit.ucx.error_handling)
        bool peer_errors_ = true;

        hpx::spinlock endpoints_mtx_;
        std::map<std::uint32_t, endpoint_entry> endpoints_;
        std::uint64_t generation_ = 0;

        // the requests closing the endpoints of removed localities
        // (ucs_status_ptr_t, guarded by endpoints_mtx_)
        std::vector<void*> closing_;

        // the GPU of the last device memory sent or received: progress makes
        // its context current where none is (with several GPUs per locality,
        // the context of the last one)
        std::atomic<int> device_ = -1;

        // one thread at a time makes progress
        hpx::spinlock progress_mtx_;

        // the deferred completions (a lock that never suspends: taken in
        // UCX's callbacks)
        std::mutex completions_mtx_;
        std::vector<hpx::move_only_function<void()>> completions_;
    };
}    // namespace hpx::parcelset::device_conduits

#endif
