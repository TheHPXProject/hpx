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

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

// universal_vector is serialized from its own memory only with allocators of
// memory the host can access
static_assert(hpx::serialization::detail::is_universal_allocator_v<int,
    ::thrust::universal_vector<int>::allocator_type>);
static_assert(!hpx::serialization::detail::is_universal_allocator_v<int,
    ::thrust::device_allocator<int>>);

// A filter that passes the data through unchanged (e.g. like a compressor,
// it reads and writes all data of the archive on the host).
struct identity_filter : hpx::serialization::binary_filter
{
    void set_max_length(std::size_t const size) override
    {
        buffer_.reserve(size);
    }

    void save(void const* src, std::size_t const src_count) override
    {
        char const* begin = static_cast<char const*>(src);
        buffer_.insert(buffer_.end(), begin, begin + src_count);
    }

    bool flush(
        void* dst, std::size_t const dst_count, std::size_t& written) override
    {
        written = (std::min) (dst_count, buffer_.size() - current_);
        std::memcpy(dst, buffer_.data() + current_, written);
        current_ += written;
        return current_ == buffer_.size();
    }

    std::size_t init_data(void const* buffer, std::size_t const size,
        std::size_t const buffer_size) override
    {
        char const* begin = static_cast<char const*>(buffer);
        buffer_.assign(begin, begin + size);
        current_ = 0;
        return buffer_size;
    }

    void load(void* dst, std::size_t const dst_count) override
    {
        HPX_TEST(current_ + dst_count <= buffer_.size());
        if (current_ + dst_count > buffer_.size())
        {
            throw std::runtime_error("identity_filter: too little data");
        }
        std::memcpy(dst, buffer_.data() + current_, dst_count);
        current_ += dst_count;
    }

    template <typename Archive>
    void serialize(Archive&, unsigned)
    {
    }

    HPX_SERIALIZATION_POLYMORPHIC(identity_filter, override);

private:
    std::vector<char> buffer_;
    std::size_t current_ = 0;
};

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
        bool device_receive;    // only with zero_copy_receive
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
        test::received const r = test::receive(
            m, o.zero_copy_receive,
            [&w](hpx::serialization::input_archive& ar) { ar >> w; },
            o.device_receive);

        HPX_TEST_EQ(w.size(), size);
        HPX_TEST(::thrust::host_vector<T>(w) == expected);

        // with zero-copy receive the data goes to pinned memory directly, and
        // to device memory if the parcelport receives into device memory
        // (whatever memory it was sent from), else through a staging buffer
        // and a step that copies it to the device
        if (o.zero_copy_receive && chunk)
        {
            void const* destination = ::thrust::raw_pointer_cast(w.data());
            bool const direct = is_pinned(destination) || o.device_receive;

            HPX_TEST_EQ(r.destinations.size(), std::size_t(1));
            if (!r.destinations.empty())
            {
                HPX_TEST_EQ(r.destinations[0] == destination, direct);

                // the parcelport must know that it writes to device memory
                // (the test's copy would work with either)
                bool const to_device = direct && !is_pinned(destination);
                HPX_TEST_EQ(
                    r.device_destinations, std::size_t(to_device ? 1 : 0));
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
                for (auto const [zero_copy_receive, device_receive] :
                    {std::pair(false, false), std::pair(true, false),
                        std::pair(true, true)})
                {
                    test_device_vector<From, To>(size,
                        {preprocess, false, false, zero_copy_receive,
                            device_receive});
                    test_device_vector<From, To>(size,
                        {preprocess, true, false, zero_copy_receive,
                            device_receive});
                    test_device_vector<From, To>(size,
                        {preprocess, true, true, zero_copy_receive,
                            device_receive});

                    // the parcelport sends device memory, but the archive
                    // has no chunks: the data must go through the host
                    test_device_vector<From, To>(size,
                        {preprocess, false, true, zero_copy_receive,
                            device_receive});
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

    // a field the serialization leaves out
    struct partial
    {
        int a = 0;
        short b = 0;

        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            // clang-format off
            ar & b;
            // clang-format on
        }
    };

    // with the array optimization disabled the elements are received one by
    // one into fresh elements, as for std::vector
    [[maybe_unused]] void test_array_optimization_disabled()
    {
        using hpx::serialization::archive_flags;

        ::thrust::host_vector<partial> v(3);
        for (std::size_t i = 0; i != v.size(); ++i)
        {
            v[i].a = static_cast<int>(i + 1);
            v[i].b = static_cast<short>(10 * i);
        }

        std::uint32_t const flags = test::archive_flags(true) |
            static_cast<std::uint32_t>(
                archive_flags::disable_array_optimization);
        test::message m = test::send(flags, true,
            [&v](hpx::serialization::output_archive& ar) { ar << v; });

        // an element the vector holds already doesn't keep its value
        ::thrust::host_vector<partial> w(1);
        w[0].a = 99;
        test::receive(
            m, false, [&w](hpx::serialization::input_archive& ar) { ar >> w; });

        HPX_TEST_EQ(w.size(), v.size());
        for (std::size_t i = 0; i != w.size() && i != v.size(); ++i)
        {
            partial const& got = w[i];
            HPX_TEST_EQ(got.b, v[i].b);
            HPX_TEST_EQ(got.a, 0);
        }
    }

    // the elements of a device_vector are sent as raw bytes: an archive with
    // another byte order is rejected when saving, as when loading (only
    // with HPX_SERIALIZATION_WITH_SUPPORTS_ENDIANESS archives can have one)
    // A filter reads and writes all data on the host: a device_vector in a
    // filtered archive is sent and received through host memory, also if the
    // archive may refer to device memory and zero-copy chunks are on.
    void test_filtered_archive()
    {
        using hpx::serialization::archive_flags;

        std::size_t const size = 65536;
        ::thrust::host_vector<int> const expected = values<int>(size);
        ::thrust::device_vector<int> const v(expected);

        std::vector<char> buffer;
        std::vector<hpx::serialization::serialization_chunk> chunks;
        identity_filter filter;
        std::uint32_t const flags = test::archive_flags(true, true) |
            static_cast<std::uint32_t>(archive_flags::enable_compression);

        std::size_t written = 0;
        {
            hpx::serialization::output_archive ar(buffer, flags, &chunks,
                &filter, HPX_ZERO_COPY_SERIALIZATION_THRESHOLD);
            HPX_TEST(ar.is_filtered());
            ar << v;
            ar.flush();
            written = ar.bytes_written();
        }

        // no chunk refers to the device memory
        for (auto const& chunk : chunks)
        {
            HPX_TEST(chunk.memory_ != hpx::serialization::chunk_memory::device);
        }

        hpx::serialization::input_archive ar(buffer, written, &chunks);
        HPX_TEST(ar.is_filtered());
        ::thrust::device_vector<int> w;
        ar >> w;

        HPX_TEST_EQ(w.size(), size);
        HPX_TEST(::thrust::host_vector<int>(w) == expected);
    }

    void test_other_byte_order()
    {
#if defined(HPX_SERIALIZATION_HAVE_SUPPORTS_ENDIANESS)
        using hpx::serialization::archive_flags;

        std::uint32_t const both = static_cast<std::uint32_t>(
            archive_flags::endian_big | archive_flags::endian_little);
        std::uint32_t flags = test::archive_flags(true);
        std::uint32_t const other =
            (flags & static_cast<std::uint32_t>(archive_flags::endian_big)) ?
            static_cast<std::uint32_t>(archive_flags::endian_little) :
            static_cast<std::uint32_t>(archive_flags::endian_big);
        flags = (flags & ~both) | other;

        ::thrust::device_vector<int> const v(16, 7);
        bool caught = false;
        try
        {
            test::send(flags, false,
                [&v](hpx::serialization::output_archive& ar) { ar << v; });
        }
        catch (hpx::exception const&)
        {
            caught = true;
        }
        HPX_TEST(caught);

        // an empty vector carries no elements, it is fine
        ::thrust::device_vector<int> const empty;
        test::send(flags, false,
            [&empty](hpx::serialization::output_archive& ar) { ar << empty; });
#endif
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
#if !defined(HPX_SERIALIZATION_HAVE_ALL_TYPES_ARE_BITWISE_SERIALIZABLE)
    // all types are copied as bytes otherwise
    test_array_optimization_disabled();
#endif
    test_other_byte_order();
    test_filtered_archive();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
