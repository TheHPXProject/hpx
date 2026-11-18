//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Creating a component on another locality from a CUDA (or HIP) translation
// unit: runtime_support::create_component was stubbed out whenever nvcc
// compiled it, also in the host compilation pass, so the target locality
// returned an invalid id instead of creating the component.

#include <hpx/config.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/testing.hpp>

#include <cstdint>
#include <vector>

// makes this a translation unit with device code
__global__ void kernel() {}

struct test_server : hpx::components::component_base<test_server>
{
    test_server() = default;
    explicit test_server(std::uint32_t value)
      : value_(value)
    {
    }

    std::uint32_t get_value()
    {
        return value_;
    }

    std::uint32_t get_locality()
    {
        return hpx::get_locality_id();
    }

    HPX_DEFINE_COMPONENT_ACTION(test_server, get_value)
    HPX_DEFINE_COMPONENT_ACTION(test_server, get_locality)

    std::uint32_t value_ = 0;
};

using test_server_type = hpx::components::component<test_server>;
HPX_REGISTER_COMPONENT(test_server_type, test_server)
HPX_REGISTER_ACTION(test_server::get_value_action, test_server_get_value_action)
HPX_REGISTER_ACTION(
    test_server::get_locality_action, test_server_get_locality_action)

int hpx_main()
{
    std::vector<hpx::id_type> const remotes = hpx::find_remote_localities();
    HPX_TEST(!remotes.empty());

    for (hpx::id_type const& where : remotes)
    {
        // default-constructed and with an argument
        hpx::id_type const plain = hpx::new_<test_server>(where).get();
        hpx::id_type const with_value = hpx::new_<test_server>(where, 42).get();

        HPX_TEST(plain);
        HPX_TEST(with_value);
        if (!plain || !with_value)
        {
            continue;
        }

        std::uint32_t const locality =
            hpx::naming::get_locality_id_from_id(where);
        HPX_TEST_EQ(hpx::async<test_server::get_locality_action>(plain).get(),
            locality);
        HPX_TEST_EQ(hpx::async<test_server::get_value_action>(with_value).get(),
            std::uint32_t(42));
    }

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(
        hpx::init(argc, argv), 0, "HPX main exited with non-zero status");
    return hpx::util::report_errors();
}
