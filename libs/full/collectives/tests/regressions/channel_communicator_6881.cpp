//  Copyright (c) 2026 Fabian C.
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/barrier.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/futures.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/runtime.hpp>

#include <cstdint>
#include <string>
#include <vector>

void test_channel_communicator(std::uint32_t this_site, std::uint32_t num_sites,
    std::uint32_t delayed_site)
{
    using hpx::collectives::num_sites_arg;
    using hpx::collectives::that_site_arg;
    using hpx::collectives::this_site_arg;

    std::string const basename =
        "/test/channel_communicator_6881/" + std::to_string(delayed_site) + "/";

    hpx::distributed::barrier rendezvous(
        basename + "rendezvous/", num_sites, this_site);
    hpx::distributed::barrier lifetime_barrier(
        basename + "lifetime/", num_sites, this_site);

    // Do not let this site register its channel endpoint until all other
    // synchronous factories have returned.
    if (this_site == delayed_site)
    {
        rendezvous.wait();
    }

    auto comm = hpx::collectives::create_channel_communicator(hpx::launch::sync,
        basename.c_str(), num_sites_arg(num_sites), this_site_arg(this_site));

    HPX_TEST(static_cast<bool>(comm));

    if (this_site != delayed_site)
    {
        rendezvous.wait();
    }

    // Exercise every peer so that all pending peer lookups are resolved
    // before any communicator is destroyed.
    std::vector<hpx::future<void>> sends;
    std::vector<hpx::future<std::uint32_t>> receives;

    sends.reserve(num_sites);
    receives.reserve(num_sites);

    for (std::uint32_t peer = 0; peer != num_sites; ++peer)
    {
        receives.push_back(
            hpx::collectives::get<std::uint32_t>(comm, that_site_arg(peer)));
        sends.push_back(hpx::collectives::set(
            comm, that_site_arg(peer), this_site * num_sites + peer));
    }

    hpx::wait_all(sends, receives);

    for (std::uint32_t peer = 0; peer != num_sites; ++peer)
    {
        HPX_TEST(!sends[peer].has_exception());
        HPX_TEST(!receives[peer].has_exception());
        HPX_TEST_EQ(receives[peer].get(), peer * num_sites + this_site);
    }

    // Keep every registered endpoint alive until all sites have finished
    // using the communicator.
    lifetime_barrier.wait();
}

int hpx_main()
{
    auto const this_site = hpx::get_locality_id();
    auto const num_sites = hpx::get_num_localities(hpx::launch::sync);

    HPX_TEST_EQ(num_sites, std::uint32_t{3});

    for (std::uint32_t delayed_site = 0; delayed_site != num_sites;
        ++delayed_site)
    {
        test_channel_communicator(this_site, num_sites, delayed_site);
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    std::vector<std::string> const cfg = {"hpx.run_hpx_main!=1"};

    hpx::init_params init_args;
    init_args.cfg = cfg;

    int const result = hpx::init(argc, argv, init_args);
    return result != 0 ? result : hpx::util::report_errors();
}

#endif
