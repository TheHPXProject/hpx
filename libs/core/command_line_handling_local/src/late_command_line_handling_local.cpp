//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/assert.hpp>
#include <hpx/command_line_handling_local/late_command_line_handling_local.hpp>
#include <hpx/command_line_handling_local/parse_command_line_local.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/version.hpp>

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

namespace hpx::local::detail {

    std::string enquote(std::string arg)
    {
        if (arg.find_first_of(" \t\"") != std::string::npos)
        {
#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 110000
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wrestrict"
#endif
            return std::string("\"") + HPX_MOVE(arg) + "\"";
#if defined(HPX_GCC_VERSION) && HPX_GCC_VERSION >= 110000
#pragma GCC diagnostic pop
#endif
        }
        return arg;
    }

    void decode(std::string& str, char const* s, char const* r)
    {
        std::string::size_type pos = 0;
        while ((pos = str.find(s, pos)) != std::string::npos)
        {
            str.replace(pos, 2, r);
        }
    }

    std::string decode_string(std::string str)
    {
        decode(str, "\\n", "\n");
        return str;
    }

    void set_unknown_commandline_options(util::runtime_configuration& ini,
        std::vector<std::string> const& still_unregistered_options)
    {
        std::string still_unknown_commandline;
        for (std::size_t i = 1; i < still_unregistered_options.size(); ++i)
        {
            if (i != 1)
            {
                still_unknown_commandline += " ";
            }
            still_unknown_commandline += enquote(still_unregistered_options[i]);
        }

        if (!still_unknown_commandline.empty())
        {
            util::section* s = ini.get_section("hpx");
            HPX_ASSERT(s != nullptr);
            s->add_entry("unknown_cmd_line_option", still_unknown_commandline);
        }
    }

    bool handle_full_help(util::runtime_configuration const& ini,
        hpx::program_options::options_description const& options)
    {
        if (std::string const fullhelp(ini.get_entry("hpx.cmd_line_help", ""));
            !fullhelp.empty())
        {
            if (std::string const help_option(
                    ini.get_entry("hpx.cmd_line_help_option", ""));
                0 == std::string("full").find(help_option))
            {
                std::cout << decode_string(fullhelp);
                std::cout << options << std::endl;
            }
            else
            {
                throw hpx::detail::command_line_error(
                    "unknown help option: " + help_option);
            }
            return true;
        }
        return false;
    }

    std::string get_full_commandline(util::runtime_configuration const& ini)
    {
        // the parts separated by spaces: an option of HPX_COMMANDLINE_OPTIONS
        // (prepend_options) must not run into the first one of the command
        // line
        return ini.get_entry("hpx.commandline.command", "") + " " +
            ini.get_entry("hpx.commandline.prepend_options", "") + " " +
            ini.get_entry("hpx.commandline.options", "") + " " +
            ini.get_entry("hpx.commandline.config_options", "");
    }

    bool handle_late_options(util::runtime_configuration const& ini,
        hpx::program_options::variables_map const& vm,
        void (*handle_print_bind)(std::size_t))
    {
        if (handle_print_bind != nullptr && vm.count("hpx:print-bind"))
        {
            std::size_t const num_threads = hpx::util::from_string<std::size_t>(
                ini.get_entry("hpx.os_threads", 1));
            handle_print_bind(num_threads);
        }

        // print version and copyright information, here rather than with
        // the other options of the first command line pass so that the
        // libraries of the plugins (device conduits, parcelports) are listed
        // too: they register their versions when they are loaded
        if (vm.count("hpx:version"))
        {
            std::cout << std::endl << hpx::copyright() << std::endl;
            std::cout << hpx::complete_version() << std::endl;
            return true;
        }

        if (vm.count("hpx:exit"))
        {
            return true;
        }

        return false;
    }

    int handle_late_commandline_options(util::runtime_configuration& ini,
        hpx::program_options::options_description const& options,
        void (*handle_print_bind)(std::size_t))
    {
        // do secondary command line processing, check validity of options only
        try
        {
            if (std::string const unknown_cmd_line(
                    ini.get_entry("hpx.unknown_cmd_line", ""));
                !unknown_cmd_line.empty())
            {
                util::commandline_error_mode mode =
                    util::commandline_error_mode::rethrow_on_error;
                if (std::string const allow_unknown(
                        ini.get_entry("hpx.commandline.allow_unknown", "0"));
                    allow_unknown != "0")
                {
                    mode |= util::commandline_error_mode::allow_unregistered;
                }

                hpx::program_options::variables_map vm;
                std::vector<std::string> still_unregistered_options;
                parse_commandline(ini, options, unknown_cmd_line, vm, mode,
                    nullptr, &still_unregistered_options);

                set_unknown_commandline_options(
                    ini, still_unregistered_options);
            }

            if (handle_full_help(ini, options))
            {
                return 1;
            }

            // secondary command line handling, looking for --exit and other
            // options
            if (std::string const cmd_line(get_full_commandline(ini));
                !cmd_line.empty())
            {
                hpx::program_options::variables_map vm;

                parse_commandline(ini, options, cmd_line, vm,
                    util::commandline_error_mode::allow_unregistered |
                        util::commandline_error_mode::
                            report_missing_config_file);

                if (handle_late_options(ini, vm, handle_print_bind))
                {
                    return 1;
                }
            }
        }
        catch (std::exception const& e)
        {
            std::cerr
                << "handle_late_commandline_options: command line processing: "
                << e.what() << std::endl;
            return -1;
        }

        return 0;
    }
}    // namespace hpx::local::detail
