//  Copyright (c) 2026 TheHPXProject
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The nodes of a two-node measurement in the fingerprint, also where the
// tool runs on the node of the measurement's second locality (the V100 runs
// of the tuner repository's chapter 07: the tool on diablo, the localities on
// bahram and diablo) or on a node of its own.

#include <hpx/config.hpp>
#include <hpx/modules/parcel_tuning.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/tuning.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace tuning = hpx::tuning;

namespace {

    tuning::locality_entries node(
        std::string const& name, std::string const& cores)
    {
        return {{"host_name", name}, {"cpu_model", "Xeon Gold 6148"},
            {"cores", cores}};
    }

    tuning::host_fingerprint tool_on(
        std::string const& name, std::string const& cores)
    {
        tuning::host_fingerprint f;
        f.add("host_name", name, false);
        f.add("cpu_model", "Xeon Gold 6148");
        f.add("cores", cores);
        return f;
    }

    std::string value(tuning::host_fingerprint const& f, char const* key)
    {
        auto const* e = f.find(key);
        return e != nullptr ? e->value : std::string("<none>");
    }

    // the tool on locality 0's node: locality 1 is the peer
    void test_tool_on_locality_0()
    {
        auto f = tool_on("medusa14", "40");
        HPX_TEST_EQ(tuning::add_peer_nodes(
                        f, {node("medusa14", "40"), node("medusa15", "40")}),
            std::size_t(1));
        HPX_TEST_EQ(value(f, "parcel_peer_host_name"), std::string("medusa15"));
        HPX_TEST(f.find("parcel_peer_host_name")->compared == false);
        HPX_TEST(f.find("parcel_peer_cores") == nullptr);
        HPX_TEST(f.find("parcel_peer_cpu_model") == nullptr);
    }

    // the tool on locality 1's node: locality 0 is the peer
    void test_tool_on_locality_1()
    {
        auto f = tool_on("diablo", "40");
        HPX_TEST_EQ(tuning::add_peer_nodes(
                        f, {node("bahram", "20"), node("diablo", "40")}),
            std::size_t(1));
        HPX_TEST_EQ(value(f, "parcel_peer_host_name"), std::string("bahram"));
        HPX_TEST_EQ(value(f, "parcel_peer_cores"), std::string("20"));
        HPX_TEST(f.find("parcel_peer_cores")->compared);
        HPX_TEST(f.find("parcel_peer_cpu_model") == nullptr);
        HPX_TEST_EQ(value(f, "host_name"), std::string("diablo"));
    }

    // the tool on a node of its own: both localities' nodes
    void test_tool_elsewhere()
    {
        auto f = tool_on("login", "8");
        HPX_TEST_EQ(tuning::add_peer_nodes(
                        f, {node("bahram", "20"), node("diablo", "40")}),
            std::size_t(2));
        HPX_TEST_EQ(value(f, "parcel_peer_host_name"), std::string("bahram"));
        HPX_TEST_EQ(value(f, "parcel_peer_cores"), std::string("20"));
        HPX_TEST_EQ(value(f, "parcel_peer2_host_name"), std::string("diablo"));
        HPX_TEST_EQ(value(f, "parcel_peer2_cores"), std::string("40"));
    }

    // the parcel area's own entries keep one "parcel_" (the V100 pair: the
    // tool on diablo at FDR, bahram at QDR)
    void test_parcel_keys()
    {
        auto f = tool_on("diablo", "40");
        f.add("parcel_network", "mlx4_0 MT4099 56 Gb/sec (4X FDR)");
        auto bahram = node("bahram", "20");
        bahram.emplace_back(
            "parcel_network", "mlx4_0 MT4099 40 Gb/sec (4X QDR)");
        auto diablo = node("diablo", "40");
        diablo.emplace_back(
            "parcel_network", "mlx4_0 MT4099 56 Gb/sec (4X FDR)");
        HPX_TEST_EQ(
            tuning::add_peer_nodes(f, {bahram, diablo}), std::size_t(1));
        HPX_TEST_EQ(value(f, "parcel_peer_network"),
            std::string("mlx4_0 MT4099 40 Gb/sec (4X QDR)"));
        HPX_TEST(f.find("parcel_peer_parcel_network") == nullptr);
    }

    // the network of this node: whatever it is, no line breaks, and the
    // same text twice
    void test_describe_network()
    {
        std::string const n = tuning::describe_network();
        HPX_TEST(n.find('\n') == std::string::npos);
        HPX_TEST_EQ(tuning::describe_network(), n);
    }

    // both localities on the tool's node (the same-node placement): nothing
    void test_same_node()
    {
        auto f = tool_on("medusa14", "40");
        std::size_t const before = f.entries.size();
        HPX_TEST_EQ(tuning::add_peer_nodes(
                        f, {node("medusa14", "40"), node("medusa14", "40")}),
            std::size_t(0));
        HPX_TEST_EQ(f.entries.size(), before);
    }
}    // namespace

int main()
{
    test_tool_on_locality_0();
    test_tool_on_locality_1();
    test_tool_elsewhere();
    test_same_node();
    test_parcel_keys();
    test_describe_network();
    return hpx::util::report_errors();
}
