//  Copyright (c) 2020 John Biddiscombe
//  Copyright (c) 2026 Sai Charan Arvapally
//  Copyright (c) 2020 Teodor Nikolov
//  Copyright (c) 2024-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>

#if defined(HPX_HAVE_GPU_SUPPORT) && defined(HPX_HAVE_GPUBLAS)
#include <hpx/async_cuda/cuda_exception.hpp>
#include <hpx/async_cuda/cuda_executor.hpp>
#include <hpx/async_cuda/cuda_future.hpp>
#include <hpx/async_cuda/detail/cuda_debug.hpp>
#include <hpx/async_cuda/target.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/futures.hpp>

// CUDA runtime
#include <hpx/async_cuda/custom_gpu_api.hpp>
// CuBLAS
#include <hpx/async_cuda/custom_blas_api.hpp>

#include <cstddef>
#include <exception>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace hpx::cuda::experimental {

    namespace detail {

        HPX_CXX_CORE_EXPORT inline constexpr print_on cub_debug("CUBLAS_");

        // -------------------------------------------------------------------------
        // Error handling in cublas calls
        // not all of these are supported by all cuda/cublas versions
        // (comment them out if they cause compiler errors)
        HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT char const* _cublasGetErrorString(
            cublasStatus_t error);
    }    // namespace detail

    // -------------------------------------------------------------------------
    // exception type for failed launch of cuda functions
    HPX_CXX_CORE_EXPORT struct HPX_ALWAYS_EXPORT cublas_exception
      : hpx::exception
    {
        cublas_exception(std::string const& msg, cublasStatus_t err)
          : hpx::exception(hpx::error::bad_function_call, msg)
          , err_(err)
        {
        }

        cublasStatus_t get_cublas_errorcode() const
        {
            return err_;
        }

    protected:
        cublasStatus_t err_;
    };

    HPX_CXX_CORE_EXPORT HPX_CORE_EXPORT cublasStatus_t check_cublas_error(
        cublasStatus_t err);

    namespace detail {

        // specialization for return type of cublasStatus_t
        HPX_CXX_CORE_EXPORT template <typename... Args>
        struct dispatch_helper<cublasStatus_t, Args...>
        {
            inline cublasStatus_t operator()(
                cublasStatus_t (*f)(Args...), Args... args) const
            {
                cublasStatus_t err = f(args...);
                return check_cublas_error(err);
            }
        };

        HPX_CXX_CORE_EXPORT struct cublas_handle
        {
            static cublasHandle_t create()
            {
                cublasHandle_t handle;
                check_cublas_error(cublasCreate(&handle));
                return handle;
            }

            // deleter for shared_ptr
            void operator()(cublasHandle_t handle) const
            {
                check_cublas_error(cublasDestroy(handle));
            }
        };
    }    // namespace detail

    // -------------------------------------------------------------------------
    // a simple cublas wrapper helper object that can be used to synchronize
    // cublas calls with an hpx future.
    // -------------------------------------------------------------------------
    HPX_CXX_CORE_EXPORT struct cublas_executor : cuda_executor
    {
#ifdef HPX_HAVE_HIP
        // hipblas handle is type : void*
        using handle_ptr = std::shared_ptr<void>;
#else
        // cublas handle is type : struct cublasContext *
        using handle_ptr = std::shared_ptr<struct cublasContext>;
#endif

        // construct a cublas stream
        explicit cublas_executor(std::size_t device,
            cublasPointerMode_t pointer_mode = CUBLAS_POINTER_MODE_HOST,
            bool event_mode = false)
          : hpx::cuda::experimental::cuda_executor(device, event_mode)
          , pointer_mode_(pointer_mode)
        {
            detail::cub_debug.debug(
                debug::str<>("cublas_executor"), "event mode", event_mode);

            handle_ = handle_ptr(
                detail::cublas_handle::create(), detail::cublas_handle{});
        }

        ~cublas_executor() {}

        // -------------------------------------------------------------------------
        // OneWay Execution
        // -------------------------------------------------------------------------
        template <typename F, typename... Ts>
        void post(F&& f, Ts&&... ts) const
        {
            post_impl(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

        // -------------------------------------------------------------------------
        // TwoWay Execution
        // -------------------------------------------------------------------------
        template <typename F, typename... Ts>
        decltype(auto) async_execute(F&& f, Ts&&... ts) const
        {
            return async_impl(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

    protected:
        // This is a simple wrapper for any cublas call, pass in the same arguments
        // that you would use for a cublas call except the cublas handle which is omitted
        // as the wrapper will supply that for you
        template <typename R, typename... Params, typename... Args>
        std::enable_if_t<std::is_same_v<cublasStatus_t, R>, R> post_impl(
            R (*cublas_function)(Params...), Args&&... args) const
        {
            // make sure we run on the correct device
            check_cuda_error(cudaSetDevice(device_));

            // make sure this operation takes place on our stream
            check_cublas_error(cublasSetStream(handle_.get(), stream_));
            check_cublas_error(
                cublasSetPointerMode(handle_.get(), pointer_mode_));

            // insert the cublas handle in the arg list and call the cublas function
            detail::dispatch_helper<R, Params...> helper{};
            return helper(
                cublas_function, handle_.get(), HPX_FORWARD(Args, args)...);
        }

        // -------------------------------------------------------------------------
        // forward a cuda function through to the cuda executor base class
        // (we permit the use of a cublas executor for cuda calls)
        template <typename R, typename... Params, typename... Args>
        inline std::enable_if_t<std::is_same_v<cudaError_t, R>> post_impl(
            R (*cuda_function)(Params...), Args&&... args) const
        {
            return cuda_executor::post_impl(
                cuda_function, HPX_FORWARD(Args, args)...);
        }

        // -------------------------------------------------------------------------
        // TwoWay Execution
        // -------------------------------------------------------------------------
        template <typename F, typename... Ts>
        decltype(auto) async_execute(F&& f, Ts&&... ts) const
        {
            return async_impl(HPX_FORWARD(F, f), HPX_FORWARD(Ts, ts)...);
        }

    protected:
        // -------------------------------------------------------------------------
        // launch a cuBlas function and return a future that will become ready
        // when the task completes, this allows integration of GPU kernels with
        // hpx::futures and the tasking DAG.
        template <typename R, typename... Params, typename... Args>
        hpx::future<std::enable_if_t<std::is_same_v<cublasStatus_t, R>>>
        async_impl(R (*cublas_function)(Params...), Args&&... args) const
        {
            return hpx::detail::try_catch_exception_ptr(
                [&]() {
                    // make sure we run on the correct device
                    check_cuda_error(cudaSetDevice(device_));

                    // make sure this operation takes place on our stream
                    check_cublas_error(cublasSetStream(handle_.get(), stream_));

                    // insert the cublas handle in the arg list and call the
                    // cublas function
                    detail::dispatch_helper<R, Params...> helper;
                    helper(cublas_function, handle_.get(),
                        HPX_FORWARD(Args, args)...);
                    return get_future();
                },
                [&](std::exception_ptr&& ep) {
                    return hpx::make_exceptional_future<void>(HPX_MOVE(ep));
                });
        }

        // -------------------------------------------------------------------------
        // forward a cuda function through to the cuda executor base class
        template <typename R, typename... Params, typename... Args>
        inline hpx::future<std::enable_if_t<std::is_same_v<cudaError_t, R>>>
        async_impl(R (*cuda_function)(Params...), Args&&... args) const
        {
            return cuda_executor::async_impl(
                cuda_function, HPX_FORWARD(Args, args)...);
        }

        // return a copy of the cublas handle
        cublasHandle_t get_handle() const
        {
            return handle_.get();
        }

    protected:
        handle_ptr handle_;
        cublasPointerMode_t pointer_mode_;
    };
}    // namespace hpx::cuda::experimental

namespace hpx::execution::experimental {

    /// \cond NOINTERNAL
    template <>
    struct is_one_way_executor<hpx::cuda::experimental::cublas_executor>
      : std::true_type
    {
        // we support fire and forget without returning a waitable/future
    };

    template <>
    struct is_two_way_executor<hpx::cuda::experimental::cublas_executor>
      : std::true_type
    {
        // we support returning a waitable/future
    };
    /// \endcond
}    // namespace hpx::execution::experimental

#endif
