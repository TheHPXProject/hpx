//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// The pairwise all_to_all exchange delivers every row straight to the site it
// belongs to instead of routing it through one communicator site. These tests
// pin down the result layout it has to produce, which is the same layout the
// communicator-based all_to_all produces, plus the tag separation that lets
// one channel communicator carry several exchanges.

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <new>
#include <string>
#include <utility>
#include <vector>

using namespace hpx::collectives;

///////////////////////////////////////////////////////////////////////////////
// The value site source contributes for site destination. Encoding both ends
// in the value is what makes a misrouted row visible.
constexpr std::size_t exchanged_value(
    std::size_t const source, std::size_t const destination) noexcept
{
    return source * 1000 + destination;
}

std::vector<channel_communicator> create_communicators(
    char const* const phase, std::size_t const num_sites)
{
    std::string const basename =
        std::string("/test/pairwise_all_to_all/") + phase + "/";

    std::vector<hpx::future<channel_communicator>> comm_futures;
    comm_futures.reserve(num_sites);

    for (std::size_t site = 0; site != num_sites; ++site)
    {
        comm_futures.push_back(create_channel_communicator(
            basename.c_str(), num_sites_arg(num_sites), this_site_arg(site)));
    }

    hpx::wait_all(comm_futures);

    std::vector<channel_communicator> comms;
    comms.reserve(num_sites);
    for (auto& comm_future : comm_futures)
    {
        comms.push_back(comm_future.get());
    }
    return comms;
}

///////////////////////////////////////////////////////////////////////////////
// Every site contributes one scalar row per peer and must receive exactly the
// row each peer addressed to it.
void run_scalar_site(channel_communicator comm, std::size_t const num_sites,
    std::size_t const this_site, std::size_t const tag)
{
    std::vector<std::size_t> local_result;
    local_result.reserve(num_sites);
    for (std::size_t destination = 0; destination != num_sites; ++destination)
    {
        local_result.push_back(exchanged_value(this_site, destination));
    }

    std::vector<std::size_t> const result =
        detail::pairwise_all_to_all(HPX_MOVE(comm), HPX_MOVE(local_result),
            num_sites, this_site, tag_arg(tag))
            .get();

    HPX_TEST_EQ(result.size(), num_sites);
    for (std::size_t source = 0; source != num_sites; ++source)
    {
        HPX_TEST_EQ(result[source], exchanged_value(source, this_site));
    }
}

void test_scalar_exchange(char const* const phase, std::size_t const num_sites)
{
    auto comms = create_communicators(phase, num_sites);

    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);
    for (std::size_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async(run_scalar_site, HPX_MOVE(comms[site]),
            num_sites, site, std::size_t(1)));
    }

    hpx::wait_all(sites);
}

///////////////////////////////////////////////////////////////////////////////
// vector<bool> rows are proxy references until they are materialized. The
// direct path must send bool values, not the implementation-specific proxy.
void run_bool_site(channel_communicator comm, std::size_t const num_sites,
    std::size_t const this_site)
{
    std::vector<bool> local_result;
    local_result.reserve(num_sites);
    for (std::size_t destination = 0; destination != num_sites; ++destination)
    {
        local_result.push_back((this_site + destination) % 2 != 0);
    }

    std::vector<bool> const result = detail::pairwise_all_to_all(HPX_MOVE(comm),
        HPX_MOVE(local_result), num_sites, this_site, tag_arg(1))
                                         .get();

    HPX_TEST_EQ(result.size(), num_sites);
    for (std::size_t source = 0; source != num_sites; ++source)
    {
        HPX_TEST_EQ(result[source], (source + this_site) % 2 != 0);
    }
}

void test_bool_exchange(std::size_t const num_sites)
{
    auto comms = create_communicators("bool", num_sites);

    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);
    for (std::size_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(
            hpx::async(run_bool_site, HPX_MOVE(comms[site]), num_sites, site));
    }

    hpx::wait_all(sites);
}

///////////////////////////////////////////////////////////////////////////////
// The payload this path exists for is a block per peer, not a scalar.
void run_block_site(channel_communicator comm, std::size_t const num_sites,
    std::size_t const this_site, std::size_t const block_size)
{
    std::vector<std::vector<std::size_t>> local_result;
    local_result.reserve(num_sites);
    for (std::size_t destination = 0; destination != num_sites; ++destination)
    {
        local_result.emplace_back(
            block_size, exchanged_value(this_site, destination));
    }

    std::vector<std::vector<std::size_t>> const result =
        detail::pairwise_all_to_all(HPX_MOVE(comm), HPX_MOVE(local_result),
            num_sites, this_site, tag_arg(1))
            .get();

    HPX_TEST_EQ(result.size(), num_sites);
    for (std::size_t source = 0; source != num_sites; ++source)
    {
        HPX_TEST_EQ(result[source].size(), block_size);
        for (std::size_t const value : result[source])
        {
            HPX_TEST_EQ(value, exchanged_value(source, this_site));
        }
    }
}

void test_block_exchange(char const* const phase, std::size_t const num_sites)
{
    constexpr std::size_t block_size = 128;

    auto comms = create_communicators(phase, num_sites);

    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);
    for (std::size_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async(run_block_site, HPX_MOVE(comms[site]),
            num_sites, site, block_size));
    }

    hpx::wait_all(sites);
}

///////////////////////////////////////////////////////////////////////////////
// The existing all_to_all accepts rows that can be constructed in place but
// cannot be default-constructed or assigned. The direct path must preserve
// that contract.
struct nonassignable_row
{
    nonassignable_row() = delete;
    nonassignable_row(nonassignable_row const&) = default;
    nonassignable_row& operator=(nonassignable_row const&) = delete;
    nonassignable_row(nonassignable_row&&) = default;
    nonassignable_row& operator=(nonassignable_row&&) = delete;

    explicit nonassignable_row(std::size_t const value)
      : value(value)
    {
    }

    template <typename Archive>
    void serialize(Archive&, unsigned int const)
    {
    }

    template <typename Archive>
    friend void save_construct_data(
        Archive& ar, nonassignable_row const* const row, unsigned int const)
    {
        ar & row->value;
    }

    template <typename Archive>
    friend void load_construct_data(
        Archive& ar, nonassignable_row* const row, unsigned int const)
    {
        std::size_t value = 0;
        ar & value;
        ::new (row) nonassignable_row(value);
    }

    std::size_t value;
};

void run_nonassignable_site(channel_communicator comm,
    std::size_t const num_sites, std::size_t const this_site)
{
    std::vector<nonassignable_row> local_result;
    local_result.reserve(num_sites);
    for (std::size_t destination = 0; destination != num_sites; ++destination)
    {
        local_result.emplace_back(exchanged_value(this_site, destination));
    }

    std::vector<nonassignable_row> const result =
        detail::pairwise_all_to_all(HPX_MOVE(comm), HPX_MOVE(local_result),
            num_sites, this_site, tag_arg(1))
            .get();

    HPX_TEST_EQ(result.size(), num_sites);
    for (std::size_t source = 0; source != num_sites; ++source)
    {
        HPX_TEST_EQ(result[source].value, exchanged_value(source, this_site));
    }
}

void test_nonassignable_exchange(std::size_t const num_sites)
{
    auto comms = create_communicators("nonassignable", num_sites);

    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);
    for (std::size_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async(
            run_nonassignable_site, HPX_MOVE(comms[site]), num_sites, site));
    }

    hpx::wait_all(sites);
}

///////////////////////////////////////////////////////////////////////////////
// Two exchanges share one communicator and must not mix, which is what the
// tag is for. Both run concurrently so an ordering assumption would show.
void run_two_tags(channel_communicator comm, std::size_t const num_sites,
    std::size_t const this_site)
{
    std::vector<hpx::future<void>> exchanges;
    exchanges.reserve(2);

    for (std::size_t tag = 1; tag != 3; ++tag)
    {
        std::vector<std::size_t> local_result;
        local_result.reserve(num_sites);
        for (std::size_t destination = 0; destination != num_sites;
            ++destination)
        {
            local_result.push_back(
                tag * exchanged_value(this_site, destination));
        }

        exchanges.push_back(detail::pairwise_all_to_all(
            comm, HPX_MOVE(local_result), num_sites, this_site, tag_arg(tag))
                .then(hpx::launch::sync, [num_sites, this_site, tag](auto&& f) {
                    std::vector<std::size_t> const result = f.get();

                    HPX_TEST_EQ(result.size(), num_sites);
                    for (std::size_t source = 0; source != num_sites; ++source)
                    {
                        HPX_TEST_EQ(result[source],
                            tag * exchanged_value(source, this_site));
                    }
                }));
    }

    hpx::wait_all(exchanges);
}

void test_tag_separation(std::size_t const num_sites)
{
    auto comms = create_communicators("tags", num_sites);

    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);
    for (std::size_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(
            hpx::async(run_two_tags, HPX_MOVE(comms[site]), num_sites, site));
    }

    hpx::wait_all(sites);
}

///////////////////////////////////////////////////////////////////////////////
// A malformed call has to fail through the future rather than exchange a
// truncated row set.
void test_rejects_bad_arguments()
{
    constexpr std::size_t num_sites = 2;

    auto comms = create_communicators("validation", num_sites);

    bool caught_wrong_size = false;
    try
    {
        std::vector<std::size_t> too_few(num_sites - 1);
        detail::pairwise_all_to_all(
            comms[0], HPX_MOVE(too_few), num_sites, 0, tag_arg(1))
            .get();
    }
    catch (hpx::exception const& e)
    {
        caught_wrong_size = e.get_error() == hpx::error::bad_parameter;
    }
    HPX_TEST(caught_wrong_size);

    bool caught_bad_site = false;
    try
    {
        std::vector<std::size_t> rows(num_sites);
        detail::pairwise_all_to_all(
            comms[0], HPX_MOVE(rows), num_sites, num_sites, tag_arg(1))
            .get();
    }
    catch (hpx::exception const& e)
    {
        caught_bad_site = e.get_error() == hpx::error::bad_parameter;
    }
    HPX_TEST(caught_bad_site);
}

struct opaque_row
{
    std::vector<int> data;

    template <typename Archive>
    void serialize(Archive& ar, unsigned)
    {
        // clang-format off
        ar & data;
        // clang-format on
    }
};

// An automatic decision may only rest on what every site computes the same
// way, which is the element type. Measuring contributed rows cannot serve:
// two sites are free to contribute rows of different length, and if that
// split the exchange path between them the operation would never complete.
void test_type_size_estimate()
{
    static_assert(
        detail::pairwise_type_bytes<std::size_t>() == sizeof(std::size_t));
    static_assert(detail::pairwise_type_bytes<std::vector<int>>() == 0);
    static_assert(detail::pairwise_type_bytes<opaque_row>() == 0);
}

void test_dispatch_decision()
{
    constexpr auto threshold = pairwise_threshold_arg(4096);

    // below three sites there is no routing detour to remove
    HPX_TEST(!detail::exchange_pairwise(1, 1 << 20, threshold));
    HPX_TEST(!detail::exchange_pairwise(2, 1 << 20, threshold));

    // small rows stay on the routed path, large rows go direct
    HPX_TEST(!detail::exchange_pairwise(8, 4095, threshold));
    HPX_TEST(detail::exchange_pairwise(8, 4096, threshold));
    HPX_TEST(detail::exchange_pairwise(8, 1 << 20, threshold));

    // an unmeasurable payload never selects the direct path on size alone
    HPX_TEST(!detail::exchange_pairwise(8, 0, threshold));

    // a zero threshold forces the direct path, unmeasurable payload included
    HPX_TEST(detail::exchange_pairwise(8, 0, pairwise_threshold_arg(0)));
    HPX_TEST(detail::exchange_pairwise(8, 1, pairwise_threshold_arg(0)));
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_type_size_estimate();
    test_dispatch_decision();

    // A single site exchanges with itself and never touches the network.
    test_scalar_exchange("scalar-1", 1);

    // Odd and non-power-of-two counts exercise the send/receive stagger.
    test_scalar_exchange("scalar-2", 2);
    test_scalar_exchange("scalar-3", 3);
    test_scalar_exchange("scalar-5", 5);
    test_scalar_exchange("scalar-8", 8);
    test_bool_exchange(5);

    test_block_exchange("block-4", 4);
    test_block_exchange("block-7", 7);
    test_nonassignable_exchange(4);

    test_tag_separation(4);

    test_rejects_bad_arguments();

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::init(argc, argv), 0);
    return hpx::util::report_errors();
}
#endif
