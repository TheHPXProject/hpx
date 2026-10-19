//  Copyright (c) 2026 adhithyaragavan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Invoke actions created by hpx::actions::lambda_to_action on all localities

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/async.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/testing.hpp>

#include <cstdint>
#include <string>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
static auto const where =
    hpx::actions::lambda_to_action([](std::int32_t i) -> hpx::id_type {
        HPX_TEST_EQ(i, std::int32_t(42));
        return hpx::find_here();
    });

static auto const describe = hpx::actions::lambda_to_action(
    [](std::string s, std::vector<int> v) -> std::string {
        return s + ":" + std::to_string(v.size());
    });

static auto const nothing = hpx::actions::lambda_to_action([]() {});

///////////////////////////////////////////////////////////////////////////////
void test_async_lambda(hpx::id_type const& target)
{
    {
        hpx::future<hpx::id_type> f = hpx::async(where, target, 42);
        HPX_TEST_EQ(f.get(), target);
    }
    {
        hpx::future<hpx::id_type> f =
            hpx::async(hpx::launch::async, where, target, 42);
        HPX_TEST_EQ(f.get(), target);
    }
    {
        hpx::future<std::string> f = hpx::async(
            describe, target, std::string("abc"), std::vector<int>(3, 1));
        HPX_TEST_EQ(f.get(), std::string("abc:3"));
    }
    {
        hpx::future<void> f = hpx::async(nothing, target);
        f.get();
    }
}

int hpx_main()
{
    for (hpx::id_type const& id : hpx::find_all_localities())
    {
        test_async_lambda(id);
    }
    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(
        hpx::init(argc, argv), 0, "HPX main exited with non-zero status");
    return hpx::util::report_errors();
}

#endif    // HPX_COMPUTE_DEVICE_CODE
