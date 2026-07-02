//  Copyright (c) 2019-2024 Hartmut Kaiser
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

constexpr char const* exclusive_scan_basename = "/test/exclusive_scan/";
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
    for (int i = 0; i != ITERATIONS; ++i)
    {
        hpx::future<std::uint32_t> overall_result = exclusive_scan(
            exclusive_scan_basename, here + i, static_cast<std::uint32_t>(i),
            std::plus<std::uint32_t>{}, num_sites_arg(num_localities),
            this_site_arg(here), generation_arg(i + 1));

        std::uint32_t sum = i;
        for (std::uint32_t j = 0; j < here; ++j)
        {
            sum += j + i;
        }
        HPX_TEST_EQ(sum, overall_result.get());
    }
}

void test_multiple_use()
{
    std::uint32_t const here = hpx::get_locality_id();
    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    HPX_TEST_LTE(static_cast<std::uint32_t>(2), num_localities);

    auto const exclusive_scan_client =
        create_communicator(exclusive_scan_basename,
            num_sites_arg(num_localities), this_site_arg(here));

    // test functionality based on immediate local result value
    for (int i = 0; i != ITERATIONS; ++i)
    {
        hpx::future<std::uint32_t> overall_result =
            exclusive_scan(exclusive_scan_client, here + i,
                static_cast<std::uint32_t>(i), std::plus<std::uint32_t>{});

        std::uint32_t sum = i;
        for (std::uint32_t j = 0; j < here; ++j)
        {
            sum += j + i;
        }
        HPX_TEST_EQ(sum, overall_result.get());
    }
}

void test_multiple_use_with_generation()
{
    std::uint32_t const here = hpx::get_locality_id();
    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    HPX_TEST_LTE(static_cast<std::uint32_t>(2), num_localities);

    auto const exclusive_scan_client =
        create_communicator(exclusive_scan_basename,
            num_sites_arg(num_localities), this_site_arg(here));

    hpx::chrono::high_resolution_timer const t;

    for (int i = 0; i != ITERATIONS; ++i)
    {
        hpx::future<std::uint32_t> overall_result = exclusive_scan(
            exclusive_scan_client, here + i, static_cast<std::uint32_t>(i),
            std::plus<std::uint32_t>{}, generation_arg(i + 1));

        std::uint32_t sum = i;
        for (std::uint32_t j = 0; j < here; ++j)
        {
            sum += j + i;
        }
        HPX_TEST_EQ(sum, overall_result.get());
    }

    auto const elapsed = t.elapsed();
    if (here == 0)
    {
        std::cout << "remote timing: " << elapsed / ITERATIONS << "[s]\n";
    }
}

void test_local_use(std::uint32_t num_sites)
{
    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    // launch num_sites threads to represent different sites
    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            auto const exclusive_scan_client =
                create_communicator(exclusive_scan_basename,
                    num_sites_arg(num_sites), this_site_arg(site));

            hpx::chrono::high_resolution_timer const t;

            for (std::uint32_t i = 0; i != 10 * ITERATIONS; ++i)
            {
                hpx::future<std::uint32_t> overall_result = exclusive_scan(
                    exclusive_scan_client, site + i, i, std::plus<>{},
                    this_site_arg(site), generation_arg(i + 1));

                auto const result = overall_result.get();

                std::uint32_t sum = i;
                for (std::uint32_t j = 0; j != site; ++j)
                {
                    sum += j + i;
                }
                HPX_TEST_EQ(sum, result);
            }

            auto const elapsed = t.elapsed();
            if (site == 0)
            {
                std::cout << "local timing: " << elapsed / (10 * ITERATIONS)
                          << "[s]\n";
            }
        }));
    }

    hpx::wait_all(sites);
    for (auto& site : sites)
    {
        site.get();
    }
}

void test_init_type_conversion(std::uint32_t num_sites)
{
    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            std::string const basename = std::string(exclusive_scan_basename) +
                "init_conversion/" + std::to_string(num_sites) + "/";

            auto const exclusive_scan_client =
                create_communicator(basename.c_str(), num_sites_arg(num_sites),
                    this_site_arg(site));

            auto add = [](double lhs, double rhs) { return lhs + rhs; };

            double const result = exclusive_scan(exclusive_scan_client,
                static_cast<double>(site) + 0.25, 1, add, this_site_arg(site),
                generation_arg(1))
                                      .get();

            double expected = 1.0;
            for (std::uint32_t j = 0; j != site; ++j)
            {
                expected += static_cast<double>(j) + 0.25;
            }

            HPX_TEST_EQ(result, expected);
        }));
    }

    hpx::wait_all(sites);
    for (auto& site : sites)
    {
        site.get();
    }
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
        test_local_use(1);
        test_local_use(10);
        test_init_type_conversion(10);
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
