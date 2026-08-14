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

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace hpx::collectives;

constexpr char const* scatter_direct_basename = "/test/scatter_direct/";
#if defined(HPX_DEBUG)
constexpr int ITERATIONS = 100;
#else
constexpr int ITERATIONS = 1000;
#endif

void test_one_shot_use()
{
    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    HPX_TEST_LTE(static_cast<std::uint32_t>(2), num_localities);

    std::uint32_t const this_locality = hpx::get_locality_id();

    // test functionality based on immediate local result value
    for (std::uint32_t i = 0; i != ITERATIONS; ++i)
    {
        if (this_locality == 0)
        {
            std::vector<std::uint32_t> data(num_localities);
            std::iota(data.begin(), data.end(), 42 + i);

            std::uint32_t result =
                scatter_to(hpx::launch::sync, scatter_direct_basename,
                    std::move(data), num_sites_arg(num_localities),
                    this_site_arg(this_locality), generation_arg(i + 1));

            HPX_TEST_EQ(i + 42 + this_locality, result);
        }
        else
        {
            std::uint32_t result = scatter_from<std::uint32_t>(
                hpx::launch::sync, scatter_direct_basename,
                this_site_arg(this_locality), generation_arg(i + 1));

            HPX_TEST_EQ(i + 42 + this_locality, result);
        }
    }
}

void test_multiple_use()
{
    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    HPX_TEST_LTE(static_cast<std::uint32_t>(2), num_localities);

    std::uint32_t const this_locality = hpx::get_locality_id();

    auto const scatter_direct_client =
        create_communicator(scatter_direct_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality));

    // test functionality based on immediate local result value
    for (std::uint32_t i = 0; i != ITERATIONS; ++i)
    {
        if (this_locality == 0)
        {
            std::vector<std::uint32_t> data(num_localities);
            std::iota(data.begin(), data.end(), 42 + i);

            std::uint32_t result = scatter_to(
                hpx::launch::sync, scatter_direct_client, std::move(data));

            HPX_TEST_EQ(i + 42 + this_locality, result);
        }
        else
        {
            std::uint32_t result = scatter_from<std::uint32_t>(
                hpx::launch::sync, scatter_direct_client);

            HPX_TEST_EQ(i + 42 + this_locality, result);
        }
    }
}

void test_multiple_use_with_generation()
{
    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    HPX_TEST_LTE(static_cast<std::uint32_t>(2), num_localities);

    std::uint32_t const this_locality = hpx::get_locality_id();

    auto const scatter_direct_client =
        create_communicator(scatter_direct_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality));

    hpx::chrono::high_resolution_timer const t;

    for (std::uint32_t i = 0; i != ITERATIONS; ++i)
    {
        if (this_locality == 0)
        {
            std::vector<std::uint32_t> data(num_localities);
            std::iota(data.begin(), data.end(), 42 + i);

            std::uint32_t result = scatter_to(hpx::launch::sync,
                scatter_direct_client, std::move(data), generation_arg(i + 1));

            HPX_TEST_EQ(i + 42 + this_locality, result);
        }
        else
        {
            std::uint32_t result =
                scatter_from<std::uint32_t>(hpx::launch::sync,
                    scatter_direct_client, generation_arg(i + 1));

            HPX_TEST_EQ(i + 42 + this_locality, result);
        }
    }

    auto const elapsed = t.elapsed();
    if (this_locality == 0)
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
            auto const scatter_direct_client =
                create_communicator(scatter_direct_basename,
                    num_sites_arg(num_sites), this_site_arg(site));

            hpx::chrono::high_resolution_timer const t;

            for (std::uint32_t i = 0; i != ITERATIONS; ++i)
            {
                if (site == 0)
                {
                    std::vector<std::uint32_t> data(num_sites);
                    std::iota(data.begin(), data.end(), 42 + i);

                    std::uint32_t result = scatter_to(hpx::launch::sync,
                        scatter_direct_client, std::move(data),
                        generation_arg(i + 1), this_site_arg(site));

                    HPX_TEST_EQ(i + 42 + site, result);
                }
                else
                {
                    std::uint32_t result = scatter_from<std::uint32_t>(
                        hpx::launch::sync, scatter_direct_client,
                        generation_arg(i + 1), this_site_arg(site));

                    HPX_TEST_EQ(i + 42 + site, result);
                }
            }

            auto const elapsed = t.elapsed();
            if (site == 0)
            {
                std::cout << "local timing: " << elapsed / ITERATIONS
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
    }
#endif

    if (hpx::get_locality_id() == 0)
    {
        test_local_use();
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
