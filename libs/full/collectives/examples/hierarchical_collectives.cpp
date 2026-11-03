//  Copyright (c) 2026 Ishaan400
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This example shows how to use a hierarchical communicator. It creates one
// communicator spanning all localities and runs several different collective
// operations on it: all_reduce, all_gather, and a broadcast from the root.
//
// Run it on more than one locality, for instance:
//
//     hpxrun.py -l 4 -t 1 bin/hierarchical_collectives -- --arity=2

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/program_options.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

using namespace hpx::collectives;

constexpr char const* hierarchical_basename = "/example/hierarchical/";

int hpx_main(hpx::program_options::variables_map& vm)
{
    std::uint32_t const num_localities =
        hpx::get_num_localities(hpx::launch::sync);
    std::uint32_t const this_locality = hpx::get_locality_id();
    int const arity = vm["arity"].as<int>();

    //[hierarchical_collectives_create
    // All sites create the communicator with the same arguments. The arity is
    // the number of children of each node in the tree of communicators.
    //
    // Below the flat fallback threshold (16 sites by default) the
    // communicator collapses to a single flat communicator. Pass 0 to always
    // build a tree, which is what this example does so that the tree is used
    // even when it runs on a few localities only.
    auto const comm = create_hierarchical_communicator(hierarchical_basename,
        num_sites_arg(num_localities), this_site_arg(this_locality),
        arity_arg(arity), generation_arg(), root_site_arg(0),
        flat_fallback_threshold_arg(0));
    //]

    //[hierarchical_collectives_use
    // A communicator that is shared between several operations needs an
    // explicit generation number on every call, and the numbers must be
    // strictly consecutive across all of those calls, starting at 1. Keep a
    // single counter and increment it for every call, whatever the
    // operation. Two-phase operations such as all_reduce and all_gather
    // reject the default generation even when the communicator is not shared.
    std::size_t generation = 0;

    // Sum up the locality ids. Every site receives the result.
    std::uint32_t const sum =
        all_reduce(comm, this_locality, std::plus<std::uint32_t>{},
            this_site_arg(this_locality), generation_arg(++generation))
            .get();

    // Collect the squares of the locality ids on every site.
    std::vector<std::uint32_t> const squares =
        all_gather(comm, this_locality * this_locality,
            this_site_arg(this_locality), generation_arg(++generation))
            .get();

    // Send a value from the root site to all other sites. Only the root site
    // calls broadcast_to; all others call broadcast_from.
    std::string message;
    if (this_locality == 0)
    {
        message = broadcast_to(comm, std::string("hello from the root"),
            this_site_arg(this_locality), generation_arg(++generation))
                      .get();
    }
    else
    {
        message = broadcast_from<std::string>(
            comm, this_site_arg(this_locality), generation_arg(++generation))
                      .get();
    }
    //]

    // Check the results on every site.
    std::uint32_t expected_sum = 0;
    for (std::uint32_t i = 0; i != num_localities; ++i)
    {
        expected_sum += i;
    }

    bool ok = sum == expected_sum && squares.size() == num_localities &&
        message == "hello from the root";
    for (std::uint32_t i = 0; ok && i != squares.size(); ++i)
    {
        ok = squares[i] == i * i;
    }

    if (!ok)
    {
        std::cerr << "locality " << this_locality
                  << ": unexpected result of a hierarchical collective\n";
    }

    if (this_locality == 0)
    {
        std::cout << "localities: " << num_localities << ", arity: " << arity
                  << "\n";
        std::cout << "all_reduce: sum of locality ids = " << sum << "\n";
        std::cout << "all_gather: squares of locality ids =";
        for (std::uint32_t const value : squares)
        {
            std::cout << " " << value;
        }
        std::cout << "\n";
        std::cout << "broadcast: " << message << "\n" << std::flush;
    }

    hpx::finalize();
    return ok ? 0 : 1;
}

int main(int argc, char* argv[])
{
    hpx::program_options::options_description desc(
        "usage: hierarchical_collectives [options]");

    // clang-format off
    desc.add_options()
        ("arity", hpx::program_options::value<int>()->default_value(2),
         "the number of children of each node in the tree of communicators "
         "(must be at least 2)");
    // clang-format on

    // Run hpx_main on all localities.
    std::vector<std::string> const cfg = {"hpx.run_hpx_main!=1"};

    hpx::init_params init_args;
    init_args.desc_cmdline = desc;
    init_args.cfg = cfg;

    return hpx::init(argc, argv, init_args);
}

#else

int main()
{
    return 0;
}

#endif
