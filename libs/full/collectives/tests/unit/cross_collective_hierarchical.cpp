//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Cross-collective use of hierarchical communicators. Every hierarchical
// collective advances each sub-communicator it touches by exactly two internal
// generations per call (see the note on create_hierarchical_communicator in
// create_communicator.hpp), so a single hierarchical_communicator instance can
// be shared freely across collective operations as long as every call uses an
// explicit, strictly consecutive generation number. These tests interleave
// different collectives on one instance with a single shared generation
// counter and verify the results of each operation, including the all_to_all +
// all_reduce mix that used to deadlock before the generation step was unified.

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

constexpr int ITERATIONS = 4;

void wait_for_sites(std::vector<hpx::future<void>>& sites)
{
    hpx::wait_all(sites);
    for (auto& site : sites)
    {
        site.get();
    }
}

// Interleave all_reduce and all_gather on one hierarchical communicator
// instance, advancing a single shared, strictly consecutive user-generation
// counter. This is legal because both operations consume the identical
// 2k-1/2k internal generation scheme on every sub-communicator.
void test_same_instance_all_reduce_all_gather()
{
    constexpr std::uint32_t num_sites = 8;

    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            auto const comms = create_hierarchical_communicator(
                "/test/cross_collective/same_instance/",
                num_sites_arg(num_sites), this_site_arg(site), arity_arg(2),
                generation_arg(), root_site_arg(),
                flat_fallback_threshold_arg(0));

            std::size_t generation = 0;
            for (int i = 0; i != ITERATIONS; ++i)
            {
                std::uint32_t const value = site + i;

                std::uint32_t const reduced = all_reduce(hpx::launch::sync,
                    comms, std::uint32_t(value), std::plus<std::uint32_t>{},
                    this_site_arg(site), generation_arg(++generation));

                std::uint32_t expected_sum = 0;
                for (std::uint32_t j = 0; j != num_sites; ++j)
                {
                    expected_sum += j + i;
                }
                HPX_TEST_EQ(reduced, expected_sum);

                std::vector<std::uint32_t> const gathered =
                    all_gather(hpx::launch::sync, comms, std::uint32_t(value),
                        this_site_arg(site), generation_arg(++generation));

                HPX_TEST_EQ(
                    gathered.size(), static_cast<std::size_t>(num_sites));
                for (std::uint32_t j = 0; j != num_sites; ++j)
                {
                    HPX_TEST_EQ(gathered[j], j + i);
                }
            }
        }));
    }

    wait_for_sites(sites);
}

// Interleave all_to_all and all_reduce, each on its own hierarchical
// communicator instance with an independent, strictly consecutive
// user-generation counter. Separate instances are always valid; the
// same-instance variant below additionally exercises sharing one instance.
void test_separate_instances_all_to_all_all_reduce()
{
    constexpr std::uint32_t num_sites = 8;

    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            auto const a2a_comms = create_hierarchical_communicator(
                "/test/cross_collective/separate_a2a/",
                num_sites_arg(num_sites), this_site_arg(site), arity_arg(2),
                generation_arg(), root_site_arg(),
                flat_fallback_threshold_arg(0));

            auto const reduce_comms = create_hierarchical_communicator(
                "/test/cross_collective/separate_all_reduce/",
                num_sites_arg(num_sites), this_site_arg(site), arity_arg(2),
                generation_arg(), root_site_arg(),
                flat_fallback_threshold_arg(0));

            std::size_t a2a_generation = 0;
            std::size_t reduce_generation = 0;
            for (int i = 0; i != ITERATIONS; ++i)
            {
                std::vector<std::uint32_t> values(num_sites);
                for (std::uint32_t j = 0; j != num_sites; ++j)
                {
                    values[j] = site * num_sites + j + i;
                }

                std::vector<std::uint32_t> const exchanged =
                    all_to_all(hpx::launch::sync, a2a_comms, std::move(values),
                        this_site_arg(site), generation_arg(++a2a_generation));

                HPX_TEST_EQ(
                    exchanged.size(), static_cast<std::size_t>(num_sites));
                for (std::uint32_t s = 0; s != num_sites; ++s)
                {
                    HPX_TEST_EQ(exchanged[s], s * num_sites + site + i);
                }

                std::uint32_t const reduced = all_reduce(hpx::launch::sync,
                    reduce_comms, std::uint32_t(site + i),
                    std::plus<std::uint32_t>{}, this_site_arg(site),
                    generation_arg(++reduce_generation));

                std::uint32_t expected_sum = 0;
                for (std::uint32_t j = 0; j != num_sites; ++j)
                {
                    expected_sum += j + i;
                }
                HPX_TEST_EQ(reduced, expected_sum);
            }
        }));
    }

    wait_for_sites(sites);
}

// Interleave all_to_all and all_reduce on ONE hierarchical communicator
// instance with a single shared, strictly consecutive user-generation
// counter. Every hierarchical collective now advances every communicator by
// two internal generations per call (all_to_all's inter-group exchange is
// padded to two), so a single instance can be shared across collective
// families. This is the case that used to deadlock.
void test_same_instance_all_to_all_all_reduce()
{
    constexpr std::uint32_t num_sites = 8;

    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            auto const comms = create_hierarchical_communicator(
                "/test/cross_collective/same_instance_a2a_reduce/",
                num_sites_arg(num_sites), this_site_arg(site), arity_arg(2),
                generation_arg(), root_site_arg(),
                flat_fallback_threshold_arg(0));

            std::size_t generation = 0;
            for (int i = 0; i != ITERATIONS; ++i)
            {
                std::vector<std::uint32_t> values(num_sites);
                for (std::uint32_t j = 0; j != num_sites; ++j)
                {
                    values[j] = site * num_sites + j + i;
                }

                std::vector<std::uint32_t> const exchanged =
                    all_to_all(hpx::launch::sync, comms, std::move(values),
                        this_site_arg(site), generation_arg(++generation));

                HPX_TEST_EQ(
                    exchanged.size(), static_cast<std::size_t>(num_sites));
                for (std::uint32_t s = 0; s != num_sites; ++s)
                {
                    HPX_TEST_EQ(exchanged[s], s * num_sites + site + i);
                }

                std::uint32_t const reduced = all_reduce(hpx::launch::sync,
                    comms, std::uint32_t(site + i), std::plus<std::uint32_t>{},
                    this_site_arg(site), generation_arg(++generation));

                std::uint32_t expected_sum = 0;
                for (std::uint32_t j = 0; j != num_sites; ++j)
                {
                    expected_sum += j + i;
                }
                HPX_TEST_EQ(reduced, expected_sum);
            }
        }));
    }

    wait_for_sites(sites);
}

// Interleave broadcast and all_gather on ONE hierarchical communicator
// instance with a single shared, strictly consecutive user-generation
// counter. Standalone broadcast now advances every communicator by two
// internal generations per call (the same as all_gather), so the two can
// share an instance.
void test_same_instance_broadcast_all_gather()
{
    constexpr std::uint32_t num_sites = 8;

    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            auto const comms = create_hierarchical_communicator(
                "/test/cross_collective/same_instance_bcast_gather/",
                num_sites_arg(num_sites), this_site_arg(site), arity_arg(2),
                generation_arg(), root_site_arg(),
                flat_fallback_threshold_arg(0));

            std::size_t generation = 0;
            for (int i = 0; i != ITERATIONS; ++i)
            {
                std::uint32_t const bcast_value = 42 + i;
                if (site == 0)
                {
                    std::uint32_t const received = broadcast_to(
                        hpx::launch::sync, comms, std::uint32_t(bcast_value),
                        this_site_arg(site), generation_arg(++generation));
                    HPX_TEST_EQ(received, bcast_value);
                }
                else
                {
                    std::uint32_t const received =
                        broadcast_from<std::uint32_t>(hpx::launch::sync, comms,
                            this_site_arg(site), generation_arg(++generation));
                    HPX_TEST_EQ(received, bcast_value);
                }

                std::vector<std::uint32_t> const gathered = all_gather(
                    hpx::launch::sync, comms, std::uint32_t(site + i),
                    this_site_arg(site), generation_arg(++generation));

                HPX_TEST_EQ(
                    gathered.size(), static_cast<std::size_t>(num_sites));
                for (std::uint32_t j = 0; j != num_sites; ++j)
                {
                    HPX_TEST_EQ(gathered[j], j + i);
                }
            }
        }));
    }

    wait_for_sites(sites);
}

// Interleave gather and all_reduce on ONE hierarchical communicator instance
// with a single shared, strictly consecutive user-generation counter.
// Standalone gather now advances every communicator by two internal
// generations per call, matching all_reduce.
void test_same_instance_gather_all_reduce()
{
    constexpr std::uint32_t num_sites = 8;

    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            auto const comms = create_hierarchical_communicator(
                "/test/cross_collective/same_instance_gather_reduce/",
                num_sites_arg(num_sites), this_site_arg(site), arity_arg(2),
                generation_arg(), root_site_arg(),
                flat_fallback_threshold_arg(0));

            std::size_t generation = 0;
            for (int i = 0; i != ITERATIONS; ++i)
            {
                std::uint32_t const value = site + 42 + i;
                if (site == 0)
                {
                    std::vector<std::uint32_t> const gathered =
                        gather_here(comms, std::uint32_t(value),
                            this_site_arg(site), generation_arg(++generation))
                            .get();

                    HPX_TEST_EQ(
                        gathered.size(), static_cast<std::size_t>(num_sites));
                    for (std::uint32_t j = 0; j != num_sites; ++j)
                    {
                        HPX_TEST_EQ(gathered[j], j + 42 + i);
                    }
                }
                else
                {
                    gather_there(comms, std::uint32_t(value),
                        this_site_arg(site), generation_arg(++generation))
                        .get();
                }

                std::uint32_t const reduced = all_reduce(hpx::launch::sync,
                    comms, std::uint32_t(site + i), std::plus<std::uint32_t>{},
                    this_site_arg(site), generation_arg(++generation));

                std::uint32_t expected_sum = 0;
                for (std::uint32_t j = 0; j != num_sites; ++j)
                {
                    expected_sum += j + i;
                }
                HPX_TEST_EQ(reduced, expected_sum);
            }
        }));
    }

    wait_for_sites(sites);
}

// Interleave scatter and all_reduce on ONE hierarchical communicator instance
// with a single shared, strictly consecutive user-generation counter.
// Standalone scatter now advances every communicator by two internal
// generations per call, matching all_reduce.
void test_same_instance_scatter_all_reduce()
{
    constexpr std::uint32_t num_sites = 8;

    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            auto const comms = create_hierarchical_communicator(
                "/test/cross_collective/same_instance_scatter_reduce/",
                num_sites_arg(num_sites), this_site_arg(site), arity_arg(2),
                generation_arg(), root_site_arg(),
                flat_fallback_threshold_arg(0));

            std::size_t generation = 0;
            for (int i = 0; i != ITERATIONS; ++i)
            {
                std::uint32_t const expected = site + 42 + i;
                if (site == 0)
                {
                    std::vector<std::uint32_t> data(num_sites);
                    for (std::uint32_t j = 0; j != num_sites; ++j)
                    {
                        data[j] = j + 42 + i;
                    }
                    std::uint32_t const received =
                        scatter_to(comms, std::move(data), this_site_arg(site),
                            generation_arg(++generation))
                            .get();
                    HPX_TEST_EQ(received, expected);
                }
                else
                {
                    std::uint32_t const received =
                        scatter_from<std::uint32_t>(comms, this_site_arg(site),
                            generation_arg(++generation))
                            .get();
                    HPX_TEST_EQ(received, expected);
                }

                std::uint32_t const reduced = all_reduce(hpx::launch::sync,
                    comms, std::uint32_t(site + i), std::plus<std::uint32_t>{},
                    this_site_arg(site), generation_arg(++generation));

                std::uint32_t expected_sum = 0;
                for (std::uint32_t j = 0; j != num_sites; ++j)
                {
                    expected_sum += j + i;
                }
                HPX_TEST_EQ(reduced, expected_sum);
            }
        }));
    }

    wait_for_sites(sites);
}

// Interleave inclusive_scan, exclusive_scan and all_reduce on ONE
// hierarchical communicator instance with a single shared, strictly
// consecutive user-generation counter. The scan overloads are two-phase
// collectives and must consume the same 2k-1/2k generation block as the
// existing hierarchical collectives.
void test_same_instance_scans_all_reduce()
{
    constexpr std::uint32_t num_sites = 8;

    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            auto const comms = create_hierarchical_communicator(
                "/test/cross_collective/same_instance_scans_reduce/",
                num_sites_arg(num_sites), this_site_arg(site), arity_arg(2),
                generation_arg(), root_site_arg(),
                flat_fallback_threshold_arg(0));

            std::size_t generation = 0;
            for (int i = 0; i != ITERATIONS; ++i)
            {
                std::uint32_t const value = site + i;

                std::uint32_t const inclusive =
                    inclusive_scan(hpx::launch::sync, comms,
                        std::uint32_t(value), std::plus<std::uint32_t>{},
                        this_site_arg(site), generation_arg(++generation));

                std::uint32_t expected_inclusive = 0;
                for (std::uint32_t j = 0; j != site + 1; ++j)
                {
                    expected_inclusive += j + i;
                }
                HPX_TEST_EQ(inclusive, expected_inclusive);

                std::uint32_t const exclusive = exclusive_scan(
                    hpx::launch::sync, comms, std::uint32_t(value),
                    std::uint32_t(100 + i), std::plus<std::uint32_t>{},
                    this_site_arg(site), generation_arg(++generation));

                std::uint32_t expected_exclusive = 100 + i;
                for (std::uint32_t j = 0; j != site; ++j)
                {
                    expected_exclusive += j + i;
                }
                HPX_TEST_EQ(exclusive, expected_exclusive);

                std::uint32_t const reduced = all_reduce(hpx::launch::sync,
                    comms, std::uint32_t(value), std::plus<std::uint32_t>{},
                    this_site_arg(site), generation_arg(++generation));

                std::uint32_t expected_sum = 0;
                for (std::uint32_t j = 0; j != num_sites; ++j)
                {
                    expected_sum += j + i;
                }
                HPX_TEST_EQ(reduced, expected_sum);
            }
        }));
    }

    wait_for_sites(sites);
}

int hpx_main()
{
    if (hpx::get_locality_id() == 0)
    {
        test_same_instance_all_reduce_all_gather();
        test_separate_instances_all_to_all_all_reduce();
        test_same_instance_all_to_all_all_reduce();
        test_same_instance_broadcast_all_gather();
        test_same_instance_gather_all_reduce();
        test_same_instance_scatter_all_reduce();
        test_same_instance_scans_all_reduce();
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
