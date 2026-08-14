//  Copyright (c) 2022 Gregor Daiss
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// hpxinspect:noascii

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/threading_base.hpp>

#include <string>

#include <sycl/sycl.hpp>

// Check compiler compatibility:
// Needs to be done AFTER sycl include for HipSYCL
// (intel dpcpp would be fine without the include)
//
// Will raise compile-time errors/warning in case of a unexpected configuration!
//
// This file is a good point to add this check as it will get included by
// sycl_future.hpp and thus indirectly by sycl_executor.hpp
#if defined(SYCL_LANGUAGE_VERSION)
#if !defined(__HIPSYCL__) &&                                                   \
    !(defined(__INTEL_LLVM_COMPILER) ||                                        \
        (defined(__clang__) && defined(SYCL_IMPLEMENTATION_ONEAPI) &&          \
            defined(SYCL_IMPLEMENTATION_ONEAPI)))
#warning "HPX-SYCL integration only tested with Intel oneapi and HipSYCL. \
Utilized compiler appears to be neither of those!"
#endif
#else
#error                                                                         \
    "Compiler does not seem to support SYCL! SYCL_LANGUAGE_VERSION is undefined!"
#endif
#if defined(__SYCL_SINGLE_SOURCE__)
#warning "SYCL single source compiler not tested! Use one with multiple passes"
#endif

namespace hpx { namespace sycl { namespace experimental { namespace detail {

    /// Type of the event_callback function used. Unlike the CUDA counterpart we are
    /// not using an error code parameter as SYCL does not provide one
    using event_callback_function_type = hpx::move_only_function<void(void)>;

    /// Add callback to be called when all commands up to and including
    /// the passed event are done
    /** Adds an SYCL event directly to the sycl event_callback queue/vector
     * (without using a dummy kernel).
     * The event is polled periodically by the scheduler.
     * When done, the callback function f will be called.
     * Intended to be used with a callback function that sets the future data of an
     * hpx::future representing the completion of an asynchronous SYCL kernel call.
     * NOTE: For hipsycl it is required to flush the internal DAG of the queue which is
     * done by this method as well
    */
    HPX_CORE_EXPORT void add_event_callback(
        event_callback_function_type&& f, ::sycl::event event);

    /// Register SYCL event polling function with the scheduler (see scheduler_base.hpp)
    HPX_CORE_EXPORT void register_polling(hpx::threads::thread_pool_base& pool);
    /// Unregister SYCL event polling function -- only use when all kernels are done
    HPX_CORE_EXPORT void unregister_polling(
        hpx::threads::thread_pool_base& pool);
}}}}    // namespace hpx::sycl::experimental::detail
