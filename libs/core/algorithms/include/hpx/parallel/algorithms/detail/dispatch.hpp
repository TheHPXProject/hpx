//  Copyright (c) 2007-2025 Hartmut Kaiser
//  Copyright (c) 2021 Giannis Gonidelis
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/algorithms/traits/segmented_iterator_traits.hpp>
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/type_support.hpp>
#include <hpx/parallel/util/detail/algorithm_result.hpp>
#include <hpx/parallel/util/detail/scoped_executor_parameters.hpp>
#include <hpx/parallel/util/result_types.hpp>

#include <exception>
#if defined(HPX_HAVE_CXX17_STD_EXECUTION_POLICES)
#include <execution>
#endif
#include <type_traits>
#include <utility>

namespace hpx::parallel::detail {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Result>
    struct local_algorithm_result
    {
        using type = typename hpx::traits::segmented_local_iterator_traits<
            Result>::local_raw_iterator;
    };

    template <typename Result1, typename Result2>
    struct local_algorithm_result<util::in_out_result<Result1, Result2>>
    {
        using type1 = typename hpx::traits::segmented_local_iterator_traits<
            Result1>::local_raw_iterator;
        using type2 = typename hpx::traits::segmented_local_iterator_traits<
            Result2>::local_raw_iterator;

        using type = util::in_out_result<type1, type2>;
    };

    template <typename Result>
    struct local_algorithm_result<util::min_max_result<Result>>
    {
        using type1 = typename hpx::traits::segmented_local_iterator_traits<
            Result>::local_raw_iterator;

        using type = util::min_max_result<type1>;
    };

    template <typename Result1, typename Result2, typename Result3>
    struct local_algorithm_result<
        util::in_in_out_result<Result1, Result2, Result3>>
    {
        using type1 = typename hpx::traits::segmented_local_iterator_traits<
            Result1>::local_raw_iterator;
        using type2 = typename hpx::traits::segmented_local_iterator_traits<
            Result2>::local_raw_iterator;
        using type3 = typename hpx::traits::segmented_local_iterator_traits<
            Result3>::local_raw_iterator;

        using type = util::in_in_out_result<type1, type2, type3>;
    };

    template <>
    struct local_algorithm_result<void>
    {
        using type = void;
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    using local_algorithm_result_t = typename local_algorithm_result<T>::type;

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_CORE_EXPORT template <typename Derived, typename Result = void>
    struct algorithm
    {
    private:
        [[nodiscard]] constexpr Derived const& derived() const noexcept
        {
            return static_cast<Derived const&>(*this);
        }

    public:
        // Algorithms with legacy future-based task graphs opt in to the
        // scheduler adapter. Algorithms with native sender paths leave this
        // false and receive the scheduler policy directly.
        static constexpr bool uses_futures = false;

        using result_type = Result;
        using local_result_type = local_algorithm_result_t<result_type>;

    private:
        template <typename Policy>
        struct future_algorithm_error_handler
        {
            auto operator()(std::exception_ptr error) const
            {
                namespace ex = hpx::execution::experimental;
                using policy_type =
                    decltype(ex::to_non_task(std::declval<Policy>()));
                using exception_handler =
                    hpx::parallel::detail::handle_exception<policy_type,
                        local_result_type>;
                using error_sender_type = decltype(ex::just_error(error));

                return hpx::detail::try_catch_exception_ptr(
                    [&]() -> error_sender_type {
                        exception_handler::call(error);
                        HPX_UNREACHABLE;
                    },
                    [](std::exception_ptr transformed_error) {
                        return ex::just_error(HPX_MOVE(transformed_error));
                    });
            }
        };

        template <typename Policy>
        struct future_algorithm_invoker
        {
            Policy policy;

            template <typename... Ts>
            auto operator()(Ts&&... values)
            {
                namespace ex = hpx::execution::experimental;
                auto sender = ex::as_sender(Derived{}.call(
                    HPX_MOVE(policy), HPX_FORWARD(Ts, values)...));
                return ex::let_error(
                    HPX_MOVE(sender), future_algorithm_error_handler<Policy>{});
            }
        };

        template <typename Policy, typename Sender>
        [[noreturn]] static Sender handle_future_algorithm_error(
            std::exception_ptr error)
        {
            namespace ex = hpx::execution::experimental;
            using policy_type =
                decltype(ex::to_non_task(std::declval<Policy>()));
            hpx::parallel::detail::handle_exception<policy_type,
                local_result_type>::call(error);
            HPX_UNREACHABLE;
        }

        template <typename Policy, typename Tuple>
        struct future_algorithm_sender_factory
        {
            Policy policy;
            Tuple args;

            auto operator()()
            {
                using sender_type = decltype(hpx::invoke_fused(
                    future_algorithm_invoker<Policy>{HPX_MOVE(policy)},
                    HPX_MOVE(args)));

                return hpx::detail::try_catch_exception_ptr(
                    [&]() -> sender_type {
                        return hpx::invoke_fused(
                            future_algorithm_invoker<Policy>{HPX_MOVE(policy)},
                            HPX_MOVE(args));
                    },
                    &handle_future_algorithm_error<Policy, sender_type>);
            }
        };

    public:
        // NOLINTNEXTLINE(bugprone-crtp-constructor-accessibility)
        explicit constexpr algorithm(char const* const name) noexcept
          : name_(name)
        {
        }

        ///////////////////////////////////////////////////////////////////////
        // this equivalent to sequential execution
        // clang-format off
        template <typename ExPolicy, typename... Args>
        HPX_HOST_DEVICE decltype(auto) operator()(
            ExPolicy&& policy, Args&&... args) const
        // clang-format on
        {
#if !defined(__CUDA_ARCH__)
            try
            {
#endif
                using parameters_type =
                    typename std::decay_t<ExPolicy>::executor_parameters_type;
                using executor_type =
                    typename std::decay_t<ExPolicy>::executor_type;

                hpx::parallel::util::detail::scoped_executor_parameters_ref<
                    parameters_type, executor_type>
                    scoped_param(policy.parameters(), policy.executor());

                return Derived::sequential(
                    HPX_FORWARD(ExPolicy, policy), HPX_FORWARD(Args, args)...);
#if !defined(__CUDA_ARCH__)
            }
            catch (...)
            {
                // this does not return
                using policy_type =
                    decltype(hpx::execution::experimental::to_non_task(
                        std::declval<ExPolicy&&>()));

                return hpx::parallel::detail::handle_exception<policy_type,
                    std::conditional_t<std::is_void_v<local_result_type>,
                        hpx::util::unused_type, local_result_type>>::call();
            }
#endif
        }

    public:
        ///////////////////////////////////////////////////////////////////////
        // main sequential dispatch entry points
        template <typename ExPolicy, typename... Args>
        constexpr decltype(auto) call2(
            ExPolicy&& policy, std::true_type, Args&&... args) const
        {
            using result_handler =
                hpx::parallel::util::detail::algorithm_result<ExPolicy,
                    local_result_type>;

            decltype(auto) exec = policy.executor();    // avoid use after move
            if constexpr (hpx::is_async_execution_policy_v<ExPolicy>)
            {
                // specialization for all task-based (asynchronous) execution
                // policies

                // run the launched task on the requested executor
                return result_handler::get(execution::async_execute(
                    exec, derived(), policy, HPX_FORWARD(Args, args)...));
            }
            else if constexpr (std::is_void_v<local_result_type>)
            {
                execution::sync_execute(
                    exec, derived(), policy, HPX_FORWARD(Args, args)...);
                return result_handler::get();
            }
            else
            {
                return result_handler::get(execution::sync_execute(
                    exec, derived(), policy, HPX_FORWARD(Args, args)...));
            }
        }

        // main parallel dispatch entry point
        template <typename ExPolicy, typename... Args>
        HPX_FORCEINLINE static constexpr decltype(auto) call2(
            ExPolicy&& policy, std::false_type, Args&&... args)
        {
            using result_handler =
                hpx::parallel::util::detail::algorithm_result<ExPolicy,
                    local_result_type>;

            if constexpr (hpx::execution_policy_has_scheduler_executor_v<
                              ExPolicy> &&
                Derived::uses_futures)
            {
                namespace ex = hpx::execution::experimental;
                auto sched = policy.executor().sched();
                auto future_policy =
                    ex::to_task(policy.on(ex::scheduler_executor(sched)));
                using future_policy_type = decltype(future_policy);
                static_assert(!hpx::execution_policy_has_scheduler_executor_v<
                                  future_policy_type>,
                    "the future adapter policy must leave scheduler dispatch");
                static_assert(std::is_default_constructible_v<Derived>,
                    "future-based algorithms must be default constructible");
                auto args_tuple = hpx::make_tuple(HPX_FORWARD(Args, args)...);
                using args_tuple_type = decltype(args_tuple);
                auto sender = ex::let_value(ex::schedule(sched),
                    future_algorithm_sender_factory<future_policy_type,
                        args_tuple_type>{
                        HPX_MOVE(future_policy), HPX_MOVE(args_tuple)});
                return result_handler::get(
                    ex::continues_on(HPX_MOVE(sender), HPX_MOVE(sched)));
            }
            else
            {
                using result = decltype(Derived::parallel(
                    HPX_FORWARD(ExPolicy, policy), HPX_FORWARD(Args, args)...));

                // Executor customizations can make void algorithms such as
                // for_loop return void directly instead of a future.
                if constexpr (std::is_void_v<result>)
                {
                    Derived::parallel(HPX_FORWARD(ExPolicy, policy),
                        HPX_FORWARD(Args, args)...);
                    return result_handler::get();
                }
                else
                {
                    return result_handler::get(
                        Derived::parallel(HPX_FORWARD(ExPolicy, policy),
                            HPX_FORWARD(Args, args)...));
                }
            }
        }

        template <typename ExPolicy, typename... Args>
        HPX_FORCEINLINE constexpr decltype(auto) call(
            ExPolicy&& policy, Args&&... args)
        {
            using is_seq = hpx::is_sequenced_execution_policy<ExPolicy>;
            return call2(HPX_FORWARD(ExPolicy, policy), is_seq(),
                HPX_FORWARD(Args, args)...);
        }

#if defined(HPX_HAVE_CXX17_STD_EXECUTION_POLICES)
        // main dispatch entry points for std execution policies
        template <typename... Args>
        HPX_FORCEINLINE constexpr decltype(auto) call(
            std::execution::sequenced_policy, Args&&... args)
        {
            return call2(hpx::execution::seq, std::true_type(),
                HPX_FORWARD(Args, args)...);
        }

        template <typename... Args>
        HPX_FORCEINLINE constexpr decltype(auto) call(
            std::execution::parallel_policy, Args&&... args)
        {
            return call2(hpx::execution::par, std::false_type(),
                HPX_FORWARD(Args, args)...);
        }

        template <typename... Args>
        HPX_FORCEINLINE constexpr decltype(auto) call(
            std::execution::parallel_unsequenced_policy, Args&&... args)
        {
            return call2(hpx::execution::par_unseq, std::false_type(),
                HPX_FORWARD(Args, args)...);
        }

#if defined(HPX_HAVE_CXX20_STD_EXECUTION_POLICES)
        template <typename... Args>
        HPX_FORCEINLINE constexpr decltype(auto) call(
            std::execution::unsequenced_policy, Args&&... args)
        {
            return call2(hpx::execution::unseq, std::false_type(),
                HPX_FORWARD(Args, args)...);
        }
#endif
#endif

    private:
        char const* const name_;

        friend class hpx::serialization::access;

        template <typename Archive>
        static constexpr void serialize(Archive&, unsigned int) noexcept
        {
            // no need to serialize 'name_' as it is always initialized by the
            // default constructor of the derived class
        }
    };
}    // namespace hpx::parallel::detail
