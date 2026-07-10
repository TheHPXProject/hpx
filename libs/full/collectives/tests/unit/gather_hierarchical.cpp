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
#include <new>
#include <string>
#include <utility>
#include <vector>

using namespace hpx::collectives;

constexpr char const* gather_direct_basename = "/test/gather_hierarchical/";
#if defined(HPX_DEBUG)
constexpr int ITERATIONS = 50;
#else
constexpr int ITERATIONS = 500;
#endif

struct non_default_payload
{
    non_default_payload() = delete;

    explicit non_default_payload(std::uint32_t const value)
      : value(value)
    {
    }

    template <typename Archive>
    void serialize(Archive&, unsigned int const)
    {
    }

    template <typename Archive>
    friend void save_construct_data(Archive& ar,
        non_default_payload const* const payload, unsigned int const)
    {
        ar & payload->value;
    }

    template <typename Archive>
    friend void load_construct_data(
        Archive& ar, non_default_payload* const payload, unsigned int const)
    {
        std::uint32_t value = 0;
        ar & value;
        ::new (payload) non_default_payload(value);
    }

    std::uint32_t value;
};

void test_non_default_payload()
{
    std::uint32_t const this_locality = hpx::get_locality_id();
    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);

    auto const gather_clients = create_hierarchical_communicator(
        "/test/gather_hierarchical_non_default/", num_sites_arg(num_localities),
        this_site_arg(this_locality), arity_arg(2), generation_arg(),
        root_site_arg(), flat_fallback_threshold_arg(0));

    non_default_payload value(this_locality + 42);
    if (this_locality == 0)
    {
        auto const result = gather_here(gather_clients, HPX_MOVE(value),
            this_site_arg(this_locality), generation_arg(1))
                                .get();
        HPX_TEST_EQ(result.size(), static_cast<std::size_t>(num_localities));
        for (std::uint32_t site = 0; site != num_localities; ++site)
        {
            HPX_TEST_EQ(result[site].value, site + 42);
        }
    }
    else
    {
        gather_there(gather_clients, HPX_MOVE(value),
            this_site_arg(this_locality), generation_arg(1))
            .get();
    }
}

void test_multiple_use(int arity = 2)
{
    std::uint32_t const this_locality = hpx::get_locality_id();
    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    HPX_TEST_LTE(static_cast<std::uint32_t>(2), num_localities);

    // test functionality based on immediate local result value
    auto const gather_clients = create_hierarchical_communicator(
        gather_direct_basename, num_sites_arg(num_localities),
        this_site_arg(this_locality), arity_arg(arity), generation_arg(),
        root_site_arg(), flat_fallback_threshold_arg(0));

    hpx::chrono::high_resolution_timer const t;

    for (std::uint32_t i = 0; i != ITERATIONS; ++i)
    {
        if (this_locality == 0)
        {
            hpx::future<std::vector<std::uint32_t>> overall_result =
                gather_here(gather_clients, 42 + i);

            std::vector<std::uint32_t> sol = overall_result.get();
            for (std::size_t j = 0; j != sol.size(); ++j)
            {
                HPX_TEST(j + 42 + i == sol[j]);
            }
        }
        else
        {
            hpx::future<void> overall_result =
                gather_there(gather_clients, this_locality + 42 + i);
            overall_result.get();
        }
    }

    auto const elapsed = t.elapsed();
    if (this_locality == 0)
    {
        std::cout << "remote timing: " << elapsed / ITERATIONS << "[s]\n"
                  << std::flush;
    }
}

void test_multiple_use_with_generation(int arity = 2)
{
    std::uint32_t const this_locality = hpx::get_locality_id();
    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    HPX_TEST_LTE(static_cast<std::uint32_t>(2), num_localities);

    // test functionality based on immediate local result value
    auto const gather_clients = create_hierarchical_communicator(
        gather_direct_basename, num_sites_arg(num_localities),
        this_site_arg(this_locality), arity_arg(arity), generation_arg(),
        root_site_arg(), flat_fallback_threshold_arg(0));

    hpx::chrono::high_resolution_timer const t;

    for (std::uint32_t i = 0; i != ITERATIONS; ++i)
    {
        if (this_locality == 0)
        {
            hpx::future<std::vector<std::uint32_t>> overall_result =
                gather_here(gather_clients, 42 + i, this_site_arg(),
                    generation_arg(i + 1));

            std::vector<std::uint32_t> sol = overall_result.get();
            for (std::size_t j = 0; j != sol.size(); ++j)
            {
                HPX_TEST(j + 42 + i == sol[j]);
            }
        }
        else
        {
            hpx::future<void> overall_result = gather_there(gather_clients,
                this_locality + 42 + i, this_site_arg(), generation_arg(i + 1));
            overall_result.get();
        }
    }

    auto const elapsed = t.elapsed();
    if (this_locality == 0)
    {
        std::cout << "remote timing (with generation): " << elapsed / ITERATIONS
                  << "[s]\n"
                  << std::flush;
    }
}

void test_local_use(std::uint32_t num_sites, int arity = 2)
{
    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    // launch num_sites threads to represent different sites
    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            auto const gather_clients = create_hierarchical_communicator(
                gather_direct_basename, num_sites_arg(num_sites),
                this_site_arg(site), arity_arg(arity), generation_arg(),
                root_site_arg(), flat_fallback_threshold_arg(0));

            hpx::chrono::high_resolution_timer const t;

            for (std::uint32_t i = 0; i != 10 * ITERATIONS; ++i)
            {
                if (site == 0)
                {
                    hpx::future<std::vector<std::uint32_t>> overall_result =
                        gather_here(gather_clients, 42 + i, this_site_arg(site),
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
                        gather_there(gather_clients, site + 42 + i,
                            this_site_arg(site), generation_arg(i + 1));
                    overall_result.get();
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

int hpx_main()
{
#if defined(HPX_HAVE_NETWORKING)
    if (hpx::get_num_localities(hpx::launch::sync) > 1)
    {
        test_multiple_use();
        test_multiple_use_with_generation();
        test_non_default_payload();
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
