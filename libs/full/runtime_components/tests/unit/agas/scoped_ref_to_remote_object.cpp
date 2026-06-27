//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/iostream.hpp>
#include <hpx/modules/components_base.hpp>
#include <hpx/modules/testing.hpp>

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "components/managed_refcnt_checker.hpp"
#include "components/simple_refcnt_checker.hpp"

using hpx::program_options::options_description;
using hpx::program_options::value;
using hpx::program_options::variables_map;

using hpx::finalize;
using hpx::init;

using std::chrono::milliseconds;

using hpx::id_type;
using hpx::naming::get_management_type_name;

using hpx::components::component_type;
using hpx::components::get_component_type;

using hpx::applier::get_applier;

using hpx::agas::garbage_collect;

using hpx::test::managed_refcnt_monitor;
using hpx::test::simple_refcnt_monitor;

using hpx::util::report_errors;

using hpx::cout;

///////////////////////////////////////////////////////////////////////////////
template <typename Client>
void hpx_test_main(variables_map& vm)
{
    std::uint64_t const delay = vm["delay"].as<std::uint64_t>();

    {
        /// AGAS reference-counting test 2 (from #126):
        ///
        ///     Create a component remotely and let all references to it go out
        ///     of scope. The component should be deleted.

        typedef typename Client::server_type server_type;

        component_type ctype = get_component_type<server_type>();
        std::vector<id_type> remote_localities =
            hpx::find_remote_localities(ctype);

        if (remote_localities.empty())
            throw std::logic_error("this test cannot be run on one locality");

        Client monitor(remote_localities[0]);

        cout << "id: " << monitor.get_id() << " "
             << get_management_type_name(monitor.get_id().get_management_type())
             << "\n"
             << std::flush;

        {
            // Detach the reference.
            id_type id = monitor.detach().get();
            (void) id;

            // The component should still be alive.
            HPX_TEST_EQ(false, monitor.is_ready(milliseconds(delay)));
        }

        // Flush pending reference counting operations.
        garbage_collect(remote_localities[0]);
        garbage_collect();
        garbage_collect(remote_localities[0]);
        garbage_collect();

        // The component should be out of scope now.
        HPX_TEST_EQ(true, monitor.is_ready(milliseconds(delay)));
    }
}

///////////////////////////////////////////////////////////////////////////////
int hpx_main(variables_map& vm)
{
    {
        cout << std::string(80, '#') << "\n"
             << "simple component test\n"
             << std::string(80, '#') << "\n"
             << std::flush;

        hpx_test_main<simple_refcnt_monitor>(vm);

        cout << std::string(80, '#') << "\n"
             << "managed component test\n"
             << std::string(80, '#') << "\n"
             << std::flush;

        hpx_test_main<managed_refcnt_monitor>(vm);
    }

    finalize();
    return report_errors();
}

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
    // Configure application-specific options.
    options_description cmdline("usage: " HPX_APPLICATION_STRING " [options]");

    cmdline.add_options()("delay", value<std::uint64_t>()->default_value(1000),
        "number of milliseconds to wait for object destruction");

    // We need to explicitly enable the test components used by this test.
    std::vector<std::string> const cfg = {
        "hpx.components.simple_refcnt_checker.enabled! = 1",
        "hpx.components.managed_refcnt_checker.enabled! = 1"};
    // Initialize and run HPX.
    hpx::init_params init_args;
    init_args.desc_cmdline = cmdline;
    init_args.cfg = cfg;

    return hpx::init(argc, argv, init_args);
}
#endif
