//  Copyright (c) 2007-2015 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_init.hpp>
#include <hpx/include/async.hpp>
#include <hpx/include/components.hpp>
#include <hpx/include/lcos.hpp>
#include <hpx/include/post.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
std::int32_t increment(std::int32_t i)
{
    return i + 1;
}
HPX_PLAIN_ACTION(increment)    // defines increment_action

std::int32_t increment_with_future(hpx::shared_future<std::int32_t> fi)
{
    return fi.get() + 1;
}
HPX_PLAIN_ACTION(increment_with_future)

///////////////////////////////////////////////////////////////////////////////
std::int32_t mult2(std::int32_t i)
{
    return i * 2;
}
HPX_PLAIN_ACTION(mult2)    // defines mult2_action

///////////////////////////////////////////////////////////////////////////////
struct test_server : hpx::components::component_base<test_server>
{
};

typedef hpx::components::component<test_server> test_server_type;
HPX_REGISTER_COMPONENT(test_server_type, test_server)

struct test_client : hpx::components::client_base<test_client, test_server>
{
    typedef hpx::components::client_base<test_client, test_server> base_type;

    test_client(hpx::id_type const& id)
      : base_type(id)
    {
    }
    test_client(hpx::future<hpx::id_type>&& id)
      : base_type(std::move(id))
    {
    }
};

///////////////////////////////////////////////////////////////////////////////
void test_async_continue_colocated(test_client const& target)
{
    using hpx::make_continuation;

    increment_action inc;
    increment_with_future_action inc_f;
    mult2_action mult;

    // test locally, fully equivalent to plain hpx::async
    {
        hpx::future<int> f1 = hpx::async_continue(
            inc, make_continuation(), hpx::colocated(target), 42);
        HPX_TEST_EQ(f1.get(), 43);

        hpx::distributed::promise<std::int32_t> p;
        hpx::shared_future<std::int32_t> f = p.get_future();

        hpx::future<int> f2 = hpx::async_continue(
            inc_f, make_continuation(), hpx::colocated(target), f);

        p.set_value(42);
        HPX_TEST_EQ(f2.get(), 43);
    }

    {
        hpx::future<int> f1 = hpx::async_continue(
            inc, make_continuation(), hpx::colocated(target), 42);
        HPX_TEST_EQ(f1.get(), 43);

        hpx::distributed::promise<std::int32_t> p;
        hpx::shared_future<std::int32_t> f = p.get_future();

        hpx::future<int> f2 = hpx::async_continue(
            inc_f, make_continuation(), hpx::colocated(target), f);

        p.set_value(42);
        HPX_TEST_EQ(f2.get(), 43);
    }

    // test chaining locally
    {
        hpx::future<int> f = hpx::async_continue(
            inc, make_continuation(mult), hpx::colocated(target), 42);
        HPX_TEST_EQ(f.get(), 86);

        f = hpx::async_continue(inc,
            make_continuation(mult, make_continuation()),
            hpx::colocated(target), 42);
        HPX_TEST_EQ(f.get(), 86);

        f = hpx::async_continue(inc,
            make_continuation(mult, make_continuation(inc)),
            hpx::colocated(target), 42);
        HPX_TEST_EQ(f.get(), 87);

        f = hpx::async_continue(inc,
            make_continuation(
                mult, make_continuation(inc, make_continuation())),
            hpx::colocated(target), 42);
        HPX_TEST_EQ(f.get(), 87);
    }

    {
        hpx::future<int> f = hpx::async_continue(inc,
            make_continuation(mult, make_continuation()),
            hpx::colocated(target), 42);
        HPX_TEST_EQ(f.get(), 86);
    }
}

int hpx_main()
{
    std::vector<hpx::id_type> localities = hpx::find_all_localities();
    for (hpx::id_type const& id : localities)
    {
        test_client client(hpx::new_<test_client>(id));
        test_async_continue_colocated(client);
    }
    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    // Initialize and run HPX
    HPX_TEST_EQ_MSG(
        hpx::init(argc, argv), 0, "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
#endif
