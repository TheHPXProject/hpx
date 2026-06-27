//  Copyright (c) 2007-2012 Hartmut Kaiser
//  Copyright (c) 2011      Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/include/util.hpp>
#include <hpx/modules/components_base.hpp>

#include "throttle/throttle.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using hpx::program_options::variables_map;

using hpx::string_util::is_space;
using hpx::string_util::split;

using hpx::naming::get_agas_client;

///////////////////////////////////////////////////////////////////////////////
int hpx_main(variables_map& vm)
{
    try
    {
        hpx::util::format_to(std::cout, "prefix: {}",
            hpx::naming::get_locality_id_from_id(hpx::find_here()))
            << std::endl;

        // Try to connect to existing throttle instance, create a new one if
        // this fails.
        char const* throttle_component_name = "/throttle/0";
        hpx::id_type gid =
            hpx::agas::resolve_name(hpx::launch::sync, throttle_component_name);
        throttle::throttle t;
        if (!t.get_id())
        {
            std::vector<hpx::id_type> localities =
                hpx::find_remote_localities();

            // create throttle on the console, register the instance with AGAS
            // and add an additional reference count to keep it alive
            if (!localities.empty())
            {
                // use AGAS client to get the component type as we do not
                // register any factories
                t = hpx::new_<throttle::throttle>(localities[0]);
                hpx::agas::register_name(
                    hpx::launch::sync, throttle_component_name, t.get_id());
            }
            else
            {
                std::cerr << "Can't find throttle component." << std::endl;
            }
        }

        // handle commands
        if (t.get_id())
        {
            if (vm.count("suspend"))
            {
                t.suspend(vm["suspend"].as<int>());
            }
            else if (vm.count("resume"))
            {
                t.resume(vm["resume"].as<int>());
            }
            else if (vm.count("release"))
            {
                // unregister from AGAS, remove additional reference count which
                // will allow for the throttle instance to be released
                hpx::agas::unregister_name(
                    hpx::launch::sync, throttle_component_name);
            }
        }
    }
    catch (hpx::exception const& e)
    {
        std::cerr << "throttle_client: caught exception: " << e.what()
                  << std::endl;
    }

    hpx::disconnect();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    namespace po = hpx::program_options;

    // Configure application-specific options
    po::options_description cmdline(
        "Usage: " HPX_APPLICATION_STRING " [options]");
    cmdline.add_options()(
        "suspend", po::value<int>(), "suspend thread with given number")(
        "resume", po::value<int>(), "resume thread with given number")(
        "release", "release throttle component instance");

    // Disable loading of all external components
    std::vector<std::string> const cfg = {
        "hpx.components.load_external=0", "hpx.run_hpx_main!=1"};

    hpx::init_params init_args;
    init_args.mode = hpx::runtime_mode::connect;
    init_args.cfg = cfg;
    init_args.desc_cmdline = cmdline;

    return hpx::init(argc, argv, init_args);
}

#endif
