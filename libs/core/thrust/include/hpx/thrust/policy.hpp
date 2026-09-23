//  Copyright (c)      2025 Aditya Sapra
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/async_cuda.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/thrust/thrust_headers.hpp>

#include <memory>
#include <type_traits>
#include <utility>

namespace hpx::thrust {

    HPX_CXX_CORE_EXPORT struct thrust_task_policy;    // for async ops
    HPX_CXX_CORE_EXPORT template <typename Executor, typename Parameters>
    struct thrust_task_policy_shim;

    HPX_CXX_CORE_EXPORT struct thrust_policy;
    HPX_CXX_CORE_EXPORT template <typename Executor, typename Parameters>
    struct thrust_policy_shim;

    HPX_CXX_CORE_EXPORT struct thrust_host_policy;
    HPX_CXX_CORE_EXPORT struct thrust_device_policy;

    HPX_CXX_CORE_EXPORT struct thrust_task_policy
    {
        using executor_type = hpx::execution::parallel_executor;
        using executor_parameters_type =
            hpx::execution::experimental::extract_executor_parameters<
                executor_type>::type;
        using execution_category = hpx::execution::parallel_execution_tag;

        template <typename Executor_, typename Parameters_>
        struct rebind
        {
            using type = thrust_task_policy_shim<Executor_, Parameters_>;
        };

        constexpr thrust_task_policy() {}

        thrust_task_policy operator()(
            hpx::execution::experimental::to_task_t) const
        {
            return *this;
        }

        static thrust_task_policy_shim<executor_type, executor_parameters_type>
        on(hpx::cuda::experimental::target const& t);

        // HPX execution policy interface
        executor_type executor() const
        {
            return executor_type{};
        }

        executor_parameters_type& parameters()
        {
            return params_;
        }
        constexpr executor_parameters_type const& parameters() const
        {
            return params_;
        }

        // Async helpers with default-target fallback for base policy
        bool has_target() const
        {
            return false;
        }

        hpx::cuda::experimental::target const& target_or_default() const
        {
            return hpx::cuda::experimental::get_default_target();
        }

        cudaStream_t stream() const
        {
            return target_or_default().native_handle().get_stream();
        }

        auto get() const
        {
            return ::thrust::cuda::par_nosync.on(stream());
        }

        hpx::future<void> get_future() const
        {
            return target_or_default().get_future_with_event();
        }

    private:
        executor_parameters_type params_{};
    };

    HPX_CXX_CORE_EXPORT template <typename Executor, typename Parameters>
    struct thrust_task_policy_shim : thrust_task_policy
    {
        using executor_type = Executor;
        using executor_parameters_type = Parameters;
        using execution_category =
            typename hpx::traits::executor_execution_category<
                executor_type>::type;

        template <typename Executor_, typename Parameters_>
        struct rebind
        {
            using type = thrust_task_policy_shim<Executor_, Parameters_>;
        };

        thrust_task_policy_shim operator()(
            hpx::execution::experimental::to_task_t) const
        {
            return *this;
        }

        // Bind a CUDA target explicitly for async GPU execution (returns a new shim)
        thrust_task_policy_shim on(
            hpx::cuda::experimental::target const& t) const
        {
            thrust_task_policy_shim copy = *this;
            copy.bound_target_ =
                std::make_shared<hpx::cuda::experimental::target>(t);
            return copy;
        }

        // Async helpers with default-target fallback
        bool has_target() const
        {
            return static_cast<bool>(bound_target_);
        }

        hpx::cuda::experimental::target const& target_or_default() const
        {
            return bound_target_ ?
                *bound_target_ :
                hpx::cuda::experimental::get_default_target();
        }

        cudaStream_t stream() const
        {
            return target_or_default().native_handle().get_stream();
        }

        auto get() const
        {
            return ::thrust::cuda::par_nosync.on(stream());
        }

        hpx::future<void> get_future() const
        {
            return target_or_default().get_future_with_event();
        }

        // HPX execution policy interface for shim
        Executor& executor()
        {
            return exec_;
        }
        Executor const& executor() const
        {
            return exec_;
        }

        Parameters& parameters()
        {
            return params_;
        }
        Parameters const& parameters() const
        {
            return params_;
        }

        template <typename Dependent = void,
            typename Enable =
                std::enable_if_t<std::is_constructible_v<Executor> &&
                        std::is_constructible_v<Parameters>,
                    Dependent>>
        constexpr thrust_task_policy_shim()
        {
        }

        template <typename Executor_, typename Parameters_>
        constexpr thrust_task_policy_shim(
            Executor_&& exec, Parameters_&& params)
          : exec_(std::forward<Executor_>(exec))
          , params_(std::forward<Parameters_>(params))
        {
        }

        // Construct with an already bound CUDA target
        explicit thrust_task_policy_shim(
            std::shared_ptr<hpx::cuda::experimental::target> tgt)
          : bound_target_(std::move(tgt))
        {
        }

    private:
        Executor exec_{};
        Parameters params_{};
        std::shared_ptr<hpx::cuda::experimental::target> bound_target_{};
    };

    HPX_CXX_CORE_EXPORT inline thrust_task_policy_shim<
        thrust_task_policy::executor_type,
        thrust_task_policy::executor_parameters_type>
    thrust_task_policy::on(hpx::cuda::experimental::target const& t)
    {
        using shim_type =
            thrust_task_policy_shim<executor_type, executor_parameters_type>;
        return shim_type(std::make_shared<hpx::cuda::experimental::target>(t));
    }

    // Base thrust_policy
    HPX_CXX_CORE_EXPORT struct thrust_policy
    {
        using executor_type = hpx::execution::parallel_executor;
        using executor_parameters_type =
            hpx::execution::experimental::extract_executor_parameters<
                executor_type>::type;
        using execution_category = hpx::execution::parallel_execution_tag;

        template <typename Executor_, typename Parameters_>
        struct rebind
        {
            using type = thrust_policy_shim<Executor_, Parameters_>;
        };

        constexpr thrust_policy() {}

        thrust_task_policy operator()(
            hpx::execution::experimental::to_task_t) const
        {
            return thrust_task_policy();
        }

        template <typename Executor_>
        typename hpx::execution::experimental::rebind_executor<thrust_policy,
            Executor_, executor_parameters_type>::type
        on(Executor_&& exec) const
        {
            using executor_type = std::decay_t<Executor_>;
            static_assert(hpx::traits::is_executor_any_v<executor_type>,
                "hpx::traits::is_executor_any_v<Executor>");

            return hpx::execution::experimental::create_rebound_policy(
                thrust_policy(), HPX_FORWARD(Executor_, exec), parameters());
        }

        template <typename... Parameters_,
            typename ParametersType = typename hpx::execution::experimental::
                executor_parameters_join<Parameters_...>::type>
        typename hpx::execution::experimental::rebind_executor<thrust_policy,
            executor_type, ParametersType>::type
        with(Parameters_&&... params) const
        {
            return hpx::execution::experimental::create_rebound_policy(
                thrust_policy(), executor(),
                hpx::execution::experimental::join_executor_parameters(
                    HPX_FORWARD(Parameters_, params)...));
        }

        executor_type executor() const
        {
            return executor_type{};
        }

        executor_parameters_type& parameters()
        {
            return params_;
        }

        [[nodiscard]] constexpr executor_parameters_type const& parameters()
            const
        {
            return params_;
        }

    private:
        executor_parameters_type params_{};
    };

    HPX_CXX_CORE_EXPORT template <typename Executor, typename Parameters>
    struct thrust_policy_shim : thrust_policy
    {
        using executor_type = Executor;
        using executor_parameters_type = Parameters;
        using execution_category =
            typename hpx::traits::executor_execution_category<
                executor_type>::type;

        template <typename Executor_, typename Parameters_>
        struct rebind
        {
            using type = thrust_policy_shim<Executor_, Parameters_>;
        };

        thrust_task_policy_shim<Executor, Parameters> operator()(
            hpx::execution::experimental::to_task_t) const
        {
            return thrust_task_policy_shim<Executor, Parameters>(
                exec_, params_);
        }

        template <typename Executor_>
        typename hpx::execution::experimental::rebind_executor<
            thrust_policy_shim, Executor_, executor_parameters_type>::type
        on(Executor_&& exec) const
        {
            using executor_type = std::decay_t<Executor_>;
            static_assert(hpx::traits::is_executor_any_v<executor_type>,
                "hpx::traits::is_executor_any_v<Executor>");

            return hpx::execution::experimental::create_rebound_policy(
                thrust_policy_shim(), HPX_FORWARD(Executor_, exec),
                parameters());
        }

        template <typename... Parameters_,
            typename ParametersType = typename hpx::execution::experimental::
                executor_parameters_join<Parameters_...>::type>
        typename hpx::execution::experimental::rebind_executor<
            thrust_policy_shim, executor_type, ParametersType>::type
        with(Parameters_&&... params) const
        {
            return hpx::execution::experimental::create_rebound_policy(
                thrust_policy_shim(), executor(),
                hpx::execution::experimental::join_executor_parameters(
                    HPX_FORWARD(Parameters_, params)...));
        }

        Executor& executor()
        {
            return exec_;
        }

        Executor const& executor() const
        {
            return exec_;
        }

        Parameters& parameters()
        {
            return params_;
        }

        Parameters const& parameters() const
        {
            return params_;
        }

        template <typename Dependent = void,
            typename Enable =
                std::enable_if_t<std::is_constructible_v<Executor> &&
                        std::is_constructible_v<Parameters>,
                    Dependent>>
        constexpr thrust_policy_shim()
        {
        }

        template <typename Executor_, typename Parameters_>
        constexpr thrust_policy_shim(Executor_&& exec, Parameters_&& params)
          : exec_(std::forward<Executor_>(exec))
          , params_(std::forward<Parameters_>(params))
        {
        }

    private:
        Executor exec_;
        Parameters params_;
    };

    // Host-specific policy that inherits from thrust_policy
    HPX_CXX_CORE_EXPORT struct thrust_host_policy : thrust_policy
    {
        constexpr thrust_host_policy() = default;

        // Return thrust::host execution policy
        constexpr auto get() const
        {
            return ::thrust::host;
        }
    };

    // Device-specific policy that inherits from thrust_policy
    HPX_CXX_CORE_EXPORT struct thrust_device_policy : thrust_policy
    {
        constexpr thrust_device_policy() = default;

        // Return thrust::device execution policy
        static constexpr auto get()
        {
            return ::thrust::device;
        }
    };

    // Global policy instances
    HPX_CXX_CORE_EXPORT inline constexpr thrust_host_policy thrust_host{};
    HPX_CXX_CORE_EXPORT inline constexpr thrust_device_policy thrust_device{};

    // Legacy support - default thrust policy (keep for backward compatibility)
    HPX_CXX_CORE_EXPORT inline constexpr thrust_policy thrust;

    HPX_CXX_CORE_EXPORT template <typename ExecutionPolicy>
    struct is_thrust_execution_policy : std::false_type
    {
    };

    template <>
    struct is_thrust_execution_policy<hpx::thrust::thrust_policy>
      : std::true_type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Executor, typename Parameters>
    struct is_thrust_execution_policy<
        hpx::thrust::thrust_policy_shim<Executor, Parameters>> : std::true_type
    {
    };

    template <>
    struct is_thrust_execution_policy<hpx::thrust::thrust_host_policy>
      : std::true_type
    {
    };

    template <>
    struct is_thrust_execution_policy<hpx::thrust::thrust_device_policy>
      : std::true_type
    {
    };

    template <>
    struct is_thrust_execution_policy<hpx::thrust::thrust_task_policy>
      : std::true_type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename Executor, typename Parameters>
    struct is_thrust_execution_policy<
        hpx::thrust::thrust_task_policy_shim<Executor, Parameters>>
      : std::true_type
    {
    };

    HPX_CXX_CORE_EXPORT template <typename T>
    inline constexpr bool is_thrust_execution_policy_v =
        is_thrust_execution_policy<T>::value;

    namespace detail {

        HPX_CXX_CORE_EXPORT template <typename ExecutionPolicy,
            typename Enable = void>
        struct get_policy_result;

        HPX_CXX_CORE_EXPORT template <typename ExecutionPolicy>
        struct get_policy_result<ExecutionPolicy,
            std::enable_if_t<hpx::is_async_execution_policy_v<
                std::decay_t<ExecutionPolicy>>>>
        {
            static_assert(is_thrust_execution_policy<
                              std::decay_t<ExecutionPolicy>>::value,
                "get_policy_result can only be used with Thrust execution "
                "policies");

            using type = hpx::future<void>;

            template <typename Future>
            static constexpr decltype(auto) call(Future&& future)
            {
                return std::forward<Future>(future);
            }
        };

        HPX_CXX_CORE_EXPORT template <typename ExecutionPolicy>
        struct get_policy_result<ExecutionPolicy,
            std::enable_if_t<!hpx::is_async_execution_policy_v<
                std::decay_t<ExecutionPolicy>>>>
        {
            static_assert(is_thrust_execution_policy<
                              std::decay_t<ExecutionPolicy>>::value,
                "get_policy_result can only be used with Thrust execution "
                "policies");

            template <typename Future>
            static constexpr decltype(auto) call(Future&& future)
            {
                return std::forward<Future>(future).get();
            }
        };
    }    // namespace detail
}    // namespace hpx::thrust

namespace hpx::detail {

    // Register each thrust execution policy defined above with
    // policy_traits, replacing what used to be a separate specialization of
    // is_execution_policy, is_parallel_execution_policy,
    // is_async_execution_policy, and is_rebound_execution_policy for each of
    // the policies below.
    template <>
    struct policy_traits<hpx::thrust::thrust_policy> : policy_traits_default
    {
        static constexpr bool is_policy = true;
        static constexpr bool is_parallel = true;
    };

    template <typename Executor, typename Parameters>
    struct policy_traits<hpx::thrust::thrust_policy_shim<Executor, Parameters>>
      : policy_traits_default
    {
        static constexpr bool is_policy = true;
        static constexpr bool is_rebound = true;
        static constexpr bool is_parallel = true;
    };

    template <>
    struct policy_traits<hpx::thrust::thrust_host_policy>
      : policy_traits_default
    {
        static constexpr bool is_policy = true;
        static constexpr bool is_parallel = true;
    };

    template <>
    struct policy_traits<hpx::thrust::thrust_device_policy>
      : policy_traits_default
    {
        static constexpr bool is_policy = true;
        static constexpr bool is_parallel = true;
    };

    template <>
    struct policy_traits<hpx::thrust::thrust_task_policy>
      : policy_traits_default
    {
        static constexpr bool is_policy = true;
        static constexpr bool is_parallel = true;
        static constexpr bool is_async = true;
    };

    template <typename Executor, typename Parameters>
    struct policy_traits<
        hpx::thrust::thrust_task_policy_shim<Executor, Parameters>>
      : policy_traits_default
    {
        static constexpr bool is_policy = true;
        static constexpr bool is_rebound = true;
        static constexpr bool is_parallel = true;
        static constexpr bool is_async = true;
    };
}    // namespace hpx::detail
