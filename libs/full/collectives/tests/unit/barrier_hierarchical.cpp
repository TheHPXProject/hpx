//  Copyright (c) 2026 Abhishek Bansal
//  Copyright (c) 2026 Alexander Strack
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

#include "hierarchical_roots.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

using namespace hpx::collectives;

// Keep independently created communicators from aliasing in AGAS while
// localities transition between test phases.
constexpr char const* barrier_direct_basename = "/test/barrier_hierarchical/";
constexpr char const* barrier_direct_local_basename =
    "/test/barrier_hierarchical/local/";
#if defined(HPX_DEBUG)
constexpr int ITERATIONS = 50;
#else
constexpr int ITERATIONS = 500;
#endif

// Only the explicit-generation variant is tested for remote (multi-locality)
// reuse: the hierarchical barrier overload requires a non-default generation
// for its 2k-1/2k internal mapping and throws otherwise.
void test_multiple_use_with_generation(int arity = 2, std::uint32_t root = 0)
{
    std::uint32_t const this_locality = hpx::get_locality_id();
    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    HPX_TEST_LTE(static_cast<std::uint32_t>(2), num_localities);

    auto const barrier_clients = create_hierarchical_communicator(
        barrier_direct_basename, num_sites_arg(num_localities),
        this_site_arg(this_locality), arity_arg(arity), generation_arg(),
        root_site_arg(root), flat_fallback_threshold_arg(0));

    // A multi-level tree only guarantees multiple communicators on the
    // representative path. The root is always on that path.
    if (this_locality == root &&
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
    if (this_locality == root)
    {
        std::cout << "remote timing (with generation): " << elapsed / ITERATIONS
                  << "[s]\n"
                  << std::flush;
    }
}

void test_local_use(std::uint32_t num_sites, int arity, std::uint32_t root = 0)
{
    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            auto const barrier_clients = create_hierarchical_communicator(
                barrier_direct_local_basename, num_sites_arg(num_sites),
                this_site_arg(site), arity_arg(arity), generation_arg(),
                root_site_arg(root), flat_fallback_threshold_arg(0));

            // Only the representative path is guaranteed to span multiple
            // communicators. The root is always the representative at each
            // level.
            if (site == root && num_sites > static_cast<std::uint32_t>(arity))
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
            if (site == root)
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

void test_invalid_hierarchical_arguments_rejected()
{
    auto const barrier_clients = create_hierarchical_communicator(
        "/test/barrier_hierarchical/invalid_arguments/", num_sites_arg(2),
        this_site_arg(0), arity_arg(2), generation_arg(), root_site_arg(),
        flat_fallback_threshold_arg(0));

    bool mismatched_root_rejected = false;
    try
    {
        hpx::collectives::barrier(hpx::launch::sync, barrier_clients,
            this_site_arg(0), generation_arg(1), root_site_arg(1));
    }
    catch (hpx::exception const& e)
    {
        mismatched_root_rejected = true;
        HPX_TEST_EQ(e.get_error(), hpx::error::bad_parameter);
    }
    HPX_TEST(mismatched_root_rejected);

    bool mismatched_site_rejected = false;
    try
    {
        hpx::collectives::barrier(hpx::launch::sync, barrier_clients,
            this_site_arg(1), generation_arg(1));
    }
    catch (hpx::exception const& e)
    {
        mismatched_site_rejected = true;
        HPX_TEST_EQ(e.get_error(), hpx::error::bad_parameter);
    }
    HPX_TEST(mismatched_site_rejected);

    // this_site >= num_sites must be rejected before the mismatched-site
    // check has a chance to classify it.
    bool out_of_range_site_rejected = false;
    try
    {
        hpx::collectives::barrier(hpx::launch::sync, barrier_clients,
            this_site_arg(5), generation_arg(1));
    }
    catch (hpx::exception const& e)
    {
        out_of_range_site_rejected = true;
        HPX_TEST_EQ(e.get_error(), hpx::error::bad_parameter);
        HPX_TEST(std::string(e.what()).find(
                     "this_site must be smaller than the number of "
                     "participating sites") != std::string::npos);
    }
    HPX_TEST(out_of_range_site_rejected);
}

void test_invalid_communicator_rejected()
{
    auto comms =
        create_hierarchical_communicator("/test/barrier_hierarchical/invalid/",
            num_sites_arg(1), this_site_arg(0), arity_arg(2), generation_arg(),
            root_site_arg(), flat_fallback_threshold_arg(0));

    HPX_TEST(comms.valid());
    comms.get(0) = communicator{};
    HPX_TEST(!comms.valid());

    bool rejected = false;
    try
    {
        hpx::collectives::barrier(
            hpx::launch::sync, comms, this_site_arg(0), generation_arg(1));
    }
    catch (hpx::exception const& e)
    {
        rejected = true;
        HPX_TEST_EQ(e.get_error(), hpx::error::invalid_status);
    }
    HPX_TEST(rejected);
}

void test_large_single_pass_generations_rejected()
{
    auto const comms = create_hierarchical_communicator(
        "/test/barrier_hierarchical/large_single_pass_generation/",
        num_sites_arg(1), this_site_arg(0), arity_arg(2), generation_arg(),
        root_site_arg(), flat_fallback_threshold_arg(0));

    generation_arg const too_large(
        (std::numeric_limits<std::size_t>::max)() / 2 + 1);

    bool broadcast_rejected = false;
    try
    {
        broadcast_to(hpx::launch::sync, comms, std::uint32_t(1),
            this_site_arg(0), too_large);
    }
    catch (hpx::exception const& e)
    {
        broadcast_rejected = true;
        HPX_TEST_EQ(e.get_error(), hpx::error::bad_parameter);
    }
    HPX_TEST(broadcast_rejected);

    bool gather_rejected = false;
    try
    {
        gather_here(hpx::launch::sync, comms, std::uint32_t(1),
            this_site_arg(0), too_large);
    }
    catch (hpx::exception const& e)
    {
        gather_rejected = true;
        HPX_TEST_EQ(e.get_error(), hpx::error::bad_parameter);
    }
    HPX_TEST(gather_rejected);

    bool reduce_rejected = false;
    try
    {
        reduce_here(hpx::launch::sync, comms, std::uint32_t(1),
            std::plus<std::uint32_t>{}, this_site_arg(0), too_large);
    }
    catch (hpx::exception const& e)
    {
        reduce_rejected = true;
        HPX_TEST_EQ(e.get_error(), hpx::error::bad_parameter);
    }
    HPX_TEST(reduce_rejected);

    bool scatter_rejected = false;
    try
    {
        scatter_to(hpx::launch::sync, comms, std::vector<std::uint32_t>{1},
            this_site_arg(0), too_large);
    }
    catch (hpx::exception const& e)
    {
        scatter_rejected = true;
        HPX_TEST_EQ(e.get_error(), hpx::error::bad_parameter);
    }
    HPX_TEST(scatter_rejected);
}

// Communicators with different roots may share a base name. The composite
// operations accept an explicit root_site matching the one of the communicator
// and reject any other one. This also covers the flat communicators (N == 2,
// arity >= N and the flat fallback), which are rooted at the given site too.
void test_shared_basename_with_roots(
    std::uint32_t num_sites, int arity, std::size_t threshold)
{
    std::string const basename = "/test/barrier_hierarchical/shared_roots/" +
        std::to_string(num_sites) + "/" + std::to_string(arity) + "/" +
        std::to_string(threshold) + "/";

    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);
    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            // two different roots on the same base name (just one if N == 2)
            std::vector<std::uint32_t> roots{1u};
            if (num_sites > 2)
            {
                roots.push_back(num_sites - 1);
            }
            for (std::uint32_t const root : roots)
            {
                auto const comms =
                    create_hierarchical_communicator(basename.c_str(),
                        num_sites_arg(num_sites), this_site_arg(site),
                        arity_arg(arity), generation_arg(), root_site_arg(root),
                        flat_fallback_threshold_arg(threshold));
                HPX_TEST_EQ(static_cast<std::size_t>(comms.get_root_site()),
                    static_cast<std::size_t>(root));

                // N == 2 has no third site to use as a wrong root
                if (num_sites > 2)
                {
                    bool rejected = false;
                    try
                    {
                        barrier(hpx::launch::sync, comms, this_site_arg(site),
                            generation_arg(1),
                            root_site_arg(root == 1 ? 2 : 1));
                    }
                    catch (hpx::exception const& e)
                    {
                        rejected = true;
                        HPX_TEST_EQ(e.get_error(), hpx::error::bad_parameter);
                    }
                    HPX_TEST(rejected);
                }

                barrier(hpx::launch::sync, comms, this_site_arg(site),
                    generation_arg(1), root_site_arg(root));
                barrier(hpx::launch::sync, comms, this_site_arg(site),
                    generation_arg(2));
            }
        }));
    }

    hpx::wait_all(std::move(sites));
}

// Generations of one communicator are processed in order: a barrier for a
// later generation waits until the earlier ones have completed, whichever order
// they were started in. A first call at generation 2 therefore blocks until
// generation 1 has run; it is not an error.
void test_out_of_order_generations(std::uint32_t num_sites, int arity,
    std::uint32_t root, char const* basename)
{
    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);
    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            auto const comms = create_hierarchical_communicator(basename,
                num_sites_arg(num_sites), this_site_arg(site), arity_arg(arity),
                generation_arg(), root_site_arg(root),
                flat_fallback_threshold_arg(0));

            // the hierarchical barrier waits for the earlier generations inside
            // the call, so the later generation is started from its own task
            auto second = hpx::async([&]() {
                barrier(comms, this_site_arg(site), generation_arg(2)).get();
            });
            // can't finish before generation 1 has even started
            HPX_TEST(!second.is_ready());
            barrier(comms, this_site_arg(site), generation_arg(1)).get();
            second.get();
        }));
    }
    hpx::wait_all(std::move(sites));
}

int hpx_main()
{
#if defined(HPX_HAVE_NETWORKING)
    if (hpx::get_num_localities(hpx::launch::sync) > 1)
    {
        for_each_distributed_root([](std::uint32_t root) {
            test_multiple_use_with_generation(2, root);
        });
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

        // non-zero root sites
        for_each_local_root([](std::uint32_t n, int arity, std::uint32_t root) {
            test_local_use(n, arity, root);
        });

        test_out_of_order_generations(
            4, 2, 0, "/test/barrier_hierarchical/out_of_order/0/");
        test_out_of_order_generations(
            5, 2, 3, "/test/barrier_hierarchical/out_of_order/3/");
        test_out_of_order_generations(
            2, 2, 1, "/test/barrier_hierarchical/out_of_order/1/");

        test_shared_basename_with_roots(4, 2, 0);
        test_shared_basename_with_roots(2, 2, 0);
        test_shared_basename_with_roots(4, 2, 16);
        test_shared_basename_with_roots(5, 8, 0);

        test_invalid_arity_rejected();
        test_invalid_hierarchical_arguments_rejected();
        test_invalid_communicator_rejected();
        test_large_single_pass_generations_rejected();
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
