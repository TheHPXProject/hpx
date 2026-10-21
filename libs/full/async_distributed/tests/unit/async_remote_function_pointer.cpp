//  Copyright (c) 2026 adhithyaragavan
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Invoke free functions and member functions of components on all localities
// using hpx::async<&func>, without defining or registering actions for them

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)

#include <hpx/hpx_init.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/async.hpp>
#include <hpx/include/components.hpp>
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
// the component still has to be registered, its actions don't
struct counter_server : hpx::components::managed_component_base<counter_server>
{
    // non-const
    std::int32_t add(std::int32_t i)
    {
        value_ += i;
        return value_;
    }

    // non-const noexcept
    std::int32_t reset() noexcept
    {
        std::int32_t const old_value = value_;
        value_ = 0;
        return old_value;
    }

    // const
    std::int32_t get() const
    {
        return value_;
    }

    // const noexcept
    hpx::id_type where() const noexcept
    {
        return hpx::find_here();
    }

private:
    std::int32_t value_ = 0;
};

using counter_server_type = hpx::components::managed_component<counter_server>;
HPX_REGISTER_COMPONENT(counter_server_type, counter_server)

using counter_client = hpx::components::client<counter_server>;

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

void test_async_member_function_pointer(hpx::id_type const& target)
{
    // id_type of the component
    {
        hpx::id_type const counter =
            hpx::components::new_<counter_server>(target).get();

        HPX_TEST_EQ(hpx::async<&counter_server::add>(counter, 2).get(),
            std::int32_t(2));
        HPX_TEST_EQ(
            hpx::async<&counter_server::add>(hpx::launch::async, counter, 3)
                .get(),
            std::int32_t(5));
        HPX_TEST_EQ(
            hpx::async<&counter_server::get>(counter).get(), std::int32_t(5));
        HPX_TEST_EQ(hpx::async<&counter_server::where>(counter).get(), target);

        HPX_TEST_EQ(
            hpx::async<&counter_server::reset>(counter).get(), std::int32_t(5));
        HPX_TEST_EQ(
            hpx::async<&counter_server::get>(counter).get(), std::int32_t(0));
    }

    // client of the component
    {
        counter_client const counter =
            hpx::components::new_<counter_client>(target);

        HPX_TEST_EQ(hpx::async<&counter_server::add>(counter, 7).get(),
            std::int32_t(7));
        HPX_TEST_EQ(
            hpx::async<&counter_server::get>(counter).get(), std::int32_t(7));
        HPX_TEST_EQ(hpx::async<&counter_server::where>(counter).get(), target);

        HPX_TEST_EQ(
            hpx::async<&counter_server::reset>(counter).get(), std::int32_t(7));
        HPX_TEST_EQ(
            hpx::async<&counter_server::get>(counter).get(), std::int32_t(0));
    }
}

int hpx_main()
{
    for (hpx::id_type const& id : hpx::find_all_localities())
    {
        test_async_function_pointer(id);
        test_async_member_function_pointer(id);
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
