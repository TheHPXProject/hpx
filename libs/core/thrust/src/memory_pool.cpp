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
            try
            {
                return std::stoull(value);
            }
            catch (std::exception const&)
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::thrust::max_cached_config",
                    "hpx.thrust.max_cached must be a number of bytes, not "
                    "'{}'",
                    value);
            }
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
              , max_cached_(max_cached_config(fallback_max_cached))
              , final_(hpx::get_runtime_ptr() != nullptr)
            {
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
                {
                    std::lock_guard<hpx::spinlock> l(mtx_);
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
                    std::lock_guard<hpx::spinlock> l(mtx_);
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

            // whether the limit comes from the complete configuration
            [[nodiscard]] bool is_final() const noexcept
            {
                return final_.load(std::memory_order_acquire);
            }

            // Before the runtime exists the configuration isn't complete, the
            // limit is the fallback: applies hpx.thrust.max_cached once it is
            // and frees the blocks above the limit.
            void configure()
            {
                std::size_t const max_cached =
                    max_cached_config(fallback_max_cached_);

                std::vector<void*> excess;
                {
                    std::lock_guard<hpx::spinlock> l(mtx_);
                    max_cached_ = max_cached;
                    while (cached_ > max_cached_ && !free_.empty())
                    {
                        // the largest blocks first
                        auto it = std::prev(free_.end());
                        if (it->second.empty())
                        {
                            free_.erase(it);
                            continue;
                        }
                        excess.push_back(it->second.back());
                        it->second.pop_back();
                        cached_ -= it->first;
                        if (it->second.empty())
                        {
                            free_.erase(it);
                        }
                    }
                    final_.store(true, std::memory_order_release);
                }
                for (void* p : excess)
                {
                    free_block(p);
                }
            }

            // frees the cached blocks
            void release() noexcept
            {
                std::map<std::size_t, std::vector<void*>> blocks;
                {
                    std::lock_guard<hpx::spinlock> l(mtx_);
                    std::swap(blocks, free_);
                    cached_ = 0;
                }
                for (auto& [size_class, ps] : blocks)
                {
                    for (void* p : ps)
                    {
                        free_block(p);
                    }
                }
            }

        private:
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
            std::size_t max_cached_;
            std::atomic<bool> final_;

            hpx::spinlock mtx_;
            std::map<std::size_t, std::vector<void*>> free_;
            std::size_t cached_ = 0;
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

        // The pools of all devices, created on first use of a device (each
        // knows its device's memory size only then), and the blocks handed
        // out, by address. Never destroyed: memory may be deallocated by
        // the destructors of other static objects.
        class pools
        {
            static constexpr int unknown_kind = -1;

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

            memory_kind default_kind()
            {
                if (int const value = kind_.load(std::memory_order_acquire);
                    value != unknown_kind)
                {
                    return static_cast<memory_kind>(value);
                }

                // the configuration is complete only once the runtime exists
                memory_kind const kind = configured_memory_kind();
                if (hpx::get_runtime_ptr() != nullptr)
                {
                    kind_.store(
                        static_cast<int>(kind), std::memory_order_release);
                }
                return kind;
            }

            void* allocate(std::size_t const bytes, memory_kind const kind)
            {
                std::size_t const size_class = size_class_of(bytes);
                pool& p = pool_for(kind, current_device());
                void* block = p.allocate(size_class);
                try
                {
                    std::lock_guard<hpx::spinlock> l(mtx_);
                    blocks_.emplace(block, &p);
                }
                catch (...)
                {
                    p.deallocate(block, size_class);
                    throw;
                }
                return block;
            }

            void deallocate(void* block, std::size_t const bytes) noexcept
            {
                pool* p = nullptr;
                {
                    std::lock_guard<hpx::spinlock> l(mtx_);
                    auto const it = blocks_.find(block);
                    if (it == blocks_.end())
                    {
                        return;    // not ours
                    }
                    p = it->second;
                    blocks_.erase(it);
                }
                p->deallocate(block, size_class_of(bytes));
            }

            void release() noexcept
            {
                std::vector<pool*> all;
                try
                {
                    std::lock_guard<hpx::spinlock> l(mtx_);
                    for (auto& p : device_pools_)
                    {
                        if (p)
                            all.push_back(p.get());
                    }
                    if (pinned_pool_)
                        all.push_back(pinned_pool_.get());
                }
                catch (...)
                {
                    return;
                }
                for (pool* p : all)
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
            // runtime existed
            void configure_pools()
            {
                std::vector<pool*> pending;
                {
                    std::lock_guard<hpx::spinlock> l(mtx_);
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
                all_final_.store(true, std::memory_order_release);
            }

            pool& pool_for(memory_kind const kind, int const device)
            {
                if (!all_final_.load(std::memory_order_acquire) &&
                    hpx::get_runtime_ptr() != nullptr)
                {
                    configure_pools();
                }

                std::unique_ptr<pool>& p = kind == memory_kind::pinned ?
                    pinned_pool_ :
                    device_pools_.at(static_cast<std::size_t>(device));
                {
                    std::lock_guard<hpx::spinlock> l(mtx_);
                    if (p)
                        return *p;
                }

                auto new_pool = std::make_unique<pool>(
                    kind, device, default_max_cached(kind));

                pool* result = nullptr;
                {
                    std::lock_guard<hpx::spinlock> l(mtx_);
                    if (!p)
                        p = HPX_MOVE(new_pool);
                    result = p.get();
                }

                // created just before the runtime existed, after the pools
                // were configured
                if (!result->is_final() && hpx::get_runtime_ptr() != nullptr)
                {
                    result->configure();
                }
                return *result;
            }

            std::atomic<int> kind_ = unknown_kind;
            std::atomic<bool> all_final_ = false;

            hpx::spinlock mtx_;
            std::vector<std::unique_ptr<pool>> device_pools_;
            std::unique_ptr<pool> pinned_pool_;
            std::unordered_map<void*, pool*> blocks_;
        };
    }    // namespace

    memory_kind default_memory_kind()
    {
        return pools::instance().default_kind();
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
