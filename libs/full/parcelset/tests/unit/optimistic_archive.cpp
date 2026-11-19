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
#include <hpx/include/components.hpp>
#include <hpx/modules/naming.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
// A component, to get an id_type that is managed
struct test_server : hpx::components::component_base<test_server>
{
};

using test_server_type = hpx::components::component<test_server>;
HPX_REGISTER_COMPONENT(test_server_type, test_server)

///////////////////////////////////////////////////////////////////////////////
// Save t into an optimistic archive. Returns true if the archive stopped. The
// preprocessing archive has the side effects the stop must come before
// (splitting credits, running deferred futures).
template <typename T>
bool stops_in(hpx::serialization::output_archive& ar, T& t)
{
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
    return false;
}

template <typename T>
bool stops(T& t, std::vector<char>* out = nullptr)
{
    std::vector<char> buffer;
    hpx::serialization::output_archive ar(buffer);
    bool const stopped = stops_in(ar, t);
    if (out != nullptr)
    {
        *out = HPX_MOVE(buffer);
    }
    return stopped;
}

template <typename T>
bool stops_preprocessing(T& t)
{
    hpx::serialization::detail::preprocess_container data;
    hpx::serialization::output_archive ar(data);
    return stops_in(ar, t);
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

    HPX_TEST(!stops_preprocessing(data));

    std::string text("no futures here");
    HPX_TEST(!stops(text));
    HPX_TEST(!stops_preprocessing(text));
}

void test_futures()
{
    hpx::future<int> ready = hpx::make_ready_future(42);
    HPX_TEST(!stops(ready));
    HPX_TEST(!stops_preprocessing(ready));

    hpx::promise<int> p;
    hpx::future<int> pending = p.get_future();
    HPX_TEST(stops(pending));
    HPX_TEST(stops_preprocessing(pending));

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

    // the preprocessing pass runs a deferred future, the stop comes first
    bool ran = false;
    hpx::future<int> deferred = hpx::async(hpx::launch::deferred, [&ran]() {
        ran = true;
        return 44;
    });
    HPX_TEST(stops_preprocessing(deferred));
    HPX_TEST(!ran);
    HPX_TEST_EQ(deferred.get(), 44);
    HPX_TEST(ran);
}

void test_task_group()
{
    hpx::promise<void> p;
    hpx::shared_future<void> release = p.get_future().share();

    hpx::experimental::task_group tg;
    tg.run([release]() { release.get(); });
    HPX_TEST(stops(tg));
    HPX_TEST(stops_preprocessing(tg));

    p.set_value();
    tg.wait();
    HPX_TEST(!stops(tg));
}

std::int64_t get_credit(hpx::id_type const& id)
{
    return hpx::naming::detail::get_credit_from_gid(id.get_gid());
}

void test_ids()
{
    hpx::id_type here = hpx::find_here();
    HPX_TEST(
        here.get_management_type() == hpx::id_type::management_type::unmanaged);
    HPX_TEST(!stops(here));
    HPX_TEST(!stops_preprocessing(here));

    hpx::id_type managed = hpx::new_<test_server>(hpx::find_here()).get();
    HPX_TEST(managed.get_management_type() ==
        hpx::id_type::management_type::managed);

    // the preprocessing pass splits the credit, the stop comes first
    std::int64_t const credit = get_credit(managed);
    HPX_TEST(credit != 0);
    HPX_TEST(stops(managed));
    HPX_TEST(stops_preprocessing(managed));
    HPX_TEST_EQ(get_credit(managed), credit);

    // the real pass moves all credits of this id, the stop comes first
    hpx::id_type move_credit(
        hpx::naming::detail::split_credits_for_gid(managed.get_gid()),
        hpx::id_type::management_type::managed_move_credit);
    std::int64_t const moved = get_credit(move_credit);
    HPX_TEST(moved != 0);
    HPX_TEST(stops(move_credit));
    HPX_TEST_EQ(get_credit(move_credit), moved);
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
