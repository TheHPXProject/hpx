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

constexpr char const* subtree_gs_basename = "/test/subtree_gs/";

#if defined(HPX_DEBUG)
constexpr int ITERATIONS = 10;
#else
constexpr int ITERATIONS = 50;
#endif

///////////////////////////////////////////////////////////////////////////
// Test: gather to rep then scatter back (round-trip), scalar payload.
// Each site contributes its site id. After gather, the rep should hold
// [left, left+1, ..., right]. After scatter, each site should get its
// own id back.
void test_round_trip(std::uint32_t num_sites, int arity)
{
    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            std::string basename(subtree_gs_basename);
            basename +=
                std::to_string(num_sites) + "_" + std::to_string(arity) + "/";

            auto const comms = create_hierarchical_communicator(
                basename.c_str(), num_sites_arg(num_sites), this_site_arg(site),
                arity_arg(arity), generation_arg(), root_site_arg(),
                flat_fallback_threshold_arg(0));

            auto [ns, ts] = comms.get_info();
            bool const is_rep =
                detail::is_top_level_rep(ts, ns, comms.get_arity());

            for (int iter = 0; iter != ITERATIONS; ++iter)
            {
                std::size_t const gather_gen = iter * 2 + 1;
                std::size_t const scatter_gen = iter * 2 + 2;

                if (is_rep)
                {
                    // Gather: collect subtree data.
                    auto gathered = detail::subtree_gather_at_top_rep(
                        comms, std::uint32_t(site), generation_arg(gather_gen));

                    // Verify: gathered should contain contiguous site ids.
                    auto groups =
                        detail::get_top_level_groups(ns, comms.get_arity());
                    auto gidx = detail::classify_site(ts, groups);

                    HPX_TEST_EQ(gathered.size(),
                        static_cast<std::size_t>(groups[gidx].size));
                    for (std::size_t j = 0; j != gathered.size(); ++j)
                    {
                        HPX_TEST_EQ(gathered[j],
                            static_cast<std::uint32_t>(groups[gidx].left + j));
                    }

                    // Scatter: send each site its value back.
                    auto my_val = detail::subtree_scatter_at_top_rep(
                        comms, HPX_MOVE(gathered), generation_arg(scatter_gen))
                                      .get();
                    HPX_TEST_EQ(my_val, site);
                }
                else
                {
                    // Send our value to the rep.
                    detail::subtree_send_to_top_rep(
                        comms, std::uint32_t(site), generation_arg(gather_gen));

                    // Receive our value back.
                    auto my_val =
                        detail::subtree_receive_from_top_rep<std::uint32_t>(
                            comms, generation_arg(scatter_gen))
                            .get();
                    HPX_TEST_EQ(my_val, site);
                }
            }
        }));
    }

    hpx::wait_all(std::move(sites));
}

///////////////////////////////////////////////////////////////////////////
// Test: vector payload round-trip. Each site contributes a vector<uint32_t>
// of size 3: {site*10, site*10+1, site*10+2}. After gather, the rep should
// have a flat vector of all contributions. After scatter, each site should
// get its original vector back.
void test_vector_payload(std::uint32_t num_sites, int arity)
{
    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);

    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([=]() {
            std::string basename(subtree_gs_basename);
            basename += "vec_" + std::to_string(num_sites) + "_" +
                std::to_string(arity) + "/";

            auto const comms = create_hierarchical_communicator(
                basename.c_str(), num_sites_arg(num_sites), this_site_arg(site),
                arity_arg(arity), generation_arg(), root_site_arg(),
                flat_fallback_threshold_arg(0));

            auto [ns, ts] = comms.get_info();
            bool const is_rep =
                detail::is_top_level_rep(ts, ns, comms.get_arity());

            std::vector<std::uint32_t> my_data = {
                site * 10, site * 10 + 1, site * 10 + 2};

            if (is_rep)
            {
                // Gather: each site contributes a vector<uint32_t>.
                // gather_data flattens one nesting level per gather step,
                // so the result is vector<vector<uint32_t>>.
                auto gathered = detail::subtree_gather_at_top_rep(
                    comms, HPX_MOVE(my_data), generation_arg(1));

                auto groups =
                    detail::get_top_level_groups(ns, comms.get_arity());
                auto gidx = detail::classify_site(ts, groups);

                // gathered is vector<vector<uint32_t>> with one entry
                // per subtree site (gather_data flattens one level per
                // gather step, preserving inner vectors).
                HPX_TEST_EQ(gathered.size(),
                    static_cast<std::size_t>(groups[gidx].size));

                // Verify values.
                for (std::size_t j = 0; j != groups[gidx].size; ++j)
                {
                    std::uint32_t expected_site =
                        static_cast<std::uint32_t>(groups[gidx].left + j);
                    HPX_TEST_EQ(
                        gathered[j].size(), static_cast<std::size_t>(3));
                    HPX_TEST_EQ(gathered[j][0], expected_site * 10);
                    HPX_TEST_EQ(gathered[j][1], expected_site * 10 + 1);
                    HPX_TEST_EQ(gathered[j][2], expected_site * 10 + 2);
                }

                // Scatter: gathered is already vector<vector<uint32_t>>,
                // which is the right shape for scatter (one entry per
                // subtree site). Pass directly.
                auto my_result = detail::subtree_scatter_at_top_rep(
                    comms, HPX_MOVE(gathered), generation_arg(2))
                                     .get();

                HPX_TEST_EQ(my_result.size(), static_cast<std::size_t>(3));
                HPX_TEST_EQ(my_result[0], site * 10);
                HPX_TEST_EQ(my_result[1], site * 10 + 1);
                HPX_TEST_EQ(my_result[2], site * 10 + 2);
            }
            else
            {
                detail::subtree_send_to_top_rep(
                    comms, HPX_MOVE(my_data), generation_arg(1));

                auto my_result = detail::subtree_receive_from_top_rep<
                    std::vector<std::uint32_t>>(comms, generation_arg(2))
                                     .get();

                HPX_TEST_EQ(my_result.size(), static_cast<std::size_t>(3));
                HPX_TEST_EQ(my_result[0], site * 10);
                HPX_TEST_EQ(my_result[1], site * 10 + 1);
                HPX_TEST_EQ(my_result[2], site * 10 + 2);
            }
        }));
    }

    hpx::wait_all(std::move(sites));
}

int hpx_main()
{
    if (hpx::get_locality_id() == 0)
    {
        // Balanced, power-of-arity cases.
        for (auto num_sites : {2u, 4u, 8u, 16u})
        {
            for (int arity : {2, 4})
            {
                test_round_trip(num_sites, arity);
            }
        }

        // Unbalanced, non-power-of-arity cases.
        for (auto num_sites : {3u, 5u, 7u, 9u, 11u})
        {
            for (int arity : {2, 3, 4})
            {
                test_round_trip(num_sites, arity);
            }
        }

        // Vector payload tests.
        test_vector_payload(8, 2);
        test_vector_payload(11, 4);
        test_vector_payload(5, 2);
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
