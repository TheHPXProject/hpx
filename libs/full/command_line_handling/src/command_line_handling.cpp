//  Copyright (c) 2007-2025 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/assert.hpp>
#include <hpx/modules/asio.hpp>
#include <hpx/modules/batch_environments.hpp>
#include <hpx/modules/debugging.hpp>
#include <hpx/modules/format.hpp>
#include <hpx/modules/functional.hpp>
#include <hpx/modules/logging.hpp>
#if defined(HPX_HAVE_MODULE_MPI_BASE)
#include <hpx/modules/mpi_base.hpp>
#endif
#if defined(HPX_HAVE_MODULE_LCI_BASE)
#include <hpx/modules/lci_base.hpp>
#endif
#if defined(HPX_HAVE_MODULE_LCW_BASE)
#include <hpx/modules/lcw_base.hpp>
#endif
#if defined(HPX_HAVE_MODULE_GASNET_BASE)
#include <hpx/modules/gasnet_base.hpp>
#endif
#include <hpx/modules/program_options.hpp>
#include <hpx/modules/runtime_configuration.hpp>
#include <hpx/modules/topology.hpp>
#include <hpx/modules/util.hpp>
#if defined(HPX_HAVE_MAX_CPU_COUNT)
#include <hpx/modules/preprocessor.hpp>
#endif
#include <hpx/version.hpp>

#include <hpx/command_line_handling/command_line_handling.hpp>
#include <hpx/command_line_handling/parse_command_line.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <hpx/config/warnings_prefix.hpp>

namespace hpx::util {

    namespace detail {

        ///////////////////////////////////////////////////////////////////////
        std::string runtime_configuration_string(
            util::command_line_handling const& cfg)
        {
            std::ostringstream strm;

            // runtime mode
            strm << "  {mode}: " << get_runtime_mode_name(cfg.rtcfg_.mode_)
                 << "\n";

            if (cfg.num_localities_ != 1)
                strm << "  {localities}: " << cfg.num_localities_ << "\n";

            strm << hpx::local::detail::runtime_configuration_string(cfg);

            return strm.str();
        }

        ///////////////////////////////////////////////////////////////////////
        void report_thread_warning(std::string const& batch_name,
            std::size_t threads, std::size_t batch_threads)
        {
            std::cerr << "hpx::init: command line warning: --hpx:threads "
                         "used when running with "
                      << batch_name
                      << ", requesting a larger number of threads (" << threads
                      << ") than cores have been assigned by " << batch_name
                      << " (" << batch_threads
                      << "), the application might not run properly."
                      << std::endl;
        }

        void report_locality_warning_batch(std::string const& batch_name,
            std::size_t batch_localities, std::size_t num_localities)
        {
            std::cerr << "hpx::init: command line warning: "
                         "--hpx:localities used when running with "
                      << batch_name
                      << ", requesting a different number of localities ("
                      << num_localities << ") than have been assigned by "
                      << batch_name << " (" << batch_localities
                      << "), the application might not run properly."
                      << std::endl;
        }

        void report_locality_warning(std::string const& batch_name,
            std::size_t cmdline_localities, std::size_t num_localities)
        {
            std::cerr << "hpx::init: command line warning: "
                         "--hpx:localities used when running with "
                      << batch_name
                      << ", requesting a different number of localities ("
                      << num_localities
                      << ") than have been assigned on the command line "
                      << " (" << cmdline_localities
                      << "), the application might not run properly."
                      << std::endl;
        }

        ///////////////////////////////////////////////////////////////////////
        std::size_t handle_num_localities(util::manage_config const& cfgmap,
            hpx::program_options::variables_map const& vm,
            util::batch_environment const& env, bool using_nodelist,
            std::size_t num_localities, bool initial)
        {
            std::size_t const batch_localities =
                env.retrieve_number_of_localities();
            if (num_localities == 1 &&
                batch_localities != static_cast<std::size_t>(-1))
            {
                if (auto const cfg_num_localities =
                        cfgmap.get_value<std::size_t>(
                            "hpx.localities", batch_localities);
                    cfg_num_localities > 1)
                {
                    num_localities = cfg_num_localities;
                }
            }

            if (!initial && env.found_batch_environment() && using_nodelist &&
                (batch_localities != num_localities) && (num_localities != 1))
            {
                detail::report_locality_warning_batch(
                    env.get_batch_name(), batch_localities, num_localities);
            }

            if (vm.count("hpx:localities"))
            {
                std::size_t const localities =
                    vm["hpx:localities"].as<std::size_t>();

                if (localities == 0)
                {
                    throw hpx::detail::command_line_error(
                        "Number of --hpx:localities must be greater than 0");
                }

                if (!initial && env.found_batch_environment() &&
                    using_nodelist && (localities != num_localities) &&
                    (num_localities != 1))
                {
                    detail::report_locality_warning(
                        env.get_batch_name(), localities, num_localities);
                }
                num_localities = localities;
            }

#if !defined(HPX_HAVE_NETWORKING)
            if (num_localities != 1)
            {
                throw hpx::detail::command_line_error(
                    "Number of --hpx:localities must be equal to 1, please "
                    "enable networking to run distributed HPX applications "
                    "(use -DHPX_WITH_NETWORKING=On during configuration)");
            }
#endif
            return num_localities;
        }

        ///////////////////////////////////////////////////////////////////////
        std::size_t get_number_of_default_cores(
            util::batch_environment const& env, bool use_process_mask)
        {
            std::size_t const num_cores =
                hpx::local::detail::get_number_of_default_cores(
                    use_process_mask);
            if (use_process_mask)
            {
                return num_cores;
            }

            std::size_t batch_threads = env.retrieve_number_of_threads();
            if (batch_threads == static_cast<std::size_t>(-1))
            {
                return num_cores;
            }

            // assuming we assign the first N cores ...
            threads::topology const& top = threads::create_topology();
            std::size_t core = 0;
            for (/**/; core < num_cores; ++core)
            {
                batch_threads -= top.get_number_of_core_pus(core);
                if (batch_threads == 0)
                    break;
            }
            return core + 1;
        }

        ///////////////////////////////////////////////////////////////////////
        std::size_t handle_num_threads(util::manage_config const& cfgmap,
            util::runtime_configuration const& rtcfg,
            hpx::program_options::variables_map const& vm,
            util::batch_environment const& env, bool using_nodelist,
            bool initial, bool use_process_mask)
        {
            // If using the process mask we override "cores" and "all" options
            // but keep explicit numeric values.
            std::size_t const init_threads =
                hpx::local::detail::get_number_of_default_threads(
                    use_process_mask);
            std::size_t const init_cores =
                detail::get_number_of_default_cores(env, use_process_mask);
            std::size_t const batch_threads = env.retrieve_number_of_threads();

            auto threads_str = cfgmap.get_value<std::string>("hpx.os_threads",
                rtcfg.get_entry(
                    "hpx.os_threads", std::to_string(init_threads)));

            std::size_t threads;
            if ("cores" == threads_str)
            {
                threads = init_cores;
                if (batch_threads != static_cast<std::size_t>(-1))
                {
                    threads = batch_threads;
                }
            }
            else if ("all" == threads_str)
            {
                threads = init_threads;
                if (batch_threads != static_cast<std::size_t>(-1))
                {
                    threads = batch_threads;
                }
            }
            else if (batch_threads != static_cast<std::size_t>(-1))
            {
                threads = batch_threads;
            }
            else
            {
                threads = cfgmap.get_value<std::size_t>("hpx.os_threads",
                    hpx::util::from_string<std::size_t>(threads_str));
            }

            if (vm.count("hpx:threads"))
            {
                threads_str = vm["hpx:threads"].as<std::string>();
                if ("all" == threads_str)
                {
                    threads = init_threads;
                    if (batch_threads != static_cast<std::size_t>(-1))
                    {
                        threads = batch_threads;
                    }
                }
                else if ("cores" == threads_str)
                {
                    threads = init_cores;
                    if (batch_threads != static_cast<std::size_t>(-1))
                    {
                        threads = batch_threads;
                    }
                }
                else
                {
                    threads = hpx::util::from_string<std::size_t>(threads_str);
                }

                if (threads == 0)
                {
                    throw hpx::detail::command_line_error(
                        "Number of --hpx:threads must be greater than 0");
                }

#if defined(HPX_HAVE_MAX_CPU_COUNT)
                if (threads > HPX_HAVE_MAX_CPU_COUNT)
                {
                    // clang-format off
                    throw hpx::detail::command_line_error("Requested more than "
                        HPX_PP_STRINGIZE(HPX_HAVE_MAX_CPU_COUNT)" --hpx:threads "
                        "to use for this application, use the option "
                        "-DHPX_WITH_MAX_CPU_COUNT=<N> when configuring HPX.");
                    // clang-format on
                }
#endif
            }

            // make sure minimal requested number of threads is observed
            auto min_os_threads = cfgmap.get_value<std::size_t>(
                "hpx.force_min_os_threads", threads);

            if (min_os_threads == 0)
            {
                throw hpx::detail::command_line_error(
                    "Number of hpx.force_min_os_threads must be greater than "
                    "0");
            }

#if defined(HPX_HAVE_MAX_CPU_COUNT)
            if (min_os_threads > HPX_HAVE_MAX_CPU_COUNT)
            {
                // clang-format off
                throw hpx::detail::command_line_error("Requested more than "
                    HPX_PP_STRINGIZE(HPX_HAVE_MAX_CPU_COUNT)
                    " hpx.force_min_os_threads to use for this application, "
                    "use the option -DHPX_WITH_MAX_CPU_COUNT=<N> when "
                    "configuring HPX.");
                // clang-format on
            }
#endif

            threads = (std::max) (threads, min_os_threads);

            if (!initial && env.found_batch_environment() && using_nodelist &&
                (threads > batch_threads))
            {
                detail::report_thread_warning(
                    env.get_batch_name(), threads, batch_threads);
            }
            return threads;
        }

        ///////////////////////////////////////////////////////////////////////
#if !defined(HPX_HAVE_NETWORKING)
        void check_networking_option(
            hpx::program_options::variables_map const& vm, char const* option)
        {
            if (vm.count(option) != 0)
            {
                throw hpx::detail::command_line_error(
                    std::string("Invalid command line option: '--") + option +
                    "', networking was disabled at configuration time. "
                    "Reconfigure HPX using -DHPX_WITH_NETWORKING=On.");
            }
        }
#endif

        void check_networking_options(
            [[maybe_unused]] hpx::program_options::variables_map const& vm)
        {
#if !defined(HPX_HAVE_NETWORKING)
            check_networking_option(vm, "hpx:agas");
            check_networking_option(vm, "hpx:run-agas-server-only");
            check_networking_option(vm, "hpx:hpx");
            check_networking_option(vm, "hpx:nodefile");
            check_networking_option(vm, "hpx:nodes");
            check_networking_option(vm, "hpx:endnodes");
            check_networking_option(vm, "hpx:ifsuffix");
            check_networking_option(vm, "hpx:ifprefix");
            check_networking_option(vm, "hpx:iftransform");
            check_networking_option(vm, "hpx:localities");
            check_networking_option(vm, "hpx:node");
            check_networking_option(vm, "hpx:expect-connecting-localities");
#endif
        }
    }    // namespace detail

    ///////////////////////////////////////////////////////////////////////////
    command_line_handling::command_line_handling(runtime_configuration rtcfg,
        std::vector<std::string> ini_config,
        hpx::function<int(hpx::program_options::variables_map& vm)> hpx_main_f)
      : base_type(HPX_MOVE(rtcfg), HPX_MOVE(ini_config), HPX_MOVE(hpx_main_f))
      , node_(static_cast<std::size_t>(-1))
      , num_localities_(1)
    {
    }

    bool command_line_handling::handle_arguments(util::manage_config& cfgmap,
        hpx::program_options::variables_map& vm,
        std::vector<std::string>& ini_config, std::size_t& node, bool initial)
    {
        // handle local options
        base_type::handle_arguments(cfgmap, vm, ini_config);

        // verify that no networking options were used if networking was
        // disabled
        detail::check_networking_options(vm);

        bool debug_clp =
            node != static_cast<std::size_t>(-1) && vm.count("hpx:debug-clp");

        // create host name mapping
        [[maybe_unused]] bool have_tcp =
            rtcfg_.get_entry("hpx.parcel.tcp.enable", "1") != "0";
        util::map_hostnames mapnames(debug_clp);

        if (vm.count("hpx:ifsuffix"))
            mapnames.use_suffix(vm["hpx:ifsuffix"].as<std::string>());
        if (vm.count("hpx:ifprefix"))
            mapnames.use_prefix(vm["hpx:ifprefix"].as<std::string>());
        mapnames.force_ipv4(vm.count("hpx:force_ipv4") != 0);

        // The AGAS host name and port number are pre-initialized from
        //the command line
        auto agas_host = cfgmap.get_value<std::string>(
            "hpx.agas.address", rtcfg_.get_entry("hpx.agas.address", ""));
        auto agas_port = cfgmap.get_value<std::uint16_t>("hpx.agas.port",
            hpx::util::from_string<std::uint16_t>(
                rtcfg_.get_entry("hpx.agas.port", HPX_INITIAL_IP_PORT)));

        if (vm.count("hpx:agas"))
        {
            if (!util::split_ip_address(
                    vm["hpx:agas"].as<std::string>(), agas_host, agas_port))
            {
                std::cerr << "hpx::init: command line warning: illegal port "
                             "number given, using default value instead."
                          << std::endl;
            }
        }

        // Check command line arguments.
        if (vm.count("hpx:iftransform"))
        {
            util::sed_transform iftransform(
                vm["hpx:iftransform"].as<std::string>());

            // Check for parsing failures
            if (!iftransform)
            {
                throw hpx::detail::command_line_error(hpx::util::format(
                    "Could not parse --hpx:iftransform argument '{1}'",
                    vm["hpx:iftransform"].as<std::string>()));
            }

            using transform_function_type =
                util::map_hostnames::transform_function_type;
            mapnames.use_transform(transform_function_type(iftransform));
        }

        bool using_nodelist = false;

        std::vector<std::string> nodelist;

#if defined(HPX_HAVE_NETWORKING)
        if (vm.count("hpx:nodefile"))
        {
            if (vm.count("hpx:nodes"))
            {
                throw hpx::detail::command_line_error(
                    "Ambiguous command line options. Do not specify more than "
                    "one of the --hpx:nodefile and --hpx:nodes options at the "
                    "same time.");
            }

            std::string node_file = vm["hpx:nodefile"].as<std::string>();
            ini_config.emplace_back("hpx.nodefile!=" + node_file);
            std::ifstream ifs(node_file.c_str());
            if (ifs.is_open())
            {
                if (debug_clp)
                    std::cerr << "opened: " << node_file << std::endl;
                std::string line;
                while (std::getline(ifs, line))
                {
                    if (!line.empty())
                    {
                        nodelist.push_back(line);
                    }
                }
            }
            else
            {
                if (debug_clp)
                    std::cerr << "failed opening: " << node_file << std::endl;

                // raise hard error if node file could not be opened
                throw hpx::detail::command_line_error(hpx::util::format(
                    "Could not open nodefile: '{}'", node_file));
            }
        }
        else if (vm.count("hpx:nodes"))
        {
            nodelist = vm["hpx:nodes"].as<std::vector<std::string>>();
        }
#endif
        bool enable_batch_env =
            ((cfgmap.get_value<std::size_t>("hpx.ignore_batch_env", 0) +
                 vm.count("hpx:ignore-batch-env")) == 0) &&
            !use_process_mask_;

#if defined(HPX_HAVE_MODULE_MPI_BASE)
        bool have_mpi = util::mpi_environment::check_mpi_environment(rtcfg_);
#else
        bool have_mpi = false;
#endif

        util::batch_environment env(
            nodelist, have_mpi, debug_clp, enable_batch_env);

#if defined(HPX_HAVE_NETWORKING)
        if (!nodelist.empty())
        {
            using_nodelist = true;
            ini_config.emplace_back("hpx.nodes!=" +
                env.init_from_nodelist(nodelist, agas_host, have_tcp));
        }

        // let the batch environment decide about the AGAS host
        agas_host = env.agas_host_name(
            agas_host.empty() ? HPX_INITIAL_IP_ADDRESS : agas_host);
#endif

        [[maybe_unused]] bool run_agas_server = false;
        [[maybe_unused]] std::string hpx_host;
        [[maybe_unused]] std::uint16_t hpx_port = 0;

#if defined(HPX_HAVE_NETWORKING)
        bool expect_connections = false;
        std::uint16_t initial_hpx_port = 0;

        // handling number of localities, those might have already been
        // initialized from MPI environment
        num_localities_ = detail::handle_num_localities(
            cfgmap, vm, env, using_nodelist, num_localities_, initial);

        // Determine our network port, use arbitrary port if running on one
        // locality.
        hpx_host = cfgmap.get_value<std::string>("hpx.parcel.address",
            env.host_name(rtcfg_.get_entry(
                "hpx.parcel.address", HPX_INITIAL_IP_ADDRESS)));

        // we expect dynamic connections if:
        //  - --hpx:expect-connecting-localities or
        //  - hpx.expect_connecting_localities=1 is given, or
        //  - num_localities > 1
        expect_connections =
            cfgmap.get_value<int>("hpx.expect_connecting_localities",
                num_localities_ > 1 ? 1 : 0) != 0;

        if (vm.count("hpx:expect-connecting-localities"))
            expect_connections = true;

        ini_config.emplace_back(
            std::string("hpx.expect_connecting_localities=") +
            (expect_connections ? "1" : "0"));

        if (num_localities_ != 1 || expect_connections)
        {
            initial_hpx_port = hpx::util::from_string<std::uint16_t>(
                rtcfg_.get_entry("hpx.parcel.port", HPX_INITIAL_IP_PORT));
        }

        hpx_port = cfgmap.get_value<std::uint16_t>(
            "hpx.parcel.port", initial_hpx_port);

        run_agas_server = vm.count("hpx:run-agas-server") != 0;
        if (node == static_cast<std::size_t>(-1))
            node = env.retrieve_node_number();

        // make sure that TCP parcelport will only be enabled if necessary
        if (num_localities_ == 1 && !expect_connections)
            have_tcp = false;
#else
        num_localities_ = 1;
        node = 0;
        have_tcp = false;
#endif

        // If the user has not specified an explicit runtime mode we retrieve it
        // from the command line.
        if (hpx::runtime_mode::default_ == rtcfg_.mode_)
        {
#if defined(HPX_HAVE_NETWORKING)
            // The default mode is console, i.e. all workers need to be started
            // with --worker/-w.
            rtcfg_.mode_ = hpx::runtime_mode::console;
            if (vm.count("hpx:local") + vm.count("hpx:console") +
                    vm.count("hpx:worker") + vm.count("hpx:connect") >
                1)
            {
                throw hpx::detail::command_line_error(
                    "Ambiguous command line options. Do not specify more than "
                    "one of --hpx:local, --hpx:console, --hpx:worker, or "
                    "--hpx:connect");
            }

            // In these cases we default to executing with an empty hpx_main,
            // except if specified otherwise.
            if (vm.count("hpx:worker"))
            {
                rtcfg_.mode_ = hpx::runtime_mode::worker;

                // do not execute any explicit hpx_main except if asked
                // otherwise
                if (!vm.count("hpx:run-hpx-main") &&
                    !cfgmap.get_value<int>("hpx.run_hpx_main", 0))
                {
                    util::detail::reset_function(hpx_main_f_);
                }
            }
            else if (vm.count("hpx:connect"))
            {
                rtcfg_.mode_ = hpx::runtime_mode::connect;
            }
            else if (vm.count("hpx:local"))
            {
                rtcfg_.mode_ = hpx::runtime_mode::local;
            }
#elif defined(HPX_HAVE_DISTRIBUTED_RUNTIME)
            if (vm.count("hpx:local"))
            {
                rtcfg_.mode_ = hpx::runtime_mode::local;
            }
            else
            {
                rtcfg_.mode_ = hpx::runtime_mode::console;
            }
#else
            rtcfg_.mode_ = hpx::runtime_mode::local;
#endif
        }

#if defined(HPX_HAVE_NETWORKING)
        if (rtcfg_.mode_ != hpx::runtime_mode::local)
        {
            // we initialize certain settings if --node is specified (or data
            // has been retrieved from the environment)
            if (rtcfg_.mode_ == hpx::runtime_mode::connect)
            {
                // when connecting we need to select a unique port
                hpx_port = cfgmap.get_value<std::uint16_t>("hpx.parcel.port",
                    hpx::util::from_string<std::uint16_t>(rtcfg_.get_entry(
                        "hpx.parcel.port", HPX_CONNECTING_IP_PORT)));

                // do not execute any explicit hpx_main except if asked
                // otherwise
                if (!vm.count("hpx:run-hpx-main") &&
                    !cfgmap.get_value<int>("hpx.run_hpx_main", 0))
                {
                    util::detail::reset_function(hpx_main_f_);
                }
            }
            else if (node != static_cast<std::size_t>(-1) ||
                vm.count("hpx:node"))
            {
                // command line overwrites the environment
                if (vm.count("hpx:node"))
                {
                    if (vm.count("hpx:agas"))
                    {
                        throw hpx::detail::command_line_error(
                            "Command line option --hpx:node is not compatible "
                            "with --hpx:agas");
                    }
                    node = vm["hpx:node"].as<std::size_t>();
                }

                if (!vm.count("hpx:worker"))
                {
                    if (env.agas_node() == node)
                    {
                        // console node, by default runs AGAS
                        run_agas_server = true;
                        rtcfg_.mode_ = hpx::runtime_mode::console;
                    }
                    else
                    {
                        // don't use port zero for non-console localities
                        if (hpx_port == 0 && node != 0)
                            hpx_port = HPX_INITIAL_IP_PORT;

                        // each node gets a unique port
                        hpx_port = static_cast<std::uint16_t>(hpx_port + node);
                        rtcfg_.mode_ = hpx::runtime_mode::worker;

                        // do not execute any explicit hpx_main except if asked
                        // otherwise
                        if (!vm.count("hpx:run-hpx-main") &&
                            !cfgmap.get_value<int>("hpx.run_hpx_main", 0))
                        {
                            util::detail::reset_function(hpx_main_f_);
                        }
                    }
                }

                // store node number in configuration, don't do that if we're on
                // a worker and the node number is zero
                if (!vm.count("hpx:worker") || node != 0)
                {
                    ini_config.emplace_back(
                        "hpx.locality!=" + std::to_string(node));
                }
            }

            if (vm.count("hpx:hpx"))
            {
                if (!util::split_ip_address(
                        vm["hpx:hpx"].as<std::string>(), hpx_host, hpx_port))
                {
                    std::cerr
                        << "hpx::init: command line warning: illegal port "
                           "number given, using default value instead."
                        << std::endl;
                }
            }

            if ((vm.count("hpx:connect") ||
                    rtcfg_.mode_ == hpx::runtime_mode::connect) &&
                hpx_host == "127.0.0.1")
            {
                hpx_host = hpx::util::resolve_public_ip_address();
            }

            ini_config.emplace_back("hpx.node!=" + std::to_string(node));
        }
#endif

        // handle number of cores and threads
        num_threads_ = detail::handle_num_threads(cfgmap, rtcfg_, vm, env,
            using_nodelist, initial, use_process_mask_);

        num_cores_ = hpx::local::detail::handle_num_cores_default(cfgmap, vm,
            num_threads_,
            detail::get_number_of_default_cores(env, use_process_mask_));

        // Set number of cores and OS threads in configuration.
        ini_config.emplace_back(
            "hpx.os_threads=" + std::to_string(num_threads_));
        ini_config.emplace_back("hpx.cores=" + std::to_string(num_cores_));

        // handle high-priority threads
        handle_high_priority_threads(vm, ini_config);

#if defined(HPX_HAVE_PARCELPORT_TCP)
        // map host names to ip addresses, if requested
        if (have_tcp)
        {
            hpx_host = mapnames.map(hpx_host, hpx_port);
            agas_host = mapnames.map(agas_host, agas_port);
        }
#endif

        // sanity checks
        if (rtcfg_.mode_ != hpx::runtime_mode::local && num_localities_ == 1 &&
            !vm.count("hpx:agas") && !vm.count("hpx:node"))
        {
            // We assume we have to run the AGAS server if the number of
            // localities to run on is not specified (or is '1') and no
            // additional option (--hpx:agas or --hpx:node) has been specified.
            // That simplifies running small standalone applications on one
            // locality.
            run_agas_server = rtcfg_.mode_ != runtime_mode::connect;
        }

        if (rtcfg_.mode_ != hpx::runtime_mode::local)
        {
#if defined(HPX_HAVE_NETWORKING)
            if (hpx_host == agas_host && hpx_port == agas_port)
            {
                // we assume that we need to run the agas server if the user
                // asked for the same network addresses for HPX and AGAS
                run_agas_server = rtcfg_.mode_ != runtime_mode::connect;
            }
            else if (run_agas_server)
            {
                // otherwise, if the user instructed us to run the AGAS server,
                // we set the AGAS network address to the same value as the HPX
                // network address
                if (agas_host == HPX_INITIAL_IP_ADDRESS)
                {
                    agas_host = hpx_host;
                    agas_port = hpx_port;
                }
            }
            else if (env.found_batch_environment())
            {
                // in batch mode, if the network addresses are different, and we
                // should not run the AGAS server we assume to be in worker mode
                rtcfg_.mode_ = hpx::runtime_mode::worker;

                // do not execute any explicit hpx_main except if asked
                // otherwise
                if (!vm.count("hpx:run-hpx-main") &&
                    !cfgmap.get_value<int>("hpx.run_hpx_main", 0))
                {
                    util::detail::reset_function(hpx_main_f_);
                }
            }

            // write HPX and AGAS network parameters to the proper ini-file
            // entries
            ini_config.emplace_back("hpx.parcel.address=" + hpx_host);
            ini_config.emplace_back(
                "hpx.parcel.port=" + std::to_string(hpx_port));
            ini_config.emplace_back("hpx.agas.address=" + agas_host);
            ini_config.emplace_back(
                "hpx.agas.port=" + std::to_string(agas_port));

            if (run_agas_server)
            {
                ini_config.emplace_back("hpx.agas.service_mode=bootstrap");
            }

            // we can't run the AGAS server while connecting
            if (run_agas_server && rtcfg_.mode_ == runtime_mode::connect)
            {
                throw hpx::detail::command_line_error(
                    "Command line option error: can't run AGAS server"
                    "while connecting to a running application.");
            }
#else
            ini_config.emplace_back("hpx.agas.service_mode=bootstrap");
#endif
        }

        enable_logging_settings(vm, ini_config);

        // handle command line arguments after logging defaults
        if (vm.count("hpx:ini"))
        {
            std::vector<std::string> cfg =
                vm["hpx:ini"].as<std::vector<std::string>>();
            std::copy(cfg.begin(), cfg.end(), std::back_inserter(ini_config));
            cfgmap.add(cfg);
        }

        if (rtcfg_.mode_ != hpx::runtime_mode::local)
        {
            // Set number of localities in configuration (do it everywhere, even
            // if this information is only used by the AGAS server).
            ini_config.emplace_back(
                "hpx.localities!=" + std::to_string(num_localities_));

            // FIXME: AGAS V2: if a locality is supposed to run the AGAS
            //        service only and requests to use 'priority_local' as the
            //        scheduler, switch to the 'local' scheduler instead.
            ini_config.emplace_back(std::string("hpx.runtime_mode=") +
                get_runtime_mode_name(rtcfg_.mode_));

            bool noshutdown_evaluate = false;
            if (vm.count("hpx:print-counter-at"))
            {
                std::vector<std::string> print_counters_at =
                    vm["hpx:print-counter-at"].as<std::vector<std::string>>();

                for (std::string const& s : print_counters_at)
                {
                    if (0 == std::string("startup").find(s))
                    {
                        ini_config.emplace_back("hpx.print_counter.startup!=1");
                        continue;
                    }
                    if (0 == std::string("shutdown").find(s))
                    {
                        ini_config.emplace_back(
                            "hpx.print_counter.shutdown!=1");
                        continue;
                    }
                    if (0 == std::string("noshutdown").find(s))
                    {
                        ini_config.emplace_back(
                            "hpx.print_counter.shutdown!=0");
                        noshutdown_evaluate = true;
                        continue;
                    }

                    throw hpx::detail::command_line_error(hpx::util::format(
                        "Invalid argument for option --hpx:print-counter-at: "
                        "'{1}', allowed values: 'startup', 'shutdown' "
                        "(default), 'noshutdown'",
                        s));
                }
            }

            // if any counters have to be evaluated, always print at the end
            if (vm.count("hpx:print-counter") ||
                vm.count("hpx:print-counter-reset"))
            {
                if (!noshutdown_evaluate)
                    ini_config.emplace_back("hpx.print_counter.shutdown!=1");
                if (vm.count("hpx:reset-counters"))
                    ini_config.emplace_back("hpx.print_counter.reset!=1");
            }
        }

        if (debug_clp)
        {
            hpx::local::detail::print_config(ini_config);
        }

        return true;
    }

    ///////////////////////////////////////////////////////////////////////////
    void command_line_handling::enable_logging_settings(
        hpx::program_options::variables_map& vm,
        std::vector<std::string>& ini_config)
    {
        base_type::enable_logging_settings(vm, ini_config);

#if defined(HPX_HAVE_LOGGING) && defined(HPX_LOGGING_HAVE_SEPARATE_DESTINATIONS)
        if (vm.count("hpx:debug-agas-log"))
        {
            ini_config.emplace_back("hpx.logging.console.agas.destination=" +
                hpx::local::detail::convert_to_log_file(
                    vm["hpx:debug-agas-log"].as<std::string>()));
            ini_config.emplace_back("hpx.logging.agas.destination=" +
                hpx::local::detail::convert_to_log_file(
                    vm["hpx:debug-agas-log"].as<std::string>()));
            ini_config.emplace_back("hpx.logging.console.agas.level=5");
            ini_config.emplace_back("hpx.logging.agas.level=5");
        }

        if (vm.count("hpx:debug-parcel-log"))
        {
            ini_config.emplace_back("hpx.logging.console.parcel.destination=" +
                hpx::local::detail::convert_to_log_file(
                    vm["hpx:debug-parcel-log"].as<std::string>()));
            ini_config.emplace_back("hpx.logging.parcel.destination=" +
                hpx::local::detail::convert_to_log_file(
                    vm["hpx:debug-parcel-log"].as<std::string>()));
            ini_config.emplace_back("hpx.logging.console.parcel.level=5");
            ini_config.emplace_back("hpx.logging.parcel.level=5");
        }
#else
        if (vm.count("hpx:debug-agas-log") || vm.count("hpx:debug-parcel-log"))
        {
            throw hpx::detail::command_line_error(
                "Command line option error: can't enable logging while it was "
                "disabled at configuration time. Please re-configure HPX using "
                "the options -DHPX_WITH_LOGGING=On and "
                "-DHPX_LOGGING_WITH_SEPARATE_DESTINATIONS=On.");
        }
#endif
    }

    ///////////////////////////////////////////////////////////////////////////
    int command_line_handling::call(
        hpx::program_options::options_description const& desc_cmdline, int argc,
        char** argv,
        std::vector<std::shared_ptr<components::component_registry_base>>&
            component_registries)
    {
        // set the flag signaling that command line parsing has been done
        cmd_line_parsed_ = true;

        // separate command line arguments from configuration settings
        std::vector<std::string> args = preprocess_config_settings(argc, argv);

        util::manage_config cfgmap(ini_config_);

        // insert the pre-configured ini settings before loading modules
        for (std::string const& e : ini_config_)
            rtcfg_.parse("<user supplied config>", e, true, false);

        // support re-throwing command line exceptions for testing purposes
        util::commandline_error_mode error_mode =
            util::commandline_error_mode::allow_unregistered;
        if (cfgmap.get_value("hpx.commandline.rethrow_errors", 0) != 0)
        {
            error_mode |= util::commandline_error_mode::rethrow_on_error;
        }

        // The cfg registry may hold command line options to prepend to the
        // real command line.
        std::string prepend_command_line =
            rtcfg_.get_entry("hpx.commandline.prepend_options");

        args = hpx::local::detail::prepend_options(
            HPX_MOVE(args), HPX_MOVE(prepend_command_line));

        // Initial analysis of the command line options. This is preliminary as
        // it will not take into account any aliases as defined in any of the
        // runtime configuration files.
        {
            // Boost V1.47 and before do not properly reset a variables_map when
            // calling vm.clear(). We work around that problems by creating a
            // separate instance just for the preliminary command line handling.
            error_mode |= util::commandline_error_mode::ignore_aliases;
            hpx::program_options::variables_map prevm;
            if (!util::parse_commandline(rtcfg_, desc_cmdline, argv[0], args,
                    prevm, static_cast<std::size_t>(-1), error_mode,
                    rtcfg_.mode_))
            {
                return -1;
            }

            // handle all --hpx:foo options, determine node
            std::vector<std::string> ini_config;    // discard
            if (!handle_arguments(cfgmap, prevm, ini_config, node_, true))
            {
                return -2;
            }

            reconfigure(cfgmap, prevm);
        }

#if (defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_MPI)) ||      \
    defined(HPX_HAVE_MODULE_MPI_BASE)
        // getting localities from MPI environment (support mpirun)
        if (util::mpi_environment::check_mpi_environment(rtcfg_))
        {
            util::mpi_environment::init(&argc, &argv, rtcfg_);
            num_localities_ =
                static_cast<std::size_t>(util::mpi_environment::size());
            node_ = static_cast<std::size_t>(util::mpi_environment::rank());
        }
#endif
#if (defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_LCI)) ||      \
    defined(HPX_HAVE_MODULE_LCI_BASE)
        if (util::lci_environment::check_lci_environment(rtcfg_))
        {
            util::lci_environment::init(&argc, &argv, rtcfg_);
            num_localities_ =
                static_cast<std::size_t>(util::lci_environment::size());
            node_ = static_cast<std::size_t>(util::lci_environment::rank());
        }
#endif
#if (defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_LCW)) ||      \
    defined(HPX_HAVE_MODULE_LCW_BASE)
        if (util::lcw_environment::check_lcw_environment(rtcfg_))
        {
            util::lcw_environment::init(&argc, &argv, rtcfg_);
            num_localities_ =
                static_cast<std::size_t>(util::lcw_environment::size());
            node_ = static_cast<std::size_t>(util::lcw_environment::rank());
        }
#endif
#if (defined(HPX_HAVE_NETWORKING) && defined(HPX_HAVE_PARCELPORT_GASNET)) ||   \
    defined(HPX_HAVE_MODULE_GASNET_BASE)
        // better to put GASNET init after MPI init, since GASNET will also
        // initialize MPI if MPI is not already initialized.
        if (util::gasnet_environment::check_gasnet_environment(rtcfg_))
        {
            util::gasnet_environment::init(&argc, &argv, rtcfg_);
            num_localities_ =
                static_cast<std::size_t>(util::gasnet_environment::size());
            node_ = static_cast<std::size_t>(util::gasnet_environment::rank());
        }
#endif

        // load plugin modules (after first pass of command line handling, so
        // that settings given on command line could propagate to modules)
        std::vector<std::shared_ptr<plugins::plugin_registry_base>>
            plugin_registries = rtcfg_.load_modules(component_registries);

        // Re-run program option analysis, ini settings (such as aliases) will
        // be considered now.

        // minimally assume one locality and this is the console
        if (node_ == static_cast<std::size_t>(-1))
            node_ = 0;

        for (std::shared_ptr<plugins::plugin_registry_base>& reg :
            plugin_registries)
        {
            reg->init(&argc, &argv, rtcfg_);
        }

        // Now reparse the command line using the node number (if given). This
        // will additionally detect any --hpx:N:foo options.
        hpx::program_options::options_description help;
        std::vector<std::string> unregistered_options;

        error_mode |= util::commandline_error_mode::report_missing_config_file;
        if (!util::parse_commandline(rtcfg_, desc_cmdline, argv[0], args, vm_,
                node_, error_mode, rtcfg_.mode_, &help, &unregistered_options))
        {
            return -1;
        }

        // break into debugger, if requested
        handle_attach_debugger();

        // handle all --hpx:foo and --hpx:*:foo options
        if (!handle_arguments(cfgmap, vm_, ini_config_, node_))
        {
            return -2;
        }

        return finalize_commandline_handling(
            argc, argv, help, unregistered_options);
    }
}    // namespace hpx::util
