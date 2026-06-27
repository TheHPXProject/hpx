//  Copyright (c) 2007-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/command_line_handling_local.hpp>
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/runtime_configuration.hpp>

#include <hpx/command_line_handling/late_command_line_handling.hpp>
#include <hpx/command_line_handling/parse_command_line.hpp>

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::util {

    int handle_late_commandline_options(util::runtime_configuration& ini,
        hpx::program_options::options_description const& options,
        void (*handle_print_bind)(std::size_t),
        [[maybe_unused]] void (*handle_list_parcelports)())
    {
        // do secondary command line processing, check validity of options only
        try
        {
            std::string unknown_cmd_line(
                ini.get_entry("hpx.unknown_cmd_line", ""));
            if (!unknown_cmd_line.empty())
            {
                std::string runtime_mode(ini.get_entry("hpx.runtime_mode", ""));

                util::commandline_error_mode mode =
                    util::commandline_error_mode::rethrow_on_error;
                std::string allow_unknown(
                    ini.get_entry("hpx.commandline.allow_unknown", "0"));
                if (allow_unknown != "0")
                    mode |= util::commandline_error_mode::allow_unregistered;

                hpx::program_options::variables_map vm;
                std::vector<std::string> still_unregistered_options;
                util::parse_commandline(ini, options, unknown_cmd_line, vm,
                    static_cast<std::size_t>(-1), mode,
                    get_runtime_mode_from_name(runtime_mode), nullptr,
                    &still_unregistered_options);

                hpx::local::detail::set_unknown_commandline_options(
                    ini, still_unregistered_options);
            }

            if (hpx::local::detail::handle_full_help(ini, options))
            {
                return 1;
            }

            // secondary command line handling, looking for --exit and other
            // options
            std::string cmd_line(hpx::local::detail::get_full_commandline(ini));
            if (!cmd_line.empty())
            {
                std::string runtime_mode(ini.get_entry("hpx.runtime_mode", ""));
                hpx::program_options::variables_map vm;

                util::parse_commandline(ini, options, cmd_line, vm,
                    static_cast<std::size_t>(-1),
                    util::commandline_error_mode::allow_unregistered |
                        util::commandline_error_mode::
                            report_missing_config_file,
                    get_runtime_mode_from_name(runtime_mode));

#if defined(HPX_HAVE_NETWORKING)
                if (vm.count("hpx:list-parcel-ports"))
                {
                    if (handle_list_parcelports == nullptr)
                    {
                        throw hpx::detail::command_line_error(
                            "unexpected invalid function for "
                            "handle_list_parcelports");
                    }
                    handle_list_parcelports();
                }
#endif
                if (hpx::local::detail::handle_late_options(
                        ini, vm, handle_print_bind))
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
}    // namespace hpx::util

#include <hpx/config/warnings_suffix.hpp>
