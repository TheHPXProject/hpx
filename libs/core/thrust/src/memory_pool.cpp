//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/modules/async_cuda.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/synchronization.hpp>

#include <hpx/thrust/memory.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <exception>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace hpx::thrust {

    namespace {

        // The pools hand out memory by size class: eight per power of two
        // (at most 12.5% more than asked for), at least min_size_class bytes.
        constexpr std::size_t min_size_class = 512;

        std::size_t size_class_of(std::size_t const bytes) noexcept
        {
            if (bytes <= min_size_class)
            {
                return min_size_class;
            }
            std::size_t power = min_size_class;
            while (power <= bytes / 2)
            {
                power *= 2;
            }
            std::size_t const step = power / 8;
            if (bytes > (std::numeric_limits<std::size_t>::max)() - step)
            {
                // more than any memory, allocating it fails
                return (std::numeric_limits<std::size_t>::max)();
            }
            return (bytes + step - 1) / step * step;
        }

        // the number of bytes the configuration entry hpx.thrust.max_cached
        // gives, or 'fallback' if there's none
        std::size_t max_cached_config(std::size_t const fallback)
        {
            std::string const value =
                hpx::get_config_entry("hpx.thrust.max_cached", "");
            if (value.empty())
            {
                return fallback;
            }
            // the whole value must be a non-negative number (stoull would
            // take "1GiB" as 1 and "-1" as the largest value)
            if (value.find_first_not_of("0123456789") == std::string::npos)
            {
                try
                {
                    return std::stoull(value);
                }
                catch (std::exception const&)
                {
                    // out of range
                }
            }
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::thrust::max_cached_config",
                "hpx.thrust.max_cached must be a number of bytes, not '{}'",
                value);
        }

        // Memory of one kind (device memory of one device, or pinned host
        // memory): blocks are allocated by size class and kept for reuse, at
        // most max_cached bytes of them (hpx.thrust.max_cached, else
        // 'fallback_max_cached').
        class pool
        {
        public:
            pool(memory_kind const kind, int const device,
                std::size_t const fallback_max_cached)
              : kind_(kind)
              , device_(device)
              , fallback_max_cached_(fallback_max_cached)
              , final_(hpx::is_running())
              , max_cached_(max_cached_config(fallback_max_cached))
            {
                // final_ is sampled first: it is set only if the limit was
                // read from the complete configuration
            }

            pool(pool const&) = delete;
            pool(pool&&) = delete;
            pool& operator=(pool const&) = delete;
            pool& operator=(pool&&) = delete;

            ~pool() = default;

            // a block of size class 'size_class'; the device memory is
            // allocated on the current device, which is this pool's device
            void* allocate(std::size_t const size_class)
            {
                reclaim_deferred();
                {
                    std::lock_guard<hpx::spinlock_no_backoff> l(mtx_);
                    if (auto it = free_.find(size_class);
                        it != free_.end() && !it->second.empty())
                    {
                        void* p = it->second.back();
                        it->second.pop_back();
                        if (it->second.empty())
                        {
                            free_.erase(it);
                        }
                        cached_ -= size_class;
                        return p;
                    }
                }

                if (void* p = allocate_new(size_class))
                {
                    return p;
                }

                // out of memory: give the cached blocks back and try again
                release();
                if (void* p = allocate_new(size_class))
                {
                    return p;
                }
                throw std::bad_alloc();
            }

            void deallocate(void* p, std::size_t const size_class) noexcept
            {
                try
                {
                    std::lock_guard<hpx::spinlock_no_backoff> l(mtx_);
                    if (cached_ + size_class <= max_cached_)
                    {
                        free_[size_class].push_back(p);
                        cached_ += size_class;
                        return;
                    }
                }
                catch (...)
                {
                    // could not keep it, free it instead
                }
                free_block(p);
            }

            // The block 'p' is given back while device work still reads it:
            // it is reused only once 'event' has completed.
            void deallocate_after(void* p, std::size_t const size_class,
                cudaEvent_t const event) noexcept
            {
                try
                {
                    std::lock_guard<hpx::spinlock_no_backoff> l(mtx_);
                    deferred_.push_back(deferred_block{p, size_class, event});
                    return;
                }
                catch (...)
                {
                    // could not keep it aside, wait for the reads instead
                }
                cudaError_t const error = cudaEventSynchronize(event);
                (void) cudaEventDestroy(event);
                if (error == cudaSuccess)
                {
                    deallocate(p, size_class);
                }

                // Otherwise the reads may not have finished: the block is
                // neither reused nor freed (it is lost, which takes an
                // allocation failure followed by a CUDA error).
            }

            // whether the limit comes from the complete configuration
            [[nodiscard]] bool is_final() const noexcept
            {
                return final_.load(std::memory_order_acquire);
            }

            // Before the runtime runs the configuration isn't complete, the
            // limit is the fallback: applies hpx.thrust.max_cached once it is
            // (again for each runtime, see unconfigure) and frees the blocks
            // above the limit. The limit is final once the cache is within
            // it.
            void configure()
            {
                std::size_t const max_cached =
                    max_cached_config(fallback_max_cached_);
                {
                    std::lock_guard<hpx::spinlock_no_backoff> l(mtx_);
                    max_cached_ = max_cached;
                }

                // one block at a time, the largest first: this allocates
                // nothing, memory may be short
                while (true)
                {
                    void* p = nullptr;
                    {
                        std::lock_guard<hpx::spinlock_no_backoff> l(mtx_);
                        if (cached_ <= max_cached_ || free_.empty())
                        {
                            final_.store(true, std::memory_order_release);
                            return;
                        }
                        auto it = std::prev(free_.end());
                        if (it->second.empty())
                        {
                            // left by a failed deallocate
                            free_.erase(it);
                            continue;
                        }
                        p = it->second.back();
                        it->second.pop_back();
                        cached_ -= it->first;
                        if (it->second.empty())
                        {
                            free_.erase(it);
                        }
                    }
                    free_block(p);
                }
            }

            // the next runtime may configure another limit
            void unconfigure() noexcept
            {
                final_.store(false, std::memory_order_release);
            }

            // frees the cached blocks, and those given back while device
            // work still reads them once it has finished
            void release() noexcept
            {
                std::map<std::size_t, std::vector<void*>> blocks;
                std::vector<deferred_block> deferred;
                {
                    std::lock_guard<hpx::spinlock_no_backoff> l(mtx_);
                    std::swap(blocks, free_);
                    std::swap(deferred, deferred_);
                    cached_ = 0;
                }
                for (auto& [size_class, ps] : blocks)
                {
                    for (void* p : ps)
                    {
                        free_block(p);
                    }
                }
                for (deferred_block const& b : deferred)
                {
                    (void) cudaEventSynchronize(b.event);
                    (void) cudaEventDestroy(b.event);
                    free_block(b.p);
                }
            }

        private:
            // a block given back while device work still reads it
            struct deferred_block
            {
                void* p;
                std::size_t size_class;
                cudaEvent_t event;
            };

            // The deferred blocks whose reads have finished go to the cache.
            // An error (e.g. of an earlier launch) doesn't show that the
            // reads have finished: those blocks stay deferred.
            void reclaim_deferred() noexcept
            {
                while (true)
                {
                    deferred_block b{};
                    {
                        std::lock_guard<hpx::spinlock_no_backoff> l(mtx_);
                        auto const it = std::find_if(deferred_.begin(),
                            deferred_.end(), [](deferred_block const& d) {
                                return cudaEventQuery(d.event) == cudaSuccess;
                            });
                        if (it == deferred_.end())
                        {
                            return;
                        }
                        b = *it;
                        *it = deferred_.back();
                        deferred_.pop_back();
                    }
                    (void) cudaEventDestroy(b.event);
                    deallocate(b.p, b.size_class);
                }
            }

            // nullptr if out of memory, throws for other errors
            void* allocate_new(std::size_t const size_class) const
            {
                void* p = nullptr;
                cudaError_t const error = kind_ == memory_kind::device ?
                    cudaMalloc(&p, size_class) :
                    cudaMallocHost(&p, size_class);
                if (error == cudaErrorMemoryAllocation)
                {
                    (void) cudaGetLastError();
                    return nullptr;
                }
                hpx::cuda::experimental::check_cuda_error(error);
                return p;
            }

            // errors are ignored, the runtime may be gone already
            void free_block(void* p) const noexcept
            {
                if (kind_ == memory_kind::pinned)
                {
                    (void) cudaFreeHost(p);
                    return;
                }

                // device memory is freed on its own device
                int current = -1;
                (void) cudaGetDevice(&current);
                if (current != device_)
                {
                    (void) cudaSetDevice(device_);
                }
                (void) cudaFree(p);
                if (current != device_ && current >= 0)
                {
                    (void) cudaSetDevice(current);
                }
            }

            memory_kind const kind_;
            int const device_;
            std::size_t const fallback_max_cached_;
            std::atomic<bool> final_;
            std::size_t max_cached_;

            // never suspends the calling HPX thread: the device memory is
            // allocated on the GPU current on the OS thread, which may be
            // another one once a suspended HPX thread resumes, and a
            // suspended thread could be interrupted in noexcept functions
            hpx::spinlock_no_backoff mtx_;
            std::map<std::size_t, std::vector<void*>> free_;
            std::size_t cached_ = 0;
            std::vector<deferred_block> deferred_;
        };

        int current_device()
        {
            int device = 0;
            hpx::cuda::experimental::check_cuda_error(cudaGetDevice(&device));
            return device;
        }

        // the memory the configuration entry hpx.thrust.memory selects
        memory_kind configured_memory_kind()
        {
            std::string const value =
                hpx::get_config_entry("hpx.thrust.memory", "device");
            if (value == "device")
            {
                return memory_kind::device;
            }
            if (value == "pinned")
            {
                return memory_kind::pinned;
            }
            HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                "hpx::thrust::default_memory_kind",
                "hpx.thrust.memory must be device or pinned, not '{}'", value);
        }

        // The default memory kind while the runtime runs (the configuration
        // is complete only then), else unknown_kind.
        constexpr int unknown_kind = -1;

        std::atomic<int>& cached_kind() noexcept
        {
            static std::atomic<int> kind{unknown_kind};
            return kind;
        }

        bool forget_configuration_when_stopped();

        // The pools of all devices, created on first use of a device (each
        // knows its device's memory size only then), and the blocks handed
        // out, by address. Never destroyed: memory may be deallocated by
        // the destructors of other static objects.
        class pools
        {
        public:
            static pools& instance()
            {
                static pools* p = create();
                return *p;
            }

            // the instance if it has been created, else nullptr
            static pools* existing() noexcept
            {
                return published().load(std::memory_order_acquire);
            }

            // The runtime has stopped: the next one may configure other
            // limits.
            // The blocks stay cached (within the old limits) until the pools
            // are configured again.
            void unconfigure() noexcept
            {
                all_final_.store(false, std::memory_order_release);

                std::lock_guard<hpx::spinlock_no_backoff> l(mtx_);
                for (auto& p : device_pools_)
                {
                    if (p)
                        p->unconfigure();
                }
                if (pinned_pool_)
                    pinned_pool_->unconfigure();
            }

            void* allocate(std::size_t const bytes, memory_kind const kind)
            {
                std::size_t const size_class = size_class_of(bytes);
                pool& p = pool_for(kind, current_device());
                void* block = p.allocate(size_class);
                try
                {
                    std::lock_guard<hpx::spinlock_no_backoff> l(mtx_);
                    blocks_.emplace(block, &p);
                }
                catch (...)
                {
                    p.deallocate(block, size_class);
                    throw;
                }
                return block;
            }

            // a staging copy queued on 'stream' reads 'block'
            void note_read(void const* block, cudaStream_t const stream)
            {
                {
                    std::lock_guard<hpx::spinlock_no_backoff> l(mtx_);
                    if (blocks_.find(const_cast<void*>(block)) == blocks_.end())
                    {
                        return;    // not ours, freeing it synchronizes
                    }
                }

                cudaEvent_t event = nullptr;
                hpx::cuda::experimental::check_cuda_error(
                    cudaEventCreateWithFlags(&event, cudaEventDisableTiming));
                cudaEvent_t replaced = nullptr;
                try
                {
                    // One event covers all reads of the block: an earlier
                    // read may have been queued on another stream, the event
                    // completes only after it (the new copy was queued
                    // before, it doesn't wait).
                    std::lock_guard<hpx::spinlock_no_backoff> l(mtx_);
                    cudaEvent_t& entry = reads_[block];
                    if (entry != nullptr)
                    {
                        hpx::cuda::experimental::check_cuda_error(
                            cudaStreamWaitEvent(stream, entry, 0));
                    }
                    hpx::cuda::experimental::check_cuda_error(
                        cudaEventRecord(event, stream));
                    replaced = std::exchange(entry, event);
                }
                catch (...)
                {
                    (void) cudaEventDestroy(event);
                    throw;
                }
                if (replaced != nullptr)
                {
                    (void) cudaEventDestroy(replaced);
                }
            }

            void deallocate(void* block, std::size_t const bytes) noexcept
            {
                // a block allocated before the runtime ran, returned once
                // it runs: the pool keeps it only within the configured limit
                if (!all_final_.load(std::memory_order_acquire) &&
                    hpx::is_running())
                {
                    try
                    {
                        configure_pools();
                    }
                    catch (...)
                    {
                        // an invalid configuration is reported by the next
                        // allocation
                    }
                }

                pool* p = nullptr;
                cudaEvent_t read = nullptr;
                {
                    std::lock_guard<hpx::spinlock_no_backoff> l(mtx_);
                    auto const it = blocks_.find(block);
                    if (it == blocks_.end())
                    {
                        return;    // not ours
                    }
                    p = it->second;
                    blocks_.erase(it);
                    if (auto const r = reads_.find(block); r != reads_.end())
                    {
                        read = r->second;
                        reads_.erase(r);
                    }
                }

                // a staging copy may still read it: reused only afterwards
                // (an error doesn't show that the read has finished)
                if (read != nullptr)
                {
                    if (cudaEventQuery(read) != cudaSuccess)
                    {
                        p->deallocate_after(block, size_class_of(bytes), read);
                        return;
                    }
                    (void) cudaEventDestroy(read);
                }
                p->deallocate(block, size_class_of(bytes));
            }

            // allocates nothing, it runs when memory is short (the number of
            // device pools is fixed when the pools are created)
            void release() noexcept
            {
                for (std::size_t i = 0; i != device_pools_.size(); ++i)
                {
                    pool* p = nullptr;
                    {
                        std::lock_guard<hpx::spinlock_no_backoff> l(mtx_);
                        p = device_pools_[i].get();
                    }
                    if (p != nullptr)
                    {
                        p->release();
                    }
                }

                pool* p = nullptr;
                {
                    std::lock_guard<hpx::spinlock_no_backoff> l(mtx_);
                    p = pinned_pool_.get();
                }
                if (p != nullptr)
                {
                    p->release();
                }
            }

        private:
            static std::atomic<pools*>& published() noexcept
            {
                static std::atomic<pools*> p{nullptr};
                return p;
            }

            static pools* create()
            {
                auto* p = new pools();
                published().store(p, std::memory_order_release);
                return p;
            }

            pools()
            {
                int count = 0;
                hpx::cuda::experimental::check_cuda_error(
                    cudaGetDeviceCount(&count));
                device_pools_.resize(static_cast<std::size_t>(count));
            }

            // Pools keep at most hpx.thrust.max_cached bytes each, by default
            // an eighth of the device's memory, at least 256 MiB of pinned
            // memory (staging buffers of the serialization). Returns the
            // default.
            static std::size_t default_max_cached(memory_kind const kind)
            {
                std::size_t free = 0;
                std::size_t total = 0;
                hpx::cuda::experimental::check_cuda_error(
                    cudaMemGetInfo(&free, &total));
                std::size_t max_cached = total / 8;
                if (kind == memory_kind::pinned &&
                    max_cached < (std::size_t(256) << 20))
                {
                    max_cached = std::size_t(256) << 20;
                }
                return max_cached;
            }

            // applies the configuration to the pools created before the
            // runtime ran (or used by an earlier runtime)
            void configure_pools()
            {
                // configured once for each runtime (else again on each use)
                bool const final = forget_configuration_when_stopped();

                std::vector<pool*> pending;
                {
                    std::lock_guard<hpx::spinlock_no_backoff> l(mtx_);
                    for (auto& p : device_pools_)
                    {
                        if (p && !p->is_final())
                            pending.push_back(p.get());
                    }
                    if (pinned_pool_ && !pinned_pool_->is_final())
                        pending.push_back(pinned_pool_.get());
                }
                for (pool* p : pending)
                {
                    p->configure();
                }
                all_final_.store(final, std::memory_order_release);
            }

            pool& pool_for(memory_kind const kind, int const device)
            {
                if (!all_final_.load(std::memory_order_acquire) &&
                    hpx::is_running())
                {
                    configure_pools();
                }

                std::unique_ptr<pool>& p = kind == memory_kind::pinned ?
                    pinned_pool_ :
                    device_pools_.at(static_cast<std::size_t>(device));
                {
                    std::lock_guard<hpx::spinlock_no_backoff> l(mtx_);
                    if (p)
                        return *p;
                }

                auto new_pool = std::make_unique<pool>(
                    kind, device, default_max_cached(kind));

                pool* result = nullptr;
                {
                    std::lock_guard<hpx::spinlock_no_backoff> l(mtx_);
                    if (!p)
                        p = HPX_MOVE(new_pool);
                    result = p.get();
                }

                // created just before the runtime ran, after the pools
                // were configured
                if (!result->is_final() && hpx::is_running())
                {
                    result->configure();
                }
                return *result;
            }

            std::atomic<bool> all_final_ = false;

            // never suspends the calling HPX thread: the device memory is
            // allocated on the GPU current on the OS thread, which may be
            // another one once a suspended HPX thread resumes, and a
            // suspended thread could be interrupted in noexcept functions
            hpx::spinlock_no_backoff mtx_;
            std::vector<std::unique_ptr<pool>> device_pools_;
            std::unique_ptr<pool> pinned_pool_;
            std::unordered_map<void*, pool*> blocks_;

            // the last staging copy reading a block, by address
            std::unordered_map<void const*, cudaEvent_t> reads_;
        };
        // The configuration cached for a runtime (the default memory kind,
        // the limits of the pools) is forgotten when it ends: a runtime
        // started later in the same process may configure others. Returns
        // whether it will be: only then may it be cached, i.e. while the
        // runtime is running (not yet while it starts, when the configuration
        // may not be complete, nor once it stops, when it has been forgotten
        // already).
        bool forget_configuration_when_stopped()
        {
            static std::atomic<bool> registered(false);
            if (!hpx::is_running())
            {
                return false;
            }
            if (registered.exchange(true, std::memory_order_acq_rel))
            {
                return true;
            }
            try
            {
                // called once the runtime has stopped running
                bool const registered_now = hpx::register_on_exit([]() {
                    cached_kind().store(
                        unknown_kind, std::memory_order_release);
                    if (pools* p = pools::existing())
                    {
                        p->unconfigure();
                    }
                    registered.store(false, std::memory_order_release);
                });
                if (registered_now)
                {
                    return true;
                }
            }
            // NOLINTNEXTLINE(bugprone-empty-catch)
            catch (...)
            {
            }
            registered.store(false, std::memory_order_release);
            return false;
        }
    }    // namespace

    // reads the configuration only, the GPU isn't touched
    memory_kind default_memory_kind()
    {
        if (int const value = cached_kind().load(std::memory_order_acquire);
            value != unknown_kind)
        {
            return static_cast<memory_kind>(value);
        }

        // the configuration is complete only once the runtime runs, it is
        // cached for the rest of the runtime
        memory_kind const kind = configured_memory_kind();
        if (forget_configuration_when_stopped())
        {
            cached_kind().store(
                static_cast<int>(kind), std::memory_order_release);
        }
        return kind;
    }

    void release_cached_memory() noexcept
    {
        // without pools nothing is cached (and the GPU runtime isn't touched)
        if (pools* p = pools::existing())
        {
            p->release();
        }
    }
}    // namespace hpx::thrust

namespace hpx::thrust::detail {

    void* pool_allocate(std::size_t const bytes)
    {
        return pool_allocate(bytes, default_memory_kind());
    }

    void* pool_allocate(std::size_t const bytes, memory_kind const kind)
    {
        return pools::instance().allocate(bytes, kind);
    }

    void pool_deallocate(void* p, std::size_t const bytes) noexcept
    {
        if (p != nullptr)
        {
            pools::instance().deallocate(p, bytes);
        }
    }

    void pool_note_read(void const* p, void* stream)
    {
        // without pools no memory comes from them
        if (pools* existing = pools::existing(); existing != nullptr)
        {
            existing->note_read(p, static_cast<cudaStream_t>(stream));
        }
    }

    bool is_pinned_host_memory(void const* p) noexcept
    {
        cudaPointerAttributes attributes{};
        if (cudaPointerGetAttributes(&attributes, p) != cudaSuccess)
        {
            // older runtimes report unregistered host memory as an error
            (void) cudaGetLastError();
            return false;
        }
        return attributes.type == cudaMemoryTypeHost;
    }
}    // namespace hpx::thrust::detail
