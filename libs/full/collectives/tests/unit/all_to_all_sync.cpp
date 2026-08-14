//  Copyright (c) 2019-2025 Hartmut Kaiser
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

constexpr char const* all_to_all_direct_basename = "/test/all_to_all_direct/";
#if defined(HPX_DEBUG)
constexpr int ITERATIONS = 100;
#else
constexpr int ITERATIONS = 1000;
#endif

void test_one_shot_use()
{
    std::uint32_t const this_locality = hpx::get_locality_id();
    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    HPX_TEST_LTE(static_cast<std::uint32_t>(2), num_localities);

    // test functionality based on immediate local result value
    for (int i = 0; i != ITERATIONS; ++i)
    {
        std::vector<std::uint32_t> values(num_localities);
        std::fill(values.begin(), values.end(), this_locality + i);

        std::vector<std::uint32_t> r =
            all_to_all(hpx::launch::sync, all_to_all_direct_basename,
                std::move(values), num_sites_arg(num_localities),
                this_site_arg(this_locality), generation_arg(i + 1));

        HPX_TEST_EQ(r.size(), num_localities);

        for (std::size_t j = 0; j != r.size(); ++j)
        {
            HPX_TEST_EQ(r[j], j + i);
        }
    }
}

void test_multiple_use()
{
    std::uint32_t const this_locality = hpx::get_locality_id();
    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    HPX_TEST_LTE(static_cast<std::uint32_t>(2), num_localities);

    auto const all_to_all_direct_client =
        create_communicator(all_to_all_direct_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality));

    // test functionality based on immediate local result value
    for (int i = 0; i != ITERATIONS; ++i)
    {
        std::vector<std::uint32_t> values(num_localities);
        std::fill(values.begin(), values.end(), this_locality + i);

        std::vector<std::uint32_t> r = all_to_all(
            hpx::launch::sync, all_to_all_direct_client, std::move(values));

        HPX_TEST_EQ(r.size(), num_localities);

        for (std::size_t j = 0; j != r.size(); ++j)
        {
            HPX_TEST_EQ(r[j], j + i);
        }
    }
}

void test_multiple_use_with_generation()
{
    std::uint32_t const this_locality = hpx::get_locality_id();
    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    HPX_TEST_LTE(static_cast<std::uint32_t>(2), num_localities);

    auto const all_to_all_direct_client =
        create_communicator(all_to_all_direct_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality));

    hpx::chrono::high_resolution_timer const t;

    for (int i = 0; i != ITERATIONS; ++i)
    {
        std::vector<std::uint32_t> values(num_localities);
        std::fill(values.begin(), values.end(), this_locality + i);

        std::vector<std::uint32_t> r = all_to_all(hpx::launch::sync,
            all_to_all_direct_client, std::move(values), generation_arg(i + 1));

        HPX_TEST_EQ(r.size(), num_localities);

        for (std::size_t j = 0; j != r.size(); ++j)
        {
            HPX_TEST_EQ(r[j], j + i);
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
            auto const all_to_all_direct_client =
                create_local_communicator(all_to_all_direct_basename,
                    num_sites_arg(num_sites), this_site_arg(site));

            hpx::chrono::high_resolution_timer const t;

            for (std::uint32_t i = 0; i != 10 * ITERATIONS; ++i)
            {
                std::vector<std::uint32_t> values(num_sites);
                std::fill(values.begin(), values.end(), site + i);

                std::vector<std::uint32_t> r = all_to_all(hpx::launch::sync,
                    all_to_all_direct_client, std::move(values),
                    this_site_arg(site), generation_arg(i + 1));

                HPX_TEST_EQ(r.size(), num_sites);

                for (std::size_t j = 0; j != r.size(); ++j)
                {
                    HPX_TEST_EQ(r[j], j + i);
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
