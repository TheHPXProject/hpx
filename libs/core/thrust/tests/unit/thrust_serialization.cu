//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Serialization of thrust containers the way a parcelport does it:
// preprocessing passes, zero-copy chunks (of device memory if the parcelport
// moves device memory), and zero-copy receive with the steps that complete
// the data afterwards. device_vectors of device memory are sent from a
// pinned host copy the preprocessing prepares and received through a pinned
// staging buffer, device_vectors of pinned host memory (run with
// hpx.thrust.memory=pinned) are sent and received in place.

#include <hpx/config.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/thrust.hpp>

#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <thrust/universal_vector.h>

#include "thrust_serialization.hpp"

#include <cstddef>
#include <new>
#include <optional>
#include <type_traits>

// universal_vector is serialized from its own memory only with allocators of
// memory the host can access
static_assert(hpx::serialization::detail::is_universal_allocator_v<int,
    ::thrust::universal_vector<int>::allocator_type>);
static_assert(!hpx::serialization::detail::is_universal_allocator_v<int,
    ::thrust::device_allocator<int>>);

namespace {

    template <typename T>
    T value_at(std::size_t const i)
    {
        if constexpr (std::is_same_v<T, bool>)
        {
            return i % 3 == 0;
        }
        else
        {
            return static_cast<T>(i);
        }
    }

    template <typename T>
    ::thrust::host_vector<T> values(std::size_t const size)
    {
        ::thrust::host_vector<T> result(size);
        for (std::size_t i = 0; i != size; ++i)
        {
            result[i] = value_at<T>(i);
        }
        return result;
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
    };

    template <typename From, typename To>
    void test_device_vector(std::size_t const size, options const o)
    {
        using T = typename From::value_type;

        ::thrust::host_vector<T> const expected = values<T>(size);
        From const v(expected);
        void const* source =
            size != 0 ? ::thrust::raw_pointer_cast(v.data()) : nullptr;
        bool const pinned = is_pinned(source);
        bool const above =
            size * sizeof(T) >= HPX_ZERO_COPY_SERIALIZATION_THRESHOLD;

        test::message m = test::send(
            test::archive_flags(o.chunks, o.device_chunks), o.preprocess,
            [&v](hpx::serialization::output_archive& ar) { ar << v; });

        // pinned memory and device memory the parcelport moves are sent in
        // place, else the preprocessing prepares a pinned host copy of large
        // data (also if chunks are off, it can't know)
        bool const in_place = pinned || o.device_chunks;
        bool const host_copy = !in_place && o.preprocess && above;
        HPX_TEST_EQ(m.keep_alive, std::size_t(host_copy ? 1 : 0));

        bool const chunk = above && o.chunks && (in_place || host_copy);
        HPX_TEST_EQ(m.sources.size(), std::size_t(chunk ? 1 : 0));
        if (chunk && !m.sources.empty())
        {
            HPX_TEST_EQ(m.sources[0] == source, in_place);
        }

        // without a keep-alive (no parcel) the preprocessing prepares no
        // copy and nothing to wait for; the data is copied into the archive
        if (o.preprocess)
        {
            test::preprocessed const p = test::preprocess_only(
                test::archive_flags(o.chunks, o.device_chunks),
                [&v](hpx::serialization::output_archive& ar) { ar << v; });
            HPX_TEST(!p.futures);
            if (!in_place)
            {
                HPX_TEST_LTE(size * sizeof(T), p.size);
            }
        }

        To w;
        test::received const r = test::receive(m, o.zero_copy_receive,
            [&w](hpx::serialization::input_archive& ar) { ar >> w; });

        HPX_TEST_EQ(w.size(), size);
        HPX_TEST(::thrust::host_vector<T>(w) == expected);

        // with zero-copy receive the data goes to pinned memory and to device
        // memory from a device chunk directly, else through a staging buffer
        // and a step that copies it to the device
        if (o.zero_copy_receive && chunk)
        {
            void const* destination = ::thrust::raw_pointer_cast(w.data());
            bool const device_chunk = !pinned && o.device_chunks;
            bool const direct = is_pinned(destination) || device_chunk;

            HPX_TEST_EQ(r.destinations.size(), std::size_t(1));
            if (!r.destinations.empty())
            {
                HPX_TEST_EQ(r.destinations[0] == destination, direct);
            }
            HPX_TEST_EQ(r.steps, std::size_t(direct ? 0 : 1));
        }
        else
        {
            HPX_TEST(r.destinations.empty());
            HPX_TEST_EQ(r.steps, std::size_t(0));
        }
    }

    template <typename From, typename To>
    void test_device_vectors()
    {
        for (std::size_t size : {0, 16, 1 << 16})
        {
            for (bool preprocess : {false, true})
            {
                for (bool zero_copy_receive : {false, true})
                {
                    test_device_vector<From, To>(
                        size, {preprocess, false, false, zero_copy_receive});
                    test_device_vector<From, To>(
                        size, {preprocess, true, false, zero_copy_receive});
                    test_device_vector<From, To>(
                        size, {preprocess, true, true, zero_copy_receive});

                    // the parcelport sends device memory, but the archive
                    // has no chunks: the data must go through the host
                    test_device_vector<From, To>(
                        size, {preprocess, false, true, zero_copy_receive});
                }
            }
        }
    }

    // Two device vectors with different values serialized from the same
    // address (temporaries a serialize() function creates on the fly), with
    // the preprocessing rounds of a parcel: each gets its own host copy (the
    // copies are matched by position, not by address).
    template <typename Vector>
    void test_same_address(bool const chunks)
    {
        using T = typename Vector::value_type;

        std::size_t const size = 1 << 16;
        ::thrust::host_vector<T> const first = values<T>(size);
        ::thrust::host_vector<T> second(size);
        for (std::size_t i = 0; i != size; ++i)
        {
            second[i] = value_at<T>(i + 1);
        }

        test::message m = test::send(test::archive_flags(chunks, false), true,
            [&](hpx::serialization::output_archive& ar) {
                std::optional<Vector> temporary;
                temporary.emplace(first);
                ar << *temporary;
                temporary.emplace(second);
                ar << *temporary;
            });
        HPX_TEST_EQ(m.keep_alive, std::size_t(2));

        Vector received_first;
        Vector received_second;
        test::receive(m, false, [&](hpx::serialization::input_archive& ar) {
            ar >> received_first >> received_second;
        });
        HPX_TEST(::thrust::host_vector<T>(received_first) == first);
        HPX_TEST(::thrust::host_vector<T>(received_second) == second);
    }

    // host_vector and universal_vector: zero-copy chunks of their own memory
    template <typename Vector>
    void test_host_accessible(
        std::size_t const size, bool const chunks, bool const zero_copy_receive)
    {
        using T = typename Vector::value_type;

        ::thrust::host_vector<T> const expected = values<T>(size);
        Vector const v(expected);
        bool const above =
            size * sizeof(T) >= HPX_ZERO_COPY_SERIALIZATION_THRESHOLD;

        test::message m = test::send(test::archive_flags(chunks), true,
            [&v](hpx::serialization::output_archive& ar) { ar << v; });

        HPX_TEST_EQ(m.keep_alive, std::size_t(0));
        bool const chunk = above && chunks;
        HPX_TEST_EQ(m.sources.size(), std::size_t(chunk ? 1 : 0));
        if (chunk && !m.sources.empty())
        {
            HPX_TEST(m.sources[0] == ::thrust::raw_pointer_cast(v.data()));
        }

        Vector w;
        test::received const r = test::receive(m, zero_copy_receive,
            [&w](hpx::serialization::input_archive& ar) { ar >> w; });

        HPX_TEST_EQ(w.size(), size);
        HPX_TEST(::thrust::host_vector<T>(w) == expected);
        HPX_TEST_EQ(r.steps, std::size_t(0));
        if (zero_copy_receive && chunk)
        {
            HPX_TEST_EQ(r.destinations.size(), std::size_t(1));
            HPX_TEST(!r.destinations.empty() &&
                r.destinations[0] == ::thrust::raw_pointer_cast(w.data()));
        }
        else
        {
            HPX_TEST(r.destinations.empty());
        }
    }

    template <typename Vector>
    void test_host_accessible()
    {
        for (std::size_t size : {0, 16, 1 << 16})
        {
            for (bool chunks : {false, true})
            {
                test_host_accessible<Vector>(size, chunks, false);
                test_host_accessible<Vector>(size, chunks, true);
            }
        }
    }

    // elements without a default constructor go element by element, with
    // the construction hooks, as for std::vector
    struct no_default
    {
        explicit no_default(int const value)
          : a(value)
        {
        }

        int a;
        short b = 0;

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            // clang-format off
            ar & b;
            // clang-format on
        }
    };

    template <typename Archive>
    void save_construct_data(Archive& ar, no_default const* p, unsigned)
    {
        ar << p->a;
    }

    template <typename Archive>
    void load_construct_data(Archive& ar, no_default* p, unsigned)
    {
        int a = 0;
        ar >> a;
        ::new (p) no_default(a);
    }

    void test_non_default_constructible()
    {
        ::thrust::host_vector<no_default> v;
        for (int i = 0; i != 5; ++i)
        {
            no_default e(i);
            e.b = static_cast<short>(10 * i);
            v.push_back(e);
        }

        test::message m = test::send(test::archive_flags(true), true,
            [&v](hpx::serialization::output_archive& ar) { ar << v; });

        ::thrust::host_vector<no_default> w;
        test::receive(
            m, false, [&w](hpx::serialization::input_archive& ar) { ar >> w; });

        HPX_TEST_EQ(w.size(), v.size());
        for (std::size_t i = 0; i != w.size() && i != v.size(); ++i)
        {
            no_default const& got = w[i];
            no_default const& sent = v[i];
            HPX_TEST_EQ(got.a, sent.a);
            HPX_TEST_EQ(got.b, sent.b);
        }
    }
}    // namespace

int hpx_main(hpx::program_options::variables_map&)
{
    using plain = ::thrust::device_vector<int>;
    using pooled = hpx::thrust::device_vector<int>;

    // with hpx.thrust.memory=pinned the pooled vectors are sent and received
    // in place
    bool const pinned =
        hpx::get_config_entry("hpx.thrust.memory", "device") == "pinned";
    HPX_TEST_EQ(
        is_pinned(::thrust::raw_pointer_cast(pooled(1).data())), pinned);
    HPX_TEST(!is_pinned(::thrust::raw_pointer_cast(plain(1).data())));

    test_device_vectors<plain, plain>();
    test_device_vectors<pooled, pooled>();
    test_device_vectors<plain, pooled>();
    test_device_vectors<pooled, plain>();
    test_device_vectors<::thrust::device_vector<bool>,
        ::thrust::device_vector<bool>>();

    test_same_address<plain>(true);
    test_same_address<plain>(false);

    test_host_accessible<::thrust::host_vector<int>>();
    test_host_accessible<::thrust::universal_vector<int>>();
    test_non_default_constructible();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
