//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// A request for a later generation on a resolved communicator must return
// right away instead of suspending the calling thread until the earlier
// generations have finished. Every site requests the generations of one
// order from the same thread and waits for the results only afterwards.

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
constexpr std::uint32_t sum_of_sites = num_sites * (num_sites - 1) / 2;
constexpr int iterations = 5;

// The values depend on the generation, so a result of another generation
// fails the checks.
constexpr std::uint32_t value(std::uint32_t site, std::size_t generation)
{
    return site + static_cast<std::uint32_t>(10 * generation);
}

communicator make_communicator(std::string const& basename, std::uint32_t site)
{
    communicator comm = create_communicator(
        basename.c_str(), num_sites_arg(num_sites), this_site_arg(site));
    comm.wait();
    return comm;
}

template <typename F>
void run_sites(F f)
{
    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);
    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async(f, site));
    }
    for (auto& site : hpx::when_all(std::move(sites)).get())
    {
        site.get();
    }
}

template <typename Op>
void test_early_generation(
    char const* name, std::vector<std::size_t> const& order, Op op)
{
    for (int i = 0; i != iterations; ++i)
    {
        std::string basename = "/test/early_generation/" + std::string(name) +
            "/" + std::to_string(order.front()) + "/" + std::to_string(i) + "/";

        run_sites([&](std::uint32_t site) {
            communicator comm = make_communicator(basename, site);

            std::vector<hpx::future<void>> results;
            results.reserve(order.size());
            for (std::size_t generation : order)
            {
                results.push_back(op(comm, site, generation));
            }
            for (auto& result : results)
            {
                result.get();
            }
        });
    }
}

template <typename T>
hpx::future<void> check_equal(hpx::future<T> f, T expected)
{
    return f.then([expected = std::move(expected)](
                      hpx::future<T>&& r) { HPX_TEST(r.get() == expected); });
}

void test_all_operations(std::vector<std::size_t> const& order)
{
    test_early_generation("all_gather", order,
        [](communicator c, std::uint32_t s, std::size_t g) {
            std::vector<std::uint32_t> expected;
            for (std::uint32_t i = 0; i != num_sites; ++i)
            {
                expected.push_back(value(i, g));
            }
            return check_equal(all_gather(std::move(c), value(s, g),
                                   this_site_arg(s), generation_arg(g)),
                std::move(expected));
        });

    test_early_generation("all_reduce", order,
        [](communicator c, std::uint32_t s, std::size_t g) {
            return check_equal(all_reduce(std::move(c), value(s, g),
                                   std::plus<std::uint32_t>{}, this_site_arg(s),
                                   generation_arg(g)),
                value(sum_of_sites, num_sites * g));
        });

    test_early_generation("all_to_all", order,
        [](communicator c, std::uint32_t s, std::size_t g) {
            std::vector<std::uint32_t> data, expected;
            for (std::uint32_t i = 0; i != num_sites; ++i)
            {
                data.push_back(value(100 * s + i, g));
                expected.push_back(value(100 * i + s, g));
            }
            return check_equal(all_to_all(std::move(c), std::move(data),
                                   this_site_arg(s), generation_arg(g)),
                std::move(expected));
        });

    test_early_generation(
        "barrier", order, [](communicator c, std::uint32_t s, std::size_t g) {
            return barrier(std::move(c), this_site_arg(s), generation_arg(g));
        });

    test_early_generation(
        "broadcast", order, [](communicator c, std::uint32_t s, std::size_t g) {
            if (s == 0)
            {
                return check_equal(broadcast_to(std::move(c), value(7, g),
                                       this_site_arg(s), generation_arg(g)),
                    value(7, g));
            }
            return check_equal(broadcast_from<std::uint32_t>(std::move(c),
                                   this_site_arg(s), generation_arg(g)),
                value(7, g));
        });

    test_early_generation("exclusive_scan", order,
        [](communicator c, std::uint32_t s, std::size_t g) {
            return check_equal(exclusive_scan(std::move(c), value(s, g),
                                   std::uint32_t(0), std::plus<std::uint32_t>{},
                                   this_site_arg(s), generation_arg(g)),
                value(s * (s - 1) / 2, s * g));
        });

    test_early_generation(
        "gather", order, [](communicator c, std::uint32_t s, std::size_t g) {
            if (s == 0)
            {
                std::vector<std::uint32_t> expected;
                for (std::uint32_t i = 0; i != num_sites; ++i)
                {
                    expected.push_back(value(i, g));
                }
                return check_equal(gather_here(std::move(c), value(s, g),
                                       this_site_arg(s), generation_arg(g)),
                    std::move(expected));
            }
            return gather_there(
                std::move(c), value(s, g), this_site_arg(s), generation_arg(g));
        });

    test_early_generation("inclusive_scan", order,
        [](communicator c, std::uint32_t s, std::size_t g) {
            return check_equal(inclusive_scan(std::move(c), value(s, g),
                                   std::plus<std::uint32_t>{}, this_site_arg(s),
                                   generation_arg(g)),
                value(s * (s + 1) / 2, (s + 1) * g));
        });

    test_early_generation(
        "reduce", order, [](communicator c, std::uint32_t s, std::size_t g) {
            if (s == 0)
            {
                return check_equal(reduce_here(std::move(c), value(s, g),
                                       std::plus<std::uint32_t>{},
                                       this_site_arg(s), generation_arg(g)),
                    value(sum_of_sites, num_sites * g));
            }
            return hpx::future<void>(reduce_there(std::move(c), value(s, g),
                this_site_arg(s), generation_arg(g)));
        });

    test_early_generation(
        "scatter", order, [](communicator c, std::uint32_t s, std::size_t g) {
            if (s == 0)
            {
                std::vector<std::uint32_t> data;
                for (std::uint32_t i = 0; i != num_sites; ++i)
                {
                    data.push_back(value(i, g));
                }
                return check_equal(scatter_to(std::move(c), std::move(data),
                                       this_site_arg(s), generation_arg(g)),
                    value(s, g));
            }
            return check_equal(scatter_from<std::uint32_t>(std::move(c),
                                   this_site_arg(s), generation_arg(g)),
                value(s, g));
        });
}

// An explicit generation after an auto-generation operation is rejected when
// the request arrives, also when the gate has not reached that generation.
void test_explicit_after_auto()
{
    run_sites([](std::uint32_t site) {
        communicator comm =
            make_communicator("/test/early_generation/after_auto/", site);
        barrier(comm, this_site_arg(site)).get();

        bool rejected = false;
        try
        {
            all_reduce(comm, value(site, 5), std::plus<std::uint32_t>{},
                this_site_arg(site), generation_arg(5))
                .get();
        }
        catch (hpx::exception const& e)
        {
            rejected = e.get_error() == hpx::error::bad_parameter;
        }
        HPX_TEST(rejected);
    });
}

int main()
{
    if (hpx::get_num_localities(hpx::launch::sync) != 1)
    {
        return hpx::util::report_errors();
    }

    // generation 2 before generation 1
    test_all_operations({2, 1});
    // generations 3 and 4 (in reverse order) before generation 2
    test_all_operations({4, 3, 1, 2});

    test_explicit_after_auto();

    return hpx::util::report_errors();
}
#endif
