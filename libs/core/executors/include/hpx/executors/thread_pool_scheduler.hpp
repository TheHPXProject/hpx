//  Copyright (c) 2020 ETH Zurich
//  Copyright (c) 2026 Sai Charan Arvapally
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
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>

#include <hpx/executors/thread_pool_continues_on_sender.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>

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
            requires(std::same_as<
                std::decay_t<decltype(hpx::execution::experimental::
                        get_scheduler(std::declval<Env const&>()))>,
                thread_pool_policy_scheduler<Policy>>)
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

        // transform_sender for continues_on operations.
        //
        // When stdexec's domain resolution encounters
        //   continues_on(sender, hpx_thread_pool_scheduler)
        // it calls this transform_sender to replace the generic double-state
        // continues_on with our optimized single-state implementation.
        //
        // The continues_on expression tree is:
        //   continues_on_t [ scheduler, schedule_from_t [ {}, original_sender ] ]
        //
        // We extract the original sender and scheduler, then return our
        // thread_pool_continues_on_sender which uses a single operation
        // state and inline forwarding when already on an HPX thread.
        //
        // Note: We constrain on the sender being a continues_on expression
        // only. The scheduler type check is done inside the body since the
        // domain dispatch already ensures we are in thread_pool_domain.
        template <typename Sender, typename Env>
            requires sender_invokes_algorithm_v<Sender,
                hpx::execution::experimental::continues_on_t>
        constexpr auto transform_sender(
            hpx::execution::experimental::set_value_t, Sender&& sndr,
            Env const& /*env*/) const noexcept
        {
            // Destructure the continues_on expression:
            //   [continues_on_tag, scheduler, schedule_from_child]
            auto&& [tag, sched, schedule_from_child] =
                HPX_FORWARD(Sender, sndr);

            // Destructure the schedule_from wrapper to get the original
            // predecessor sender:
            //   [schedule_from_tag, empty_data, original_sender]
            auto&& [sf_tag, sf_data, original_sender] =
                HPX_FORWARD(decltype(schedule_from_child), schedule_from_child);

            return hpx::execution::experimental::detail::
                thread_pool_continues_on_sender<
                    std::decay_t<decltype(original_sender)>,
                    std::decay_t<decltype(sched)>>{
                    HPX_FORWARD(decltype(original_sender), original_sender),
                    HPX_FORWARD(decltype(sched), sched)};
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
            return lhs.pool_ == rhs.pool_ && lhs.policy_ == rhs.policy_;
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

        [[nodiscard]] auto query(
            with_processing_units_count_t, std::size_t num_cores) const
        {
            if (num_cores == 0)
            {
                auto pool =
                    pool_ ? pool_ : threads::detail::get_self_or_default_pool();
                num_cores = pool->get_active_os_thread_count();
            }
            auto scheduler_with_num_cores = *this;
            scheduler_with_num_cores.num_cores_ = num_cores;
            return scheduler_with_num_cores;
        }

        template <executor_parameters Parameters>
        [[nodiscard]] std::size_t query(processing_units_count_t,
            Parameters&& params,
            hpx::chrono::steady_duration const& iter_dur =
                hpx::chrono::null_duration,
            std::size_t num_tasks = 0) const
        {
            using exec_type = std::decay_t<decltype(*this)>;
            if constexpr (requires(std::decay_t<Parameters> const& p,
                              exec_type const& e,
                              hpx::chrono::steady_duration const& d) {
                              p.processing_units_count(e, d, std::size_t{});
                          })
            {
                return HPX_FORWARD(Parameters, params)
                    .processing_units_count(*this, iter_dur, num_tasks);
            }
            else
            {
                return get_num_cores();
            }
        }

        [[nodiscard]] auto query(
            with_first_core_t, std::size_t first_core) const noexcept
        {
            auto scheduler_with_first_core = *this;
            scheduler_with_first_core.first_core_ = first_core;
            return scheduler_with_first_core;
        }

        [[nodiscard]] constexpr std::size_t query(
            get_first_core_t) const noexcept
        {
            return get_first_core();
        }

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
        [[nodiscard]] auto query(
            with_annotation_t, char const* annotation) const
        {
            auto sched_with_annotation = *this;
            sched_with_annotation.annotation_ = annotation;
            return sched_with_annotation;
        }

        [[nodiscard]] auto query(
            with_annotation_t, std::string annotation) const
        {
            auto sched_with_annotation = *this;
            sched_with_annotation.annotation_ =
                hpx::detail::store_function_annotation(HPX_MOVE(annotation));
            return sched_with_annotation;
        }

        [[nodiscard]] constexpr char const* query(
            get_annotation_t) const noexcept
        {
            return annotation_;
        }
#endif

        [[nodiscard]] auto query(get_processing_units_mask_t) const
        {
            auto pool =
                pool_ ? pool_ : threads::detail::get_self_or_default_pool();
            return pool->get_used_processing_units(get_num_cores(), false);
        }

        [[nodiscard]] auto query(get_cores_mask_t) const
        {
            auto pool =
                pool_ ? pool_ : threads::detail::get_self_or_default_pool();
            return pool->get_used_processing_units(get_num_cores(), true);
        }

        template <typename Tag, typename Property>
            requires(
                hpx::execution::experimental::is_scheduling_property_v<Tag> &&
                hpx::execution::experimental::has_query_v<Policy const&, Tag,
                    Property>)
        [[nodiscard]] auto query(Tag tag, Property&& prop) const
        {
            auto scheduler_with_prop = *this;
            scheduler_with_prop.policy(
                tag(policy(), HPX_FORWARD(Property, prop)));
            return scheduler_with_prop;
        }

        template <typename Tag>
            requires(
                hpx::execution::experimental::is_scheduling_property_v<Tag> &&
                hpx::execution::experimental::has_query_v<Policy const&, Tag>)
        [[nodiscard]] auto query(Tag tag) const
        {
            return tag(policy());
        }

        template <typename Sender, typename Shape, typename F>
        [[nodiscard]] auto query(hpx::execution::experimental::bulk_t,
            Sender&& sender, Shape const& shape, F&& f) const
        {
            constexpr bool is_parallel =
                !std::is_same_v<Policy, hpx::launch::sync_policy> &&
                !is_sequenced_policy_v<Policy>;
            constexpr bool is_unsequenced =
                is_unsequenced_bulk_policy_v<Policy>;

            if constexpr (std::is_integral_v<std::decay_t<Shape>>)
            {
                auto iota_shape = hpx::util::counting_shape(shape);

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
                        *this, HPX_FORWARD(Sender, sender), iota_shape,
                        HPX_MOVE(wrapped_f)};
                }
                else
                {
                    return detail::thread_pool_bulk_sender<Policy,
                        std::decay_t<Sender>, decltype(iota_shape),
                        std::decay_t<F>, false, is_parallel, is_unsequenced>{
                        *this, HPX_FORWARD(Sender, sender), iota_shape,
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
                        *this, HPX_FORWARD(Sender, sender), shape,
                        HPX_MOVE(wrapped_f)};
                }
                else
                {
                    return detail::thread_pool_bulk_sender<Policy,
                        std::decay_t<Sender>, std::decay_t<Shape>,
                        std::decay_t<F>, false, is_parallel, is_unsequenced>{
                        *this, HPX_FORWARD(Sender, sender), shape,
                        HPX_FORWARD(F, f)};
                }
            }
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
                    requires(meta::value<
                        meta::one_of<CPO, set_value_t, set_stopped_t>>)
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

                // P2300 get_allocator query
                constexpr auto query(
                    hpx::execution::experimental::get_allocator_t)
                    const noexcept
                {
                    return std::allocator<std::byte>{};
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

    HPX_CXX_CORE_EXPORT using thread_pool_scheduler =
        thread_pool_policy_scheduler<hpx::launch>;

}    // namespace hpx::execution::experimental

// Include the full bulk sender definition after the scheduler is fully defined
// to avoid circular dependency issues
#include <hpx/executors/thread_pool_scheduler_bulk.hpp>
