//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Acceptance check: discover_and_join() is the single composed entry point
// deferred by fan_out_join.cpp's test_fan_out_join_bidirectional() -- it
// performs exactly one discover_peers() pull followed by one fan_out_join()
// pass, with no polling thread, timer, or repeated broadcast of its own. Run
// with two localities so both sides invoke discover_and_join() concurrently
// (SPMD), exercising the same bidirectional join race as
// test_fan_out_join_bidirectional(), but now driven entirely through the
// composed API instead of its two constituent calls.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/errors.hpp>
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

// Both localities register their sentinel/registry names, then call *only*
// discover_and_join() -- no explicit discover_peers()/fan_out_join() calls.
// Since both sides run this identical sequence concurrently, each ends up
// owning exactly one shadow of the peer's sentinel, seeded with event::started.
// A second discover_and_join() invocation must be idempotent, returning the
// same shadow ids rather than minting duplicates or re-running any
// discovery/broadcast activity.
void test_discover_and_join_bidirectional()
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

    // Only the composed entry point is exercised here -- no direct
    // discover_peers()/fan_out_join() calls.
    std::vector<hpx::supervision::shadow_id> const shadows =
        hpx::supervision::discover_and_join(
            local_registry, test_discovery_timeout);

    HPX_TEST_EQ(shadows.size(), remote_localities.size());
    for (hpx::supervision::shadow_id const& shadow : shadows)
    {
        HPX_TEST_NEQ(shadow, hpx::supervision::invalid_shadow_id);

        auto const state = hpx::supervision::query_state(shadow.get());
        HPX_TEST(state.last_event == hpx::supervision::event::started);
    }

    hpx::distributed::barrier::synchronize();

    // Idempotency: a second discover_and_join() call must not create duplicate
    // shadows, mirroring fan_out_join()'s own idempotency guarantee -- this is
    // exactly the concern raised by two localities concurrently discovering and
    // joining each other.
    std::vector<hpx::supervision::shadow_id> const shadows_again =
        hpx::supervision::discover_and_join(
            local_registry, test_discovery_timeout);

    HPX_TEST_EQ(shadows_again.size(), shadows.size());
    if (shadows_again.size() == shadows.size())
    {
        for (std::size_t i = 0; i != shadows.size(); ++i)
        {
            HPX_TEST_EQ(shadows_again[i], shadows[i]);
        }
    }

    // Keep both localities' names registered until every participant has
    // finished its second discovery pass.
    hpx::distributed::barrier::synchronize();
}

// ============================================================================
// Main Test Entry Point
// ============================================================================
int hpx_main()
{
    test_discover_and_join_bidirectional();

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
