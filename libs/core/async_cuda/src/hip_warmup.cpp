//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if defined(HPX_HAVE_HIP)
#include <hpx/async_cuda/custom_gpu_api.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/executors.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/threading_base.hpp>

#include <string>

// The first use of a device by the HIP runtime (a kernel launch, a copy, a
// memset, a stream query) builds the runtime's internal blit kernels for that
// device with comgr (LLVM) on the calling thread. That needs far more stack
// than an HPX thread has by default (hpx.stacks.small_size): a task that uses
// a device first crashes with a stack overflow. The runtime therefore warms
// the devices up before hpx_main runs, on an HPX thread with a huge stack
// (hpx.hip.warmup: none, current or all devices; default: current).
namespace hpx::cuda::experimental::detail {

    // warm-up of the devices at startup, registered below
    namespace {

        // builds the blit kernels of the current device (takes about two
        // seconds per device); the stream query is the cheapest call that
        // does, hipFree(nullptr) and synchronizations don't
        void warm_up_current_device() noexcept
        {
            (void) hipStreamQuery(nullptr);
            (void) hipGetLastError();
        }

        void warm_up_devices(bool const all) noexcept
        {
            int current = 0;
            if (hipGetDevice(&current) != hipSuccess)
            {
                // no usable device, nothing to warm up
                (void) hipGetLastError();
                return;
            }

            if (!all)
            {
                warm_up_current_device();
                return;
            }

            int count = 0;
            if (hipGetDeviceCount(&count) != hipSuccess)
            {
                (void) hipGetLastError();
                return;
            }
            for (int device = 0; device != count; ++device)
            {
                if (hipSetDevice(device) == hipSuccess)
                {
                    warm_up_current_device();
                }
            }
            (void) hipSetDevice(current);
            (void) hipGetLastError();
        }

        void warm_up()
        {
            std::string const mode =
                hpx::get_config_entry("hpx.hip.warmup", "current");
            if (mode == "none")
            {
                return;
            }
            if (mode != "current" && mode != "all")
            {
                HPX_THROW_EXCEPTION(hpx::error::bad_parameter,
                    "hpx::cuda::experimental::detail::warm_up",
                    "hpx.hip.warmup must be none, current or all, not '{}'",
                    mode);
            }

            auto const exec = hpx::execution::experimental::with_stacksize(
                hpx::execution::parallel_executor(),
                hpx::threads::thread_stacksize::huge);
            hpx::parallel::execution::sync_execute(
                exec, [all = mode == "all"]() { warm_up_devices(all); });
        }

        // registers the warm-up when the library is loaded
        struct register_warm_up
        {
            register_warm_up()
            {
                hpx::register_pre_startup_function(&warm_up);
            }
        };

        register_warm_up const registration;
    }    // namespace
}    // namespace hpx::cuda::experimental::detail
#endif
