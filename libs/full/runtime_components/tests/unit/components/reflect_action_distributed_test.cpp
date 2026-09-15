//  Copyright (c) 2026 Priyanshi Sharma
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file reflect_action_distributed_test.cpp
/// \brief Integration test: reflect_action<^^func> and hpx::async<^^func>
///        dispatched across multiple HPX localities.

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE) && defined(HPX_HAVE_CXX26_REFLECTION)
#include <hpx/hpx_main.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/async_distributed.hpp>
#include <hpx/modules/testing.hpp>

#include <cstdint>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
namespace reflect_test {

    // Returns the locality this function executes on -- used to verify
    // the action actually ran on the target locality, not the caller.
    hpx::id_type identity()
    {
        return hpx::find_here();
    }

    std::int32_t increment(std::int32_t i)
    {
        return i + 1;
    }

    std::int32_t add(std::int32_t x, std::int32_t y)
    {
        return x + y;
    }

    // Verify serialization of std::string across localities
    std::string echo(std::string const& s)
    {
        return s;
    }

    // Void return -- verifies that reflect_action handles void correctly
    void fire_and_forget(std::int32_t) noexcept {}

}    // namespace reflect_test

///////////////////////////////////////////////////////////////////////////////
int main()
{
    std::vector<hpx::id_type> localities = hpx::find_all_localities();

    for (hpx::id_type const& loc : localities)
    {
        // -- Verify action actually executes on the target locality --
        // Returns hpx::find_here() from the target and compares against
        // the target id_type on the invoking side.
        {
            hpx::future<hpx::id_type> f =
                hpx::async<^^reflect_test::identity>(loc);
            HPX_TEST_EQ(f.get(), loc);
        }

        // -- reflect_action<^^func> as explicit action type --
        // Mirrors HPX_PLAIN_ACTION but requires no macro and no
        // HPX_REGISTER_ACTION call.
        {
            using inc_action =
                hpx::actions::reflect_action<^^reflect_test::increment>;
            hpx::future<std::int32_t> f =
                hpx::async<inc_action>(loc, std::int32_t(41));
            HPX_TEST_EQ(f.get(), std::int32_t(42));
        }

        // -- hpx::async<^^func> direct API (PR #7376) --
        // No action type defined at all.
        {
            hpx::future<std::int32_t> f =
                hpx::async<^^reflect_test::increment>(loc, std::int32_t(41));
            HPX_TEST_EQ(f.get(), std::int32_t(42));
        }

        // -- Multi-argument remote invocation --
        {
            hpx::future<std::int32_t> f = hpx::async<^^reflect_test::add>(
                loc, std::int32_t(20), std::int32_t(22));
            HPX_TEST_EQ(f.get(), std::int32_t(42));
        }

        // -- Serialization round-trip: std::string across localities --
        {
            std::string const msg = "hello from reflect_action";
            hpx::future<std::string> f =
                hpx::async<^^reflect_test::echo>(loc, msg);
            HPX_TEST_EQ(f.get(), msg);
        }

        // -- Void return type --
        {
            hpx::future<void> f = hpx::async<^^reflect_test::fire_and_forget>(
                loc, std::int32_t(0));
            f.get();    // must not throw
        }
    }

    return hpx::util::report_errors();
}
#endif    // !HPX_COMPUTE_DEVICE_CODE && HPX_HAVE_CXX26_REFLECTION
