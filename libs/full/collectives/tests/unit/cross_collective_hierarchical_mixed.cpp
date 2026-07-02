//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Additional cross-collective coverage for hierarchical communicators,
// complementing cross_collective_hierarchical.cpp (which pins the documented
// contract with pairwise, explicit-generation mixes). This file:
//   - chains four different collectives (broadcast, all_to_all, barrier,
//     scatter) on one shared instance with a single strictly consecutive
//     generation counter, so four different per-communicator consumption
//     patterns must stay in lock-step;
//   - separately exercises the default (auto) generation path across
//     broadcast, gather, reduce and scatter on one shared instance; and
//   - runs both locally and distributed at non-power-of-arity site counts
//     (5 and 7) with arity 2 and 3, forcing real trees with uneven groups.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

using namespace hpx::collectives;

constexpr std::uint32_t iterations = 4;

void wait_for_sites(std::vector<hpx::future<void>>& sites)
{
    hpx::wait_all(sites);
    for (auto& site : sites)
    {
        site.get();
    }
}

std::vector<std::int32_t> make_scatter_values(
    std::uint32_t const num_sites, std::uint32_t const iteration)
{
    std::vector<std::int32_t> values(num_sites);
    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        values[site] =
            70000 + static_cast<std::int32_t>(100 * iteration + site);
    }
    return values;
}

void run_explicit_mixed_sequence(std::string const& basename,
    std::uint32_t const num_sites, std::uint32_t const site, int const arity)
{
    auto const comms = create_hierarchical_communicator(basename.c_str(),
        num_sites_arg(num_sites), this_site_arg(site), arity_arg(arity),
        generation_arg(), root_site_arg(), flat_fallback_threshold_arg(0));

    std::size_t generation = 0;
    for (std::uint32_t i = 0; i != iterations; ++i)
    {
        std::int32_t const broadcast_value =
            10000 + static_cast<std::int32_t>(i);
        std::int32_t received_broadcast = 0;
        if (site == 0)
        {
            received_broadcast = broadcast_to(hpx::launch::sync, comms,
                std::int32_t(broadcast_value), this_site_arg(site),
                generation_arg(++generation));
        }
        else
        {
            received_broadcast = broadcast_from<std::int32_t>(hpx::launch::sync,
                comms, this_site_arg(site), generation_arg(++generation));
        }
        HPX_TEST_EQ(received_broadcast, broadcast_value);

        std::vector<std::int32_t> outgoing(num_sites);
        for (std::uint32_t dest = 0; dest != num_sites; ++dest)
        {
            outgoing[dest] =
                static_cast<std::int32_t>(100000 * i + 1000 * site + dest);
        }

        std::vector<std::int32_t> const exchanged =
            all_to_all(hpx::launch::sync, comms, HPX_MOVE(outgoing),
                this_site_arg(site), generation_arg(++generation));

        HPX_TEST_EQ(exchanged.size(), static_cast<std::size_t>(num_sites));
        for (std::uint32_t source = 0; source != num_sites; ++source)
        {
            HPX_TEST_EQ(exchanged[source],
                static_cast<std::int32_t>(100000 * i + 1000 * source + site));
        }

        std::int32_t const inclusive = inclusive_scan(hpx::launch::sync, comms,
            static_cast<std::int32_t>(site + i), std::plus<std::int32_t>{},
            this_site_arg(site), generation_arg(++generation));
        std::int32_t expected_inclusive = 0;
        for (std::uint32_t source = 0; source != site + 1; ++source)
        {
            expected_inclusive += static_cast<std::int32_t>(source + i);
        }
        HPX_TEST_EQ(inclusive, expected_inclusive);

        std::int32_t const exclusive = exclusive_scan(hpx::launch::sync, comms,
            static_cast<std::int32_t>(site + i),
            static_cast<std::int32_t>(90000 + i), std::plus<std::int32_t>{},
            this_site_arg(site), generation_arg(++generation));
        std::int32_t expected_exclusive = 90000 + static_cast<std::int32_t>(i);
        for (std::uint32_t source = 0; source != site; ++source)
        {
            expected_exclusive += static_cast<std::int32_t>(source + i);
        }
        HPX_TEST_EQ(exclusive, expected_exclusive);

        barrier(hpx::launch::sync, comms, this_site_arg(site),
            generation_arg(++generation));

        std::int32_t scattered = 0;
        if (site == 0)
        {
            scattered = scatter_to(hpx::launch::sync, comms,
                make_scatter_values(num_sites, i), this_site_arg(site),
                generation_arg(++generation));
        }
        else
        {
            scattered = scatter_from<std::int32_t>(hpx::launch::sync, comms,
                this_site_arg(site), generation_arg(++generation));
        }
        HPX_TEST_EQ(
            scattered, static_cast<std::int32_t>(70000 + 100 * i + site));
    }
}

void test_local_explicit_mixed_sequence(
    std::uint32_t const num_sites, int const arity)
{
    std::string const basename =
        "/test/cross_collective_hierarchical_mixed/local/" +
        std::to_string(num_sites) + "/" + std::to_string(arity) + "/";

    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            run_explicit_mixed_sequence(basename, num_sites, site, arity);
        }));
    }

    wait_for_sites(sites);
}

void run_default_generation_sequence(std::string const& basename,
    std::uint32_t const num_sites, std::uint32_t const site, int const arity)
{
    auto const comms = create_hierarchical_communicator(basename.c_str(),
        num_sites_arg(num_sites), this_site_arg(site), arity_arg(arity),
        generation_arg(), root_site_arg(), flat_fallback_threshold_arg(0));

    for (std::uint32_t i = 0; i != iterations; ++i)
    {
        std::int32_t const broadcast_value =
            20000 + static_cast<std::int32_t>(i);
        std::int32_t received_broadcast = 0;
        if (site == 0)
        {
            received_broadcast = broadcast_to(hpx::launch::sync, comms,
                std::int32_t(broadcast_value), this_site_arg(site),
                generation_arg());
        }
        else
        {
            received_broadcast = broadcast_from<std::int32_t>(hpx::launch::sync,
                comms, this_site_arg(site), generation_arg());
        }
        HPX_TEST_EQ(received_broadcast, broadcast_value);
    }

    for (std::uint32_t i = 0; i != iterations; ++i)
    {
        if (site == 0)
        {
            std::vector<std::int32_t> const gathered =
                gather_here(hpx::launch::sync, comms,
                    static_cast<std::int32_t>(30000 + i + site),
                    this_site_arg(site), generation_arg());

            HPX_TEST_EQ(gathered.size(), static_cast<std::size_t>(num_sites));
            for (std::uint32_t source = 0; source != num_sites; ++source)
            {
                HPX_TEST_EQ(gathered[source],
                    static_cast<std::int32_t>(30000 + i + source));
            }
        }
        else
        {
            gather_there(comms, static_cast<std::int32_t>(30000 + i + site),
                this_site_arg(site), generation_arg())
                .get();
        }
    }

    for (std::uint32_t i = 0; i != iterations; ++i)
    {
        if (site == 0)
        {
            std::int32_t const reduced = reduce_here(hpx::launch::sync, comms,
                static_cast<std::int32_t>(40000 + i + site),
                std::plus<std::int32_t>{}, this_site_arg(site),
                generation_arg());

            std::int32_t expected = 0;
            for (std::uint32_t source = 0; source != num_sites; ++source)
            {
                expected += static_cast<std::int32_t>(40000 + i + source);
            }
            HPX_TEST_EQ(reduced, expected);
        }
        else
        {
            reduce_there(comms, static_cast<std::int32_t>(40000 + i + site),
                std::plus<std::int32_t>{}, this_site_arg(site),
                generation_arg())
                .get();
        }
    }

    for (std::uint32_t i = 0; i != iterations; ++i)
    {
        std::int32_t scattered = 0;
        if (site == 0)
        {
            scattered = scatter_to(hpx::launch::sync, comms,
                make_scatter_values(num_sites, i), this_site_arg(site),
                generation_arg());
        }
        else
        {
            scattered = scatter_from<std::int32_t>(hpx::launch::sync, comms,
                this_site_arg(site), generation_arg());
        }
        HPX_TEST_EQ(
            scattered, static_cast<std::int32_t>(70000 + 100 * i + site));
    }
}

void test_local_default_generation(
    std::uint32_t const num_sites, int const arity)
{
    std::string const basename =
        "/test/cross_collective_hierarchical_mixed/default/local/" +
        std::to_string(num_sites) + "/" + std::to_string(arity) + "/";

    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            run_default_generation_sequence(basename, num_sites, site, arity);
        }));
    }

    wait_for_sites(sites);
}

void test_distributed_explicit_mixed_sequence()
{
    std::uint32_t const site = hpx::get_locality_id();
    std::uint32_t const num_sites = hpx::get_num_localities(hpx::launch::sync);

    if (num_sites < 2)
    {
        return;
    }

    int const arity = num_sites % 3 == 1 ? 2 : 3;
    run_explicit_mixed_sequence(
        "/test/cross_collective_hierarchical_mixed/distributed/explicit/",
        num_sites, site, arity);
}

void test_distributed_default_generation()
{
    std::uint32_t const site = hpx::get_locality_id();
    std::uint32_t const num_sites = hpx::get_num_localities(hpx::launch::sync);

    if (num_sites < 2)
    {
        return;
    }

    int const arity = num_sites % 3 == 1 ? 2 : 3;
    run_default_generation_sequence(
        "/test/cross_collective_hierarchical_mixed/distributed/default/",
        num_sites, site, arity);
}

// Flat-fallback sharing. With the default threshold (16), a small site count
// collapses to a single flat communicator (create_hierarchical_communicator
// overrides arity to num_sites). Mixing collectives on that flat instance must
// still keep the shared generation sequence gap-free: every collective has to
// advance the flat gate by two per call, just like in tree mode. This is a
// regression guard for the flat fast paths of all_gather/all_reduce/all_to_all,
// which previously advanced the flat gate by only one.
void test_local_flat_fallback_sharing(std::uint32_t const num_sites)
{
    std::string const basename =
        "/test/cross_collective_hierarchical_mixed/flat_fallback/" +
        std::to_string(num_sites) + "/";

    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            // No flat_fallback_threshold_arg, so the default (16) applies and a
            // site count below it produces a single flat communicator.
            auto const comms = create_hierarchical_communicator(
                basename.c_str(), num_sites_arg(num_sites), this_site_arg(site),
                arity_arg(2), generation_arg(), root_site_arg());

            std::size_t generation = 0;
            for (std::uint32_t i = 0; i != iterations; ++i)
            {
                // all_gather (flat fast path) mixed with broadcast (step 2).
                std::vector<std::int32_t> const gathered =
                    all_gather(hpx::launch::sync, comms,
                        static_cast<std::int32_t>(site + i),
                        this_site_arg(site), generation_arg(++generation));
                HPX_TEST_EQ(
                    gathered.size(), static_cast<std::size_t>(num_sites));
                for (std::uint32_t source = 0; source != num_sites; ++source)
                {
                    HPX_TEST_EQ(gathered[source],
                        static_cast<std::int32_t>(source + i));
                }

                // all_reduce (flat fast path).
                std::int32_t const reduced = all_reduce(hpx::launch::sync,
                    comms, static_cast<std::int32_t>(site + i),
                    std::plus<std::int32_t>{}, this_site_arg(site),
                    generation_arg(++generation));
                std::int32_t expected_sum = 0;
                for (std::uint32_t source = 0; source != num_sites; ++source)
                {
                    expected_sum += static_cast<std::int32_t>(source + i);
                }
                HPX_TEST_EQ(reduced, expected_sum);

                // inclusive_scan and exclusive_scan (flat fast paths).
                std::int32_t const inclusive = inclusive_scan(hpx::launch::sync,
                    comms, static_cast<std::int32_t>(site + i),
                    std::plus<std::int32_t>{}, this_site_arg(site),
                    generation_arg(++generation));
                std::int32_t expected_inclusive = 0;
                for (std::uint32_t source = 0; source != site + 1; ++source)
                {
                    expected_inclusive += static_cast<std::int32_t>(source + i);
                }
                HPX_TEST_EQ(inclusive, expected_inclusive);

                std::int32_t const exclusive = exclusive_scan(hpx::launch::sync,
                    comms, static_cast<std::int32_t>(site + i),
                    static_cast<std::int32_t>(60000 + i),
                    std::plus<std::int32_t>{}, this_site_arg(site),
                    generation_arg(++generation));
                std::int32_t expected_exclusive =
                    60000 + static_cast<std::int32_t>(i);
                for (std::uint32_t source = 0; source != site; ++source)
                {
                    expected_exclusive += static_cast<std::int32_t>(source + i);
                }
                HPX_TEST_EQ(exclusive, expected_exclusive);

                // all_to_all (flat fast path).
                std::vector<std::int32_t> outgoing(num_sites);
                for (std::uint32_t dest = 0; dest != num_sites; ++dest)
                {
                    outgoing[dest] =
                        static_cast<std::int32_t>(1000 * i + 10 * site + dest);
                }
                std::vector<std::int32_t> const exchanged =
                    all_to_all(hpx::launch::sync, comms, HPX_MOVE(outgoing),
                        this_site_arg(site), generation_arg(++generation));
                HPX_TEST_EQ(
                    exchanged.size(), static_cast<std::size_t>(num_sites));
                for (std::uint32_t source = 0; source != num_sites; ++source)
                {
                    HPX_TEST_EQ(exchanged[source],
                        static_cast<std::int32_t>(
                            1000 * i + 10 * source + site));
                }

                // broadcast (single-pass, step 2).
                std::int32_t const broadcast_value =
                    50000 + static_cast<std::int32_t>(i);
                std::int32_t received = 0;
                if (site == 0)
                {
                    received = broadcast_to(hpx::launch::sync, comms,
                        std::int32_t(broadcast_value), this_site_arg(site),
                        generation_arg(++generation));
                }
                else
                {
                    received =
                        broadcast_from<std::int32_t>(hpx::launch::sync, comms,
                            this_site_arg(site), generation_arg(++generation));
                }
                HPX_TEST_EQ(received, broadcast_value);
            }
        }));
    }

    wait_for_sites(sites);
}

// Generation 0 is invalid (generations must be positive). The hierarchical
// collectives must reject it with bad_parameter, not silently treat it as an
// auto generation -- both the collectives that require an explicit generation
// (all_gather here) and the single-pass ones that map through
// hierarchical_run_params (broadcast here).
void test_local_zero_generation_rejected(std::uint32_t const num_sites)
{
    std::string const basename =
        "/test/cross_collective_hierarchical_mixed/zero_gen/" +
        std::to_string(num_sites) + "/";

    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            auto const comms = create_hierarchical_communicator(
                basename.c_str(), num_sites_arg(num_sites), this_site_arg(site),
                arity_arg(2), generation_arg(), root_site_arg(),
                flat_fallback_threshold_arg(0));

            bool all_gather_rejected = false;
            try
            {
                all_gather(hpx::launch::sync, comms,
                    static_cast<std::int32_t>(site), this_site_arg(site),
                    generation_arg(0));
            }
            catch (hpx::exception const&)
            {
                all_gather_rejected = true;
            }
            HPX_TEST(all_gather_rejected);

            bool inclusive_scan_rejected = false;
            try
            {
                inclusive_scan(hpx::launch::sync, comms,
                    static_cast<std::int32_t>(site), std::plus<std::int32_t>{},
                    this_site_arg(site), generation_arg(0));
            }
            catch (hpx::exception const&)
            {
                inclusive_scan_rejected = true;
            }
            HPX_TEST(inclusive_scan_rejected);

            bool exclusive_scan_rejected = false;
            try
            {
                exclusive_scan(hpx::launch::sync, comms,
                    static_cast<std::int32_t>(site), std::int32_t(0),
                    std::plus<std::int32_t>{}, this_site_arg(site),
                    generation_arg(0));
            }
            catch (hpx::exception const&)
            {
                exclusive_scan_rejected = true;
            }
            HPX_TEST(exclusive_scan_rejected);

            bool broadcast_rejected = false;
            try
            {
                if (site == 0)
                {
                    broadcast_to(hpx::launch::sync, comms, std::int32_t(1),
                        this_site_arg(site), generation_arg(0));
                }
                else
                {
                    broadcast_from<std::int32_t>(hpx::launch::sync, comms,
                        this_site_arg(site), generation_arg(0));
                }
            }
            catch (hpx::exception const&)
            {
                broadcast_rejected = true;
            }
            HPX_TEST(broadcast_rejected);
        }));
    }

    wait_for_sites(sites);
}

int hpx_main()
{
    if (hpx::get_locality_id() == 0)
    {
        for (std::uint32_t num_sites : {5u, 7u})
        {
            for (int arity : {2, 3})
            {
                test_local_explicit_mixed_sequence(num_sites, arity);
                test_local_default_generation(num_sites, arity);
            }
            test_local_flat_fallback_sharing(num_sites);
            test_local_zero_generation_rejected(num_sites);
        }
    }

#if defined(HPX_HAVE_NETWORKING)
    test_distributed_explicit_mixed_sequence();
    test_distributed_default_generation();
#endif

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
