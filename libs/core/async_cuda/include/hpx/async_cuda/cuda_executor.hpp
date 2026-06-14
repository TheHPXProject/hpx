//  Copyright (c) 2020 John Biddiscombe
//  Copyright (c) 2020 Teodor Nikolov
//  Copyright (c) 2024-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/async_cuda/cuda_exception.hpp>
#include <hpx/async_cuda/cuda_future.hpp>
#include <hpx/async_cuda/target.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/futures.hpp>

// CUDA runtime
#include <hpx/async_cuda/custom_gpu_api.hpp>

#include <cstddef>
#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

namespace hpx::cuda::experimental {

    namespace detail {
        // -------------------------------------------------------------------------
        // A helper object to call a cuda function returning a cudaError type
        // or a plain kernel definition (or cublas function in cublas executor)
        HPX_CXX_CORE_EXPORT template <typename R, typename... Args>
        struct dispatch_helper;

        // default implementation - call the function
        HPX_CXX_CORE_EXPORT template <typename R, typename... Args>
        struct dispatch_helper
        {
            inline R operator()(R (*f)(Args...), Args... args) const
            {
                return f(args...);
            }
        };

        // specialization for return type void
        HPX_CXX_CORE_EXPORT template <typename... Args>
        struct dispatch_helper<void, Args...>
        {
            inline void operator()(void (*f)(Args...), Args... args) const
            {
                f(args...);
            }
        };

        // specialization for return type of cudaError_t
        HPX_CXX_CORE_EXPORT template <typename... Args>
        struct dispatch_helper<cudaError_t, Args...>
        {
            inline void operator()(
                cudaError_t (*f)(Args...), Args... args) const
            {
                check_cuda_error(f(args...));
            }
        };
    }    // namespace detail

    // -------------------------------------------------------------------------
    // Allows the launching of cuda functions and kernels on a stream with futures
    // returned that are set when the async functions/kernels are ready
    // -------------------------------------------------------------------------
    HPX_CXX_CORE_EXPORT struct cuda_executor_base
    {
        using future_type = hpx::future<void>;

        // -------------------------------------------------------------------------
        // constructors - create a cuda stream that all tasks invoked by
        // this helper will use
        // assume event mode is the default
        cuda_executor_base(std::size_t device, bool const event_mode)
          : device_(static_cast<int>(device))
          , event_mode_(event_mode)
        {
            target_ =
                std::make_shared<hpx::cuda::experimental::target>(device_);
            stream_ = target_->native_handle().get_stream();
        }

        inline future_type get_future() const
        {
            if (event_mode_)
            {
                return target_->get_future_with_event();
            }
            return target_->get_future_with_callback();
        }

    protected:
        int device_;
        bool event_mode_;
        cudaStream_t stream_;
        std::shared_ptr<hpx::cuda::experimental::target> target_;
    };

    // -------------------------------------------------------------------------
    // Allows you to launch kernels on a stream and get
    // futures back when they are ready
    // -------------------------------------------------------------------------
    HPX_CXX_CORE_EXPORT struct cuda_executor : cuda_executor_base
    {
        // -------------------------------------------------------------------------
        // construct - create a cuda stream that all tasks invoked by
        // this helper will use
        explicit cuda_executor(
            std::size_t const device, bool const event_mode = true)
          : cuda_executor_base(device, event_mode)
        {
        }

        // -------------------------------------------------------------------------
        // target destructor will clean up stream handle
        ~cuda_executor() {}

        // -------------------------------------------------------------------------
        // TwoWay Execution - async_execute delegates to async()
        template <typename F, typename... Ts>
        decltype(auto) async_execute(F&& f, Ts&&... ts) const
        {
            return async(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

    public:
        // -------------------------------------------------------------------------
        // launch a kernel on our stream and return without a future
        // the return value is the value returned from the cuda call
        // (typically this will be cudaError_t).
        // Throws cuda_exception if the async launch fails.
        template <typename R, typename... Params, typename... Args>
        void post(R (*cuda_function)(Params...), Args&&... args) const
        {
            // make sure we run on the correct device
            check_cuda_error(cudaSetDevice(device_));

            // insert the stream handle in the arg list and call the cuda function
            detail::dispatch_helper<R, Params...> helper{};
            helper(cuda_function, HPX_FORWARD(Args, args)..., stream_);
        }

        // -------------------------------------------------------------------------
        // launch a kernel on our stream and return a future that will become ready
        // when the task completes, this allows integregration of GPU kernels with
        // hpx::futures and the tasking DAG.
        // Puts a cuda_exception in the future if the async launch fails.
        template <typename R, typename... Params, typename... Args>
        hpx::future<void> async(
            R (*cuda_kernel)(Params...), Args&&... args) const
        {
            return hpx::detail::try_catch_exception_ptr(
                [&]() {
                    // make sure we run on the correct device
                    check_cuda_error(cudaSetDevice(device_));

                    // insert the stream handle in the arg list and call the cuda function
                    detail::dispatch_helper<R, Params...> helper{};
                    helper(cuda_kernel, HPX_FORWARD(Args, args)..., stream_);
                    return get_future();
                },
                [&](std::exception_ptr&& ep) {
                    return hpx::make_exceptional_future<void>(HPX_MOVE(ep));
                });
        }
    };
}    // namespace hpx::cuda::experimental

namespace hpx::execution::experimental {

    /// \cond NOINTERNAL
    template <>
    struct is_one_way_executor<hpx::cuda::experimental::cuda_executor>
      : std::true_type
    {
        // we support fire and forget without returning a waitable/future
    };

    template <>
    struct is_two_way_executor<hpx::cuda::experimental::cuda_executor>
      : std::true_type
    {
        // we support returning a waitable/future
    };
    /// \endcond
}    // namespace hpx::execution::experimental
