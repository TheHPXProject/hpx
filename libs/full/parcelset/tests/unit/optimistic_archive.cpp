//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// An optimistic output archive stops with await_required where a save needs
// the preprocessing pass (a managed id_type, a future or task_group that is
// not ready), before any side effect. Everything else saves as usual.

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/experimental/task_group.hpp>
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// Save t into an optimistic archive. Returns true if the archive stopped.
template <typename T>
bool stops(T& t, std::vector<char>* out = nullptr)
{
    std::vector<char> buffer;
    hpx::serialization::output_archive ar(buffer);
    ar.enable_optimistic();
    try
    {
        ar << t;
    }
    catch (hpx::serialization::await_required const&)
    {
        return true;
    }
    ar.flush();
    if (out != nullptr)
    {
        *out = HPX_MOVE(buffer);
    }
    return false;
}

template <typename T>
std::vector<char> save(T& t)
{
    std::vector<char> buffer;
    hpx::serialization::output_archive ar(buffer);
    ar << t;
    ar.flush();
    return buffer;
}

///////////////////////////////////////////////////////////////////////////////
void test_plain_data()
{
    // the flag is set after the constructor, so it does not change the bytes
    std::vector<double> data{1.0, 2.0, 3.0};
    std::vector<char> optimistic;
    HPX_TEST(!stops(data, &optimistic));
    HPX_TEST(optimistic == save(data));

    std::string text("no futures here");
    HPX_TEST(!stops(text));
}

void test_futures()
{
    hpx::future<int> ready = hpx::make_ready_future(42);
    HPX_TEST(!stops(ready));

    hpx::promise<int> p;
    hpx::future<int> pending = p.get_future();
    HPX_TEST(stops(pending));

    // stopping left the future as it was
    HPX_TEST(pending.valid());
    HPX_TEST(!pending.is_ready());
    p.set_value(43);
    HPX_TEST_EQ(pending.get(), 43);

    // a pending future after some data that is saved already
    hpx::promise<int> q;
    std::vector<hpx::future<int>> futures;
    futures.push_back(hpx::make_ready_future(1));
    futures.push_back(q.get_future());
    HPX_TEST(stops(futures));
    q.set_value(2);
    HPX_TEST(!stops(futures));
}

void test_task_group()
{
    hpx::promise<void> p;
    hpx::shared_future<void> release = p.get_future().share();

    hpx::experimental::task_group tg;
    tg.run([release]() { release.get(); });
    HPX_TEST(stops(tg));

    p.set_value();
    tg.wait();
    HPX_TEST(!stops(tg));
}

void test_ids()
{
    hpx::id_type here = hpx::find_here();
    HPX_TEST(
        here.get_management_type() == hpx::id_type::management_type::unmanaged);
    HPX_TEST(!stops(here));

    hpx::distributed::promise<int> p;
    hpx::future<int> f = p.get_future();
    hpx::id_type managed = p.get_id();
    HPX_TEST(managed.get_management_type() ==
        hpx::id_type::management_type::managed);

    // stopping did not split the credit of the id
    std::int64_t const credit =
        hpx::naming::detail::get_credit_from_gid(managed.get_gid());
    HPX_TEST(credit != 0);
    HPX_TEST(stops(managed));
    HPX_TEST_EQ(
        hpx::naming::detail::get_credit_from_gid(managed.get_gid()), credit);

    p.set_value(42);
    HPX_TEST_EQ(f.get(), 42);
}

int hpx_main()
{
    test_plain_data();
    test_futures();
    test_task_group();
    test_ids();
    return hpx::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::init(argc, argv), 0);
    return hpx::util::report_errors();
}
#endif
