//  Copyright (c) 2017-2024 Hartmut Kaiser
//  Copyright (c) 2026 Sai Charan Arvapally
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file parallel/executors/execute_at_executor.hpp

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/threading.hpp>
#include <hpx/modules/timing.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/timed_execution/timed_execution.hpp>

#include <chrono>
#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>

namespace hpx::parallel::execution {

    namespace detail {
        /// \cond NOINTERNAL

        ///////////////////////////////////////////////////////////////////////
        template <typename Tag, typename Executor, typename F>
        struct then_execute_helper
        {
            template <typename Executor_, typename F_>
            then_execute_helper(Executor_&& exec, F_&& call)
              : exec_(HPX_FORWARD(Executor_, exec))
              , call_(HPX_FORWARD(F_, call))
            {
            }

            decltype(auto) operator()(hpx::future<void>&& fut)
            {
                return Tag()(HPX_MOVE(fut), HPX_MOVE(exec_), HPX_MOVE(call_));
            }

        private:
            Executor exec_;
            F call_;
        };

        template <typename Tag, typename Executor, typename F>
        then_execute_helper<Tag, std::decay_t<Executor>, std::decay_t<F>>
        make_then_execute_helper(Executor&& exec, F&& call)
        {
            return {HPX_FORWARD(Executor, exec), HPX_FORWARD(F, call)};
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename Tag>
        struct sync_execute_at_helper
        {
            template <typename Executor, typename F>
            decltype(auto) operator()(
                hpx::future<void>&& fut, Executor&& exec, F&& f) const
            {
                fut.get();    // rethrow exceptions
                return execution::sync_execute(
                    HPX_FORWARD(Executor, exec), HPX_FORWARD(F, f));
            }

            template <typename Executor, typename F, typename... Ts>
            static auto call(hpx::traits::detail::wrap_int, Executor&& exec,
                std::chrono::steady_clock::time_point const& abs_time, F&& f,
                Ts&&... ts)
                -> decltype(execution::async_execute(
                    HPX_FORWARD(Executor, exec), HPX_FORWARD(F, f),
                    HPX_FORWARD(Ts, ts)...)
                        .get())
            {
                auto predecessor = make_ready_future_at(abs_time);
                return execution::then_execute(
                    hpx::execution::sequenced_executor(),
                    make_then_execute_helper<sync_execute_at_helper>(
                        HPX_FORWARD(Executor, exec),
                        hpx::util::deferred_call(
                            HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...)),
                    HPX_MOVE(predecessor))
                    .get();
            }

            template <typename Executor, typename F, typename... Ts>
            static auto call(int, Executor&& exec,
                std::chrono::steady_clock::time_point const& abs_time, F&& f,
                Ts&&... ts) -> decltype(exec.sync_execute_at(abs_time,
                HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...))
            {
                return exec.sync_execute_at(
                    abs_time, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }
        };

        template <>
        struct sync_execute_at_helper<hpx::execution::sequenced_execution_tag>
        {
            template <typename Executor, typename F, typename... Ts>
            static auto call(hpx::traits::detail::wrap_int, Executor&& exec,
                std::chrono::steady_clock::time_point const& abs_time, F&& f,
                Ts&&... ts)
                -> decltype(execution::sync_execute(HPX_FORWARD(Executor, exec),
                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...))
            {
                this_thread::sleep_until(abs_time);
                return execution::sync_execute(HPX_FORWARD(Executor, exec),
                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }

            template <typename Executor, typename F, typename... Ts>
            static auto call(int, Executor&& exec,
                std::chrono::steady_clock::time_point const& abs_time, F&& f,
                Ts&&... ts) -> decltype(exec.sync_execute_at(abs_time,
                HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...))
            {
                return exec.sync_execute_at(
                    abs_time, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }
        };

        template <typename Executor, typename F, typename... Ts>
        decltype(auto) call_sync_execute_at(Executor&& exec,
            std::chrono::steady_clock::time_point const& abs_time, F&& f,
            Ts&&... ts)
        {
            using tag = hpx::traits::executor_execution_category_t<
                std::decay_t<Executor>>;

            return sync_execute_at_helper<tag>::call(0,
                HPX_FORWARD(Executor, exec), abs_time, HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename Tag>
        struct async_execute_at_helper
        {
            template <typename Executor, typename F>
            decltype(auto) operator()(
                hpx::future<void>&& fut, Executor&& exec, F&& f) const
            {
                fut.get();    // rethrow exceptions
                return execution::async_execute(
                    HPX_FORWARD(Executor, exec), HPX_FORWARD(F, f));
            }

            template <typename Executor, typename F, typename... Ts>
            static auto call(hpx::traits::detail::wrap_int, Executor&& exec,
                std::chrono::steady_clock::time_point const& abs_time, F&& f,
                Ts&&... ts)
                -> decltype(execution::async_execute(
                    HPX_FORWARD(Executor, exec), HPX_FORWARD(F, f),
                    HPX_FORWARD(Ts, ts)...))
            {
                auto predecessor = make_ready_future_at(abs_time);
                return execution::then_execute(
                    hpx::execution::sequenced_executor(),
                    make_then_execute_helper<async_execute_at_helper>(
                        HPX_FORWARD(Executor, exec),
                        hpx::util::deferred_call(
                            HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...)),
                    HPX_MOVE(predecessor));
            }

            template <typename Executor, typename F, typename... Ts>
            static auto call(int, Executor&& exec,
                std::chrono::steady_clock::time_point const& abs_time, F&& f,
                Ts&&... ts) -> decltype(exec.async_execute_at(abs_time,
                HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...))
            {
                return exec.async_execute_at(
                    abs_time, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }
        };

        template <>
        struct async_execute_at_helper<hpx::execution::sequenced_execution_tag>
        {
            template <typename Executor, typename F, typename... Ts>
            static auto call(hpx::traits::detail::wrap_int, Executor&& exec,
                std::chrono::steady_clock::time_point const& abs_time, F&& f,
                Ts&&... ts)
                -> decltype(execution::async_execute(
                    HPX_FORWARD(Executor, exec), HPX_FORWARD(F, f),
                    HPX_FORWARD(Ts, ts)...))
            {
                this_thread::sleep_until(abs_time);
                return execution::async_execute(HPX_FORWARD(Executor, exec),
                    HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }

            template <typename Executor, typename F, typename... Ts>
            static auto call(int, Executor&& exec,
                std::chrono::steady_clock::time_point const& abs_time, F&& f,
                Ts&&... ts) -> decltype(exec.async_execute_at(abs_time,
                HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...))
            {
                return exec.async_execute_at(
                    abs_time, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }
        };

        template <typename Executor, typename F, typename... Ts>
        decltype(auto) call_async_execute_at(Executor&& exec,
            std::chrono::steady_clock::time_point const& abs_time, F&& f,
            Ts&&... ts)
        {
            using tag = hpx::traits::executor_execution_category_t<
                std::decay_t<Executor>>;

            return async_execute_at_helper<tag>::call(0,
                HPX_FORWARD(Executor, exec), abs_time, HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename Tag>
        struct post_at_helper
        {
            template <typename Executor, typename F>
            void operator()(
                hpx::future<void>&& fut, Executor&& exec, F&& f) const
            {
                fut.get();    // rethrow exceptions
                execution::post(HPX_FORWARD(Executor, exec), HPX_FORWARD(F, f));
            }

            template <typename Executor, typename F, typename... Ts>
            static void call(hpx::traits::detail::wrap_int, Executor&& exec,
                std::chrono::steady_clock::time_point const& abs_time, F&& f,
                Ts&&... ts)
            {
                auto predecessor = make_ready_future_at(abs_time);
                execution::then_execute(hpx::execution::sequenced_executor(),
                    make_then_execute_helper<post_at_helper>(
                        HPX_FORWARD(Executor, exec),
                        hpx::util::deferred_call(
                            HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...)),
                    HPX_MOVE(predecessor));
            }

            template <typename Executor, typename F, typename... Ts>
            static auto call(int, Executor&& exec,
                std::chrono::steady_clock::time_point const& abs_time, F&& f,
                Ts&&... ts) -> decltype(exec.post_at(abs_time,
                HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...))
            {
                return exec.post_at(
                    abs_time, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }
        };

        template <>
        struct post_at_helper<hpx::execution::sequenced_execution_tag>
        {
            template <typename Executor, typename F, typename... Ts>
            static void call(hpx::traits::detail::wrap_int, Executor&& exec,
                std::chrono::steady_clock::time_point const& abs_time, F&& f,
                Ts&&... ts)
            {
                this_thread::sleep_until(abs_time);
                execution::post(HPX_FORWARD(Executor, exec), HPX_FORWARD(F, f),
                    HPX_FORWARD(Ts, ts)...);
            }
            template <typename Executor, typename F, typename... Ts>
            static auto call(int, Executor&& exec,
                std::chrono::steady_clock::time_point const& abs_time, F&& f,
                Ts&&... ts) -> decltype(exec.post_at(abs_time,
                HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...))
            {
                return exec.post_at(
                    abs_time, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
            }
        };

        template <typename Executor, typename F, typename... Ts>
        void call_post_at(Executor&& exec,
            std::chrono::steady_clock::time_point const& abs_time, F&& f,
            Ts&&... ts)
        {
            using tag = hpx::traits::executor_execution_category_t<
                std::decay_t<Executor>>;

            return post_at_helper<tag>::call(0, HPX_FORWARD(Executor, exec),
                abs_time, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }
        /// \endcond
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // Executor allowing to run things at a given point in time
    HPX_CXX_CORE_EXPORT template <executor_any BaseExecutor>
    struct timed_executor
    {
        using base_executor_type = std::decay_t<BaseExecutor>;

        using execution_category =
            hpx::traits::executor_execution_category_t<base_executor_type>;
        using parameters_type =
            hpx::traits::executor_parameters_type_t<base_executor_type>;

        explicit timed_executor(hpx::chrono::steady_time_point const& abs_time)
          : exec_(BaseExecutor())
          , execute_at_(abs_time.value())
        {
        }

        explicit timed_executor(hpx::chrono::steady_duration const& rel_time)
          : exec_(BaseExecutor())
          , execute_at_(rel_time.from_now())
        {
        }

        template <typename Executor>
        timed_executor(
            Executor&& exec, hpx::chrono::steady_time_point const& abs_time)
          : exec_(HPX_FORWARD(Executor, exec))
          , execute_at_(abs_time.value())
        {
        }

        template <typename Executor>
        timed_executor(
            Executor&& exec, hpx::chrono::steady_duration const& rel_time)
          : exec_(HPX_FORWARD(Executor, exec))
          , execute_at_(rel_time.from_now())
        {
        }

        /// \cond NOINTERNAL
        constexpr bool operator==(timed_executor const& rhs) const noexcept
        {
            return exec_ == rhs.exec_ && execute_at_ == rhs.execute_at_;
        }

        constexpr bool operator!=(timed_executor const& rhs) const noexcept
        {
            return !(*this == rhs);
        }

        constexpr timed_executor const& context() const noexcept
        {
            return *this;
        }
        /// \endcond

    public:
        // OneWayExecutor interface
        template <typename F, typename... Ts>
        decltype(auto) sync_execute(F&& f, Ts&&... ts) const
        {
            return detail::call_sync_execute_at(
                exec_, execute_at_, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        // TwoWayExecutor interface
        template <typename F, typename... Ts>
        decltype(auto) async_execute(F&& f, Ts&&... ts) const
        {
            return detail::call_async_execute_at(
                exec_, execute_at_, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        // NonBlockingOneWayExecutor (adapted) interface
        template <typename F, typename... Ts>
        decltype(auto) post(F&& f, Ts&&... ts) const
        {
            detail::call_post_at(
                exec_, execute_at_, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        // support all properties exposed by the wrapped executor
        template <typename Tag, typename Property>
            requires(
                hpx::execution::experimental::is_scheduling_property_v<Tag> &&
                requires(Tag tag, BaseExecutor const& exec, Property&& prop) {
                    tag(exec, HPX_FORWARD(Property, prop));
                })
        [[nodiscard]] timed_executor query(Tag, Property&& prop) const
        {
            return timed_executor(
                Tag{}(exec_, HPX_FORWARD(Property, prop)), execute_at_);
        }

        template <typename Tag>
            requires(
                hpx::execution::experimental::is_scheduling_property_v<Tag> &&
                requires(Tag tag, BaseExecutor const& exec) { tag(exec); })
        [[nodiscard]] decltype(auto) query(Tag tag) const
        {
            return tag(exec_);
        }

    private:
        BaseExecutor exec_;
        std::chrono::steady_clock::time_point execute_at_;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT using sequenced_timed_executor =
        timed_executor<hpx::execution::sequenced_executor>;

    HPX_CXX_CORE_EXPORT using parallel_timed_executor =
        timed_executor<hpx::execution::parallel_executor>;
}    // namespace hpx::parallel::execution

namespace hpx::execution::experimental {
    /// \cond NOINTERNAL

    ///////////////////////////////////////////////////////////////////////////
    template <typename BaseExecutor>
    struct is_one_way_executor<
        parallel::execution::timed_executor<BaseExecutor>>
      : is_one_way_executor<std::decay_t<BaseExecutor>>
    {
    };

    template <typename BaseExecutor>
    struct is_two_way_executor<
        parallel::execution::timed_executor<BaseExecutor>>
      : is_two_way_executor<std::decay_t<BaseExecutor>>
    {
    };

    template <typename BaseExecutor>
    struct is_never_blocking_one_way_executor<
        parallel::execution::timed_executor<BaseExecutor>>
      : is_never_blocking_one_way_executor<std::decay_t<BaseExecutor>>
    {
    };

    ///////////////////////////////////////////////////////////////////////////
    template <>
    struct is_one_way_executor<parallel::execution::sequenced_timed_executor>
      : std::true_type
    {
    };

    template <>
    struct is_two_way_executor<parallel::execution::parallel_timed_executor>
      : std::true_type
    {
    };
    /// \endcond
}    // namespace hpx::execution::experimental
