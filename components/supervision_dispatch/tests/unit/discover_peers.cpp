//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Acceptance check: discover_peers() resolves the supervision_dispatch sentinel
// and registry names (see register_name()) of remote localities that have
// already registered them, bounded by a single hpx::wait_all_for_nothrow()
// call, and excludes localities that never do so -- without hanging
// indefinitely. This test only exercises the one-time discovery pull itself;
// the resulting peers are never join()ed here (see registry_join.cpp for that,
// exercised with explicit manually-supplied peers instead).

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/supervision.hpp>
#include <hpx/modules/testing.hpp>

#include <hpx/supervision_dispatch/discovery.hpp>
#include <hpx/supervision_dispatch/registry.hpp>
#include <hpx/supervision_dispatch/sentinel.hpp>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

    // Short enough to keep the "never registers" case fast, long enough to
    // comfortably absorb AGAS/peer-startup jitter for the "already registered"
    // case.
    constexpr std::chrono::seconds test_discovery_timeout{2};
}    // namespace

// ============================================================================
// Test Cases
// ============================================================================

// A remote locality that never registers its sentinel/registry names must
// be excluded from the result, and discover_peers() must still return within
// roughly the given timeout rather than hanging indefinitely -- constructing a
// sentinel/registry client from a symbolic name never fails fast on an
// unregistered symbol, so the bound has to come from
// discover_peers()/wait_all_for() instead.
void test_discover_peers_excludes_unregistered_peer()
{
    std::vector<hpx::id_type> const remote_localities =
        hpx::find_remote_localities();
    if (remote_localities.empty())
    {
        HPX_TEST(false);
        return;    // nothing to discover without a second locality
    }

    hpx::id_type const& target_locality = remote_localities[0];

    auto const start = std::chrono::steady_clock::now();
    std::vector<hpx::supervision::discovered_peer> const peers =
        hpx::supervision::discover_peers(test_discovery_timeout);

    HPX_TEST_EQ(peers.size(), static_cast<std::size_t>(0));

    auto const elapsed = std::chrono::steady_clock::now() - start;

    // Bounded, not exact: must not hang well past the given timeout.
    HPX_TEST(elapsed < test_discovery_timeout * 2);

    for (auto const& peer : peers)
    {
        HPX_TEST_NEQ(peer.locality, target_locality);
    }
}

// If a remote locality has already registered its sentinel/registry names
// (see register_name()) before discover_peers() is called, that locality
// must be included in the result well inside the given timeout.
void test_discover_peers_finds_registered_peer()
{
    std::vector<hpx::id_type> const remote_localities =
        hpx::find_remote_localities();
    if (remote_localities.empty())
    {
        HPX_TEST(false);
        return;
    }

    hpx::id_type const& target_locality = remote_localities[0];

    hpx::supervision::sentinel s(target_locality);
    HPX_TEST(s.register_name(hpx::launch::sync));

    hpx::supervision::registry r(target_locality);
    HPX_TEST(r.register_name(hpx::launch::sync));

    auto const start = std::chrono::steady_clock::now();
    std::vector<hpx::supervision::discovered_peer> const peers =
        hpx::supervision::discover_peers(test_discovery_timeout);

    HPX_TEST_EQ(peers.size(), remote_localities.size());

    auto const elapsed = std::chrono::steady_clock::now() - start;

    HPX_TEST(elapsed < test_discovery_timeout);

    bool found = false;
    for (auto const& [locality, sentinel_client, registry_client, epoch] :
        peers)
    {
        // epoch should default to unjoined_epoch since discover_peers() never
        // calls join()
        HPX_TEST_EQ(epoch, hpx::supervision::unjoined_epoch);
        if (locality == target_locality)
        {
            found = true;
            HPX_TEST(sentinel_client.is_ready());
            HPX_TEST(registry_client.is_ready());
        }
    }
    HPX_TEST(found);

    s.unregister_name(hpx::launch::sync);
    r.unregister_name(hpx::launch::sync);
}

// ============================================================================
// Main Test Entry Point
// ============================================================================
int hpx_main()
{
    test_discover_peers_excludes_unregistered_peer();

    // A fast locality can register names in the second test while another is
    // still running the unregistered-peer assertion, making the first test
    // flaky. This barrier prevents this from happening
    hpx::distributed::barrier::synchronize();

    test_discover_peers_finds_registered_peer();

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::init(argc, argv), 0);
    return hpx::util::report_errors();
}

#else

int main(int, char*[])
{
    return 0;
}

#endif
