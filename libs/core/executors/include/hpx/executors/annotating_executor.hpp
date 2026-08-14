//  Copyright (c) 2021-2024 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file hpx/executors/annotating_executor.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/topology.hpp>

#include <concepts>
#include <string>
#include <type_traits>
#include <utility>

namespace hpx::execution::experimental {

    ///////////////////////////////////////////////////////////////////////////
    /// An \a annotating_executor wraps any other executor and adds the
    /// capability to add annotations to the launched threads.
    HPX_CXX_CORE_EXPORT template <executor_any BaseExecutor>
    struct annotating_executor
    {
        template <executor_any Executor>
            requires(!std::same_as<std::decay_t<Executor>, annotating_executor>)
        constexpr explicit annotating_executor(
            Executor&& exec, char const* annotation = nullptr)
          : exec_(HPX_FORWARD(Executor, exec))
          , annotation_(annotation)
        {
        }

        template <executor_any Executor>
        explicit annotating_executor(Executor&& exec, std::string annotation)
          : exec_(HPX_FORWARD(Executor, exec))
          , annotation_(
                hpx::detail::store_function_annotation(HPX_MOVE(annotation)))
        {
        }

        /// \cond NOINTERNAL
        constexpr bool operator==(annotating_executor const& rhs) const noexcept
        {
            return exec_ == rhs.exec_;
        }

        constexpr bool operator!=(annotating_executor const& rhs) const noexcept
        {
            return exec_ != rhs.exec_;
        }

        [[nodiscard]] constexpr auto const& context() const noexcept
        {
            return exec_.context();
        }

        [[nodiscard]] constexpr std::decay_t<BaseExecutor> const& get_executor()
            const noexcept
        {
            return exec_;
        }

        using execution_category =
            hpx::traits::executor_execution_category_t<BaseExecutor>;

        using parameters_type =
            hpx::traits::executor_parameters_type_t<BaseExecutor>;

        template <typename T, typename... Ts>
        using future_type =
            hpx::traits::executor_future_t<BaseExecutor, T, Ts...>;

        [[nodiscard]] constexpr annotating_executor query(
            with_annotation_t, char const* annotation) const
        {
            auto exec_with_annotation = *this;
            exec_with_annotation.annotation_ = annotation;
            return exec_with_annotation;
        }

        [[nodiscard]] annotating_executor query(
            with_annotation_t, std::string annotation) const
        {
            auto exec_with_annotation = *this;
            exec_with_annotation.annotation_ =
                hpx::detail::store_function_annotation(HPX_MOVE(annotation));
            return exec_with_annotation;
        }

        [[nodiscard]] constexpr char const* query(
            get_annotation_t) const noexcept
        {
            return annotation_;
        }

        template <typename Tag, typename Property>
            requires(is_scheduling_property_v<Tag>)
        [[nodiscard]] auto query(Tag tag, Property&& prop) const
        {
            return annotating_executor(
                exec_.query(tag, HPX_FORWARD(Property, prop)), annotation_);
        }

        template <typename Tag>
            requires(is_scheduling_property_v<Tag>)
        [[nodiscard]] auto query(Tag tag) const
        {
            return exec_.query(tag);
        }

        // NonBlockingOneWayExecutor interface
        template <typename F, typename... Ts>
            requires(std::invocable<F, Ts...>)
        decltype(auto) post(F&& f, Ts&&... ts) const
        {
            return parallel::execution::post(exec_,
                hpx::annotated_function(HPX_FORWARD(F, f), annotation_),
                HPX_FORWARD(Ts, ts)...);
        }

        // OneWayExecutor interface
        template <typename F, typename... Ts>
            requires(std::invocable<F, Ts...>)
        decltype(auto) sync_execute(F&& f, Ts&&... ts) const
        {
            return parallel::execution::sync_execute(exec_,
                hpx::annotated_function(HPX_FORWARD(F, f), annotation_),
                HPX_FORWARD(Ts, ts)...);
        }

        // TwoWayExecutor interface
        template <typename F, typename... Ts>
            requires(std::invocable<F, Ts...>)
        decltype(auto) async_execute(F&& f, Ts&&... ts) const
        {
            return parallel::execution::async_execute(exec_,
                hpx::annotated_function(HPX_FORWARD(F, f), annotation_),
                HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename Future, typename... Ts>
            requires(std::invocable<F, Ts...>)
        decltype(auto) then_execute(
            F&& f, Future&& predecessor, Ts&&... ts) const
        {
            return parallel::execution::then_execute(exec_,
                hpx::annotated_function(HPX_FORWARD(F, f), annotation_),
                HPX_FORWARD(Future, predecessor), HPX_FORWARD(Ts, ts)...);
        }

        // BulkTwoWayExecutor interface
        template <typename F, typename S, typename... Ts>
        decltype(auto) bulk_async_execute(
            F&& f, S const& shape, Ts&&... ts) const
        {
            return parallel::execution::bulk_async_execute(exec_,
                hpx::annotated_function(HPX_FORWARD(F, f), annotation_), shape,
                HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename S, typename... Ts>
        decltype(auto) bulk_sync_execute(
            F&& f, S const& shape, Ts&&... ts) const
        {
            return parallel::execution::bulk_sync_execute(exec_,
                hpx::annotated_function(HPX_FORWARD(F, f), annotation_), shape,
                HPX_FORWARD(Ts, ts)...);
        }

        template <typename F, typename S, typename Future, typename... Ts>
        decltype(auto) bulk_then_execute(
            F&& f, S const& shape, Future&& predecessor, Ts&&... ts) const
        {
            return parallel::execution::bulk_then_execute(exec_,
                hpx::annotated_function(HPX_FORWARD(F, f), annotation_), shape,
                HPX_FORWARD(Future, predecessor), HPX_FORWARD(Ts, ts)...);
        }

    private:
        friend class hpx::serialization::access;

        template <typename Archive>
        void serialize(Archive& ar, unsigned int const /* version */)
        {
            // clang-format off
            ar & exec_;
            // clang-format on
        }

        std::decay_t<BaseExecutor> exec_;
        char const* annotation_ = nullptr;
        /// \endcond
    };

    // All properties exposed by the wrapped executor are supported through the
    // public query() member functions above, which the scheduling property
    // CPOs detect directly (via property_base). No tag_invoke bridge needed.

    ///////////////////////////////////////////////////////////////////////////
#if !defined(DOXYGEN)    // doxygen gets confused by the deduction guides
    HPX_CXX_CORE_EXPORT template <typename BaseExecutor>
    explicit annotating_executor(BaseExecutor&& sched, std::string annotation)
        -> annotating_executor<std::decay_t<BaseExecutor>>;

    HPX_CXX_CORE_EXPORT template <typename BaseExecutor>
    explicit annotating_executor(
        BaseExecutor&& sched, char const* annotation = nullptr)
        -> annotating_executor<std::decay_t<BaseExecutor>>;
#endif

    ///////////////////////////////////////////////////////////////////////////
    // if the given executor does not support annotations, wrap it into
    // an annotating_executor
    //
    // The functions below are used for executors that do not directly support
    // annotations. Those are wrapped into an annotating_executor if passed
    // to `with_annotation`.
    //
    namespace detail {

        template <executor_any Executor>
        auto wrap_with_annotation(Executor&& exec, char const* annotation)
        {
            return annotating_executor<std::decay_t<Executor>>(
                HPX_FORWARD(Executor, exec), annotation);
        }

        template <executor_any Executor>
        auto wrap_with_annotation(Executor&& exec, std::string annotation)
        {
            return annotating_executor<std::decay_t<Executor>>(
                HPX_FORWARD(Executor, exec), HPX_MOVE(annotation));
        }
    }    // namespace detail
}    // namespace hpx::execution::experimental

namespace hpx::execution::experimental {

    // The annotating executor exposes the same executor categories as its
    // underlying (wrapped) executor.

    /// \cond NOINTERNAL
    template <typename BaseExecutor>
    struct is_one_way_executor<
        hpx::execution::experimental::annotating_executor<BaseExecutor>>
      : is_one_way_executor<BaseExecutor>
    {
    };

    template <typename BaseExecutor>
    struct is_never_blocking_one_way_executor<
        hpx::execution::experimental::annotating_executor<BaseExecutor>>
      : is_never_blocking_one_way_executor<BaseExecutor>
    {
    };

    template <typename BaseExecutor>
    struct is_bulk_one_way_executor<
        hpx::execution::experimental::annotating_executor<BaseExecutor>>
      : is_bulk_one_way_executor<BaseExecutor>
    {
    };

    template <typename BaseExecutor>
    struct is_two_way_executor<
        hpx::execution::experimental::annotating_executor<BaseExecutor>>
      : is_two_way_executor<BaseExecutor>
    {
    };

    template <typename BaseExecutor>
    struct is_bulk_two_way_executor<
        hpx::execution::experimental::annotating_executor<BaseExecutor>>
      : is_bulk_two_way_executor<BaseExecutor>
    {
    };

    template <typename BaseExecutor>
    struct is_scheduler_executor<
        hpx::execution::experimental::annotating_executor<BaseExecutor>>
      : is_scheduler_executor<BaseExecutor>
    {
    };
    /// \endcond
}    // namespace hpx::execution::experimental
