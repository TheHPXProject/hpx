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
#include <hpx/modules/timing.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/modules/type_support.hpp>

#include <hpx/executors/detail/index_queue_spawning.hpp>
#include <hpx/executors/parallel_scheduler.hpp>

#include <cstddef>
#include <exception>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::execution::experimental {

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
                return hpx::execution::experimental::get_first_core(sched);
            }
            static std::size_t num_cores(parallel_scheduler const& sched)
            {
                return hpx::execution::experimental::processing_units_count(
                    hpx::execution::experimental::null_parameters, sched,
                    hpx::chrono::null_duration, 0);
            }
            static auto const& policy(parallel_scheduler const& sched)
            {
                return sched.get_underlying_scheduler()->policy();
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
        };

        // Bundle pool / affinity parameters for index_queue_bulk_* fast paths.
        template <typename Scheduler>
        struct thread_pool_bulk_dispatch_data
        {
            using PT = thread_pool_params<std::decay_t<Scheduler>>;

            decltype(PT::pool(std::declval<Scheduler const&>())) pool;
            std::size_t first_core;
            std::size_t num_cores;
            decltype(PT::policy(std::declval<Scheduler const&>())) policy;
            decltype(hpx::execution::experimental::get_processing_units_mask(
                std::declval<Scheduler const&>())) mask;
        };

        template <typename Scheduler>
        HPX_FORCEINLINE thread_pool_bulk_dispatch_data<std::decay_t<Scheduler>>
        make_thread_pool_bulk_dispatch_data(Scheduler const& sched)
        {
            using PT = thread_pool_params<std::decay_t<Scheduler>>;
            return {
                PT::pool(sched),
                PT::first_core(sched),
                PT::num_cores(sched),
                PT::policy(sched),
                hpx::execution::experimental::get_processing_units_mask(sched),
            };
        }

        template <typename Scheduler, typename F, typename S, typename... Ts>
        HPX_FORCEINLINE decltype(auto) scheduler_bulk_async_via_thread_pool(
            Scheduler const& sched, F&& f, S const& shape, Ts&&... ts)
        {
            auto const env = make_thread_pool_bulk_dispatch_data(sched);
            return hpx::parallel::execution::detail::
                index_queue_bulk_async_execute(env.pool, env.first_core,
                    env.num_cores, env.policy, HPX_FORWARD(F, f), shape,
                    env.mask, HPX_FORWARD(Ts, ts)...);
        }

        template <typename Scheduler, typename F, typename S, typename... Ts>
        HPX_FORCEINLINE decltype(auto) scheduler_bulk_sync_via_thread_pool(
            Scheduler const& sched, F&& f, S const& shape, Ts&&... ts)
        {
            auto const env = make_thread_pool_bulk_dispatch_data(sched);
            return hpx::parallel::execution::detail::
                index_queue_bulk_sync_execute(env.pool, env.first_core,
                    env.num_cores, env.policy, HPX_FORWARD(F, f), shape,
                    env.mask, HPX_FORWARD(Ts, ts)...);
        }
    }    // namespace detail

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename F, typename... Ts>
        auto captured_args_then(F&& f, Ts&&... ts)
        {
            return [bound_f = hpx::bind_back(
                        HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...)](
                       auto i, auto&& predecessor, auto& v) mutable {
                v[i] = HPX_INVOKE(bound_f, i,
                    HPX_FORWARD(decltype(predecessor), predecessor));
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

        template <typename Scheduler>
            requires(
                !std::is_same_v<std::decay_t<Scheduler>, scheduler_executor> &&
                hpx::execution::experimental::is_scheduler_v<Scheduler>)
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

        template <executor_parameters Parameters>
        [[nodiscard]] auto query(processing_units_count_t, Parameters&& params,
            hpx::chrono::steady_duration const& duration =
                hpx::chrono::null_duration,
            std::size_t num_cores = 0) const
        {
            return sched_.query(processing_units_count_t{},
                HPX_FORWARD(Parameters, params), duration, num_cores);
        }

        template <typename Tag, typename Property>
            requires(
                hpx::execution::experimental::is_scheduling_property_v<Tag> &&
                hpx::execution::experimental::has_query_v<
                    std::decay_t<BaseScheduler>, Tag, Property>)
        [[nodiscard]] auto query(Tag tag, Property&& prop) const
        {
            return scheduler_executor{
                sched_.query(tag, HPX_FORWARD(Property, prop))};
        }

        template <typename Tag>
            requires(
                hpx::execution::experimental::is_scheduling_property_v<Tag> &&
                hpx::execution::experimental::has_query_v<
                    std::decay_t<BaseScheduler>, Tag>)
        [[nodiscard]] auto query(Tag tag) const
        {
            return sched_.query(tag);
        }

        // NonBlockingOneWayExecutor interface
        template <typename F, typename... Ts>
        void post(F&& f, Ts&&... ts) const
        {
            start_detached(then(schedule(sched_),
                hpx::util::deferred_call(
                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...)));
        }

        // OneWayExecutor interface
        template <typename F, typename... Ts>
        auto sync_execute(F&& f, Ts&&... ts) const
        {
            using result_type =
                hpx::util::detail::invoke_deferred_result_t<F, Ts...>;

            return hpx::util::void_guard<result_type>(),
                   // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                   *hpx::this_thread::experimental::sync_wait(
                       then(schedule(sched_),
                           hpx::util::deferred_call(
                               HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...)));
        }

        // TwoWayExecutor interface
        template <typename F, typename... Ts>
        decltype(auto) async_execute(F&& f, Ts&&... ts) const
        {
            return make_future(then(schedule(sched_),
                hpx::util::deferred_call(
                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...)));
        }

        template <typename F, typename Future, typename... Ts>
        decltype(auto) then_execute(
            F&& f, Future&& predecessor, Ts&&... ts) const
        {
            auto&& predecessor_transfer_sched = continues_on(
                keep_future(HPX_FORWARD(Future, predecessor)), sched_);

            return make_future(then(HPX_MOVE(predecessor_transfer_sched),
                hpx::bind_back(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...)));
        }

        // BulkTwoWayExecutor interface
        template <typename F, typename S, typename... Ts>
            requires(!std::is_integral_v<S>)
        auto bulk_async_execute(F&& f, S const& shape, Ts&&... ts) const
        {
            using shape_element = hpx::traits::range_traits<S>::value_type;
            using result_type = hpx::util::detail::invoke_deferred_result_t<F,
                shape_element, Ts...>;

            // hpx::execution::experimental::bulk requires integral shape
            using size_type = decltype(std::ranges::size(shape));
            size_type const n = std::ranges::size(shape);

            if constexpr (std::is_void_v<result_type>)
            {
                if constexpr (detail::has_thread_pool_backend<
                                  std::decay_t<BaseScheduler>>::value)
                {
                    return detail::scheduler_bulk_async_via_thread_pool(sched_,
                        HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
                }
                else if constexpr (requires {
                                       sched_.get_underlying_scheduler();
                                   })
                {
                    using underlying_type = std::decay_t<
                        decltype(sched_.get_underlying_scheduler())>;
                    if constexpr (detail::has_thread_pool_backend<
                                      underlying_type>::value)
                    {
                        auto const& underlying =
                            sched_.get_underlying_scheduler();
                        return detail::scheduler_bulk_async_via_thread_pool(
                            underlying, HPX_FORWARD(F, f), shape,
                            HPX_FORWARD(Ts, ts)...);
                    }
                    else
                    {
                        return make_future(bulk(schedule(sched_), n,
                            [shape,
                                bound_f = hpx::bind_back(
                                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...)](
                                size_type i) mutable {
                                auto it = std::ranges::begin(shape);
                                std::ranges::advance(it, i);
                                HPX_INVOKE(bound_f, *it);
                            }));
                    }
                }
                else
                {
                    return make_future(bulk(schedule(sched_), n,
                        [shape,
                            bound_f = hpx::bind_back(HPX_FORWARD(F, f),
                                HPX_FORWARD(Ts, ts)...)](size_type i) mutable {
                            auto it = std::ranges::begin(shape);
                            std::ranges::advance(it, i);
                            HPX_INVOKE(bound_f, *it);
                        }));
                }
            }
            else
            {
                // A boolean as result_type is disallowed because the elements
                // of a vector<bool> cannot be modified concurrently.
                static_assert(!std::is_same_v<result_type, bool>,
                    "Using an invocable that returns a boolean with "
                    "scheduler_executor::bulk_async_execution can result in "
                    "data races!");

                using promise_vector_type =
                    std::vector<hpx::promise<result_type>>;
                using result_vector_type =
                    std::vector<hpx::future<result_type>>;

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
                            auto it = std::ranges::begin(shape);
                            std::ranges::advance(it, i);
                            promises[i].set_value(HPX_INVOKE(f, *it, ts...));
                        },
                        [&](std::exception_ptr&& ep) {
                            promises[i].set_exception(HPX_MOVE(ep));
                        });
                };

                start_detached(bulk(
                    continues_on(just(HPX_MOVE(promises), HPX_FORWARD(F, f),
                                     shape, HPX_FORWARD(Ts, ts)...),
                        sched_),
                    n, HPX_MOVE(f_helper)));

                return results;
            }
        }

        template <typename F, typename S, typename... Ts>
            requires(!std::is_integral_v<S>)
        auto bulk_sync_execute(F&& f, S const& shape, Ts&&... ts) const
        {
            using shape_element = hpx::traits::range_traits<S>::value_type;
            using result_type = hpx::util::detail::invoke_deferred_result_t<F,
                shape_element, Ts...>;

            using size_type = decltype(std::ranges::size(shape));
            size_type const n = std::ranges::size(shape);

            if constexpr (detail::has_thread_pool_backend<
                              std::decay_t<BaseScheduler>>::value)
            {
                return hpx::util::void_guard<result_type>(),
                       detail::scheduler_bulk_sync_via_thread_pool(sched_,
                           HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
            }
            else if constexpr (requires { sched_.get_underlying_scheduler(); })
            {
                using underlying_type =
                    std::decay_t<decltype(sched_.get_underlying_scheduler())>;
                if constexpr (detail::has_thread_pool_backend<
                                  underlying_type>::value)
                {
                    auto const& underlying = sched_.get_underlying_scheduler();

                    return hpx::util::void_guard<result_type>(),
                           detail::scheduler_bulk_sync_via_thread_pool(
                               underlying, HPX_FORWARD(F, f), shape,
                               HPX_FORWARD(Ts, ts)...);
                }
                else
                {
                    return hpx::util::void_guard<result_type>(),
                           // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                           *hpx::this_thread::experimental::sync_wait(bulk(
                               schedule(sched_), n,
                               [shape,
                                   bound_f = hpx::bind_back(HPX_FORWARD(F, f),
                                       HPX_FORWARD(Ts, ts)...)](
                                   size_type i) mutable {
                                   auto it = std::ranges::begin(shape);
                                   std::ranges::advance(it, i);
                                   HPX_INVOKE(bound_f, *it);
                               }));
                }
            }
            else
            {
                return hpx::util::void_guard<result_type>(),
                       // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                       *hpx::this_thread::experimental::sync_wait(
                           bulk(schedule(sched_), n,
                               [shape,
                                   bound_f = hpx::bind_back(HPX_FORWARD(F, f),
                                       HPX_FORWARD(Ts, ts)...)](
                                   size_type i) mutable {
                                   auto it = std::ranges::begin(shape);
                                   std::ranges::advance(it, i);
                                   HPX_INVOKE(bound_f, *it);
                               }));
            }
        }

        template <typename F, typename S, typename Future, typename... Ts>
            requires(!std::is_integral_v<S>)
        decltype(auto) bulk_then_execute(
            F&& f, S const& shape, Future&& predecessor, Ts&&... ts) const
        {
            using result_type =
                parallel::execution::detail::then_bulk_function_result_t<F, S,
                    Future, Ts...>;

            if constexpr (std::is_void_v<result_type>)
            {
                auto pre_req =
                    when_all(keep_future(HPX_FORWARD(Future, predecessor)));

                auto loop = bulk(continues_on(HPX_MOVE(pre_req), sched_), shape,
                    hpx::bind_back(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));

                return make_future(HPX_MOVE(loop));
            }
            else
            {
                // A boolean as result_type is disallowed because the elements
                // of a vector<bool> cannot be modified concurrently.
                static_assert(!std::is_same_v<result_type, bool>,
                    "Using an invocable that returns a boolean with "
                    "scheduler_executor::bulk_then_execute can result in "
                    "data races!");

                // the overall return value is future<std::vector<result_type>>
                auto pre_req = when_all(
                    keep_future(HPX_FORWARD(Future, predecessor)),
                    just(std::vector<result_type>(std::ranges::size(shape))));

                auto loop = bulk(continues_on(HPX_MOVE(pre_req), sched_), shape,
                    detail::captured_args_then(
                        HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));

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

    // Scheduling property CPOs detect the public query() member functions
    // above directly (via property_base), so no tag_invoke bridge is needed.

    /// \cond NOINTERNAL
    template <typename BaseScheduler>
    struct is_one_way_executor<
        hpx::execution::experimental::scheduler_executor<BaseScheduler>>
      : std::true_type
    {
    };

    template <typename BaseScheduler>
    struct is_never_blocking_one_way_executor<
        hpx::execution::experimental::scheduler_executor<BaseScheduler>>
      : std::true_type
    {
    };

    template <typename BaseScheduler>
    struct is_bulk_one_way_executor<
        hpx::execution::experimental::scheduler_executor<BaseScheduler>>
      : std::true_type
    {
    };

    template <typename BaseScheduler>
    struct is_two_way_executor<
        hpx::execution::experimental::scheduler_executor<BaseScheduler>>
      : std::true_type
    {
    };

    template <typename BaseScheduler>
    struct is_bulk_two_way_executor<
        hpx::execution::experimental::scheduler_executor<BaseScheduler>>
      : std::true_type
    {
    };
    /// \endcond
}    // namespace hpx::execution::experimental
