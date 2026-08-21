//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Acceptance check: a sentinel or registry client can register its id with
// AGAS under a name pinned to the locality actually hosting its underlying
// server component, and unregister that name again, with no compile/link
// errors and no leftover AGAS state. This test only exercises the
// registration plumbing itself with explicit, manually-supplied peers; it
// deliberately never exercises name-based discovery/fan-out.

#include <hpx/hpx.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/modules/testing.hpp>

#include <hpx/supervision_dispatch.hpp>

#include <cstdint>
#include <string>
#include <vector>

// ============================================================================
// Test Cases
// ============================================================================

// register_name() must succeed and register the registry's id under a name
// pinned to the locality derived from the registry's own id.
void test_registry_register_name()
{
    hpx::id_type const target = hpx::find_here();

    hpx::supervision::registry r(target);

    bool const registered = r.register_name(hpx::launch::sync);
    HPX_TEST(registered);

    std::uint32_t const locality_id =
        hpx::naming::get_locality_id_from_id(r.get_locality());
    HPX_TEST_EQ(locality_id, hpx::naming::get_locality_id_from_id(target));

    std::string const expected_name =
        "/" + std::to_string(locality_id) + "/supervision_dispatch/registry";
    HPX_TEST_EQ(r.registered_name(), expected_name);

    r.unregister_name(hpx::launch::sync);
}

// Registering, unregistering, and then registering a freshly constructed
// registry under the very same pinned name (both instances live on the same
// locality, so they are assigned the same name) must succeed without error,
// confirming that unregister_name() actually removed the prior registration
// from AGAS rather than leaving it dangling.
void test_registry_register_unregister_cycle()
{
    hpx::id_type const here = hpx::find_here();

    {
        hpx::supervision::registry r(here);
        HPX_TEST(r.register_name(hpx::launch::sync));
        r.unregister_name(hpx::launch::sync);
    }

    hpx::supervision::registry r2(here);
    HPX_TEST(r2.register_name(hpx::launch::sync));
    r2.unregister_name(hpx::launch::sync);
}

// Same as test_registry_register_name(), but using the asynchronous
// overloads of register_name()/unregister_name().
void test_registry_register_name_async()
{
    hpx::supervision::registry r(hpx::find_here());

    hpx::future<bool> f = r.register_name();
    HPX_TEST(f.get());

    hpx::future<hpx::id_type> unreg_f = r.unregister_name();
    unreg_f.get();
}

// ============================================================================
// Main Test Entry Point
// ============================================================================
int hpx_main()
{
    test_registry_register_name();
    test_registry_register_unregister_cycle();
    test_registry_register_name_async();

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::init(argc, argv), 0);
    return hpx::util::report_errors();
}

#else

int main(int, char*[])
{
    return 0;
}

#endif
