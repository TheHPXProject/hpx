//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The all_reduce finalizer seeds its fold with data[0], which is excluded
// from the reduced range. These tests pin down the seed handling: the seed
// must be moved rather than copied, and the fold must remain correct for
// move-heavy, boolean, and throwing reduction operators.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace hpx::collectives;

///////////////////////////////////////////////////////////////////////////////
// Counts copies made from the value contributed by site zero. That value
// becomes the seed of the reduction fold; it must be moved, never copied.
struct copy_tracker
{
    static inline std::atomic<int> seed_copies{0};

    copy_tracker() = default;

    explicit copy_tracker(int value, bool seed = false) noexcept
      : value(value)
      , seed(seed)
    {
    }

    copy_tracker(copy_tracker const& rhs) noexcept
      : value(rhs.value)
      , seed(rhs.seed)
    {
        if (rhs.seed)
        {
            ++seed_copies;
        }
    }

    copy_tracker(copy_tracker&& rhs) noexcept
      : value(rhs.value)
      , seed(std::exchange(rhs.seed, false))
    {
    }

    copy_tracker& operator=(copy_tracker const& rhs) noexcept
    {
        value = rhs.value;
        seed = rhs.seed;
        if (rhs.seed)
        {
            ++seed_copies;
        }
        return *this;
    }

    copy_tracker& operator=(copy_tracker&& rhs) noexcept
    {
        value = rhs.value;
        seed = std::exchange(rhs.seed, false);
        return *this;
    }

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        ar & value;
        ar & seed;
    }

    int value = 0;
    bool seed = false;
};

std::vector<communicator> create_communicators(
    char const* phase, std::uint32_t num_sites)
{
    std::string const basename = std::string("/test/all_reduce_move_seed/") +
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

void test_seed_is_moved_not_copied(std::uint32_t num_sites)
{
    auto const comms = create_communicators("copy", num_sites);

    copy_tracker::seed_copies = 0;

    std::vector<hpx::future<copy_tracker>> results;
    results.reserve(num_sites);
    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        results.push_back(all_reduce(
            comms[site], copy_tracker(static_cast<int>(site) + 1, site == 0),
            [](copy_tracker const& lhs, copy_tracker const& rhs) {
                return copy_tracker(lhs.value + rhs.value);
            },
            this_site_arg(site), generation_arg(1)));
    }

    int expected = 0;
    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        expected += static_cast<int>(site) + 1;
    }

    for (auto& result : results)
    {
        HPX_TEST_EQ(result.get().value, expected);
    }

    HPX_TEST_EQ(copy_tracker::seed_copies.load(), 0);
}

void test_vector_payload(std::uint32_t num_sites)
{
    constexpr std::size_t size = 512;

    auto const comms = create_communicators("vector", num_sites);

    std::vector<hpx::future<std::vector<int>>> results;
    results.reserve(num_sites);
    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        results.push_back(all_reduce(
            comms[site], std::vector<int>(size, static_cast<int>(site) + 1),
            [](std::vector<int> const& lhs, std::vector<int> const& rhs) {
                HPX_TEST_EQ(lhs.size(), rhs.size());
                std::vector<int> sum(lhs.size());
                for (std::size_t i = 0; i != lhs.size(); ++i)
                {
                    sum[i] = lhs[i] + rhs[i];
                }
                return sum;
            },
            this_site_arg(site), generation_arg(1)));
    }

    int expected = 0;
    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        expected += static_cast<int>(site) + 1;
    }

    for (auto& result : results)
    {
        std::vector<int> const value = result.get();
        HPX_TEST_EQ(value.size(), size);
        for (int element : value)
        {
            HPX_TEST_EQ(element, expected);
        }
    }
}

void test_bool_payload(std::uint32_t num_sites)
{
    auto const comms = create_communicators("bool", num_sites);

    // generation 1: every site contributes true, generation 2: site one
    // contributes false
    for (std::uint32_t generation = 1; generation != 3; ++generation)
    {
        std::vector<hpx::future<bool>> results;
        results.reserve(num_sites);
        for (std::uint32_t site = 0; site != num_sites; ++site)
        {
            bool const value = generation == 1 || site != 1;

            results.push_back(all_reduce(
                comms[site], value,
                [](bool lhs, bool rhs) { return lhs && rhs; },
                this_site_arg(site), generation_arg(generation)));
        }

        bool const expected = generation == 1;
        for (auto& result : results)
        {
            HPX_TEST_EQ(result.get(), expected);
        }
    }
}

void test_throwing_operator(std::uint32_t num_sites)
{
    auto const comms = create_communicators("throw", num_sites);

    // generation 1: the operator throws; every site must observe the error
    std::vector<hpx::future<int>> failures;
    failures.reserve(num_sites);
    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        failures.push_back(all_reduce(
            comms[site], static_cast<int>(site),
            [](int, int) -> int {
                throw std::runtime_error("all_reduce_move_seed");
            },
            this_site_arg(site), generation_arg(1)));
    }

    for (auto& failure : failures)
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
            HPX_TEST_NEQ(std::string(e.what()).find("all_reduce_move_seed"),
                std::string::npos);
        }
        catch (...)
        {
            HPX_TEST(false);
        }
    }

    // generation 2: the communicator remains usable after the failure
    std::vector<hpx::future<int>> results;
    results.reserve(num_sites);
    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        results.push_back(all_reduce(comms[site], static_cast<int>(site) + 1,
            std::plus<int>{}, this_site_arg(site), generation_arg(2)));
    }

    int expected = 0;
    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        expected += static_cast<int>(site) + 1;
    }

    for (auto& result : results)
    {
        HPX_TEST_EQ(result.get(), expected);
    }
}

int hpx_main()
{
    if (hpx::get_locality_id() == 0)
    {
        test_seed_is_moved_not_copied(2);
        test_seed_is_moved_not_copied(4);
        test_vector_payload(4);
        test_bool_payload(4);
        test_throwing_operator(4);
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
