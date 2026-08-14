//  Copyright (c) 2022 Gregor Daiss
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// hpxinspect:noascii

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/async_sycl/sycl_future.hpp>
#include <hpx/modules/allocator_support.hpp>

#include <exception>
#include <iostream>

namespace hpx { namespace sycl { namespace experimental { namespace detail {

#if defined(__INTEL_LLVM_COMPILER) ||                                          \
    (defined(__clang__) && defined(SYCL_IMPLEMENTATION_ONEAPI))
    // TODO(daissgr) Re-check in the future if the underlying issue (likely
    // some race-condition/issue in the Intel GPU runtime) is fixed and we can
    // remove the workaround below:

    /// INITIALIZATION WORKAROUND REQUIRED FOR INTEL GPUs:
    /** Utility function that runs a dummy SYCL kernel on the default device /
     * (presumably a GPU when using SYCL with HPX) and waits on the event. This
     * is to work around initialization issues on Intel GPUs, where we
     * encounter segfaults otherwise. / This is most likely due to the fact
     * that we otherwise never wait on SYCL events and thus run multiple SYCL
     * operations concurrently before the initialization is done (unless we add
     * a initial dummy kernel as we do here). **/
    int enforce_oneapi_device_side_init(void)
    {
        try
        {
            ::sycl::queue q(::sycl::default_selector_v,
                ::sycl::property::queue::in_order{});
            ::sycl::event my_kernel_event = q.submit(
                [&](::sycl::handler& h) {
                    h.parallel_for(128, [=](auto i) {});
                },
                ::sycl::detail::code_location{});
            my_kernel_event.wait();
        }
        catch (::sycl::exception const& e)
        {
            std::cerr << "(NON-FATAL) ERROR: Caught sycl::exception during HPX "
                         "SYCL dummy kernel!\n";
            std::cerr << " {what}: " << e.what() << "\n ";
            std::cerr << "Continuing for now as error only occurred in the "
                         "dummy kernel meant to "
                      << "initialize the device by first touch...\n";
            return 2;
        }
        return 1;
    }

    /// Dummy variable to ensure the enforce_oneapi_device_side_init method is
    //being run
    int const run_enforced_oneapi_device_init =
        enforce_oneapi_device_side_init();

#endif

    // Convenience wrapper to get future from just a queue
    // Note: queue needs to be constructed with the in_order attribute
    hpx::future<void> get_future(::sycl::queue& command_queue)
    {
        HPX_ASSERT(queue.is_in_order());
        return hpx::detail::try_catch_exception_ptr(
            [&]() {
                // The SYCL standard does not include a eventRecord method. Instead
                // we have to submit some dummy function and use the event the
                // launch returns as a workaround
                ::sycl::event event = command_queue.submit(
                    [](::sycl::handler& h) { h.single_task([]() {}); });
                return get_future(event);
            },
            [&](std::exception_ptr&& ep) {
                return hpx::make_exceptional_future<void>(HPX_MOVE(ep));
            });
    }

#if !defined(HPX_COMPUTE_DEVICE_CODE)
    hpx::future<void> get_future(::sycl::event command_event)
    {
        return get_future(hpx::util::internal_allocator<>{}, command_event);
    }
#if !defined(__HIPSYCL__)
    hpx::future<void> get_future_using_host_task(
        ::sycl::event command_event, ::sycl::queue& command_queue)
    {
        return get_future_using_host_task(
            hpx::util::internal_allocator<>{}, command_event, command_queue);
    }
#endif
#endif
}}}}    // namespace hpx::sycl::experimental::detail
