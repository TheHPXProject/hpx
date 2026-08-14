//  Copyright (c) 2022 Gregor Daiss
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// hpxinspect:noascii

#include <hpx/config.hpp>
#include <hpx/future.hpp>
#include <hpx/hpx_init.hpp>

#if defined(HPX_HAVE_SYCL)
#include <hpx/async_sycl/sycl_future.hpp>

#include <algorithm>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "common/sycl_vector_add_test_utils.hpp"

#include <sycl/sycl.hpp>

// Check compiler compatibility:
// Needs to be done AFTER sycl include for HipSYCL
// (intel dpcpp would be fine without the include)
//
// Will raise compile-time errors in case of unexpected behaviour! (Hence part
// of the unit test) Uncomment the pragma message commands for more information
// about the compile passes (2 passes for hipsycl, 3 for dpcpp)
#if defined(SYCL_LANGUAGE_VERSION)
#if !defined(__HIPSYCL__) &&                                                   \
    !(defined(__INTEL_LLVM_COMPILER) ||                                        \
        (defined(__clang__) && defined(SYCL_IMPLEMENTATION_ONEAPI) &&          \
            defined(SYCL_IMPLEMENTATION_ONEAPI)))
#warning "HPX-SYCL integration only tested with Intel oneapi and HipSYCL. \
Utilized compiler appears to be neither of those!"
#endif
#else
#error "Compiler seems to not support SYCL! SYCL_LANGUAGE_VERSION is undefined!"
#endif

// Check for separate compiler host and device passes
#if defined(__SYCL_SINGLE_SOURCE__)
#warning "Sycl single source compiler not tested! Use one with multiple passes"
#endif

// Compile-time tests to see if the device_code macro is set correctly
#if defined(__SYCL_DEVICE_ONLY__)
/* #pragma message("Sycl device pass...") */
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#error "ERROR: SYCL device pass detected but HPX_COMPUTE_DEVICE_CODE not set!"
#endif
#else
/* #pragma message("Sycl host pass...") */
#if defined(HPX_COMPUTE_DEVICE_CODE)
#error "ERROR: SYCL host pass detected but HPX_COMPUTE_DEVICE_CODE is set!"
#endif
#endif

// This vector_size leads to 610MB per buffer (size_t datatypes)
// Entire scenario thus requires about 1830MB device memory when running
// Should work with GPUs with >= 2GB memory
constexpr size_t vector_size = 80000000;

// Useful intel profiling commands:
// advisor --collect=survey --profile-gpu -- ./bin/sycl_vector_add_test
// advisor --collect=survey --collect=tripcounts --stacks --flop
// --profile-gpu -- ./bin/sycl_vector_add_test

// This test will launch an simple vector add kernel on the given queue.
// It will then create a hpx::future from the returned sycl event.
// Includes various sanity / asynchronousy tests
void VectorAdd(sycl::queue& q, std::vector<size_t> const& a_vector,
    std::vector<size_t> const& b_vector, std::vector<size_t>& add_parallel)
{
    sycl::event my_kernel_event;
    sycl::range<1> num_items{a_vector.size()};
    {
        // buffers from host vectors
        sycl::buffer a_buf(a_vector.data(), num_items);
        sycl::buffer b_buf(b_vector.data(), num_items);
        sycl::buffer add_buf(add_parallel.data(), num_items);

        bool continuation_triggered = false;
        // Launch SYCL kernel
        my_kernel_event = q.submit([&](sycl::handler& h) {
            sycl::accessor a(a_buf, h, sycl::read_only);
            sycl::accessor b(b_buf, h, sycl::read_only);
            sycl::accessor add(add_buf, h, sycl::write_only, sycl::no_init);
            h.parallel_for(num_items, [=](auto i) { add[i] = a[i] + b[i]; });
        });
        // Get future from event
        hpx::future<void> my_kernel_future =
            hpx::sycl::experimental::detail::get_future(my_kernel_event);
        // Test 1: Is the future asynchronous?
        if (my_kernel_future.is_ready())
        {
            std::cerr
                << "ERROR: Async kernel launch future is immediately ready "
                << "(thus probably not asynchronous at at all)!" << std::endl;
            std::terminate();
        }
        else
        {
            std::cout << "OKAY: Kernel hpx::future is NOT ready immediately "
                         "after launch!"
                      << std::endl;
        }
        // Test 2: Add continutation
        auto continuation_future =
            my_kernel_future.then([&continuation_triggered](auto&& fut) {
                fut.get();
                std::cout << "OKAY: Continuation working!" << std::endl;
                continuation_triggered = true;
                return;
            });
        // Test 3: Is the kernel done after calling get?
        continuation_future.get();
        auto const event_status_after =
            my_kernel_event
                .get_info<sycl::info::event::command_execution_status>();
        if (event_status_after == sycl::info::event_command_status::complete)
        {
            std::cout << "OKAY: Kernel is done!" << std::endl;
        }
        else
        {
            std::cerr << "ERROR: Kernel still running after continuation.get()!"
                      << std::endl;
            std::terminate();
        }
        // Test 4: Was the continuation triggered by get as well?
        if (!continuation_triggered)
        {
            std::cerr << "ERROR: Continuation was apparently not triggered, "
                         "despite calling get!"
                      << std::endl;
            std::terminate();
        }

        // NOTE about usage: according to the sycl specification (2020) section
        // 3.9.8, the entire thing will synchronize here, due to the buffers
        // being destroyed!
        //
        // Hence this implicitly syncs everything, so we should use get on any
        // futures/continuations beforehand (or simply make sure that the sycl
        // buffers (a_buf, b_buf_ add_buf)
        // have a longer lifetime by moving them to another scope.
    }
}

int hpx_main(int, char*[])
{
    // Enable polling for the future
    hpx::sycl::experimental::detail::register_polling(
        hpx::resource::get_thread_pool(0));
    std::cout << "SYCL Future polling enabled!\n";

    // Sanity Test 0: Kind of superfluous check, but without that macro defined
    // event polling won't work. Might as well make sure...
#if defined(HPX_HAVE_MODULE_ASYNC_SYCL)
    std::cerr << "OKAY: HPX_HAVE_MODULE_ASYNC_SYCL is defined!" << std::endl;
#else
    std::cerr << "Error: HPX_HAVE_MODULE_ASYNC_SYCL is not defined!"
              << std::endl;
    std::terminate();
#endif

    // Input vectors
    std::vector<size_t> a(vector_size), b(vector_size),
        add_parallel(vector_size);
    fill_vector_add_input(a, b, add_parallel);

    // Create queue and run on device
    try
    {
        sycl::queue q(
            sycl::default_selector_v, sycl::property::queue::in_order{});
        std::cout << "Running on device: "
                  << q.get_device().get_info<sycl::info::device::name>()
                  << "\n";
        VectorAdd(q, a, b, add_parallel);
    }
    catch (sycl::exception const& e)
    {
        std::cout << "An exception is caught for vector add.\n" << e.what();
        std::terminate();
    }

    // Test 5: Check actual kernel results in add_parallel
    check_vector_add_results(a, b, add_parallel);

    // Print to first 3 and the last 3 results for sanity checking
    static_assert(vector_size >= 6, "vector_size unreasonably small");
    print_vector_results(a, b, add_parallel);

    // Cleanup
    std::cout << "Disabling SYCL future polling.\n";
    hpx::sycl::experimental::detail::unregister_polling(
        hpx::resource::get_thread_pool(0));
    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    return hpx::init(argc, argv);
}
#else
#include <iostream>

// Handle none-sycl builds
int main()
{
    std::cerr << "SYCL Support was not given at compile time! " << std::endl;
    std::cerr << "Please check your build configuration!" << std::endl;
    std::cerr << "Exiting..." << std::endl;
    return 1;    // Fail test, as it was meant to test SYCL...
}
#endif
