//  Copyright (c) 2020-2025 Hartmut Kaiser
//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/testing.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace hpx::collectives;

constexpr char const* broadcast_direct_basename =
    "/test/broadcast_hierarchical/";
#if defined(HPX_DEBUG)
constexpr int ITERATIONS = 50;
#else
constexpr int ITERATIONS = 500;
#endif

void test_multiple_use(int arity = 2)
{
    std::uint32_t const here = hpx::get_locality_id();
    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    HPX_TEST_LTE(static_cast<std::uint32_t>(2), num_localities);

    auto const broadcast_clients = create_hierarchical_communicator(
        broadcast_direct_basename, num_sites_arg(num_localities),
        this_site_arg(here), arity_arg(arity), generation_arg(),
        root_site_arg(), flat_fallback_threshold_arg(0));

    hpx::chrono::high_resolution_timer const t;

    // test functionality based on immediate local result value
    for (std::uint32_t i = 0; i != ITERATIONS; ++i)
    {
        if (here == 0)
        {
            hpx::future<std::uint32_t> result =
                broadcast_to(broadcast_clients, i + 42);

            HPX_TEST_EQ(i + 42, result.get());
        }
        else
        {
            hpx::future<std::uint32_t> result =
                hpx::collectives::broadcast_from<std::uint32_t>(
                    broadcast_clients);

            HPX_TEST_EQ(i + 42, result.get());
        }
    }

    auto const elapsed = t.elapsed();
    if (here == 0)
    {
        std::cout << "remote timing: " << elapsed / ITERATIONS << "[s]\n"
                  << std::flush;
    }
}

void test_multiple_use_with_generation(int arity = 2)
{
    std::uint32_t const here = hpx::get_locality_id();
    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    HPX_TEST_LTE(static_cast<std::uint32_t>(2), num_localities);

    auto const broadcast_clients = create_hierarchical_communicator(
        broadcast_direct_basename, num_sites_arg(num_localities),
        this_site_arg(here), arity_arg(arity), generation_arg(),
        root_site_arg(), flat_fallback_threshold_arg(0));

    hpx::chrono::high_resolution_timer const t;

    for (std::uint32_t i = 0; i != ITERATIONS; ++i)
    {
        if (here == 0)
        {
            hpx::future<std::uint32_t> result = broadcast_to(broadcast_clients,
                i + 42, this_site_arg(), generation_arg(i + 1));

            HPX_TEST_EQ(i + 42, result.get());
        }
        else
        {
            hpx::future<std::uint32_t> result =
                hpx::collectives::broadcast_from<std::uint32_t>(
                    broadcast_clients, this_site_arg(), generation_arg(i + 1));

            HPX_TEST_EQ(i + 42, result.get());
        }
    }

    auto const elapsed = t.elapsed();
    if (here == 0)
    {
        std::cout << "remote timing (with generation): " << elapsed / ITERATIONS
                  << "[s]\n"
                  << std::flush;
    }
}

void test_local_use(std::uint32_t num_sites, int arity)
{
    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    // launch num_sites threads to represent different sites
    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            auto const broadcast_clients = create_hierarchical_communicator(
                broadcast_direct_basename, num_sites_arg(num_sites),
                this_site_arg(site), arity_arg(arity), generation_arg(),
                root_site_arg(), flat_fallback_threshold_arg(0));

            hpx::chrono::high_resolution_timer const t;

            for (std::uint32_t i = 0; i != 10 * ITERATIONS; ++i)
            {
                // test functionality based on immediate local result value
                if (site == 0)
                {
                    hpx::future<std::uint32_t> result =
                        broadcast_to(broadcast_clients, 42 + i,
                            this_site_arg(site), generation_arg(i + 1));

                    HPX_TEST_EQ(42 + i, result.get());
                }
                else
                {
                    hpx::future<std::uint32_t> result =
                        hpx::collectives::broadcast_from<std::uint32_t>(
                            broadcast_clients, this_site_arg(site),
                            generation_arg(i + 1));

                    HPX_TEST_EQ(42 + i, result.get());
                }
            }

            auto const elapsed = t.elapsed();
            if (site == 0)
            {
                std::cout << "local timing (" << num_sites << "/" << arity
                          << "): " << elapsed / (10 * ITERATIONS) << "[s]\n"
                          << std::flush;
            }
        }));
    }

    hpx::wait_all(std::move(sites));
}

// Non-power-of-arity coverage. The hierarchical tree construction in
// create_communicator.cpp handles uneven partitioning via the
// division_steps + remainder logic and degenerate single-site leaves.
// This test exercises site counts that are not clean multiples of the
// arity, including cases where recursion produces size-1 subgroups.
void test_non_power_of_arity()
{
    // arity=2 with site counts that force uneven splits and odd-sized
    // subtrees at multiple levels of recursion.
    for (std::uint32_t num_sites : {3u, 5u, 6u, 7u, 9u, 10u, 11u, 15u})
    {
        test_local_use(num_sites, 2);
    }

    // arity=4 with site counts not divisible by 4, exercising top-level
    // partitioning into unequal subtrees.
    for (std::uint32_t num_sites : {5u, 6u, 7u, 9u, 10u, 11u, 13u, 15u})
    {
        test_local_use(num_sites, 4);
    }
}

// The hierarchical tree hardcodes site 0 as the root at every level, so
// broadcast_to (root-side) must reject any caller whose this_site matches a
// non-root communicator, and broadcast_from (non-root-side) must reject
// site 0. Both rejections happen before any communication.
void test_hierarchical_role_rejected()
{
    constexpr char const* basename =
        "/test/broadcast_hierarchical/role_rejections/";

    auto const root_comms = create_hierarchical_communicator(basename,
        num_sites_arg(2), this_site_arg(0), arity_arg(2), generation_arg(),
        root_site_arg(), flat_fallback_threshold_arg(0));
    (void) root_comms.get(0).get_id();

    auto const non_root_comms = create_hierarchical_communicator(basename,
        num_sites_arg(2), this_site_arg(1), arity_arg(2), generation_arg(),
        root_site_arg(), flat_fallback_threshold_arg(0));
    (void) non_root_comms.get(0).get_id();

    bool broadcast_to_rejected = false;
    try
    {
        broadcast_to(hpx::launch::sync, non_root_comms, std::uint32_t(1),
            this_site_arg(1), generation_arg(1));
    }
    catch (hpx::exception const& e)
    {
        broadcast_to_rejected = true;
        HPX_TEST_EQ(e.get_error(), hpx::error::bad_parameter);
    }
    HPX_TEST(broadcast_to_rejected);

    bool broadcast_from_rejected = false;
    try
    {
        hpx::collectives::broadcast_from<std::uint32_t>(
            hpx::launch::sync, root_comms, this_site_arg(0), generation_arg(1));
    }
    catch (hpx::exception const& e)
    {
        broadcast_from_rejected = true;
        HPX_TEST_EQ(e.get_error(), hpx::error::bad_parameter);
    }
    HPX_TEST(broadcast_from_rejected);
}

// The flat basename overload of broadcast_from requires this_site to differ
// from root_site; the check runs synchronously before any communicator is
// created, so this is safe to run on any locality count.
void test_flat_basename_site_equals_root_rejected()
{
    bool rejected = false;
    try
    {
        hpx::collectives::broadcast_from<std::uint32_t>(hpx::launch::sync,
            "/test/broadcast_hierarchical/flat_root_rejected/", this_site_arg(),
            generation_arg(), root_site_arg(0));
    }
    catch (hpx::exception const& e)
    {
        rejected = true;
        HPX_TEST_EQ(e.get_error(), hpx::error::bad_parameter);
    }
    HPX_TEST(rejected);
}

int hpx_main()
{
#if defined(HPX_HAVE_NETWORKING)
    if (hpx::get_num_localities(hpx::launch::sync) > 1)
    {
        test_multiple_use();
        test_multiple_use_with_generation();
    }
#endif

    if (hpx::get_locality_id() == 0)
    {
        for (auto num_localities : {2, 4, 8, 16, 32, 64})
        {
            test_local_use(num_localities, 2);
            if (num_localities >= 4)
            {
                test_local_use(num_localities, 4);
                if (num_localities >= 8)
                {
                    test_local_use(num_localities, 8);
                    if (num_localities >= 16)
                    {
                        test_local_use(num_localities, 16);
                    }
                }
            }
        }

        test_non_power_of_arity();
        test_hierarchical_role_rejected();
        test_flat_basename_site_equals_root_rejected();
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    std::vector<std::string> const cfg = {"hpx.run_hpx_main!=1"};

    hpx::init_params init_args;
    init_args.cfg = cfg;

    HPX_TEST_EQ(hpx::init(argc, argv, init_args), 0);
    return hpx::util::report_errors();
}

#endif
