//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Host chunks through the UCX device conduit: with
// hpx.parcel.conduit_host_chunks=1 the conduit also moves the zero-copy
// chunks of host memory of at least hpx.parcel.conduit_host_threshold bytes
// whose objects accept the data after their de-serialization (arrays: here
// std::vector and serialize_buffer), between localities whose endpoints say
// they receive them that way. Smaller chunks go through the parcelport. The
// test checks the data of arguments and results, many calls in flight in
// both directions, and the conduit's statistics on both sides.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

constexpr std::size_t threshold = 65536;

///////////////////////////////////////////////////////////////////////////////
std::vector<int> pattern(std::size_t const count, int const first)
{
    std::vector<int> v(count);
    std::iota(v.begin(), v.end(), first);
    return v;
}

// checks its argument (the data has arrived when the action runs), returns
// data of the same size
std::vector<int> echo(std::vector<int> const& v, std::size_t count, int first)
{
    HPX_TEST(v == pattern(count, first));
    return pattern(count, first + 1);
}
HPX_PLAIN_ACTION(echo)

using buffer_type = hpx::serialization::serialize_buffer<char>;

buffer_type echo_buffer(buffer_type const& b, std::size_t size, char first)
{
    HPX_TEST_EQ(b.size(), size);
    bool ok = true;
    for (std::size_t i = 0; i != b.size(); ++i)
    {
        ok = ok && b[i] == static_cast<char>(first + i % 61);
    }
    HPX_TEST(ok);

    buffer_type result(size);
    for (std::size_t i = 0; i != size; ++i)
    {
        result[i] = static_cast<char>(first + 1 + i % 61);
    }
    return result;
}
HPX_PLAIN_ACTION(echo_buffer)

// the conduit's statistics on this locality
std::pair<std::uint64_t, std::uint64_t> conduit_chunks()
{
    auto const* conduit = hpx::parcelset::get_device_conduit();
    if (conduit == nullptr)
    {
        return {0, 0};
    }
    auto const s = conduit->statistics();
    return {s.chunks_sent, s.chunks_received};
}
HPX_PLAIN_ACTION(conduit_chunks)

// host chunks go to 'locality_id' through the conduit once it has checked
// that it reaches it, which it starts when a message has one (they go
// through the parcelport until then)
bool wait_for_host_chunks(std::uint32_t const locality_id)
{
    auto* conduit = hpx::parcelset::get_device_conduit();
    hpx::id_type const id = hpx::naming::get_id_from_locality_id(locality_id);
    std::size_t const count = threshold / sizeof(int);
    auto const deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (!conduit->host_chunks_ready(locality_id))
    {
        if (std::chrono::steady_clock::now() >= deadline)
        {
            return false;
        }
        HPX_TEST(echo_action()(id, pattern(count, 3), count, 3) ==
            pattern(count, 4));
        hpx::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}
HPX_PLAIN_ACTION(wait_for_host_chunks)

// many calls with large vectors to 'target' at once
void echo_many(hpx::id_type const& target, std::size_t const calls)
{
    std::vector<hpx::future<std::vector<int>>> results;
    for (std::size_t i = 0; i != calls; ++i)
    {
        std::size_t const count = threshold / sizeof(int) + i * 1000;
        results.push_back(hpx::async<echo_action>(target,
            pattern(count, static_cast<int>(i)), count, static_cast<int>(i)));
    }
    for (std::size_t i = 0; i != calls; ++i)
    {
        std::size_t const count = threshold / sizeof(int) + i * 1000;
        HPX_TEST(results[i].get() == pattern(count, static_cast<int>(i) + 1));
    }
}
HPX_PLAIN_ACTION(echo_many)

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    auto const* conduit = hpx::parcelset::get_device_conduit();
    HPX_TEST(conduit != nullptr);
    if (conduit == nullptr)
    {
        return hpx::finalize();
    }
    HPX_TEST_EQ(conduit->name(), std::string("ucx"));
    HPX_TEST(conduit->moves_host_memory());

    std::vector<hpx::id_type> const remotes = hpx::find_remote_localities();
    for (hpx::id_type const& id : remotes)
    {
        // both ways checked (the first host chunks go through the
        // parcelport meanwhile)
        HPX_TEST(
            wait_for_host_chunks(hpx::naming::get_locality_id_from_id(id)));
        HPX_TEST(wait_for_host_chunks_action()(id, hpx::get_locality_id()));

        // below the threshold: through the parcelport
        auto const before = conduit_chunks();
        auto const remote_before = conduit_chunks_action()(id);
        {
            std::size_t const count = threshold / sizeof(int) / 4;
            HPX_TEST(echo_action()(id, pattern(count, 7), count, 7) ==
                pattern(count, 8));
        }
        HPX_TEST(conduit_chunks() == before);
        HPX_TEST(conduit_chunks_action()(id) == remote_before);

        // at or above it: through the conduit, the argument and the result
        for (std::size_t count :
            {threshold / sizeof(int), (std::size_t(8) << 20) / sizeof(int) + 3})
        {
            HPX_TEST(echo_action()(id, pattern(count, 1), count, 1) ==
                pattern(count, 2));
        }
        {
            std::size_t const size = std::size_t(1) << 20;
            buffer_type b(size);
            for (std::size_t i = 0; i != size; ++i)
            {
                b[i] = static_cast<char>('a' + i % 61);
            }
            buffer_type const r = echo_buffer_action()(id, b, size, 'a');
            HPX_TEST_EQ(r.size(), size);
            bool ok = true;
            for (std::size_t i = 0; i != size; ++i)
            {
                ok = ok && r[i] == static_cast<char>('b' + i % 61);
            }
            HPX_TEST(ok);
        }

        // three arguments sent and three results received here, the other
        // way round there
        auto const after = conduit_chunks();
        auto const remote_after = conduit_chunks_action()(id);
        HPX_TEST_EQ(after.first - before.first, std::uint64_t(3));
        HPX_TEST_EQ(after.second - before.second, std::uint64_t(3));
        HPX_TEST_EQ(remote_after.first - remote_before.first, std::uint64_t(3));
        HPX_TEST_EQ(
            remote_after.second - remote_before.second, std::uint64_t(3));
    }

    // many calls in flight, all localities calling all others
    std::vector<hpx::future<void>> busy;
    for (hpx::id_type const& from : hpx::find_all_localities())
    {
        for (hpx::id_type const& to : hpx::find_all_localities())
        {
            if (from != to)
            {
                busy.push_back(hpx::async<echo_many_action>(from, to, 32));
            }
        }
    }
    hpx::wait_all(busy);
    for (auto& f : busy)
    {
        HPX_TEST(!f.has_exception());
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    hpx::init_params params;
    params.cfg = {"hpx.parcel.device_conduit=ucx",
        "hpx.parcel.conduit_host_chunks=1",
        "hpx.parcel.conduit_host_threshold=" + std::to_string(threshold)};
    HPX_TEST_EQ(hpx::init(argc, argv, params), 0);
    return hpx::util::report_errors();
}
#endif
