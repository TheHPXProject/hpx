//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Acceptance check: fan_out_join() reactively joins every peer discovered by a
// prior discover_peers() call from a local registry, reusing registry::join()'s
// existing idempotency/reservation machinery without duplicating it. Run with
// two localities so that both sides fan out concurrently against each other:
// this exercises the "bidirectional join race" scenario that registry::join()'s
// reservation logic (see registry_join.cpp's
// test_registry_join_concurrent_race()) was designed to handle, now triggered
// via fan_out_join() on both localities instead of a single, explicit join()
// call. This test deliberately never exercises a composed discover_and_join()
// entry point (that is a separate, later task).

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/runtime_distributed.hpp>
#include <hpx/modules/supervision.hpp>
#include <hpx/modules/testing.hpp>

#include <hpx/supervision_dispatch/discovery.hpp>
#include <hpx/supervision_dispatch/registry.hpp>
#include <hpx/supervision_dispatch/sentinel.hpp>

#include <chrono>
#include <cstddef>
#include <vector>

namespace {

    // Long enough to comfortably absorb AGAS/peer-startup jitter across two
    // localities booting concurrently; short enough to keep the test fast.
    constexpr std::chrono::milliseconds test_discovery_timeout{5000};
}    // namespace

// ============================================================================
// Test Cases
// ============================================================================

// Both localities register their sentinel/registry names, discover each
// other, and fan out join() calls against every discovered peer. Since both
// localities run this identical sequence concurrently (SPMD), each side ends up
// joining the other's sentinel at roughly the same time -- exercising the
// bidirectional join race -- and must end up with exactly one shadow for the
// peer's sentinel, seeded with event::started. A second fan_out_join() call
// with the very same peer list must be idempotent, returning the same shadow
// ids rather than minting duplicates.
void test_fan_out_join_bidirectional()
{
    std::vector<hpx::id_type> const remote_localities =
        hpx::find_remote_localities();
    if (remote_localities.empty())
    {
        HPX_TEST(false);
        return;    // nothing to join without a second locality
    }

    hpx::supervision::sentinel local_sentinel(hpx::find_here());
    hpx::supervision::registry local_registry(hpx::find_here());

    HPX_TEST(local_sentinel.register_name(hpx::launch::sync));
    HPX_TEST(local_registry.register_name(hpx::launch::sync));

    auto at_exit = hpx::experimental::scope_exit([&]() noexcept {
        hpx::error_code ec1(hpx::throwmode::lightweight);
        local_sentinel.unregister_name(hpx::launch::sync, ec1);
        hpx::error_code ec2(hpx::throwmode::lightweight);
        local_registry.unregister_name(hpx::launch::sync, ec2);
    });

    std::vector<hpx::supervision::discovered_peer> const peers =
        hpx::supervision::discover_peers(test_discovery_timeout);

    HPX_TEST_EQ(peers.size(), remote_localities.size());

    std::vector<hpx::supervision::joined_discovery_result> const joined =
        hpx::supervision::fan_out_join(local_registry, peers);

    HPX_TEST_EQ(joined.size(), peers.size());
    for (hpx::supervision::joined_discovery_result const& j : joined)
    {
        HPX_TEST_NEQ(j.shadow, hpx::supervision::invalid_shadow_id);

        auto const state = hpx::supervision::query_state(j.shadow.get());
        HPX_TEST(state.last_event == hpx::supervision::event::started);
    }

    hpx::distributed::barrier::synchronize();

    // Idempotency: fanning out against the very same peer list again must not
    // create duplicate shadows -- this is exactly the concern raised by two
    // localities concurrently fanning out against each other.
    std::vector<hpx::supervision::joined_discovery_result> const joined_again =
        hpx::supervision::fan_out_join(local_registry, peers);

    HPX_TEST_EQ(joined_again.size(), joined.size());
    if (joined_again.size() == joined.size())
    {
        for (std::size_t i = 0; i != joined.size(); ++i)
        {
            HPX_TEST_EQ(joined_again[i].shadow, joined[i].shadow);
        }
    }
}

// ============================================================================
// Main Test Entry Point
// ============================================================================
int hpx_main()
{
    test_fan_out_join_bidirectional();

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
