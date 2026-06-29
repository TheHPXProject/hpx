//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/command_line_handling_local/config/defines.hpp>
#include <hpx/command_line_handling_local/parse_command_line_local.hpp>
#if defined(HPX_COMMAND_LINE_HANDLING_HAVE_JSON_CONFIGURATION_FILES)
#include <hpx/command_line_handling_local/json_config_file.hpp>
#endif
#include <hpx/modules/datastructures.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/filesystem.hpp>
#include <hpx/modules/ini.hpp>

#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#if defined(HPX_HAVE_UNISTD_H)
#include <unistd.h>
#endif

///////////////////////////////////////////////////////////////////////////////
namespace hpx::local::detail {

    ///////////////////////////////////////////////////////////////////////
    std::string trim_whitespace(std::string const& s)
    {
        using size_type = std::string::size_type;

        size_type const first = s.find_first_not_of(" \t");
        if (std::string::npos == first)
            return {};

        size_type const last = s.find_last_not_of(" \t");
        return s.substr(first, last - first + 1);
    }

    ///////////////////////////////////////////////////////////////////////
    // Handle aliasing of command line options based on information stored
    // in the ini-configuration
    std::pair<std::string, std::string> handle_aliasing(
        util::section const& ini, std::string const& option)
    {
        std::pair<std::string, std::string> result;

        std::string opt(trim_whitespace(option));
        if (opt.size() < 2 || opt[0] != '-')
            return result;

        util::section const* sec = ini.get_section("hpx.commandline.aliases");
        if (nullptr == sec)
            return result;    // no alias mappings are defined

        // we found shortcut option definitions, try to find mapping
        std::string expand_to;
        std::string::size_type start_at = 2;
        bool long_option = false;
        if (opt.size() > 2 && opt[1] != '-')
        {
            // short option with value: first two letters have to match
            expand_to =
                trim_whitespace(sec->get_entry(opt.substr(0, start_at), ""));
        }
        else
        {
            // short option (no value) or long option
            if (opt[1] == '-')
            {
                start_at = opt.find_last_of('=');
                long_option = true;
            }

            if (start_at != std::string::npos)
            {
                expand_to = trim_whitespace(
                    sec->get_entry(opt.substr(0, start_at), ""));
            }
            else
            {
                expand_to = trim_whitespace(sec->get_entry(opt, ""));
            }
        }

        if (expand_to.size() < 2 || expand_to.substr(0, 2) != "--")
            return result;    // no sensible alias is defined for this option
        expand_to.erase(0, 2);

        std::string::size_type p = expand_to.find_first_of('=');
        if (p != std::string::npos)
        {
            // the option alias defines its own value
            std::string o(trim_whitespace(expand_to.substr(0, p)));
            std::string v(trim_whitespace(expand_to.substr(p + 1)));
            result = std::make_pair(o, v);
        }
        else if (start_at != std::string::npos && start_at < opt.size())
        {
            // extract value from original option
            result = std::make_pair(
                expand_to, opt.substr(start_at + (long_option ? 1 : 0)));
        }
        else
        {
            // no value
            result = std::make_pair(expand_to, std::string());
        }

        return result;
    }

    ///////////////////////////////////////////////////////////////////////
    // Additional command line parser which interprets '@something' as an
    // option "options-file" with the value "something". Additionally we
    // resolve defined command line option aliases.
    option_parser::option_parser(
        util::section const& ini, bool ignore_aliases) noexcept
      : ini_(ini)
      , ignore_aliases_(ignore_aliases)
    {
    }

    std::pair<std::string, std::string> option_parser::operator()(
        std::string const& s) const
    {
        // handle special syntax for configuration files @filename
        if ('@' == s[0])
        {
            return std::make_pair(std::string("hpx:options-file"), s.substr(1));
        }

        // handle aliasing, if enabled
        if (ini_.get_entry("hpx.commandline.aliasing", "0") == "0" ||
            ignore_aliases_)
        {
            return std::make_pair(std::string(), std::string());
        }

        return handle_aliasing(ini_, s);
    }

    ///////////////////////////////////////////////////////////////////////
    hpx::program_options::basic_command_line_parser<char>&
    get_commandline_parser(
        hpx::program_options::basic_command_line_parser<char>& p,
        util::commandline_error_mode mode)
    {
        if (as_bool(mode & util::commandline_error_mode::allow_unregistered))
        {
            return p.allow_unregistered();
        }
        return p;
    }

    ///////////////////////////////////////////////////////////////////////
    // Read all options from a given config file
    std::vector<std::string> read_config_file_options(
        std::string const& filename, util::commandline_error_mode error_mode)
    {
#if defined(HPX_COMMAND_LINE_HANDLING_HAVE_JSON_CONFIGURATION_FILES)
        filesystem::path const cfgfile(filename);
        if (cfgfile.extension() == ".json")
        {
            return read_json_config_file_options(filename, error_mode);
        }
#endif

        std::vector<std::string> options;
        std::ifstream ifs(filename.c_str());
        if (!ifs.is_open())
        {
            if (as_bool(error_mode &
                    util::commandline_error_mode::report_missing_config_file))
            {
                std::cerr << "hpx::init: command line warning: command line "
                             "options file not found ("
                          << filename << ")" << std::endl;
            }
            return options;
        }

        std::string line;
        while (std::getline(ifs, line))
        {
            using hpx::local::detail::trim_whitespace;

            // skip empty lines
            std::string::size_type const pos = line.find_first_not_of(" \t");
            if (pos == std::string::npos)
                continue;

            // strip leading and trailing whitespace
            line = trim_whitespace(line);

            // skip comment lines
            if ('#' != line[0])
            {
                std::string::size_type const p1 = line.find_first_of(" \t");
                if (p1 != std::string::npos)
                {
                    // rebuild the line connecting the parts with a '='
                    line = trim_whitespace(line.substr(0, p1)) + '=' +
                        trim_whitespace(line.substr(p1));
                }
                options.push_back(line);
            }
        }

        return options;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Handle all options from a given config file, parse and add them to the
    // given variables_map
    bool handle_config_file_options(std::vector<std::string> const& options,
        hpx::program_options::options_description const& desc,
        hpx::program_options::variables_map& vm, util::section const& rtcfg,
        util::commandline_error_mode error_mode)
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

            store(get_commandline_parser(
                      command_line_parser(options)
                          .options(desc)
                          .style(unix_style)
                          .extra_parser(option_parser(rtcfg, as_bool(mode))),
                      notmode)
                      .run(),
                vm);
            notify(vm);
            return true;
        }
        return false;
    }

    // try to find a config file somewhere up the filesystem hierarchy starting
    // with the input file path. This allows to use a general <app_name>.cfg
    // file for all executables in a certain project.
    void handle_generic_config_options(std::string appname,
        hpx::program_options::variables_map& vm,
        hpx::program_options::options_description const& desc_cfgfile,
        util::section const& ini, util::commandline_error_mode error_mode)
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

            std::vector<std::string> options = read_config_file_options(
                hpx::filesystem::to_string(filename), mode);

            if (handle_config_file_options(
                    options, desc_cfgfile, vm, ini, mode))
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
        util::section const& ini, util::commandline_error_mode error_mode)
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
                    read_config_file_options(cfg_file, error_mode);

                handle_config_file_options(
                    options, desc_cfgfile, vm, ini, error_mode);
            }
        }
    }

    void verify_unknown_options(std::vector<std::string> const& opts)
    {
        for (auto const& opt : opts)
        {
            std::string::size_type const p = opt.find("--hpx:");
            if (p != std::string::npos)
            {
                throw hpx::detail::command_line_error(
                    "Unknown/misspelled HPX command line option found: " + opt);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // parse the command line
    bool parse_commandline(util::section const& rtcfg, options_map& all_options,
        hpx::program_options::options_description const& app_options,
        std::vector<std::string> const& args,
        hpx::program_options::variables_map& vm,
        util::commandline_error_mode error_mode,
        hpx::program_options::options_description* visible,
        std::vector<std::string>* unregistered_options)
    {
        using hpx::program_options::command_line_parser;
        using hpx::program_options::options_description;
        using hpx::program_options::parsed_options;
        using hpx::program_options::positional_options_description;
        using hpx::program_options::store;
        using hpx::program_options::value;
        using namespace hpx::program_options::command_line_style;

        if (rtcfg.get_entry("hpx.commandline.allow_unknown", "0") == "0")
        {
            // clang-format off
            options_description positional_options;
            positional_options.add_options()
                ("hpx:positional",
                  value<std::vector<std::string>>(), "positional options")
                ;
            // clang-format on

            all_options[options_type::desc_cmdline].add(positional_options);
            all_options[options_type::desc_cfgfile].add(positional_options);

            // move all positional options into the hpx:positional option
            // group
            positional_options_description pd;
            pd.add("hpx:positional", -1);

            // parse command line, allow for unregistered options this point
            util::commandline_error_mode const mode =
                error_mode & util::commandline_error_mode::ignore_aliases;
            util::commandline_error_mode const notmode =
                error_mode & ~util::commandline_error_mode::ignore_aliases;

            parsed_options const opts(get_commandline_parser(
                command_line_parser(args)
                    .options(all_options[options_type::desc_cmdline])
                    .positional(pd)
                    .style(unix_style)
                    .extra_parser(option_parser(rtcfg, as_bool(mode))),
                notmode)
                    .run());

            // collect unregistered options, if needed
            if (unregistered_options)
            {
                using hpx::program_options::collect_unrecognized;
                using hpx::program_options::collect_unrecognized_mode::
                    exclude_positional;
                *unregistered_options =
                    collect_unrecognized(opts.options, exclude_positional);

                verify_unknown_options(*unregistered_options);
            }

            store(opts, vm);
        }
        else
        {
            // parse command line, allow for unregistered options this point
            util::commandline_error_mode const mode =
                error_mode & util::commandline_error_mode::ignore_aliases;
            util::commandline_error_mode const notmode =
                error_mode & ~util::commandline_error_mode::ignore_aliases;

            parsed_options const opts(get_commandline_parser(
                command_line_parser(args)
                    .options(all_options[options_type::desc_cmdline])
                    .style(unix_style)
                    .extra_parser(option_parser(rtcfg, as_bool(mode))),
                notmode)
                    .run());

            // collect unregistered options, if needed
            if (unregistered_options)
            {
                using hpx::program_options::collect_unrecognized;
                using hpx::program_options::collect_unrecognized_mode::
                    include_positional;
                *unregistered_options =
                    collect_unrecognized(opts.options, include_positional);

                verify_unknown_options(*unregistered_options);
            }

            store(opts, vm);
        }

        if (vm.count("hpx:help"))
        {
            // collect help information
            if (visible != nullptr)
            {
                (*visible)
                    .add(app_options)
                    .add(all_options[options_type::commandline_options])
                    .add(all_options[options_type::hpx_options])
                    .add(all_options[options_type::debugging_options])
                    .add(all_options[options_type::config_options]);
            }
            return true;
        }

        notify(vm);

        return true;
    }

    // Special type to be able to enforce an argument value if a parameter
    // that should normally be specified only once has been used with the
    // --hpx:arg=!value syntax to override any other possibly provided
    // argument value.
    struct argument_string : std::string
    {
        using std::string::string;
    };

    void validate(hpx::any_nonser& v, std::vector<std::string> const& xs,
        argument_string* t, int)
    {
        // check whether we should override any existing values
        if (v.has_value())
        {
            // if the previous value has a '!' prepended, then discard the
            // current argument
            std::string const& arg = any_cast<std::string>(v);
            if (!arg.empty() && arg[0] == '!')
            {
                return;
            }

            // if the current argument has a '!' prepended, then we discard the
            // previous value
            if (!xs[0].empty() && xs[0][0] == '!')
            {
                // discard any existing value
                v = hpx::any_nonser();
            }
        }

        // do normal validation
        program_options::validate(v, xs, static_cast<std::string*>(t), 0);
    }

    options_map compose_local_options()
    {
        using hpx::program_options::value;

        options_map all_options;

        // clang-format off
        all_options.emplace(options_type::commandline_options,
            "HPX options (allowed on command line only)");

        all_options[options_type::commandline_options].add_options()
            ("hpx:help", value<std::string>()->implicit_value("minimal"),
                "print out program usage (default: this message), possible "
                "values: 'full' (additionally prints options from components)")
            ("hpx:version", "print out HPX version and copyright information")
            ("hpx:info", "print out HPX configuration information")
            ("hpx:options-file", value<std::vector<std::string> >()->composing(),
                "specify a file containing command line options "
                "(alternatively: @filepath)")
        ;
        // clang-format on

        all_options.emplace(options_type::hpx_options,
            "HPX options (additionally allowed in an options file)");
        all_options.emplace(options_type::hidden_options, "Hidden options");

        // general options definitions
        // clang-format off
        all_options[options_type::hpx_options].add_options()
            ("hpx:pu-offset", value<std::size_t>(),
                "the first processing unit this instance of HPX should be "
                "run on (default: 0), valid for "
                "--hpx:queuing=local, --hpx:queuing=abp-priority, "
                "--hpx:queuing=static, --hpx:queuing=static-priority, "
                "--hpx:queuing=local-workrequesting-fifo, "
                "--hpx:queuing=local-workrequesting-lifo, "
                "--hpx:queuing=local-workrequesting-mc, "
                "and --hpx:queuing=local-priority only")
            ("hpx:pu-step", value<std::size_t>(),
                "the step between used processing unit numbers for this "
                "instance of HPX (default: 1), valid for "
                "--hpx:queuing=local, --hpx:queuing=abp-priority, "
                "--hpx:queuing=static, --hpx:queuing=static-priority "
                "--hpx:queuing=local-workrequesting-fifo, "
                "--hpx:queuing=local-workrequesting-lifo, "
                "--hpx:queuing=local-workrequesting-mc, "
                "and --hpx:queuing=local-priority only")
            ("hpx:affinity", value<std::string>(),
                "the affinity domain the OS threads will be confined to, "
                "possible values: pu, core, numa, machine (default: pu), valid for "
                "--hpx:queuing=local, --hpx:queuing=abp-priority, "
                "--hpx:queuing=static, --hpx:queuing=static-priority "
                "--hpx:queuing=local-workrequesting-fifo, "
                "--hpx:queuing=local-workrequesting-lifo, "
                "--hpx:queuing=local-workrequesting-mc, "
                " and --hpx:queuing=local-priority only")
            ("hpx:bind", value<std::vector<std::string> >()->composing(),
                "the detailed affinity description for the OS threads, see "
                "the documentation for a detailed description of possible "
                "values. Do not use with --hpx:pu-step, --hpx:pu-offset, or "
                "--hpx:affinity options. Implies --hpx:numa-sensitive=1"
                "(--hpx:bind=none disables defining thread affinities).")
            ("hpx:use-process-mask", "use the process mask to restrict "
                "available hardware resources (implies "
                "--hpx:ignore-batch-env)")
            ("hpx:print-bind",
                "print to the console the bit masks calculated from the "
                "arguments specified to all --hpx:bind options.")
            ("hpx:threads", value<std::string>(),
                "the number of operating system threads to spawn for this HPX "
                "locality (default: 1, using 'all' will spawn one thread for "
                "each processing unit")
            ("hpx:cores", value<std::string>(),
                "the number of cores to utilize for this HPX "
                "locality (default: 'all', i.e. the number of cores is based "
                "on the number of total cores in the system)")
            ("hpx:queuing", value<argument_string>(),
                "the queue scheduling policy to use, options are "
                "'local', 'local-priority-fifo','local-priority-lifo', "
                "'abp-priority-fifo', 'abp-priority-lifo', 'static', "
                "'static-priority', 'local-workrequesting-fifo',"
                "'local-workrequesting-lifo', and 'local-workrequesting-mc' "
                "(default: 'local-priority'; all option values can be "
                "abbreviated)")
            ("hpx:high-priority-threads", value<std::size_t>(),
                "the number of operating system threads maintaining a high "
                "priority queue (default: number of OS threads), valid for "
                "--hpx:queuing=local-priority,--hpx:queuing=static-priority, "
                "--hpx:queuing=local-workrequesting-fifo, "
                "--hpx:queuing=local-workrequesting-lifo, "
                "--hpx:queuing=local-workrequesting-mc, "
                " and --hpx:queuing=abp-priority only)")
            ("hpx:numa-sensitive", value<std::size_t>()->implicit_value(0),
                "makes the local-priority scheduler NUMA sensitive ("
                "allowed values: 0 - no NUMA sensitivity, 1 - allow only for "
                "boundary cores to steal across NUMA domains, 2 - "
                "no cross boundary stealing is allowed (default value: 0)")
        ;

        all_options.emplace(options_type::config_options,
            "HPX configuration options");
        all_options[options_type::config_options].add_options()
            ("hpx:app-config", value<std::string>(),
                "load the specified application configuration (ini) file")
            ("hpx:config", value<std::string>()->default_value(""),
                "load the specified hpx configuration (ini) file")
            ("hpx:ini", value<std::vector<std::string> >()->composing(),
                "add a configuration definition to the default runtime "
                "configuration")
            ("hpx:exit", "exit after configuring the runtime")
        ;

        all_options.emplace(options_type::debugging_options,
            "HPX debugging options");
        all_options[options_type::debugging_options].add_options()
            ("hpx:dump-config-initial", "print the initial runtime configuration")
            ("hpx:dump-config", "print the final runtime configuration")
            // enable debug output from command line handling
            ("hpx:debug-clp", "debug command line processing")
#if defined(_POSIX_VERSION) || defined(HPX_WINDOWS)
            ("hpx:attach-debugger",
                value<std::string>()->implicit_value("startup"),
                "wait for a debugger to be attached, possible values: "
                "off, startup, exception or test-failure (default: startup)")
#endif
            ("hpx:debug-hpx-log", value<std::string>()->implicit_value("cout"),
                "enable all messages on the HPX log channel and send all "
                "HPX logs to the target destination")
            ("hpx:debug-timing-log", value<std::string>()->implicit_value("cout"),
                "enable all messages on the timing log channel and send all "
                "timing logs to the target destination")
            ("hpx:debug-app-log", value<std::string>()->implicit_value("cout"),
                "enable all messages on the application log channel and send all "
                "application logs to the target destination")
            // ("hpx:verbose_bench", "For logging benchmarks in detail")
        ;

        all_options[options_type::hidden_options].add_options()
            ("hpx:ignore", "this option will be silently ignored")
        ;
        // clang-format on

        return all_options;
    }

    void compose_all_options(
        hpx::program_options::options_description const& app_options,
        options_map& all_options)
    {
        // construct the overall options description and parse the command line
        all_options.emplace(options_type::desc_cmdline,
            "All HPX options allowed on the command line");

        all_options[options_type::desc_cmdline]
            .add(app_options)
            .add(all_options[options_type::commandline_options])
            .add(all_options[options_type::hpx_options])
            .add(all_options[options_type::config_options])
            .add(all_options[options_type::debugging_options])
            .add(all_options[options_type::hidden_options]);

        all_options.emplace(options_type::desc_cfgfile,
            "All HPX options allowed in configuration files");

        all_options[options_type::desc_cfgfile]
            .add(app_options)
            .add(all_options[options_type::hpx_options])
            .add(all_options[options_type::config_options])
            .add(all_options[options_type::debugging_options])
            .add(all_options[options_type::hidden_options]);
    }

    bool parse_commandline(util::section const& rtcfg,
        hpx::program_options::options_description const& app_options,
        std::string const& arg0, std::vector<std::string> const& args,
        hpx::program_options::variables_map& vm,
        util::commandline_error_mode error_mode,
        hpx::program_options::options_description* visible,
        std::vector<std::string>* unregistered_options)
    {
        try
        {
            options_map all_options = compose_local_options();

            compose_all_options(app_options, all_options);

            bool const result =
                parse_commandline(rtcfg, all_options, app_options, args, vm,
                    error_mode, visible, unregistered_options);

            handle_generic_config_options(arg0, vm,
                all_options[options_type::desc_cfgfile], rtcfg, error_mode);
            handle_config_options(
                vm, all_options[options_type::desc_cfgfile], rtcfg, error_mode);

            return result;
        }
        catch (std::exception const& e)
        {
            if (as_bool(error_mode &
                    util::commandline_error_mode::rethrow_on_error))
                throw;

            std::cerr << "hpx::init: exception caught: " << e.what()
                      << std::endl;
        }
        return false;
    }

    ///////////////////////////////////////////////////////////////////////////
    namespace detail {

        std::string extract_arg0(std::string const& cmdline)
        {
            std::string::size_type const p = cmdline.find_first_of(" \t");
            if (p != std::string::npos)
            {
                return cmdline.substr(0, p);
            }
            return cmdline;
        }
    }    // namespace detail

    bool parse_commandline(util::section const& rtcfg,
        hpx::program_options::options_description const& app_options,
        std::string const& cmdline, hpx::program_options::variables_map& vm,
        util::commandline_error_mode error_mode,
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
            detail::extract_arg0(cmdline), args, vm, error_mode, visible,
            unregistered_options);
    }

    ///////////////////////////////////////////////////////////////////////////
    std::string embed_in_quotes(std::string const& s)
    {
        char const quote =
            (s.find_first_of('"') != std::string::npos) ? '\'' : '"';

        if (s.find_first_of("\t ") != std::string::npos)
            return quote + s + quote;
        return s;
    }

    std::string reconstruct_command_line(int argc, char* argv[])
    {
        std::string command_line;
        for (int i = 0; i != argc; ++i)
        {
            if (!command_line.empty())
                command_line += " ";
            command_line += embed_in_quotes(argv[i]);
        }
        return command_line;
    }
}    // namespace hpx::local::detail
