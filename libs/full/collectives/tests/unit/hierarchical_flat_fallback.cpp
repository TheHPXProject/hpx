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
#include <string>
#include <utility>
#include <vector>

using namespace hpx::collectives;

// Distributed test (multi-locality): verifies that the fallback path produces
// size() == 1 and correct collective results, and that threshold == 0 forces
// the tree path with the same results.
void test_distributed_fallback()
{
    std::uint32_t const this_locality = hpx::get_locality_id();
    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    HPX_TEST_LTE(static_cast<std::uint32_t>(2), num_localities);

    // Force the fallback path by setting threshold above num_localities.
    auto const fb_clients = create_hierarchical_communicator(
        "/test/hflback_dist_fb/", num_sites_arg(num_localities),
        this_site_arg(this_locality), arity_arg(2), generation_arg(),
        root_site_arg(0), flat_fallback_threshold_arg(1024));

    HPX_TEST_EQ(fb_clients.size(), static_cast<std::size_t>(1));

    std::uint32_t const value = this_locality + 1;
    std::uint32_t const fb_result =
        all_reduce(fb_clients, std::uint32_t(value), std::plus<std::uint32_t>{},
            this_site_arg(this_locality), generation_arg(1))
            .get();

    std::uint32_t expected = 0;
    for (std::uint32_t j = 0; j != num_localities; ++j)
    {
        expected += (j + 1);
    }
    HPX_TEST_EQ(fb_result, expected);

    // Force the tree path by setting threshold to 0; results must match.
    // With 2 localities and arity 2 a tree is structurally impossible
    // (arity >= num_sites dispatches flat), so this leg needs at least 3.
    if (num_localities >= 3)
    {
        auto const tree_clients = create_hierarchical_communicator(
            "/test/hflback_dist_tree/", num_sites_arg(num_localities),
            this_site_arg(this_locality), arity_arg(2), generation_arg(),
            root_site_arg(0), flat_fallback_threshold_arg(0));

        // Assert the dispatch predicate so the comparison can never
        // silently degenerate to flat-vs-flat.
        HPX_TEST_LT(static_cast<std::size_t>(tree_clients.get_arity()),
            static_cast<std::size_t>(num_localities));

        std::uint32_t const tree_result = all_reduce(tree_clients,
            std::uint32_t(value), std::plus<std::uint32_t>{},
            this_site_arg(this_locality), generation_arg(1))
                                              .get();

        HPX_TEST_EQ(tree_result, expected);
    }
}

// Distributed test for all_to_all flat fallback: mirrors test_distributed_fallback
// but exercises the all_to_all flat dispatch branch (arity_val >= num_sites_val).
void test_distributed_fallback_all_to_all()
{
    std::uint32_t const this_locality = hpx::get_locality_id();
    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    HPX_TEST_LTE(static_cast<std::uint32_t>(2), num_localities);

    // Site s sends {s*N+0, s*N+1, ..., s*N+(N-1)}
    std::vector<std::uint32_t> send_data(num_localities);
    for (std::uint32_t d = 0; d != num_localities; ++d)
    {
        send_data[d] = this_locality * num_localities + d;
    }

    // Expected: result[i] = i * N + this_locality
    std::vector<std::uint32_t> expected(num_localities);
    for (std::uint32_t i = 0; i != num_localities; ++i)
    {
        expected[i] = i * num_localities + this_locality;
    }

    // Force flat fallback path.
    auto const fb_clients = create_hierarchical_communicator(
        "/test/hflback_dist_a2a_fb/", num_sites_arg(num_localities),
        this_site_arg(this_locality), arity_arg(2), generation_arg(),
        root_site_arg(0), flat_fallback_threshold_arg(1024));

    HPX_TEST_EQ(fb_clients.size(), static_cast<std::size_t>(1));

    auto const fb_result =
        all_to_all(fb_clients, std::vector<std::uint32_t>(send_data),
            this_site_arg(this_locality), generation_arg(1))
            .get();

    HPX_TEST_EQ(fb_result.size(), expected.size());
    for (std::size_t i = 0; i != expected.size(); ++i)
    {
        HPX_TEST_EQ(fb_result[i], expected[i]);
    }

    // Force tree path; results must match. With 2 localities and arity 2 a
    // tree is structurally impossible (arity >= num_sites dispatches flat),
    // so this leg needs at least 3.
    if (num_localities >= 3)
    {
        auto const tree_clients = create_hierarchical_communicator(
            "/test/hflback_dist_a2a_tree/", num_sites_arg(num_localities),
            this_site_arg(this_locality), arity_arg(2), generation_arg(),
            root_site_arg(0), flat_fallback_threshold_arg(0));

        // Assert the dispatch predicate so the comparison can never
        // silently degenerate to flat-vs-flat.
        HPX_TEST_LT(static_cast<std::size_t>(tree_clients.get_arity()),
            static_cast<std::size_t>(num_localities));

        auto const tree_result =
            all_to_all(tree_clients, std::vector<std::uint32_t>(send_data),
                this_site_arg(this_locality), generation_arg(1))
                .get();

        HPX_TEST_EQ(tree_result.size(), expected.size());
        for (std::size_t i = 0; i != expected.size(); ++i)
        {
            HPX_TEST_EQ(tree_result[i], expected[i]);
        }
    }
}

// Local test (single-process, multi-thread sites), exercised from locality 0
// only and parameterized over the fallback threshold: 1024 forces the flat
// path, 0 forces the tree path.
void test_local_fallback(std::uint32_t num_sites, std::size_t threshold)
{
    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            std::string const basename =
                "/test/hflback_local_t" + std::to_string(threshold) + "/";
            auto const clients = create_hierarchical_communicator(
                basename.c_str(), num_sites_arg(num_sites), this_site_arg(site),
                arity_arg(2), generation_arg(), root_site_arg(0),
                flat_fallback_threshold_arg(threshold));

            if (threshold == 0)
            {
                // Assert the dispatch predicate so the comparison can never
                // silently degenerate to flat-vs-flat.
                HPX_TEST_LT(static_cast<std::size_t>(clients.get_arity()),
                    static_cast<std::size_t>(num_sites));
            }
            else
            {
                HPX_TEST_EQ(clients.size(), static_cast<std::size_t>(1));
            }

            std::uint32_t const value = site + 1;
            std::uint32_t const result = all_reduce(clients,
                std::uint32_t(value), std::plus<std::uint32_t>{},
                this_site_arg(site), generation_arg(1))
                                             .get();

            std::uint32_t expected = 0;
            for (std::uint32_t j = 0; j != num_sites; ++j)
            {
                expected += (j + 1);
            }
            HPX_TEST_EQ(result, expected);
        }));
    }

    hpx::wait_all(std::move(sites));
}

// Local all_to_all test, parameterized over the fallback threshold like
// test_local_fallback: 1024 exercises the arity_val >= num_sites_val flat
// dispatch branch, 0 exercises the 3-phase tree path.
void test_local_fallback_all_to_all(
    std::uint32_t num_sites, std::size_t threshold)
{
    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            std::string const basename =
                "/test/hflback_local_a2a_t" + std::to_string(threshold) + "/";
            auto const clients = create_hierarchical_communicator(
                basename.c_str(), num_sites_arg(num_sites), this_site_arg(site),
                arity_arg(2), generation_arg(), root_site_arg(0),
                flat_fallback_threshold_arg(threshold));

            if (threshold == 0)
            {
                // Assert the dispatch predicate so the comparison can never
                // silently degenerate to flat-vs-flat.
                HPX_TEST_LT(static_cast<std::size_t>(clients.get_arity()),
                    static_cast<std::size_t>(num_sites));
            }
            else
            {
                HPX_TEST_EQ(clients.size(), static_cast<std::size_t>(1));
            }

            // Site s sends {s*N+0, ..., s*N+(N-1)}
            std::vector<std::uint32_t> send_data(num_sites);
            for (std::uint32_t d = 0; d != num_sites; ++d)
            {
                send_data[d] = site * num_sites + d;
            }

            auto const result = all_to_all(clients, HPX_MOVE(send_data),
                this_site_arg(site), generation_arg(1))
                                    .get();

            // Expected: result[i] = i * N + site
            HPX_TEST_EQ(result.size(), static_cast<std::size_t>(num_sites));
            for (std::uint32_t i = 0; i != num_sites; ++i)
            {
                HPX_TEST_EQ(result[i], i * num_sites + site);
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
        test_distributed_fallback();
        test_distributed_fallback_all_to_all();
    }
#endif

    if (hpx::get_locality_id() == 0)
    {
        // Flat legs: threshold above all site counts forces the fallback.
        for (std::uint32_t n : {2u, 4u, 8u})
        {
            test_local_fallback(n, 1024);
            test_local_fallback_all_to_all(n, 1024);
        }

        // Tree legs: threshold 0 forces real trees (n == 2 cannot tree at
        // arity 2 because arity >= num_sites dispatches flat).
        for (std::uint32_t n : {3u, 5u, 8u})
        {
            test_local_fallback(n, 0);
            test_local_fallback_all_to_all(n, 0);
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
