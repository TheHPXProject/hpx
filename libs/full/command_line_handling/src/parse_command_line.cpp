//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/command_line_handling/parse_command_line.hpp>
#include <hpx/modules/command_line_handling_local.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/filesystem.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/ini.hpp>

#include <cctype>
#include <cstddef>
#include <exception>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::util {

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        // All command line options which are normally formatted as --hpx:foo
        // should be usable as --hpx:N:foo, where N is the node number this
        // option should be exclusively used for.
        bool handle_node_specific_option(std::string const& s, std::size_t node,
            std::pair<std::string, std::string>& opt)
        {
            // any option not starting with --hpx: will be handled elsewhere
            constexpr char const hpx_prefix[] = "--hpx:";
            constexpr std::string::size_type const hpx_prefix_len =
                sizeof(hpx_prefix) - 1;

            if (s.size() < hpx_prefix_len ||
                s.compare(0, hpx_prefix_len, hpx_prefix) != 0 ||
                !std::isdigit(s[hpx_prefix_len]))    // -V557
            {
                return false;
            }

            // any --hpx: option without a second ':' is handled elsewhere as
            // well
            std::string::size_type const p =
                s.find_first_of(':', hpx_prefix_len);
            if (p == std::string::npos)
                return false;

            if (hpx::util::from_string<std::size_t>(
                    s.substr(hpx_prefix_len, p - hpx_prefix_len),
                    static_cast<std::size_t>(-1)) == node)
            {
                using hpx::local::detail::trim_whitespace;

                // this option is for the current locality only
                std::string::size_type const p1 = s.find_first_of('=', p);
                if (p1 != std::string::npos)
                {
                    // the option has a value
                    std::string o(
                        "hpx:" + trim_whitespace(s.substr(p + 1, p1 - p - 1)));
                    std::string v(trim_whitespace(s.substr(p1 + 1)));
                    opt = std::make_pair(o, v);
                }
                else
                {
                    // no value
                    std::string o("hpx:" + trim_whitespace(s.substr(p + 1)));
                    opt = std::make_pair(o, std::string());
                }
                return true;
            }

            // This option is specifically not for us, so we return an option
            // which will be silently ignored.
            opt = std::make_pair(std::string("hpx:ignore"), std::string());
            return true;
        }

        ///////////////////////////////////////////////////////////////////////
        // Additional command line parser which interprets '@something' as an
        // option "options-file" with the value "something". Additionally we
        // resolve defined command line option aliases.
        struct option_parser : hpx::local::detail::option_parser
        {
            using base_type = hpx::local::detail::option_parser;

            option_parser(util::section const& ini, std::size_t node,
                bool ignore_aliases) noexcept
              : base_type(ini, ignore_aliases)
              , node_(node)
            {
            }

            std::pair<std::string, std::string> operator()(
                std::string const& s) const
            {
                // handle node specific options
                std::pair<std::string, std::string> opt;
                if (handle_node_specific_option(s, node_, opt))
                    return opt;

                // handle aliasing, if enabled
                return static_cast<base_type const&>(*this)(s);
            }

            std::size_t node_;
        };

        ///////////////////////////////////////////////////////////////////////
        // Handle all options from a given config file, parse and add them to
        // the given variables_map
        bool handle_config_file_options(std::vector<std::string> const& options,
            hpx::program_options::options_description const& desc,
            hpx::program_options::variables_map& vm, util::section const& rtcfg,
            std::size_t node, util::commandline_error_mode error_mode)
        {
            // add options to parsed settings
            if (!options.empty())
            {
                using hpx::program_options::command_line_parser;
                using hpx::program_options::store;
                using hpx::program_options::command_line_style::unix_style;

                util::commandline_error_mode const mode =
                    error_mode & util::commandline_error_mode::ignore_aliases;
                util::commandline_error_mode const notmode =
                    error_mode & ~util::commandline_error_mode::ignore_aliases;

                store(hpx::local::detail::get_commandline_parser(
                          command_line_parser(options)
                              .options(desc)
                              .style(unix_style)
                              .extra_parser(hpx::util::detail::option_parser(
                                  rtcfg, node, as_bool(mode))),
                          notmode)
                          .run(),
                    vm);
                notify(vm);
                return true;
            }
            return false;
        }

        // try to find a config file somewhere up the filesystem hierarchy
        // starting with the input file path. This allows to use a general
        // <app_name>.cfg file for all executables in a certain project.
        void handle_generic_config_options(std::string appname,
            hpx::program_options::variables_map& vm,
            hpx::program_options::options_description const& desc_cfgfile,
            util::section const& ini, std::size_t node,
            util::commandline_error_mode error_mode)
        {
            if (appname.empty())
                return;

            filesystem::path dir(filesystem::initial_path());
            filesystem::path const app(appname);
            appname = filesystem::basename(app.filename());

            // walk up the hierarchy, trying to find a file <appname>.cfg
            while (!dir.empty())
            {
                filesystem::path filename = dir / (appname + ".cfg");
                util::commandline_error_mode const mode = error_mode &
                    ~util::commandline_error_mode::report_missing_config_file;
                std::vector<std::string> options =
                    hpx::local::detail::read_config_file_options(
                        hpx::filesystem::to_string(filename), mode);

                if (handle_config_file_options(
                        options, desc_cfgfile, vm, ini, node, mode))
                {
                    break;    // break on the first options file found
                }

                // Boost filesystem and C++17 filesystem behave differently
                // here. Boost filesystem returns an empty path for
                // "/".parent_path() whereas C++17 filesystem will keep
                // returning "/".
#if !defined(HPX_FILESYSTEM_HAVE_BOOST_FILESYSTEM_COMPATIBILITY)
                auto dir_prev = dir;
                dir = dir.parent_path();    // chop off last directory part
                if (dir_prev == dir)
                    break;
#else
                dir = dir.parent_path();    // chop off last directory part
#endif
            }
        }

        // handle all --options-file found on the command line
        void handle_config_options(hpx::program_options::variables_map& vm,
            hpx::program_options::options_description const& desc_cfgfile,
            util::section const& ini, std::size_t node,
            util::commandline_error_mode error_mode)
        {
            using hpx::program_options::options_description;
            if (vm.count("hpx:options-file"))
            {
                auto const& cfg_files =
                    vm["hpx:options-file"].as<std::vector<std::string>>();

                for (std::string const& cfg_file : cfg_files)
                {
                    // parse a single config file and store the results
                    std::vector<std::string> options =
                        hpx::local::detail::read_config_file_options(
                            cfg_file, error_mode);

                    handle_config_file_options(
                        options, desc_cfgfile, vm, ini, node, error_mode);
                }
            }
        }

        hpx::local::detail::options_map compose_all_options(
            hpx::runtime_mode mode)
        {
            using hpx::local::detail::options_type;
            using hpx::program_options::value;

            hpx::local::detail::options_map all_options =
                hpx::local::detail::compose_local_options();

            switch (mode)
            {
            case runtime_mode::default_:
#if defined(HPX_HAVE_NETWORKING)
                // clang-format off
                all_options[options_type::hpx_options].add_options()
                    ("hpx:worker", "run this instance in worker mode")
                    ("hpx:console", "run this instance in console mode")
                    ("hpx:connect", "run this instance in worker mode, "
                         "but connecting late")
                ;
#else
                all_options[options_type::hpx_options].add_options()
                    ("hpx:console", "run this instance in console mode")
                ;
                // clang-format on
#endif
                break;

#if defined(HPX_HAVE_NETWORKING)
            case runtime_mode::worker:
            case runtime_mode::console:
                [[fallthrough]];
            case runtime_mode::connect:
                // If the runtime for this application is always run in worker
                // mode, silently ignore the worker option for hpx_pbs
                // compatibility.

                // clang-format off
                all_options[options_type::hidden_options].add_options()
                    ("hpx:worker", "run this instance in worker mode")
                    ("hpx:console", "run this instance in console mode")
                    ("hpx:connect", "run this instance in worker mode, "
                        "but connecting late")
                ;
                // clang-format on
                break;
#else
            case runtime_mode::console:
                // clang-format off
                all_options[options_type::hidden_options].add_options()
                    ("hpx:console", "run this instance in console mode")
                ;
                // clang-format on
                break;
#endif
            case runtime_mode::local:
                break;

            case runtime_mode::invalid:
                [[fallthrough]];
            default:
                throw hpx::detail::command_line_error(
                    "Invalid runtime mode specified");
            }

            // Always add the option to start the local runtime
            // clang-format off
            all_options[options_type::hpx_options].add_options()
                ("hpx:local",
                  "run this instance in local mode (experimental; certain "
                  "functionalities are not available at runtime)")
            ;
            // clang-format on

            // general options definitions
            // clang-format off
            all_options[options_type::hpx_options].add_options()
                ("hpx:run-hpx-main",
                  "run the hpx_main function, regardless of locality mode")
#if defined(HPX_HAVE_NETWORKING)
                ("hpx:agas", value<std::string>(),
                  "the IP address the AGAS root server is running on, "
                  "expected format: `address:port' (default: "
                  "127.0.0.1:7910)")
                ("hpx:hpx", value<std::string>(),
                  "the IP address the HPX parcelport is listening on, "
                  "expected format: `address:port' (default: "
                  "127.0.0.1:7910)")
                ("hpx:nodefile", value<std::string>(),
                  "the file name of a node file to use (list of nodes, one "
                  "node name per line and core)")
                ("hpx:nodes", value<std::vector<std::string> >()->multitoken(),
                  "the (space separated) list of the nodes to use (usually "
                  "this is extracted from a node file)")
                ("hpx:endnodes", "this can be used to end the list of nodes "
                  "specified using the option --hpx:nodes")
                ("hpx:ifsuffix", value<std::string>(),
                  "suffix to append to host names in order to resolve them "
                  "to the proper network interconnect")
                ("hpx:ifprefix", value<std::string>(),
                  "prefix to prepend to host names in order to resolve them "
                  "to the proper network interconnect")
                ("hpx:iftransform", value<std::string>(),
                  "sed-style search and replace (s/search/replace/) used to "
                  "transform host names to the proper network interconnect")
                ("hpx:force_ipv4", "Force ipv4 for resolving network hostnames")
#endif
#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
                ("hpx:localities", value<std::size_t>(),
                  "the number of localities to wait for at application "
                  "startup (default: 1)")
#endif
#if defined(HPX_HAVE_NETWORKING)
                ("hpx:node", value<std::size_t>(),
                  "number of the node this locality is run on "
                  "(must be unique, alternatively: -0, -1, ..., -9)")
                ("hpx:ignore-batch-env", "ignore batch environment variables "
                 "(implied by --hpx:use-process-mask)")
                ("hpx:expect-connecting-localities",
                  "this locality expects other localities to dynamically connect "
                  "(default: false if the number of localities is equal to one, "
                  "true if the number of initial localities is larger than 1)")
#endif
            ;

            all_options[options_type::debugging_options].add_options()
                ("hpx:debug-agas-log", value<std::string>()->implicit_value("cout"),
                  "enable all messages on the AGAS log channel and send all "
                  "AGAS logs to the target destination")
                ("hpx:debug-parcel-log", value<std::string>()->implicit_value("cout"),
                  "enable all messages on the parcel transport log channel and send all "
                  "parcel transport logs to the target destination")
#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
                ("hpx:list-symbolic-names", "list all registered symbolic "
                  "names after startup")
                ("hpx:list-component-types", "list all dynamic component types "
                  "after startup")
#endif
#if defined(HPX_HAVE_NETWORKING)
                ("hpx:list-parcel-ports", "list all available parcel-ports")
#endif
            ;

#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
            all_options[options_type::counter_options].add_options()
                ("hpx:print-counter", value<std::vector<std::string> >()->composing(),
                  "print the specified performance counter either repeatedly "
                  "and/or at the times specified by --hpx:print-counter-at "
                    "(see also option --hpx:print-counter-interval)")
                ("hpx:print-counter-reset",
                        value<std::vector<std::string> >()->composing(),
                  "print the specified performance counter either repeatedly "
                  "and/or at the times specified by --hpx:print-counter-at, "
                    "reset the counter after the "
                    "value is queried (see also option --hpx:print-counter-interval)")
                ("hpx:print-counter-interval", value<std::size_t>(),
                  "print the performance counter(s) specified with --hpx:print-counter "
                  "repeatedly after the time interval (specified in milliseconds) "
                  "(default: 0, which means print once at shutdown)")
                ("hpx:print-counter-destination", value<std::string>(),
                  "print the performance counter(s) specified with --hpx:print-counter "
                  "to the given file (default: console (cout), "
                  "possible values: 'cout' (console), 'none' (no output), or "
                  "any file name")
                ("hpx:list-counters", value<std::string>()->implicit_value("minimal"),
                  "list the names of all registered performance counters, "
                  "possible values:\n"
                  "   'minimal' (prints counter name skeletons)\n"
                  "   'full' (prints all available counter names)")
                ("hpx:list-counter-infos",
                    value<std::string>()->implicit_value("minimal"),
                  "list the description of all registered performance counters, "
                  "possible values:\n"
                  "   'minimal' (prints infos for counter name skeletons)\n"
                  "   'full' (prints all available counter infos)")
                ("hpx:print-counter-format", value<std::string>(),
                  "print the performance counter(s) specified with --hpx:print-counter "
                  "in a given format (default: normal)")
                ("hpx:csv-header",
                  "print the performance counter(s) specified with --hpx:print-counter "
                  "with header when format specified with --hpx:print-counter-format"
                  "is csv or csv-short")
                ("hpx:no-csv-header",
                  "print the performance counter(s) specified with --hpx:print-counter "
                  "without header when format specified with --hpx:print-counter-format"
                  "is csv or csv-short")
                ("hpx:print-counter-at",
                    value<std::vector<std::string> >()->composing(),
                  "print the performance counter(s) specified with "
                  "--hpx:print-counter (or --hpx:print-counter-reset) at the given "
                  "point in time, possible "
                  "argument values: 'startup', 'shutdown' (default), 'noshutdown'")
                ("hpx:reset-counters",
                  "reset all performance counter(s) specified with --hpx:print-counter "
                  "after they have been evaluated")
                ("hpx:print-counters-locally",
                  "each locality prints only its own local counters")
                ("hpx:print-counter-types",
                  "append counter type description to generated output")
            ;
#endif
            // clang-format on

            return all_options;
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    // parse the command line
    bool parse_commandline(util::section const& rtcfg,
        hpx::program_options::options_description const& app_options,
        std::string const& arg0, std::vector<std::string> const& args,
        hpx::program_options::variables_map& vm, std::size_t node,
        util::commandline_error_mode error_mode, hpx::runtime_mode mode,
        hpx::program_options::options_description* visible,
        std::vector<std::string>* unregistered_options)
    {
        using hpx::local::detail::options_type;

        try
        {
            // construct the overall options description and parse the command
            // line
            hpx::local::detail::options_map all_options =
                detail::compose_all_options(mode);

            hpx::local::detail::compose_all_options(app_options, all_options);

#if defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
            all_options[options_type::desc_cmdline].add(
                all_options[options_type::counter_options]);
            all_options[options_type::desc_cfgfile].add(
                all_options[options_type::counter_options]);
#endif
            bool const result = hpx::local::detail::parse_commandline(rtcfg,
                all_options, app_options, args, vm, error_mode, visible,
                unregistered_options);

            if (result && visible != nullptr)
            {
                (*visible).add(all_options[options_type::counter_options]);
            }

            detail::handle_generic_config_options(arg0, vm,
                all_options[options_type::desc_cfgfile], rtcfg, node,
                error_mode);
            detail::handle_config_options(vm,
                all_options[options_type::desc_cfgfile], rtcfg, node,
                error_mode);

            return result;
        }
        catch (std::exception const& e)
        {
            if (as_bool(error_mode &
                    util::commandline_error_mode::rethrow_on_error))
            {
                throw;
            }

            std::cerr << "hpx::init: exception caught: " << e.what()
                      << std::endl;
        }
        return false;
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        std::string extract_arg0(std::string const& cmdline)
        {
            if (std::string::size_type const p = cmdline.find_first_of(" \t");
                p != std::string::npos)
            {
                return cmdline.substr(0, p);
            }
            return cmdline;
        }
    }    // namespace detail

    bool parse_commandline(util::section const& rtcfg,
        hpx::program_options::options_description const& app_options,
        std::string const& cmdline, hpx::program_options::variables_map& vm,
        std::size_t node, util::commandline_error_mode error_mode,
        hpx::runtime_mode mode,
        hpx::program_options::options_description* visible,
        std::vector<std::string>* unregistered_options)
    {
        using namespace hpx::program_options;
#if defined(HPX_WINDOWS)
        std::vector<std::string> args = split_winmain(cmdline);
#else
        std::vector<std::string> args = split_unix(cmdline);
#endif
        return parse_commandline(rtcfg, app_options,
            detail::extract_arg0(cmdline), args, vm, node, error_mode, mode,
            visible, unregistered_options);
    }
}    // namespace hpx::util

#include <hpx/config/warnings_suffix.hpp>
