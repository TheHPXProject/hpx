//  Copyright (c) 2026 Priyanshi Sharma
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#if defined(HPX_HAVE_CXX26_REFLECTION)

#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/async.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/testing.hpp>

#include <cstdint>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// Component action test
struct reflect_server : hpx::components::managed_component_base<reflect_server>
{
    hpx::id_type identity(std::int32_t i) const
    {
        HPX_TEST_EQ(i, std::int32_t(42));
        return hpx::find_here();
    }
};
using reflect_server_type = hpx::components::managed_component<reflect_server>;
HPX_REGISTER_COMPONENT(reflect_server_type, reflect_server)

///////////////////////////////////////////////////////////////////////////////
hpx::id_type reflect_increment(std::int32_t i)
{
    HPX_TEST_EQ(i, std::int32_t(42));
    return hpx::find_here();
}

///////////////////////////////////////////////////////////////////////////////
void test_async_reflect(hpx::id_type const& target)
{
    // Test: hpx::async<^^func>(target, ...) -- reflection-based syntax
    // reflect_increment returns hpx::find_here() so we can verify
    // the action executed on the expected target locality.
    {
        hpx::future<hpx::id_type> f1 =
            hpx::async<^^reflect_increment>(target, 42);
        HPX_TEST_EQ(f1.get(), target);
    }
    {
        hpx::future<hpx::id_type> f1 =
            hpx::async<^^reflect_increment>(hpx::launch::async, target, 42);
        HPX_TEST_EQ(f1.get(), target);
    }
}

int hpx_main()
{
    std::vector<hpx::id_type> localities = hpx::find_all_localities();
    for (hpx::id_type const& id : localities)
    {
        test_async_reflect(id);
    }
    // Test: reflect_component_action -- component member function
    // Instantiate reflect_server on each locality and verify identity()
    // returns the locality it ran on.
    for (hpx::id_type const& id : localities)
    {
        hpx::future<hpx::id_type> server_id = hpx::new_<reflect_server>(id);
        using identity_action =
            hpx::actions::reflect_component_action<^^reflect_server::identity>;
        hpx::future<hpx::id_type> f =
            hpx::async<identity_action>(server_id.get(), std::int32_t(42));
        HPX_TEST_EQ(f.get(), id);
    }
    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(
        hpx::init(argc, argv), 0, "HPX main exited with non-zero status");
    return hpx::util::report_errors();
}

#endif    // HPX_HAVE_CXX26_REFLECTION
#endif    // HPX_COMPUTE_DEVICE_CODE
