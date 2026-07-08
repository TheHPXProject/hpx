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
#include <functional>
#include <limits>
#include <string>
#include <utility>
#include <vector>

using namespace hpx::collectives;

constexpr char const* inclusive_scan_direct_basename =
    "/test/inclusive_scan_hierarchical/";
#if defined(HPX_DEBUG)
constexpr int ITERATIONS = 4;
#else
constexpr int ITERATIONS = 16;
#endif

std::uint32_t expected_inclusive_sum(
    std::uint32_t site, std::uint32_t iteration)
{
    std::uint32_t sum = 0;
    for (std::uint32_t j = 0; j != site + 1; ++j)
    {
        sum += j + iteration;
    }
    return sum;
}

void test_distributed_correctness(int arity = 2)
{
    std::uint32_t const this_locality = hpx::get_locality_id();
    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    HPX_TEST_LTE(static_cast<std::uint32_t>(2), num_localities);

    auto const comms = create_hierarchical_communicator(
        inclusive_scan_direct_basename, num_sites_arg(num_localities),
        this_site_arg(this_locality), arity_arg(arity), generation_arg(),
        root_site_arg(), flat_fallback_threshold_arg(0));

    for (std::uint32_t i = 0; i != ITERATIONS; ++i)
    {
        std::uint32_t const result = inclusive_scan(comms,
            std::uint32_t(this_locality + i), std::plus<std::uint32_t>{},
            this_site_arg(this_locality), generation_arg(i + 1))
                                         .get();

        HPX_TEST_EQ(result, expected_inclusive_sum(this_locality, i));
    }
}

void test_local_numeric(
    std::uint32_t num_sites, int arity, std::size_t threshold)
{
    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            std::string const basename =
                std::string(inclusive_scan_direct_basename) + "local/" +
                std::to_string(num_sites) + "/" + std::to_string(arity) + "/" +
                std::to_string(threshold) + "/";

            auto const comms = create_hierarchical_communicator(
                basename.c_str(), num_sites_arg(num_sites), this_site_arg(site),
                arity_arg(arity), generation_arg(), root_site_arg(),
                flat_fallback_threshold_arg(threshold));

            for (std::uint32_t i = 0; i != ITERATIONS; ++i)
            {
                std::uint32_t const result = inclusive_scan(comms,
                    std::uint32_t(site + i), std::plus<std::uint32_t>{},
                    this_site_arg(site), generation_arg(i + 1))
                                                 .get();

                HPX_TEST_EQ(result, expected_inclusive_sum(site, i));
            }
        }));
    }

    hpx::wait_all(sites);
    for (auto& site : sites)
    {
        site.get();
    }
}

void test_local_shapes()
{
    for (std::uint32_t num_sites : {2u, 4u, 8u, 16u, 32u, 64u})
    {
        test_local_numeric(num_sites, 2, 0);
        if (num_sites >= 4)
        {
            test_local_numeric(num_sites, 4, 0);
        }
    }
}

void test_non_power_of_arity()
{
    for (std::uint32_t num_sites : {3u, 5u, 6u, 7u, 9u, 10u, 11u, 15u})
    {
        test_local_numeric(num_sites, 2, 0);
    }

    for (std::uint32_t num_sites : {5u, 6u, 7u, 9u, 10u, 11u, 15u})
    {
        test_local_numeric(num_sites, 4, 0);
    }

    for (std::uint32_t num_sites : {5u, 7u, 9u, 10u, 11u, 15u})
    {
        test_local_numeric(num_sites, 3, 0);
    }
}

void test_flat_fallback_vs_tree()
{
    constexpr std::uint32_t num_sites = 8;
    constexpr int arity = 2;

    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            auto const flat = create_hierarchical_communicator(
                "/test/inclusive_scan_hierarchical/flat/",
                num_sites_arg(num_sites), this_site_arg(site), arity_arg(arity),
                generation_arg(), root_site_arg(),
                flat_fallback_threshold_arg(1024));

            auto const tree = create_hierarchical_communicator(
                "/test/inclusive_scan_hierarchical/tree/",
                num_sites_arg(num_sites), this_site_arg(site), arity_arg(arity),
                generation_arg(), root_site_arg(),
                flat_fallback_threshold_arg(0));

            HPX_TEST_EQ(flat.size(), static_cast<std::size_t>(1));
            HPX_TEST_LT(static_cast<std::size_t>(tree.get_arity()),
                static_cast<std::size_t>(num_sites));

            for (std::uint32_t i = 0; i != ITERATIONS; ++i)
            {
                std::uint32_t const value = site + i;
                std::uint32_t const flat_result = inclusive_scan(flat,
                    std::uint32_t(value), std::plus<std::uint32_t>{},
                    this_site_arg(site), generation_arg(i + 1))
                                                      .get();
                std::uint32_t const tree_result = inclusive_scan(tree,
                    std::uint32_t(value), std::plus<std::uint32_t>{},
                    this_site_arg(site), generation_arg(i + 1))
                                                      .get();

                HPX_TEST_EQ(flat_result, tree_result);
                HPX_TEST_EQ(tree_result, expected_inclusive_sum(site, i));
            }
        }));
    }

    hpx::wait_all(sites);
    for (auto& site : sites)
    {
        site.get();
    }
}

void test_generation_rejections()
{
    constexpr std::uint32_t num_sites = 4;

    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            auto const comms = create_hierarchical_communicator(
                "/test/inclusive_scan_hierarchical/rejections/",
                num_sites_arg(num_sites), this_site_arg(site), arity_arg(2),
                generation_arg(), root_site_arg(),
                flat_fallback_threshold_arg(0));

            bool default_rejected = false;
            try
            {
                inclusive_scan(hpx::launch::sync, comms, std::uint32_t(site),
                    std::plus<std::uint32_t>{}, this_site_arg(site),
                    generation_arg());
            }
            catch (hpx::exception const&)
            {
                default_rejected = true;
            }
            HPX_TEST(default_rejected);

            bool zero_rejected = false;
            try
            {
                inclusive_scan(hpx::launch::sync, comms, std::uint32_t(site),
                    std::plus<std::uint32_t>{}, this_site_arg(site),
                    generation_arg(0));
            }
            catch (hpx::exception const&)
            {
                zero_rejected = true;
            }
            HPX_TEST(zero_rejected);

            bool too_large_rejected = false;
            try
            {
                inclusive_scan(hpx::launch::sync, comms, std::uint32_t(site),
                    std::plus<std::uint32_t>{}, this_site_arg(site),
                    generation_arg(
                        (std::numeric_limits<std::size_t>::max)() / 2 + 1));
            }
            catch (hpx::exception const&)
            {
                too_large_rejected = true;
            }
            HPX_TEST(too_large_rejected);
        }));
    }

    hpx::wait_all(sites);
    for (auto& site : sites)
    {
        site.get();
    }
}

void test_string_ordering()
{
    constexpr std::uint32_t num_sites = 7;

    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            auto const comms = create_hierarchical_communicator(
                "/test/inclusive_scan_hierarchical/strings/",
                num_sites_arg(num_sites), this_site_arg(site), arity_arg(3),
                generation_arg(), root_site_arg(),
                flat_fallback_threshold_arg(0));

            auto concat = [](std::string lhs, std::string const& rhs) {
                return lhs + ":" + rhs;
            };

            std::string const result =
                inclusive_scan(hpx::launch::sync, comms, std::to_string(site),
                    concat, this_site_arg(site), generation_arg(1));

            std::string expected = "0";
            for (std::uint32_t j = 1; j != site + 1; ++j)
            {
                expected += ":" + std::to_string(j);
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

void test_bool_scan()
{
    constexpr std::uint32_t num_sites = 5;

    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            auto const comms = create_hierarchical_communicator(
                "/test/inclusive_scan_hierarchical/bool/",
                num_sites_arg(num_sites), this_site_arg(site), arity_arg(2),
                generation_arg(), root_site_arg(),
                flat_fallback_threshold_arg(0));

            for (std::uint32_t target = 0; target != num_sites; ++target)
            {
                bool const value = site == target;
                bool const result = inclusive_scan(hpx::launch::sync, comms,
                    value, std::logical_or<bool>{}, this_site_arg(site),
                    generation_arg(target + 1));

                HPX_TEST_EQ(result, site >= target);
            }
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
        test_distributed_correctness();
    }
#endif

    if (hpx::get_locality_id() == 0)
    {
        test_local_shapes();
        test_non_power_of_arity();
        test_flat_fallback_vs_tree();
        test_generation_rejections();
        test_string_ordering();
        test_bool_scan();
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
