//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2022-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/assert.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/topology.hpp>

#include <concepts>
#include <cstddef>
#include <exception>
#include <mutex>
#include <optional>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>

#include <hpx/execution/algorithms/bulk.hpp>
#include <hpx/execution_base/stdexec_forward.hpp>

#include <ranges>

// Forward declaration
namespace hpx::execution::experimental::detail {
    HPX_CXX_CORE_EXPORT template <typename Policy, typename Sender,
        typename Shape, typename F, bool IsChunked, bool IsParallel,
        bool IsUnsequenced>
    class thread_pool_bulk_sender;
}    // namespace hpx::execution::experimental::detail

namespace hpx::execution::experimental {

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename Policy>
        struct get_default_scheduler_policy
        {
            static constexpr Policy call() noexcept
            {
                return Policy{};
            }
        };

        template <>
        struct get_default_scheduler_policy<hpx::launch>
        {
            static constexpr hpx::launch::async_policy call() noexcept
            {
                return hpx::launch::async_policy{};
            }
        };
    }    // namespace detail

    // Forward declarations
    HPX_CXX_CORE_EXPORT template <typename Policy>
    struct thread_pool_policy_scheduler;

    // Forward declarations for domain system

    // Concept to match bulk sender types
    HPX_CXX_CORE_EXPORT template <typename Sender>
    concept bulk_chunked_or_unchunked_sender =
        sender_invokes_algorithm_v<Sender,
            hpx::execution::experimental::bulk_t> ||
        sender_invokes_algorithm_v<Sender,
            hpx::execution::experimental::bulk_chunked_t> ||
        sender_invokes_algorithm_v<Sender,
            hpx::execution::experimental::bulk_unchunked_t>;

    HPX_CXX_CORE_EXPORT template <typename Policy>
    inline constexpr bool is_sequenced_policy_v = false;

    template <>
    inline constexpr bool is_sequenced_policy_v<sequenced_policy> = true;

    template <>
    inline constexpr bool is_sequenced_policy_v<unsequenced_policy> = true;

    HPX_CXX_CORE_EXPORT template <typename Policy>
    inline constexpr bool is_unsequenced_bulk_policy_v = false;

    template <>
    inline constexpr bool is_unsequenced_bulk_policy_v<unsequenced_policy> =
        true;

    template <>
    inline constexpr bool
        is_unsequenced_bulk_policy_v<parallel_unsequenced_policy> = true;

    // Domain customization for stdexec bulk operations and sync_wait,
    // with thread-pool parallelism derived from wrapped execution policies.
    HPX_CXX_CORE_EXPORT template <typename Policy>
    struct thread_pool_domain
      : hpx::execution::experimental::detail::sync_wait_domain
    {
        // transform_sender for bulk operations (stdexec parallel_scheduler pattern)
        template <bulk_chunked_or_unchunked_sender Sender, typename Env>
            requires std::same_as<
                std::decay_t<decltype(hpx::execution::experimental::
                        get_scheduler(std::declval<Env const&>()))>,
                thread_pool_policy_scheduler<Policy>>
        constexpr auto transform_sender(
            hpx::execution::experimental::set_value_t, Sender&& sndr,
            Env const& env) const noexcept
        {
            auto sched = hpx::execution::experimental::get_scheduler(env);

            auto&& [tag, data, child] = sndr;
            auto&& [pol, shape, f] = data;

            auto iota_shape = hpx::util::counting_shape(shape);

            constexpr bool is_chunked = sender_invokes_algorithm_v<Sender,
                hpx::execution::experimental::bulk_chunked_t>;

            constexpr bool is_parallel =
                !is_sequenced_policy_v<std::decay_t<decltype(pol.__get())>>;

            constexpr bool is_unsequenced = is_unsequenced_bulk_policy_v<
                std::decay_t<decltype(pol.__get())>>;

            auto pu_mask =
                hpx::execution::experimental::get_processing_units_mask(sched);

            return hpx::execution::experimental::detail::
                thread_pool_bulk_sender<Policy, std::decay_t<decltype(child)>,
                    std::decay_t<decltype(iota_shape)>,
                    std::decay_t<decltype(f)>, is_chunked, is_parallel,
                    is_unsequenced>{HPX_MOVE(sched),
                    HPX_FORWARD(decltype(child), child), HPX_MOVE(iota_shape),
                    HPX_FORWARD(decltype(f), f), HPX_MOVE(pu_mask)};
        }
    };

    HPX_CXX_CORE_EXPORT template <typename Policy>
    struct thread_pool_policy_scheduler
    {
        // Expose the policy type for domain customization
        using policy_type = Policy;

        // Associate the parallel_execution_tag tag type as a default with this
        // scheduler, except if the given launch policy is sync.
        using execution_category =
            std::conditional_t<std::is_same_v<Policy, launch::sync_policy>,
                sequenced_execution_tag, parallel_execution_tag>;

        constexpr explicit thread_pool_policy_scheduler(
            Policy l = experimental::detail::get_default_scheduler_policy<
                Policy>::call())
          : policy_(l)
        {
        }

        explicit thread_pool_policy_scheduler(
            hpx::threads::thread_pool_base* pool,
            Policy l = experimental::detail::get_default_scheduler_policy<
                Policy>::call()) noexcept
          : pool_(pool)
          , policy_(l)
        {
        }

        /// \cond NOINTERNAL
        friend constexpr bool operator==(
            thread_pool_policy_scheduler const& lhs,
            thread_pool_policy_scheduler const& rhs) noexcept
        {
            if constexpr (requires { lhs.policy_ == rhs.policy_; })
            {
                return lhs.pool_ == rhs.pool_ && lhs.policy_ == rhs.policy_;
            }
            else
            {
                return lhs.pool_ == rhs.pool_;
            }
        }

        friend constexpr bool operator!=(
            thread_pool_policy_scheduler const& lhs,
            thread_pool_policy_scheduler const& rhs) noexcept
        {
            return !(lhs == rhs);
        }

        [[nodiscard]] hpx::threads::thread_pool_base* get_thread_pool()
            const noexcept
        {
            HPX_ASSERT(pool_);
            return pool_;
        }

        template <typename Executor_>
            requires(
                std::is_convertible_v<Executor_, thread_pool_policy_scheduler>)
        friend auto tag_invoke(
            hpx::execution::experimental::with_processing_units_count_t,
            Executor_ const& scheduler, std::size_t num_cores)
        {
            if (num_cores == 0)
            {
                auto pool = scheduler.pool_ ?
                    scheduler.pool_ :
                    threads::detail::get_self_or_default_pool();
                num_cores = pool->get_active_os_thread_count();
            }
            auto scheduler_with_num_cores = scheduler;
            scheduler_with_num_cores.num_cores_ = num_cores;
            return scheduler_with_num_cores;
        }

        template <executor_parameters Parameters>
        friend constexpr std::size_t tag_invoke(
            hpx::execution::experimental::processing_units_count_t,
            Parameters&&, thread_pool_policy_scheduler const& scheduler,
            hpx::chrono::steady_duration const& = hpx::chrono::null_duration,
            std::size_t = 0)
        {
            return scheduler.get_num_cores();
        }

        template <typename Sender, typename Shape, typename F>
        friend auto tag_invoke(hpx::execution::experimental::bulk_t,
            thread_pool_policy_scheduler const& scheduler, Sender&& sender,
            Shape const& shape, F&& f)
        {
            constexpr bool is_parallel =
                !std::is_same_v<Policy, hpx::launch::sync_policy> &&
                !is_sequenced_policy_v<Policy>;
            constexpr bool is_unsequenced =
                is_unsequenced_bulk_policy_v<Policy>;

            if constexpr (std::is_integral_v<std::decay_t<Shape>>)
            {
                auto iota_shape = hpx::util::counting_shape(shape);

                // For parallel policies, we want internal chunking for efficiency.
                // Since bulk_t expects an unchunked function, we wrap f to
                // handle a range of indices in a loop.
                if constexpr (!std::is_same_v<Policy, hpx::launch::sync_policy>)
                {
                    auto wrapped_f = [f = HPX_FORWARD(F, f)](auto start,
                                         auto end, auto&... ts) mutable {
                        for (auto i = start; i != end; ++i)
                        {
                            HPX_INVOKE(f, i, ts...);
                        }
                    };

                    return detail::thread_pool_bulk_sender<Policy,
                        std::decay_t<Sender>, decltype(iota_shape),
                        decltype(wrapped_f), true, is_parallel, is_unsequenced>{
                        scheduler, HPX_FORWARD(Sender, sender), iota_shape,
                        HPX_MOVE(wrapped_f)};
                }
                else
                {
                    return detail::thread_pool_bulk_sender<Policy,
                        std::decay_t<Sender>, decltype(iota_shape),
                        std::decay_t<F>, false, is_parallel, is_unsequenced>{
                        scheduler, HPX_FORWARD(Sender, sender), iota_shape,
                        HPX_FORWARD(F, f)};
                }
            }
            else
            {
                if constexpr (!std::is_same_v<Policy, hpx::launch::sync_policy>)
                {
                    auto wrapped_f = [f = HPX_FORWARD(F, f)](auto start,
                                         auto end, auto&... ts) mutable {
                        for (auto i = start; i != end; ++i)
                        {
                            HPX_INVOKE(f, i, ts...);
                        }
                    };

                    return detail::thread_pool_bulk_sender<Policy,
                        std::decay_t<Sender>, std::decay_t<Shape>,
                        decltype(wrapped_f), true, is_parallel, is_unsequenced>{
                        scheduler, HPX_FORWARD(Sender, sender), shape,
                        HPX_MOVE(wrapped_f)};
                }
                else
                {
                    return detail::thread_pool_bulk_sender<Policy,
                        std::decay_t<Sender>, std::decay_t<Shape>,
                        std::decay_t<F>, false, is_parallel, is_unsequenced>{
                        scheduler, HPX_FORWARD(Sender, sender), shape,
                        HPX_FORWARD(F, f)};
                }
            }
        }

        template <typename Executor_>
            requires(
                std::is_convertible_v<Executor_, thread_pool_policy_scheduler>)
        friend constexpr auto tag_invoke(
            hpx::execution::experimental::with_first_core_t,
            Executor_ const& exec, std::size_t first_core) noexcept
        {
            auto exec_with_first_core = exec;
            exec_with_first_core.first_core_ = first_core;
            return exec_with_first_core;
        }

        friend constexpr std::size_t tag_invoke(
            hpx::execution::experimental::get_first_core_t,
            thread_pool_policy_scheduler const& exec) noexcept
        {
            return exec.get_first_core();
        }

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
        // support with_annotation property
        template <typename Executor_>
            requires(
                std::is_convertible_v<Executor_, thread_pool_policy_scheduler>)
        friend constexpr auto tag_invoke(
            hpx::execution::experimental::with_annotation_t,
            Executor_ const& scheduler, char const* annotation)
        {
            auto sched_with_annotation = scheduler;
            sched_with_annotation.annotation_ = annotation;
            return sched_with_annotation;
        }

        template <typename Executor_>
            requires(
                std::is_convertible_v<Executor_, thread_pool_policy_scheduler>)
        friend auto tag_invoke(hpx::execution::experimental::with_annotation_t,
            Executor_ const& scheduler, std::string annotation)
        {
            auto sched_with_annotation = scheduler;
            sched_with_annotation.annotation_ =
                hpx::detail::store_function_annotation(HPX_MOVE(annotation));
            return sched_with_annotation;
        }

        // support get_annotation property
        friend constexpr char const* tag_invoke(
            hpx::execution::experimental::get_annotation_t,
            thread_pool_policy_scheduler const& scheduler) noexcept
        {
            return scheduler.annotation_;
        }
#endif

        friend auto tag_invoke(
            hpx::execution::experimental::get_processing_units_mask_t,
            thread_pool_policy_scheduler const& exec)
        {
            auto pool = exec.pool_ ?
                exec.pool_ :
                threads::detail::get_self_or_default_pool();
            return pool->get_used_processing_units(exec.get_num_cores(), false);
        }

        friend auto tag_invoke(hpx::execution::experimental::get_cores_mask_t,
            thread_pool_policy_scheduler const& exec)
        {
            auto pool = exec.pool_ ?
                exec.pool_ :
                threads::detail::get_self_or_default_pool();
            return pool->get_used_processing_units(exec.get_num_cores(), true);
        }

        template <typename F>
        void execute(F&& f, Policy const& policy) const
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            hpx::threads::thread_description desc(f, annotation_);
#else
            hpx::threads::thread_description desc(f);
#endif
            auto pool =
                pool_ ? pool_ : threads::detail::get_self_or_default_pool();

            hpx::detail::post_policy_dispatch<Policy>::call(
                policy, desc, pool, HPX_FORWARD(F, f));
        }

        template <typename F>
        HPX_FORCEINLINE void execute(F&& f) const
        {
            execute(HPX_FORWARD(F, f), policy_);
        }

        template <typename Scheduler, typename Receiver>
        struct operation_state
        {
            HPX_NO_UNIQUE_ADDRESS std::decay_t<Scheduler> scheduler;
            HPX_NO_UNIQUE_ADDRESS std::decay_t<Receiver> receiver;

            template <typename Scheduler_, typename Receiver_>
            operation_state(Scheduler_&& scheduler, Receiver_&& receiver)
              : scheduler(HPX_FORWARD(Scheduler_, scheduler))
              , receiver(HPX_FORWARD(Receiver_, receiver))
            {
            }

            operation_state(operation_state&&) = default;
            operation_state(operation_state const&) = delete;
            operation_state& operator=(operation_state&&) = default;
            operation_state& operator=(operation_state const&) = delete;

            ~operation_state() = default;

            void start() & noexcept
            {
                auto stop_token = hpx::execution::experimental::get_stop_token(
                    hpx::execution::experimental::get_env(receiver));
                if (stop_token.stop_requested())
                {
                    hpx::execution::experimental::set_stopped(
                        HPX_MOVE(receiver));
                    return;
                }
#if defined(HPX_CLANG_VERSION)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
                hpx::detail::try_catch_exception_ptr(
                    [&]() {
                        scheduler.execute(
                            [receiver = HPX_MOVE(receiver)]() mutable {
                                hpx::execution::experimental::set_value(
                                    HPX_MOVE(receiver));
                            });
                    },
                    [&](std::exception_ptr ep) {
                        hpx::execution::experimental::set_error(
                            HPX_MOVE(receiver), HPX_MOVE(ep));
                    });
#if defined(HPX_CLANG_VERSION)
#pragma clang diagnostic pop
#endif
            }
        };

        template <typename Scheduler>
        struct sender
        {
            HPX_NO_UNIQUE_ADDRESS std::decay_t<Scheduler> scheduler;
            using sender_concept = hpx::execution::experimental::sender_t;
            using completion_signatures =
                hpx::execution::experimental::completion_signatures<
                    hpx::execution::experimental::set_value_t(),
                    hpx::execution::experimental::set_error_t(
                        std::exception_ptr),
                    hpx::execution::experimental::set_stopped_t()>;

            template <typename Self, typename... Env>
            static consteval auto get_completion_signatures() noexcept
                -> completion_signatures
            {
                return {};
            }

            template <typename Receiver>
            operation_state<Scheduler, Receiver> connect(Receiver&& receiver) &&
            {
                return {HPX_MOVE(scheduler), HPX_FORWARD(Receiver, receiver)};
            }

            template <typename Receiver>
            operation_state<Scheduler, Receiver> connect(Receiver&& receiver) &
            {
                return {scheduler, HPX_FORWARD(Receiver, receiver)};
            }

            struct env
            {
                std::decay_t<Scheduler> const& sched;

                auto query(
                    hpx::execution::experimental::get_domain_t) const noexcept
                {
                    return hpx::execution::experimental::get_domain(sched);
                }

                template <typename CPO>
                    requires meta::value<
                        meta::one_of<CPO, set_value_t, set_stopped_t>>
                auto query(
                    hpx::execution::experimental::get_completion_scheduler_t<
                        CPO>) const noexcept
                {
                    return sched;
                }

                // P3826R5: get_completion_domain queries
                // The completing domain is resolved via:
                //   sender env -> get_completion_scheduler<set_value_t>
                //              -> scheduler -> get_completion_domain<set_value_t>
                //              -> thread_pool_domain
                template <typename CPO>
                auto query(
                    hpx::execution::experimental::get_completion_domain_t<CPO>)
                    const noexcept
                {
                    return sched.query(
                        hpx::execution::experimental::get_completion_domain_t<
                            CPO>{});
                }
            };

            constexpr auto get_env() const noexcept
            {
                return env{scheduler};
            }
        };

        auto query(
            hpx::execution::experimental::get_forward_progress_guarantee_t)
            const noexcept
            -> hpx::execution::experimental::forward_progress_guarantee
        {
            if (hpx::has_async_policy(policy()))
            {
                return hpx::execution::experimental::
                    forward_progress_guarantee::parallel;
            }
            else
            {
                return hpx::execution::experimental::
                    forward_progress_guarantee::concurrent;
            }
        }

        // Direct schedule() member function for newer stdexec
        constexpr sender<thread_pool_policy_scheduler> schedule() const
        {
            return {*this};
        }

        void policy(Policy policy) noexcept
        {
            policy_ = HPX_MOVE(policy);
        }

        constexpr Policy const& policy() const noexcept
        {
            return policy_;
        }

        // Returns the execution domain of this scheduler (following
        // system_context.hpp pattern).
        [[nodiscard]]
        static auto query(hpx::execution::experimental::get_domain_t) noexcept
            -> thread_pool_domain<Policy>
        {
            return {};
        }

        // P3826R5: Returns the completion domain for this scheduler. The domain
        // resolution chain uses this to determine which domains
        // transform_sender to invoke for bulk operations.
        template <typename CPO>
        [[nodiscard]]
        static auto query(
            hpx::execution::experimental::get_completion_domain_t<CPO>) noexcept
            -> thread_pool_domain<Policy>
        {
            return {};
        }
        /// \endcond

    private:
        /// \cond NOINTERNAL
        [[nodiscard]] std::size_t get_num_cores() const
        {
            if (num_cores_ != 0)
            {
                return num_cores_;
            }

            if constexpr (std::is_same_v<Policy, launch::sync_policy>)
            {
                return 1;
            }
            else
            {
                if (policy_.get_policy() == hpx::launch_policy::sync)
                {
                    return 1;
                }

                auto const* pool =
                    pool_ ? pool_ : threads::detail::get_self_or_default_pool();
                return pool->get_os_thread_count();
            }
        }

        [[nodiscard]] constexpr std::size_t get_first_core() const noexcept
        {
            return first_core_;
        }
        /// \endcond

    private:
        /// \cond NOINTERNAL
        hpx::threads::thread_pool_base* pool_ =
            hpx::threads::detail::get_self_or_default_pool();
        Policy policy_;
        std::size_t first_core_ = 0;
        std::size_t num_cores_ = 0;
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
        char const* annotation_ = nullptr;
#endif
        /// \endcond
    };

    // support all properties exposed by the embedded policy
    // clang-format off
    HPX_CXX_CORE_EXPORT template <typename Tag, typename Policy,
        typename Property,
        HPX_CONCEPT_REQUIRES_(
            hpx::execution::experimental::is_scheduling_property_v<Tag>
        )>
    // clang-format on
    auto tag_invoke(Tag tag,
        thread_pool_policy_scheduler<Policy> const& scheduler, Property&& prop)
        -> decltype(std::declval<thread_pool_policy_scheduler<Policy>>().policy(
                        std::declval<Tag>()(
                            std::declval<Policy>(), std::declval<Property>())),
            thread_pool_policy_scheduler<Policy>())
    {
        auto scheduler_with_prop = scheduler;
        scheduler_with_prop.policy(
            tag(scheduler.policy(), HPX_FORWARD(Property, prop)));
        return scheduler_with_prop;
    }

    // clang-format off
    HPX_CXX_CORE_EXPORT template <typename Tag, typename Policy,
        HPX_CONCEPT_REQUIRES_(
            hpx::execution::experimental::is_scheduling_property_v<Tag>
        )>
    // clang-format on
    auto tag_invoke(
        Tag tag, thread_pool_policy_scheduler<Policy> const& scheduler)
        -> decltype(std::declval<Tag>()(std::declval<Policy>()))
    {
        return tag(scheduler.policy());
    }

    HPX_CXX_CORE_EXPORT using thread_pool_scheduler =
        thread_pool_policy_scheduler<hpx::launch>;

}    // namespace hpx::execution::experimental

// Include the full bulk sender definition after the scheduler is fully defined
// to avoid circular dependency issues
#include <hpx/executors/thread_pool_scheduler_bulk.hpp>
