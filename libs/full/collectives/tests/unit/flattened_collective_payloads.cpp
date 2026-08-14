//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/collectives.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <cstdint>
#include <new>
#include <string>
#include <utility>
#include <vector>

using namespace hpx::collectives;

struct nonassignable_payload
{
    nonassignable_payload() = delete;
    nonassignable_payload(nonassignable_payload const&) = default;
    nonassignable_payload& operator=(nonassignable_payload const&) = delete;
    nonassignable_payload(nonassignable_payload&&) = default;
    nonassignable_payload& operator=(nonassignable_payload&&) = delete;

    explicit nonassignable_payload(std::uint32_t const value)
      : value(value)
    {
    }

    template <typename Archive>
    void serialize(Archive&, unsigned int const)
    {
    }

    template <typename Archive>
    friend void save_construct_data(Archive& ar,
        nonassignable_payload const* const payload, unsigned int const)
    {
        ar & payload->value;
    }

    template <typename Archive>
    friend void load_construct_data(
        Archive& ar, nonassignable_payload* const payload, unsigned int const)
    {
        std::uint32_t value = 0;
        ar & value;
        ::new (payload) nonassignable_payload(value);
    }

    std::uint32_t value;
};

template <typename Payload>
struct opaque_payload_traits;

template <>
struct opaque_payload_traits<
    hpx::collectives::detail::ragged_rows<std::uint32_t>>
{
    using payload_type = hpx::collectives::detail::ragged_rows<std::uint32_t>;

    static payload_type make(std::uint32_t const value)
    {
        return payload_type(
            std::vector<std::uint32_t>{value}, std::vector<std::size_t>{0, 1});
    }

    static void check_payload(
        payload_type const& payload, std::uint32_t const expected)
    {
        HPX_TEST_EQ(payload.data().size(), static_cast<std::size_t>(1));
        HPX_TEST_EQ(payload.data()[0], expected);
        HPX_TEST_EQ(payload.offsets().size(), static_cast<std::size_t>(2));
        HPX_TEST_EQ(payload.offsets()[0], static_cast<std::size_t>(0));
        HPX_TEST_EQ(payload.offsets()[1], static_cast<std::size_t>(1));
    }
};

template <>
struct opaque_payload_traits<
    hpx::collectives::detail::uniform_rows<std::uint32_t>>
{
    using payload_type = hpx::collectives::detail::uniform_rows<std::uint32_t>;

    static payload_type make(std::uint32_t const value)
    {
        return payload_type(std::vector<std::uint32_t>{value}, 1);
    }

    static void check_payload(
        payload_type const& payload, std::uint32_t const expected)
    {
        HPX_TEST_EQ(payload.data().size(), static_cast<std::size_t>(1));
        HPX_TEST_EQ(payload.data()[0], expected);
        HPX_TEST_EQ(payload.num_rows(), static_cast<std::size_t>(1));
    }
};

template <typename Payload, typename Communicator>
void run_opaque_flat_payload(Communicator const& communicator,
    std::uint32_t const num_sites, std::uint32_t const this_site)
{
    using traits = opaque_payload_traits<Payload>;

    Payload local_value = traits::make(this_site + 10);
    if (this_site == 0)
    {
        auto const gathered = gather_here(communicator, HPX_MOVE(local_value),
            this_site_arg(this_site), generation_arg(1))
                                  .get();
        HPX_TEST_EQ(gathered.size(), static_cast<std::size_t>(num_sites));
        for (std::uint32_t source = 0; source != num_sites; ++source)
        {
            traits::check_payload(gathered[source], source + 10);
        }
    }
    else
    {
        gather_there(communicator, HPX_MOVE(local_value),
            this_site_arg(this_site), generation_arg(1))
            .get();
    }

    Payload scattered = [&]() {
        if (this_site == 0)
        {
            std::vector<Payload> values;
            values.reserve(num_sites);
            for (std::uint32_t destination = 0; destination != num_sites;
                ++destination)
            {
                values.push_back(traits::make(destination + 20));
            }
            return scatter_to(communicator, HPX_MOVE(values),
                this_site_arg(this_site), generation_arg(2))
                .get();
        }

        return scatter_from<Payload>(
            communicator, this_site_arg(this_site), generation_arg(2))
            .get();
    }();
    traits::check_payload(scattered, this_site + 20);

    std::vector<Payload> values;
    values.reserve(num_sites);
    for (std::uint32_t destination = 0; destination != num_sites; ++destination)
    {
        values.push_back(traits::make(this_site * num_sites + destination));
    }
    auto const exchanged = all_to_all(communicator, HPX_MOVE(values),
        this_site_arg(this_site), generation_arg(3))
                               .get();
    HPX_TEST_EQ(exchanged.size(), static_cast<std::size_t>(num_sites));
    for (std::uint32_t source = 0; source != num_sites; ++source)
    {
        traits::check_payload(
            exchanged[source], source * num_sites + this_site);
    }
}

void run_nonassignable_hierarchical_payload(char const* const basename,
    std::uint32_t const num_sites, std::uint32_t const this_site)
{
    auto const communicators = create_hierarchical_communicator(basename,
        num_sites_arg(num_sites), this_site_arg(this_site), arity_arg(2),
        generation_arg(), root_site_arg(0), flat_fallback_threshold_arg(0));

    nonassignable_payload local_value(this_site + 30);
    if (this_site == 0)
    {
        auto const gathered = gather_here(communicators, HPX_MOVE(local_value),
            this_site_arg(this_site), generation_arg(1))
                                  .get();
        HPX_TEST_EQ(gathered.size(), static_cast<std::size_t>(num_sites));
        for (std::uint32_t source = 0; source != num_sites; ++source)
        {
            HPX_TEST_EQ(gathered[source].value, source + 30);
        }
    }
    else
    {
        gather_there(communicators, HPX_MOVE(local_value),
            this_site_arg(this_site), generation_arg(1))
            .get();
    }

    nonassignable_payload scattered = [&]() {
        if (this_site == 0)
        {
            std::vector<nonassignable_payload> values;
            values.reserve(num_sites);
            for (std::uint32_t destination = 0; destination != num_sites;
                ++destination)
            {
                values.emplace_back(destination + 40);
            }
            return scatter_to(communicators, HPX_MOVE(values),
                this_site_arg(this_site), generation_arg(2))
                .get();
        }

        return scatter_from<nonassignable_payload>(
            communicators, this_site_arg(this_site), generation_arg(2))
            .get();
    }();
    HPX_TEST_EQ(scattered.value, this_site + 40);

    std::vector<nonassignable_payload> values;
    values.reserve(num_sites);
    for (std::uint32_t destination = 0; destination != num_sites; ++destination)
    {
        values.emplace_back(this_site * num_sites + destination + 50);
    }
    auto const exchanged = all_to_all(communicators, HPX_MOVE(values),
        this_site_arg(this_site), generation_arg(3))
                               .get();
    HPX_TEST_EQ(exchanged.size(), static_cast<std::size_t>(num_sites));
    for (std::uint32_t source = 0; source != num_sites; ++source)
    {
        HPX_TEST_EQ(
            exchanged[source].value, source * num_sites + this_site + 50);
    }
}

void run_auto_generation_hierarchical_payloads(
    std::uint32_t const num_sites, std::uint32_t const this_site)
{
    auto const gather_communicators =
        create_hierarchical_communicator("/test/flattened_payload_auto_gather/",
            num_sites_arg(num_sites), this_site_arg(this_site), arity_arg(2),
            generation_arg(), root_site_arg(0), flat_fallback_threshold_arg(0));

    if (this_site == 0)
    {
        auto const gathered =
            gather_here(gather_communicators, this_site + 60).get();
        HPX_TEST_EQ(gathered.size(), static_cast<std::size_t>(num_sites));
        for (std::uint32_t source = 0; source != num_sites; ++source)
        {
            HPX_TEST_EQ(gathered[source], source + 60);
        }
    }
    else
    {
        gather_there(gather_communicators, this_site + 60).get();
    }

    // Use a distinct basename so this exercises automatic generation without
    // depending on communicator cleanup from the preceding gather.
    auto const scatter_communicators = create_hierarchical_communicator(
        "/test/flattened_payload_auto_scatter/", num_sites_arg(num_sites),
        this_site_arg(this_site), arity_arg(2), generation_arg(),
        root_site_arg(0), flat_fallback_threshold_arg(0));

    std::uint32_t const scattered = [&]() {
        if (this_site == 0)
        {
            std::vector<std::uint32_t> values;
            values.reserve(num_sites);
            for (std::uint32_t destination = 0; destination != num_sites;
                ++destination)
            {
                values.push_back(destination + 70);
            }
            return scatter_to(scatter_communicators, HPX_MOVE(values)).get();
        }

        return scatter_from<std::uint32_t>(scatter_communicators).get();
    }();
    HPX_TEST_EQ(scattered, this_site + 70);
}

template <typename F>
void run_local_sites(std::uint32_t const num_sites, F&& f)
{
    std::vector<hpx::future<void>> sites;
    sites.reserve(num_sites);
    for (std::uint32_t site = 0; site != num_sites; ++site)
    {
        sites.push_back(hpx::async([site, f]() mutable { f(site); }));
    }

    for (auto& site : sites)
    {
        site.get();
    }
}

void test_local_multisite_payload_paths()
{
    if (hpx::get_locality_id() != 0)
    {
        return;
    }

    constexpr std::uint32_t num_sites = 5;
    using ragged_payload = hpx::collectives::detail::ragged_rows<std::uint32_t>;
    using uniform_payload =
        hpx::collectives::detail::uniform_rows<std::uint32_t>;

    run_local_sites(num_sites, [](std::uint32_t const site) {
        auto const communicator =
            create_communicator("/test/flattened_payload_local_opaque_offsets/",
                num_sites_arg(num_sites), this_site_arg(site), generation_arg(),
                root_site_arg(0));
        run_opaque_flat_payload<ragged_payload>(communicator, num_sites, site);
    });
    run_local_sites(num_sites, [](std::uint32_t const site) {
        auto const communicator =
            create_communicator("/test/flattened_payload_local_opaque_rows/",
                num_sites_arg(num_sites), this_site_arg(site), generation_arg(),
                root_site_arg(0));
        run_opaque_flat_payload<uniform_payload>(communicator, num_sites, site);
    });
    run_local_sites(num_sites, [](std::uint32_t const site) {
        auto const communicators = create_hierarchical_communicator(
            "/test/flattened_payload_local_hierarchical_opaque_offsets/",
            num_sites_arg(num_sites), this_site_arg(site), arity_arg(2),
            generation_arg(), root_site_arg(0), flat_fallback_threshold_arg(0));
        run_opaque_flat_payload<ragged_payload>(communicators, num_sites, site);
    });
    run_local_sites(num_sites, [](std::uint32_t const site) {
        auto const communicators = create_hierarchical_communicator(
            "/test/flattened_payload_local_hierarchical_opaque_rows/",
            num_sites_arg(num_sites), this_site_arg(site), arity_arg(2),
            generation_arg(), root_site_arg(0), flat_fallback_threshold_arg(0));
        run_opaque_flat_payload<uniform_payload>(
            communicators, num_sites, site);
    });
    run_local_sites(num_sites, [](std::uint32_t const site) {
        run_nonassignable_hierarchical_payload(
            "/test/flattened_payload_local_nonassignable/", num_sites, site);
    });
}

void test_singleton_payload_paths()
{
    if (hpx::get_locality_id() != 0)
    {
        return;
    }

    auto const communicators =
        create_hierarchical_communicator("/test/flattened_payload_singleton/",
            num_sites_arg(1), this_site_arg(0), arity_arg(2), generation_arg(),
            root_site_arg(0), flat_fallback_threshold_arg(0));

    auto const gathered = gather_here(communicators, std::string("gather"),
        this_site_arg(0), generation_arg(1))
                              .get();
    HPX_TEST_EQ(gathered.size(), static_cast<std::size_t>(1));
    HPX_TEST_EQ(gathered[0], std::string("gather"));

    auto const scattered =
        scatter_to(communicators, std::vector<std::string>{"scatter"},
            this_site_arg(0), generation_arg(2))
            .get();
    HPX_TEST_EQ(scattered, std::string("scatter"));

    using uniform_payload =
        hpx::collectives::detail::uniform_rows<std::uint32_t>;
    using traits = opaque_payload_traits<uniform_payload>;

    auto const opaque_gathered = gather_here(
        communicators, traits::make(80), this_site_arg(0), generation_arg(3))
                                     .get();
    HPX_TEST_EQ(opaque_gathered.size(), static_cast<std::size_t>(1));
    traits::check_payload(opaque_gathered[0], 80);

    std::vector<uniform_payload> opaque_values;
    opaque_values.push_back(traits::make(90));
    auto const opaque_scattered = scatter_to(communicators,
        HPX_MOVE(opaque_values), this_site_arg(0), generation_arg(4))
                                      .get();
    traits::check_payload(opaque_scattered, 90);

    auto const exchanged =
        all_to_all(communicators, std::vector<std::string>{"all_to_all"},
            this_site_arg(0), generation_arg(5))
            .get();
    HPX_TEST_EQ(exchanged.size(), static_cast<std::size_t>(1));
    HPX_TEST_EQ(exchanged[0], std::string("all_to_all"));
}

int hpx_main()
{
    std::uint32_t const this_site = hpx::get_locality_id();
    std::uint32_t const num_sites = hpx::get_num_localities(hpx::launch::sync);
    using ragged_payload = hpx::collectives::detail::ragged_rows<std::uint32_t>;
    using uniform_payload =
        hpx::collectives::detail::uniform_rows<std::uint32_t>;

    auto const offsets_communicator = create_communicator(
        "/test/flattened_payload_opaque_offsets/", num_sites_arg(num_sites),
        this_site_arg(this_site), generation_arg(), root_site_arg(0));
    run_opaque_flat_payload<ragged_payload>(
        offsets_communicator, num_sites, this_site);

    auto const communicator = create_communicator(
        "/test/flattened_payload_opaque_rows/", num_sites_arg(num_sites),
        this_site_arg(this_site), generation_arg(), root_site_arg(0));
    run_opaque_flat_payload<uniform_payload>(
        communicator, num_sites, this_site);

    auto const hierarchical_offsets = create_hierarchical_communicator(
        "/test/flattened_payload_hierarchical_opaque_offsets/",
        num_sites_arg(num_sites), this_site_arg(this_site), arity_arg(2),
        generation_arg(), root_site_arg(0), flat_fallback_threshold_arg(0));
    run_opaque_flat_payload<ragged_payload>(
        hierarchical_offsets, num_sites, this_site);

    auto const hierarchical_communicators = create_hierarchical_communicator(
        "/test/flattened_payload_hierarchical_opaque_rows/",
        num_sites_arg(num_sites), this_site_arg(this_site), arity_arg(2),
        generation_arg(), root_site_arg(0), flat_fallback_threshold_arg(0));
    run_opaque_flat_payload<uniform_payload>(
        hierarchical_communicators, num_sites, this_site);
    run_nonassignable_hierarchical_payload(
        "/test/flattened_payload_nonassignable/", num_sites, this_site);
    run_auto_generation_hierarchical_payloads(num_sites, this_site);
    test_local_multisite_payload_paths();
    test_singleton_payload_paths();

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
