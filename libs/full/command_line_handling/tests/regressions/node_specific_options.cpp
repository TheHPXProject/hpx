//  Copyright (c) 2026 Fabian C.
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/command_line_handling/parse_command_line.hpp>
#include <hpx/modules/ini.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace {

    hpx::program_options::variables_map parse(hpx::util::section const& rtcfg,
        std::size_t node, std::vector<std::string> const& args)
    {
        hpx::program_options::options_description app_options;
        hpx::program_options::variables_map vm;

        bool const result = hpx::util::parse_commandline(rtcfg, app_options,
            "node_specific_options", args, vm, node,
            hpx::util::commandline_error_mode::rethrow_on_error);

        HPX_TEST(result);
        return vm;
    }

    void expect_missing_parameter(hpx::util::section const& rtcfg,
        std::size_t node, std::vector<std::string> const& args)
    {
        bool caught = false;

        try
        {
            (void) parse(rtcfg, node, args);
        }
        catch (hpx::program_options::invalid_command_line_syntax const& e)
        {
            caught = true;
            HPX_TEST(e.kind() ==
                hpx::program_options::invalid_command_line_syntax::
                    missing_parameter);
        }

        HPX_TEST(caught);
    }

    void test_matching_option(hpx::util::section const& rtcfg)
    {
        auto const vm = parse(rtcfg, 0, {"--hpx:0:pu-offset=16"});

        HPX_TEST_EQ(vm.count("hpx:pu-offset"), std::size_t(1));
        HPX_TEST_EQ(vm["hpx:pu-offset"].as<std::size_t>(), std::size_t(16));
    }

    void test_matching_space_separated_option(hpx::util::section const& rtcfg)
    {
        auto const vm = parse(rtcfg, 0, {"--hpx:0:threads", "2"});

        HPX_TEST_EQ(vm.count("hpx:threads"), std::size_t(1));
        HPX_TEST_EQ(vm["hpx:threads"].as<std::string>(), std::string("2"));
    }

    void test_matching_guessed_option(hpx::util::section const& rtcfg)
    {
        auto const vm = parse(rtcfg, 1, {"--hpx:1:thread", "2"});

        HPX_TEST_EQ(vm.count("hpx:threads"), std::size_t(1));
        HPX_TEST_EQ(vm["hpx:threads"].as<std::string>(), std::string("2"));
    }

    void test_matching_flag(hpx::util::section const& rtcfg)
    {
        auto const vm = parse(rtcfg, 0, {"--hpx:0:print-bind"});

        HPX_TEST_EQ(vm.count("hpx:print-bind"), std::size_t(1));
    }

    void test_node_specific_bind(hpx::util::section const& rtcfg)
    {
        auto const vm =
            parse(rtcfg, 0, {"--hpx:0:bind=compact", "--hpx:1:bind=scatter"});

        HPX_TEST_EQ(vm.count("hpx:bind"), std::size_t(1));

        auto const& bindings = vm["hpx:bind"].as<std::vector<std::string>>();

        HPX_TEST_EQ(bindings.size(), std::size_t(1));
        HPX_TEST_EQ(bindings[0], std::string("compact"));
    }

    void test_matching_space_separated_bind(hpx::util::section const& rtcfg)
    {
        auto const vm = parse(rtcfg, 0, {"--hpx:0:bind", "compact"});

        HPX_TEST_EQ(vm.count("hpx:bind"), std::size_t(1));

        auto const& bindings = vm["hpx:bind"].as<std::vector<std::string>>();

        HPX_TEST_EQ(bindings.size(), std::size_t(1));
        HPX_TEST_EQ(bindings[0], std::string("compact"));
    }

    void test_non_matching_options(hpx::util::section const& rtcfg)
    {
        auto const vm =
            parse(rtcfg, 0, {"--hpx:1:pu-offset=16", "--hpx:2:pu-step=2"});

        HPX_TEST_EQ(vm.count("hpx:pu-offset"), std::size_t(0));
        HPX_TEST_EQ(vm.count("hpx:pu-step"), std::size_t(0));
    }

    void test_non_matching_space_separated_values(
        hpx::util::section const& rtcfg)
    {
        auto const vm = parse(
            rtcfg, 0, {"--hpx:1:threads", "2", "--hpx:1:bind", "compact"});

        HPX_TEST_EQ(vm.count("hpx:threads"), std::size_t(0));
        HPX_TEST_EQ(vm.count("hpx:bind"), std::size_t(0));
        HPX_TEST_EQ(vm.count("hpx:positional"), std::size_t(0));
    }

    void test_non_matching_guessed_option(hpx::util::section const& rtcfg)
    {
        auto const vm = parse(rtcfg, 0, {"--hpx:1:thread", "2"});

        HPX_TEST_EQ(vm.count("hpx:threads"), std::size_t(0));
        HPX_TEST_EQ(vm.count("hpx:positional"), std::size_t(0));
    }

    void test_non_matching_flag_preserves_positional(
        hpx::util::section const& rtcfg)
    {
        auto const vm =
            parse(rtcfg, 0, {"--hpx:1:print-bind", "application-argument"});

        HPX_TEST_EQ(vm.count("hpx:print-bind"), std::size_t(0));
        HPX_TEST_EQ(vm.count("hpx:positional"), std::size_t(1));

        auto const& positional =
            vm["hpx:positional"].as<std::vector<std::string>>();

        HPX_TEST_EQ(positional.size(), std::size_t(1));
        HPX_TEST_EQ(positional[0], std::string("application-argument"));
    }

    void test_non_matching_optional_value(hpx::util::section const& rtcfg)
    {
        auto const vm = parse(
            rtcfg, 0, {"--hpx:1:numa-sensitive", "1", "--hpx:0:threads=2"});

        HPX_TEST_EQ(vm.count("hpx:numa-sensitive"), std::size_t(0));
        HPX_TEST_EQ(vm.count("hpx:threads"), std::size_t(1));
        HPX_TEST_EQ(vm["hpx:threads"].as<std::string>(), std::string("2"));
        HPX_TEST_EQ(vm.count("hpx:positional"), std::size_t(0));

        auto const vm_no_value =
            parse(rtcfg, 0, {"--hpx:1:numa-sensitive", "--hpx:0:threads=3"});

        HPX_TEST_EQ(vm_no_value.count("hpx:numa-sensitive"), std::size_t(0));
        HPX_TEST_EQ(vm_no_value.count("hpx:threads"), std::size_t(1));
        HPX_TEST_EQ(
            vm_no_value["hpx:threads"].as<std::string>(), std::string("3"));
        HPX_TEST_EQ(vm_no_value.count("hpx:positional"), std::size_t(0));
    }

#if defined(HPX_HAVE_NETWORKING)
    void test_non_matching_multitoken_value(hpx::util::section const& rtcfg)
    {
        auto const vm = parse(rtcfg, 0,
            {"--hpx:1:nodes", "node-a", "node-b", "--hpx:0:threads=2"});

        HPX_TEST_EQ(vm.count("hpx:nodes"), std::size_t(0));
        HPX_TEST_EQ(vm.count("hpx:threads"), std::size_t(1));
        HPX_TEST_EQ(vm["hpx:threads"].as<std::string>(), std::string("2"));
        HPX_TEST_EQ(vm.count("hpx:positional"), std::size_t(0));
    }
#endif

    void test_matching_and_non_matching_options(hpx::util::section const& rtcfg)
    {
        auto const vm = parse(rtcfg, 1,
            {"--hpx:0:pu-offset=8", "--hpx:1:pu-offset=16",
                "--hpx:2:pu-offset=24"});

        HPX_TEST_EQ(vm.count("hpx:pu-offset"), std::size_t(1));
        HPX_TEST_EQ(vm["hpx:pu-offset"].as<std::size_t>(), std::size_t(16));
    }

    void test_unknown_node_ignores_node_specific_options(
        hpx::util::section const& rtcfg)
    {
        auto const vm = parse(rtcfg, static_cast<std::size_t>(-1),
            {"--hpx:0:threads", "2", "--hpx:1:bind", "compact",
                "--hpx:2:print-bind"});

        HPX_TEST_EQ(vm.count("hpx:threads"), std::size_t(0));
        HPX_TEST_EQ(vm.count("hpx:bind"), std::size_t(0));
        HPX_TEST_EQ(vm.count("hpx:print-bind"), std::size_t(0));
        HPX_TEST_EQ(vm.count("hpx:positional"), std::size_t(0));
    }

    void test_missing_value_does_not_consume_following_option(
        hpx::util::section const& rtcfg)
    {
        std::vector<std::string> const args{
            "--hpx:1:threads", "--hpx:0:bind=compact"};

        // The required-value option is ignored on this locality. The
        // following matching option must not be consumed as its value.
        expect_missing_parameter(rtcfg, 0, args);

        // The required-value option is for this locality. The following
        // non-matching option must likewise not be consumed as its value.
        expect_missing_parameter(rtcfg, 1, args);
    }

}    // namespace

int main()
{
    hpx::util::section rtcfg;

    test_matching_option(rtcfg);
    test_matching_space_separated_option(rtcfg);
    test_matching_guessed_option(rtcfg);
    test_matching_flag(rtcfg);
    test_node_specific_bind(rtcfg);
    test_matching_space_separated_bind(rtcfg);
    test_non_matching_options(rtcfg);
    test_non_matching_space_separated_values(rtcfg);
    test_non_matching_guessed_option(rtcfg);
    test_non_matching_flag_preserves_positional(rtcfg);
    test_non_matching_optional_value(rtcfg);
#if defined(HPX_HAVE_NETWORKING)
    test_non_matching_multitoken_value(rtcfg);
#endif
    test_matching_and_non_matching_options(rtcfg);
    test_unknown_node_ignores_node_specific_options(rtcfg);
    test_missing_value_does_not_consume_following_option(rtcfg);

    return hpx::util::report_errors();
}
