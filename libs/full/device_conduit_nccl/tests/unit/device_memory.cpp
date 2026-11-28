//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The NCCL device conduit moves the zero-copy chunks of device memory of
// parcels: single chunks of several sizes, parcels with two device chunks,
// empty chunks, many parcels in flight at once in both directions (the
// receivers form rounds of several chunks, in an order the senders don't
// know), and, where each locality has two GPUs, data from each GPU of a
// locality to each GPU of the others (a component per GPU names it as its
// target, the data is received there). The data is GPU memory (NCCL moves
// nothing else); the test checks it on the host. NCCL needs a GPU of its
// own for each locality: localities on the same node use different GPUs, the
// test is skipped where a node has fewer GPUs than localities.
//
// The test allocates and frees its device memory stream ordered: cudaFree
// waits for all work on the GPU, including NCCL kernels waiting for the other
// locality, which may itself wait in cudaFree (a deadlock). With HIP, it
// allocates with hipMalloc and reuses the memory instead of freeing it: HIP
// runs the streams of a GPU on a few in-order hardware queues, so waiting for
// a stream (also the one of stream-ordered allocations) may wait for an RCCL
// kernel queued before it on the same hardware queue, which waits for the
// other locality.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/async_cuda.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <hpx/async_cuda/custom_gpu_api.hpp>

#include <algorithm>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#if !defined(HPX_WINDOWS)
#include <unistd.h>
#endif

namespace detail = hpx::serialization::detail;

///////////////////////////////////////////////////////////////////////////////
// the GPU of this locality (set by use_own_gpu: one of the GPUs it sees that
// no other locality on its node uses)
int& own_gpu()
{
    static int gpu = 0;
    return gpu;
}

int device()
{
    return own_gpu();
}

// the GPU of the device memory at 'p', -1 if it isn't device memory
int gpu_of(void const* p)
{
    cudaPointerAttributes attributes{};
#if defined(HPX_HAVE_HIP) && HIP_VERSION_MAJOR < 6
    // HIP before ROCm 6 names the field memoryType
    auto const& type = attributes.memoryType;
#else
    auto const& type = attributes.type;
#endif
    if (cudaPointerGetAttributes(&attributes, p) != cudaSuccess ||
        type != cudaMemoryTypeDevice)
    {
        (void) cudaGetLastError();
        return -1;
    }
    return attributes.device;
}

#if defined(HPX_HAVE_HIP)
// the device memory released by the test, by GPU and size, for reuse (never
// freed, see above)
std::map<std::pair<int, std::size_t>, std::vector<void*>>& released_memory()
{
    static std::map<std::pair<int, std::size_t>, std::vector<void*>> memory;
    return memory;
}

std::mutex& released_memory_mutex()
{
    static std::mutex mtx;
    return mtx;
}
#else
// the stream of the allocations on 'gpu' (the GPU is current)
cudaStream_t allocation_stream(int const gpu)
{
    static std::mutex mtx;
    static std::map<int, cudaStream_t> streams;

    std::lock_guard<std::mutex> l(mtx);
    cudaStream_t& s = streams[gpu];
    if (s == nullptr)
    {
        HPX_TEST_EQ(
            cudaStreamCreateWithFlags(&s, cudaStreamNonBlocking), cudaSuccess);
    }
    return s;
}
#endif

#if !defined(HPX_HAVE_HIP)
// a non-blocking stream for copies to 'gpu' (the GPU is current)
cudaStream_t copy_stream(int const gpu)
{
    static std::mutex mtx;
    static std::map<int, cudaStream_t> streams;

    std::lock_guard<std::mutex> l(mtx);
    cudaStream_t& s = streams[gpu];
    if (s == nullptr)
    {
        HPX_TEST_EQ(
            cudaStreamCreateWithFlags(&s, cudaStreamNonBlocking), cudaSuccess);
    }
    return s;
}
#endif

// the node of this locality and the PCI bus ids of the GPUs it sees (in the
// order of their numbers here): localities on a node may see the same GPUs or
// different ones (CUDA_VISIBLE_DEVICES set per locality)
std::pair<std::string, std::vector<std::string>> node_info()
{
    std::string host = "unknown";
#if !defined(HPX_WINDOWS)
    char name[256] = {};
    if (gethostname(name, sizeof(name) - 1) == 0)
    {
        host = name;
    }
#endif
    int count = 0;
    if (cudaGetDeviceCount(&count) != cudaSuccess)
    {
        (void) cudaGetLastError();
        count = 0;
    }
    std::vector<std::string> gpus;
    for (int gpu = 0; gpu != count; ++gpu)
    {
        char bus_id[64] = {};
#if defined(HPX_HAVE_HIP)
        // not among the names custom_gpu_api.hpp maps
        HPX_TEST_EQ(
            hipDeviceGetPCIBusId(bus_id, sizeof(bus_id), gpu), hipSuccess);
#else
        HPX_TEST_EQ(
            cudaDeviceGetPCIBusId(bus_id, sizeof(bus_id), gpu), cudaSuccess);
#endif
        gpus.emplace_back(bus_id);
    }
    return {host, gpus};
}
HPX_PLAIN_ACTION(node_info)

// this locality's GPU, for its buffers and for the results of remote calls
// (received on the GPU of the default target; localities on one node must
// not share one)
void use_own_gpu(int const gpu)
{
    own_gpu() = gpu;
    hpx::cuda::experimental::get_default_target() =
        hpx::cuda::experimental::target(gpu);
}
HPX_PLAIN_ACTION(use_own_gpu)

///////////////////////////////////////////////////////////////////////////////
// device memory, sent as a zero-copy chunk of device memory
struct device_buffer
{
    device_buffer() = default;

    // on 'gpu', the locality's GPU by default
    explicit device_buffer(std::size_t size, int const gpu = device())
      : size(size)
      , gpu(gpu)
    {
        if (size != 0)
        {
            // from any worker thread
            HPX_TEST_EQ(cudaSetDevice(gpu), cudaSuccess);
            void* p = nullptr;
#if defined(HPX_HAVE_HIP)
            {
                std::lock_guard<std::mutex> l(released_memory_mutex());
                auto& released = released_memory()[{gpu, size}];
                if (!released.empty())
                {
                    p = released.back();
                    released.pop_back();
                }
            }
            if (p == nullptr)
            {
                HPX_TEST_EQ(cudaMalloc(&p, size), cudaSuccess);
            }
            data = std::shared_ptr<char>(
                static_cast<char*>(p), [gpu, size](char* q) {
                    std::lock_guard<std::mutex> l(released_memory_mutex());
                    released_memory()[{gpu, size}].push_back(q);
                });
#else
            cudaStream_t const stream = allocation_stream(gpu);
            HPX_TEST_EQ(cudaMallocAsync(&p, size, stream), cudaSuccess);
            HPX_TEST_EQ(cudaStreamSynchronize(stream), cudaSuccess);
            data = std::shared_ptr<char>(static_cast<char*>(p),
                [stream](char* q) { (void) cudaFreeAsync(q, stream); });
#endif
        }
    }

    // device memory on 'gpu' holding 'host'
    static device_buffer from_host(
        std::vector<char> const& host, int const gpu = device())
    {
        device_buffer b(host.size(), gpu);
        if (!host.empty())
        {
#if defined(HPX_HAVE_HIP)
            // synchronous: waiting for a stream may wait for an RCCL kernel
            // queued before it (see the module's documentation)
            cudaError_t const e = cudaMemcpy(
                b.data.get(), host.data(), host.size(), cudaMemcpyHostToDevice);
            HPX_TEST_MSG(e == cudaSuccess, cudaGetErrorString(e));
#else
            // on a stream of its own, waited for: from pageable memory
            // cudaMemcpy may return before the data is on the GPU, and the
            // conduit reads it on its own stream (no device-wide wait, which
            // would wait for the conduit's work too)
            cudaStream_t const stream = copy_stream(gpu);
            cudaError_t e = cudaMemcpyAsync(b.data.get(), host.data(),
                host.size(), cudaMemcpyHostToDevice, stream);
            HPX_TEST_MSG(e == cudaSuccess, cudaGetErrorString(e));
            e = cudaStreamSynchronize(stream);
            HPX_TEST_MSG(e == cudaSuccess, cudaGetErrorString(e));
#endif
        }
        return b;
    }

    [[nodiscard]] std::vector<char> to_host() const
    {
        std::vector<char> host(size);
        if (size != 0)
        {
            HPX_TEST_EQ(cudaSetDevice(gpu), cudaSuccess);
            cudaError_t const e = cudaMemcpy(
                host.data(), data.get(), size, cudaMemcpyDeviceToHost);
            HPX_TEST_MSG(e == cudaSuccess, cudaGetErrorString(e));
        }
        return host;
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

        // on the GPU of the parcel's destination, if it names one
        auto const* receive =
            ar.template try_get_extra_data<detail::receive_device>();
        *this = device_buffer(n,
            receive != nullptr && receive->device >= 0 ? receive->device :
                                                         device());
        if (n == 0)
        {
            // the archive has no chunk for empty data
            return;
        }

        // the conduit moves the data (asked first, as device_vector does)
        HPX_TEST(ar.next_chunk_from_transport());
        ar.load_binary_chunk(detail::fundamental_types::none, data.get(), n,
            true, hpx::serialization::chunk_memory::device);
    }

    HPX_SERIALIZATION_SPLIT_MEMBER()

    std::size_t size = 0;
    int gpu = -1;
    std::shared_ptr<char> data;
};

// it is told the GPU of the parcel's destination (receive_device)
template <>
struct hpx::traits::has_device_data<device_buffer> : std::true_type
{
};

std::vector<char> pattern(std::size_t size, char first)
{
    std::vector<char> v(size);
    for (std::size_t i = 0; i != size; ++i)
    {
        v[i] = static_cast<char>(first + static_cast<char>(i % 61));
    }
    return v;
}

///////////////////////////////////////////////////////////////////////////////
// checks its arguments (the data has arrived when the action runs) and
// returns device data
device_buffer exchange(device_buffer const& d, std::size_t size, char first)
{
    HPX_TEST(d.to_host() == pattern(size, first));
    return device_buffer::from_host(
        pattern(size, static_cast<char>(first + 1)));
}
HPX_PLAIN_ACTION(exchange)

// two device chunks in one parcel
std::pair<device_buffer, device_buffer> exchange_two(
    device_buffer const& a, device_buffer const& b, std::size_t size)
{
    HPX_TEST(a.to_host() == pattern(size, 'a'));
    HPX_TEST(b.to_host() == pattern(size / 2, 'b'));
    return {device_buffer::from_host(pattern(size / 3, 'c')),
        device_buffer::from_host(pattern(size, 'd'))};
}
HPX_PLAIN_ACTION(exchange_two)

// many exchanges at once from 'source' with the localities in 'targets'
// (both directions are busy when all localities run it)
void exchange_many(std::vector<hpx::id_type> const& targets, std::size_t count)
{
    std::vector<hpx::future<device_buffer>> results;
    std::vector<std::size_t> sizes;
    for (std::size_t i = 0; i != count; ++i)
    {
        for (hpx::id_type const& id : targets)
        {
            std::size_t const size = (std::size_t(1) << (i % 21)) + i;
            char const first = static_cast<char>('A' + (i % 20));
            sizes.push_back(size);
            results.push_back(hpx::async<exchange_action>(id,
                device_buffer::from_host(pattern(size, first)), size, first));
        }
    }

    std::size_t k = 0;
    for (std::size_t i = 0; i != count; ++i)
    {
        for (std::size_t j = 0; j != targets.size(); ++j, ++k)
        {
            char const first = static_cast<char>('A' + (i % 20) + 1);
            HPX_TEST(results[k].get().to_host() == pattern(sizes[k], first));
        }
    }
}
HPX_PLAIN_ACTION(exchange_many)

///////////////////////////////////////////////////////////////////////////////
// Several GPUs per locality: a component per GPU names it as its target (the
// way hpx::cuda::experimental::target does), its argument's device data is
// received on that GPU.
struct gpu_target
{
    struct native_handle_type
    {
        int device = -1;

        [[nodiscard]] int get_device() const noexcept
        {
            return device;
        }
    };

    [[nodiscard]] native_handle_type const& native_handle() const noexcept
    {
        return handle;
    }

    native_handle_type handle;
};

struct on_gpu : hpx::components::component_base<on_gpu>
{
    on_gpu() = default;
    explicit on_gpu(int const gpu)
      : target{{gpu}}
    {
    }

    [[nodiscard]] gpu_target const& get_target() const noexcept
    {
        return target;
    }

    // the data has arrived on this component's GPU
    void arrived(
        device_buffer const& d, std::size_t const size, char const first)
    {
        HPX_TEST_EQ(d.gpu, target.handle.device);
        HPX_TEST_EQ(gpu_of(d.data.get()), target.handle.device);
        HPX_TEST(d.to_host() == pattern(size, first));
    }

    HPX_DEFINE_COMPONENT_ACTION(on_gpu, arrived)

    gpu_target target;
};

using on_gpu_type = hpx::components::component<on_gpu>;
HPX_REGISTER_COMPONENT(on_gpu_type, on_gpu)
HPX_REGISTER_ACTION(on_gpu::arrived_action, on_gpu_arrived_action)

// sends data from each of the GPUs 'first_gpu' .. 'first_gpu + count - 1' of
// this locality to each of the components 'targets' (on the other
// localities, one per GPU)
void exchange_gpus(std::vector<hpx::id_type> const& targets,
    int const first_gpu, int const count)
{
    std::vector<hpx::future<void>> checks;
    for (int gpu = first_gpu; gpu != first_gpu + count; ++gpu)
    {
        for (std::size_t i = 0; i != targets.size(); ++i)
        {
            for (std::size_t size : {std::size_t(1000), std::size_t(1) << 20})
            {
                char const first =
                    static_cast<char>('a' + (gpu + static_cast<int>(i)) % 20);
                checks.push_back(hpx::async<on_gpu::arrived_action>(targets[i],
                    device_buffer::from_host(pattern(size, first), gpu), size,
                    first));
            }
        }
    }
    hpx::wait_all(checks);
    for (auto& f : checks)
    {
        HPX_TEST(!f.has_exception());
    }
}
HPX_PLAIN_ACTION(exchange_gpus)

// each locality uses 'per_locality' GPUs of its node, disjoint from the
// other localities on the node (NCCL doesn't allow two ranks of a
// communicator on one GPU): data from each of them to each GPU of the other
// localities, all localities at once
void test_several_gpus(
    std::map<hpx::id_type, int> const& first_gpus, int const per_locality)
{
    std::map<hpx::id_type, std::vector<hpx::id_type>> components;
    for (auto const& [id, first] : first_gpus)
    {
        for (int gpu = first; gpu != first + per_locality; ++gpu)
        {
            components[id].push_back(hpx::new_<on_gpu>(id, gpu).get());
        }
    }

    std::vector<hpx::future<void>> busy;
    for (auto const& [id, first] : first_gpus)
    {
        std::vector<hpx::id_type> targets;
        for (auto const& [other, ids] : components)
        {
            if (other != id)
            {
                targets.insert(targets.end(), ids.begin(), ids.end());
            }
        }
        busy.push_back(
            hpx::async<exchange_gpus_action>(id, targets, first, per_locality));
    }
    hpx::wait_all(busy);
    for (auto& f : busy)
    {
        HPX_TEST(!f.has_exception());
    }
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    hpx::parcelset::device_conduit const* conduit =
        hpx::parcelset::get_device_conduit();
    HPX_TEST(conduit != nullptr);
    if (conduit == nullptr)
    {
        return hpx::finalize();
    }
    HPX_TEST_EQ(conduit->name(), std::string("nccl"));

    // a GPU for each locality, one no other locality on its node uses, and
    // the GPUs of each locality for the test of several GPUs per locality
    // (at most two each, the first ones of its share of the node's GPUs;
    // only where the localities of a node see the same GPUs)
    std::map<hpx::id_type, int> first_gpus;
    int per_locality = 2;
    {
        std::map<std::string,
            std::vector<std::pair<hpx::id_type, std::vector<std::string>>>>
            nodes;
        for (hpx::id_type const& id : hpx::find_all_localities())
        {
            auto [host, gpus] = node_info_action()(id);
            nodes[host].emplace_back(id, HPX_MOVE(gpus));
        }
        for (auto const& [host, localities] : nodes)
        {
            bool const same_gpus = std::all_of(
                localities.begin(), localities.end(), [&](auto const& l) {
                    return l.second == localities.front().second;
                });
            int const n = static_cast<int>(localities.size());
            if (same_gpus)
            {
                int const count =
                    static_cast<int>(localities.front().second.size());
                if (count < n)
                {
                    std::cout << "skipped: node " << host << " has " << count
                              << " GPUs for " << n << " localities\n";
                    return hpx::finalize();
                }
                per_locality = (std::min) (per_locality, count / n);
                for (int i = 0; i != n; ++i)
                {
                    first_gpus[localities[i].first] = i;    // for now
                }
                continue;
            }

            // each sees GPUs of its own: the first one not taken yet (by its
            // PCI bus id); one GPU per locality
            per_locality = 1;
            std::vector<std::string> taken;
            for (auto const& [id, gpus] : localities)
            {
                auto const free = std::find_if(
                    gpus.begin(), gpus.end(), [&](std::string const& gpu) {
                        return std::find(taken.begin(), taken.end(), gpu) ==
                            taken.end();
                    });
                if (free == gpus.end())
                {
                    std::cout << "skipped: no GPU of its own for a locality "
                                 "on node "
                              << host << "\n";
                    return hpx::finalize();
                }
                taken.push_back(*free);
                first_gpus[id] = -1 - static_cast<int>(free - gpus.begin());
            }
        }
        // the shared GPUs' indices on their node into GPU numbers (the
        // others are stored as -1 - number)
        for (auto& [id, first] : first_gpus)
        {
            first = first < 0 ? -1 - first : first * per_locality;
        }
    }

    for (auto const& [id, first] : first_gpus)
    {
        use_own_gpu_action()(id, first);
    }

    std::vector<hpx::id_type> const remotes = hpx::find_remote_localities();
    for (hpx::id_type const& id : remotes)
    {
        // single chunks, from small to larger than NCCL's buffers
        for (std::size_t size :
            {std::size_t(1), std::size_t(16), std::size_t(4000),
                std::size_t(1) << 20, (std::size_t(8) << 20) + 3})
        {
            device_buffer const result = exchange_action()(
                id, device_buffer::from_host(pattern(size, 'a')), size, 'a');
            HPX_TEST(result.to_host() == pattern(size, 'b'));
        }

        // empty data has no chunk
        device_buffer const empty =
            exchange_action()(id, device_buffer(0), 0, 'a');
        HPX_TEST_EQ(empty.size, std::size_t(0));

        // two chunks in a parcel
        std::size_t const size = std::size_t(3) << 16;
        auto const [c, d] = exchange_two_action()(id,
            device_buffer::from_host(pattern(size, 'a')),
            device_buffer::from_host(pattern(size / 2, 'b')), size);
        HPX_TEST(c.to_host() == pattern(size / 3, 'c'));
        HPX_TEST(d.to_host() == pattern(size, 'd'));
    }

    // many parcels in flight, all localities sending to all others
    std::vector<hpx::future<void>> busy;
    for (hpx::id_type const& id : hpx::find_all_localities())
    {
        std::vector<hpx::id_type> targets;
        for (hpx::id_type const& other : hpx::find_all_localities())
        {
            if (other != id)
            {
                targets.push_back(other);
            }
        }
        busy.push_back(hpx::async<exchange_many_action>(id, targets, 64));
    }
    hpx::wait_all(busy);
    for (auto& f : busy)
    {
        HPX_TEST(!f.has_exception());
    }

    if (per_locality >= 2)
    {
        test_several_gpus(first_gpus, per_locality);
    }
    else
    {
        std::cout << "several GPUs per locality: skipped (a node has fewer "
                     "than two GPUs per locality)\n";
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    hpx::init_params params;
    params.cfg = {"hpx.parcel.device_conduit=nccl"};
    HPX_TEST_EQ(hpx::init(argc, argv, params), 0);
    return hpx::util::report_errors();
}
#endif
