//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// hpx::thrust::pooled_allocator: memory of the configured kind
// (hpx.thrust.memory), reuse of deallocated memory, and serialization of
// device_vectors of pooled memory, also to and from other device_vectors.

#include <hpx/config.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/thrust.hpp>

#include <thrust/device_vector.h>
#include <thrust/reduce.h>
#include <thrust/sequence.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <vector>

// thrust_pooled_allocator_archives.cpp: nvcc aborts compiling the construction
// of the archives
namespace test {

    hpx::serialization::output_archive* make_output_archive(
        std::vector<char>& buffer);
    hpx::serialization::input_archive* make_input_archive(
        std::vector<char>& buffer);
    void destroy(hpx::serialization::output_archive* archive) noexcept;
    void destroy(hpx::serialization::input_archive* archive) noexcept;
}    // namespace test

namespace {

    struct archive_deleter
    {
        template <typename Archive>
        void operator()(Archive* archive) const noexcept
        {
            test::destroy(archive);
        }
    };

    // the archive's destructor completes the data
    template <typename Archive>
    using archive = std::unique_ptr<Archive, archive_deleter>;

    void test_memory_kind()
    {
        std::string const config =
            hpx::get_config_entry("hpx.thrust.memory", "device");
        hpx::thrust::memory_kind const kind =
            hpx::thrust::default_memory_kind();
        HPX_TEST(kind ==
            (config == "pinned" ? hpx::thrust::memory_kind::pinned :
                                  hpx::thrust::memory_kind::device));

        hpx::thrust::device_vector<int> v(1000);
        HPX_TEST_EQ(hpx::thrust::detail::is_pinned_host_memory(
                        ::thrust::raw_pointer_cast(v.data())),
            kind == hpx::thrust::memory_kind::pinned);

        // either kind of memory, explicitly
        std::size_t const bytes = 4096;
        void* device = hpx::thrust::detail::pool_allocate(
            bytes, hpx::thrust::memory_kind::device);
        void* pinned = hpx::thrust::detail::pool_allocate(
            bytes, hpx::thrust::memory_kind::pinned);
        HPX_TEST(!hpx::thrust::detail::is_pinned_host_memory(device));
        HPX_TEST(hpx::thrust::detail::is_pinned_host_memory(pinned));
        hpx::thrust::detail::pool_deallocate(device, bytes);
        hpx::thrust::detail::pool_deallocate(pinned, bytes);
    }

    void test_device_work()
    {
        std::size_t const size = 100000;
        hpx::thrust::device_vector<std::int64_t> v(size);
        ::thrust::sequence(v.begin(), v.end());
        std::int64_t const sum =
            ::thrust::reduce(v.begin(), v.end(), std::int64_t(0));
        HPX_TEST_EQ(sum, static_cast<std::int64_t>(size * (size - 1) / 2));

        // copies to and from other device_vectors
        ::thrust::device_vector<std::int64_t> plain(v);
        hpx::thrust::device_vector<std::int64_t> back(plain);
        HPX_TEST(back == v);
    }

    void test_reuse()
    {
        // memory of the same size class comes back from the pool (main sets
        // a cache limit of 16 MiB), starting with an empty cache
        hpx::thrust::release_cached_memory();
        void* first = nullptr;
        {
            hpx::thrust::device_vector<char> v(std::size_t(1) << 20);
            first = ::thrust::raw_pointer_cast(v.data());
        }
        {
            // 1 MiB - 100 bytes is in the same size class as 1 MiB
            hpx::thrust::device_vector<char> v((std::size_t(1) << 20) - 100);
            HPX_TEST_EQ(
                static_cast<void*>(::thrust::raw_pointer_cast(v.data())),
                first);
        }

        hpx::thrust::release_cached_memory();
    }

    template <typename To, typename From>
    void test_round_trip(std::size_t const size)
    {
        From v(size);
        ::thrust::sequence(v.begin(), v.end());

        std::vector<char> buffer;
        {
            archive<hpx::serialization::output_archive> oarchive(
                test::make_output_archive(buffer));
            *oarchive << v;
        }

        To w;
        {
            archive<hpx::serialization::input_archive> iarchive(
                test::make_input_archive(buffer));
            *iarchive >> w;
        }
        HPX_TEST_EQ(w.size(), size);
        HPX_TEST(
            ::thrust::device_vector<int>(w) == ::thrust::device_vector<int>(v));
    }

    void test_serialization()
    {
        using pooled = hpx::thrust::device_vector<int>;
        using plain = ::thrust::device_vector<int>;

        // below and above the zero-copy threshold
        for (std::size_t size : {0, 10, 1000000})
        {
            test_round_trip<pooled, pooled>(size);
            test_round_trip<pooled, plain>(size);
            test_round_trip<plain, pooled>(size);
        }
    }

    // element counts whose size in bytes overflows are rejected instead of
    // wrapping around to a small allocation
    void test_overflow()
    {
        hpx::thrust::pooled_allocator<std::int64_t> alloc;
        std::size_t const too_many =
            (std::numeric_limits<std::size_t>::max)() / sizeof(std::int64_t) +
            1;

        bool caught = false;
        try
        {
            (void) alloc.allocate(too_many);
        }
        catch (std::bad_alloc const&)
        {
            caught = true;
        }
        HPX_TEST(caught);
    }
}    // namespace

// the memory kind of the first runtime (see hpx_main_restarted)
hpx::thrust::memory_kind first_kind = hpx::thrust::memory_kind::device;

int hpx_main(hpx::program_options::variables_map&)
{
    first_kind = hpx::thrust::default_memory_kind();
    test_memory_kind();
    test_overflow();
    test_device_work();
    test_reuse();
    test_serialization();
    return hpx::local::finalize();
}

// A runtime started later in the same process uses its own configuration,
// here the other kind of memory.
int hpx_main_restarted(hpx::program_options::variables_map&)
{
    hpx::thrust::memory_kind const expected =
        first_kind == hpx::thrust::memory_kind::device ?
        hpx::thrust::memory_kind::pinned :
        hpx::thrust::memory_kind::device;
    HPX_TEST(hpx::thrust::default_memory_kind() == expected);

    hpx::thrust::device_vector<int> v(1000);
    HPX_TEST_EQ(hpx::thrust::detail::is_pinned_host_memory(
                    ::thrust::raw_pointer_cast(v.data())),
        expected == hpx::thrust::memory_kind::pinned);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    hpx::local::init_params init_args;
    init_args.cfg = {"hpx.thrust.max_cached=16777216"};
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv, init_args), 0);

    // the command line of the first runtime may select the memory kind
    hpx::local::init_params restarted_args;
    restarted_args.cfg = {first_kind == hpx::thrust::memory_kind::device ?
            "hpx.thrust.memory=pinned" :
            "hpx.thrust.memory=device",
        "hpx.thrust.max_cached=0"};
    char* restarted_argv[] = {argv[0], nullptr};
    HPX_TEST_EQ(
        hpx::local::init(hpx_main_restarted, 1, restarted_argv, restarted_args),
        0);
    return hpx::util::report_errors();
}
