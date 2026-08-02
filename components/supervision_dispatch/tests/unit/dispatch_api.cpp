//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/modules/agas.hpp>
#include <hpx/modules/async_combinators.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/testing.hpp>

#include <hpx/supervision_dispatch/dispatch_api.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

namespace {

    // Short enough to keep the "never registers" case fast, long enough to
    // comfortably absorb AGAS/peer-startup jitter for the "already registered"
    // case.
    constexpr std::chrono::seconds test_discovery_timeout{2};
}    // namespace

// Scenario 1: no side effects before init.
//
// finalize() must be a documented no-op whenever the lifecycle state is
// anything other than `active` -- in particular, calling it before any init()
// call has ever run (state == uninitialized) must not publish any event,
// unregister any symbol name, or otherwise touch component state. Calling it
// repeatedly in that state must remain equally inert.
void test_no_side_effects_before_init()
{
    // Precondition: nothing has been initialized yet on this locality.
    HPX_TEST(!hpx::supervision::is_initialized());

    // A single finalize-before-init call must be a harmless no-op...
    hpx::supervision::finalize();
    HPX_TEST(!hpx::supervision::is_initialized());

    // ...and so must repeat calls in the same uninitialized state, with no
    // stateful side effect accumulating across calls.
    hpx::supervision::finalize();
    hpx::supervision::finalize();
    HPX_TEST(!hpx::supervision::is_initialized());
}

// Scenario 2: concurrent-init idempotency.
//
// N racing init() calls must all resolve to `true`, but the underlying
// uninitialized -> initializing CAS guarantees exactly one caller actually runs
// the create-sentinel/create-registry/start/register/ discover_and_join()
// sequence; every other racer either observes `active` immediately or attaches
// to that single winner's in-flight future. No duplicate sentinel/registry pair
// should ever be created.
void test_concurrent_init_idempotency()
{
    HPX_TEST(!hpx::supervision::is_initialized());

    constexpr std::size_t num_racers = 16;
    std::vector<hpx::future<void>> results;
    results.reserve(num_racers);

    // Launch all racers as close together as possible to maximize the chance of
    // actually exercising the CAS race rather than serializing trivially
    // through hpx::async's scheduling.
    for (std::size_t i = 0; i != num_racers; ++i)
    {
        results.push_back(hpx::async([]() {
            return hpx::supervision::init(
                hpx::launch::sync, test_discovery_timeout);
        }));
    }

    // Every racer must observe success -- there is no legitimate reason for a
    // losing racer to see anything but `true` when the winner succeeds.
    hpx::wait_all_nothrow(results);

    for (auto& f : results)
    {
        HPX_TEST_NO_THROW(f.get());
    }

    // After all racers settle, the lifecycle must have landed in exactly one
    // place: active.
    HPX_TEST(hpx::supervision::is_initialized());

    // Clean up so later tests in this process start from a known state.
    hpx::supervision::finalize();
    HPX_TEST(!hpx::supervision::is_initialized());
}

// Scenario 3: concurrent-finalize idempotency.
//
// Mirrors the concurrent-init test on the teardown side: once a single init()
// call has succeeded, N racing finalize() calls must all complete without
// error, with the active -> finalizing CAS guaranteeing exactly one caller
// actually publishes event::completed, unregisters symbol names, and releases
// component ownership; every other racer's call must be the documented no-op.
void test_concurrent_finalize_idempotency()
{
    // Establish the active state that finalize() requires to do
    // real work at all.
    HPX_TEST_NO_THROW(
        hpx::supervision::init(hpx::launch::sync, test_discovery_timeout));
    HPX_TEST(hpx::supervision::is_initialized());

    constexpr std::size_t num_racers = 16;
    std::vector<hpx::future<void>> results;
    results.reserve(num_racers);

    for (std::size_t i = 0; i != num_racers; ++i)
    {
        results.push_back(hpx::async([]() { hpx::supervision::finalize(); }));
    }

    // No racer should throw: exactly one performs real teardown, the rest
    // observe a non-active state and no-op.
    hpx::wait_all_nothrow(results);

    for (auto& f : results)
    {
        HPX_TEST_NO_THROW(f.get());
    }

    // Regardless of how many racers "won" vs. no-op'd, the end state must be
    // uninitialized exactly once, not toggled back and forth.
    HPX_TEST(!hpx::supervision::is_initialized());
}

// Scenario 4: full init -> finalize -> init cycle (re-armability).
//
// Verifies the lifecycle can be run through more than one complete cycle: after
// finalize() resets state to uninitialized, a subsequent init() call must
// succeed again from scratch (fresh sentinel/registry pair, fresh started
// publication at the next epoch) -- this is the property that makes the
// epoch-increment-on-init fix (rather than on finalize) necessary, since
// reusing the finalize epoch for the next started publication would otherwise
// be silently dropped as stale/no-op.
void test_reinit_after_finalize()
{
    HPX_TEST(!hpx::supervision::is_initialized());

    // First cycle.
    HPX_TEST_NO_THROW(
        hpx::supervision::init(hpx::launch::sync, test_discovery_timeout));
    HPX_TEST(hpx::supervision::is_initialized());

    hpx::supervision::finalize();
    HPX_TEST(!hpx::supervision::is_initialized());

    // Second cycle: must succeed just as cleanly as the first, confirming
    // finalize() left the lifecycle fully re-armable (no stale sentinel_/
    // registry_ pointers, no epoch collision blocking the new started
    // publication).
    HPX_TEST_NO_THROW(
        hpx::supervision::init(hpx::launch::sync, test_discovery_timeout));
    HPX_TEST(hpx::supervision::is_initialized());

    hpx::supervision::finalize();
    HPX_TEST(!hpx::supervision::is_initialized());
}

int hpx_main()
{
    test_no_side_effects_before_init();
    test_concurrent_init_idempotency();
    test_concurrent_finalize_idempotency();
    test_reinit_after_finalize();

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
