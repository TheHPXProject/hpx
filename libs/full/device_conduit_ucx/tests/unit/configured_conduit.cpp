//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The device conduit the configuration selects, by default or as set (e.g.
// with HPX_PARCEL_DEVICE_CONDUIT, as a CI may do for all tests): the test
// checks that the runtime uses that conduit (with the default, 'auto', the
// UCX conduit unless HPX uses MPI) and, where host chunks are enabled (the
// default), that host chunks of at least hpx.parcel.conduit_host_threshold
// bytes go through it, with the conduit's statistics and, where HPX has the
// parcelport counters, its performance counters.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/runtime.hpp>
#if defined(HPX_HAVE_MODULE_MPI_BASE)
#include <hpx/modules/mpi_base.hpp>
#endif
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/testing.hpp>
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
#include <hpx/include/performance_counters.hpp>
#endif

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
std::vector<int> echo(std::vector<int> v)
{
    return v;
}
HPX_PLAIN_ACTION(echo)

std::size_t host_threshold()
{
    return std::stoull(
        hpx::get_config_entry("hpx.parcel.conduit_host_threshold", "1048576"));
}

// a vector whose chunk goes through the conduit
std::vector<int> large_vector()
{
    std::vector<int> v(host_threshold() / sizeof(int) + 1);
    std::iota(v.begin(), v.end(), 0);
    return v;
}

// the conduit's statistics on this locality: chunks sent and received
std::pair<std::uint64_t, std::uint64_t> conduit_chunks()
{
    auto const s = hpx::parcelset::get_device_conduit()->statistics();
    return {s.chunks_sent, s.chunks_received};
}
HPX_PLAIN_ACTION(conduit_chunks)

#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
// the conduit's counters on this locality: chunks sent and received
std::pair<std::uint64_t, std::uint64_t> conduit_counters()
{
    std::string const prefix = "/device_conduit{locality#" +
        std::to_string(hpx::get_locality_id()) + "/total}/count/" +
        hpx::parcelset::get_device_conduit()->name();
    hpx::performance_counters::performance_counter sent(prefix + "/sent");
    hpx::performance_counters::performance_counter received(
        prefix + "/received");
    return {sent.get_value<std::uint64_t>(hpx::launch::sync),
        received.get_value<std::uint64_t>(hpx::launch::sync)};
}
HPX_PLAIN_ACTION(conduit_counters)
#endif

// host chunks go to 'locality_id' through the conduit once it has checked
// that it reaches it, which it starts when a message has one (they go
// through the parcelport until then)
bool wait_for_host_chunks(std::uint32_t const locality_id)
{
    auto* conduit = hpx::parcelset::get_device_conduit();
    hpx::id_type const id = hpx::naming::get_id_from_locality_id(locality_id);
    std::vector<int> const v = large_vector();
    auto const deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (!conduit->host_chunks_ready(locality_id))
    {
        if (std::chrono::steady_clock::now() >= deadline)
        {
            return false;
        }
        HPX_TEST(echo_action()(id, v) == v);
        hpx::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}
HPX_PLAIN_ACTION(wait_for_host_chunks)

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    std::string const configured =
        hpx::get_config_entry("hpx.parcel.device_conduit", "auto");
    auto const* conduit = hpx::parcelset::get_device_conduit();
    if (!hpx::is_networking_enabled())
    {
        // one locality without networking: no conduit
        HPX_TEST(conduit == nullptr);
        return hpx::finalize();
    }
    if (configured == "none")
    {
        HPX_TEST(conduit == nullptr);
        return hpx::finalize();
    }

    std::string expected = configured;
    if (configured == "auto")
    {
        // the UCX conduit (compiled in, as this test is) unless HPX uses MPI
        expected = "ucx";
#if defined(HPX_HAVE_MODULE_MPI_BASE)
        if (hpx::util::mpi_environment::enabled())
        {
            HPX_TEST(conduit == nullptr);
            return hpx::finalize();
        }
#endif
    }
    HPX_TEST(conduit != nullptr);
    if (conduit == nullptr)
    {
        return hpx::finalize();
    }
    HPX_TEST_EQ(conduit->name(), expected);

    // host chunks through the conduit, for the parcelport in use
    std::string const parcelport =
        hpx::get_config_entry("hpx.parcel.bootstrap", "tcp");
    if (!conduit->moves_host_memory() ||
        hpx::get_config_entry(
            "hpx.parcel." + parcelport + ".conduit_host_chunks", "0") == "0")
    {
        std::cout << "the device conduit moves no host chunks here, no data "
                     "to check\n";
        return hpx::finalize();
    }

    std::vector<int> const v = large_vector();
    for (hpx::id_type const& id : hpx::find_remote_localities())
    {
        HPX_TEST(
            wait_for_host_chunks(hpx::naming::get_locality_id_from_id(id)));
        HPX_TEST(wait_for_host_chunks_action()(id, hpx::get_locality_id()));

        // the argument and the result go through the conduit
        auto const before = conduit_chunks();
        auto const remote_before = conduit_chunks_action()(id);
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
        auto const counters_before = conduit_counters();
        auto const remote_counters_before = conduit_counters_action()(id);
#endif
        HPX_TEST(echo_action()(id, v) == v);

        auto const after = conduit_chunks();
        auto const remote_after = conduit_chunks_action()(id);
        HPX_TEST_EQ(after.first - before.first, std::uint64_t(1));
        HPX_TEST_EQ(after.second - before.second, std::uint64_t(1));
        HPX_TEST_EQ(remote_after.first - remote_before.first, std::uint64_t(1));
        HPX_TEST_EQ(
            remote_after.second - remote_before.second, std::uint64_t(1));
#if defined(HPX_HAVE_PARCELPORT_COUNTERS)
        auto const counters_after = conduit_counters();
        auto const remote_counters_after = conduit_counters_action()(id);
        HPX_TEST_EQ(
            counters_after.first - counters_before.first, std::uint64_t(1));
        HPX_TEST_EQ(
            counters_after.second - counters_before.second, std::uint64_t(1));
        HPX_TEST_EQ(remote_counters_after.first - remote_counters_before.first,
            std::uint64_t(1));
        HPX_TEST_EQ(
            remote_counters_after.second - remote_counters_before.second,
            std::uint64_t(1));
#endif
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::init(argc, argv), 0);
    return hpx::util::report_errors();
}
#endif
