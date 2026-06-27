//  Copyright (c) 2007-2024 Hartmut Kaiser
//  Copyright (c) 2013 Agustin Berge
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/execution/traits/executor_traits.hpp>
#include <hpx/execution/traits/future_then_result_exec.hpp>
#include <hpx/modules/allocator_support.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/tag_invoke.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/modules/timing.hpp>

#include <exception>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace hpx::lcos::detail {

    HPX_CXX_CORE_EXPORT template <typename Executor, typename Future,
        typename F>
    inline hpx::traits::future_then_executor_result_t<Executor,
        std::decay_t<Future>, F>
    then_execute_helper(Executor&& exec, F&& f, Future&& predecessor)
    {
        // simply forward this to executor
        return parallel::execution::then_execute(
            exec, HPX_FORWARD(F, f), HPX_FORWARD(Future, predecessor));
    }

    ///////////////////////////////////////////////////////////////////////////
    // launch
    template <typename Future, typename Policy>
    struct future_then_dispatch<Future, Policy,
        std::enable_if_t<traits::is_launch_policy_v<Policy>>>
    {
        template <typename Policy_, typename F>
        HPX_FORCEINLINE static hpx::traits::future_then_result_t<Future, F>
        call(Future&& fut, Policy_&& policy, F&& f)
        {
            using result_type = typename hpx::traits::future_then_result<Future,
                F>::result_type;
            using continuation_result_type =
                hpx::util::invoke_result_t<F, Future>;

            using allocator_type = hpx::util::thread_local_caching_allocator<
                hpx::lockfree::variable_size_stack,
                hpx::util::internal_allocator<>>;

            hpx::traits::detail::shared_state_ptr_t<result_type> p =
                detail::make_continuation_alloc<continuation_result_type>(
                    allocator_type{}, HPX_MOVE(fut),
                    HPX_FORWARD(Policy_, policy), HPX_FORWARD(F, f));

            return hpx::traits::future_access<hpx::future<result_type>>::create(
                HPX_MOVE(p));
        }

        template <typename Allocator, typename Policy_, typename F>
        HPX_FORCEINLINE static hpx::traits::future_then_result_t<Future, F>
        call_alloc(
            Allocator const& alloc, Future&& fut, Policy_&& policy, F&& f)
        {
            using result_type = typename hpx::traits::future_then_result<Future,
                F>::result_type;
            using continuation_result_type =
                hpx::util::invoke_result_t<F, Future>;

            hpx::traits::detail::shared_state_ptr_t<result_type> p =
                detail::make_continuation_alloc<continuation_result_type>(alloc,
                    HPX_MOVE(fut), HPX_FORWARD(Policy_, policy),
                    HPX_FORWARD(F, f));

            return hpx::traits::future_access<hpx::future<result_type>>::create(
                HPX_MOVE(p));
        }
    };

    // The overload for future::then taking an executor simply forwards to the
    // corresponding executor customization point.
    //
    // parallel executors v2
    // threads::executor
    template <typename Future, typename Executor>
    struct future_then_dispatch<Future, Executor,
        std::enable_if_t<traits::is_one_way_executor_v<Executor> ||
            traits::is_two_way_executor_v<Executor>>>
    {
        template <typename Executor_, typename F>
        HPX_FORCEINLINE static hpx::traits::future_then_executor_result_t<
            Executor_, Future, F>
        call(Future&& fut, Executor_&& exec, F&& f)
        {
            // simply forward this to executor
            return detail::then_execute_helper(
                HPX_FORWARD(Executor_, exec), HPX_FORWARD(F, f), HPX_MOVE(fut));
        }

        template <typename Allocator, typename Executor_, typename F>
        HPX_FORCEINLINE static hpx::traits::future_then_executor_result_t<
            Executor_, Future, F>
        call_alloc(Allocator const&, Future&& fut, Executor_&& exec, F&& f)
        {
            return call(HPX_FORWARD(Future, fut), HPX_FORWARD(Executor_, exec),
                HPX_FORWARD(F, f));
        }
    };

    // plain function, or function object
    template <typename Future, typename FD>
    struct future_then_dispatch<Future, FD,
        std::enable_if_t<!traits::is_launch_policy_v<FD> &&
            !(traits::is_one_way_executor_v<FD> ||
                traits::is_two_way_executor_v<FD>)>>
    {
        template <typename F>
        HPX_FORCEINLINE static auto call(Future&& fut, F&& f)
            -> decltype(future_then_dispatch<Future, launch>::call(
                HPX_MOVE(fut), launch::all, HPX_FORWARD(F, f)))
        {
            return future_then_dispatch<Future, launch>::call(
                HPX_MOVE(fut), launch::all, HPX_FORWARD(F, f));
        }

        template <typename Allocator, typename F>
        HPX_FORCEINLINE static auto call_alloc(
            Allocator const& alloc, Future&& fut, F&& f)
            -> decltype(future_then_dispatch<Future, launch>::call_alloc(
                alloc, HPX_MOVE(fut), launch::all, HPX_FORWARD(F, f)))
        {
            return future_then_dispatch<Future, launch>::call_alloc(
                alloc, HPX_MOVE(fut), launch::all, HPX_FORWARD(F, f));
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT struct post_policy_spawner
    {
        template <typename F>
        void operator()(F&& f, hpx::threads::thread_description desc,
            threads::thread_id_ref_type& id) const
        {
            threads::thread_init_data data(
                threads::make_thread_function_nullary(HPX_FORWARD(F, f)),
                HPX_MOVE(desc), threads::thread_priority::default_,
                threads::thread_schedule_hint(),
                threads::thread_stacksize::default_,
                threads::thread_schedule_state::pending);

            threads::register_thread(data, id);
        }
    };

    HPX_CXX_CORE_EXPORT template <typename Executor>
    struct executor_spawner
    {
        Executor exec;

        template <typename F>
        void operator()(F&& f, hpx::threads::thread_description,
            threads::thread_id_ref_type& id) const
        {
            id = threads::invalid_thread_id;
            hpx::parallel::execution::post(exec, HPX_FORWARD(F, f));
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename ContResult, typename Future,
        typename Policy, typename F>
    inline traits::detail::shared_state_ptr_t<continuation_result_t<ContResult>>
    make_continuation(Future&& future, Policy&& policy, F&& f)
    {
        using result_type = continuation_result_t<ContResult>;
        using shared_state = detail::continuation<Future, F, result_type>;
        using init_no_addref = typename shared_state::init_no_addref;
        using spawner_type = post_policy_spawner;

        // create a continuation
        traits::detail::shared_state_ptr_t<result_type> p(
            new shared_state(init_no_addref{}, HPX_FORWARD(F, f)), false);

        static_cast<shared_state*>(p.get())->template attach<true>(
            HPX_FORWARD(Future, future), spawner_type{},
            HPX_FORWARD(Policy, policy));

        return p;
    }

    // same as above, except with allocator
    HPX_CXX_CORE_EXPORT template <typename ContResult, typename Allocator,
        typename Future, typename Policy, typename F>
    inline traits::detail::shared_state_ptr_t<continuation_result_t<ContResult>>
    make_continuation_alloc(
        Allocator const& a, Future&& future, Policy&& policy, F&& f)
    {
        using result_type = continuation_result_t<ContResult>;

        using base_allocator = Allocator;
        using shared_state = traits::shared_state_allocator_t<
            detail::continuation<Future, F, result_type>, base_allocator>;

        using other_allocator = typename std::allocator_traits<
            base_allocator>::template rebind_alloc<shared_state>;
        using traits = std::allocator_traits<other_allocator>;

        using init_no_addref = typename shared_state::init_no_addref;

        using unique_ptr = std::unique_ptr<shared_state,
            util::allocator_deleter<other_allocator>>;

        using spawner_type = post_policy_spawner;

        other_allocator alloc(a);
        unique_ptr p(traits::allocate(alloc, 1),
            util::allocator_deleter<other_allocator>{alloc});
        traits::construct(
            alloc, p.get(), init_no_addref{}, alloc, HPX_FORWARD(F, f));

        // create a continuation
        hpx::traits::detail::shared_state_ptr_t<result_type> r(
            p.release(), false);

        static_cast<shared_state*>(r.get())->template attach<true>(
            HPX_FORWARD(Future, future), spawner_type{},
            HPX_FORWARD(Policy, policy));

        return r;
    }

    // same as above, except with allocator and without unwrapping returned
    // futures
    HPX_CXX_CORE_EXPORT template <typename ContResult, typename Allocator,
        typename Future, typename Policy, typename F>
    inline traits::detail::shared_state_ptr_t<ContResult>
    make_continuation_alloc_nounwrap(
        Allocator const& a, Future&& future, Policy&& policy, F&& f)
    {
        using result_type = ContResult;

        using base_allocator = Allocator;
        using shared_state = traits::shared_state_allocator_t<
            detail::continuation<Future, F, result_type>, base_allocator>;

        using other_allocator = typename std::allocator_traits<
            base_allocator>::template rebind_alloc<shared_state>;
        using traits = std::allocator_traits<other_allocator>;

        using init_no_addref = typename shared_state::init_no_addref;

        using unique_ptr = std::unique_ptr<shared_state,
            util::allocator_deleter<other_allocator>>;

        using spawner_type = post_policy_spawner;

        other_allocator alloc(a);
        unique_ptr p(traits::allocate(alloc, 1),
            util::allocator_deleter<other_allocator>{alloc});
        traits::construct(
            alloc, p.get(), init_no_addref{}, alloc, HPX_FORWARD(F, f));

        // create a continuation
        hpx::traits::detail::shared_state_ptr_t<result_type> r(
            p.release(), false);

        static_cast<shared_state*>(r.get())->template attach<false>(
            HPX_FORWARD(Future, future), spawner_type{},
            HPX_FORWARD(Policy, policy));

        return r;
    }

    HPX_CXX_CORE_EXPORT template <typename ContResult, typename Future,
        typename Executor, typename Policy, typename F>
    inline traits::detail::shared_state_ptr_t<ContResult>
    make_continuation_exec_policy(
        Future&& future, Executor&& exec, Policy&& policy, F&& f)
    {
        using shared_state = detail::continuation<Future, F, ContResult>;
        using init_no_addref = typename shared_state::init_no_addref;
        using spawner_type = executor_spawner<std::decay_t<Executor>>;

        // create a continuation
        traits::detail::shared_state_ptr_t<ContResult> p(
            new shared_state(init_no_addref{}, HPX_FORWARD(F, f)), false);

        static_cast<shared_state*>(p.get())->template attach<false>(
            HPX_FORWARD(Future, future),
            spawner_type{HPX_FORWARD(Executor, exec)},
            HPX_FORWARD(Policy, policy));

        return p;
    }

    HPX_CXX_CORE_EXPORT template <typename ContResult, typename Future,
        typename Executor, typename F>
    inline traits::detail::shared_state_ptr_t<ContResult>
    make_continuation_exec(Future&& future, Executor&& exec, F&& f)
    {
        return make_continuation_exec_policy<ContResult>(
            HPX_FORWARD(Future, future), HPX_FORWARD(Executor, exec),
            launch::async_policy{}, HPX_FORWARD(F, f));
    }
}    // namespace hpx::lcos::detail
