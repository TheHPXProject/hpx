//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The NCCL conduit reports a round in flight for longer than
// hpx.parcel.conduit.nccl.stall_timeout: the sender holds its rounds back
// (hpx.parcel.conduit.nccl.test_hold_rounds) for longer than that, so the
// receiver's round waits; with stall_action = report the transfer finishes
// after the hold, and the receiver has reported the round.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <hpx/async_cuda/custom_gpu_api.hpp>
#include <hpx/device_conduit_nccl/nccl_conduit.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace detail = hpx::serialization::detail;

// device memory on GPU 0 of the locality, sent as a chunk of device memory
// (the localities of the test run on different nodes, or the node has a GPU
// for each)
int device()
{
    int count = 0;
    HPX_TEST_EQ(cudaGetDeviceCount(&count), cudaSuccess);
    return count == 0 ? 0 : static_cast<int>(hpx::get_locality_id()) % count;
}

struct device_buffer
{
    device_buffer() = default;

    // filled with 7 (before any transfer: waiting for the GPU while one is
    // in flight may deadlock, see the module's documentation)
    explicit device_buffer(std::size_t const size, bool const fill = false)
      : size(size)
    {
        HPX_TEST_EQ(cudaSetDevice(device()), cudaSuccess);
        void* p = nullptr;
        HPX_TEST_EQ(cudaMalloc(&p, size), cudaSuccess);
        if (fill)
        {
            std::vector<char> const host(size, 7);
            HPX_TEST_EQ(
                cudaMemcpy(p, host.data(), size, cudaMemcpyHostToDevice),
                cudaSuccess);
            HPX_TEST_EQ(cudaDeviceSynchronize(), cudaSuccess);
        }
        // never freed: freeing waits for the GPU (see the module's
        // documentation)
        data = std::shared_ptr<char>(static_cast<char*>(p), [](char*) {});
    }

    template <typename Archive>
    void save(Archive& ar, unsigned) const
    {
        ar << size;
        ar.save_binary_chunk(detail::fundamental_types::none, data.get(), size,
            hpx::serialization::chunk_memory::device);
    }

    template <typename Archive>
    void load(Archive& ar, unsigned)
    {
        std::size_t n = 0;
        ar >> n;
        *this = device_buffer(n);
        HPX_TEST(ar.next_chunk_from_transport());
        ar.load_binary_chunk(detail::fundamental_types::none, data.get(), n,
            true, hpx::serialization::chunk_memory::device);
    }

    HPX_SERIALIZATION_SPLIT_MEMBER()

    std::size_t size = 0;
    std::shared_ptr<char> data;
};

template <>
struct hpx::traits::has_device_data<device_buffer> : std::true_type
{
};

// receives the data, returns how many rounds this locality reported
std::uint64_t receive(device_buffer const& d)
{
    std::vector<char> host(d.size);
    HPX_TEST_EQ(
        cudaMemcpy(host.data(), d.data.get(), d.size, cudaMemcpyDeviceToHost),
        cudaSuccess);
    for (char const c : host)
    {
        HPX_TEST_EQ(c, char(7));
    }
    auto const* conduit =
        dynamic_cast<hpx::parcelset::device_conduits::nccl_conduit const*>(
            hpx::parcelset::get_device_conduit());
    HPX_TEST(conduit != nullptr);
    return conduit != nullptr ? conduit->stalled_rounds() : 0;
}
HPX_PLAIN_ACTION(receive)

int hpx_main()
{
    std::vector<hpx::id_type> const remotes = hpx::find_remote_localities();
    HPX_TEST(!remotes.empty());
    for (hpx::id_type const& id : remotes)
    {
        // held back 2 s by the sender, reported after 0.5 s
        std::uint64_t const reported =
            receive_action()(id, device_buffer(std::size_t(1) << 20, true));
        HPX_TEST_LTE(std::uint64_t(1), reported);
    }
    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    hpx::init_params params;
    params.cfg = {"hpx.parcel.device_conduit=nccl",
        "hpx.parcel.conduit.nccl.stall_timeout=0.5",
        "hpx.parcel.conduit.nccl.stall_action=report",
        "hpx.parcel.conduit.nccl.test_hold_rounds=2000"};
    HPX_TEST_EQ(hpx::init(argc, argv, params), 0);
    return hpx::util::report_errors();
}
#endif
