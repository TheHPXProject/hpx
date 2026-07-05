// Copyright (c) 2026 Sai Charan Arvapally
//
// SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#include <hpx/modules/async_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/topology.hpp>

#include <hpx/executors/thread_pool_scheduler.hpp>
#include <hpx/executors/thread_pool_scheduler_bulk.hpp>

#include <cstddef>
#include <exception>
#include <memory>
#include <span>

namespace hpx::execution::experimental {

    // P2079R10: Abstract backend interface for parallel_scheduler.
    // This mirrors stdexec's system_context_replaceability::parallel_scheduler_backend
    // but is expressed as a simple abstract class rather than using stdexec's __any
    // type-erasure machinery.
    //
    // The backend is responsible for:
    //   - schedule(): post a unit of work to the execution context
    //   - schedule_bulk_chunked(): post chunked bulk work
    //   - schedule_bulk_unchunked(): post unchunked bulk work
    //
    // The receiver_proxy / bulk_item_receiver_proxy interfaces allow the backend
    // to complete operations without knowing the concrete receiver type.

    // P2079R10 / P3804R2 receiver_proxy: type-erased completion interface.
    // The backend calls these to signal completion back to the frontend.
    // stop_requested() allows the backend to poll for cancellation during
    // execution (partial substitute for try_query<inplace_stop_token>).
    //
    // P3804R2: No virtual destructor - objects are never destroyed polymorphically.
    // The frontend knows the concrete type and destroys it directly.
    HPX_CXX_CORE_EXPORT struct parallel_scheduler_receiver_proxy
    {
        virtual void set_value() noexcept = 0;
        virtual void set_error(std::exception_ptr) noexcept = 0;
        virtual void set_stopped() noexcept = 0;
        // P2079R10 4.2 / P3804R2: backends can poll this to check if work should stop.
        // Returns true if the associated stop token has been signalled.
        // const-qualified per P3804R2 (aligns with try_query being const).
        virtual bool stop_requested() const noexcept
        {
            return false;
        }

    protected:
        // P3804R2: Protected non-virtual destructor.
        // Prevents polymorphic deletion while allowing derived classes to clean up.
        ~parallel_scheduler_receiver_proxy() = default;
    };

    // P2079R10 bulk_item_receiver_proxy: extends receiver_proxy with
    // execute(begin, end) for bulk work items.
    HPX_CXX_CORE_EXPORT struct parallel_scheduler_bulk_item_receiver_proxy
      : parallel_scheduler_receiver_proxy
    {
        virtual void execute(std::size_t begin, std::size_t end) noexcept = 0;
    };

    // P2079R10 4.2: Pre-allocated storage for backend operation states.
    // The frontend provides a std::span<std::byte> of this size to each
    // backend method so the backend can avoid heap allocation.
    // Backends that need more can fall back to their own allocation.
    HPX_CXX_CORE_EXPORT inline constexpr std::size_t
        parallel_scheduler_storage_size = 256;
    HPX_CXX_CORE_EXPORT inline constexpr std::size_t
        parallel_scheduler_storage_alignment = alignof(std::max_align_t);

    // P2079R10 / P3927R2: Abstract backend interface
    HPX_CXX_CORE_EXPORT struct parallel_scheduler_backend
    {
        virtual ~parallel_scheduler_backend() = default;

        // Schedule a single unit of work. On completion, call proxy.set_value().
        // storage: pre-allocated scratch space from the frontend's
        //          operation_state (parallel_scheduler_storage_size bytes).
        // P3927R2: parameter order is (receiver, storage)
        virtual void schedule(parallel_scheduler_receiver_proxy& proxy,
            std::span<std::byte> storage) noexcept = 0;

        // Schedule chunked bulk work of size count.
        // The backend partitions [0, count) into subranges and calls
        // proxy.execute(begin, end) for each subrange, then proxy.set_value().
        // P3927R2: parameter order is (shape, receiver, storage)
        virtual void schedule_bulk_chunked(std::size_t count,
            parallel_scheduler_bulk_item_receiver_proxy& proxy,
            std::span<std::byte> storage) noexcept = 0;

        // Schedule unchunked bulk work of size count.
        // The backend calls proxy.execute(i, i+1) for each i in [0, count),
        // then proxy.set_value().
        // P3927R2: parameter order is (shape, receiver, storage)
        virtual void schedule_bulk_unchunked(std::size_t count,
            parallel_scheduler_bulk_item_receiver_proxy& proxy,
            std::span<std::byte> storage) noexcept = 0;

        // custom equality for backends.
        // P2079R10 section 6.4 defines parallel_scheduler equality purely by
        // shared_ptr target identity (pointer equality), so this method is
        // NOT called by parallel_scheduler::operator==.
        // Custom backends may implement it for their own comparisons.
        virtual bool equal_to(
            parallel_scheduler_backend const& other) const noexcept = 0;

        // Access the underlying thread pool scheduler (HPX-specific).
        // Returns nullptr if this backend doesn't wrap a thread_pool_policy_scheduler.
        // Used by parallel_scheduler_domain::transform_sender to create
        // optimized thread_pool_bulk_sender directly (bypassing virtual dispatch
        // for bulk operations when the default HPX backend is in use).
        virtual thread_pool_policy_scheduler<hpx::launch> const*
        get_underlying_scheduler() const noexcept
        {
            return nullptr;
        }

        // Access the cached PU mask (HPX-specific).
        // Returns nullptr if unavailable.
        virtual hpx::threads::mask_type const* get_pu_mask() const noexcept
        {
            return nullptr;
        }
    };

    // P2079R10: Function pointer factory type for replacing the default
    // backend. Using a function pointer avoids platform-specific weak-linking
    // issues while still providing P2079R10 replaceability semantics.
    HPX_CXX_CORE_EXPORT using parallel_scheduler_backend_factory_t =
        std::shared_ptr<parallel_scheduler_backend> (*)();

    // P2079R10: Get the current parallel_scheduler_backend.
    // Thread-safe. Creates the default backend on first call via the factory.
    // Can be replaced at any time via set_parallel_scheduler_backend().
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT
        std::shared_ptr<parallel_scheduler_backend>
        query_parallel_scheduler_backend();

    // P2079R10: Replace the parallel scheduler backend factory.
    // The new factory is used the next time query_parallel_scheduler_backend()
    // creates a backend (only if no backend has been created yet, or after
    // set_parallel_scheduler_backend() clears the current one).
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT parallel_scheduler_backend_factory_t
    set_parallel_scheduler_backend_factory(
        parallel_scheduler_backend_factory_t new_factory) noexcept;

    // P2079R10: Directly replace the active backend.
    // Takes effect immediately: the next get_parallel_scheduler() call
    // returns a scheduler backed by new_backend.
    // Thread-safe, but must not be called while active operations are
    // in-flight on the current backend.
    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT void set_parallel_scheduler_backend(
        std::shared_ptr<parallel_scheduler_backend> new_backend);

}    // namespace hpx::execution::experimental
