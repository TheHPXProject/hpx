//  Copyright (c) 2019-2020 ETH Zurich
//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2019 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/executors/detail/hierarchical_spawning.hpp>
#include <hpx/executors/detail/index_queue_spawning.hpp>
#include <hpx/executors/execution_policy_mappings.hpp>
#include <hpx/modules/allocator_support.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/topology.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::parallel::execution::detail {

    HPX_CXX_CORE_EXPORT template <typename Policy>
    struct get_default_policy
    {
        static constexpr Policy call() noexcept
        {
            return Policy{};
        }
    };

    template <>
    struct get_default_policy<hpx::launch>
    {
        static constexpr hpx::launch::async_policy call() noexcept
        {
            return hpx::launch::async_policy{};
        }
    };

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename F, typename Shape, typename... Ts>
    struct bulk_function_result;

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename F, typename Shape, typename Future,
        typename... Ts>
    struct bulk_then_execute_result;

    HPX_CXX_CORE_EXPORT template <typename F, typename Shape, typename Future,
        typename... Ts>
    struct then_bulk_function_result;
}    // namespace hpx::parallel::execution::detail

namespace hpx::execution {

    ///////////////////////////////////////////////////////////////////////////
    /// A \a parallel_executor creates groups of parallel execution agents
    /// which execute in threads implicitly created by the executor. This
    /// executor prefers continuing with the creating thread first before
    /// executing newly created threads.
    ///
    /// This executor conforms to the concepts of a TwoWayExecutor,
    /// and a BulkTwoWayExecutor
    HPX_CXX_CORE_EXPORT template <typename Policy>
    struct parallel_policy_executor_base
    {
        /// Associate the parallel_execution_tag executor tag type as a default
        /// with this executor, except if the given launch policy is sync.
        using execution_category =
            std::conditional_t<std::is_same_v<Policy, launch::sync_policy>,
                sequenced_execution_tag, parallel_execution_tag>;

        /// Associate the default_parameters executor parameters type as a default
        /// with this executor.
        using executor_parameters_type = experimental::default_parameters;

    protected:
        // NOLINTBEGIN(bugprone-crtp-constructor-accessibility)

        /// Create a new parallel executor
        constexpr explicit parallel_policy_executor_base(
            threads::thread_priority priority,
            threads::thread_stacksize stacksize =
                threads::thread_stacksize::default_,
            threads::thread_schedule_hint schedulehint = {},
            Policy l = parallel::execution::detail::get_default_policy<
                Policy>::call()) noexcept
          : policy_(l, priority, stacksize, schedulehint)
        {
        }

        constexpr explicit parallel_policy_executor_base(
            threads::thread_stacksize stacksize,
            threads::thread_schedule_hint schedulehint = {},
            Policy l = parallel::execution::detail::get_default_policy<
                Policy>::call()) noexcept
          : policy_(l, l.priority(), stacksize, schedulehint)
        {
        }

        constexpr explicit parallel_policy_executor_base(
            threads::thread_schedule_hint schedulehint,
            Policy l = parallel::execution::detail::get_default_policy<
                Policy>::call()) noexcept
          : policy_(l, l.priority(), l.stacksize(), schedulehint)
        {
        }

        constexpr explicit parallel_policy_executor_base(Policy l) noexcept
          : policy_(l)
        {
        }

        constexpr parallel_policy_executor_base() noexcept
          : policy_(
                parallel::execution::detail::get_default_policy<Policy>::call())
        {
        }

        constexpr explicit parallel_policy_executor_base(
            threads::thread_pool_base* pool, Policy l) noexcept
          : pool_(pool)
          , policy_(l)
        {
        }

        constexpr explicit parallel_policy_executor_base(
            threads::thread_pool_base* pool,
            threads::thread_priority priority =
                threads::thread_priority::default_,
            threads::thread_stacksize stacksize =
                threads::thread_stacksize::default_,
            threads::thread_schedule_hint schedulehint = {},
            Policy l =
                parallel::execution::detail::get_default_policy<Policy>::call())
          : pool_(pool)
          , policy_(l, priority, stacksize, schedulehint)
        {
        }

    public:
        parallel_policy_executor_base(parallel_policy_executor_base const& rhs)
          : pool_(rhs.pool_)
          , policy_(rhs.policy_)
          , first_core_(rhs.first_core_)
          , num_cores_(rhs.num_cores_)
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
          , annotation_(rhs.annotation_)
#endif
        {
        }
        // NOLINTEND(bugprone-crtp-constructor-accessibility)

        parallel_policy_executor_base& operator=(
            parallel_policy_executor_base const& rhs)
        {
            if (this != &rhs)
            {
                pool_ = rhs.pool_;
                policy_ = rhs.policy_;
                first_core_ = rhs.first_core_;
                num_cores_ = rhs.num_cores_;

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
                annotation_ = rhs.annotation_;
#endif
            }
            return *this;
        }

        constexpr ~parallel_policy_executor_base() = default;

        // backwards compatibility support, will be removed in the future
        template <typename Parameters>
        std::size_t processing_units_count(Parameters&&,
            hpx::chrono::steady_duration const& = hpx::chrono::null_duration,
            std::size_t = 0) const
        {
            return get_num_cores();
        }

    public:
        /// \cond NOINTERNAL
        void policy(Policy policy) noexcept
        {
            policy_ = HPX_MOVE(policy);
        }

        [[nodiscard]] constexpr Policy const& policy() const noexcept
        {
            return policy_;
        }

    protected:
        // OneWayExecutor interface
        template <typename F, typename... Ts>
        decltype(auto) sync_impl(F&& f, Ts&&... ts) const
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            hpx::scoped_annotation annotate(annotation_ ?
                    annotation_ :
                    "parallel_policy_executor_base::sync_execute");
#endif
            return hpx::detail::sync_launch_policy_dispatch<
                launch::sync_policy>::call(policy_, HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }

        // TwoWayExecutor interface
        template <typename F, typename... Ts>
        decltype(auto) async_impl(F&& f, Ts&&... ts) const
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            hpx::threads::thread_description desc(f, annotation_);
#else
            hpx::threads::thread_description desc(f);
#endif

            return hpx::detail::async_launch_policy_dispatch<Policy>::call(
                policy_, desc, pool(), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename Future, typename... Ts>
        decltype(auto) then_impl(F&& f, Future&& predecessor, Ts&&... ts) const
        {
            using result_type =
                hpx::util::detail::invoke_deferred_result_t<F, Future, Ts...>;

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            auto&& func = hpx::util::one_shot(hpx::bind_back(
                hpx::annotated_function(HPX_FORWARD(F, f), annotation_),
                HPX_FORWARD(Ts, ts)...));
#else
            auto&& func = hpx::util::one_shot(
                hpx::bind_back(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...));
#endif

            using allocator_type = hpx::util::thread_local_caching_allocator<
                hpx::lockfree::variable_size_stack,
                hpx::util::internal_allocator<>>;
            hpx::traits::detail::shared_state_ptr_t<result_type> p =
                lcos::detail::make_continuation_alloc_nounwrap<result_type>(
                    allocator_type{}, HPX_FORWARD(Future, predecessor), policy_,
                    HPX_MOVE(func));

            return hpx::traits::future_access<hpx::future<result_type>>::create(
                HPX_MOVE(p));
        }

        // NonBlockingOneWayExecutor (adapted) interface
        template <typename F, typename... Ts>
        void post_impl(F&& f, Ts&&... ts) const
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            hpx::threads::thread_description desc(f, annotation_);
#else
            hpx::threads::thread_description desc(f);
#endif

            hpx::detail::post_policy_dispatch<Policy>::call(policy_, desc,
                pool(), HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename Derived, typename F, typename S, typename Future,
            typename... Ts>
            requires(!std::is_integral_v<S>)
        static decltype(auto) bulk_then_impl(Derived&& self, F&& f,
            S const& shape, Future&& predecessor, Ts&&... ts)
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            return parallel::execution::detail::
                hierarchical_bulk_then_execute_helper(self, self.policy_,
                    hpx::annotated_function(
                        HPX_FORWARD(F, f), self.annotation_),
                    shape, HPX_FORWARD(Future, predecessor),
                    HPX_FORWARD(Ts, ts)...);
#else
            return parallel::execution::detail::
                hierarchical_bulk_then_execute_helper(self, self.policy_,
                    HPX_FORWARD(F, f), shape, HPX_FORWARD(Future, predecessor),
                    HPX_FORWARD(Ts, ts)...);
#endif
        }

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
                return pool()->get_os_thread_count();
            }
        }

        [[nodiscard]] std::size_t get_first_core() const noexcept
        {
            return first_core_;
        }

        HPX_FORCEINLINE static constexpr std::uint32_t wrapped_pu_num(
            std::uint32_t const pu, bool const needs_wraparound,
            std::uint32_t const available_threads) noexcept
        {
            if (!needs_wraparound || pu < available_threads)
            {
                return pu;
            }
            return pu % available_threads;
        }

        hpx::threads::mask_type pu_mask() const
        {
            auto const num_threads = get_num_cores();
            auto const available_threads = static_cast<std::uint32_t>(
                pool()->get_active_os_thread_count());
            bool const needs_wraparound =
                num_threads > available_threads || get_first_core() != 0;

            std::uint32_t const overall_threads =    //-V101
                hpx::threads::hardware_concurrency();
            auto mask = hpx::threads::mask_type();
            hpx::threads::resize(mask, overall_threads);

            auto const& rp = hpx::resource::get_partitioner();
            for (std::uint32_t i = 0; i != num_threads; ++i)
            {
                auto const thread_mask = rp.get_pu_mask(wrapped_pu_num(
                    static_cast<std::uint32_t>(i + get_first_core()),
                    needs_wraparound, available_threads));
                for (std::uint32_t j = 0; j != overall_threads; ++j)
                {
                    if (threads::test(thread_mask, j))
                    {
                        threads::set(mask, j);
                    }
                }
            }

            return mask;
        }

        threads::thread_pool_base* pool() const
        {
            return pool_ ? pool_ : threads::detail::get_self_or_default_pool();
        }

    public:
        /// \cond NOINTERNAL
        threads::thread_pool_base* pool_ = nullptr;
        Policy policy_;
        std::size_t first_core_ = 0;
        std::size_t num_cores_ = 0;
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
        char const* annotation_ = nullptr;
#endif
        /// \endcond
    };

    ////////////////////////////////////////////////////////////////////////////
    // parallel executor that uses a flat index_queue for spawning threads
    HPX_CXX_CORE_EXPORT template <typename Policy,
        bool HierarchicalSpawning = false>
    struct parallel_policy_executor : parallel_policy_executor_base<Policy>
    {
        using base_type = parallel_policy_executor_base<Policy>;

        // Create a new parallel executor
        constexpr explicit parallel_policy_executor(
            threads::thread_priority priority,
            threads::thread_stacksize stacksize =
                threads::thread_stacksize::default_,
            threads::thread_schedule_hint schedulehint = {},
            Policy l = parallel::execution::detail::get_default_policy<
                Policy>::call()) noexcept
          : base_type(priority, stacksize, schedulehint, l)
        {
        }

        constexpr explicit parallel_policy_executor(
            threads::thread_stacksize stacksize,
            threads::thread_schedule_hint schedulehint = {},
            Policy l = parallel::execution::detail::get_default_policy<
                Policy>::call()) noexcept
          : base_type(stacksize, schedulehint, l)
        {
        }

        constexpr explicit parallel_policy_executor(
            threads::thread_schedule_hint schedulehint,
            Policy l = parallel::execution::detail::get_default_policy<
                Policy>::call()) noexcept
          : base_type(schedulehint, l)
        {
        }

        constexpr explicit parallel_policy_executor(Policy l) noexcept
          : base_type(l)
        {
        }

        constexpr parallel_policy_executor() noexcept = default;

        constexpr explicit parallel_policy_executor(
            threads::thread_pool_base* pool, Policy l) noexcept
          : base_type(pool, l)
        {
        }

        constexpr explicit parallel_policy_executor(
            threads::thread_pool_base* pool,
            threads::thread_priority priority =
                threads::thread_priority::default_,
            threads::thread_stacksize stacksize =
                threads::thread_stacksize::default_,
            threads::thread_schedule_hint schedulehint = {},
            Policy l = parallel::execution::detail::get_default_policy<
                Policy>::call()) noexcept
          : base_type(pool, priority, stacksize, schedulehint, l)
        {
        }

        parallel_policy_executor(parallel_policy_executor const&) = default;
        parallel_policy_executor(parallel_policy_executor&&) = default;
        parallel_policy_executor& operator=(
            parallel_policy_executor const&) = default;
        parallel_policy_executor& operator=(
            parallel_policy_executor&&) = default;

#if defined(__NVCC__) || defined(__CUDACC__)
        constexpr ~parallel_policy_executor() {}
#endif

    private:
        // property implementations
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
        template <typename Executor_>
            requires(std::is_convertible_v<Executor_, parallel_policy_executor>)
        friend constexpr auto tag_invoke(
            hpx::execution::experimental::with_annotation_t,
            Executor_ const& exec, char const* annotation)
        {
            auto exec_with_annotation = exec;
            exec_with_annotation.annotation_ = annotation;
            return exec_with_annotation;
        }

        template <typename Executor_>
            requires(std::is_convertible_v<Executor_, parallel_policy_executor>)
        friend auto tag_invoke(hpx::execution::experimental::with_annotation_t,
            Executor_ const& exec, std::string annotation)
        {
            auto exec_with_annotation = exec;
            exec_with_annotation.annotation_ =
                hpx::detail::store_function_annotation(HPX_MOVE(annotation));
            return exec_with_annotation;
        }

        friend constexpr char const* tag_invoke(
            hpx::execution::experimental::get_annotation_t,
            parallel_policy_executor const& exec) noexcept
        {
            return exec.annotation_;
        }
#endif

        template <typename Executor_>
            requires(std::is_convertible_v<Executor_, parallel_policy_executor>)
        friend auto tag_invoke(
            hpx::execution::experimental::with_processing_units_count_t,
            Executor_ const& exec, std::size_t num_cores)
        {
            if (num_cores == 0)
            {
                num_cores = exec.pool()->get_active_os_thread_count();
            }

            auto exec_with_num_cores = exec;
            exec_with_num_cores.num_cores_ = num_cores;
            return exec_with_num_cores;
        }

        template <typename Parameters>
            requires(hpx::traits::is_executor_parameters_v<Parameters>)
        friend constexpr std::size_t tag_invoke(
            hpx::execution::experimental::processing_units_count_t,
            Parameters&&, parallel_policy_executor const& exec,
            hpx::chrono::steady_duration const& = hpx::chrono::null_duration,
            std::size_t = 0)
        {
            return exec.get_num_cores();
        }

        template <typename Executor_>
            requires(std::is_convertible_v<Executor_, parallel_policy_executor>)
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
            parallel_policy_executor const& exec) noexcept
        {
            return exec.get_first_core();
        }

        friend auto tag_invoke(
            hpx::execution::experimental::get_processing_units_mask_t,
            parallel_policy_executor const& exec)
        {
            return exec.pu_mask();
        }

        friend auto tag_invoke(hpx::execution::experimental::get_cores_mask_t,
            parallel_policy_executor const& exec)
        {
            return exec.pool()->get_used_processing_units(
                exec.get_num_cores(), true);
        }

        template <typename F, typename... Ts>
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::sync_execute_t,
            parallel_policy_executor const& exec, F&& f, Ts&&... ts)
        {
            return exec.sync_impl(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename... Ts>
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::async_execute_t,
            parallel_policy_executor const& exec, F&& f, Ts&&... ts)
        {
            return exec.async_impl(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename Future, typename... Ts>
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::then_execute_t,
            parallel_policy_executor const& exec, F&& f, Future&& predecessor,
            Ts&&... ts)
        {
            return exec.then_impl(HPX_FORWARD(F, f),
                HPX_FORWARD(Future, predecessor), HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename... Ts>
        friend void tag_invoke(hpx::parallel::execution::post_t,
            parallel_policy_executor const& exec, F&& f, Ts&&... ts)
        {
            exec.post_impl(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename S, typename Future, typename... Ts>
            requires(!std::is_integral_v<S>)
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::bulk_then_execute_t,
            parallel_policy_executor const& exec, F&& f, S const& shape,
            Future&& predecessor, Ts&&... ts)
        {
            return base_type::bulk_then_impl(exec, HPX_FORWARD(F, f), shape,
                HPX_FORWARD(Future, predecessor), HPX_FORWARD(Ts, ts)...);
        }

        // BulkTwoWayExecutor interface
        template <typename F, typename S, typename... Ts>
            requires(!std::is_integral_v<S>)
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::bulk_sync_execute_t,
            parallel_policy_executor const& exec, F&& f, S const& shape,
            Ts&&... ts)
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            hpx::threads::thread_description desc(f, exec.annotation_);
#else
            hpx::threads::thread_description desc(f);
#endif

            // use scheduling based on index_queue if no hierarchical threshold
            // is given
            HPX_ASSERT(!hpx::threads::do_not_combine_tasks(
                exec.policy().get_hint().sharing_mode()));

            return parallel::execution::detail::index_queue_bulk_sync_execute(
                desc, exec.pool(), exec.get_first_core(), exec.get_num_cores(),
                exec.policy_, HPX_FORWARD(F, f), shape, exec.pu_mask(),
                HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename S, typename... Ts>
            requires(!std::is_integral_v<S>)
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::bulk_async_execute_t,
            parallel_policy_executor const& exec, F&& f, S const& shape,
            Ts&&... ts)
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            hpx::threads::thread_description desc(f, exec.annotation_);
#else
            hpx::threads::thread_description desc(f);
#endif

            // use scheduling based on index_queue only if no hierarchical
            // threshold is given
            HPX_ASSERT(!hpx::threads::do_not_combine_tasks(
                exec.policy().get_hint().sharing_mode()));

            return parallel::execution::detail::index_queue_bulk_async_execute(
                desc, exec.pool(), exec.get_first_core(), exec.get_num_cores(),
                exec.policy_, HPX_FORWARD(F, f), shape, exec.pu_mask(),
                HPX_FORWARD(Ts, ts)...);
        }

        // map execution policy categories to proper executor
        friend decltype(auto) tag_invoke(
            hpx::execution::experimental::to_non_par_t,
            parallel_policy_executor const& exec)
        {
            if constexpr (std::is_same_v<Policy, launch::sync_policy>)
            {
                return exec;
            }
            else
            {
                auto non_par_exec =
                    parallel_policy_executor<launch::sync_policy>(exec.pool_,
                        launch::sync_policy(exec.policy_.priority(),
                            exec.policy_.stacksize(), exec.policy_.hint()));

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
                return hpx::execution::experimental::with_annotation(
                    HPX_MOVE(non_par_exec), exec.annotation_);
#else
                return non_par_exec;
#endif
            }
        }
        /// \endcond

    public:
        /// \cond NOINTERNAL
        constexpr bool operator==(
            parallel_policy_executor const& rhs) const noexcept
        {
            return base_type::policy_ == rhs.policy_ &&
                base_type::pool_ == rhs.pool;
        }

        constexpr bool operator!=(
            parallel_policy_executor const& rhs) const noexcept
        {
            return !(*this == rhs);
        }

        [[nodiscard]] constexpr parallel_policy_executor const& context()
            const noexcept
        {
            return *this;
        }
        /// \endcond

    private:
        /// \cond NOINTERNAL
        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, unsigned int const /* version */)
        {
            // clang-format off
            ar & base_type::policy_ & base_type::first_core_
               & base_type::num_cores_;
            // clang-format on
        }
        /// \endcond
    };

    ////////////////////////////////////////////////////////////////////////////
    // parallel executor that spawns threads hierarchically
    HPX_CXX_CORE_EXPORT template <typename Policy>
    struct parallel_policy_executor<Policy, true>
      : parallel_policy_executor_base<Policy>
    {
        using base_type = parallel_policy_executor_base<Policy>;

        // Create a new parallel executor
        constexpr explicit parallel_policy_executor(
            threads::thread_priority priority,
            threads::thread_stacksize stacksize =
                threads::thread_stacksize::default_,
            threads::thread_schedule_hint schedulehint = {},
            Policy l =
                parallel::execution::detail::get_default_policy<Policy>::call(),
            std::size_t const hierarchical_threshold =
                hierarchical_threshold_default_) noexcept
          : base_type(priority, stacksize, schedulehint, l)
          , hierarchical_threshold_(hierarchical_threshold)
        {
        }

        constexpr explicit parallel_policy_executor(
            threads::thread_stacksize stacksize,
            threads::thread_schedule_hint schedulehint = {},
            Policy l = parallel::execution::detail::get_default_policy<
                Policy>::call()) noexcept
          : base_type(stacksize, schedulehint, l)
        {
        }

        constexpr explicit parallel_policy_executor(
            threads::thread_schedule_hint schedulehint,
            Policy l = parallel::execution::detail::get_default_policy<
                Policy>::call()) noexcept
          : base_type(schedulehint, l)
        {
        }

        constexpr explicit parallel_policy_executor(Policy l) noexcept
          : base_type(l)
        {
        }

        constexpr parallel_policy_executor() noexcept = default;

        constexpr explicit parallel_policy_executor(
            threads::thread_pool_base* pool, Policy l,
            std::size_t const hierarchical_threshold =
                hierarchical_threshold_default_) noexcept
          : base_type(pool, l)
          , hierarchical_threshold_(hierarchical_threshold)
        {
        }

        constexpr explicit parallel_policy_executor(
            threads::thread_pool_base* pool,
            threads::thread_priority priority =
                threads::thread_priority::default_,
            threads::thread_stacksize stacksize =
                threads::thread_stacksize::default_,
            threads::thread_schedule_hint schedulehint = {},
            Policy l =
                parallel::execution::detail::get_default_policy<Policy>::call(),
            std::size_t const hierarchical_threshold =
                hierarchical_threshold_default_) noexcept
          : base_type(pool, priority, stacksize, schedulehint, l)
          , hierarchical_threshold_(hierarchical_threshold)
        {
        }

        constexpr void set_hierarchical_threshold(
            std::size_t const threshold) noexcept
        {
            hierarchical_threshold_ = threshold;
        }

        parallel_policy_executor(parallel_policy_executor const&) = default;
        parallel_policy_executor(parallel_policy_executor&&) = default;
        parallel_policy_executor& operator=(
            parallel_policy_executor const&) = default;
        parallel_policy_executor& operator=(
            parallel_policy_executor&&) = default;

        ~parallel_policy_executor() = default;

    private:
        // property implementations

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
        template <typename Executor_>
            requires(std::is_convertible_v<Executor_, parallel_policy_executor>)
        friend constexpr auto tag_invoke(
            hpx::execution::experimental::with_annotation_t,
            Executor_ const& exec, char const* annotation)
        {
            auto exec_with_annotation = exec;
            exec_with_annotation.annotation_ = annotation;
            return exec_with_annotation;
        }

        template <typename Executor_>
            requires(std::is_convertible_v<Executor_, parallel_policy_executor>)
        friend auto tag_invoke(hpx::execution::experimental::with_annotation_t,
            Executor_ const& exec, std::string annotation)
        {
            auto exec_with_annotation = exec;
            exec_with_annotation.annotation_ =
                hpx::detail::store_function_annotation(HPX_MOVE(annotation));
            return exec_with_annotation;
        }

        friend constexpr char const* tag_invoke(
            hpx::execution::experimental::get_annotation_t,
            parallel_policy_executor const& exec) noexcept
        {
            return exec.annotation_;
        }
#endif

        template <typename Executor_>
            requires(std::is_convertible_v<Executor_, parallel_policy_executor>)
        friend auto tag_invoke(
            hpx::execution::experimental::with_processing_units_count_t,
            Executor_ const& exec, std::size_t num_cores)
        {
            if (num_cores == 0)
            {
                num_cores = exec.pool()->get_active_os_thread_count();
            }

            auto exec_with_num_cores = exec;
            exec_with_num_cores.num_cores_ = num_cores;
            return exec_with_num_cores;
        }

        template <typename Parameters>
            requires(hpx::traits::is_executor_parameters_v<Parameters>)
        friend constexpr std::size_t tag_invoke(
            hpx::execution::experimental::processing_units_count_t,
            Parameters&&, parallel_policy_executor const& exec,
            hpx::chrono::steady_duration const& = hpx::chrono::null_duration,
            std::size_t = 0)
        {
            return exec.get_num_cores();
        }

        template <typename Executor_>
            requires(std::is_convertible_v<Executor_, parallel_policy_executor>)
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
            parallel_policy_executor const& exec) noexcept
        {
            return exec.get_first_core();
        }

        friend auto tag_invoke(
            hpx::execution::experimental::get_processing_units_mask_t,
            parallel_policy_executor const& exec)
        {
            return exec.pu_mask();
        }

        friend auto tag_invoke(hpx::execution::experimental::get_cores_mask_t,
            parallel_policy_executor const& exec)
        {
            return exec.pool()->get_used_processing_units(
                exec.get_num_cores(), true);
        }

        template <typename F, typename... Ts>
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::sync_execute_t,
            parallel_policy_executor const& exec, F&& f, Ts&&... ts)
        {
            return exec.sync_impl(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename... Ts>
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::async_execute_t,
            parallel_policy_executor const& exec, F&& f, Ts&&... ts)
        {
            return exec.async_impl(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename Future, typename... Ts>
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::then_execute_t,
            parallel_policy_executor const& exec, F&& f, Future&& predecessor,
            Ts&&... ts)
        {
            return exec.then_impl(HPX_FORWARD(F, f),
                HPX_FORWARD(Future, predecessor), HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename... Ts>
        friend void tag_invoke(hpx::parallel::execution::post_t,
            parallel_policy_executor const& exec, F&& f, Ts&&... ts)
        {
            exec.post_impl(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename S, typename Future, typename... Ts>
            requires(!std::is_integral_v<S>)
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::bulk_then_execute_t,
            parallel_policy_executor const& exec, F&& f, S const& shape,
            Future&& predecessor, Ts&&... ts)
        {
            return base_type::bulk_then_impl(exec, HPX_FORWARD(F, f), shape,
                HPX_FORWARD(Future, predecessor), HPX_FORWARD(Ts, ts)...);
        }

        // BulkTwoWayExecutor interface
        template <typename F, typename S, typename... Ts>
            requires(!std::is_integral_v<S>)
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::bulk_sync_execute_t,
            parallel_policy_executor const& exec, F&& f, S const& shape,
            Ts&&... ts)
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            hpx::threads::thread_description desc(f, exec.annotation_);
#else
            hpx::threads::thread_description desc(f);
#endif

            return parallel::execution::detail::hierarchical_bulk_sync_execute(
                desc, exec.pool(), exec.get_first_core(), exec.get_num_cores(),
                exec.hierarchical_threshold_, exec.policy_, HPX_FORWARD(F, f),
                shape, HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename S, typename... Ts>
            requires(!std::is_integral_v<S>)
        friend decltype(auto) tag_invoke(
            hpx::parallel::execution::bulk_async_execute_t,
            parallel_policy_executor const& exec, F&& f, S const& shape,
            Ts&&... ts)
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            hpx::threads::thread_description desc(f, exec.annotation_);
#else
            hpx::threads::thread_description desc(f);
#endif

            return parallel::execution::detail::hierarchical_bulk_async_execute(
                desc, exec.pool(), exec.get_first_core(), exec.get_num_cores(),
                exec.hierarchical_threshold_, exec.policy_, HPX_FORWARD(F, f),
                shape, HPX_FORWARD(Ts, ts)...);
        }

        // map execution policy categories to proper executor
        friend decltype(auto) tag_invoke(
            hpx::execution::experimental::to_non_par_t,
            parallel_policy_executor const& exec)
        {
            if constexpr (std::is_same_v<Policy, launch::sync_policy>)
            {
                return exec;
            }
            else
            {
                auto non_par_exec =
                    parallel_policy_executor<launch::sync_policy, true>(
                        exec.pool_,
                        launch::sync_policy(exec.policy_.priority(),
                            exec.policy_.stacksize(), exec.policy_.hint()),
                        exec.hierarchical_threshold_);

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
                return hpx::execution::experimental::with_annotation(
                    HPX_MOVE(non_par_exec), exec.annotation_);
#else
                return non_par_exec;
#endif
            }
        }
        /// \endcond

    public:
        /// \cond NOINTERNAL
        constexpr bool operator==(
            parallel_policy_executor const& rhs) const noexcept
        {
            return base_type::policy_ == rhs.policy_ &&
                base_type::pool_ == rhs.pool_ &&
                hierarchical_threshold_ == rhs.hierarchical_threshold_;
        }

        constexpr bool operator!=(
            parallel_policy_executor const& rhs) const noexcept
        {
            return !(*this == rhs);
        }

        [[nodiscard]] constexpr parallel_policy_executor const& context()
            const noexcept
        {
            return *this;
        }

    private:
        /// \cond NOINTERNAL
        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, unsigned int const /* version */)
        {
            // clang-format off
            ar & base_type::policy_ & hierarchical_threshold_
               & base_type::first_core_ & base_type::num_cores_;
            // clang-format on
        }
        /// \endcond

    private:
        /// \cond NOINTERNAL
        static constexpr std::size_t hierarchical_threshold_default_ = 7;
        std::size_t hierarchical_threshold_ = hierarchical_threshold_default_;
        /// \endcond
    };

    ////////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Policy>
    constexpr parallel_policy_executor<Policy, true> to_hierarchical_spawning(
        parallel_policy_executor<Policy>& exec) noexcept
    {
        parallel_policy_executor<Policy, true> ret(exec.pool_, exec.policy_);
        ret.first_core_ = exec.first_core_;
        ret.num_cores_ = exec.num_cores_;
        return ret;
    }

    HPX_CXX_CORE_EXPORT template <typename Policy>
    constexpr parallel_policy_executor<Policy, true> to_hierarchical_spawning(
        parallel_policy_executor<Policy>&& exec) noexcept
    {
        parallel_policy_executor<Policy, true> ret(exec.pool_, exec.policy_);
        ret.first_core_ = exec.first_core_;
        ret.num_cores_ = exec.num_cores_;
        return ret;
    }

    HPX_CXX_CORE_EXPORT template <typename Policy>
    constexpr parallel_policy_executor<Policy, true> to_hierarchical_spawning(
        parallel_policy_executor<Policy> const& exec) noexcept
    {
        parallel_policy_executor<Policy, true> ret(exec.pool_, exec.policy_);
        ret.first_core_ = exec.first_core_;
        ret.num_cores_ = exec.num_cores_;
        return ret;
    }

    HPX_CXX_CORE_EXPORT template <typename Executor>
    constexpr Executor to_hierarchical_spawning(Executor&& exec) noexcept
    {
        return HPX_FORWARD(Executor, exec);
    }

    HPX_CXX_CORE_EXPORT template <typename Policy>
    constexpr parallel_policy_executor<Policy> to_non_hierarchical_spawning(
        parallel_policy_executor<Policy, true>& exec) noexcept
    {
        parallel_policy_executor<Policy> ret(exec.pool_, exec.policy_);
        ret.first_core_ = exec.first_core_;
        ret.num_cores_ = exec.num_cores_;
        return ret;
    }

    HPX_CXX_CORE_EXPORT template <typename Policy>
    constexpr parallel_policy_executor<Policy> to_non_hierarchical_spawning(
        parallel_policy_executor<Policy, true>&& exec) noexcept
    {
        parallel_policy_executor<Policy> ret(exec.pool_, exec.policy_);
        ret.first_core_ = exec.first_core_;
        ret.num_cores_ = exec.num_cores_;
        return ret;
    }

    HPX_CXX_CORE_EXPORT template <typename Policy>
    constexpr parallel_policy_executor<Policy> to_non_hierarchical_spawning(
        parallel_policy_executor<Policy, true> const& exec) noexcept
    {
        parallel_policy_executor<Policy> ret(exec.pool_, exec.policy_);
        ret.first_core_ = exec.first_core_;
        ret.num_cores_ = exec.num_cores_;
        return ret;
    }

    HPX_CXX_CORE_EXPORT template <typename Executor>
    constexpr Executor to_non_hierarchical_spawning(Executor&& exec) noexcept
    {
        return HPX_FORWARD(Executor, exec);
    }

    ////////////////////////////////////////////////////////////////////////////
    // support all properties exposed by the embedded policy
    HPX_CXX_CORE_EXPORT template <typename Tag, typename Policy,
        bool HierarchicalSpawning, typename Property,
        HPX_CONCEPT_REQUIRES_(
            hpx::execution::experimental::is_scheduling_property_v<Tag>)>
    auto tag_invoke(Tag tag,
        parallel_policy_executor<Policy, HierarchicalSpawning> const& exec,
        Property&& prop)
        -> decltype(std::declval<parallel_policy_executor<Policy,
                        HierarchicalSpawning>>()
                        .policy(std::declval<Tag>()(
                            std::declval<Policy>(), std::declval<Property>())),
            parallel_policy_executor<Policy, HierarchicalSpawning>())
    {
        auto exec_with_prop = exec;
        exec_with_prop.policy(tag(exec.policy(), HPX_FORWARD(Property, prop)));
        return exec_with_prop;
    }

    HPX_CXX_CORE_EXPORT template <typename Tag, typename Policy,
        bool HierarchicalSpawning,
        HPX_CONCEPT_REQUIRES_(
            hpx::execution::experimental::is_scheduling_property_v<Tag>)>
    auto tag_invoke(Tag tag,
        parallel_policy_executor<Policy, HierarchicalSpawning> const& exec)
        -> decltype(std::declval<Tag>()(std::declval<Policy>()))
    {
        return tag(exec.policy());
    }

    HPX_CXX_CORE_EXPORT using parallel_executor =
        parallel_policy_executor<hpx::launch>;

    HPX_CXX_CORE_EXPORT using parallel_executor_spawn_hierarchically =
        parallel_policy_executor<hpx::launch, true>;
}    // namespace hpx::execution

namespace hpx::execution::experimental {

    /// \cond NOINTERNAL
    HPX_CXX_CORE_EXPORT template <typename Policy, bool HierarchicalSpawning>
    struct is_one_way_executor<
        hpx::execution::parallel_policy_executor<Policy, HierarchicalSpawning>>
      : std::true_type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Policy, bool HierarchicalSpawning>
    struct is_never_blocking_one_way_executor<
        hpx::execution::parallel_policy_executor<Policy, HierarchicalSpawning>>
      : std::true_type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Policy, bool HierarchicalSpawning>
    struct is_two_way_executor<
        hpx::execution::parallel_policy_executor<Policy, HierarchicalSpawning>>
      : std::true_type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Policy, bool HierarchicalSpawning>
    struct is_bulk_one_way_executor<
        hpx::execution::parallel_policy_executor<Policy, HierarchicalSpawning>>
      : std::true_type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Policy, bool HierarchicalSpawning>
    struct is_bulk_two_way_executor<
        hpx::execution::parallel_policy_executor<Policy, HierarchicalSpawning>>
      : std::true_type
    {
    };
    /// \endcond
}    // namespace hpx::execution::experimental
