//  Copyright (c) 2017 Thomas Heller
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

// NVCC fails unceremoniously with this test
#if !defined(HPX_CUDA_VERSION)

#include <hpx/chrono.hpp>
#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/async_cuda.hpp>

#include <cstddef>
#include <iostream>

__global__ void dummy() {}

int hpx_main(hpx::program_options::variables_map& vm)
{
    std::size_t const iterations = vm["iterations"].as<std::size_t>();
    std::size_t const batch_size = 10;
    std::size_t const batch_iterations = iterations / batch_size;
    std::size_t const non_batch_iterations = iterations % batch_size;

    cudaStream_t cuda_stream;
    hpx::cuda::experimental::check_cuda_error(cudaStreamCreate(&cuda_stream));

    // Warmup
    {
        hpx::chrono::high_resolution_timer timer;
        for (std::size_t i = 0; i != iterations; ++i)
        {
            dummy<<<1, 1, 0, cuda_stream>>>();
            hpx::cuda::experimental::check_cuda_error(
                cudaStreamSynchronize(cuda_stream));
        }
        double elapsed = timer.elapsed();
        std::cout
            << "native + synchronize (warmup):                                 "
            << elapsed << '\n';
    }

    {
        hpx::chrono::high_resolution_timer timer;
        for (std::size_t i = 0; i != iterations; ++i)
        {
            dummy<<<1, 1, 0, cuda_stream>>>();
            hpx::cuda::experimental::check_cuda_error(
                cudaStreamSynchronize(cuda_stream));
        }
        double elapsed = timer.elapsed();
        std::cout
            << "native + synchronize:                                          "
            << elapsed << '\n';
    }

    {
        hpx::chrono::high_resolution_timer timer;

        for (std::size_t i = 0; i < batch_iterations; ++i)
        {
            for (std::size_t b = 0; b < batch_size; ++b)
            {
                dummy<<<1, 1, 0, cuda_stream>>>();
            }
            hpx::cuda::experimental::check_cuda_error(
                cudaStreamSynchronize(cuda_stream));
        }

        for (std::size_t i = 0; i < non_batch_iterations; ++i)
        {
            dummy<<<1, 1, 0, cuda_stream>>>();
        }
        hpx::cuda::experimental::check_cuda_error(
            cudaStreamSynchronize(cuda_stream));

        double elapsed = timer.elapsed();
        std::cout
            << "native + synchronize batched:                                  "
            << elapsed << '\n';
    }

    {
        hpx::cuda::experimental::enable_user_polling poll("default");

        namespace ex = hpx::execution::experimental;
        namespace cu = hpx::cuda::experimental;
        namespace tt = hpx::this_thread::experimental;

        auto const f = [](cudaStream_t cuda_stream) {
            dummy<<<1, 1, 0, cuda_stream>>>();
        };

        hpx::chrono::high_resolution_timer timer;
        for (std::size_t i = 0; i != iterations; ++i)
        {
            tt::sync_wait(cu::transform_stream(ex::just(), f, cuda_stream));
        }
        double elapsed = timer.elapsed();
        std::cout
            << "transform_stream:                                              "
            << elapsed << '\n';
    }

    {
        hpx::cuda::experimental::enable_user_polling poll("default");

        namespace ex = hpx::execution::experimental;
        namespace cu = hpx::cuda::experimental;
        namespace tt = hpx::this_thread::experimental;

        auto const f = [](cudaStream_t cuda_stream) {
            dummy<<<1, 1, 0, cuda_stream>>>();
        };

        hpx::chrono::high_resolution_timer timer;
        for (std::size_t i = 0; i < batch_iterations; ++i)
        {
            // We have to manually unroll this loop, because the type of the
            // sender changes for each additional transform_stream call. The
            // number of unrolled calls must match batch_size above.
            tt::sync_wait(cu::transform_stream(ex::just(), f, cuda_stream) |
                cu::transform_stream(f, cuda_stream) |
                cu::transform_stream(f, cuda_stream) |
                cu::transform_stream(f, cuda_stream) |
                cu::transform_stream(f, cuda_stream) |
                cu::transform_stream(f, cuda_stream) |
                cu::transform_stream(f, cuda_stream) |
                cu::transform_stream(f, cuda_stream) |
                cu::transform_stream(f, cuda_stream) |
                cu::transform_stream(f, cuda_stream));
        }
        // Do the remainder one-by-one
        for (std::size_t i = 0; i < non_batch_iterations; ++i)
        {
            tt::sync_wait(cu::transform_stream(ex::just(), f, cuda_stream));
        }
        double elapsed = timer.elapsed();
        std::cout
            << "transform_stream batched:                                      "
            << elapsed << '\n';
    }

    {
        hpx::cuda::experimental::enable_user_polling poll("default");

        namespace ex = hpx::execution::experimental;
        namespace cu = hpx::cuda::experimental;
        namespace tt = hpx::this_thread::experimental;

        auto const f = [](cudaStream_t cuda_stream) {
            dummy<<<1, 1, 0, cuda_stream>>>();
        };

        hpx::chrono::high_resolution_timer timer;
        for (std::size_t i = 0; i < batch_iterations; ++i)
        {
            // We have to manually unroll this loop, because the type of the
            // sender changes for each additional transform_stream call. The
            // number of unrolled calls must match batch_size above. Here we
            // intentionally insert dummy then([]{}) calls between the
            // transform_stream calls to force synchronization between the
            // kernel launches.
            tt::sync_wait(cu::transform_stream(ex::just(), f, cuda_stream) |
                ex::then([] {}) | cu::transform_stream(f, cuda_stream) |
                ex::then([] {}) | cu::transform_stream(f, cuda_stream) |
                ex::then([] {}) | cu::transform_stream(f, cuda_stream) |
                ex::then([] {}) | cu::transform_stream(f, cuda_stream) |
                ex::then([] {}) | cu::transform_stream(f, cuda_stream) |
                ex::then([] {}) | cu::transform_stream(f, cuda_stream) |
                ex::then([] {}) | cu::transform_stream(f, cuda_stream) |
                ex::then([] {}) | cu::transform_stream(f, cuda_stream) |
                ex::then([] {}) | cu::transform_stream(f, cuda_stream));
        }
        // Do the remainder one-by-one
        for (std::size_t i = 0; i < non_batch_iterations; ++i)
        {
            tt::sync_wait(cu::transform_stream(ex::just(), f, cuda_stream));
        }
        double elapsed = timer.elapsed();
        std::cout
            << "transform_stream force synchronize batched:                    "
            << elapsed << '\n';
    }

    {
        hpx::cuda::experimental::enable_user_polling poll("default");

        namespace ex = hpx::execution::experimental;
        namespace cu = hpx::cuda::experimental;
        namespace tt = hpx::this_thread::experimental;

        auto const f = [](cudaStream_t cuda_stream) {
            dummy<<<1, 1, 0, cuda_stream>>>();
        };

        hpx::chrono::high_resolution_timer timer;
        for (std::size_t i = 0; i != iterations; ++i)
        {
            tt::sync_wait(cu::transform_stream(ex::just(), f, cuda_stream) |
                ex::continues_on(ex::thread_pool_scheduler{}));
        }
        double elapsed = timer.elapsed();
        std::cout
            << "transform_stream with continues_on:                            "
            << elapsed << '\n';
    }

    {
        hpx::cuda::experimental::enable_user_polling poll("default");

        namespace ex = hpx::execution::experimental;
        namespace cu = hpx::cuda::experimental;
        namespace tt = hpx::this_thread::experimental;

        auto const f = [](cudaStream_t cuda_stream) {
            dummy<<<1, 1, 0, cuda_stream>>>();
        };

        hpx::chrono::high_resolution_timer timer;
        for (std::size_t i = 0; i < batch_iterations; ++i)
        {
            // We have to manually unroll this loop, because the type of the
            // sender changes for each additional transform_stream call. The
            // number of unrolled calls must match batch_size above.
            tt::sync_wait(cu::transform_stream(ex::just(), f, cuda_stream) |
                cu::transform_stream(f, cuda_stream) |
                cu::transform_stream(f, cuda_stream) |
                cu::transform_stream(f, cuda_stream) |
                cu::transform_stream(f, cuda_stream) |
                cu::transform_stream(f, cuda_stream) |
                cu::transform_stream(f, cuda_stream) |
                cu::transform_stream(f, cuda_stream) |
                cu::transform_stream(f, cuda_stream) |
                cu::transform_stream(f, cuda_stream) |
                ex::continues_on(ex::thread_pool_scheduler{}));
        }
        // Do the remainder one-by-one
        for (std::size_t i = 0; i < non_batch_iterations; ++i)
        {
            tt::sync_wait(cu::transform_stream(ex::just(), f, cuda_stream) |
                ex::continues_on(ex::thread_pool_scheduler{}));
        }
        double elapsed = timer.elapsed();
        std::cout
            << "transform_stream with continues_on batched:                    "
            << elapsed << '\n';
    }

    hpx::cuda::experimental::check_cuda_error(cudaStreamDestroy(cuda_stream));

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;

    options_description cmdline("usage: " HPX_APPLICATION_STRING " [options]");
    cmdline.add_options()("iterations",
        hpx::program_options::value<std::size_t>()->default_value(1024),
        "number of iterations (default: 1024)");
    hpx::local::init_params init_args;
    init_args.desc_cmdline = cmdline;

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
#else
int main(int, char*[])
{
    return 0;
}
#endif
