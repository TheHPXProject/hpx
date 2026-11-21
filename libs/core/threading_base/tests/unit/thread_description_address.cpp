//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// With HPX_HAVE_THREAD_DESCRIPTION_FULL an unnamed task is described by the
// address of its function. That address must never be read as a name: not
// by the launch policies, which pass the description on, and not by the
// Tracy backend when it names the task's zones.

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/latch.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>

#include <cstddef>
#include <string>

#if defined(HPX_HAVE_THREAD_DESCRIPTION_FULL)
namespace {

    using hpx::threads::thread_description;

    thread_description current_description()
    {
        return hpx::threads::get_thread_description(
            hpx::threads::get_self_id());
    }

    void plain_function()
    {
        // an unnamed function run on the calling task names that task
        // <unknown> for the call, instead of using its address as a name
        thread_description const desc = current_description();
        HPX_TEST(desc.kind() == thread_description::data_type::description);
        if (desc.kind() == thread_description::data_type::description)
        {
            HPX_TEST_EQ(
                std::string(desc.get_description()), std::string("<unknown>"));
        }
    }

    void plain_function_on_new_task()
    {
        // a new task created for an unnamed function is described by that
        // function's address
        thread_description const desc = current_description();
        HPX_TEST(desc.kind() == thread_description::data_type::address);
        if (desc.kind() == thread_description::data_type::address)
        {
            HPX_TEST_EQ(desc.get_address(),
                reinterpret_cast<std::size_t>(&plain_function_on_new_task));
        }
    }

    void test_launch_policies()
    {
        hpx::async(&plain_function_on_new_task).get();
        hpx::async(hpx::launch::async, &plain_function_on_new_task).get();
        hpx::async(hpx::launch::fork, &plain_function_on_new_task).get();

        // these run the function on the calling task
        hpx::async(hpx::launch::sync, &plain_function).get();
        hpx::async(hpx::launch::deferred, &plain_function).get();

        auto lambda = [] {
            HPX_TEST(current_description().kind() ==
                thread_description::data_type::address);
        };
        hpx::async(lambda).get();
        hpx::async(hpx::launch::fork, lambda).get();
    }

#if defined(HPX_HAVE_TRACY)
    void test_tracy_region_names()
    {
        // hpx::post gives an unnamed lambda an address description
        hpx::latch l(2);
        hpx::post([&l] {
            auto const* thrd = hpx::threads::get_self_id_data();
            HPX_TEST(thrd->get_description().kind() ==
                thread_description::data_type::address);

            HPX_TEST_EQ(
                std::string(hpx::threads::get_region_init_data(thrd).name),
                std::string("<unknown>"));
            HPX_TEST_EQ(
                std::string(
                    hpx::threads::get_fiber_region_init_data(thrd).name),
                std::string("<unknown>"));

            l.count_down(1);
        });
        l.arrive_and_wait();
    }
#endif
}    // namespace
#endif

int hpx_main()
{
#if defined(HPX_HAVE_THREAD_DESCRIPTION_FULL)
    test_launch_policies();
#if defined(HPX_HAVE_TRACY)
    test_tracy_region_names();
#endif
#endif
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
