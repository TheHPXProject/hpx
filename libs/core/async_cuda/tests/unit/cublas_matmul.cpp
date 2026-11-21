//  Copyright (c) 2017-2018 John Biddiscombe
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For compliance with the NVIDIA EULA:
// "This software contains source code provided by NVIDIA Corporation."

// This is a conversion of the NVIDIA cublas example matrixMulCUBLAS to use
// HPX style data structures, executors and futures and demonstrate a simple use
// of computing a number of iteration of a matrix multiply on a stream and returning
// a future when it completes. This can be used to chain/schedule other task
// in a manner consistent with the future based API of HPX.
//
// Example usage: bin/cublas_matmul --sizemult=10 --iterations=25 --hpx:threads=8
// NB. The hpx::threads param only controls how many parallel tasks to use for the CPU
// comparison/checks and makes no difference to the GPU execution.
//
// Note: The hpx::cuda::experimental::allocator makes use of device code and if used
// this example must be compiled with nvcc instead of c++ which requires the following
// cmake setting
// set_source_files_properties(cublas_matmul.cpp
//     PROPERTIES CUDA_SOURCE_PROPERTY_FORMAT OBJ)
// Currently, nvcc does not handle lambda functions properly and it is simpler to use
// cudaMalloc/cudaMemcpy etc, so we do not #define HPX_CUBLAS_DEMO_WITH_ALLOCATOR

#include <hpx/algorithm.hpp>
#include <hpx/assert.hpp>
#include <hpx/chrono.hpp>
#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/async_cuda.hpp>
#include <hpx/modules/testing.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <random>
#include <sstream>
#include <utility>
#include <vector>

// CUDA 13.5 overloads cudaMemcpyAsync (a function template taking
// cudaMemcpyAttributes), so it can't be passed by name to the executors, which
// take a pointer to the CUDA function
constexpr cudaError_t (*memcpy_async)(void*, void const*, std::size_t,
    cudaMemcpyKind, cudaStream_t) = &cudaMemcpyAsync;

//
std::mt19937 gen;

// -------------------------------------------------------------------------
// Optional Command-line multiplier for matrix sizes
struct sMatrixSize
{
    unsigned int uiWA, uiHA, uiWB, uiHB, uiWC, uiHC;
};

// -------------------------------------------------------------------------
// Compute reference data set matrix multiply on CPU
// C = A * B
// @param C          reference data, computed but preallocated
// @param A          matrix A as provided to device
// @param B          matrix B as provided to device
// @param hA         height of matrix A
// @param wB         width of matrix B
// -------------------------------------------------------------------------
template <typename T>
void matrixMulCPU(T* C, T const* A, T const* B, unsigned int hA,
    unsigned int wA, unsigned int wB)
{
    hpx::experimental::for_loop(hpx::execution::par, 0, hA, [&](int i) {
        for (unsigned int j = 0; j < wB; ++j)
        {
            T sum = 0;
            for (unsigned int k = 0; k < wA; ++k)
            {
                T a = A[i * wA + k];
                T b = B[k * wB + j];
                sum += a * b;
            }
            C[i * wB + j] = (T) sum;
        }
    });
}

// -------------------------------------------------------------------------
// Compute the L2 norm difference between two arrays
inline bool compare_L2_err(float const* reference, float const* data,
    unsigned int const len, float const epsilon)
{
    HPX_ASSERT(epsilon >= 0);

    float error = 0;
    float ref = 0;

    hpx::experimental::for_loop(hpx::execution::par, 0, len, [&](int i) {
        float diff = reference[i] - data[i];
        error += diff * diff;
        ref += reference[i] * reference[i];
    });

    float normRef = sqrtf(ref);
    if (std::fabs(ref) < 1e-7f)
    {
        return false;
    }

    float normError = sqrtf(error);
    error = normError / normRef;
    bool result = error < epsilon;
    return result;
}

// -------------------------------------------------------------------------
// Run a simple test matrix multiply using CUBLAS
// -------------------------------------------------------------------------
template <typename T>
void matrixMultiply(hpx::cuda::experimental::cublas_executor& cublas,
    sMatrixSize& matrix_size, std::size_t /* device */, std::size_t iterations)
{
    using hpx::execution::par;

    // Allocate host memory for matrices A and B
    unsigned int size_A = matrix_size.uiWA * matrix_size.uiHA;
    unsigned int size_B = matrix_size.uiWB * matrix_size.uiHB;
    unsigned int size_C = matrix_size.uiWC * matrix_size.uiHC;

    std::vector<T> h_A(size_A);
    std::vector<T> h_B(size_B);
    std::vector<T> h_C(size_C);
    std::vector<T> h_CUBLAS(size_C);

    // Fill A and B with random numbers
    auto randfunc = [](T& x) { x = gen() / (T) RAND_MAX; };
    hpx::for_each(par, h_A.begin(), h_A.end(), randfunc);
    hpx::for_each(par, h_B.begin(), h_B.end(), randfunc);

    // create a cublas executor we'll use to futurize cuda events
    using namespace hpx::cuda::experimental;
    using cublas_future = typename cuda_executor::future_type;

    T *d_A, *d_B, *d_C;
    hpx::cuda::experimental::check_cuda_error(
        cudaMalloc((void**) &d_A, size_A * sizeof(T)));

    hpx::cuda::experimental::check_cuda_error(
        cudaMalloc((void**) &d_B, size_B * sizeof(T)));

    hpx::cuda::experimental::check_cuda_error(
        cudaMalloc((void**) &d_C, size_C * sizeof(T)));

    // adding async copy operations into the stream before cublas calls puts
    // the copies in the queue before the matrix operations.
    hpx::post(cublas, memcpy_async, d_A, h_A.data(), size_A * sizeof(T),
        cudaMemcpyHostToDevice);

    auto copy_future = hpx::async(cublas, memcpy_async, d_B, h_B.data(),
        size_B * sizeof(T), cudaMemcpyHostToDevice);

    // we can call get_future multiple times on the cublas helper.
    // Each one returns a new future that will be set ready when the stream event
    // for this point is triggered
    copy_future.then([](cublas_future&&) {
        std::cout << "The async host->device copy operation completed"
                  << std::endl;
    });

    std::cout << "Computing result using CUBLAS...\n";
    T const alpha = 1.0f;
    T const beta = 0.0f;

    // Perform warmup operation with cublas
    // note cublas is column major ordering : transpose the order
    hpx::chrono::high_resolution_timer t1;
    //
    std::cout << "calling CUBLAS...\n";
    auto fut = hpx::async(cublas, cublasSgemm, CUBLAS_OP_N, CUBLAS_OP_N,
        matrix_size.uiWB, matrix_size.uiHA, matrix_size.uiWA, &alpha, d_B,
        matrix_size.uiWB, d_A, matrix_size.uiWA, &beta, d_C, matrix_size.uiWA);

    // wait until the operation completes
    fut.get();

    double us1 = t1.elapsed_microseconds();
    std::cout << "warmup: elapsed_microseconds " << us1 << std::endl;

    // once the future has been retrieved, the next call to
    // get_future will create a new event attached to a new future
    // so we can reuse the same cublas executor stream if we want

    hpx::chrono::high_resolution_timer t2;
    for (std::size_t j = 0; j < iterations; j++)
    {
        hpx::post(cublas, cublasSgemm, CUBLAS_OP_N, CUBLAS_OP_N,
            matrix_size.uiWB, matrix_size.uiHA, matrix_size.uiWA, &alpha, d_B,
            matrix_size.uiWB, d_A, matrix_size.uiWA, &beta, d_C,
            matrix_size.uiWA);
    }
    // get a future for when the stream reaches this point (matrix operations complete)
    auto matrix_finished = cublas.get_future();

    // when the matrix operations complete, copy the result to the host
    auto copy_finished = hpx::async(cublas, memcpy_async, h_CUBLAS.data(), d_C,
        size_C * sizeof(T), cudaMemcpyDeviceToHost);

    // attach a continuation to the cublas future
    auto new_future = matrix_finished.then([&](cublas_future&&) {
        double us2 = t2.elapsed_microseconds();
        std::cout << "actual: elapsed_microseconds " << us2 << " iterations "
                  << iterations << std::endl;

        // Compute and print the performance
        double usecPerMatrixMul = us2 / iterations;
        double flopsPerMatrixMul = 2.0 * (double) matrix_size.uiWA *
            (double) matrix_size.uiHA * (double) matrix_size.uiWB;
        double gigaFlops =
            (flopsPerMatrixMul * 1.0e-9) / (usecPerMatrixMul / 1e6);
        printf("Performance = %.2f GFlop/s, Time = %.3f msec/iter, Size = %.0f "
               "Ops\n",
            gigaFlops, 1e-3 * usecPerMatrixMul, flopsPerMatrixMul);
    });

    // wait for the timing to complete, and then do a CPU comparison
    auto finished = new_future.then([&](cublas_future&&) {
        // compute reference solution on the CPU
        std::cout << "\nComputing result using host CPU...\n";
        // just wait for the device->host copy to complete if it hasn't already
        copy_finished.get();

        // compute reference solution on the CPU
        // allocate storage for the CPU result
        std::vector<T> reference(size_C);

        hpx::chrono::high_resolution_timer t3;
        matrixMulCPU<T>(reference.data(), h_A.data(), h_B.data(),
            matrix_size.uiHA, matrix_size.uiWA, matrix_size.uiWB);
        double us3 = t3.elapsed_microseconds();
        std::cout << "CPU elapsed_microseconds (1 iteration) " << us3
                  << std::endl;

        // check result (CUBLAS)
        bool resCUBLAS =
            compare_L2_err(reference.data(), h_CUBLAS.data(), size_C, 1e-6);
        HPX_TEST_MSG(resCUBLAS, "matrix CPU/GPU comparison error");

        // if the result was incorrect, we throw an exception, so here it's ok
        if (resCUBLAS)
        {
            std::cout
                << "\nComparing CUBLAS Matrix Multiply with CPU results: OK \n";
        }
    });

    finished.get();
    ::hpx::cuda::experimental::check_cuda_error(cudaFree(d_A));
    ::hpx::cuda::experimental::check_cuda_error(cudaFree(d_B));
    ::hpx::cuda::experimental::check_cuda_error(cudaFree(d_C));
}

// -------------------------------------------------------------------------
int hpx_main(hpx::program_options::variables_map& vm)
{
    // install cuda future polling handler
    hpx::cuda::experimental::enable_user_polling poll("default");
    //
    std::size_t device = vm["device"].as<std::size_t>();
    std::size_t sizeMult = vm["sizemult"].as<std::size_t>();
    std::size_t iterations = vm["iterations"].as<std::size_t>();
    //
    unsigned int seed = std::random_device{}();
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    gen.seed(seed);
    std::cout << "using seed: " << seed << std::endl;

    //
    sizeMult = (std::min) (sizeMult, std::size_t(100));
    sizeMult = (std::max) (sizeMult, std::size_t(1));
    //
    // use a larger block size for Fermi and above, query default cuda target properties
    hpx::cuda::experimental::target target(device);

    std::cout << "GPU Device " << target.native_handle().get_device() << ": \""
              << target.native_handle().processor_name() << "\" "
              << "with compute capability "
              << target.native_handle().processor_family() << "\n";

    int block_size = (target.native_handle().processor_family() < 2) ? 16 : 32;

    sMatrixSize matrix_size;
    matrix_size.uiWA = 2 * block_size * sizeMult;
    matrix_size.uiHA = 4 * block_size * sizeMult;
    matrix_size.uiWB = 2 * block_size * sizeMult;
    matrix_size.uiHB = 4 * block_size * sizeMult;
    matrix_size.uiWC = 2 * block_size * sizeMult;
    matrix_size.uiHC = 4 * block_size * sizeMult;

    printf("MatrixA(%u,%u), MatrixB(%u,%u), MatrixC(%u,%u)\n\n",
        matrix_size.uiWA, matrix_size.uiHA, matrix_size.uiWB, matrix_size.uiHB,
        matrix_size.uiWC, matrix_size.uiHC);

    // --------------------------------
    // test matrix multiply using cublas executor
    hpx::cuda::experimental::cublas_executor cublas(device,
        CUBLAS_POINTER_MODE_HOST, hpx::cuda::experimental::event_mode{});
    matrixMultiply<float>(cublas, matrix_size, device, iterations);

    // --------------------------------
    // sanity check : test again using a copy of the cublas executor
    std::cout << "\n\n\n------------" << std::endl;
    std::cout << "Checking copy semantics of cublas executor" << std::endl;
    hpx::cuda::experimental::cublas_executor cublas2 = cublas;
    matrixMultiply<float>(cublas2, matrix_size, device, 1);

    // --------------------------------
    // sanity check : test again using a moved copy of the cublas executor
    std::cout << "\n\n\n------------" << std::endl;
    std::cout << "Checking move semantics of cublas executor" << std::endl;
    hpx::cuda::experimental::cublas_executor cublas3(std::move(cublas));
    matrixMultiply<float>(cublas3, matrix_size, device, 1);

    return hpx::local::finalize();
}

// -------------------------------------------------------------------------
int main(int argc, char** argv)
{
    printf("[HPX Matrix Multiply CUBLAS] - Starting...\n");

    using namespace hpx::program_options;
    options_description cmdline("usage: " HPX_APPLICATION_STRING " [options]");
    // clang-format off
    cmdline.add_options()
        ("device",
        hpx::program_options::value<std::size_t>()->default_value(0),
        "Device to use")
        ("sizemult",
        hpx::program_options::value<std::size_t>()->default_value(5),
        "Multiplier")
        ("iterations",
        hpx::program_options::value<std::size_t>()->default_value(30),
        "iterations")
        ("no-cpu",
        hpx::program_options::value<bool>()->default_value(false),
        "disable CPU validation to save time")
        ("seed,s",
        hpx::program_options::value<unsigned int>(),
        "the random number generator seed to use for this run");
    // clang-format on
    hpx::local::init_params init_args;
    init_args.desc_cmdline = cmdline;

    auto result = hpx::local::init(hpx_main, argc, argv, init_args);
    return result || hpx::util::report_errors();
}
