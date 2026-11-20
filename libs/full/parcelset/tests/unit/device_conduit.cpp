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
// going through the parcelport. The conduit completes its sends late (after
// the parcels have left), as a network would: the statistics lag behind, and
// the conduit must outlive the completions at shutdown.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/plugin_factories.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <mutex>
#include <random>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

namespace detail = hpx::serialization::detail;

///////////////////////////////////////////////////////////////////////////////
// the directory of the chunk files, unique for each run of the test (locality
// 0 creates it and hands it to all localities before any data is sent)
std::mutex chunk_directory_mtx;
std::filesystem::path chunk_directory;

void set_chunk_directory(std::string const& dir)
{
    std::lock_guard<std::mutex> l(chunk_directory_mtx);
    chunk_directory = dir;
}
HPX_PLAIN_ACTION(set_chunk_directory)

std::filesystem::path get_chunk_directory()
{
    std::lock_guard<std::mutex> l(chunk_directory_mtx);
    HPX_TEST(!chunk_directory.empty());
    return chunk_directory;
}

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
        void const* p, std::size_t size,
        hpx::move_only_function<void(std::exception_ptr)> sent)
    {
        std::exception_ptr error = write(d, p, size);

        // complete the send late, after the parcel has left (the conduit
        // must stay alive until then, also at shutdown)
        std::thread([sent = HPX_MOVE(sent), error]() mutable {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            sent(HPX_MOVE(error));
        }).detach();
    }

    void recv_impl(detail::device_chunk_descriptor const& d, void* p,
        std::size_t size, std::function<void(std::exception_ptr)> done)
    {
        std::filesystem::path const name = path(d);
        bool const read =
            std::ifstream(name, std::ios::binary)
                .read(static_cast<char*>(p), static_cast<std::streamsize>(size))
                .good();
        std::error_code ec;
        std::filesystem::remove(name, ec);
        if (!read)
        {
            done(std::make_exception_ptr(
                std::runtime_error("can't read " + name.string())));
            return;
        }
        done(nullptr);
    }

private:
    // writes the file of a chunk (the sender writes it before the parcel
    // referring to it leaves); the error if it couldn't
    static std::exception_ptr write(detail::device_chunk_descriptor const& d,
        void const* p, std::size_t size)
    {
        std::filesystem::path const name = path(d);
        std::filesystem::path tmp = name;
        tmp += ".tmp";

        std::ofstream out(tmp, std::ios::binary);
        out.write(
            static_cast<char const*>(p), static_cast<std::streamsize>(size));
        bool const written = out.good();
        out.close();
        if (!written || out.fail())
        {
            std::error_code ec;
            std::filesystem::remove(tmp, ec);
            return std::make_exception_ptr(
                std::runtime_error("can't write " + tmp.string()));
        }

        std::error_code ec;
        std::filesystem::rename(tmp, name, ec);
        if (ec)
        {
            std::filesystem::remove(tmp, ec);
            return std::make_exception_ptr(
                std::runtime_error("can't rename " + tmp.string()));
        }
        return nullptr;
    }

    // the descriptor holds the sending locality and a tag unique for it
    static std::filesystem::path path(detail::device_chunk_descriptor const& d)
    {
        return get_chunk_directory() /
            ("chunk-" + std::to_string(d.source) + "-" + std::to_string(d.tag));
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

// a new directory for the chunk files of this run
std::filesystem::path make_chunk_directory()
{
    std::random_device rd;
    std::mt19937_64 gen(rd());
    for (int i = 0; i != 100; ++i)
    {
        std::filesystem::path const dir =
            std::filesystem::temp_directory_path() /
            ("hpx-device-conduit-" + std::to_string(gen()));
        if (std::filesystem::create_directory(dir))
        {
            return dir;
        }
    }
    throw std::runtime_error("can't create a directory for the chunk files");
}

// waits (bounded) until the sends that 'before' doesn't count yet have
// completed: the conduit completes them after the parcels have left
hpx::parcelset::device_conduit_statistics wait_for_sends(
    hpx::parcelset::device_conduit const* conduit,
    hpx::parcelset::device_conduit_statistics const& before,
    std::uint64_t chunks)
{
    auto const deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(10);
    hpx::parcelset::device_conduit_statistics s = conduit->statistics();
    while (s.chunks_sent - before.chunks_sent < chunks &&
        std::chrono::steady_clock::now() < deadline)
    {
        hpx::this_thread::sleep_for(std::chrono::milliseconds(1));
        s = conduit->statistics();
    }
    return s;
}

int hpx_main()
{
    hpx::parcelset::device_conduit const* conduit =
        hpx::parcelset::get_device_conduit();
    HPX_TEST(conduit != nullptr);
    HPX_TEST_EQ(conduit->name(), std::string("test_file"));

    std::filesystem::path const dir = make_chunk_directory();
    {
        std::vector<hpx::future<void>> set;
        for (hpx::id_type const& id : hpx::find_all_localities())
        {
            set.push_back(
                hpx::async<set_chunk_directory_action>(id, dir.string()));
        }
        hpx::wait_all(set);
    }

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
            wait_for_sends(conduit, before, 2);
        HPX_TEST_EQ(after.chunks_sent - before.chunks_sent, std::uint64_t(2));
        HPX_TEST_EQ(after.bytes_sent - before.bytes_sent,
            std::uint64_t(16 + (1 << 20)));
        HPX_TEST_EQ(
            after.chunks_received - before.chunks_received, std::uint64_t(2));
        HPX_TEST_EQ(
            chunks_received_action()(id) - remote_before, std::uint64_t(2));
    }

    // the files are gone once received; the sends of the last results may
    // still complete during shutdown (the conduit waits for them)
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);

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
