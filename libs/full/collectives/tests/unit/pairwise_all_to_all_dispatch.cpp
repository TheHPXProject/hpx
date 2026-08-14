//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// all_to_all can route every row through one communicator site or send each
// row straight to the site it belongs to. Which path runs is a performance
// choice, so the two must be indistinguishable from the outside. These tests
// drive the public basename API across real localities and require both paths
// to produce the same result.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/testing.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

using namespace hpx::collectives;

///////////////////////////////////////////////////////////////////////////////
// A row whose type alone reaches the default threshold, which is all an
// automatic decision is allowed to look at: the contributed data may differ
// from site to site, and sites that disagreed on the path would wait on each
// other forever.
using big_row = std::array<std::uint64_t, 512>;

static_assert(detail::pairwise_type_bytes<big_row>() >=
    static_cast<std::size_t>(pairwise_threshold_arg()));
static_assert(detail::pairwise_type_bytes<std::uint32_t>() <
    static_cast<std::size_t>(pairwise_threshold_arg()));

///////////////////////////////////////////////////////////////////////////////
// The value site source contributes for site destination. Encoding both ends
// makes a misrouted row visible instead of merely wrong.
constexpr std::uint32_t exchanged_value(
    std::uint32_t const source, std::uint32_t const destination) noexcept
{
    return source * 1000 + destination;
}

std::vector<std::uint32_t> contribution(
    std::uint32_t const this_locality, std::uint32_t const num_localities)
{
    std::vector<std::uint32_t> values;
    values.reserve(num_localities);
    for (std::uint32_t destination = 0; destination != num_localities;
        ++destination)
    {
        values.push_back(exchanged_value(this_locality, destination));
    }
    return values;
}

void check_result(std::vector<std::uint32_t> const& result,
    std::uint32_t const this_locality, std::uint32_t const num_localities)
{
    HPX_TEST_EQ(result.size(), std::size_t(num_localities));
    for (std::uint32_t source = 0; source != num_localities; ++source)
    {
        HPX_TEST_EQ(result[source], exchanged_value(source, this_locality));
    }
}

///////////////////////////////////////////////////////////////////////////////
// A threshold of 0 selects the direct exchange, the default threshold leaves
// a four-byte row on the routed one. Both have to agree, element for element.
// The first call is therefore also the end-to-end case for a small row being
// left alone by an automatic decision.
void test_both_paths_agree(
    std::uint32_t const this_locality, std::uint32_t const num_localities)
{
    std::vector<std::uint32_t> const routed =
        all_to_all("/test/pairwise_dispatch/routed/",
            contribution(this_locality, num_localities),
            num_sites_arg(num_localities), this_site_arg(this_locality),
            generation_arg(1))
            .get();

    check_result(routed, this_locality, num_localities);

    std::vector<std::uint32_t> const direct =
        all_to_all("/test/pairwise_dispatch/direct/",
            contribution(this_locality, num_localities),
            num_sites_arg(num_localities), this_site_arg(this_locality),
            generation_arg(1), root_site_arg(), pairwise_threshold_arg(0))
            .get();

    check_result(direct, this_locality, num_localities);
    HPX_TEST(routed == direct);
}

///////////////////////////////////////////////////////////////////////////////
// Successive generations on one basename must stay separate on the direct
// path too, which is what the exchange tag is for.
void test_direct_path_generations(
    std::uint32_t const this_locality, std::uint32_t const num_localities)
{
    for (std::uint32_t generation = 1; generation != 4; ++generation)
    {
        std::vector<std::uint32_t> values;
        values.reserve(num_localities);
        for (std::uint32_t destination = 0; destination != num_localities;
            ++destination)
        {
            values.push_back(
                generation * exchanged_value(this_locality, destination));
        }

        std::vector<std::uint32_t> const result =
            all_to_all("/test/pairwise_dispatch/generations/", HPX_MOVE(values),
                num_sites_arg(num_localities), this_site_arg(this_locality),
                generation_arg(generation), root_site_arg(),
                pairwise_threshold_arg(0))
                .get();

        HPX_TEST_EQ(result.size(), std::size_t(num_localities));
        for (std::uint32_t source = 0; source != num_localities; ++source)
        {
            HPX_TEST_EQ(result[source],
                generation * exchanged_value(source, this_locality));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// A routed explicit generation gets a generation-specific collective
// communicator, while the direct path separates generations with channel
// tags. They do not share a generation gate, so one basename may switch paths.
void test_paths_change_across_generations(
    std::uint32_t const this_locality, std::uint32_t const num_localities)
{
    char const* const basename = "/test/pairwise_dispatch/path_changes/";

    std::vector<std::uint32_t> const direct =
        all_to_all(basename, contribution(this_locality, num_localities),
            num_sites_arg(num_localities), this_site_arg(this_locality),
            generation_arg(1), root_site_arg(), pairwise_threshold_arg(0))
            .get();
    check_result(direct, this_locality, num_localities);

    std::vector<std::uint32_t> const routed =
        all_to_all(basename, contribution(this_locality, num_localities),
            num_sites_arg(num_localities), this_site_arg(this_locality),
            generation_arg(2))
            .get();
    check_result(routed, this_locality, num_localities);

    std::vector<std::uint32_t> const direct_again =
        all_to_all(basename, contribution(this_locality, num_localities),
            num_sites_arg(num_localities), this_site_arg(this_locality),
            generation_arg(3), root_site_arg(), pairwise_threshold_arg(0))
            .get();
    check_result(direct_again, this_locality, num_localities);
}

///////////////////////////////////////////////////////////////////////////////
// The sync overload carries the same knob and must reach the same place.
void test_sync_overload(
    std::uint32_t const this_locality, std::uint32_t const num_localities)
{
    std::vector<std::uint32_t> const result =
        all_to_all(hpx::launch::sync, "/test/pairwise_dispatch/sync/",
            contribution(this_locality, num_localities),
            num_sites_arg(num_localities), this_site_arg(this_locality),
            generation_arg(1), root_site_arg(), pairwise_threshold_arg(0));

    check_result(result, this_locality, num_localities);
}

///////////////////////////////////////////////////////////////////////////////
// The tests above force a path. These two leave the choice to the default
// threshold, which is how a caller who never touches the knob reaches either
// path.
//
// A completed exchange says nothing about which path ran -- that is the point
// of the dispatch -- so the decision itself is pinned down separately, for the
// two row types used here and at the site count these tests run at.
void test_default_threshold_decides(std::uint32_t const num_localities)
{
    HPX_TEST(!detail::exchange_pairwise(num_localities,
        detail::pairwise_type_bytes<std::uint32_t>(),
        pairwise_threshold_arg()));

    HPX_TEST(detail::exchange_pairwise(num_localities,
        detail::pairwise_type_bytes<big_row>(), pairwise_threshold_arg()));
}

// A row large enough for the default threshold to send it directly, driven
// through the public API with no threshold argument at all. Only the first
// element is checked against; the rest is filled so that a row arriving from
// the wrong peer cannot pass by accident.
big_row big_value(
    std::uint32_t const source, std::uint32_t const destination) noexcept
{
    big_row row{};
    for (std::size_t i = 0; i != row.size(); ++i)
    {
        row[i] = exchanged_value(source, destination) + i;
    }
    return row;
}

void test_auto_selects_direct_for_large_rows(
    std::uint32_t const this_locality, std::uint32_t const num_localities)
{
    std::vector<big_row> values;
    values.reserve(num_localities);
    for (std::uint32_t destination = 0; destination != num_localities;
        ++destination)
    {
        values.push_back(big_value(this_locality, destination));
    }

    std::vector<big_row> const result =
        all_to_all("/test/pairwise_dispatch/auto_direct/", HPX_MOVE(values),
            num_sites_arg(num_localities), this_site_arg(this_locality),
            generation_arg(1))
            .get();

    HPX_TEST_EQ(result.size(), std::size_t(num_localities));
    for (std::uint32_t source = 0; source != num_localities; ++source)
    {
        HPX_TEST(result[source] == big_value(source, this_locality));
    }
}

///////////////////////////////////////////////////////////////////////////////
// A default generation gives the sites no tag they all derive the same way,
// so the call stays on the routed path rather than failing. It still has to
// deliver the right answer, which is the part that matters to a caller.
void test_default_generation_still_works(
    std::uint32_t const this_locality, std::uint32_t const num_localities)
{
    std::vector<std::uint32_t> const result =
        all_to_all("/test/pairwise_dispatch/default_generation/",
            contribution(this_locality, num_localities),
            num_sites_arg(num_localities), this_site_arg(this_locality),
            generation_arg(), root_site_arg(), pairwise_threshold_arg(0))
            .get();

    check_result(result, this_locality, num_localities);
}

///////////////////////////////////////////////////////////////////////////////
// Selecting the direct path must not bypass validation performed by the
// established routed implementation.
void test_direct_path_preserves_root_validation(
    std::uint32_t const this_locality, std::uint32_t const num_localities)
{
    bool caught_bad_root = false;
    try
    {
        all_to_all("/test/pairwise_dispatch/invalid_root/",
            contribution(this_locality, num_localities),
            num_sites_arg(num_localities), this_site_arg(this_locality),
            generation_arg(1), root_site_arg(num_localities),
            pairwise_threshold_arg(0))
            .get();
    }
    catch (hpx::exception const& e)
    {
        caught_bad_root = e.get_error() == hpx::error::bad_parameter;
    }
    HPX_TEST(caught_bad_root);
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    std::uint32_t const this_locality = hpx::get_locality_id();
    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);

    // the direct path only differs from the routed one above two sites
    HPX_TEST_LTE(std::uint32_t(3), num_localities);

    test_both_paths_agree(this_locality, num_localities);
    test_direct_path_generations(this_locality, num_localities);
    test_paths_change_across_generations(this_locality, num_localities);
    test_sync_overload(this_locality, num_localities);
    test_default_threshold_decides(num_localities);
    test_auto_selects_direct_for_large_rows(this_locality, num_localities);
    test_default_generation_still_works(this_locality, num_localities);
    test_direct_path_preserves_root_validation(this_locality, num_localities);

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    // every locality has to reach the collective, not just the console
    std::vector<std::string> const cfg = {"hpx.run_hpx_main!=1"};

    hpx::init_params init_args;
    init_args.cfg = cfg;

    HPX_TEST_EQ(hpx::init(argc, argv, init_args), 0);
    return hpx::util::report_errors();
}
#endif
