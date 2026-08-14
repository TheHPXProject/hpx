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
#include <hpx/async_sycl/sycl_executor.hpp>
#include <hpx/async_sycl/sycl_future.hpp>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

#include "common/sycl_vector_add_test_utils.hpp"

#include <sycl/sycl.hpp>

constexpr size_t vectorsize = 200000000;
constexpr size_t num_bytes = vectorsize * sizeof(float);

int hpx_main(int, char*[])
{
    // Enable polling for the future
    hpx::sycl::experimental::detail::register_polling(
        hpx::resource::get_thread_pool(0));
    std::cout << "SYCL Future polling enabled!\n";
    std::cout << "SYCL language version: " << SYCL_LANGUAGE_VERSION << "\n";

    hpx::sycl::experimental::sycl_executor exec(sycl::default_selector_v);
    auto a = sycl::malloc_device<float>(
        vectorsize, exec.get_device(), exec.get_context());
    auto b = sycl::malloc_device<float>(
        vectorsize, exec.get_device(), exec.get_context());
    auto c = sycl::malloc_device<float>(
        vectorsize, exec.get_device(), exec.get_context());
    auto a_host = sycl::malloc_host<float>(vectorsize, exec.get_context());
    auto b_host = sycl::malloc_host<float>(vectorsize, exec.get_context());
    auto c_host = sycl::malloc_host<float>(vectorsize, exec.get_context());

    // Note: Parameter types needs to match exactly, otherwise the correct
    // function won't be found -- in this case we need to cast from float* to void*
    // otherwise the correct memset overload won't be found
    hpx::apply(exec, &sycl::queue::memset, static_cast<void*>(c), 0,
        static_cast<size_t>(num_bytes));
    hpx::apply(exec, &sycl::queue::memset, static_cast<void*>(a_host), 0,
        static_cast<size_t>(num_bytes));
    hpx::apply(exec, &sycl::queue::memset, static_cast<void*>(b_host), 0,
        static_cast<size_t>(num_bytes));
    hpx::apply(exec, &sycl::queue::memset, static_cast<void*>(c_host), 0,
        static_cast<size_t>(num_bytes));

    float aj = 1.0;
    float bj = 2.0;
    float cj = 0.0;
    float const scalar = 3.0;
    auto const reset_input_method = [=](sycl::id<1> i) {
        a[i] = aj;
        b[i] = bj;
    };
    hpx::apply(exec, &sycl::queue::parallel_for, sycl::range<1>{vectorsize},
        reset_input_method);

    // Note: shortcut function like sycl::queue::parallel_for (which bypass
    // the usual queue.submit pattern) require a reference to a kernel function
    // (hence we cannot pass a temporary. Instead we define our kernel lambdas
    // here...
    auto const copy_step = [=](sycl::id<1> i) { c[i] = a[i]; };
    auto const scale_step = [=](sycl::id<1> i) { b[i] = scalar * c[i]; };
    auto const add_step = [=](sycl::id<1> i) { c[i] = a[i] + b[i]; };
    auto const triad_step = [=](sycl::id<1> i) { a[i] = b[i] + scalar * c[i]; };

    // ... and call them here
    hpx::apply(exec, &sycl::queue::parallel_for, sycl::range<1>{vectorsize},
        copy_step);
    hpx::apply(exec, &sycl::queue::parallel_for, sycl::range<1>{vectorsize},
        scale_step);
    hpx::apply(
        exec, &sycl::queue::parallel_for, sycl::range<1>{vectorsize}, add_step);
    hpx::apply(exec, &sycl::queue::parallel_for, sycl::range<1>{vectorsize},
        triad_step);

    // Note: Parameter types needs to match exactly, otherwise the correct
    // function won't be found -- in this case we need to cast from float* to const float*
    // otherwise the correct copy overload won't be found
    hpx::apply(exec, &sycl::queue::copy, static_cast<float const*>(c),
        static_cast<float*>(c_host), static_cast<size_t>(vectorsize));
    hpx::apply(exec, &sycl::queue::copy, static_cast<float const*>(b),
        static_cast<float*>(b_host), static_cast<size_t>(vectorsize));
    auto fut =
        hpx::async(exec, &sycl::queue::copy, static_cast<float const*>(a),
            static_cast<float*>(a_host), static_cast<size_t>(vectorsize));

    fut.get();

    cj = aj;
    bj = scalar * cj;
    cj = aj + bj;
    aj = bj + scalar * cj;

    float aSumErr = 0.0;
    float bSumErr = 0.0;
    float cSumErr = 0.0;
    for (std::size_t j = 0; j < vectorsize; j++)
    {
        aSumErr += std::abs(a_host[j] - aj);
        bSumErr += std::abs(b_host[j] - bj);
        cSumErr += std::abs(c_host[j] - cj);
    }
    float const aAvgErr = aSumErr / static_cast<float>(vectorsize);
    float const bAvgErr = bSumErr / static_cast<float>(vectorsize);
    float const cAvgErr = cSumErr / static_cast<float>(vectorsize);
    float epsilon = 1.e-6;
    if (std::abs(aAvgErr / aj) > epsilon)
    {
        std::cerr << "Validation error! Wrong results in array a!" << std::endl;
        std::terminate();
    }
    if (std::abs(bAvgErr / bj) > epsilon)
    {
        std::cerr << "Validation error! Wrong results in array b!" << std::endl;
        std::terminate();
    }
    if (std::abs(cAvgErr / cj) > epsilon)
    {
        std::cerr << "Validation error! Wrong results in array c!" << std::endl;
        std::terminate();
    }
    std::cerr << "Validation passed!" << std::endl;

    // Test running single_tasks with executor (to test single_task overloads are working)
    auto const single_task_test1 = [=]() { a[42] = 137.0f; };
    auto const single_task_test2 = [=]() { a[137] = 42.0f; };
    hpx::apply(exec, &sycl::queue::single_task, single_task_test1);
    hpx::apply(exec, &sycl::queue::single_task, single_task_test2);

    auto fut_single_task_test =
        hpx::async(exec, &sycl::queue::copy, static_cast<float const*>(a),
            static_cast<float*>(a_host), static_cast<size_t>(vectorsize));
    fut_single_task_test.get();

    if (std::abs(a_host[42] - 137.0f) > epsilon ||
        std::abs(a_host[137] - 42.0f) > epsilon)
    {
        std::cerr << "Validation error! Wrong results in array a after "
                     "single_task test!"
                  << std::endl;
        std::terminate();
    }
    std::cerr << "Single_task validation passed!" << std::endl;

    // Cleanup
    std::cout << "\nAll done! Disabling SYCL future polling now...\n";
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
    return 1;    // Fail test, as it was meant to test SYCL
}
#endif
