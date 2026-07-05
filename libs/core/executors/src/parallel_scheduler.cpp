// Copyright (c) 2026 Sai Charan Arvapally
//
// SPDX-License-Identifier: BSL-1.0
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#include <hpx/assert.hpp>
#include <hpx/errors/try_catch_exception_ptr.hpp>
#include <hpx/executors/parallel_scheduler.hpp>
#include <hpx/executors/parallel_scheduler_backend.hpp>
#include <hpx/executors/thread_pool_scheduler.hpp>
#include <hpx/executors/thread_pool_scheduler_bulk.hpp>
#include <hpx/threading_base/thread_pool_base.hpp>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <mutex>
#include <ostream>
#include <span>
#include <utility>

namespace hpx::execution::experimental {

    namespace detail {

        // Default HPX backend: wraps the existing thread_pool_policy_scheduler.
        // This is the backend returned by query_parallel_scheduler_backend()
        // unless the user provides a replacement at runtime.
        class hpx_parallel_scheduler_backend final
          : public parallel_scheduler_backend
        {
        public:
            explicit hpx_parallel_scheduler_backend(
                thread_pool_policy_scheduler<hpx::launch> sched)
              : scheduler_(sched)
              , pu_mask_(hpx::execution::experimental::detail::full_mask(
                    hpx::execution::experimental::get_first_core(scheduler_),
                    hpx::execution::experimental::processing_units_count(
                        hpx::execution::experimental::null_parameters,
                        scheduler_, hpx::chrono::null_duration, 0)))
            {
            }

            void schedule(parallel_scheduler_receiver_proxy& proxy,
                std::span<std::byte>) noexcept override
            {
                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        scheduler_.execute(
                            [&proxy]() mutable { proxy.set_value(); });
                    },
                    [&](std::exception_ptr ep) {
                        proxy.set_error(HPX_MOVE(ep));
                    });
            }

            void schedule_bulk_chunked(std::size_t count,
                parallel_scheduler_bulk_item_receiver_proxy& proxy,
                std::span<std::byte>) noexcept override
            {
                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        if (count == 0)
                        {
                            proxy.set_value();
                            return;
                        }

                        auto const num_threads = static_cast<std::uint32_t>(
                            hpx::execution::experimental::
                                processing_units_count(
                                    hpx::execution::experimental::
                                        null_parameters,
                                    scheduler_, hpx::chrono::null_duration, 0));
                        auto const chunk_size = static_cast<std::size_t>(
                            hpx::execution::experimental::detail::
                                get_bulk_scheduler_chunk_size_chunked(
                                    num_threads, count));
                        auto const n_chunks =
                            (count + chunk_size - 1) / chunk_size;

                        auto sync = std::make_shared<bulk_sync_state>(n_chunks);
                        std::size_t chunks_posted = 0;

                        for (std::size_t c = 0; c < n_chunks; ++c)
                        {
                            auto const begin = c * chunk_size;
                            auto const end =
                                (std::min) (begin + chunk_size, count);

                            bool post_ok = true;
                            hpx::detail::try_catch_exception_ptr(
                                [&]() {
                                    // Each task owns a copy of the shared_ptr,
                                    // keeping sync alive until the last task
                                    // finishes (i.e., until set_value/set_error
                                    // is called).
                                    scheduler_.execute(
                                        [&proxy, sync, begin, end]() noexcept {
                                            proxy.execute(begin, end);
                                            if (sync->decrement())
                                                sync->signal(proxy);
                                        });
                                    ++chunks_posted;
                                },
                                [&](std::exception_ptr ep) {
                                    post_ok = false;
                                    sync->try_set_error(HPX_MOVE(ep));
                                });

                            if (!post_ok)
                                break;
                        }

                        // Retire any chunks that were never posted so the
                        // countdown can reach zero even when posting failed.
                        auto const not_posted = n_chunks - chunks_posted;
                        if (not_posted > 0 && sync->decrement(not_posted))
                            sync->signal(proxy);
                    },
                    [&](std::exception_ptr ep) {
                        // Setup (make_shared / chunk size computation) threw;
                        // no tasks have been posted yet.
                        proxy.set_error(HPX_MOVE(ep));
                    });
            }

            void schedule_bulk_unchunked(std::size_t count,
                parallel_scheduler_bulk_item_receiver_proxy& proxy,
                std::span<std::byte>) noexcept override
            {
                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        if (count == 0)
                        {
                            proxy.set_value();
                            return;
                        }

                        auto const num_threads = static_cast<std::uint32_t>(
                            hpx::execution::experimental::
                                processing_units_count(
                                    hpx::execution::experimental::
                                        null_parameters,
                                    scheduler_, hpx::chrono::null_duration, 0));
                        // Reuse the chunked helper: ceil(count / num_threads)
                        // elements per task, giving roughly one task per thread.
                        auto const chunk_size = static_cast<std::size_t>(
                            hpx::execution::experimental::detail::
                                get_bulk_scheduler_chunk_size_chunked(
                                    num_threads, count));
                        auto const n_chunks =
                            (count + chunk_size - 1) / chunk_size;

                        auto sync = std::make_shared<bulk_sync_state>(n_chunks);
                        std::size_t chunks_posted = 0;

                        for (std::size_t c = 0; c < n_chunks; ++c)
                        {
                            auto const begin = c * chunk_size;
                            auto const end =
                                (std::min) (begin + chunk_size, count);

                            bool post_ok = true;
                            hpx::detail::try_catch_exception_ptr(
                                [&]() {
                                    scheduler_.execute(
                                        [&proxy, sync, begin, end]() noexcept {
                                            // Call execute(i, i+1) for every
                                            // element in this task's slice.
                                            for (std::size_t i = begin; i < end;
                                                ++i)
                                                proxy.execute(i, i + 1);
                                            if (sync->decrement())
                                                sync->signal(proxy);
                                        });
                                    ++chunks_posted;
                                },
                                [&](std::exception_ptr ep) {
                                    post_ok = false;
                                    sync->try_set_error(HPX_MOVE(ep));
                                });

                            if (!post_ok)
                                break;
                        }

                        auto const not_posted = n_chunks - chunks_posted;
                        if (not_posted > 0 && sync->decrement(not_posted))
                            sync->signal(proxy);
                    },
                    [&](std::exception_ptr ep) {
                        proxy.set_error(HPX_MOVE(ep));
                    });
            }

            bool equal_to(
                parallel_scheduler_backend const& other) const noexcept override
            {
                auto const* p =
                    dynamic_cast<hpx_parallel_scheduler_backend const*>(&other);
                return p != nullptr && p->scheduler_ == scheduler_;
            }

            thread_pool_policy_scheduler<hpx::launch> const*
            get_underlying_scheduler() const noexcept override
            {
                return &scheduler_;
            }

            hpx::threads::mask_type const* get_pu_mask() const noexcept override
            {
                return &pu_mask_;
            }

        private:
            thread_pool_policy_scheduler<hpx::launch> scheduler_;
            hpx::threads::mask_type pu_mask_;

            // Shared synchronization state for a single parallel bulk dispatch.
            // One instance is created per schedule_bulk_* call and shared among
            // all chunk tasks via shared_ptr.
            //
            // Lifetime guarantee: the shared_ptr keeps this object alive until
            // the last task drops its copy, which only happens after one of the
            // completion signals (set_value / set_error) has been called on the
            // proxy. The proxy itself is guaranteed alive until that point by
            // the P2079R10 precondition on schedule_bulk_chunked/unchunked.
            struct bulk_sync_state
            {
                // Counts down from n_chunks to 0. The task that observes 0 is
                // responsible for calling the completion signal on the proxy.
                std::atomic<std::size_t> remaining;

                // Set to true by the first task that encounters an error.
                // Written before remaining reaches 0, so the acq_rel fence on
                // remaining guarantees visibility for the completing task.
                std::atomic<bool> has_error{false};

                // Stores the first error. Protected by the has_error CAS:
                // only one thread writes it, and it is read after acquiring
                // has_error with memory_order_acquire.
                std::exception_ptr first_error;

                explicit bulk_sync_state(std::size_t n) noexcept
                  : remaining(n)
                {
                }

                // Record ep as the first error (thread-safe; first caller wins).
                void try_set_error(std::exception_ptr ep) noexcept
                {
                    bool expected = false;
                    if (has_error.compare_exchange_strong(
                            expected, true, std::memory_order_acq_rel))
                    {
                        first_error = HPX_MOVE(ep);
                    }
                }

                // Subtract n from remaining. Returns true iff remaining was
                // exactly n before the subtraction (i.e., it is now 0).
                bool decrement(std::size_t n = 1) noexcept
                {
                    return remaining.fetch_sub(n, std::memory_order_acq_rel) ==
                        n;
                }

                // Call set_value or set_error on proxy based on error state.
                // Must only be called by the single task for which decrement()
                // returned true (i.e., the task that made remaining reach 0).
                void signal(
                    parallel_scheduler_bulk_item_receiver_proxy& proxy) noexcept
                {
                    if (has_error.load(std::memory_order_acquire))
                        proxy.set_error(HPX_MOVE(first_error));
                    else
                        proxy.set_value();
                }
            };
        };

        // Singleton-like shared thread pool for parallel_scheduler
        static hpx::threads::thread_pool_base* get_default_parallel_pool()
        {
            // clang-format off
            static hpx::threads::thread_pool_base* default_pool =
                hpx::threads::detail::get_self_or_default_pool();
            // clang-format on
            return default_pool;
        }

        // Default factory creates the HPX backend
        static std::shared_ptr<parallel_scheduler_backend>
        default_parallel_scheduler_backend_factory()
        {
            auto pool = get_default_parallel_pool();
            if (!pool)
            {
                std::terminate();
            }
            return std::make_shared<hpx_parallel_scheduler_backend>(
                thread_pool_policy_scheduler<hpx::launch>(
                    pool, hpx::launch::async));
        }

        // Mutex protecting the live backend instance.
        static std::mutex& get_backend_mutex() noexcept
        {
            static std::mutex mtx;
            return mtx;
        }

        // The live backend instance. nullptr until first query.
        // Protected by get_backend_mutex().
        static std::shared_ptr<parallel_scheduler_backend>&
        get_backend_storage() noexcept
        {
            static std::shared_ptr<parallel_scheduler_backend> backend;
            return backend;
        }

        // Storage for the current factory (only used to create the first
        // backend, or after set_parallel_scheduler_backend() clears the
        // current one).
        static parallel_scheduler_backend_factory_t&
        get_backend_factory_storage() noexcept
        {
            static parallel_scheduler_backend_factory_t factory =
                &default_parallel_scheduler_backend_factory;
            return factory;
        }

    }    // namespace detail

    std::shared_ptr<parallel_scheduler_backend>
    query_parallel_scheduler_backend()
    {
        std::lock_guard<std::mutex> lock(detail::get_backend_mutex());
        auto& storage = detail::get_backend_storage();
        if (!storage)
        {
            storage = detail::get_backend_factory_storage()();
        }
        return storage;
    }

    parallel_scheduler_backend_factory_t set_parallel_scheduler_backend_factory(
        parallel_scheduler_backend_factory_t new_factory) noexcept
    {
        std::lock_guard<std::mutex> lock(detail::get_backend_mutex());
        auto& storage = detail::get_backend_factory_storage();
        auto old = storage;
        storage = new_factory;
        return old;
    }

    void set_parallel_scheduler_backend(
        std::shared_ptr<parallel_scheduler_backend> new_backend)
    {
        std::lock_guard<std::mutex> lock(detail::get_backend_mutex());
        detail::get_backend_storage() = HPX_MOVE(new_backend);
    }

    parallel_scheduler get_parallel_scheduler()
    {
        auto backend = query_parallel_scheduler_backend();
        if (!backend)
        {
            // As per P2079R10, terminate if backend is unavailable.
            std::terminate();
        }
        return parallel_scheduler(HPX_MOVE(backend));
    }

    std::ostream& operator<<(std::ostream& os, parallel_scheduler const&)
    {
        return os << "parallel_scheduler";
    }

}    // namespace hpx::execution::experimental
