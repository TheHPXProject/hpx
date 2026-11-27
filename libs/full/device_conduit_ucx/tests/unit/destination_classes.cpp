//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Host chunks through the UCX device conduit by destination class: the
// parcelport's settings hpx.parcel.<parcelport>.same_node.conduit_host_chunks
// and .other_nodes.conduit_host_chunks decide for destinations on the
// sender's node and on other nodes (each locality publishes its node with its
// endpoints). The test runs its localities on one node, so they all see each
// other as on the same node: run with --same-node-conduit=1 (same_node on,
// other_nodes off) large host chunks go through the conduit, with
// --same-node-conduit=0 (the other way round) through the parcelport. The
// threshold for the same node (hpx.parcel.same_node.conduit_host_threshold,
// 1 MiB here) applies instead of the plain one (64 KiB here).

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/naming_base.hpp>
#include <hpx/modules/parcelset.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/testing.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

constexpr std::size_t threshold = 65536;
constexpr std::size_t same_node_threshold = std::size_t(1) << 20;

///////////////////////////////////////////////////////////////////////////////
std::vector<int> pattern(std::size_t const count, int const first)
{
    std::vector<int> v(count);
    std::iota(v.begin(), v.end(), first);
    return v;
}

std::vector<int> echo(std::vector<int> const& v, std::size_t count, int first)
{
    HPX_TEST(v == pattern(count, first));
    return pattern(count, first + 1);
}
HPX_PLAIN_ACTION(echo)

// the conduit's statistics on this locality: chunks sent and received
std::pair<std::uint64_t, std::uint64_t> conduit_chunks()
{
    auto const s = hpx::parcelset::get_device_conduit()->statistics();
    return {s.chunks_sent, s.chunks_received};
}
HPX_PLAIN_ACTION(conduit_chunks)

std::uint64_t node_identity()
{
    return hpx::parcelset::this_node_identity();
}
HPX_PLAIN_ACTION(node_identity)

// host chunks go to 'locality_id' through the conduit once it has checked
// that it reaches it, which it starts when a message has one
bool wait_for_host_chunks(std::uint32_t const locality_id)
{
    auto* conduit = hpx::parcelset::get_device_conduit();
    hpx::id_type const id = hpx::naming::get_id_from_locality_id(locality_id);
    std::size_t const count = same_node_threshold / sizeof(int);
    auto const deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (!conduit->host_chunks_ready(locality_id))
    {
        if (std::chrono::steady_clock::now() >= deadline)
        {
            return false;
        }
        HPX_TEST(echo_action()(id, pattern(count, 9), count, 9) ==
            pattern(count, 10));
    }
    return true;
}
HPX_PLAIN_ACTION(wait_for_host_chunks)

///////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    bool const expect_conduit = vm["same-node-conduit"].as<int>() != 0;

    auto const* conduit = hpx::parcelset::get_device_conduit();
    HPX_TEST(conduit != nullptr);
    if (conduit == nullptr)
    {
        return hpx::finalize();
    }
    HPX_TEST(conduit->moves_host_memory());

    std::uint64_t const here = hpx::parcelset::this_node_identity();
    HPX_TEST_NEQ(here, std::uint64_t(0));

    for (hpx::id_type const& id : hpx::find_remote_localities())
    {
        // on one node, the same identity
        HPX_TEST_EQ(node_identity_action()(id), here);

        std::uint32_t const locality_id =
            hpx::naming::get_locality_id_from_id(id);
        if (expect_conduit)
        {
            HPX_TEST(wait_for_host_chunks(locality_id));
            HPX_TEST(wait_for_host_chunks_action()(id, hpx::get_locality_id()));
        }

        // above the plain threshold, below the one for the same node:
        // through the parcelport
        {
            auto const before = conduit_chunks();
            std::size_t const count = 4 * threshold / sizeof(int);
            HPX_TEST(echo_action()(id, pattern(count, 3), count, 3) ==
                pattern(count, 4));
            HPX_TEST(conduit_chunks() == before);
        }

        // a large argument and result: through the conduit only if the
        // same-node settings say so
        auto const before = conduit_chunks();
        auto const remote_before = conduit_chunks_action()(id);
        std::size_t const count = (std::size_t(4) << 20) / sizeof(int);
        HPX_TEST(echo_action()(id, pattern(count, 1), count, 1) ==
            pattern(count, 2));
        auto const after = conduit_chunks();
        auto const remote_after = conduit_chunks_action()(id);

        std::uint64_t const expected = expect_conduit ? 1 : 0;
        HPX_TEST_EQ(after.first - before.first, expected);
        HPX_TEST_EQ(after.second - before.second, expected);
        HPX_TEST_EQ(remote_after.first - remote_before.first, expected);
        HPX_TEST_EQ(remote_after.second - remote_before.second, expected);
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    hpx::program_options::options_description desc(
        "usage: " HPX_APPLICATION_STRING " [options]");
    desc.add_options()("same-node-conduit",
        hpx::program_options::value<int>()->default_value(1),
        "1: host chunks through the conduit to destinations on this node, "
        "not to other nodes; 0: the other way round");

    // the options of all parcelports the test may run with, before the
    // command line is known: both settings given for each
    int same_node = 1;
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--same-node-conduit=0")
        {
            same_node = 0;
        }
    }
    std::string const same = std::to_string(same_node);
    std::string const other = std::to_string(1 - same_node);

    hpx::init_params params;
    params.desc_cmdline = desc;
    params.cfg = {"hpx.parcel.device_conduit=ucx",
        "hpx.parcel.conduit_host_threshold=" + std::to_string(threshold),
        "hpx.parcel.same_node.conduit_host_threshold=" +
            std::to_string(same_node_threshold)};
    for (char const* pp : {
#if defined(HPX_HAVE_PARCELPORT_TCP)
             "tcp",
#endif
#if defined(HPX_HAVE_PARCELPORT_MPI)
             "mpi",
#endif
#if defined(HPX_HAVE_PARCELPORT_LCI)
             "lci",
#endif
#if defined(HPX_HAVE_PARCELPORT_LCW)
             "lcw",
#endif
#if defined(HPX_HAVE_PARCELPORT_GASNET)
             "gasnet",
#endif
             ""})
    {
        if (*pp == '\0')
        {
            continue;
        }
        std::string const key = std::string("hpx.parcel.") + pp;
        params.cfg.push_back(key + ".same_node.conduit_host_chunks=" + same);
        params.cfg.push_back(key + ".other_nodes.conduit_host_chunks=" + other);
    }
    HPX_TEST_EQ(hpx::init(argc, argv, params), 0);
    return hpx::util::report_errors();
}
#endif
