//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Cross-collective use of hierarchical communicators. A single
// hierarchical_communicator instance may only be shared by collectives with
// the same internal generation-consumption scheme (see the note on
// create_hierarchical_communicator in create_communicator.hpp): all_gather
// and all_reduce both consume internal generations 2k-1/2k per call on every
// sub-communicator and may be mixed on one instance, while all_to_all
// consumes generations at a different rate and needs its own instance.
//
// Deliberately not tested: mixing all_to_all with all_reduce on a single
// instance. That misuse desynchronizes the per-communicator generation
// sequences, and its failure mode is a deadlock, which cannot be asserted
// without timeout scaffolding.

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

    hpx::wait_all(std::move(sites));
}

// Interleave all_to_all and all_reduce, each on its own hierarchical
// communicator instance with an independent, strictly consecutive
// user-generation counter. This pins the supported usage pattern under the
// documented one-consumption-scheme-per-instance restriction.
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

    hpx::wait_all(std::move(sites));
}

int hpx_main()
{
    if (hpx::get_locality_id() == 0)
    {
        test_same_instance_all_reduce_all_gather();
        test_separate_instances_all_to_all_all_reduce();
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
