//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/executors/sequenced_executor.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/executors/execution_policy_mappings.hpp>
#include <hpx/executors/executor_scheduler.hpp>
#include <hpx/executors/parallel_executor.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/pack_traversal.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/topology.hpp>

#include <cstddef>
#include <iterator>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace hpx::execution {

    ///////////////////////////////////////////////////////////////////////////
    /// A \a sequential_executor creates groups of sequential execution agents
    /// which execute in the calling thread. The sequential order is given by
    /// the lexicographical order of indices in the index space.
    ///
    HPX_CXX_CORE_EXPORT struct sequenced_executor
    {
        /// \cond NOINTERNAL
        bool operator==(sequenced_executor const& /*rhs*/) const noexcept
        {
            return true;
        }

        bool operator!=(sequenced_executor const& /*rhs*/) const noexcept
        {
            return false;
        }

        sequenced_executor const& context() const noexcept
        {
            return *this;
        }

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
        [[nodiscard]] auto query(
            experimental::with_annotation_t, char const* annotation) const
        {
            auto exec_with_annotation = *this;
            exec_with_annotation.annotation_ = annotation;
            return exec_with_annotation;
        }

        [[nodiscard]] auto query(
            experimental::with_annotation_t, std::string annotation) const
        {
            auto exec_with_annotation = *this;
            exec_with_annotation.annotation_ =
                hpx::detail::store_function_annotation(HPX_MOVE(annotation));
            return exec_with_annotation;
        }

        [[nodiscard]] constexpr char const* query(
            experimental::get_annotation_t) const noexcept
        {
            return annotation_;
        }
#endif

        template <typename Parameters>
            requires(hpx::executor_parameters<Parameters>)
        [[nodiscard]] constexpr std::size_t query(
            experimental::processing_units_count_t, Parameters&&,
            hpx::chrono::steady_duration const& = hpx::chrono::null_duration,
            std::size_t = 0) const
        {
            return 1;
        }

        [[nodiscard]] auto query(
            experimental::get_processing_units_mask_t) const
        {
            return threads::detail::get_self_or_default_pool()
                ->get_used_processing_unit(hpx::get_worker_thread_num(), false);
        }

        [[nodiscard]] auto query(experimental::get_cores_mask_t) const
        {
            return threads::detail::get_self_or_default_pool()
                ->get_used_processing_unit(hpx::get_worker_thread_num(), true);
        }

        /// \endcond

        /// \cond NOINTERNAL
        using execution_category = sequenced_execution_tag;

        // OneWayExecutor interface
        template <typename F, typename... Ts>
        decltype(auto) sync_execute(F&& f, Ts&&... ts) const
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            hpx::scoped_annotation annotate(annotation_ ?
                    annotation_ :
                    "parallel_policy_executor::sync_execute");
#endif
            return sync_execute_impl(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        // TwoWayExecutor interface
        template <typename F, typename... Ts>
        decltype(auto) async_execute(F&& f, Ts&&... ts) const
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            hpx::threads::thread_description desc(f, annotation_);
#else
            hpx::threads::thread_description desc(f);
#endif
            return async_execute_impl(
                desc, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        // NonBlockingOneWayExecutor (adapted) interface
        template <typename F, typename... Ts>
        void post(F&& f, Ts&&... ts) const
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            hpx::scoped_annotation annotate(annotation_ ?
                    annotation_ :
                    "parallel_policy_executor::sync_execute");
#endif
            sync_execute_impl(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        // BulkTwoWayExecutor interface
        template <typename F, typename S, typename... Ts>
            requires(!std::is_integral_v<S>)
        decltype(auto) bulk_async_execute(
            F&& f, S const& shape, Ts&&... ts) const
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            hpx::threads::thread_description desc(f, annotation_);
#else
            hpx::threads::thread_description desc(f);
#endif

            using result_type =
                parallel::execution::detail::bulk_function_result_t<F, S,
                    Ts...>;
            std::vector<hpx::future<result_type>> results;

            try
            {
                for (auto const& elem : shape)
                {
                    results.push_back(async_execute_impl(desc, f, elem, ts...));
                }
            }
            catch (std::bad_alloc const&)
            {
                throw;
            }
            catch (...)
            {
                throw exception_list(std::current_exception());
            }

            return results;
        }

        template <typename F, typename S, typename... Ts>
            requires(!std::is_integral_v<S>)
        decltype(auto) bulk_sync_execute(
            F&& f, S const& shape, Ts&&... ts) const
        {
            return hpx::unwrap(bulk_async_execute(
                HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...));
        }

        decltype(auto) to_par() const
        {
#if defined(HPX_HAVE_THREAD_DESCRIPTION)
            return hpx::execution::experimental::with_annotation(
                parallel_executor(), annotation_);
#else
            return parallel_executor();
#endif
        }

    public:
        /// \cond NOINTERNAL
        constexpr hpx::execution::experimental::executor_scheduler<
            sequenced_executor>
        query(hpx::execution::experimental::get_scheduler_t) const noexcept
        {
            return hpx::execution::experimental::executor_scheduler<
                sequenced_executor>(*this);
        }
        /// \endcond

    private:
        template <typename F, typename... Ts>
        static decltype(auto) sync_execute_impl(F&& f, Ts&&... ts)
        {
            return hpx::detail::sync_launch_policy_dispatch<
                launch::sync_policy>::call(launch::sync, HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename... Ts>
        static decltype(auto) async_execute_impl(
            hpx::threads::thread_description const& desc, F&& f, Ts&&... ts)
        {
            return hpx::detail::async_launch_policy_dispatch<
                launch::deferred_policy>::call(launch::deferred, desc,
                HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        friend class hpx::serialization::access;

        template <typename Archive>
        static constexpr void serialize(Archive&, unsigned int const) noexcept
        {
        }

#if defined(HPX_HAVE_THREAD_DESCRIPTION)
        char const* annotation_ = nullptr;
#endif
        /// \endcond
    };
}    // namespace hpx::execution

namespace hpx::execution::experimental {

    /// \cond NOINTERNAL
    template <>
    struct is_one_way_executor<hpx::execution::sequenced_executor>
      : std::true_type
    {
    };

    template <>
    struct is_bulk_one_way_executor<hpx::execution::sequenced_executor>
      : std::true_type
    {
    };

    template <>
    struct is_never_blocking_one_way_executor<
        hpx::execution::sequenced_executor> : std::true_type
    {
    };

    template <>
    struct is_two_way_executor<hpx::execution::sequenced_executor>
      : std::true_type
    {
    };

    template <>
    struct is_bulk_two_way_executor<hpx::execution::sequenced_executor>
      : std::true_type
    {
    };
    /// \endcond
}    // namespace hpx::execution::experimental
