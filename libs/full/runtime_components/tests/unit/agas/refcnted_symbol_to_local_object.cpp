//  Copyright (c) 2011 Bryce Adelstein-Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
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
using hpx::find_here;
using hpx::init;

using std::chrono::milliseconds;

using hpx::id_type;
using hpx::naming::get_management_type_name;

using hpx::agas::garbage_collect;
using hpx::agas::register_name;
using hpx::agas::unregister_name;

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
        /// AGAS reference-counting test 7 (from #126):
        ///
        ///     Create a component locally, and register a symbolic name for it.
        ///     Then, let all references to the component go out of scope. The
        ///     component should still be alive. Finally, unregister the
        ///     symbolic name. The component should be deleted after the
        ///     symbolic name is unregistered.

        char const name[] = "/tests(refcnt_checker#7)";

        Client monitor(find_here());

        cout << "id: " << monitor.get_id() << " "
             << get_management_type_name(monitor.get_id().get_management_type())
             << "\n"
             << std::flush;

        // Associate a symbolic name with the object.
        HPX_TEST_EQ(
            true, register_name(hpx::launch::sync, name, monitor.get_id()));

        hpx::naming::gid_type gid;

        {
            // Detach the reference.
            id_type id = monitor.detach().get();

            // The component should still be alive.
            HPX_TEST_EQ(false, monitor.is_ready(milliseconds(delay)));

            gid = id.get_gid();
            (void) gid;

            // let id go out of scope
        }

        // The component should still be alive, as the symbolic binding holds
        // a reference to it.
        HPX_TEST_EQ(false, monitor.is_ready(milliseconds(delay)));

        // Remove the symbolic name. This should return the final credits
        // to AGAS.
        HPX_TEST_EQ(gid, unregister_name(hpx::launch::sync, name).get_gid());

        // Flush pending reference counting operations.
        garbage_collect();
        garbage_collect();

        // The component should be destroyed.
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

    cmdline.add_options()("delay", value<std::uint64_t>()->default_value(500),
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
