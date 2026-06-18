//  Copyright (c) 2017-2025 Hartmut Kaiser
//  Copyright (c) 2017 Google
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/execution_base/detail/execution_member_detect.hpp>
#include <hpx/execution_base/traits/is_executor.hpp>
#include <hpx/modules/iterator_support.hpp>
#include <hpx/modules/tag_invoke.hpp>

#include <concepts>
#include <type_traits>
#include <utility>

namespace hpx::execution {

    ///////////////////////////////////////////////////////////////////////////
    /// Function invocations executed by a group of sequential execution agents
    /// execute in sequential order.
    HPX_CXX_CORE_EXPORT struct sequenced_execution_tag
    {
    };

    /// Function invocations executed by a group of parallel execution agents
    /// execute in unordered fashion. Any such invocations executing in the
    /// same thread are indeterminately sequenced with respect to each other.
    ///
    /// \note \a parallel_execution_tag is weaker than
    ///       \a sequenced_execution_tag.
    HPX_CXX_CORE_EXPORT struct parallel_execution_tag
    {
    };

    /// Function invocations executed by a group of vector execution agents are
    /// permitted to execute in unordered fashion when executed in different
    /// threads, and un-sequenced with respect to one another when executed in
    /// the same thread.
    ///
    /// \note \a unsequenced_execution_tag is weaker than
    ///       \a parallel_execution_tag.
    HPX_CXX_CORE_EXPORT struct unsequenced_execution_tag
    {
    };
}    // namespace hpx::execution

namespace hpx::parallel::execution {

    ///////////////////////////////////////////////////////////////////////////
    // Executor customization points
    namespace detail {

        /// \cond NOINTERNAL
        HPX_CXX_CORE_EXPORT template <typename Executor, typename Enable = void>
        struct async_execute_fn_helper;

        HPX_CXX_CORE_EXPORT template <typename Executor, typename Enable = void>
        struct sync_execute_fn_helper;

        HPX_CXX_CORE_EXPORT template <typename Executor, typename Enable = void>
        struct then_execute_fn_helper;

        HPX_CXX_CORE_EXPORT template <typename Executor, typename Enable = void>
        struct post_fn_helper;

        HPX_CXX_CORE_EXPORT template <typename Executor, typename Enable = void>
        struct bulk_async_execute_fn_helper;

        HPX_CXX_CORE_EXPORT template <typename Executor, typename Enable = void>
        struct bulk_sync_execute_fn_helper;

        HPX_CXX_CORE_EXPORT template <typename Executor, typename Enable = void>
        struct bulk_then_execute_fn_helper;

        HPX_CXX_CORE_EXPORT template <typename Executor, typename Enable = void>
        struct async_invoke_fn_helper;

        HPX_CXX_CORE_EXPORT template <typename Executor, typename Enable = void>
        struct sync_invoke_fn_helper;
        /// \endcond
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // define customization points

    // OneWayExecutor customization point: execution::sync_execute

    /// Customization point for synchronous execution agent creation.
    ///
    /// This synchronously creates a single function invocation f() using
    /// the associated executor. The execution of the supplied function
    /// synchronizes with the caller
    ///
    /// \param exec [in] The executor object to use for scheduling of the
    ///             function \a f.
    /// \param f    [in] The function which will be scheduled using the
    ///             given executor.
    /// \param ts   [in] Additional arguments to use to invoke \a f.
    ///
    /// \returns f(ts...)'s result
    ///
    /// \note It will call tag_invoke(sync_execute_t, exec, f, ts...) if it
    ///       exists. For two-way executors it will invoke async_execute_t
    ///       and wait for the task's completion before returning.
    ///
    HPX_CXX_CORE_EXPORT inline constexpr struct sync_execute_t final
      : hpx::functional::detail::tag_fallback<sync_execute_t>
    {
    private:
        // Bridge: forward to member function if available
        template <typename Executor, typename F, typename... Ts>
            requires(detail::has_sync_execute_member<Executor, F, Ts...>)
        friend HPX_FORCEINLINE decltype(auto) tag_invoke(
            sync_execute_t, Executor&& exec, F&& f, Ts&&... ts)
        {
            return HPX_FORWARD(Executor, exec)
                .sync_execute(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <executor_any Executor, typename F, typename... Ts>
            requires(std::invocable<F &&, Ts && ...>)
        friend HPX_FORCEINLINE decltype(auto) tag_fallback_invoke(
            sync_execute_t, Executor&& exec, F&& f, Ts&&... ts)
        {
            return detail::sync_execute_fn_helper<std::decay_t<Executor>>::call(
                HPX_FORWARD(Executor, exec), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }
    } sync_execute{};

    ///////////////////////////////////////////////////////////////////////////
    // TwoWayExecutor customization points: execution::async_execute,
    // execution::sync_execute, and execution::then_execute

    /// Customization point for asynchronous execution agent creation.
    ///
    /// This asynchronously creates a single function invocation f() using
    /// the associated executor.
    ///
    /// \param exec [in] The executor object to use for scheduling of the
    ///             function \a f.
    /// \param f    [in] The function which will be scheduled using the
    ///             given executor.
    /// \param ts   [in] Additional arguments to use to invoke \a f.
    ///
    /// \note Executors have to implement only `async_execute()`. All other
    ///       functions will be emulated by this or other customization
    ///       points in terms of this single basic primitive. However, some
    ///       executors will naturally specialize all operations for
    ///       maximum efficiency.
    ///
    /// \note This is valid for one way executors (calls
    ///       make_ready_future(exec.sync_execute(f, ts...) if it exists)
    ///       and for two-way executors (calls exec.async_execute(f, ts...)
    ///       if it exists).
    ///
    /// \returns f(ts...)'s result through a future
    ///
    HPX_CXX_CORE_EXPORT inline constexpr struct async_execute_t final
      : hpx::functional::detail::tag_fallback<async_execute_t>
    {
    private:
        // Bridge: forward to member function if available
        template <typename Executor, typename F, typename... Ts>
            requires(detail::has_async_execute_member<Executor, F, Ts...>)
        friend HPX_FORCEINLINE decltype(auto) tag_invoke(
            async_execute_t, Executor&& exec, F&& f, Ts&&... ts)
        {
            return HPX_FORWARD(Executor, exec)
                .async_execute(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <executor_any Executor, typename F, typename... Ts>
            requires(std::invocable<F &&, Ts && ...>)
        friend HPX_FORCEINLINE decltype(auto) tag_fallback_invoke(
            async_execute_t, Executor&& exec, F&& f, Ts&&... ts)
        {
            return detail::async_execute_fn_helper<
                std::decay_t<Executor>>::call(HPX_FORWARD(Executor, exec),
                HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }
    } async_execute{};

    /// Customization point for execution agent creation depending on a
    /// given future.
    ///
    /// This creates a single function invocation f() using the associated
    /// executor after the given future object has become ready.
    ///
    /// \param exec [in] The executor object to use for scheduling of the
    ///             function \a f.
    /// \param f    [in] The function which will be scheduled using the
    ///             given executor.
    /// \param predecessor [in] The future object the execution of the
    ///             given function depends on.
    /// \param ts   [in] Additional arguments to use to invoke \a f.
    ///
    /// \returns f(ts...)'s result through a future
    ///
    /// \note This is valid for two-way executors (calls
    ///       exec.then_execute(f, predecessor, ts...) if it exists) and
    ///       for one way executors (calls predecessor.then(bind(f, ts...))).
    ///
    HPX_CXX_CORE_EXPORT inline constexpr struct then_execute_t final
      : hpx::functional::detail::tag_fallback<then_execute_t>
    {
    private:
        // Bridge: forward to member function if available
        template <typename Executor, typename F, typename Future,
            typename... Ts>
            requires(
                detail::has_then_execute_member<Executor, F, Future, Ts...>)
        friend HPX_FORCEINLINE decltype(auto) tag_invoke(then_execute_t,
            Executor&& exec, F&& f, Future&& predecessor, Ts&&... ts)
        {
            return HPX_FORWARD(Executor, exec)
                .then_execute(HPX_FORWARD(F, f),
                    HPX_FORWARD(Future, predecessor), HPX_FORWARD(Ts, ts)...);
        }

        template <executor_any Executor, typename F, typename Future,
            typename... Ts>
            requires(std::invocable<F &&, Future &&, Ts && ...>)
        friend HPX_FORCEINLINE decltype(auto) tag_fallback_invoke(
            then_execute_t, Executor&& exec, F&& f, Future&& predecessor,
            Ts&&... ts)
        {
            return detail::then_execute_fn_helper<std::decay_t<Executor>>::call(
                HPX_FORWARD(Executor, exec), HPX_FORWARD(F, f),
                HPX_FORWARD(Future, predecessor), HPX_FORWARD(Ts, ts)...);
        }
    } then_execute{};

    ///////////////////////////////////////////////////////////////////////
    // NonBlockingOneWayExecutor customization point: execution::post

    /// Customization point for asynchronous fire & forget execution
    /// agent creation.
    ///
    /// This asynchronously (fire & forget) creates a single function
    /// invocation f() using the associated executor.
    ///
    /// \param exec [in] The executor object to use for scheduling of the
    ///             function \a f.
    /// \param f    [in] The function which will be scheduled using the
    ///             given executor.
    /// \param ts   [in] Additional arguments to use to invoke \a f.
    ///
    /// \note This is valid for two-way executors (calls
    ///       exec.post(f, ts...), if available, otherwise
    ///       it calls exec.async_execute(f, ts...) while discarding the
    ///       returned future), and for non-blocking two-way executors
    ///       (calls exec.post(f, ts...) if it exists).
    ///
    HPX_CXX_CORE_EXPORT inline constexpr struct post_t final
      : hpx::functional::detail::tag_fallback<post_t>
    {
    private:
        // Bridge: forward to member function if available
        template <typename Executor, typename F, typename... Ts>
            requires(detail::has_post_member<Executor, F, Ts...>)
        friend HPX_FORCEINLINE decltype(auto) tag_invoke(
            post_t, Executor&& exec, F&& f, Ts&&... ts)
        {
            return HPX_FORWARD(Executor, exec)
                .post(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        template <executor_any Executor, typename F, typename... Ts>
            requires(std::invocable<F &&, Ts && ...>)
        friend HPX_FORCEINLINE decltype(auto) tag_fallback_invoke(
            post_t, Executor&& exec, F&& f, Ts&&... ts)
        {
            return detail::post_fn_helper<std::decay_t<Executor>>::call(
                HPX_FORWARD(Executor, exec), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }
    } post{};

    ///////////////////////////////////////////////////////////////////////
    // BulkTwoWayExecutor customization points:
    // execution::bulk_async_execute, execution::bulk_sync_execute,
    // execution::bulk_then_execute

    /// Bulk form of synchronous execution agent creation.
    ///
    /// \note This is deliberately different from the bulk_sync_execute
    ///       customization points specified in P0443.The bulk_sync_execute
    ///       customization point defined here is more generic and is used
    ///       as the workhorse for implementing the specified APIs.
    ///
    /// This synchronously creates a group of function invocations f(i)
    /// whose ordering is given by the execution_category associated with
    /// the executor. The function synchronizes the execution of all
    /// scheduled functions with the caller.
    ///
    /// Here \a i takes on all values in the index space implied by shape.
    /// All exceptions thrown by invocations of f(i) are reported in a
    /// manner consistent with parallel algorithm execution through the
    /// returned future.
    ///
    /// \param exec  [in] The executor object to use for scheduling of the
    ///              function \a f.
    /// \param f     [in] The function which will be scheduled using the
    ///              given executor.
    /// \param shape [in] The shape objects which defines the iteration
    ///              boundaries for the arguments to be passed to \a f.
    /// \param ts    [in] Additional arguments to use to invoke \a f.
    ///
    /// \returns The return type of \a executor_type::bulk_sync_execute
    ///          if defined by \a executor_type. Otherwise, a vector holding
    ///          the returned values of each invocation of \a f except when
    ///          \a f returns void, which case void is returned.
    ///
    /// \note This calls exec.bulk_sync_execute(f, shape, ts...) if it
    ///       exists; otherwise it executes sync_execute(f, shape, ts...)
    ///       as often as needed.
    ///
    HPX_CXX_CORE_EXPORT inline constexpr struct bulk_sync_execute_t final
      : hpx::functional::detail::tag_fallback<bulk_sync_execute_t>
    {
    private:
        // Bridge: forward to member function if available
        template <typename Executor, typename F, typename Shape, typename... Ts>
            requires(!std::integral<Shape> &&
                detail::has_bulk_sync_execute_member<Executor, F, Shape, Ts...>)
        friend HPX_FORCEINLINE decltype(auto) tag_invoke(bulk_sync_execute_t,
            Executor&& exec, F&& f, Shape const& shape, Ts&&... ts)
        {
            return HPX_FORWARD(Executor, exec)
                .bulk_sync_execute(
                    HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
        }

        template <executor_any Executor, typename F, typename Shape,
            typename... Ts>
            requires(!std::integral<Shape>)
        friend HPX_FORCEINLINE decltype(auto) tag_fallback_invoke(
            bulk_sync_execute_t, Executor&& exec, F&& f, Shape const& shape,
            Ts&&... ts)
        {
            return detail::bulk_sync_execute_fn_helper<
                std::decay_t<Executor>>::call(HPX_FORWARD(Executor, exec),
                HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
        }

        template <executor_any Executor, typename F, typename Shape,
            typename... Ts>
            requires(std::integral<Shape>)
        friend HPX_FORCEINLINE decltype(auto) tag_fallback_invoke(
            bulk_sync_execute_t tag, Executor&& exec, F&& f, Shape const& shape,
            Ts&&... ts)
        {
            return tag(HPX_FORWARD(Executor, exec), HPX_FORWARD(F, f),
                hpx::util::counting_shape(shape), HPX_FORWARD(Ts, ts)...);
        }
    } bulk_sync_execute{};

    /// Bulk form of asynchronous execution agent creation.
    ///
    /// \note This is deliberately different from the bulk_async_execute
    ///       customization points specified in P0443.The bulk_async_execute
    ///       customization point defined here is more generic and is used
    ///       as the workhorse for implementing the specified APIs.
    ///
    /// This asynchronously creates a group of function invocations f(i)
    /// whose ordering is given by the execution_category associated with
    /// the executor.
    ///
    /// Here \a i takes on all values in the index space implied by shape.
    /// All exceptions thrown by invocations of f(i) are reported in a
    /// manner consistent with parallel algorithm execution through the
    /// returned future.
    ///
    /// \param exec  [in] The executor object to use for scheduling of the
    ///              function \a f.
    /// \param f     [in] The function which will be scheduled using the
    ///              given executor.
    /// \param shape [in] The shape objects which defines the iteration
    ///              boundaries for the arguments to be passed to \a f.
    /// \param ts    [in] Additional arguments to use to invoke \a f.
    ///
    /// \returns The return type of \a executor_type::bulk_async_execute if
    ///          defined by \a executor_type. Otherwise, a vector
    ///          of futures holding the returned values of each invocation
    ///          of \a f.
    ///
    /// \note This calls exec.bulk_async_execute(f, shape, ts...) if it
    ///       exists; otherwise it executes async_execute(f, shape, ts...)
    ///       as often as needed.
    ///
    HPX_CXX_CORE_EXPORT inline constexpr struct bulk_async_execute_t final
      : hpx::functional::detail::tag_fallback<bulk_async_execute_t>
    {
    private:
        // Bridge: forward to member function if available
        template <typename Executor, typename F, typename Shape, typename... Ts>
            requires(!std::integral<Shape> &&
                detail::has_bulk_async_execute_member<Executor, F, Shape,
                    Ts...>)
        friend HPX_FORCEINLINE decltype(auto) tag_invoke(bulk_async_execute_t,
            Executor&& exec, F&& f, Shape const& shape, Ts&&... ts)
        {
            return HPX_FORWARD(Executor, exec)
                .bulk_async_execute(
                    HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
        }

        template <executor_any Executor, typename F, typename Shape,
            typename... Ts>
            requires(!std::integral<Shape>)
        friend HPX_FORCEINLINE decltype(auto) tag_fallback_invoke(
            bulk_async_execute_t, Executor&& exec, F&& f, Shape const& shape,
            Ts&&... ts)
        {
            return detail::bulk_async_execute_fn_helper<
                std::decay_t<Executor>>::call(HPX_FORWARD(Executor, exec),
                HPX_FORWARD(F, f), shape, HPX_FORWARD(Ts, ts)...);
        }

        template <executor_any Executor, typename F, typename Shape,
            typename... Ts>
            requires(std::integral<Shape>)
        friend HPX_FORCEINLINE decltype(auto) tag_fallback_invoke(
            bulk_async_execute_t tag, Executor&& exec, F&& f,
            Shape const& shape, Ts&&... ts)
        {
            return tag(HPX_FORWARD(Executor, exec), HPX_FORWARD(F, f),
                hpx::util::counting_shape(shape), HPX_FORWARD(Ts, ts)...);
        }
    } bulk_async_execute{};

    /// Bulk form of execution agent creation depending on a given future.
    ///
    /// \note This is deliberately different from the then_sync_execute
    ///       customization points specified in P0443.The bulk_then_execute
    ///       customization point defined here is more generic and is used
    ///       as the workhorse for implementing the specified APIs.
    ///
    /// This creates a group of function invocations f(i)
    /// whose ordering is given by the execution_category associated with
    /// the executor.
    ///
    /// Here \a i takes on all values in the index space implied by shape.
    /// All exceptions thrown by invocations of f(i) are reported in a
    /// manner consistent with parallel algorithm execution through the
    /// returned future.
    ///
    /// \param exec  [in] The executor object to use for scheduling of the
    ///              function \a f.
    /// \param f     [in] The function which will be scheduled using the
    ///              given executor.
    /// \param shape [in] The shape objects which defines the iteration
    ///              boundaries for the arguments to be passed to \a f.
    /// \param predecessor [in] The future object the execution of the
    ///             given function depends on.
    /// \param ts    [in] Additional arguments to use to invoke \a f.
    ///
    /// \returns The return type of \a executor_type::bulk_then_execute
    ///          if defined by \a executor_type. Otherwise, a vector holding
    ///          the returned values of each invocation of \a f.
    ///
    /// \note This calls exec.bulk_then_execute(f, shape, pred, ts...) if it
    ///       exists; otherwise it executes
    ///       sync_execute(f, shape, pred.share(), ts...) (if this executor
    ///       is also an OneWayExecutor), or
    ///       async_execute(f, shape, pred.share(), ts...) (if this executor
    ///       is also a TwoWayExecutor) - as often as needed.
    ///
    HPX_CXX_CORE_EXPORT inline constexpr struct bulk_then_execute_t final
      : hpx::functional::detail::tag_fallback<bulk_then_execute_t>
    {
    private:
        // Bridge: forward to member function if available
        template <typename Executor, typename F, typename Shape,
            typename Future, typename... Ts>
            requires(!std::integral<Shape> &&
                detail::has_bulk_then_execute_member<Executor, F, Shape, Future,
                    Ts...>)
        friend HPX_FORCEINLINE decltype(auto) tag_invoke(bulk_then_execute_t,
            Executor&& exec, F&& f, Shape const& shape, Future&& predecessor,
            Ts&&... ts)
        {
            return HPX_FORWARD(Executor, exec)
                .bulk_then_execute(HPX_FORWARD(F, f), shape,
                    HPX_FORWARD(Future, predecessor), HPX_FORWARD(Ts, ts)...);
        }

        template <executor_any Executor, typename F, typename Shape,
            typename Future, typename... Ts>
            requires(!std::integral<Shape>)
        friend HPX_FORCEINLINE decltype(auto) tag_fallback_invoke(
            bulk_then_execute_t, Executor&& exec, F&& f, Shape const& shape,
            Future&& predecessor, Ts&&... ts)
        {
            return detail::bulk_then_execute_fn_helper<
                std::decay_t<Executor>>::call(HPX_FORWARD(Executor, exec),
                HPX_FORWARD(F, f), shape, HPX_FORWARD(Future, predecessor),
                HPX_FORWARD(Ts, ts)...);
        }

        template <executor_any Executor, typename F, typename Shape,
            typename Future, typename... Ts>
            requires(std::integral<Shape>)
        friend HPX_FORCEINLINE decltype(auto) tag_fallback_invoke(
            bulk_then_execute_t tag, Executor&& exec, F&& f, Shape const& shape,
            Future&& predecessor, Ts&&... ts)
        {
            return tag(HPX_FORWARD(Executor, exec), HPX_FORWARD(F, f),
                hpx::util::counting_shape(shape),
                HPX_FORWARD(Future, predecessor), HPX_FORWARD(Ts, ts)...);
        }
    } bulk_then_execute{};

    /// Asynchronously invoke the given set of nullary functions, each on its
    /// own execution agent
    ///
    /// This creates a group of function invocations whose ordering is given by
    /// the execution_category associated with the executor.
    ///
    /// All exceptions thrown by invocations of the functions are reported in a
    /// manner consistent with parallel algorithm execution through the returned
    /// future.
    ///
    /// \param exec  [in] The executor object to use for scheduling of the
    ///              functions \a fs.
    /// \param fs    [in] The functions which will be scheduled using the
    ///              given executor.
    ///
    /// \returns The return type of \a executor_type::async_invoke if defined by
    ///          \a executor_type. Otherwise, a future<void>
    ///          representing finishing the execution of all functions \a fs.
    ///
    /// \note This calls exec.async_invoke(fs...) if it exists; otherwise it
    ///       executes async_execute(fs) for each fs.
    ///
    HPX_CXX_CORE_EXPORT inline constexpr struct async_invoke_t final
      : hpx::functional::detail::tag_fallback<async_invoke_t>
    {
    private:
        // Bridge: forward to member function if available
        template <typename Executor, typename F, typename... Fs>
            requires(detail::has_async_invoke_member<Executor, F, Fs...>)
        friend HPX_FORCEINLINE decltype(auto) tag_invoke(
            async_invoke_t, Executor&& exec, F&& f, Fs&&... fs)
        {
            return HPX_FORWARD(Executor, exec)
                .async_invoke(HPX_FORWARD(F, f), HPX_FORWARD(Fs, fs)...);
        }

        template <executor_any Executor, typename F, typename... Fs>
            requires(std::invocable<F> && (std::invocable<Fs> && ...))
        friend HPX_FORCEINLINE decltype(auto) tag_fallback_invoke(
            async_invoke_t, Executor&& exec, F&& f, Fs&&... fs)
        {
            return detail::async_invoke_fn_helper<std::decay_t<Executor>>::call(
                HPX_FORWARD(Executor, exec), HPX_FORWARD(F, f),
                HPX_FORWARD(Fs, fs)...);
        }
    } async_invoke{};

    /// Synchronously invoke the given set of nullary functions, each on its own
    /// execution agent
    ///
    /// This creates a group of function invocations whose ordering is given by
    /// the execution_category associated with the executor.
    ///
    /// All exceptions thrown by invocations of the functions are reported in a
    /// manner consistent with parallel algorithm execution through the returned
    /// future.
    ///
    /// \param exec  [in] The executor object to use for scheduling of the
    ///              functions \a fs.
    /// \param fs    [in] The functions which will be scheduled using the
    ///              given executor.
    ///
    /// \returns The return type of \a executor_type::async_invoke if defined by
    ///          \a executor_type.
    ///
    /// \note This calls exec.sync_invoke(fs...) if it exists; otherwise it
    ///       executes sync_execute(fs) for each fs.
    ///
    HPX_CXX_CORE_EXPORT inline constexpr struct sync_invoke_t final
      : hpx::functional::detail::tag_fallback<sync_invoke_t>
    {
    private:
        // Bridge: forward to member function if available
        template <typename Executor, typename F, typename... Fs>
            requires(detail::has_sync_invoke_member<Executor, F, Fs...>)
        friend HPX_FORCEINLINE decltype(auto) tag_invoke(
            sync_invoke_t, Executor&& exec, F&& f, Fs&&... fs)
        {
            return HPX_FORWARD(Executor, exec)
                .sync_invoke(HPX_FORWARD(F, f), HPX_FORWARD(Fs, fs)...);
        }

        template <executor_any Executor, typename F, typename... Fs>
            requires(std::invocable<F> && (std::invocable<Fs> && ...))
        friend HPX_FORCEINLINE decltype(auto) tag_fallback_invoke(
            sync_invoke_t, Executor&& exec, F&& f, Fs&&... fs)
        {
            return detail::sync_invoke_fn_helper<std::decay_t<Executor>>::call(
                HPX_FORWARD(Executor, exec), HPX_FORWARD(F, f),
                HPX_FORWARD(Fs, fs)...);
        }
    } sync_invoke{};
}    // namespace hpx::parallel::execution
