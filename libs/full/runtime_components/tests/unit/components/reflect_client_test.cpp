//  Copyright (c) 2026 Priyanshi Sharma
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE) && defined(HPX_HAVE_CXX26_REFLECTION)
#include <hpx/hpx_main.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/runtime_components.hpp>
#include <hpx/modules/testing.hpp>
#include <cstdint>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// Server: a simple component with two public member functions
struct reflect_test_server
  : hpx::components::component_base<reflect_test_server>
{
    /// Returns the locality this component lives on.
    hpx::id_type identity() const
    {
        return hpx::find_here();
    }

    /// Returns i + 1 and verifies the argument.
    std::int32_t increment(std::int32_t i) const
    {
        HPX_TEST(i >= 0);
        return i + 1;
    }
};

using reflect_test_server_type =
    hpx::components::component<reflect_test_server>;
HPX_REGISTER_COMPONENT(reflect_test_server_type, reflect_test_server)

///////////////////////////////////////////////////////////////////////////////
// Server with constructor arguments
struct reflect_test_server2
  : hpx::components::component_base<reflect_test_server2>
{
    explicit reflect_test_server2(std::int32_t base)
      : base_(base)
    {
    }

    std::int32_t increment(std::int32_t i) const
    {
        return base_ + i;
    }

    std::int32_t base_;
};

using reflect_test_server2_type =
    hpx::components::component<reflect_test_server2>;
HPX_REGISTER_COMPONENT(reflect_test_server2_type, reflect_test_server2)

HPX_CLIENT(reflect_test_server2)

///////////////////////////////////////////////////////////////////////////////
// Generate client — single argument, no client name needed.
// reflect_client<reflect_test_server> is the generated client type.
HPX_CLIENT(reflect_test_server)

///////////////////////////////////////////////////////////////////////////////
int main()
{
    std::vector<hpx::id_type> localities = hpx::find_all_localities();

    for (hpx::id_type const& loc : localities)
    {
        // Instantiate component on locality and get client in one call
        auto c = hpx::components::make_client<reflect_test_server>(loc);

        // Test identity() — verifies action ran on expected locality
        hpx::future<hpx::id_type> f1 = c.identity();
        HPX_TEST_EQ(f1.get(), loc);

        // Test increment() — verifies argument passing and return value
        hpx::future<std::int32_t> f2 = c.increment(41);
        HPX_TEST_EQ(f2.get(), std::int32_t(42));
    }

    // Test make_client with constructor arguments
    for (hpx::id_type const& loc : localities)
    {
        // Pass constructor argument (base = 10) to make_client
        auto c2 = hpx::components::make_client<reflect_test_server2>(
            loc, std::int32_t(10));
        hpx::future<std::int32_t> f3 = c2.increment(32);
        HPX_TEST_EQ(f3.get(), std::int32_t(42));
    }

    return hpx::util::report_errors();
}
#endif    // !HPX_COMPUTE_DEVICE_CODE && HPX_HAVE_CXX26_REFLECTION
