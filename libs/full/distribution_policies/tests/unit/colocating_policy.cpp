//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_main.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/runtime.hpp>

#include <hpx/modules/distribution_policies.hpp>
#include <hpx/modules/testing.hpp>

#include <cstddef>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
struct test_server : hpx::components::component_base<test_server>
{
    hpx::id_type call() const
    {
        return hpx::find_here();
    }

    HPX_DEFINE_COMPONENT_ACTION(test_server, call)
};

using server_type = hpx::components::component<test_server>;
HPX_REGISTER_COMPONENT(server_type, test_server)

using call_action = test_server::call_action;
HPX_REGISTER_ACTION(call_action)

hpx::id_type get_locality()
{
    return hpx::find_here();
}

HPX_PLAIN_ACTION(get_locality, get_locality_action)

///////////////////////////////////////////////////////////////////////////////
// direct-locality routing: create() and async() should target the given
// locality id directly (no colocation hop)
void test_direct_locality_routing()
{
    std::vector<hpx::id_type> const localities = hpx::find_remote_localities();
    hpx::id_type const& target_locality =
        localities.empty() ? hpx::find_here() : localities[0];

    auto const policy = hpx::components::colocated(target_locality);

    hpx::id_type obj_id = policy.create<test_server>().get();
    HPX_TEST_EQ(hpx::async<call_action>(obj_id).get(), target_locality);

    auto const obj_policy = hpx::components::colocated(obj_id);

    HPX_TEST_EQ(obj_policy.async<get_locality_action>(hpx::launch::async).get(),
        target_locality);
}

////////////////////////////////////////////////////////////////////////////////
// bulk_create regression test for the locality-id continuation bug: when id_ is
// itself a locality id, the reported bulk_locality_result.first must be that
// same locality id, not a default-constructed id_type
void test_bulk_create_reports_locality_id()
{
    std::vector<hpx::id_type> const localities = hpx::find_remote_localities();
    hpx::id_type const& target_locality =
        localities.empty() ? hpx::find_here() : localities[0];

    auto const policy = hpx::components::colocated(target_locality);

    auto const results = policy.bulk_create<false, test_server>(3).get();

    HPX_TEST_EQ(results.size(), static_cast<std::size_t>(1));
    if (!results.empty())
    {
        HPX_TEST(static_cast<bool>(results[0].first));
        HPX_TEST_EQ(results[0].first, target_locality);
        HPX_TEST_EQ(results[0].second.size(), static_cast<std::size_t>(3));

        for (hpx::id_type const& id : results[0].second)
        {
            HPX_TEST_EQ(hpx::async<call_action>(id).get(), target_locality);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// empty policy (default-constructed) falls back to the local locality
void test_empty_policy_uses_local_locality()
{
    hpx::components::colocating_distribution_policy const policy;

    hpx::id_type obj_id = policy.create<test_server>().get();
    HPX_TEST_EQ(hpx::async<call_action>(obj_id).get(), hpx::find_here());
}

///////////////////////////////////////////////////////////////////////////////
// colocation with a non-migratable component's id still resolves to that
// component's owning locality directly (not through create_colocated)
void test_colocated_with_object_id()
{
    hpx::id_type anchor = hpx::new_<test_server>(hpx::find_here()).get();
    hpx::id_type const anchor_locality = hpx::async<call_action>(anchor).get();

    auto const policy = hpx::components::colocated(anchor);

    hpx::id_type obj_id = policy.create<test_server>().get();
    HPX_TEST_EQ(hpx::async<call_action>(obj_id).get(), anchor_locality);
}

///////////////////////////////////////////////////////////////////////////////
int main()
{
    test_empty_policy_uses_local_locality();
    test_direct_locality_routing();
    test_bulk_create_reports_locality_id();
    test_colocated_with_object_id();

    return hpx::util::report_errors();
}

#endif
