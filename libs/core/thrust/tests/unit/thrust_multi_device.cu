//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Serialization of thrust device_vectors with several GPUs, the way a
// parcelport does it (see thrust_serialization.hpp): the data is sent from
// the GPU it is on, whatever GPU is current, and received on the GPU of the
// receiving vector's memory if it has memory, else on the GPU the receiving
// side asks for (the GPU of the destination's target), else on the current
// GPU. The current GPU is the same afterwards. With one GPU, all of these
// are the same GPU.

#include <hpx/config.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/async_cuda.hpp>
#include <hpx/modules/compute_local.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/thrust.hpp>

#include <thrust/device_vector.h>
#include <thrust/host_vector.h>

#include "thrust_serialization.hpp"

#include <cstddef>
#include <string>

namespace {

    using hpx::cuda::experimental::check_cuda_error;
    using hpx::serialization::detail::device_of;
    using hpx::serialization::detail::scoped_device;

    int device_count()
    {
        int count = 0;
        check_cuda_error(cudaGetDeviceCount(&count));
        return count;
    }

    int current_device()
    {
        int device = 0;
        check_cuda_error(cudaGetDevice(&device));
        return device;
    }

    ::thrust::host_vector<int> values(std::size_t const size, int const first)
    {
        ::thrust::host_vector<int> result(size);
        for (std::size_t i = 0; i != size; ++i)
        {
            result[i] = first + static_cast<int>(i);
        }
        return result;
    }

    // a vector of 'size' elements whose memory is on 'device'
    template <typename Vector>
    Vector make_on(int const device, ::thrust::host_vector<int> const& data)
    {
        scoped_device const on_device(device);
        return Vector(data);
    }

    bool is_pinned(void const* p)
    {
        return p != nullptr && hpx::thrust::detail::is_pinned_host_memory(p);
    }

    struct options
    {
        bool preprocess;
        bool chunks;
        bool device_chunks;
        bool zero_copy_receive;
        bool device_receive;    // only with zero_copy_receive
    };

    // Sends a vector made on GPU 'from' and receives it with GPU 'current'
    // current, the receiving side asking for GPU 'requested' (-1: none),
    // into a vector with memory on GPU 'existing' (-1: an empty vector).
    template <typename Vector>
    void test_transfer(std::size_t const size, int const from,
        int const current, int const requested, int const existing,
        options const o)
    {
        int const first =
            1000 * from + 100 * current + 10 * (requested + 1) + (existing + 1);
        ::thrust::host_vector<int> const expected = values(size, first);
        Vector const v = make_on<Vector>(from, expected);

        check_cuda_error(cudaSetDevice(current));
        test::message m = test::send(
            test::archive_flags(o.chunks, o.device_chunks), o.preprocess,
            [&v](hpx::serialization::output_archive& ar) { ar << v; });
        HPX_TEST_EQ(current_device(), current);

        // memory on GPU 'existing' that has to grow
        Vector w;
        if (existing >= 0)
        {
            w = make_on<Vector>(existing, values(1, -1));
        }

        check_cuda_error(cudaSetDevice(current));
        test::receive(
            m, o.zero_copy_receive,
            [&w](hpx::serialization::input_archive& ar) { ar >> w; },
            o.device_receive, requested);
        HPX_TEST_EQ(current_device(), current);

        HPX_TEST_EQ(w.size(), size);
        HPX_TEST(::thrust::host_vector<int>(w) == expected);

        // pinned host memory (hpx.thrust.memory=pinned) belongs to no GPU
        void const* data = ::thrust::raw_pointer_cast(w.data());
        if (size != 0 && !is_pinned(data))
        {
            int const want = existing >= 0 ?
                existing :
                (requested >= 0 ? requested : current);
            HPX_TEST_EQ(device_of(data), want);
        }
    }

    template <typename Vector>
    void test_transfers(int const count)
    {
        for (std::size_t size : {16, 1 << 16})
        {
            for (bool preprocess : {false, true})
            {
                for (options o :
                    {options{preprocess, false, false, false, false},
                        options{preprocess, true, false, false, false},
                        options{preprocess, true, false, true, false},
                        options{preprocess, true, true, true, true}})
                {
                    for (int from = 0; from != count; ++from)
                    {
                        for (int to = 0; to != count; ++to)
                        {
                            // with several GPUs, the current GPU and the
                            // memory of the receiving vector are other GPUs
                            int const current = (to + 1) % count;
                            int const existing = (to + 2) % count;

                            test_transfer<Vector>(
                                size, from, current, -1, -1, o);
                            test_transfer<Vector>(
                                size, from, current, to, -1, o);
                            test_transfer<Vector>(
                                size, from, current, to, existing, o);
                        }
                    }
                }
            }
        }
    }

    // the GPU a target stands for
    void test_target_device(int const count)
    {
        for (int k = 0; k != count; ++k)
        {
            HPX_TEST_EQ(
                hpx::traits::target_device<hpx::cuda::experimental::target>::
                    call(hpx::cuda::experimental::target(k)),
                k);
        }
        HPX_TEST_EQ(
            hpx::traits::target_device<hpx::compute::host::target>::call(
                hpx::compute::host::target()),
            -1);

        // results of remote calls are received on the GPU of the default
        // target
        HPX_TEST_EQ(hpx::detail::result_device(),
            hpx::cuda::experimental::get_default_target()
                .native_handle()
                .get_device());
    }
}    // namespace

int hpx_main(hpx::program_options::variables_map&)
{
    int const count = device_count();
    HPX_TEST_LT(0, count);

    // the current GPU belongs to the OS thread: the test runs on one
    int const initial = current_device();

    test_target_device(count);

    test_transfers<::thrust::device_vector<int>>(count);
    test_transfers<hpx::thrust::device_vector<int>>(count);

    check_cuda_error(cudaSetDevice(initial));
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
