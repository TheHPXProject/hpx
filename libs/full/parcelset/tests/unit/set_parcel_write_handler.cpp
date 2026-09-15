//  Copyright (c) 2015-2021 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx.hpp>
#include <hpx/hpx_main.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <cstddef>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
void test(hpx::id_type const& id)
{
    hpx::trigger_lco_event(id);
}
HPX_PLAIN_ACTION(test)    // defines test_action

///////////////////////////////////////////////////////////////////////////////
std::atomic<std::size_t> write_handler_called(0);

bool is_test_action(hpx::parcelset::parcel const& p)
{
#if defined(HPX_HAVE_NETWORKING)
    return std::string(p.get_action_name()) == std::string("test_action");
#else
    return true;
#endif
}

bool write_handler(std::error_code const&, hpx::parcelset::parcel const& p)
{
    if (is_test_action(p))
        ++write_handler_called;
    return true;
}

// working around non-copy-ability of packaged_task
struct indirect_packaged_task
{
    using packaged_task_type = hpx::packaged_task<void()>;

    indirect_packaged_task()
      : pt(std::make_shared<packaged_task_type>([]() {}))
    {
    }

    auto get_future()
    {
        return pt->get_future();
    }

    template <typename... Ts>
    void operator()(Ts&&...)
    {
        // This needs to be run on a HPX thread
        hpx::post(std::move(*pt));
    }

    std::shared_ptr<packaged_task_type> pt;
};

///////////////////////////////////////////////////////////////////////////////
int main()
{
    hpx::parcel_write_handler_type wh(&write_handler);

    // test that handler is called for every parcel
    hpx::parcel_write_handler_type f1 = hpx::set_parcel_write_handler(wh);
    HPX_TEST(!f1.empty());

    std::vector<hpx::id_type> localities = hpx::find_remote_localities();

    {
        std::vector<hpx::future<void>> wait_for;
        for (hpx::id_type const& id : localities)
        {
            hpx::distributed::promise<void> p_remote;
            auto f_remote = p_remote.get_future();

            indirect_packaged_task p_local;
            auto f_local = p_local.get_future();

            hpx::post_cb<test_action>(id, p_local, p_remote.get_id());

            wait_for.push_back(std::move(f_remote));
            wait_for.push_back(std::move(f_local));
        }

        hpx::wait_all(wait_for);
        HPX_TEST_EQ(write_handler_called, localities.size());
    }

    // test that handler is not called anymore
    write_handler_called.store(0);
    hpx::parcel_write_handler_type f2 = hpx::set_parcel_write_handler(f1);

    {
        std::vector<hpx::future<void>> wait_for;
        for (hpx::id_type const& id : localities)
        {
            hpx::distributed::promise<void> p_remote;
            auto f_remote = p_remote.get_future();

            indirect_packaged_task p_local;
            auto f_local = p_local.get_future();

            hpx::post_cb<test_action>(id, p_local, p_remote.get_id());

            wait_for.push_back(std::move(f_remote));
            wait_for.push_back(std::move(f_local));
        }

        hpx::wait_all(wait_for);
        HPX_TEST_EQ(write_handler_called, std::size_t(0));
    }

    HPX_TEST(f2.target<hpx::parcel_write_handler_type>() ==
        wh.target<hpx::parcel_write_handler_type>());

    return hpx::util::report_errors();
}
#endif
