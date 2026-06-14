//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/executors/parallel_executor.hpp>
#include <hpx/modules/allocator_support.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/concepts.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/memory.hpp>
#include <hpx/modules/pack_traversal.hpp>
#include <hpx/modules/threading_base.hpp>

#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
// forward declare the type we will get function annotations from
namespace hpx::lcos::detail {

    HPX_CXX_CORE_EXPORT template <typename Frame>
    struct dataflow_finalization;
}    // namespace hpx::lcos::detail

#if defined(HPX_HAVE_THREAD_DESCRIPTION)

///////////////////////////////////////////////////////////////////////////
// traits specialization to get annotation from dataflow_finalization
HPX_CXX_CORE_EXPORT template <typename Frame>
struct hpx::traits::get_function_annotation<
    hpx::lcos::detail::dataflow_finalization<Frame>>
{
    using function_type = typename Frame::function_type;

    static constexpr char const* call(
        lcos::detail::dataflow_finalization<Frame> const& f) noexcept
    {
        char const* annotation = hpx::traits::get_function_annotation<
            std::decay_t<function_type>>::call(f.this_->func_);
        return annotation;
    }
};    // namespace hpx::traits

#endif

///////////////////////////////////////////////////////////////////////////////
namespace hpx::lcos::detail {

    HPX_CXX_CORE_EXPORT template <typename Frame>
    struct dataflow_finalization
    {
        explicit dataflow_finalization(Frame* df) noexcept
          : this_(df)
        {
        }

        template <typename Futures>
        void operator()(Futures&& futures) const
        {
            return this_->execute(HPX_FORWARD(Futures, futures));
        }

        // keep the dataflow frame alive with this pointer reference
        hpx::intrusive_ptr<Frame> this_;
    };

    HPX_CXX_CORE_EXPORT template <typename F, typename Args>
    struct dataflow_not_callable
    {
        static auto error(F f, Args args)
        {
            hpx::invoke_fused(HPX_MOVE(f), HPX_MOVE(args));
        }

        using type = decltype(error(std::declval<F>(), std::declval<Args>()));
    };

    ///////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <bool IsAction, typename Policy, typename F,
        typename Args, typename Enable = void>
    struct dataflow_return_impl
    {
        using type = typename dataflow_not_callable<F, Args>::type;
    };

    template <typename Policy, typename F, typename Args>
    struct dataflow_return_impl<false, Policy, F, Args,
        std::enable_if_t<traits::is_launch_policy_v<Policy>>>
    {
        using type = hpx::future<hpx::detail::invoke_fused_result_t<F, Args>>;
    };

    HPX_CXX_CORE_EXPORT template <typename Executor, typename F, typename Args>
    struct dataflow_return_impl_executor;

    template <typename Executor, typename F, typename... Ts>
    struct dataflow_return_impl_executor<Executor, F, hpx::tuple<Ts...>>
    {
        // clang-format off
        using type = decltype(hpx::parallel::execution::async_execute(
            std::declval<Executor&&>(), std::declval<F>(),
            std::declval<Ts>()...));
        // clang-format on
    };

    template <typename Policy, typename F, typename Args>
    struct dataflow_return_impl<false, Policy, F, Args,
        std::enable_if_t<traits::is_one_way_executor_v<Policy> ||
            traits::is_two_way_executor_v<Policy>>>
      : dataflow_return_impl_executor<Policy, F, Args>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Policy, typename F, typename Args>
    struct dataflow_return
      : detail::dataflow_return_impl<traits::is_action_v<F>, Policy, F, Args>
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Policy, typename F, typename Args>
    using dataflow_return_t = typename dataflow_return<Policy, F, Args>::type;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Executor, typename Frame,
        typename Func, typename Futures, typename Enable = void>
    struct has_dataflow_finalize : std::false_type
    {
    };

    // clang-format off
    template <typename Executor, typename Frame, typename Func,
        typename Futures>
    struct has_dataflow_finalize<Executor, Frame, Func, Futures,
        std::void_t<decltype(
            std::declval<Executor>().dataflow_finalize(
            std::declval<Frame>(), std::declval<Func>(),
            std::declval<Futures>())
        )>> : std::true_type
    {
    };
    // clang-format on

    HPX_CXX_CORE_EXPORT template <typename Executor, typename Frame,
        typename Func, typename Futures>
    inline constexpr bool has_dataflow_finalize_v =
        has_dataflow_finalize<Executor, Frame, Func, Futures>::value;

    ///////////////////////////////////////////////////////////////////////////
    template <typename Policy, typename Func, typename Futures>
    struct dataflow_frame    //-V690
      : hpx::lcos::detail::future_data<hpx::traits::future_traits_t<
            detail::dataflow_return_t<Policy, Func, Futures>>>
    {
        using type = detail::dataflow_return_t<Policy, Func, Futures>;
        using result_type = hpx::traits::future_traits_t<type>;
        using base_type = hpx::lcos::detail::future_data<result_type>;

        using is_void = std::is_void<result_type>;

        using function_type = Func;
        using dataflow_type = dataflow_frame<Policy, Func, Futures>;

        friend struct dataflow_finalization<dataflow_type>;
        friend struct traits::get_function_annotation<
            dataflow_finalization<dataflow_type>>;

    private:
        // workaround gcc regression wrongly instantiating constructors
        dataflow_frame();
        dataflow_frame(dataflow_frame const&);

    public:
        using init_no_addref = typename base_type::init_no_addref;

        // A struct to construct the dataflow_frame in-place
        struct construction_data
        {
            Policy policy_;
            Func func_;
        };

        // Construct the dataflow_frame from the given policy and callable
        // object.
        static constexpr construction_data construct_from(
            Policy policy, Func func) noexcept
        {
            return construction_data{HPX_MOVE(policy), HPX_MOVE(func)};
        }

        explicit dataflow_frame(construction_data data) noexcept
          : base_type(init_no_addref{})
          , policy_(HPX_MOVE(data.policy_))
          , func_(HPX_MOVE(data.func_))
        {
        }

    private:
        ///////////////////////////////////////////////////////////////////////
        // Passes the futures into the evaluation function and
        // sets the result future.
        template <typename Futures_>
        HPX_FORCEINLINE void execute(Futures_&& futures)
        {
            hpx::detail::try_catch_exception_ptr(
                [&]() {
                    if constexpr (is_void::value)
                    {
                        hpx::invoke_fused(
                            HPX_MOVE(func_), HPX_FORWARD(Futures_, futures));

                        this->set_data(util::unused_type());
                    }
                    else
                    {
                        this->set_data(hpx::invoke_fused(
                            HPX_MOVE(func_), HPX_FORWARD(Futures_, futures)));
                    }
                },
                [&](std::exception_ptr ep) {
                    this->set_exception(HPX_MOVE(ep));
                });
        }

        ///////////////////////////////////////////////////////////////////////
        template <typename Futures_>
        void finalize(hpx::launch::async_policy policy, Futures_&& futures)
        {
            detail::dataflow_finalization<dataflow_type> this_f_(this);

            hpx::execution::parallel_policy_executor<launch::async_policy> exec{
                policy};

            hpx::parallel::execution::post(
                exec, HPX_MOVE(this_f_), HPX_FORWARD(Futures_, futures));
        }

        template <typename Futures_>
        void finalize(hpx::launch::fork_policy policy, Futures_&& futures)
        {
            detail::dataflow_finalization<dataflow_type> this_f_(this);

            hpx::execution::parallel_policy_executor<launch::fork_policy> exec{
                policy};

            hpx::parallel::execution::post(
                exec, HPX_MOVE(this_f_), HPX_FORWARD(Futures_, futures));
        }

        template <typename Futures_>
        HPX_FORCEINLINE void finalize(
            hpx::launch::sync_policy, Futures_&& futures)
        {
            // We need to run the completion on a new thread if we are on a
            // non HPX thread.
            bool recurse_asynchronously =
                hpx::threads::get_self_ptr() == nullptr;
#if defined(HPX_HAVE_THREADS_GET_STACK_POINTER)
            recurse_asynchronously = !this_thread::has_sufficient_stack_space();
#else
            struct handle_continuation_recursion_count
            {
                handle_continuation_recursion_count()
                  : count_(threads::get_continuation_recursion_count())
                {
                    ++count_;
                }
                ~handle_continuation_recursion_count()
                {
                    --count_;
                }

                std::size_t& count_;
            } cnt;
            recurse_asynchronously = recurse_asynchronously ||
                cnt.count_ > HPX_CONTINUATION_MAX_RECURSION_DEPTH;
#endif
            if (!recurse_asynchronously)
            {
                hpx::scoped_annotation annotate(func_);
                execute(HPX_FORWARD(Futures_, futures));
            }
            else
            {
                finalize(hpx::launch::async, HPX_FORWARD(Futures_, futures));
            }
        }

        template <typename Futures_>
        void finalize(launch policy, Futures_&& futures)
        {
            if (policy == launch::sync)
            {
                finalize(launch::sync, HPX_FORWARD(Futures_, futures));
            }
            else if (policy == launch::fork)
            {
                finalize(launch::fork, HPX_FORWARD(Futures_, futures));
            }
            else
            {
                finalize(launch::async, HPX_FORWARD(Futures_, futures));
            }
        }

        // The overload for hpx::dataflow taking an executor simply forwards
        // to the corresponding executor customization point.
        //
        template <typename Executor, typename Futures_>
        // clang-format off
            requires ((
                traits::is_one_way_executor_v<Executor> ||
                traits::is_two_way_executor_v<Executor>) &&
                !has_dataflow_finalize_v<
                    Executor, dataflow_frame, Func, Futures_>
            )
        // clang-format on
        HPX_FORCEINLINE void finalize(Executor&& exec, Futures_&& futures)
        {
            detail::dataflow_finalization<dataflow_type> this_f_(this);

            hpx::parallel::execution::post(HPX_FORWARD(Executor, exec),
                HPX_MOVE(this_f_), HPX_FORWARD(Futures_, futures));
        }

        template <typename Executor, typename Futures_>
        // clang-format off
            requires ((
                traits::is_one_way_executor_v<Executor> ||
                traits::is_two_way_executor_v<Executor>) &&
                has_dataflow_finalize_v<
                    Executor, dataflow_frame, Func, Futures_>
            )
        // clang-format on
        HPX_FORCEINLINE void finalize(Executor&& exec, Futures_&& futures)
        {
            // uses std::forward for HPX_CUDA_VERSION
            std::forward<Executor>(exec).dataflow_finalize(
                this, HPX_MOVE(func_), HPX_FORWARD(Futures_, futures));
        }

    public:
        // Check whether the current future is ready
        template <typename T>
        auto operator()(util::async_traverse_visit_tag, T&& current)
            -> decltype(async_visit_future(HPX_FORWARD(T, current)))
        {
            return async_visit_future(HPX_FORWARD(T, current));
        }

        // Detach the current execution context and continue when the current
        // future was set to be ready.
        template <typename T, typename N>
        auto operator()(util::async_traverse_detach_tag, T&& current, N&& next)
            -> decltype(async_detach_future(
                HPX_FORWARD(T, current), HPX_FORWARD(N, next)))
        {
            return async_detach_future(
                HPX_FORWARD(T, current), HPX_FORWARD(N, next));
        }

        // Finish the dataflow when the traversal has finished
        template <typename Futures_>
        HPX_FORCEINLINE void operator()(
            util::async_traverse_complete_tag, Futures_&& futures)
        {
            finalize(policy_, HPX_FORWARD(Futures_, futures));
        }

    private:
        Policy policy_;
        Func func_;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Allocator, typename Policy,
        typename Func, typename... Ts,
        typename Frame = dataflow_frame<std::decay_t<Policy>,
            std::decay_t<Func>, hpx::tuple<std::decay_t<Ts>...>>>
    typename Frame::type create_dataflow(
        Allocator const& alloc, Policy&& policy, Func&& func, Ts&&... ts)
    {
        // Create the data that is used to construct the dataflow_frame
        auto data = Frame::construct_from(
            HPX_FORWARD(Policy, policy), HPX_FORWARD(Func, func));

        // Construct the dataflow_frame and traverse the arguments
        // asynchronously
        hpx::intrusive_ptr<Frame> p;
        if constexpr (std::is_same_v<Allocator,
                          hpx::util::internal_allocator<>>)
        {
            p = util::traverse_pack_async(
                util::async_traverse_in_place_tag<Frame>{}, HPX_MOVE(data),
                HPX_FORWARD(Ts, ts)...);
        }
        else
        {
            p = util::traverse_pack_async_allocator(alloc,
                util::async_traverse_in_place_tag<Frame>{}, HPX_MOVE(data),
                HPX_FORWARD(Ts, ts)...);
        }

        using traits::future_access;
        return future_access<typename Frame::type>::create(HPX_MOVE(p));
    }

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <bool IsAction, typename Policy,
        typename Enable = void>
    struct dataflow_dispatch_impl;

    // launch
    template <typename Policy>
    struct dataflow_dispatch_impl<false, Policy,
        std::enable_if_t<traits::is_launch_policy_v<Policy>>>
    {
        template <typename Allocator, typename Policy_, typename F,
            typename... Ts>
        HPX_FORCEINLINE static decltype(auto) call(
            Allocator const& alloc, Policy_&& policy, F&& f, Ts&&... ts)
        {
            return detail::create_dataflow(alloc, HPX_FORWARD(Policy_, policy),
                HPX_FORWARD(F, f),
                traits::acquire_future_disp()(HPX_FORWARD(Ts, ts))...);
        }
    };

    // any action, plain function, or function object
    template <typename FD>
    struct dataflow_dispatch_impl<false, FD,
        std::enable_if_t<!traits::is_launch_policy_v<FD> &&
            !traits::is_one_way_executor_v<FD> &&
            !traits::is_two_way_executor_v<FD>>>
    {
        template <typename Allocator, typename F, typename... Ts>
        HPX_FORCEINLINE static decltype(auto) call(
            Allocator const& alloc, F&& f, Ts&&... ts)
        {
            return detail::create_dataflow(alloc, launch::async,
                HPX_FORWARD(F, f),
                traits::acquire_future_disp()(HPX_FORWARD(Ts, ts))...);
        }
    };
}    // namespace hpx::lcos::detail

namespace hpx::detail {

    // Specializations of dataflow_dispatch_impl (declared in
    // hpx/async_base/dataflow.hpp) providing the actual dispatch logic.

    // Specialization for launch policies
    template <typename Policy>
    struct dataflow_dispatch_impl<Policy,
        std::enable_if_t<hpx::traits::is_launch_policy_v<Policy>>>
    {
        template <typename Allocator, typename Policy_, typename F,
            typename... Ts>
        HPX_FORCEINLINE static decltype(auto) call(
            Allocator const& alloc, Policy_&& policy, F&& f, Ts&&... ts)
        {
            return hpx::lcos::detail::dataflow_dispatch_impl<
                traits::is_action_v<std::decay_t<F>>,
                std::decay_t<Policy_>>::call(alloc,
                HPX_FORWARD(Policy_, policy), HPX_FORWARD(F, f),
                HPX_FORWARD(Ts, ts)...);
        }
    };

    // Specialization for executors
    template <typename Executor>
    struct dataflow_dispatch_impl<Executor,
        std::enable_if_t<!hpx::traits::is_launch_policy_v<Executor> &&
            (hpx::traits::is_one_way_executor_v<Executor> ||
                hpx::traits::is_two_way_executor_v<Executor>)>>
    {
        template <typename Allocator, typename Executor_, typename F,
            typename... Ts>
        HPX_FORCEINLINE static decltype(auto) call(
            Allocator const& alloc, Executor_&& exec, F&& f, Ts&&... ts)
        {
            return hpx::lcos::detail::create_dataflow(alloc,
                HPX_FORWARD(Executor_, exec), HPX_FORWARD(F, f),
                traits::acquire_future_disp()(HPX_FORWARD(Ts, ts))...);
        }
    };

    // Specialization for plain callables (not policy, not executor)
    template <typename FD>
    struct dataflow_dispatch_impl<FD,
        std::enable_if_t<!hpx::traits::is_launch_policy_v<FD> &&
            !hpx::traits::is_one_way_executor_v<FD> &&
            !hpx::traits::is_two_way_executor_v<FD>>>
    {
        template <typename Allocator, typename F, typename... Ts>
        HPX_FORCEINLINE static decltype(auto) call(
            Allocator const& alloc, F&& f, Ts&&... ts)
        {
            return hpx::lcos::detail::dataflow_dispatch_impl<
                traits::is_action_v<std::decay_t<F>>, launch>::call(alloc,
                launch::async, HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }
    };

}    // namespace hpx::detail
