//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Smoke test for the OpenSHMEM parcelport: runs remote actions on every other
// locality, with a small and a large (multi-page) payload, and checks the
// results. It then posts fire-and-forget parcels and finalizes right away, so
// parcels are still arriving while the runtime shuts down (this exercises the
// parcelport's shutdown path: late parcels must be dropped, not crash).

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/async.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <cstdint>
#include <numeric>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
std::uint32_t increment(std::uint32_t i)
{
    return i + 1;
}
HPX_PLAIN_ACTION(increment)

std::uint64_t sum_and_locality(std::vector<std::uint64_t> const& data)
{
    return std::accumulate(data.begin(), data.end(), std::uint64_t(0)) +
        hpx::get_locality_id();
}
HPX_PLAIN_ACTION(sum_and_locality)

///////////////////////////////////////////////////////////////////////////////
void test_remote(hpx::id_type const& target, std::uint32_t target_id)
{
    // small payload
    HPX_TEST_EQ(hpx::async<increment_action>(target, 41U).get(), 42U);

    // large payload (several MiB), exercises chunking in the parcelport
    std::vector<std::uint64_t> data(1U << 20);
    std::iota(data.begin(), data.end(), std::uint64_t(0));
    std::uint64_t const expected =
        std::accumulate(data.begin(), data.end(), std::uint64_t(0)) + target_id;
    HPX_TEST_EQ(hpx::async<sum_and_locality_action>(target, data).get(),
        expected);
}

// Parcels that nobody waits for: they may still be in flight when the runtime
// shuts down.
void post_fire_and_forget(hpx::id_type const& target)
{
    for (std::uint32_t i = 0; i != 64; ++i)
        hpx::post<increment_action>(target, i);
}

int hpx_main()
{
    std::vector<hpx::id_type> const localities = hpx::find_all_localities();
    if (localities.size() < 2)
    {
        // without a second locality nothing would be sent: fail the test
        HPX_TEST_MSG(false, "the test needs at least 2 localities");
        return hpx::finalize();
    }

    std::uint32_t const here = hpx::get_locality_id();
    for (std::uint32_t id = 0; id != localities.size(); ++id)
    {
        if (id != here)
            test_remote(localities[id], id);
    }

    for (std::uint32_t id = 0; id != localities.size(); ++id)
    {
        if (id != here)
            post_fire_and_forget(localities[id]);
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(
        hpx::init(argc, argv), 0, "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
#endif
