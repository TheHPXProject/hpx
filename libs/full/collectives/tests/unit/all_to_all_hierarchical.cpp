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
#include <string>
#include <utility>
#include <vector>

using namespace hpx::collectives;

constexpr char const* all_to_all_hierarchical_basename =
    "/test/all_to_all_hierarchical/";

#if defined(HPX_DEBUG)
constexpr int ITERATIONS = 10;
#else
constexpr int ITERATIONS = 50;
#endif

///////////////////////////////////////////////////////////////////////////
// Uniform payload: each site fills all slots with the same value.
// values[j] = site + iteration for all j. After exchange:
// result[s] = s + iteration.
void test_uniform_payload(std::uint32_t num_sites, int arity)
{
    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            std::string basename(all_to_all_hierarchical_basename);
            basename += "uniform_" + std::to_string(num_sites) + "_" +
                std::to_string(arity) + "/";

            auto const comms = create_hierarchical_communicator(
                basename.c_str(), num_sites_arg(num_sites), this_site_arg(site),
                arity_arg(arity), generation_arg(), root_site_arg(),
                flat_fallback_threshold_arg(0));

            for (int i = 0; i != ITERATIONS; ++i)
            {
                std::vector<std::uint32_t> values(num_sites);
                std::fill(values.begin(), values.end(), site + i);

                auto result =
                    all_to_all(hpx::launch::sync, comms, std::move(values),
                        this_site_arg(site), generation_arg(i + 1));

                HPX_TEST_EQ(result.size(), static_cast<std::size_t>(num_sites));
                for (std::size_t j = 0; j != result.size(); ++j)
                {
                    HPX_TEST_EQ(result[j], static_cast<std::uint32_t>(j + i));
                }
            }
        }));
    }

    hpx::wait_all(std::move(sites));
}

///////////////////////////////////////////////////////////////////////////
// Distinct payload: values[j] = site * num_sites + j + i.
// After exchange: result[s] = s * num_sites + this_site + i.
// This tests that each (source, destination) pair carries a unique value
// through the hierarchical tree.
void test_distinct_payload(std::uint32_t num_sites, int arity)
{
    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            std::string basename(all_to_all_hierarchical_basename);
            basename += "distinct_" + std::to_string(num_sites) + "_" +
                std::to_string(arity) + "/";

            auto const comms = create_hierarchical_communicator(
                basename.c_str(), num_sites_arg(num_sites), this_site_arg(site),
                arity_arg(arity), generation_arg(), root_site_arg(),
                flat_fallback_threshold_arg(0));

            for (int i = 0; i != ITERATIONS; ++i)
            {
                std::vector<std::uint32_t> values(num_sites);
                for (std::uint32_t j = 0; j != num_sites; ++j)
                {
                    values[j] = site * num_sites + j + i;
                }

                auto result =
                    all_to_all(hpx::launch::sync, comms, std::move(values),
                        this_site_arg(site), generation_arg(i + 1));

                HPX_TEST_EQ(result.size(), static_cast<std::size_t>(num_sites));
                for (std::size_t s = 0; s != result.size(); ++s)
                {
                    HPX_TEST_EQ(result[s],
                        static_cast<std::uint32_t>(s * num_sites + site + i));
                }
            }
        }));
    }

    hpx::wait_all(std::move(sites));
}

int hpx_main()
{
    if (hpx::get_locality_id() == 0)
    {
        // Balanced cases
        for (int arity : {2, 4})
        {
            for (std::uint32_t n : {2, 4, 8, 16})
            {
                test_uniform_payload(n, arity);
                test_distinct_payload(n, arity);
            }
        }

        // Unbalanced cases (num_sites not a power of arity)
        for (int arity : {2, 3, 4})
        {
            for (std::uint32_t n : {3, 5, 7, 11})
            {
                test_uniform_payload(n, arity);
                test_distinct_payload(n, arity);
            }
        }
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
