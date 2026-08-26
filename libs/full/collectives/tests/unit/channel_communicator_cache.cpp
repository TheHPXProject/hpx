//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The process-local cache of channel communicators hands the same communicator
// back for the same (basename, site) pair, so repeated exchanges over one
// fixed set of sites do not pay a fresh AGAS registration and peer lookup on
// every repetition. This test pins the cache contract down directly: one entry
// per site, a default site resolving to the locality id, and a call that
// disagrees about the number of sites being rejected instead of silently
// reusing a communicator built for another participant count.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

using namespace hpx::collectives;

///////////////////////////////////////////////////////////////////////////////
namespace {

    /// The cache is per process, so every locality needs a basename of its own
    /// or the sites would collide on the AGAS names they register.
    std::string cache_basename(
        std::uint32_t const this_locality, char const* leaf = "")
    {
        return std::string("/test/channel_communicator_cache/") +
            std::to_string(this_locality) + "/" + leaf;
    }

    /// The communicator records the (num_sites, this_site) pair it was built
    /// for, which is what a site that received a foreign communicator would
    /// give away.
    std::pair<std::size_t, std::size_t> cached_info(
        hpx::shared_future<hpx::collectives::channel_communicator> const& comm)
    {
        auto const info = comm.get().get_info();
        return std::make_pair(static_cast<std::size_t>(info.first),
            static_cast<std::size_t>(info.second));
    }

    /// Each site must receive the communicator registered for its own endpoint.
    /// Were the cache keyed by basename alone, every site after the first would
    /// get that site's communicator, whose this_site reports a foreign index.
    void test_each_site_gets_its_own_communicator(
        std::uint32_t const this_locality)
    {
        std::string const basename = cache_basename(this_locality, "distinct/");

        std::size_t const count_before =
            detail::get_cached_channel_communicator_count();

        for (std::size_t site = 0; site != 4; ++site)
        {
            auto const comm = detail::get_cached_channel_communicator(
                basename, num_sites_arg(4), this_site_arg(site));

            auto const info = cached_info(comm);
            HPX_TEST_EQ(info.first, std::size_t(4));
            HPX_TEST_EQ(info.second, site);
        }

        HPX_TEST_EQ(
            detail::get_cached_channel_communicator_count(), count_before + 4);
    }

    /// Repeating a lookup for the same (basename, site) must reuse the entry
    /// rather than create and register a second communicator.
    void test_repeated_lookup_reuses_entry(std::uint32_t const this_locality)
    {
        std::string const basename = cache_basename(this_locality, "reuse/");

        std::size_t const count_before =
            detail::get_cached_channel_communicator_count();

        auto const first = detail::get_cached_channel_communicator(
            basename, num_sites_arg(4), this_site_arg(2));
        auto const second = detail::get_cached_channel_communicator(
            basename, num_sites_arg(4), this_site_arg(2));

        HPX_TEST_EQ(
            detail::get_cached_channel_communicator_count(), count_before + 1);

        auto const info = cached_info(first);
        HPX_TEST_EQ(info.first, std::size_t(4));
        HPX_TEST_EQ(info.second, std::size_t(2));

        auto const info_second = cached_info(second);
        HPX_TEST_EQ(info_second.first, info.first);
        HPX_TEST_EQ(info_second.second, info.second);
    }

    /// A default site resolves to the locality id, so the default and the
    /// explicit spelling of that site must meet in the same cache entry.
    void test_default_site_resolves_to_locality(
        std::uint32_t const this_locality)
    {
        std::string const basename = cache_basename(this_locality, "default/");

        std::size_t const count_before =
            detail::get_cached_channel_communicator_count();

        auto const resolved =
            detail::get_cached_channel_communicator(basename, num_sites_arg(4));
        auto const explicit_site = detail::get_cached_channel_communicator(
            basename, num_sites_arg(4), this_site_arg(this_locality));

        HPX_TEST_EQ(
            detail::get_cached_channel_communicator_count(), count_before + 1);

        auto const info = cached_info(resolved);
        HPX_TEST_EQ(info.first, std::size_t(4));
        HPX_TEST_EQ(info.second, std::size_t(this_locality));

        auto const info_explicit = cached_info(explicit_site);
        HPX_TEST_EQ(info_explicit.first, info.first);
        HPX_TEST_EQ(info_explicit.second, info.second);
    }

    /// A second call that disagrees about the number of sites must not reuse
    /// the cached communicator: the exchange would run with the wrong group
    /// size while the caller believes it named another one.
    void test_num_sites_mismatch_is_rejected(std::uint32_t const this_locality)
    {
        std::string const basename = cache_basename(this_locality, "mismatch/");

        detail::get_cached_channel_communicator(
            basename, num_sites_arg(4), this_site_arg(1))
            .get();

        bool caught = false;
        try
        {
            detail::get_cached_channel_communicator(
                basename, num_sites_arg(3), this_site_arg(1));
        }
        catch (hpx::exception const& e)
        {
            caught = e.get_error() == hpx::error::bad_parameter;
        }

        HPX_TEST(caught);
    }

    /// A default number of sites resolves to the number of localities before
    /// the lookup, so it is compared against the entry like an explicit one.
    void test_default_num_sites_mismatch_is_rejected(
        std::uint32_t const this_locality)
    {
        std::string const basename =
            cache_basename(this_locality, "default_mismatch/");

        detail::get_cached_channel_communicator(
            basename, num_sites_arg(), this_site_arg(0))
            .get();

        bool caught = false;
        try
        {
            detail::get_cached_channel_communicator(basename,
                num_sites_arg(
                    hpx::agas::get_num_localities(hpx::launch::sync) + 1),
                this_site_arg(0));
        }
        catch (hpx::exception const& e)
        {
            caught = e.get_error() == hpx::error::bad_parameter;
        }

        HPX_TEST(caught);
    }
}    // namespace

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    std::uint32_t const this_locality = hpx::get_locality_id();

    test_each_site_gets_its_own_communicator(this_locality);
    test_repeated_lookup_reuses_entry(this_locality);
    test_default_site_resolves_to_locality(this_locality);
    test_num_sites_mismatch_is_rejected(this_locality);
    test_default_num_sites_mismatch_is_rejected(this_locality);

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::init(argc, argv), 0);
    return hpx::util::report_errors();
}
#endif
