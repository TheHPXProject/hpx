//  Copyright (c) 2026 adhithyaragavan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Invoke free functions on all localities using hpx::async<&func>, without
// defining or registering actions for them

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/async.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstdint>
#include <string>

///////////////////////////////////////////////////////////////////////////////
hpx::id_type where(std::int32_t i)
{
    HPX_TEST_EQ(i, std::int32_t(42));
    return hpx::find_here();
}

hpx::id_type where_noexcept() noexcept
{
    return hpx::find_here();
}

std::string repeat(std::string s, int count)
{
    std::string result;
    for (int i = 0; i != count; ++i)
    {
        result += s;
    }
    return result;
}

std::atomic<int> called_nothing(0);

void nothing()
{
    ++called_nothing;
}

///////////////////////////////////////////////////////////////////////////////
void test_async_function_pointer(hpx::id_type const& target)
{
    {
        hpx::future<hpx::id_type> f = hpx::async<&where>(target, 42);
        HPX_TEST_EQ(f.get(), target);
    }
    {
        hpx::future<hpx::id_type> f =
            hpx::async<&where>(hpx::launch::async, target, 42);
        HPX_TEST_EQ(f.get(), target);
    }
    {
        hpx::future<hpx::id_type> f = hpx::async<&where_noexcept>(target);
        HPX_TEST_EQ(f.get(), target);
    }
    {
        hpx::future<std::string> f =
            hpx::async<&repeat>(target, std::string("ab"), 3);
        HPX_TEST_EQ(f.get(), std::string("ababab"));
    }
    {
        hpx::future<void> f = hpx::async<&nothing>(target);
        f.get();
    }
}

int hpx_main()
{
    for (hpx::id_type const& id : hpx::find_all_localities())
    {
        test_async_function_pointer(id);
    }

    // nothing() ran here only for the local target
    HPX_TEST_EQ(called_nothing.load(), 1);

    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(
        hpx::init(argc, argv), 0, "HPX main exited with non-zero status");
    return hpx::util::report_errors();
}

#endif    // HPX_COMPUTE_DEVICE_CODE
