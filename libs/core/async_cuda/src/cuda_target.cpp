//  Copyright (c) 2016 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#include <hpx/assert.hpp>
#include <hpx/async_cuda/target.hpp>
#include <hpx/modules/allocator_support.hpp>
#include <hpx/modules/config_registry.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/threading_base.hpp>

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

#include <hpx/async_cuda/custom_gpu_api.hpp>

namespace hpx::cuda::experimental {

    namespace {

        // the runtime and the driver of the GPUs this process loaded, and the
        // runtime HPX was built with, for hpx::complete_version()
        std::string gpu_runtime_version()
        {
            int runtime = 0;
            int driver = 0;
            if (cudaRuntimeGetVersion(&runtime) != cudaSuccess)
                runtime = 0;
            if (cudaDriverGetVersion(&driver) != cudaSuccess)
                driver = 0;
#if defined(HPX_HAVE_HIP)
            // major * 10000000 + minor * 100000 + patch
            auto const text = [](int v) {
                return hpx::util::format(
                    "V{}.{}.{}", v / 10000000, (v / 100000) % 100, v % 100000);
            };
            return hpx::util::format("runtime {}, driver {} (built with "
                                     "V{}.{}.{})",
                text(runtime), driver != 0 ? text(driver) : "none",
                HIP_VERSION_MAJOR, HIP_VERSION_MINOR, HIP_VERSION_PATCH);
#else
            // major * 1000 + minor * 10
            auto const text = [](int v) {
                return hpx::util::format("V{}.{}", v / 1000, (v % 1000) / 10);
            };
            return hpx::util::format("runtime {}, driver {} (built with {})",
                text(runtime), driver != 0 ? text(driver) : "none",
                text(CUDART_VERSION));
#endif
        }

        config_registry::add_library_version_helper register_gpu_version{
#if defined(HPX_HAVE_HIP)
            {"HIP", &gpu_runtime_version}};
#else
            {"CUDA", &gpu_runtime_version}};
#endif
    }    // namespace

    void target::native_handle_type::init_processing_units()
    {
        cudaDeviceProp props;
        if (cudaError_t error = cudaGetDeviceProperties(&props, device_);
            error != cudaSuccess)
        {
            // report error
            HPX_THROW_EXCEPTION(hpx::error::kernel_error,
                "cuda::init_processing_unit()",
                std::string("cudaGetDeviceProperties failed: ") +
                    cudaGetErrorString(error));
        }

        std::size_t mp = props.multiProcessorCount;
        switch (props.major)
        {
        case 2:    // Fermi
            if (props.minor == 1)
            {
                mp = mp * 48;
            }
            else
            {
                mp = mp * 32;
            }
            break;
        case 3:    // Kepler
            mp = mp * 192;
            break;
        case 5:    // Maxwell
            mp = mp * 128;
            break;
        case 6:    // Pascal
            mp = mp * 64;
            break;
        default:
            break;
        }
        processing_units_ = mp;
        processor_family_ = props.major;
        processor_name_ = props.name;
    }

    target::native_handle_type::native_handle_type(int device)
      : device_(device)
      , stream_(nullptr)
    {
        init_processing_units();
    }

    target::native_handle_type::~native_handle_type()
    {
        reset();
    }

    void target::native_handle_type::reset() const noexcept
    {
        if (stream_)
        {
            [[maybe_unused]] cudaError_t err =
                cudaStreamDestroy(stream_);    // ignore error
        }
    }

    target::native_handle_type::native_handle_type(
        target::native_handle_type const& rhs) noexcept
      : device_(rhs.device_)
      , processing_units_(rhs.processing_units_)
      , processor_family_(rhs.processor_family_)
      , processor_name_(rhs.processor_name_)
      , stream_(nullptr)
    {
    }

    target::native_handle_type::native_handle_type(
        target::native_handle_type&& rhs) noexcept
      : device_(rhs.device_)
      , processing_units_(rhs.processing_units_)
      , processor_family_(rhs.processor_family_)
      , processor_name_(rhs.processor_name_)
      , stream_(rhs.stream_)
    {
        rhs.stream_ = nullptr;
    }

    target::native_handle_type& target::native_handle_type::operator=(
        target::native_handle_type const& rhs) noexcept
    {
        if (this == &rhs)
            return *this;

        device_ = rhs.device_;
        processing_units_ = rhs.processing_units_;
        processor_family_ = rhs.processor_family_;
        processor_name_ = rhs.processor_name_;
        reset();

        return *this;
    }

    target::native_handle_type& target::native_handle_type::operator=(
        target::native_handle_type&& rhs) noexcept
    {
        if (this == &rhs)
            return *this;

        device_ = rhs.device_;
        processing_units_ = rhs.processing_units_;
        processor_family_ = rhs.processor_family_;
        processor_name_ = rhs.processor_name_;
        stream_ = rhs.stream_;
        rhs.stream_ = nullptr;

        return *this;
    }

    cudaStream_t target::native_handle_type::get_stream() const
    {
        std::lock_guard<mutex_type> l(mtx_);

        if (stream_ == nullptr)
        {
            cudaError_t error = cudaSetDevice(device_);
            if (error != cudaSuccess)
            {
                HPX_THROW_EXCEPTION(hpx::error::kernel_error,
                    "cuda::experimental::target::native_handle::get_stream()",
                    std::string("cudaSetDevice failed: ") +
                        cudaGetErrorString(error));
            }

            error = cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking);
            if (error != cudaSuccess)
            {
                HPX_THROW_EXCEPTION(hpx::error::kernel_error,
                    "cuda::experimental::target::native_handle::get_stream()",
                    std::string("cudaStreamCreate failed: ") +
                        cudaGetErrorString(error));
            }
        }
        return stream_;
    }

    ///////////////////////////////////////////////////////////////////////////
    void target::synchronize() const
    {
        cudaStream_t const stream = handle_.get_stream();

        if (stream == nullptr)
        {
            HPX_THROW_EXCEPTION(hpx::error::invalid_status,
                "cuda::experimental::target::synchronize",
                "no stream available");
        }

        if (cudaError_t const error = cudaStreamSynchronize(stream);
            error != cudaSuccess)
        {
            HPX_THROW_EXCEPTION(hpx::error::kernel_error,
                "cuda::experimental::target::synchronize",
                std::string("cudaStreamSynchronize failed: ") +
                    cudaGetErrorString(error));
        }
    }

    hpx::future<void> target::get_future_with_event() const
    {
        return detail::get_future_with_event(
            handle_.get_stream(), handle_.get_device());
    }

    hpx::future<void> target::get_future_with_callback() const
    {
        return detail::get_future_with_callback(handle_.get_stream());
    }

    target& get_default_target()
    {
        static target target_;
        return target_;
    }
}    // namespace hpx::cuda::experimental
