//  Copyright (c) 2020-2024 Hartmut Kaiser
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
constexpr char const* gather_direct_basename = "/test/gather_direct/";
constexpr char const* gather_direct_multiple_use_basename =
    "/test/gather_direct/multiple_use/";
constexpr char const* gather_direct_explicit_generation_basename =
    "/test/gather_direct/explicit_generation/";
constexpr char const* gather_direct_local_basename =
    "/test/gather_direct/local/";
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
    for (std::uint32_t i = 0; i != ITERATIONS; ++i)
    {
        if (this_locality == 0)
        {
            std::uint32_t value = 42 + i;
            hpx::future<std::vector<std::uint32_t>> overall_result =
                gather_here(gather_direct_basename, value,
                    num_sites_arg(num_localities), this_site_arg(this_locality),
                    generation_arg(i + 1));

            std::vector<std::uint32_t> sol = overall_result.get();
            for (std::size_t j = 0; j != sol.size(); ++j)
            {
                HPX_TEST(j + 42 + i == sol[j]);
            }
        }
        else
        {
            hpx::future<void> overall_result =
                gather_there(gather_direct_basename, this_locality + 42 + i,
                    this_site_arg(this_locality), generation_arg(i + 1));
            overall_result.get();
        }
    }
}

void test_multiple_use()
{
    std::uint32_t const this_locality = hpx::get_locality_id();
    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    HPX_TEST_LTE(static_cast<std::uint32_t>(2), num_localities);

    // test functionality based on immediate local result value
    auto const gather_direct_client =
        create_communicator(gather_direct_multiple_use_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality));

    for (std::uint32_t i = 0; i != ITERATIONS; ++i)
    {
        if (this_locality == 0)
        {
            hpx::future<std::vector<std::uint32_t>> overall_result =
                gather_here(gather_direct_client, 42 + i);

            std::vector<std::uint32_t> sol = overall_result.get();
            for (std::size_t j = 0; j != sol.size(); ++j)
            {
                HPX_TEST(j + 42 + i == sol[j]);
            }
        }
        else
        {
            hpx::future<void> overall_result =
                gather_there(gather_direct_client, this_locality + 42 + i);
            overall_result.get();
        }
    }
}

void test_multiple_use_with_generation()
{
    std::uint32_t const this_locality = hpx::get_locality_id();
    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    HPX_TEST_LTE(static_cast<std::uint32_t>(2), num_localities);

    // test functionality based on immediate local result value
    auto const gather_direct_client =
        create_communicator(gather_direct_explicit_generation_basename,
            num_sites_arg(num_localities), this_site_arg(this_locality));

    hpx::chrono::high_resolution_timer const t;

    for (std::uint32_t i = 0; i != ITERATIONS; ++i)
    {
        if (this_locality == 0)
        {
            hpx::future<std::vector<std::uint32_t>> overall_result =
                gather_here(
                    gather_direct_client, 42 + i, generation_arg(i + 1));

            std::vector<std::uint32_t> sol = overall_result.get();
            for (std::size_t j = 0; j != sol.size(); ++j)
            {
                HPX_TEST(j + 42 + i == sol[j]);
            }
        }
        else
        {
            hpx::future<void> overall_result =
                gather_there(gather_direct_client, this_locality + 42 + i,
                    generation_arg(i + 1));
            overall_result.get();
        }
    }

    auto const elapsed = t.elapsed();
    if (this_locality == 0)
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
            auto const gather_direct_client =
                create_communicator(gather_direct_local_basename,
                    num_sites_arg(num_sites), this_site_arg(site));

            hpx::chrono::high_resolution_timer const t;

            for (std::uint32_t i = 0; i != 10 * ITERATIONS; ++i)
            {
                if (site == 0)
                {
                    hpx::future<std::vector<std::uint32_t>> overall_result =
                        gather_here(gather_direct_client, 42 + i,
                            generation_arg(i + 1), this_site_arg(site));

                    std::vector<std::uint32_t> sol = overall_result.get();
                    for (std::size_t j = 0; j != sol.size(); ++j)
                    {
                        HPX_TEST(j + 42 + i == sol[j]);
                    }
                }
                else
                {
                    hpx::future<void> overall_result =
                        gather_there(gather_direct_client, site + 42 + i,
                            generation_arg(i + 1), this_site_arg(site));
                    overall_result.get();
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
        test_local_use(1);
        test_local_use(10);
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
