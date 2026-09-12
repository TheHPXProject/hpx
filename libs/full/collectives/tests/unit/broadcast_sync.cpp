//  Copyright (c) 2020-2025 Hartmut Kaiser
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

// Keep independently created communicators from aliasing in AGAS while
// localities transition between test phases.
constexpr char const* broadcast_direct_basename = "/test/broadcast_direct/";
constexpr char const* broadcast_direct_multiple_use_basename =
    "/test/broadcast_direct/multiple_use/";
constexpr char const* broadcast_direct_explicit_generation_basename =
    "/test/broadcast_direct/explicit_generation/";
constexpr char const* broadcast_direct_explicit_generation_sync_basename =
    "/test/broadcast_direct/explicit_generation_sync/";
constexpr char const* broadcast_direct_local_basename =
    "/test/broadcast_direct/local/";
constexpr char const* broadcast_direct_local_sync_basename =
    "/test/broadcast_direct/local_sync/";
#if defined(HPX_DEBUG)
constexpr int ITERATIONS = 100;
#else
constexpr int ITERATIONS = 1000;
#endif

void test_one_shot_use()
{
    std::uint32_t const here = hpx::get_locality_id();
    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    HPX_TEST_LTE(static_cast<std::uint32_t>(2), num_localities);

    // test functionality based on immediate local result value
    for (std::uint32_t i = 0; i != ITERATIONS; ++i)
    {
        if (here == 0)
        {
            std::uint32_t result =
                broadcast_to(hpx::launch::sync, broadcast_direct_basename,
                    i + 42, num_sites_arg(num_localities), this_site_arg(here),
                    generation_arg(i + 1));

            HPX_TEST_EQ(i + 42, result);
        }
        else
        {
            std::uint32_t result = broadcast_from<std::uint32_t>(
                hpx::launch::sync, broadcast_direct_basename,
                this_site_arg(here), generation_arg(i + 1));

            HPX_TEST_EQ(i + 42, result);
        }
    }
}

void test_multiple_use()
{
    std::uint32_t const here = hpx::get_locality_id();
    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    HPX_TEST_LTE(static_cast<std::uint32_t>(2), num_localities);

    auto const broadcast_direct_client =
        create_communicator(broadcast_direct_multiple_use_basename,
            num_sites_arg(num_localities), this_site_arg(here));

    // test functionality based on immediate local result value
    for (std::uint32_t i = 0; i != ITERATIONS; ++i)
    {
        if (here == 0)
        {
            std::uint32_t result = broadcast_to(
                hpx::launch::sync, broadcast_direct_client, i + 42);

            HPX_TEST_EQ(i + 42, result);
        }
        else
        {
            std::uint32_t result = broadcast_from<std::uint32_t>(
                hpx::launch::sync, broadcast_direct_client);

            HPX_TEST_EQ(i + 42, result);
        }
    }
}

void test_multiple_use_with_generation()
{
    std::uint32_t const here = hpx::get_locality_id();
    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    HPX_TEST_LTE(static_cast<std::uint32_t>(2), num_localities);

    auto const broadcast_direct_client =
        create_communicator(broadcast_direct_explicit_generation_basename,
            num_sites_arg(num_localities), this_site_arg(here));

    hpx::chrono::high_resolution_timer const t;

    for (std::uint32_t i = 0; i != ITERATIONS; ++i)
    {
        if (here == 0)
        {
            std::uint32_t result = broadcast_to(hpx::launch::sync,
                broadcast_direct_client, i + 42, generation_arg(i + 1));

            HPX_TEST_EQ(i + 42, result);
        }
        else
        {
            std::uint32_t result =
                broadcast_from<std::uint32_t>(hpx::launch::sync,
                    broadcast_direct_client, generation_arg(i + 1));

            HPX_TEST_EQ(i + 42, result);
        }
    }

    auto const elapsed = t.elapsed();
    if (here == 0)
    {
        std::cout << "remote timing: " << elapsed / ITERATIONS << "[s]\n";
    }
}

void test_multiple_use_with_generation_sync()
{
    std::uint32_t const here = hpx::get_locality_id();
    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    HPX_TEST_LTE(static_cast<std::uint32_t>(2), num_localities);

    auto const broadcast_direct_client =
        create_communicator(broadcast_direct_explicit_generation_sync_basename,
            num_sites_arg(num_localities), this_site_arg(here));

    hpx::chrono::high_resolution_timer const t;

    for (std::uint32_t i = 0; i != ITERATIONS; ++i)
    {
        if (here == 0)
        {
            std::uint32_t result = i + 42;
            broadcast(broadcast_direct_client, result, this_site_arg(),
                generation_arg(i + 1));

            HPX_TEST_EQ(i + 42, result);
        }
        else
        {
            std::uint32_t result = -1;
            broadcast(broadcast_direct_client, result, this_site_arg(),
                generation_arg(i + 1));

            HPX_TEST_EQ(i + 42, result);
        }
    }

    auto const elapsed = t.elapsed();
    if (here == 0)
    {
        std::cout << "remote timing: " << elapsed / ITERATIONS << "[s]\n";
    }
}

void test_local_use()
{
    constexpr std::uint32_t num_sites = 10;

    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    // launch num_sites threads to represent different sites
    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            auto const broadcast_direct_client =
                create_communicator(broadcast_direct_local_basename,
                    num_sites_arg(num_sites), this_site_arg(site));

            hpx::chrono::high_resolution_timer const t;

            for (std::uint32_t i = 0; i != 10 * ITERATIONS; ++i)
            {
                // test functionality based on immediate local result value
                if (site == 0)
                {
                    std::uint32_t result =
                        broadcast_to(hpx::launch::sync, broadcast_direct_client,
                            42 + i, this_site_arg(site), generation_arg(i + 1));

                    HPX_TEST_EQ(42 + i, result);
                }
                else
                {
                    std::uint32_t result = broadcast_from<std::uint32_t>(
                        hpx::launch::sync, broadcast_direct_client,
                        this_site_arg(site), generation_arg(i + 1));

                    HPX_TEST_EQ(42 + i, result);
                }
            }

            auto const elapsed = t.elapsed();
            if (site == 0)
            {
                std::cout << "local timing: " << elapsed / (10 * ITERATIONS)
                          << "[s]\n";
            }
        }));
    }

    hpx::wait_all(std::move(sites));
}

void test_local_use_sync()
{
    constexpr std::uint32_t num_sites = 10;

    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    // launch num_sites threads to represent different sites
    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            auto const broadcast_direct_client =
                create_communicator(broadcast_direct_local_sync_basename,
                    num_sites_arg(num_sites), this_site_arg(site));

            hpx::chrono::high_resolution_timer const t;

            for (std::uint32_t i = 0; i != 10 * ITERATIONS; ++i)
            {
                // test functionality based on immediate local result value
                if (site == 0)
                {
                    std::uint32_t result = 42 + i;
                    broadcast(broadcast_direct_client, result,
                        this_site_arg(site), generation_arg(i + 1));

                    HPX_TEST_EQ(42 + i, result);
                }
                else
                {
                    std::uint32_t result = -1;
                    broadcast(broadcast_direct_client, result,
                        this_site_arg(site), generation_arg(i + 1));

                    HPX_TEST_EQ(42 + i, result);
                }
            }

            auto const elapsed = t.elapsed();
            if (site == 0)
            {
                std::cout << "local timing: " << elapsed / (10 * ITERATIONS)
                          << "[s]\n";
            }
        }));
    }

    hpx::wait_all(std::move(sites));
}

int hpx_main()
{
#if defined(HPX_HAVE_NETWORKING)
    if (hpx::get_num_localities(hpx::launch::sync) > 1)
    {
        test_one_shot_use();
        test_multiple_use();
        test_multiple_use_with_generation();
        test_multiple_use_with_generation_sync();
    }
#endif

    if (hpx::get_locality_id() == 0)
    {
        test_local_use();
        test_local_use_sync();
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
