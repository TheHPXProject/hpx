//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Acceptance check: a standalone test/snippet can construct a sentinel client
// on a locality and call start(), resulting in a call to publish_event with
// event::started at epoch 0 -- with no compile/link errors and no registry
// dependency required. This test only exercises the sentinel client in
// isolation; it deliberately never constructs or refers to
// hpx::supervision::registry.

#include <hpx/hpx.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/modules/supervision.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/supervision_dispatch/sentinel.hpp>

// ============================================================================
// Test Cases
// ============================================================================

// Constructing a sentinel on a locality and calling the asynchronous start()
// overload must complete without throwing and must publish the `started`
// event at epoch 0 -- verified indirectly by the returned publish_result
// (which is `applied` since this is a freshly-created target that has never
// published any event before).
void test_sentinel_start_async()
{
    hpx::supervision::sentinel const s(hpx::find_here());

    hpx::future<hpx::supervision::publish_result> f = s.start();
    hpx::supervision::publish_result const result = f.get();

    HPX_TEST(result == hpx::supervision::publish_result::applied);
}

// Same as above, but using the blocking (hpx::launch::sync) overload of
// start().
void test_sentinel_start_sync()
{
    hpx::supervision::sentinel const s(hpx::find_here());

    hpx::supervision::publish_result const result = s.start(hpx::launch::sync);

    HPX_TEST(result == hpx::supervision::publish_result::applied);
}

// ============================================================================
// Main Test Entry Point
// ============================================================================
int hpx_main()
{
    test_sentinel_start_async();
    test_sentinel_start_sync();

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
