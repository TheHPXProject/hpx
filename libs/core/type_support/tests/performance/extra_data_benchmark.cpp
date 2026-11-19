//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Looking up extra data (as serialization does for each tracked pointer, each
// id_type, ...) in an object holding several types of it.

#include <hpx/config.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/type_support.hpp>

#include <cstddef>
#include <utility>

namespace {

    template <int N>
    struct item
    {
        int value = N;
    };
}    // namespace

template <int N>
struct hpx::util::extra_data_helper<item<N>>
{
    static extra_data_id_type id() noexcept
    {
        static extra_data_id_type const id =
            hpx::util::detail::next_extra_data_id();
        return id;
    }

    static void reset(item<N>*) noexcept {}
};

namespace {

    // the result of the lookups, so they aren't optimized away
    int sink = 0;

    template <int... Ns>
    void add_items(
        hpx::util::extra_data& data, std::integer_sequence<int, Ns...>)
    {
        (data.get<item<Ns>>(), ...);
    }

    template <int N>
    void lookups(hpx::util::extra_data const& data, std::size_t const count)
    {
        int sum = 0;
        for (std::size_t i = 0; i != count; ++i)
        {
            if (auto const* p = data.try_get<item<N>>())
            {
                sum += p->value;
            }
        }
        sink += sum;
    }
}    // namespace

int hpx_main(hpx::program_options::variables_map& vm)
{
    std::size_t const repeat = vm["test_count"].as<std::size_t>();
    std::size_t const count = vm["lookups"].as<std::size_t>();

    hpx::util::perftests_init(vm, "extra_data_benchmark");

    // eight types of extra data, added in the order of their numbers
    hpx::util::extra_data data;
    add_items(data, std::make_integer_sequence<int, 8>{});

    hpx::util::perftests_report("hpx::util::extra_data::try_get",
        "first added of 8", repeat, [&] { lookups<0>(data, count); });
    hpx::util::perftests_report("hpx::util::extra_data::try_get",
        "last added of 8", repeat, [&] { lookups<7>(data, count); });
    hpx::util::perftests_report("hpx::util::extra_data::try_get", "missing",
        repeat, [&] { lookups<8>(data, count); });

    // a new object with one type of extra data, as an archive often has
    hpx::util::perftests_report(
        "hpx::util::extra_data::get", "new object", repeat, [&] {
            for (std::size_t i = 0; i != count / 100; ++i)
            {
                hpx::util::extra_data fresh;
                sink += fresh.get<item<0>>().value;
            }
        });

    hpx::util::perftests_print_times();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    using namespace hpx::program_options;
    options_description cmdline("usage: " HPX_APPLICATION_STRING " [options]");

    // clang-format off
    cmdline.add_options()
        ("test_count", value<std::size_t>()->default_value(100),
            "number of tests to be averaged")
        ("lookups", value<std::size_t>()->default_value(1000000),
            "number of lookups per test")
        ;
    // clang-format on

    hpx::util::perftests_cfg(cmdline);

    hpx::local::init_params init_args;
    init_args.desc_cmdline = cmdline;
    init_args.cfg = {"hpx.os_threads=1"};

    return hpx::local::init(hpx_main, argc, argv, init_args);
}
