//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// A device conduit moves the data of zero-copy chunks of device memory
// between localities below the parcelports, the parcels carry only
// descriptors. The conduit here moves the data through files (both
// localities run on the same node); a type that marks its zero-copy chunk
// as device memory stands in for device data (the archives never touch the
// data of such chunks). Ordinary zero-copy chunks in the same parcels keep
// going through the parcelport.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/plugin_factories.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace detail = hpx::serialization::detail;

///////////////////////////////////////////////////////////////////////////////
// the test conduit: one file per chunk, named after the sending locality and
// the tag of the chunk
class file_conduit : public hpx::parcelset::device_conduit_impl<file_conduit>
{
public:
    explicit file_conduit(hpx::util::runtime_configuration const&) {}

    static std::string name_impl()
    {
        return "test_file";
    }

    bool can_reach_impl(std::uint32_t) const
    {
        return true;
    }

    void send_impl(std::uint32_t, detail::device_chunk_descriptor const& d,
        void const* p, std::size_t size, hpx::move_only_function<void()> sent)
    {
        std::string const tmp = path(d) + ".tmp";
        std::ofstream(tmp, std::ios::binary)
            .write(static_cast<char const*>(p),
                static_cast<std::streamsize>(size));
        std::filesystem::rename(tmp, path(d));
        sent();
    }

    void recv_impl(detail::device_chunk_descriptor const& d, void* p,
        std::size_t size, std::function<void(std::exception_ptr)> done)
    {
        std::string const name = path(d);
        bool const read =
            std::ifstream(name, std::ios::binary)
                .read(static_cast<char*>(p), static_cast<std::streamsize>(size))
                .good();
        std::filesystem::remove(name);
        if (!read)
        {
            done(std::make_exception_ptr(
                std::runtime_error("can't read " + name)));
            return;
        }
        done(nullptr);
    }

private:
    // the descriptor holds the sending locality and a tag unique for it (the
    // tests of this directory run one at a time; the sender writes the file
    // before the parcel referring to it leaves)
    static std::string path(detail::device_chunk_descriptor const& d)
    {
        return (std::filesystem::temp_directory_path() /
            ("hpx-device-conduit-" + std::to_string(d.source) + "-" +
                std::to_string(d.tag)))
            .string();
    }
};

// registers the conduit before the runtime starts
hpx::plugins::device_conduit_factory<file_conduit> const file_conduit_factory;

///////////////////////////////////////////////////////////////////////////////
// data whose zero-copy chunk is marked as device memory
struct device_like
{
    std::vector<char> data;

    template <typename Archive>
    void save(Archive& ar, unsigned) const
    {
        ar << data.size();
        ar.save_binary_chunk(detail::fundamental_types::none, data.data(),
            data.size(), hpx::serialization::chunk_memory::device);
    }

    template <typename Archive>
    void load(Archive& ar, unsigned)
    {
        std::size_t size = 0;
        ar >> size;
        data.resize(size);

        // the conduit moves the data (asked first, as device_vector does)
        HPX_TEST(ar.next_chunk_from_transport());
        ar.load_binary_chunk(detail::fundamental_types::none, data.data(), size,
            true, hpx::serialization::chunk_memory::device);
    }

    HPX_SERIALIZATION_SPLIT_MEMBER()
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
device_like exchange(
    device_like const& d, std::vector<char> const& host, std::size_t size)
{
    HPX_TEST(d.data == pattern(size, 'a'));
    HPX_TEST(host == pattern(size, 'h'));
    return device_like{pattern(size, 'r')};
}
HPX_PLAIN_ACTION(exchange)

std::uint64_t chunks_received()
{
    return hpx::parcelset::get_device_conduit()->statistics().chunks_received;
}
HPX_PLAIN_ACTION(chunks_received)

int hpx_main()
{
    hpx::parcelset::device_conduit const* conduit =
        hpx::parcelset::get_device_conduit();
    HPX_TEST(conduit != nullptr);
    HPX_TEST_EQ(conduit->name(), std::string("test_file"));

    for (hpx::id_type const& id : hpx::find_remote_localities())
    {
        std::uint64_t const remote_before = chunks_received_action()(id);
        hpx::parcelset::device_conduit_statistics const before =
            conduit->statistics();

        for (std::size_t size : {std::size_t(16), std::size_t(1) << 20})
        {
            device_like const result = exchange_action()(
                id, device_like{pattern(size, 'a')}, pattern(size, 'h'), size);
            HPX_TEST(result.data == pattern(size, 'r'));
        }

        // the device data of both arguments and both results went through
        // the conduit, in both directions
        hpx::parcelset::device_conduit_statistics const after =
            conduit->statistics();
        HPX_TEST_EQ(after.chunks_sent - before.chunks_sent, std::uint64_t(2));
        HPX_TEST_EQ(after.bytes_sent - before.bytes_sent,
            std::uint64_t(16 + (1 << 20)));
        HPX_TEST_EQ(
            after.chunks_received - before.chunks_received, std::uint64_t(2));
        HPX_TEST_EQ(
            chunks_received_action()(id) - remote_before, std::uint64_t(2));
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    hpx::init_params params;
    params.cfg = {"hpx.parcel.device_conduit=test_file"};

    HPX_TEST_EQ(hpx::init(argc, argv, params), 0);
    return hpx::util::report_errors();
}
#endif
