//  Copyright (c) 2019-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/actions.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstdint>
#include <iostream>
#include <map>
#include <random>
#include <string>
#include <utility>
#include <vector>

using namespace hpx::collectives;

constexpr char const* concurrent_basename = "/test/concurrent_communicator/";
#if defined(HPX_DEBUG)
constexpr int ITERATIONS = 100;
#else
constexpr int ITERATIONS = 1000;
#endif
constexpr std::uint32_t num_sites = 10;

////////////////////////////////////////////////////////////////////////////////
// Each generation of the communicator has to consistently represent a
// particular operation across all localities. For this reason, this test first
// generates generations to be used for the distributed tests and exposes those
// to all participating localities.
struct generations
{
    constexpr static char const* operations[] = {"all_gather", "all_reduce",
        "all_to_all", "broadcast", "exclusive_scan", "gather", "inclusive_scan",
        "reduce", "scatter"};

    generations() = default;

    void init(std::mt19937& gen, std::size_t iterations)
    {
        std::uniform_int_distribution<std::size_t> dist(
            0, std::size(operations) - 1);

        for (std::size_t i = 0; i != iterations * std::size(operations); ++i)
        {
            data[operations[dist(gen)]].generations.push_back(i + 1);
        }

        was_initialized = true;
    }

    bool get_next_generation(char const* operation, std::size_t& generation)
    {
        if (auto it = data.find(operation); it != data.end())
        {
            if (it->second.current < it->second.generations.size())
            {
                generation = it->second.generations[it->second.current++];
                return true;
            }
            return false;    // no more generations available
        }
        return false;
    }

    std::size_t get_iterations(char const* operation) const
    {
        if (auto it = data.find(operation); it != data.end())
        {
            return it->second.generations.size();
        }
        return 0;
    }

    bool initialized() const
    {
        return was_initialized;
    }

private:
    friend class hpx::serialization::access;

    template <typename Archive>
    void load(Archive& ar, unsigned)
    {
        // clang-format off
        ar & data;
        // clang-format on

        for (auto& [k, v] : data)
        {
            v.current = 0;
        }
    }

    template <typename Archive>
    void save(Archive& ar, unsigned) const
    {
        // clang-format off
        ar & data;
        // clang-format on
    }

    HPX_SERIALIZATION_SPLIT_MEMBER();

    bool was_initialized = false;
    struct generation_data
    {
        template <typename Archive>
        void serialize(Archive& ar, unsigned)
        {
            // Only serialize the generated sequence; current is a local
            // cursor and is reset after deserialization.
            ar & generations;
        }

        std::size_t current = 0;
        std::vector<std::size_t> generations;
    };
    std::map<std::string, generation_data> data;
};

generations distributed;
generations local;

generations get_generations(bool local_generations)
{
    generations const& result = local_generations ? local : distributed;
    hpx::util::yield_while([&] { return !result.initialized(); });
    return result;
}

HPX_PLAIN_ACTION(get_generations);

////////////////////////////////////////////////////////////////////////////////
double test_all_gather(
    communicator const& comm, std::uint32_t num_localities, std::uint32_t here)
{
    hpx::chrono::high_resolution_timer const t;

    std::size_t gen = 0;
    for (int i = 0; distributed.get_next_generation("all_gather", gen); ++i)
    {
        hpx::future<std::vector<std::uint32_t>> overall_result =
            all_gather(comm, here + i, generation_arg(gen));

        std::vector<std::uint32_t> r = overall_result.get();
        HPX_TEST_EQ(r.size(), num_localities);

        for (std::size_t j = 0; j != r.size(); ++j)
        {
            HPX_TEST_EQ(r[j], j + i);
        }
    }

    return t.elapsed();
}

double test_all_reduce(
    communicator const& comm, std::uint32_t num_localities, std::uint32_t here)
{
    hpx::chrono::high_resolution_timer const t;

    std::size_t gen = 0;
    for (int i = 0; distributed.get_next_generation("all_reduce", gen); ++i)
    {
        hpx::future<std::uint32_t> overall_result = all_reduce(
            comm, here + i, std::plus<std::uint32_t>{}, generation_arg(gen));

        std::uint32_t sum = 0;
        for (std::uint32_t j = 0; j != num_localities; ++j)
        {
            sum += j + i;
        }
        HPX_TEST_EQ(sum, overall_result.get());
    }

    return t.elapsed();
}

double test_all_to_all(
    communicator const& comm, std::uint32_t num_localities, std::uint32_t here)
{
    hpx::chrono::high_resolution_timer const t;

    std::size_t gen = 0;
    for (int i = 0; distributed.get_next_generation("all_to_all", gen); ++i)
    {
        std::vector<std::uint32_t> values(num_localities);
        std::fill(values.begin(), values.end(), here + i);

        hpx::future<std::vector<std::uint32_t>> overall_result =
            all_to_all(comm, std::move(values), generation_arg(gen));

        std::vector<std::uint32_t> r = overall_result.get();

        HPX_TEST_EQ(r.size(), num_localities);

        for (std::size_t j = 0; j != r.size(); ++j)
        {
            HPX_TEST_EQ(r[j], j + i);
        }
    }

    return t.elapsed();
}

double test_broadcast(communicator const& comm, std::uint32_t here)
{
    hpx::chrono::high_resolution_timer const t;

    std::size_t gen = 0;
    // clang-format off
    for (std::uint32_t i = 0; distributed.get_next_generation("broadcast", gen);
         ++i)
    // clang-format on
    {
        if (here == 0)
        {
            hpx::future<std::uint32_t> result =
                broadcast_to(comm, i + 42, generation_arg(gen));

            HPX_TEST_EQ(i + 42, result.get());
        }
        else
        {
            hpx::future<std::uint32_t> result =
                hpx::collectives::broadcast_from<std::uint32_t>(
                    comm, generation_arg(gen));

            HPX_TEST_EQ(i + 42, result.get());
        }
    }

    return t.elapsed();
}

double test_exclusive_scan(communicator const& comm, std::uint32_t here)
{
    hpx::chrono::high_resolution_timer const t;

    std::size_t gen = 0;
    for (int i = 0; distributed.get_next_generation("exclusive_scan", gen); ++i)
    {
        hpx::future<std::uint32_t> overall_result =
            exclusive_scan(comm, here + i, static_cast<std::uint32_t>(i),
                std::plus<>{}, generation_arg(gen));

        std::uint32_t sum = i;
        for (std::uint32_t j = 0; j < here; ++j)
        {
            sum += j + i;
        }
        HPX_TEST_EQ(sum, overall_result.get());
    }

    return t.elapsed();
}

double test_gather(communicator const& comm, std::uint32_t here)
{
    hpx::chrono::high_resolution_timer const t;

    std::size_t gen = 0;
    // clang-format off
    for (std::uint32_t i = 0; distributed.get_next_generation("gather", gen);
         ++i)
    // clang-format on
    {
        if (here == 0)
        {
            hpx::future<std::vector<std::uint32_t>> overall_result =
                gather_here(comm, 42 + i, generation_arg(gen));

            std::vector<std::uint32_t> sol = overall_result.get();
            for (std::size_t j = 0; j != sol.size(); ++j)
            {
                HPX_TEST(j + 42 + i == sol[j]);
            }
        }
        else
        {
            hpx::future<void> overall_result =
                gather_there(comm, here + 42 + i, generation_arg(gen));
            overall_result.get();
        }
    }

    return t.elapsed();
}

double test_inclusive_scan(communicator const& comm, std::uint32_t here)
{
    hpx::chrono::high_resolution_timer const t;

    std::size_t gen = 0;
    // clang-format off
    for (std::uint32_t i = 0;
         distributed.get_next_generation("inclusive_scan", gen); ++i)
    // clang-format on
    {
        hpx::future<std::uint32_t> overall_result = inclusive_scan(
            comm, here + i, std::plus<std::uint32_t>{}, generation_arg(gen));

        std::uint32_t sum = 0;
        for (std::uint32_t j = 0; j != here + 1; ++j)
        {
            sum += j + i;
        }
        HPX_TEST_EQ(sum, overall_result.get());
    }

    return t.elapsed();
}

double test_reduce(
    communicator const& comm, std::uint32_t num_localities, std::uint32_t here)
{
    hpx::chrono::high_resolution_timer const t;

    std::size_t gen = 0;
    for (int i = 0; distributed.get_next_generation("reduce", gen); ++i)
    {
        std::uint32_t value = here + i;
        if (here == 0)
        {
            hpx::future<std::uint32_t> overall_result = reduce_here(
                comm, std::move(value), std::plus<>{}, generation_arg(gen));

            std::uint32_t sum = 0;
            for (std::uint32_t j = 0; j != num_localities; ++j)
            {
                sum += j + i;
            }
            HPX_TEST_EQ(sum, overall_result.get());
        }
        else
        {
            hpx::future<void> overall_result =
                reduce_there(comm, std::move(value), generation_arg(gen));
            overall_result.get();
        }
    }

    return t.elapsed();
}

double test_scatter(
    communicator const& comm, std::uint32_t num_localities, std::uint32_t here)
{
    hpx::chrono::high_resolution_timer const t;

    std::size_t gen = 0;
    for (int i = 0; distributed.get_next_generation("scatter", gen); ++i)
    {
        if (here == 0)
        {
            std::vector<std::uint32_t> data(num_localities);
            std::iota(data.begin(), data.end(), 42 + i);

            hpx::future<std::uint32_t> result =
                scatter_to(comm, std::move(data), generation_arg(gen));

            HPX_TEST_EQ(i + 42 + here, result.get());
        }
        else
        {
            hpx::future<std::uint32_t> result =
                scatter_from<std::uint32_t>(comm, generation_arg(gen));

            HPX_TEST_EQ(i + 42 + here, result.get());
        }
    }

    return t.elapsed();
}

////////////////////////////////////////////////////////////////////////////////
double test_local_all_gather(std::vector<communicator> const& comms)
{
    double elapsed = 0.;

    std::size_t gen = 0;
    for (std::uint32_t i = 0; local.get_next_generation("all_gather", gen); ++i)
    {
        std::vector<hpx::future<void>> sites;
        sites.reserve(num_sites);

        // launch num_sites threads to represent different sites
        for (std::uint32_t site = 0; site != num_sites; ++site)
        {
            sites.push_back(hpx::async([&, site, i] {
                hpx::chrono::high_resolution_timer const t;

                auto const value = site;

                hpx::future<std::vector<std::uint32_t>> overall_result =
                    all_gather(comms[site], value + i, this_site_arg(site),
                        generation_arg(gen));

                std::vector<std::uint32_t> const r = overall_result.get();
                HPX_TEST_EQ(r.size(), num_sites);

                for (std::size_t j = 0; j != r.size(); ++j)
                {
                    HPX_TEST_EQ(r[j], j + i);
                }

                if (site == 0)
                {
                    elapsed += t.elapsed();
                }
            }));
        }

        hpx::wait_all(std::move(sites));
    }

    return elapsed;
}

double test_local_all_reduce(std::vector<communicator> const& comms)
{
    double elapsed = 0.;

    std::size_t gen = 0;
    // clang-format off
    for ([[maybe_unused]] std::uint32_t i = 0;
         local.get_next_generation("all_reduce", gen); ++i)
    // clang-format on
    {
        std::vector<hpx::future<void>> sites;
        sites.reserve(num_sites);

        // launch num_sites threads to represent different sites
        for (std::uint32_t site = 0; site != num_sites; ++site)
        {
            sites.push_back(hpx::async([&, site] {
                hpx::chrono::high_resolution_timer const t;

                // test functionality based on immediate local result value
                auto value = site;

                hpx::future<std::uint32_t> result =
                    all_reduce(comms[site], value, std::plus<>{},
                        this_site_arg(site), generation_arg(gen));

                std::uint32_t sum = 0;
                for (std::uint32_t j = 0; j != num_sites; ++j)
                {
                    sum += j;
                }

                HPX_TEST_EQ(sum, result.get());

                if (site == 0)
                {
                    elapsed += t.elapsed();
                }
            }));
        }

        hpx::wait_all(std::move(sites));
    }

    return elapsed;
}

double test_local_all_to_all(std::vector<communicator> const& comms)
{
    double elapsed = 0.;

    std::size_t gen = 0;
    // clang-format off
    for ([[maybe_unused]] std::uint32_t i = 0;
         local.get_next_generation("all_to_all", gen); ++i)
    // clang-format on
    {
        std::vector<hpx::future<void>> sites;
        sites.reserve(num_sites);

        // launch num_sites threads to represent different sites
        for (std::uint32_t site = 0; site != num_sites; ++site)
        {
            sites.push_back(hpx::async([&, site]() {
                hpx::chrono::high_resolution_timer const t;

                std::vector<std::uint32_t> values(num_sites);
                std::fill(values.begin(), values.end(), site + i);

                hpx::future<std::vector<std::uint32_t>> overall_result =
                    all_to_all(comms[site], std::move(values),
                        this_site_arg(site), generation_arg(gen));

                std::vector<std::uint32_t> const r = overall_result.get();
                HPX_TEST_EQ(r.size(), num_sites);

                for (std::size_t j = 0; j != r.size(); ++j)
                {
                    HPX_TEST_EQ(r[j], j + i);
                }

                if (site == 0)
                {
                    elapsed += t.elapsed();
                }
            }));
        }

        hpx::wait_all(std::move(sites));
    }

    return elapsed;
}

double test_local_broadcast(std::vector<communicator> const& comms)
{
    double elapsed = 0.;

    std::size_t gen = 0;
    for (std::uint32_t i = 0; local.get_next_generation("broadcast", gen); ++i)
    {
        std::vector<hpx::future<void>> sites;
        sites.reserve(num_sites);

        // launch num_sites threads to represent different sites
        for (std::uint32_t site = 0; site != num_sites; ++site)
        {
            sites.push_back(hpx::async([&, site]() {
                hpx::chrono::high_resolution_timer const t;

                // test functionality based on immediate local result value
                if (site == 0)
                {
                    hpx::future<std::uint32_t> result =
                        broadcast_to(comms[site], 42 + i, this_site_arg(site),
                            generation_arg(gen));

                    HPX_TEST_EQ(42 + i, result.get());
                }
                else
                {
                    hpx::future<std::uint32_t> result =
                        hpx::collectives::broadcast_from<std::uint32_t>(
                            comms[site], this_site_arg(site),
                            generation_arg(gen));

                    HPX_TEST_EQ(42 + i, result.get());
                }

                if (site == 0)
                {
                    elapsed += t.elapsed();
                }
            }));
        }

        hpx::wait_all(std::move(sites));
    }

    return elapsed;
}

double test_local_exclusive_scan(std::vector<communicator> const& comms)
{
    double elapsed = 0.;

    std::size_t gen = 0;
    // clang-format off
    for (std::uint32_t i = 0; local.get_next_generation("exclusive_scan", gen);
         ++i)
    // clang-format on
    {
        std::vector<hpx::future<void>> sites;
        sites.reserve(num_sites);

        // launch num_sites threads to represent different sites
        for (std::uint32_t site = 0; site != num_sites; ++site)
        {
            sites.push_back(hpx::async([&, site]() {
                hpx::chrono::high_resolution_timer const t;

                hpx::future<std::uint32_t> overall_result =
                    exclusive_scan(comms[site], site + i, i, std::plus<>{},
                        this_site_arg(site), generation_arg(gen));

                auto const result = overall_result.get();

                std::uint32_t sum = i;
                for (std::uint32_t j = 0; j != site; ++j)
                {
                    sum += j + i;
                }
                HPX_TEST_EQ(sum, result);

                if (site == 0)
                {
                    elapsed += t.elapsed();
                }
            }));
        }

        hpx::wait_all(std::move(sites));
    }

    return elapsed;
}

double test_local_gather(std::vector<communicator> const& comms)
{
    double elapsed = 0.;

    std::size_t gen = 0;
    for (std::uint32_t i = 0; local.get_next_generation("gather", gen); ++i)
    {
        std::vector<hpx::future<void>> sites;
        sites.reserve(num_sites);

        // launch num_sites threads to represent different sites
        for (std::uint32_t site = 0; site != num_sites; ++site)
        {
            sites.push_back(hpx::async([&, site]() {
                hpx::chrono::high_resolution_timer const t;

                if (site == 0)
                {
                    hpx::future<std::vector<std::uint32_t>> overall_result =
                        gather_here(comms[site], 42 + i, generation_arg(gen),
                            this_site_arg(site));

                    std::vector<std::uint32_t> const sol = overall_result.get();
                    for (std::size_t j = 0; j != sol.size(); ++j)
                    {
                        HPX_TEST(j + 42 + i == sol[j]);
                    }
                }
                else
                {
                    hpx::future<void> overall_result =
                        gather_there(comms[site], site + 42 + i,
                            generation_arg(gen), this_site_arg(site));
                    overall_result.get();
                }

                if (site == 0)
                {
                    elapsed += t.elapsed();
                }
            }));
        }

        hpx::wait_all(std::move(sites));
    }

    return elapsed;
}

double test_local_inclusive_scan(std::vector<communicator> const& comms)
{
    double elapsed = 0.;

    std::size_t gen = 0;
    // clang-format off
    for (std::uint32_t i = 0; local.get_next_generation("inclusive_scan", gen);
         ++i)
    // clang-format on
    {
        std::vector<hpx::future<void>> sites;
        sites.reserve(num_sites);

        // launch num_sites threads to represent different sites
        for (std::uint32_t site = 0; site != num_sites; ++site)
        {
            sites.push_back(hpx::async([&, site]() {
                hpx::chrono::high_resolution_timer const t;

                hpx::future<std::uint32_t> overall_result =
                    inclusive_scan(comms[site], site + i, std::plus<>{},
                        this_site_arg(site), generation_arg(gen));

                auto const result = overall_result.get();

                std::uint32_t sum = 0;
                for (std::uint32_t j = 0; j != site + 1; ++j)
                {
                    sum += j + i;
                }
                HPX_TEST_EQ(sum, result);

                if (site == 0)
                {
                    elapsed += t.elapsed();
                }
            }));
        }

        hpx::wait_all(std::move(sites));
    }

    return elapsed;
}

double test_local_reduce(std::vector<communicator> const& comms)
{
    double elapsed = 0.;

    std::size_t gen = 0;
    for (std::uint32_t i = 0; local.get_next_generation("reduce", gen); ++i)
    {
        std::vector<hpx::future<void>> sites;
        sites.reserve(num_sites);

        // launch num_sites threads to represent different sites
        for (std::uint32_t site = 0; site != num_sites; ++site)
        {
            sites.push_back(hpx::async([&, site]() {
                hpx::chrono::high_resolution_timer const t;

                // test functionality based on immediate local result value
                auto value = site + i;
                if (site == 0)
                {
                    hpx::future<std::uint32_t> overall_result = reduce_here(
                        comms[site], std::move(value), std::plus<>{},
                        generation_arg(gen), this_site_arg(site));

                    std::uint32_t sum = 0;
                    for (std::uint32_t j = 0; j != num_sites; ++j)
                    {
                        sum += j + i;
                    }
                    HPX_TEST_EQ(sum, overall_result.get());
                }
                else
                {
                    hpx::future<void> overall_result =
                        reduce_there(comms[site], std::move(value),
                            generation_arg(gen), this_site_arg(site));
                    overall_result.get();
                }

                if (site == 0)
                {
                    elapsed += t.elapsed();
                }
            }));
        }

        hpx::wait_all(std::move(sites));
    }

    return elapsed;
}

double test_local_scatter(std::vector<communicator> const& comms)
{
    double elapsed = 0.;

    std::size_t gen = 0;
    for (std::uint32_t i = 0; local.get_next_generation("scatter", gen); ++i)
    {
        std::vector<hpx::future<void>> sites;
        sites.reserve(num_sites);

        // launch num_sites threads to represent different sites
        for (std::uint32_t site = 0; site != num_sites; ++site)
        {
            sites.push_back(hpx::async([&, site] {
                hpx::chrono::high_resolution_timer const t;

                if (site == 0)
                {
                    std::vector<std::uint32_t> data(num_sites);
                    std::iota(data.begin(), data.end(), 42 + i);

                    hpx::future<std::uint32_t> result =
                        scatter_to(comms[site], std::move(data),
                            generation_arg(gen), this_site_arg(site));

                    HPX_TEST_EQ(i + 42 + site, result.get());
                }
                else
                {
                    hpx::future<std::uint32_t> result =
                        scatter_from<std::uint32_t>(comms[site],
                            generation_arg(gen), this_site_arg(site));

                    HPX_TEST_EQ(i + 42 + site, result.get());
                }

                if (site == 0)
                {
                    elapsed += t.elapsed();
                }
            }));
        }

        hpx::wait_all(std::move(sites));
    }

    return elapsed;
}

////////////////////////////////////////////////////////////////////////////////
int hpx_main(hpx::program_options::variables_map& vm)
{
    std::uint32_t const here = hpx::get_locality_id();

    unsigned int seed = std::random_device{}();
    if (vm.count("seed"))
        seed = vm["seed"].as<unsigned int>();

    std::cout << "using seed: " << seed << std::endl;
    std::mt19937 gen(seed);

    if (here == 0)
    {
        distributed.init(gen, ITERATIONS);
        local.init(gen,
            static_cast<std::size_t>(10) *
                static_cast<std::size_t>(ITERATIONS));
    }
    else
    {
        auto console = hpx::agas::get_console_locality();
        distributed =
            hpx::async(get_generations_action(), console, false).get();
        local = hpx::async(get_generations_action(), console, true).get();
    }

#if defined(HPX_HAVE_NETWORKING)
    if (hpx::get_num_localities(hpx::launch::sync) > 1)
    {
        std::uint32_t const num_localities =
            hpx::get_num_localities(hpx::launch::sync);
        HPX_TEST_LTE(static_cast<std::uint32_t>(2), num_localities);

        auto const concurrent_comm = create_communicator(concurrent_basename,
            num_sites_arg(num_localities), this_site_arg(here));

        auto f1 =
            hpx::async(&test_all_gather, concurrent_comm, num_localities, here);
        auto f2 =
            hpx::async(&test_all_reduce, concurrent_comm, num_localities, here);
        auto f3 =
            hpx::async(&test_all_to_all, concurrent_comm, num_localities, here);
        auto f4 = hpx::async(&test_broadcast, concurrent_comm, here);
        auto f5 = hpx::async(&test_exclusive_scan, concurrent_comm, here);
        auto f6 = hpx::async(&test_gather, concurrent_comm, here);
        auto f7 = hpx::async(&test_inclusive_scan, concurrent_comm, here);
        auto f8 =
            hpx::async(&test_reduce, concurrent_comm, num_localities, here);
        auto f9 =
            hpx::async(&test_scatter, concurrent_comm, num_localities, here);

        hpx::wait_all(f1, f2, f3, f4, f5, f6, f7, f8, f9);

        if (here == 0)
        {
            // NOLINTBEGIN(bugprone-narrowing-conversions)
            std::cout << "remote all_gather timing:     "
                      << f1.get() / distributed.get_iterations("all_gather")
                      << " [s]\n";
            std::cout << "remote all_reduce timing:     "
                      << f2.get() / distributed.get_iterations("all_reduce")
                      << " [s]\n";
            std::cout << "remote all_to_all timing:     "
                      << f3.get() / distributed.get_iterations("all_to_all")
                      << " [s]\n";
            std::cout << "remote broadcast timing:      "
                      << f4.get() / distributed.get_iterations("broadcast")
                      << " [s]\n";
            std::cout << "remote exclusive_scan timing: "
                      << f5.get() / distributed.get_iterations("exclusive_scan")
                      << " [s]\n";
            std::cout << "remote gather timing:         "
                      << f6.get() / distributed.get_iterations("gather")
                      << " [s]\n";
            std::cout << "remote inclusive_scan timing: "
                      << f7.get() / distributed.get_iterations("inclusive_scan")
                      << " [s]\n";
            std::cout << "remote reduce timing:         "
                      << f8.get() / distributed.get_iterations("reduce")
                      << " [s]\n";
            std::cout << "remote scatter timing:        "
                      << f9.get() / distributed.get_iterations("scatter")
                      << " [s]\n";
            // NOLINTEND(bugprone-narrowing-conversions)
        }
    }
#endif

    if (here == 0)
    {
        std::vector<communicator> concurrent_comms;
        concurrent_comms.reserve(num_sites);

        for (std::uint32_t site = 0; site != num_sites; ++site)
        {
            concurrent_comms.push_back(
                create_local_communicator(concurrent_basename,
                    num_sites_arg(num_sites), this_site_arg(site)));
        }

        auto f1 = hpx::async(&test_local_all_gather, concurrent_comms);
        auto f2 = hpx::async(&test_local_all_reduce, concurrent_comms);
        auto f3 = hpx::async(&test_local_all_to_all, concurrent_comms);
        auto f4 = hpx::async(&test_local_broadcast, concurrent_comms);
        auto f5 = hpx::async(&test_local_exclusive_scan, concurrent_comms);
        auto f6 = hpx::async(&test_local_gather, concurrent_comms);
        auto f7 = hpx::async(&test_local_inclusive_scan, concurrent_comms);
        auto f8 = hpx::async(&test_local_reduce, concurrent_comms);
        auto f9 = hpx::async(&test_local_scatter, concurrent_comms);

        hpx::wait_all(f1, f2, f3, f4, f5, f6, f7, f8, f9);

        // NOLINTBEGIN(bugprone-narrowing-conversions)
        std::cout << "local all_gather timing:     "
                  << f1.get() / local.get_iterations("all_gather") << " [s]\n";
        std::cout << "local all_reduce timing:     "
                  << f2.get() / local.get_iterations("all_reduce") << " [s]\n";
        std::cout << "local all_to_all timing:     "
                  << f3.get() / local.get_iterations("all_to_all") << " [s]\n";
        std::cout << "local broadcast timing:      "
                  << f4.get() / local.get_iterations("broadcast") << " [s]\n";
        std::cout << "local exclusive_scan timing: "
                  << f5.get() / local.get_iterations("exclusive_scan")
                  << " [s]\n";
        std::cout << "local gather timing:         "
                  << f6.get() / local.get_iterations("gather") << " [s]\n";
        std::cout << "local inclusive_scan timing: "
                  << f7.get() / local.get_iterations("inclusive_scan")
                  << " [s]\n";
        std::cout << "local reduce timing:         "
                  << f8.get() / local.get_iterations("reduce") << " [s]\n";
        std::cout << "local scatter timing:        "
                  << f9.get() / local.get_iterations("scatter") << " [s]\n";
        // NOLINTEND(bugprone-narrowing-conversions)
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    // add command line option which controls the random number generator seed
    using namespace hpx::program_options;
    options_description desc_commandline(
        "Usage: " HPX_APPLICATION_STRING " [options]");

    desc_commandline.add_options()("seed,s", value<unsigned int>(),
        "the random number generator seed to use for this run");

    std::vector<std::string> const cfg = {"hpx.run_hpx_main!=1"};

    hpx::init_params init_args;
    init_args.desc_cmdline = desc_commandline;
    init_args.cfg = cfg;

    HPX_TEST_EQ(hpx::init(argc, argv, init_args), 0);
    return hpx::util::report_errors();
}

#endif
