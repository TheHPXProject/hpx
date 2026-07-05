//  Copyright (c) 2026 Abhishek Bansal
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

constexpr char const* barrier_direct_basename = "/test/barrier_hierarchical/";
#if defined(HPX_DEBUG)
constexpr int ITERATIONS = 50;
#else
constexpr int ITERATIONS = 500;
#endif

// Only the explicit-generation variant is tested for remote (multi-locality)
// reuse: the hierarchical barrier overload requires a non-default generation
// for its 2k-1/2k internal mapping and throws otherwise.
void test_multiple_use_with_generation(int arity = 2)
{
    std::uint32_t const this_locality = hpx::get_locality_id();
    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    HPX_TEST_LTE(static_cast<std::uint32_t>(2), num_localities);

    auto const barrier_clients = create_hierarchical_communicator(
        barrier_direct_basename, num_sites_arg(num_localities),
        this_site_arg(this_locality), arity_arg(arity), generation_arg(),
        root_site_arg(), flat_fallback_threshold_arg(0));

    // A multi-level tree only guarantees multiple communicators on the
    // representative path. Locality 0 is always on that path.
    if (this_locality == 0 &&
        num_localities > static_cast<std::uint32_t>(arity))
    {
        HPX_TEST_LTE(static_cast<std::size_t>(2), barrier_clients.size());
    }

    hpx::chrono::high_resolution_timer const t;

    for (std::uint32_t i = 0; i != ITERATIONS; ++i)
    {
        hpx::collectives::barrier(
            barrier_clients, this_site_arg(), generation_arg(i + 1))
            .get();
    }

    auto const elapsed = t.elapsed();
    if (this_locality == 0)
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

    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            auto const barrier_clients = create_hierarchical_communicator(
                barrier_direct_basename, num_sites_arg(num_sites),
                this_site_arg(site), arity_arg(arity), generation_arg(),
                root_site_arg(), flat_fallback_threshold_arg(0));

            // Only the representative path is guaranteed to span multiple
            // communicators. Site 0 is always the representative at each level.
            if (site == 0 && num_sites > static_cast<std::uint32_t>(arity))
            {
                HPX_TEST_LTE(
                    static_cast<std::size_t>(2), barrier_clients.size());
            }

            hpx::chrono::high_resolution_timer const t;

            for (std::uint32_t i = 0; i != ITERATIONS; ++i)
            {
                hpx::collectives::barrier(
                    barrier_clients, this_site_arg(site), generation_arg(i + 1))
                    .get();
            }

            auto const elapsed = t.elapsed();
            if (site == 0)
            {
                std::cout << "local timing (" << num_sites << "/" << arity
                          << "): " << elapsed / ITERATIONS << "[s]\n"
                          << std::flush;
            }
        }));
    }

    hpx::wait_all(std::move(sites));
}

void test_non_power_of_arity()
{
    for (std::uint32_t num_sites : {3u, 5u, 6u, 7u, 9u, 10u, 11u, 15u})
    {
        test_local_use(num_sites, 2);
    }

    for (std::uint32_t num_sites : {5u, 6u, 7u, 9u, 10u, 11u, 13u, 15u})
    {
        test_local_use(num_sites, 4);
    }
}

void test_invalid_arity_rejected()
{
    for (std::uint32_t arity : {0u, 1u})
    {
        bool caught_exception = false;
        try
        {
            [[maybe_unused]] auto const barrier_clients =
                create_hierarchical_communicator(
                    "/test/barrier_hierarchical/invalid_arity/",
                    num_sites_arg(1), this_site_arg(0), arity_arg(arity),
                    generation_arg(), root_site_arg(),
                    flat_fallback_threshold_arg(0));
        }
        catch (hpx::exception const& e)
        {
            caught_exception = true;
            HPX_TEST_EQ(e.get_error(), hpx::error::bad_parameter);
        }
        HPX_TEST(caught_exception);
    }
}

int hpx_main()
{
#if defined(HPX_HAVE_NETWORKING)
    if (hpx::get_num_localities(hpx::launch::sync) > 1)
    {
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
        test_invalid_arity_rejected();
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
