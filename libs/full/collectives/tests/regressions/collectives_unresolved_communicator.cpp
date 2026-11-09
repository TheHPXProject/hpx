//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Requests started on a communicator that is not resolved yet must not block
// each other. Generation 2 is started first and has to wait for generation 1,
// which is attached to the same communicator.

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_main.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

using namespace hpx::collectives;

constexpr std::uint32_t num_sites = 4;
constexpr int iterations = 50;

template <typename Op>
void test_out_of_order_generations(char const* name, Op op)
{
    for (int i = 0; i != iterations; ++i)
    {
        std::string const basename = std::string("/test/unresolved/") + name +
            "/" + std::to_string(i) + "/";

        std::vector<hpx::future<void>> sites;
        sites.reserve(num_sites);
        for (std::uint32_t site = 0; site != num_sites; ++site)
        {
            sites.push_back(hpx::async([=]() {
                communicator comm = create_communicator(basename.c_str(),
                    num_sites_arg(num_sites), this_site_arg(site));

                // a request on a resolved communicator waits for the earlier
                // generations inside the call, so the later generation is
                // started from its own task
                auto second = hpx::async(
                    [&]() { op(comm, site, generation_arg(2)).get(); });

                op(comm, site, generation_arg(1)).get();
                second.get();
            }));
        }
        hpx::wait_all(std::move(sites));
    }
}

int main()
{
    if (hpx::get_num_localities(hpx::launch::sync) != 1)
    {
        return hpx::util::report_errors();
    }

    test_out_of_order_generations(
        "all_gather", [](communicator c, std::uint32_t s, generation_arg g) {
            return hpx::future<void>(
                all_gather(std::move(c), s, this_site_arg(s), g));
        });

    test_out_of_order_generations(
        "all_reduce", [](communicator c, std::uint32_t s, generation_arg g) {
            return hpx::future<void>(all_reduce(std::move(c), s,
                std::plus<std::uint32_t>{}, this_site_arg(s), g));
        });

    test_out_of_order_generations(
        "all_to_all", [](communicator c, std::uint32_t s, generation_arg g) {
            return hpx::future<void>(all_to_all(std::move(c),
                std::vector<std::uint32_t>(num_sites), this_site_arg(s), g));
        });

    test_out_of_order_generations(
        "broadcast", [](communicator c, std::uint32_t s, generation_arg g) {
            if (s == 0)
            {
                return hpx::future<void>(
                    broadcast_to(std::move(c), s, this_site_arg(s), g));
            }
            return hpx::future<void>(broadcast_from<std::uint32_t>(
                std::move(c), this_site_arg(s), g));
        });

    test_out_of_order_generations("exclusive_scan",
        [](communicator c, std::uint32_t s, generation_arg g) {
            return hpx::future<void>(
                exclusive_scan(std::move(c), s, std::uint32_t(0),
                    std::plus<std::uint32_t>{}, this_site_arg(s), g));
        });

    test_out_of_order_generations("inclusive_scan",
        [](communicator c, std::uint32_t s, generation_arg g) {
            return hpx::future<void>(inclusive_scan(std::move(c), s,
                std::plus<std::uint32_t>{}, this_site_arg(s), g));
        });

    test_out_of_order_generations(
        "gather", [](communicator c, std::uint32_t s, generation_arg g) {
            if (s == 0)
            {
                return hpx::future<void>(
                    gather_here(std::move(c), s, this_site_arg(s), g));
            }
            return gather_there(std::move(c), s, this_site_arg(s), g);
        });

    test_out_of_order_generations(
        "reduce", [](communicator c, std::uint32_t s, generation_arg g) {
            if (s == 0)
            {
                return hpx::future<void>(reduce_here(std::move(c), s,
                    std::plus<std::uint32_t>{}, this_site_arg(s), g));
            }
            return hpx::future<void>(
                reduce_there(std::move(c), s, this_site_arg(s), g));
        });

    test_out_of_order_generations(
        "scatter", [](communicator c, std::uint32_t s, generation_arg g) {
            if (s == 0)
            {
                return hpx::future<void>(scatter_to(std::move(c),
                    std::vector<std::uint32_t>(num_sites), this_site_arg(s),
                    g));
            }
            return hpx::future<void>(
                scatter_from<std::uint32_t>(std::move(c), this_site_arg(s), g));
        });

    return hpx::util::report_errors();
}
#endif
