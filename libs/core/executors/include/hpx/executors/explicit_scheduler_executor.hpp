//  Copyright (c) 2021-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/executors/explicit_scheduler_executor.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/topology.hpp>

#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::execution::experimental {

    ///////////////////////////////////////////////////////////////////////////
    // A explicit_scheduler_executor wraps any P2300 scheduler and implements
    // the executor functionalities for those. All scheduling functions return
    // senders.
    HPX_CXX_CORE_EXPORT template <typename BaseScheduler>
    struct explicit_scheduler_executor
    {
        static_assert(hpx::execution::experimental::is_scheduler_v<
                          std::decay_t<BaseScheduler>>,
            "explicit_scheduler_executor requires a scheduler");

        constexpr explicit_scheduler_executor() = default;

        template <typename Scheduler>
            requires(!std::is_same_v<std::decay_t<Scheduler>,
                         explicit_scheduler_executor> &&
                hpx::execution::experimental::is_scheduler_v<Scheduler>)
        constexpr explicit explicit_scheduler_executor(Scheduler&& sched)
          : sched_(HPX_FORWARD(Scheduler, sched))
        {
        }

        constexpr explicit_scheduler_executor(
            explicit_scheduler_executor&&) = default;
        constexpr explicit_scheduler_executor& operator=(
            explicit_scheduler_executor&&) = default;
        constexpr explicit_scheduler_executor(
            explicit_scheduler_executor const&) = default;
        constexpr explicit_scheduler_executor& operator=(
            explicit_scheduler_executor const&) = default;

        /// \cond NOINTERNAL
        constexpr bool operator==(
            explicit_scheduler_executor const& rhs) const noexcept
        {
            return sched_ == rhs.sched_;
        }

        constexpr bool operator!=(
            explicit_scheduler_executor const& rhs) const noexcept
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

        template <typename Parameters>
            requires(hpx::traits::is_executor_parameters_v<Parameters>)
        [[nodiscard]] auto query(processing_units_count_t, Parameters&& params,
            hpx::chrono::steady_duration const& = hpx::chrono::null_duration,
            std::size_t = 0) const
        {
            return sched_.query(processing_units_count_t{},
                HPX_FORWARD(Parameters, params), hpx::chrono::null_duration, 0);
        }

        template <typename Tag, typename Property>
            requires(
                hpx::execution::experimental::is_scheduling_property_v<Tag> &&
                requires(BaseScheduler const& sched, Tag tag, Property prop) {
                    sched.query(tag, HPX_FORWARD(Property, prop));
                })
        [[nodiscard]] auto query(Tag tag, Property&& prop) const
        {
            return explicit_scheduler_executor{
                sched_.query(tag, HPX_FORWARD(Property, prop))};
        }

        template <typename Tag>
            requires(
                hpx::execution::experimental::is_scheduling_property_v<Tag> &&
                requires(
                    BaseScheduler const& sched, Tag tag) { sched.query(tag); })
        [[nodiscard]] auto query(Tag tag) const
        {
            return sched_.query(tag);
        }

        // Associate the parallel_execution_tag executor tag type as a default
        // with this executor.
        using execution_category =
            hpx::traits::executor_execution_category_t<BaseScheduler>;

        // Associate the default_parameters executor parameters type as a default
        // with this executor.
        using executor_parameters_type = default_parameters;

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
            auto result = hpx::this_thread::experimental::sync_wait(
                async_execute(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));
            constexpr std::size_t size =
                hpx::tuple_size<std::decay_t<decltype(*result)>>::value;
            if constexpr (size == 0)
            {
                return;
            }
            else
            {
                // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                return hpx::get<0>(HPX_MOVE(*result));
            }
        }

        // TwoWayExecutor interface
        template <typename F, typename... Ts>
        auto async_execute(F&& f, Ts&&... ts) const
        {
            return then(schedule(sched_),
                hpx::util::deferred_call(
                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));
        }

        template <typename F, typename Future, typename... Ts>
        auto then_execute(F&& f, Future&& predecessor, Ts&&... ts) const
        {
            auto&& predecessor_continues_on_sched = continues_on(
                keep_future(HPX_FORWARD(Future, predecessor)), sched_);

            return then(HPX_MOVE(predecessor_continues_on_sched),
                hpx::bind_back(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));
        }

        // BulkTwoWayExecutor interface
        // Integral shape overload - passes integral directly to bulk
        template <typename F, typename S, typename... Ts>
            requires(std::is_integral_v<S>)
        decltype(auto) bulk_async_execute(
            F&& f, S const& shape, Ts&&... ts) const
        {
            return bulk(schedule(sched_), shape,
                hpx::bind_back(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));
        }

        // Range shape overload
        template <typename F, typename S, typename... Ts>
            requires(!std::is_integral_v<S>)
        decltype(auto) bulk_async_execute(
            F&& f, S const& shape, Ts&&... ts) const
        {
            using shape_element = hpx::traits::range_traits<S>::value_type;
            using result_type = hpx::util::detail::invoke_deferred_result_t<F,
                shape_element, Ts...>;

            if constexpr (std::is_void_v<result_type>)
            {
                // stdexec::bulk requires integral shape and execution policy
                using size_type = decltype(std::ranges::size(shape));
                size_type const n = std::ranges::size(shape);
                return bulk(schedule(sched_), n,
                    [shape,
                        bound_f = hpx::bind_back(HPX_FORWARD(F, f),
                            HPX_FORWARD(Ts, ts)...)](size_type i) mutable {
                        auto it = std::ranges::begin(shape);
                        std::ranges::advance(it, i);
                        HPX_INVOKE(bound_f, *it);
                    });
            }
            else
            {
                // A boolean as result_type is disallowed because the elements
                // of a vector<bool> cannot be modified concurrently.
                static_assert(!std::is_same_v<result_type, bool>,
                    "Using an invocable that returns a boolean with "
                    "explicit_scheduler_executor::bulk_async_execution "
                    "can result in data races!");

                using size_type = decltype(std::ranges::size(shape));
                size_type const shape_size = std::ranges::size(shape);

                using result_vector_type = std::vector<result_type>;
                result_vector_type result_vector(shape_size);

                auto f_wrapper = [](size_type const i,
                                     result_vector_type& result_vector,
                                     S const& shape, F& f, Ts&... ts) {
                    auto it = std::ranges::begin(shape);
                    result_vector[i] = HPX_INVOKE(f, *std::next(it, i), ts...);
                };

                auto get_result = [](result_vector_type&& result_vector,
                                      S const&, F&&, Ts&&...) {
                    return HPX_MOVE(result_vector);
                };

                return continues_on(
                           just(HPX_MOVE(result_vector), shape,
                               HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...),
                           sched_) |
                    bulk(shape_size, HPX_MOVE(f_wrapper)) |
                    then(HPX_MOVE(get_result));
            }
        }

        // Integral shape overload - passes integral directly
        template <typename F, typename S, typename... Ts>
            requires(std::is_integral_v<S>)
        decltype(auto) bulk_sync_execute(
            F&& f, S const& shape, Ts&&... ts) const
        {
            hpx::this_thread::experimental::sync_wait(bulk_async_execute(
                HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...));
        }

        // Range shape overload
        template <typename F, typename S, typename... Ts>
            requires(!std::is_integral_v<S>)
        decltype(auto) bulk_sync_execute(
            F&& f, S const& shape, Ts&&... ts) const
        {
            hpx::this_thread::experimental::sync_wait(bulk_async_execute(
                HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...));
        }

        // Integral shape overload - passes integral directly to bulk
        template <typename F, typename S, typename Future, typename... Ts>
            requires(std::is_integral_v<S>)
        auto bulk_then_execute(
            F&& f, S const& shape, Future&& predecessor, Ts&&... ts) const
        {
            auto pre_req =
                when_all(keep_future(HPX_FORWARD(Future, predecessor)));

            return continues_on(HPX_MOVE(pre_req), sched_) |
                bulk(shape,
                    hpx::bind_back(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));
        }

        // Range shape overload
        template <typename F, typename S, typename Future, typename... Ts>
            requires(!std::is_integral_v<S>)
        auto bulk_then_execute(
            F&& f, S const& shape, Future&& predecessor, Ts&&... ts) const
        {
            using result_type =
                parallel::execution::detail::then_bulk_function_result_t<F, S,
                    Future, Ts...>;

            static_assert(
                std::is_void_v<result_type>, "std::is_void_v<result_type>");

            auto pre_req =
                when_all(keep_future(HPX_FORWARD(Future, predecessor)));

            using size_type = decltype(std::ranges::size(shape));
            size_type const n = std::ranges::size(shape);
            return continues_on(HPX_MOVE(pre_req), sched_) |
                bulk(n,
                    [shape,
                        bound_f = hpx::bind_back(
                            HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...)](
                        size_type i, auto&... receiver_args) mutable {
                        auto it = std::ranges::begin(shape);
                        std::ranges::advance(it, i);
                        HPX_INVOKE(bound_f, *it, receiver_args...);
                    });
        }

    private:
        std::decay_t<BaseScheduler> sched_;
        /// \endcond
    };

    HPX_CXX_CORE_EXPORT template <typename BaseScheduler>
    explicit explicit_scheduler_executor(BaseScheduler&& sched)
        -> explicit_scheduler_executor<std::decay_t<BaseScheduler>>;

    // Scheduling property CPOs (and processing_units_count) detect the public
    // query() member functions directly, so no tag_invoke bridge is needed.
}    // namespace hpx::execution::experimental

namespace hpx::execution::experimental {

    /// \cond NOINTERNAL
    template <typename BaseScheduler>
    struct is_one_way_executor<hpx::execution::experimental::
            explicit_scheduler_executor<BaseScheduler>> : std::true_type
    {
    };

    template <typename BaseScheduler>
    struct is_never_blocking_one_way_executor<hpx::execution::experimental::
            explicit_scheduler_executor<BaseScheduler>> : std::true_type
    {
    };

    template <typename BaseScheduler>
    struct is_bulk_one_way_executor<hpx::execution::experimental::
            explicit_scheduler_executor<BaseScheduler>> : std::true_type
    {
    };

    template <typename BaseScheduler>
    struct is_two_way_executor<hpx::execution::experimental::
            explicit_scheduler_executor<BaseScheduler>> : std::true_type
    {
    };

    template <typename BaseScheduler>
    struct is_bulk_two_way_executor<hpx::execution::experimental::
            explicit_scheduler_executor<BaseScheduler>> : std::true_type
    {
    };

    template <typename BaseScheduler>
    struct is_scheduler_executor<hpx::execution::experimental::
            explicit_scheduler_executor<BaseScheduler>> : std::true_type
    {
    };
    /// \endcond
}    // namespace hpx::execution::experimental
