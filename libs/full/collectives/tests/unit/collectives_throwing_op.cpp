//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// A collective finalizer that throws for one site must not be re-entered for
// the remaining sites: the shared payload and the captured operator have
// already been consumed by the failed fold. The communicator caches the
// first failure and rethrows it for every site, so all sites observe the
// same outcome, and the communicator stays usable for the next generation.
//
// The reduction operator below throws on exactly one invocation and then
// behaves like plus. Without the failure caching, the sites whose callbacks
// run after the failed one would re-enter the fold, succeed on moved-from
// state, and produce results that differ from the exception observed by the
// first site.
//
// A throwing step function (assigning the contributed value into the
// collected data) is covered as well: the failure is cached the same way so
// the gate still completes, and every site - with or without a finalizer -
// observes the cached exception instead of waiting forever.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace hpx::collectives;

constexpr char const* error_marker = "collectives_throwing_op";

std::atomic<int> op_calls{0};

struct throw_once_plus
{
    int operator()(int const lhs, int const rhs) const
    {
        if (op_calls++ == 0)
        {
            throw std::runtime_error(error_marker);
        }
        return lhs + rhs;
    }
};

// Payload whose assignment throws while armed. The step function assigns the
// contributed value into the collected data, so an armed payload makes the
// step of exactly one site fail.
struct throwing_payload
{
    throwing_payload() = default;

    explicit throwing_payload(int const value, bool const armed = false)
      : value(value)
      , armed(armed)
    {
    }

    throwing_payload(throwing_payload const&) = default;
    throwing_payload(throwing_payload&&) = default;

    throwing_payload& operator=(throwing_payload const& rhs)
    {
        throw_if_armed(rhs);
        value = rhs.value;
        armed = rhs.armed;
        return *this;
    }

    throwing_payload& operator=(throwing_payload&& rhs)
    {
        throw_if_armed(rhs);
        value = rhs.value;
        armed = rhs.armed;
        return *this;
    }

    static void throw_if_armed(throwing_payload const& rhs)
    {
        if (rhs.armed)
        {
            throw std::runtime_error(error_marker);
        }
    }

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & value;
        ar & armed;
    }

    int value = 0;
    bool armed = false;
};

std::vector<communicator> create_communicators(
    char const* phase, std::uint32_t num_sites)
{
    std::string const basename = std::string("/test/collectives_throwing_op/") +
        phase + "/" + std::to_string(num_sites) + "/";

    std::vector<communicator> comms;
    comms.reserve(num_sites);
    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        comms.push_back(create_local_communicator(
            basename.c_str(), num_sites_arg(num_sites), this_site_arg(site)));
    }
    return comms;
}

template <typename T>
void expect_marker_failure(hpx::future<T>&& failure)
{
    try
    {
        failure.get();
        HPX_TEST(false);
    }
    catch (std::runtime_error const& e)
    {
        // the transported message keeps the original text, possibly
        // followed by HPX diagnostic annotations
        HPX_TEST_NEQ(
            std::string(e.what()).find(error_marker), std::string::npos);
    }
    catch (...)
    {
        HPX_TEST(false);
    }
}

template <typename Collective, typename Op>
std::vector<hpx::future<int>> run_generation(
    std::vector<communicator> const& comms, std::uint32_t const generation,
    Collective&& collective, Op&& op)
{
    std::uint32_t const num_sites = static_cast<std::uint32_t>(comms.size());

    std::vector<hpx::future<int>> results;
    results.reserve(num_sites);
    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        results.push_back(collective(comms[site], site, generation, op));
    }
    return results;
}

// generation 1: the operator throws once; every site must observe that one
// cached failure and the operator must not be re-invoked. generation 2: the
// same communicator produces correct results again.
template <typename Collective, typename Expected>
void test_collective(char const* phase, std::uint32_t const num_sites,
    Collective&& collective, Expected&& expected)
{
    auto const comms = create_communicators(phase, num_sites);

    op_calls = 0;

    auto failures = run_generation(comms, 1, collective, throw_once_plus{});
    for (auto& failure : failures)
    {
        expect_marker_failure(HPX_MOVE(failure));
    }
    HPX_TEST_EQ(op_calls.load(), 1);

    auto results = run_generation(comms, 2, collective, std::plus<int>{});
    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        HPX_TEST_EQ(results[site].get(), expected(site));
    }
}

void test_all_reduce(std::uint32_t const num_sites)
{
    int expected = 0;
    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        expected += static_cast<int>(site) + 1;
    }

    test_collective(
        "all_reduce", num_sites,
        [](communicator const& comm, std::uint32_t const site,
            std::uint32_t const generation, auto op) {
            return all_reduce(comm, static_cast<int>(site) + 1, HPX_MOVE(op),
                this_site_arg(site), generation_arg(generation));
        },
        [expected](std::uint32_t) { return expected; });
}

void test_inclusive_scan(std::uint32_t const num_sites)
{
    test_collective(
        "inclusive_scan", num_sites,
        [](communicator const& comm, std::uint32_t const site,
            std::uint32_t const generation, auto op) {
            return inclusive_scan(comm, static_cast<int>(site) + 1,
                HPX_MOVE(op), this_site_arg(site), generation_arg(generation));
        },
        [](std::uint32_t const site) {
            int const rank = static_cast<int>(site);
            return (rank + 1) * (rank + 2) / 2;
        });
}

void test_exclusive_scan(std::uint32_t const num_sites)
{
    test_collective(
        "exclusive_scan", num_sites,
        [](communicator const& comm, std::uint32_t const site,
            std::uint32_t const generation, auto op) {
            return exclusive_scan(comm, static_cast<int>(site) + 1,
                HPX_MOVE(op), this_site_arg(site), generation_arg(generation));
        },
        [](std::uint32_t const site) {
            int const rank = static_cast<int>(site);
            return rank * (rank + 1) / 2;
        });
}

void test_exclusive_scan_init(std::uint32_t const num_sites)
{
    test_collective(
        "exclusive_scan_init", num_sites,
        [](communicator const& comm, std::uint32_t const site,
            std::uint32_t const generation, auto op) {
            return exclusive_scan(comm, static_cast<int>(site) + 1, 10,
                HPX_MOVE(op), this_site_arg(site), generation_arg(generation));
        },
        [](std::uint32_t const site) {
            int const rank = static_cast<int>(site);
            return 10 + rank * (rank + 1) / 2;
        });
}

// generation 1: the step of the armed site fails while assigning its
// contribution; every site, including the one that threw, must observe the
// cached failure instead of waiting on a gate segment that is never set.
// generation 2: the same communicator produces correct results again.
void test_step_throw(std::uint32_t const num_sites)
{
    auto const comms = create_communicators("step_throw", num_sites);

    auto const all_reduce_payloads = [&](std::uint32_t const generation,
                                         bool const armed) {
        std::vector<hpx::future<throwing_payload>> results;
        results.reserve(num_sites);
        for (std::uint32_t site = 0; site != num_sites; ++site)
        {
            results.push_back(all_reduce(
                comms[site],
                throwing_payload(
                    static_cast<int>(site) + 1, armed && site == 1),
                [](throwing_payload const& lhs, throwing_payload const& rhs) {
                    return throwing_payload(lhs.value + rhs.value);
                },
                this_site_arg(site), generation_arg(generation)));
        }
        return results;
    };

    for (auto& failure : all_reduce_payloads(1, true))
    {
        expect_marker_failure(HPX_MOVE(failure));
    }

    int expected = 0;
    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        expected += static_cast<int>(site) + 1;
    }

    for (auto& result : all_reduce_payloads(2, false))
    {
        HPX_TEST_EQ(result.get().value, expected);
    }
}

// A failing step must also fail the sites that pass no finalizer: reduce
// collects the contributions of the reduce_there sites, whose futures carry
// no value at all.
void test_step_throw_reduce(std::uint32_t const num_sites)
{
    auto const comms = create_communicators("step_throw_reduce", num_sites);

    auto const run_reduce = [&](std::uint32_t const generation,
                                bool const armed) {
        auto root = reduce_here(
            comms[0], throwing_payload(1),
            [](throwing_payload const& lhs, throwing_payload const& rhs) {
                return throwing_payload(lhs.value + rhs.value);
            },
            this_site_arg(0), generation_arg(generation));

        std::vector<hpx::future<void>> there;
        there.reserve(num_sites - 1);
        for (std::uint32_t site = 1; site != num_sites; ++site)
        {
            there.push_back(reduce_there(comms[site],
                throwing_payload(
                    static_cast<int>(site) + 1, armed && site == 1),
                this_site_arg(site), generation_arg(generation)));
        }
        return std::make_pair(HPX_MOVE(root), HPX_MOVE(there));
    };

    {
        auto [root, there] = run_reduce(1, true);
        expect_marker_failure(HPX_MOVE(root));
        for (auto& failure : there)
        {
            expect_marker_failure(HPX_MOVE(failure));
        }
    }

    {
        auto [root, there] = run_reduce(2, false);

        int expected = 0;
        for (std::uint32_t site = 0; site != num_sites; ++site)
        {
            expected += static_cast<int>(site) + 1;
        }
        HPX_TEST_EQ(root.get().value, expected);
        for (auto& done : there)
        {
            done.get();    // must not throw
        }
    }
}

int hpx_main()
{
    if (hpx::get_locality_id() == 0)
    {
        test_all_reduce(3);
        test_all_reduce(5);
        test_inclusive_scan(3);
        test_exclusive_scan(3);
        test_exclusive_scan_init(3);
        test_step_throw(3);
        test_step_throw_reduce(3);
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
