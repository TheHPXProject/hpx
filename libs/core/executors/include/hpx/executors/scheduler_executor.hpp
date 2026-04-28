//  Copyright (c) 2021-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/executors/scheduler_executor.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/modules/type_support.hpp>

#if defined(HPX_HAVE_STDEXEC)
#include <hpx/executors/detail/index_queue_spawning.hpp>
#include <hpx/executors/parallel_scheduler.hpp>
#endif

#include <cstddef>
#include <exception>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::execution::experimental {

#if defined(HPX_HAVE_STDEXEC)
    namespace detail {

        // Trait to detect schedulers that expose a thread pool backend,
        // enabling direct dispatch via index_queue_bulk_sync_execute
        // instead of the slower sender/receiver pipeline.
        template <typename Scheduler>
        struct has_thread_pool_backend : std::false_type
        {
        };

        template <typename Policy>
        struct has_thread_pool_backend<thread_pool_policy_scheduler<Policy>>
          : std::true_type
        {
        };

        // parallel_scheduler wraps thread_pool_policy_scheduler; use the same
        // index_queue fast path with thread_pool_params<parallel_scheduler>
        // so pu_mask() can return the cached mask from get_pu_mask().
        template <>
        struct has_thread_pool_backend<parallel_scheduler> : std::true_type
        {
        };

        // Helper to extract thread pool parameters from a scheduler
        template <typename Scheduler>
        struct thread_pool_params;    // primary: not defined

        template <>
        struct thread_pool_params<parallel_scheduler>
        {
            static auto* pool(parallel_scheduler const& sched)
            {
                return sched.get_underlying_scheduler()->get_thread_pool();
            }
            static std::size_t first_core(parallel_scheduler const& sched)
            {
                return hpx::execution::experimental::get_first_core(
                    *sched.get_underlying_scheduler());
            }
            static std::size_t num_cores(parallel_scheduler const& sched)
            {
                return hpx::execution::experimental::processing_units_count(
                    hpx::execution::experimental::null_parameters,
                    *sched.get_underlying_scheduler(),
                    hpx::chrono::null_duration, 0);
            }
            static auto const& policy(parallel_scheduler const& sched)
            {
                return sched.get_underlying_scheduler()->policy();
            }
            static hpx::threads::mask_type pu_mask(
                parallel_scheduler const& sched)
            {
                return *sched.get_pu_mask();
            }
        };

        template <typename Policy>
        struct thread_pool_params<thread_pool_policy_scheduler<Policy>>
        {
            static auto* pool(thread_pool_policy_scheduler<Policy> const& sched)
            {
                return sched.get_thread_pool();
            }
            static std::size_t first_core(
                thread_pool_policy_scheduler<Policy> const& sched)
            {
                return hpx::execution::experimental::get_first_core(sched);
            }
            static std::size_t num_cores(
                thread_pool_policy_scheduler<Policy> const& sched)
            {
                return hpx::execution::experimental::processing_units_count(
                    hpx::execution::experimental::null_parameters, sched,
                    hpx::chrono::null_duration, 0);
            }
            static Policy const& policy(
                thread_pool_policy_scheduler<Policy> const& sched)
            {
                return sched.policy();
            }
            static auto pu_mask(
                thread_pool_policy_scheduler<Policy> const& sched)
            {
                return hpx::execution::experimental::get_processing_units_mask(
                    sched);
            }
        };
    }    // namespace detail
#endif

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename F, typename... Ts>
        auto captured_args_then(F&& f, Ts&&... ts)
        {
            return [f = HPX_FORWARD(F, f), ... ts = HPX_FORWARD(Ts, ts)](
                       auto i, auto&& predecessor, auto& v) mutable {
                v[i] = HPX_INVOKE(HPX_FORWARD(F, f), i,
                    HPX_FORWARD(decltype(predecessor), predecessor),
                    HPX_FORWARD(Ts, ts)...);
            };
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // A scheduler_executor wraps any P2300 scheduler and implements the
    // executor functionalities for those.
    HPX_CXX_CORE_EXPORT template <typename BaseScheduler>
    struct scheduler_executor
    {
        static_assert(hpx::execution::experimental::is_scheduler_v<
                          std::decay_t<BaseScheduler>>,
            "scheduler_executor requires a scheduler");

        constexpr scheduler_executor() = default;

        template <typename Scheduler,
            typename Enable = std::enable_if_t<
                hpx::execution::experimental::is_scheduler_v<Scheduler> &&
                !std::is_same_v<std::decay_t<Scheduler>, scheduler_executor>>>
        constexpr explicit scheduler_executor(Scheduler&& sched)
          : sched_(HPX_FORWARD(Scheduler, sched))
        {
        }

        constexpr scheduler_executor(scheduler_executor&&) = default;
        constexpr scheduler_executor& operator=(scheduler_executor&&) = default;
        constexpr scheduler_executor(scheduler_executor const&) = default;
        constexpr scheduler_executor& operator=(
            scheduler_executor const&) = default;

        /// \cond NOINTERNAL
        constexpr bool operator==(scheduler_executor const& rhs) const noexcept
        {
            return sched_ == rhs.sched_;
        }

        constexpr bool operator!=(scheduler_executor const& rhs) const noexcept
        {
            return sched_ != rhs.sched_;
        }

        constexpr auto const& context() const noexcept
        {
            return *this;
        }

        constexpr BaseScheduler const& sched() const noexcept
        {
            return sched_;
        }

        // Associate the parallel_execution_tag executor tag type as a default
        // with this executor.
        using execution_category = parallel_execution_tag;

        // Associate the default_parameters executor parameters type as a default
        // with this executor.
        using executor_parameters_type = default_parameters;

        template <typename T, typename... Ts>
        using future_type = hpx::future<T>;

    private:
        template <executor_parameters Parameters>
        friend auto tag_invoke(
            hpx::execution::experimental::processing_units_count_t tag,
            Parameters&& params, scheduler_executor const& exec,
            hpx::chrono::steady_duration const& duration =
                hpx::chrono::null_duration,
            std::size_t num_cores = 0)
            -> decltype(std::declval<
                hpx::execution::experimental::processing_units_count_t>()(
                std::declval<Parameters>(), std::declval<BaseScheduler>(),
                std::declval<hpx::chrono::steady_duration>(), 0))
        {
            return tag(HPX_FORWARD(Parameters, params), exec.sched_, duration,
                num_cores);
        }

        // NonBlockingOneWayExecutor interface
        template <typename F, typename... Ts>
        friend decltype(auto) tag_invoke(hpx::parallel::execution::post_t,
            scheduler_executor const& exec, F&& f, Ts&&... ts)
        {
            start_detached(then(schedule(exec.sched_),
                hpx::util::deferred_call(
                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...)));
        }

        // OneWayExecutor interface
        template <typename F, typename... Ts>
        friend auto tag_invoke(hpx::parallel::execution::sync_execute_t,
            scheduler_executor const& exec, F&& f, Ts&&... ts)
        {
            using result_type =
                hpx::util::detail::invoke_deferred_result_t<F, Ts...>;

            return hpx::util::void_guard<result_type>(),
                   // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                   *hpx::this_thread::experimental::sync_wait(
                       then(schedule(exec.sched_),
                           hpx::util::deferred_call(
                               HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...)));
        }

        // TwoWayExecutor interface
        template <typename F, typename... Ts>
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::async_execute_t,
            scheduler_executor const& exec, F&& f, Ts&&... ts)
        {
            return make_future(then(schedule(exec.sched_),
                hpx::util::deferred_call(
                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...)));
        }

        template <typename F, typename Future, typename... Ts>
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::then_execute_t,
            scheduler_executor const& exec, F&& f, Future&& predecessor,
            Ts&&... ts)
        {
            auto&& predecessor_transfer_sched = transfer(
                keep_future(HPX_FORWARD(Future, predecessor)), exec.sched_);

            return make_future(then(HPX_MOVE(predecessor_transfer_sched),
                hpx::bind_back(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...)));
        }

        // BulkTwoWayExecutor interface
        template <typename F, typename S, typename... Ts>
            requires(!std::is_integral_v<S>)
        friend auto tag_invoke(hpx::parallel::execution::bulk_async_execute_t,
            scheduler_executor const& exec, F&& f, S const& shape, Ts&&... ts)
        {
            using shape_element =
                typename hpx::traits::range_traits<S>::value_type;
            using result_type = hpx::util::detail::invoke_deferred_result_t<F,
                shape_element, Ts...>;

            if constexpr (std::is_void_v<result_type>)
            {
#if defined(HPX_HAVE_STDEXEC)
                // Fast path: direct thread pool dispatch
                if constexpr (detail::has_thread_pool_backend<
                                  std::decay_t<BaseScheduler>>::value)
                {
                    using params_type =
                        detail::thread_pool_params<std::decay_t<BaseScheduler>>;
                    auto* pool = params_type::pool(exec.sched_);
                    auto first_core = params_type::first_core(exec.sched_);
                    auto num_cores = params_type::num_cores(exec.sched_);
                    auto const& policy = params_type::policy(exec.sched_);
                    auto mask = params_type::pu_mask(exec.sched_);

                    return hpx::parallel::execution::detail::
                        index_queue_bulk_async_execute(pool, first_core,
                            num_cores, policy, HPX_FORWARD(F, f), shape, mask,
                            HPX_FORWARD(Ts, ts)...);
                }
                else if constexpr (requires {
                                       exec.sched_.get_underlying_scheduler();
                                   })
                {
                    using underlying_type = std::decay_t<
                        decltype(exec.sched_.get_underlying_scheduler())>;
                    if constexpr (detail::has_thread_pool_backend<
                                      underlying_type>::value)
                    {
                        using params_type =
                            detail::thread_pool_params<underlying_type>;
                        auto const& underlying =
                            exec.sched_.get_underlying_scheduler();
                        auto* pool = params_type::pool(underlying);
                        auto first_core = params_type::first_core(underlying);
                        auto num_cores = params_type::num_cores(underlying);
                        auto const& policy = params_type::policy(underlying);
                        auto mask = params_type::pu_mask(underlying);

                        return hpx::parallel::execution::detail::
                            index_queue_bulk_async_execute(pool, first_core,
                                num_cores, policy, HPX_FORWARD(F, f), shape,
                                mask, HPX_FORWARD(Ts, ts)...);
                    }
                    else
                    {
                        using size_type = decltype(hpx::util::size(shape));
                        size_type const n = hpx::util::size(shape);
                        return make_future(bulk(schedule(exec.sched_), par, n,
                            [shape, f = HPX_FORWARD(F, f),
                                ... args = HPX_FORWARD(Ts, ts)](
                                size_type i) mutable {
                                auto it = hpx::util::begin(shape);
                                std::advance(it, i);
                                HPX_INVOKE(f, *it, args...);
                            }));
                    }
                }
                else
                {
                    using size_type = decltype(hpx::util::size(shape));
                    size_type const n = hpx::util::size(shape);
                    return make_future(bulk(schedule(exec.sched_), par, n,
                        [shape, f = HPX_FORWARD(F, f),
                            ... args = HPX_FORWARD(Ts, ts)](
                            size_type i) mutable {
                            auto it = hpx::util::begin(shape);
                            std::advance(it, i);
                            HPX_INVOKE(f, *it, args...);
                        }));
                }
#else
                return make_future(bulk(schedule(exec.sched_), shape,
                    hpx::bind_back(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...)));
#endif
            }
            else
            {
                using promise_vector_type =
                    std::vector<hpx::promise<result_type>>;
                using result_vector_type =
                    std::vector<hpx::future<result_type>>;

                using size_type = decltype(hpx::util::size(shape));
                size_type const n = hpx::util::size(shape);

                promise_vector_type promises(n);
                result_vector_type results;
                results.reserve(n);

                for (size_type i = 0; i < n; ++i)
                {
                    results.emplace_back(promises[i].get_future());
                }

                auto f_helper = [](size_type const i,
                                    promise_vector_type& promises, F& f,
                                    S const& shape, Ts&... ts) {
                    hpx::detail::try_catch_exception_ptr(
                        [&]() mutable {
                            auto it = hpx::util::begin(shape);
                            std::advance(it, i);
                            promises[i].set_value(HPX_INVOKE(f, *it, ts...));
                        },
                        [&](std::exception_ptr&& ep) {
                            promises[i].set_exception(HPX_MOVE(ep));
                        });
                };

#if defined(HPX_HAVE_STDEXEC)
                start_detached(
                    bulk(transfer_just(exec.sched_, HPX_MOVE(promises),
                             HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...),
                        par, n, HPX_MOVE(f_helper)));
#else
                start_detached(
                    bulk(transfer_just(exec.sched_, HPX_MOVE(promises),
                             HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...),
                        n, HPX_MOVE(f_helper)));
#endif

                return results;
            }
        }

        template <typename F, typename S, typename... Ts>
            requires(!std::is_integral_v<S>)
        friend auto tag_invoke(hpx::parallel::execution::bulk_sync_execute_t,
            scheduler_executor const& exec, F&& f, S const& shape, Ts&&... ts)
        {
            using shape_element =
                typename hpx::traits::range_traits<S>::value_type;
            using result_type = hpx::util::detail::invoke_deferred_result_t<F,
                shape_element, Ts...>;

#if defined(HPX_HAVE_STDEXEC)
            // Fast path: if the scheduler (or its underlying scheduler)
            // is backed by a thread pool, bypass the sender/receiver
            // pipeline and call index_queue_bulk_sync_execute directly.
            // This matches the same path that parallel_executor uses.
            if constexpr (detail::has_thread_pool_backend<
                              std::decay_t<BaseScheduler>>::value)
            {
                using params_type =
                    detail::thread_pool_params<std::decay_t<BaseScheduler>>;
                auto* pool = params_type::pool(exec.sched_);
                auto first_core = params_type::first_core(exec.sched_);
                auto num_cores = params_type::num_cores(exec.sched_);
                auto const& policy = params_type::policy(exec.sched_);
                auto mask = params_type::pu_mask(exec.sched_);

                return hpx::util::void_guard<result_type>(),
                       hpx::parallel::execution::detail::
                           index_queue_bulk_sync_execute(pool, first_core,
                               num_cores, policy, HPX_FORWARD(F, f), shape,
                               mask, HPX_FORWARD(Ts, ts)...);
            }
            // Check if the scheduler has get_underlying_scheduler()
            // (e.g. parallel_scheduler wrapping thread_pool_policy_scheduler)
            else if constexpr (requires {
                                   exec.sched_.get_underlying_scheduler();
                               })
            {
                using underlying_type = std::decay_t<
                    decltype(exec.sched_.get_underlying_scheduler())>;
                if constexpr (detail::has_thread_pool_backend<
                                  underlying_type>::value)
                {
                    using params_type =
                        detail::thread_pool_params<underlying_type>;
                    auto const& underlying =
                        exec.sched_.get_underlying_scheduler();
                    auto* pool = params_type::pool(underlying);
                    auto first_core = params_type::first_core(underlying);
                    auto num_cores = params_type::num_cores(underlying);
                    auto const& policy = params_type::policy(underlying);
                    auto mask = params_type::pu_mask(underlying);

                    return hpx::util::void_guard<result_type>(),
                           hpx::parallel::execution::detail::
                               index_queue_bulk_sync_execute(pool, first_core,
                                   num_cores, policy, HPX_FORWARD(F, f), shape,
                                   mask, HPX_FORWARD(Ts, ts)...);
                }
                else
                {
                    // Fallback: underlying scheduler doesn't have a pool
                    using size_type = decltype(hpx::util::size(shape));
                    size_type const n = hpx::util::size(shape);
                    return hpx::util::void_guard<result_type>(),
                           // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                           *hpx::this_thread::experimental::sync_wait(
                               bulk(schedule(exec.sched_), par, n,
                                   [shape, f = HPX_FORWARD(F, f),
                                       ... args = HPX_FORWARD(Ts, ts)](
                                       size_type i) mutable {
                                       auto it = hpx::util::begin(shape);
                                       std::advance(it, i);
                                       HPX_INVOKE(f, *it, args...);
                                   }));
                }
            }
            else
            {
                // Generic fallback: use sender/receiver pipeline
                using size_type = decltype(hpx::util::size(shape));
                size_type const n = hpx::util::size(shape);
                return hpx::util::void_guard<result_type>(),
                       // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                       *hpx::this_thread::experimental::sync_wait(
                           bulk(schedule(exec.sched_), par, n,
                               [shape, f = HPX_FORWARD(F, f),
                                   ... args = HPX_FORWARD(Ts, ts)](
                                   size_type i) mutable {
                                   auto it = hpx::util::begin(shape);
                                   std::advance(it, i);
                                   HPX_INVOKE(f, *it, args...);
                               }));
            }
#else
            return hpx::util::void_guard<result_type>(),
                   // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                   *hpx::this_thread::experimental::sync_wait(
                       bulk(schedule(exec.sched_), shape,
                           hpx::bind_back(
                               HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...)));
#endif
        }

        template <typename F, typename S, typename Future, typename... Ts>
            requires(!std::is_integral_v<S>)
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::bulk_then_execute_t,
            scheduler_executor const& exec, F&& f, S const& shape,
            Future&& predecessor, Ts&&... ts)
        {
            using result_type =
                parallel::execution::detail::then_bulk_function_result_t<F, S,
                    Future, Ts...>;

            if constexpr (std::is_void_v<result_type>)
            {
#if defined(HPX_HAVE_STDEXEC)
                // Fast path: wait on predecessor, then direct dispatch
                if constexpr (detail::has_thread_pool_backend<
                                  std::decay_t<BaseScheduler>>::value)
                {
                    using params_type =
                        detail::thread_pool_params<std::decay_t<BaseScheduler>>;

                    return hpx::async(
                        [&exec, f = HPX_FORWARD(F, f), &shape,
                            ... ts = HPX_FORWARD(Ts, ts)](
                            Future&& pred) mutable {
                            pred.get();    // wait for predecessor
                            auto* pool = params_type::pool(exec.sched_);
                            auto first_core =
                                params_type::first_core(exec.sched_);
                            auto num_cores =
                                params_type::num_cores(exec.sched_);
                            auto const& policy =
                                params_type::policy(exec.sched_);
                            auto mask = params_type::pu_mask(exec.sched_);

                            hpx::parallel::execution::detail::
                                index_queue_bulk_sync_execute(pool, first_core,
                                    num_cores, policy,
                                    HPX_FORWARD(decltype(f), f), shape, mask,
                                    HPX_FORWARD(decltype(ts), ts)...);
                        },
                        HPX_FORWARD(Future, predecessor));
                }
                else if constexpr (requires {
                                       exec.sched_.get_underlying_scheduler();
                                   })
                {
                    using underlying_type = std::decay_t<
                        decltype(exec.sched_.get_underlying_scheduler())>;
                    if constexpr (detail::has_thread_pool_backend<
                                      underlying_type>::value)
                    {
                        using uparams_type =
                            detail::thread_pool_params<underlying_type>;

                        return hpx::async(
                            [&exec, f = HPX_FORWARD(F, f), &shape,
                                ... ts = HPX_FORWARD(Ts, ts)](
                                Future&& pred) mutable {
                                pred.get();
                                auto const& underlying =
                                    exec.sched_.get_underlying_scheduler();
                                auto* pool = uparams_type::pool(underlying);
                                auto first_core =
                                    uparams_type::first_core(underlying);
                                auto num_cores =
                                    uparams_type::num_cores(underlying);
                                auto const& policy =
                                    uparams_type::policy(underlying);
                                auto mask = uparams_type::pu_mask(underlying);

                                hpx::parallel::execution::detail::
                                    index_queue_bulk_sync_execute(pool,
                                        first_core, num_cores, policy,
                                        HPX_FORWARD(decltype(f), f), shape,
                                        mask, HPX_FORWARD(decltype(ts), ts)...);
                            },
                            HPX_FORWARD(Future, predecessor));
                    }
                    else
                    {
                        // Fallback: sender pipeline
                        auto pre_req = when_all(
                            keep_future(HPX_FORWARD(Future, predecessor)));
                        using size_type = decltype(hpx::util::size(shape));
                        size_type const n = hpx::util::size(shape);
                        auto loop = bulk(
                            transfer(HPX_MOVE(pre_req), exec.sched_), par, n,
                            [shape, f = HPX_FORWARD(F, f),
                                ... args = HPX_FORWARD(Ts, ts)](
                                size_type i, auto&... receiver_args) mutable {
                                auto it = hpx::util::begin(shape);
                                std::advance(it, i);
                                HPX_INVOKE(f, *it, args..., receiver_args...);
                            });
                        return make_future(HPX_MOVE(loop));
                    }
                }
                else
                {
                    // Fallback: sender pipeline
                    auto pre_req =
                        when_all(keep_future(HPX_FORWARD(Future, predecessor)));
                    using size_type = decltype(hpx::util::size(shape));
                    size_type const n = hpx::util::size(shape);
                    auto loop =
                        bulk(transfer(HPX_MOVE(pre_req), exec.sched_), par, n,
                            [shape, f = HPX_FORWARD(F, f),
                                ... args = HPX_FORWARD(Ts, ts)](
                                size_type i, auto&... receiver_args) mutable {
                                auto it = hpx::util::begin(shape);
                                std::advance(it, i);
                                HPX_INVOKE(f, *it, args..., receiver_args...);
                            });
                    return make_future(HPX_MOVE(loop));
                }
#else
                // the overall return value is future<void>
                auto pre_req =
                    when_all(keep_future(HPX_FORWARD(Future, predecessor)));
                auto loop = bulk(transfer(HPX_MOVE(pre_req), exec.sched_),
                    shape,
                    hpx::bind_back(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));
                return make_future(HPX_MOVE(loop));
#endif
            }
            else
            {
                // the overall return value is future<std::vector<result_type>>
                auto pre_req =
                    when_all(keep_future(HPX_FORWARD(Future, predecessor)),
                        just(std::vector<result_type>(hpx::util::size(shape))));

#if defined(HPX_HAVE_STDEXEC)
                using size_type = decltype(hpx::util::size(shape));
                size_type const n = hpx::util::size(shape);
                auto loop =
                    bulk(transfer(HPX_MOVE(pre_req), exec.sched_), par, n,
                        detail::captured_args_then(
                            HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));
#else
                auto loop =
                    bulk(transfer(HPX_MOVE(pre_req), exec.sched_), shape,
                        detail::captured_args_then(
                            HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));
#endif

                return make_future(then(
                    HPX_MOVE(loop), [](auto&&, std::vector<result_type>&& v) {
                        return HPX_MOVE(v);
                    }));
            }
        }

    private:
        std::decay_t<BaseScheduler> sched_;
        /// \endcond
    };

    HPX_CXX_CORE_EXPORT template <typename BaseScheduler>
    explicit scheduler_executor(BaseScheduler&& sched)
        -> scheduler_executor<std::decay_t<BaseScheduler>>;

    // support all properties exposed by the wrapped scheduler
    HPX_CXX_CORE_EXPORT template <typename Tag, typename BaseScheduler,
        typename Property,
        HPX_CONCEPT_REQUIRES_(
            hpx::execution::experimental::is_scheduling_property_v<Tag>)>
    auto tag_invoke(
        Tag tag, scheduler_executor<BaseScheduler> const& exec, Property&& prop)
        -> decltype(scheduler_executor<BaseScheduler>(std::declval<Tag>()(
            std::declval<BaseScheduler>(), std::declval<Property>())))
    {
        return scheduler_executor<BaseScheduler>(
            tag(exec.sched(), HPX_FORWARD(Property, prop)));
    }

    HPX_CXX_CORE_EXPORT template <typename Tag, typename BaseScheduler>
        requires(hpx::execution::experimental::is_scheduling_property_v<Tag>)
    auto tag_invoke(Tag tag, scheduler_executor<BaseScheduler> const& exec)
        -> decltype(std::declval<Tag>()(std::declval<BaseScheduler>()))
    {
        return tag(exec.sched());
    }

    /// \cond NOINTERNAL
    HPX_CXX_CORE_EXPORT template <typename BaseScheduler>
    struct is_one_way_executor<
        hpx::execution::experimental::scheduler_executor<BaseScheduler>>
      : std::true_type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename BaseScheduler>
    struct is_never_blocking_one_way_executor<
        hpx::execution::experimental::scheduler_executor<BaseScheduler>>
      : std::true_type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename BaseScheduler>
    struct is_bulk_one_way_executor<
        hpx::execution::experimental::scheduler_executor<BaseScheduler>>
      : std::true_type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename BaseScheduler>
    struct is_two_way_executor<
        hpx::execution::experimental::scheduler_executor<BaseScheduler>>
      : std::true_type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename BaseScheduler>
    struct is_bulk_two_way_executor<
        hpx::execution::experimental::scheduler_executor<BaseScheduler>>
      : std::true_type
    {
    };
    /// \endcond
}    // namespace hpx::execution::experimental
