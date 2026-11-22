//  Copyright (c) 2026 Anshuman Agrawal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// A save that needs the preprocessing pass (a managed id_type, a future or
// task_group that is not ready) marks an archive that has
// detail::optimistic_save and saves nothing, with no side effect. Everything
// else saves as usual.

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/experimental/task_group.hpp>
#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/include/components.hpp>
#include <hpx/modules/lcos_local.hpp>
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
// Save t into an optimistic archive. Returns true if a save asked for the
// preprocessing pass. The preprocessing archive has the side effects that the
// skipped saves must not do (splitting credits, running deferred futures).
template <typename T>
bool defers_in(hpx::serialization::output_archive& ar, T& t)
{
    auto& optimistic =
        ar.get_extra_data<hpx::serialization::detail::optimistic_save>();
    ar << t;
    ar.flush();
    if (optimistic.needs_await)
    {
        // nothing waits for a future or splits a credit
        using hpx::serialization::detail::preprocess_futures;
        using hpx::serialization::detail::preprocess_gid_types;
        HPX_TEST(ar.try_get_extra_data<preprocess_futures>() == nullptr);
        HPX_TEST(ar.try_get_extra_data<preprocess_gid_types>() == nullptr);
    }
    return optimistic.needs_await;
}

template <typename T>
bool defers(T& t, std::vector<char>* out = nullptr)
{
    std::vector<char> buffer;
    hpx::serialization::output_archive ar(buffer);
    bool const asked = defers_in(ar, t);
    if (out != nullptr)
    {
        *out = HPX_MOVE(buffer);
    }
    return asked;
}

template <typename T>
bool defers_preprocessing(T& t)
{
    hpx::serialization::detail::preprocess_container data;
    hpx::serialization::output_archive ar(data);
    return defers_in(ar, t);
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
    // the marker is not saved, so it does not change the bytes
    std::vector<double> data{1.0, 2.0, 3.0};
    std::vector<char> optimistic;
    HPX_TEST(!defers(data, &optimistic));
    HPX_TEST(optimistic == save(data));

    HPX_TEST(!defers_preprocessing(data));

    std::string text("no futures here");
    HPX_TEST(!defers(text));
    HPX_TEST(!defers_preprocessing(text));
}

void test_futures()
{
    hpx::future<int> ready = hpx::make_ready_future(42);
    HPX_TEST(!defers(ready));
    HPX_TEST(!defers_preprocessing(ready));

    hpx::promise<int> p;
    hpx::future<int> pending = p.get_future();
    HPX_TEST(defers(pending));
    HPX_TEST(defers_preprocessing(pending));

    // the skipped save left the future as it was
    HPX_TEST(pending.valid());
    HPX_TEST(!pending.is_ready());
    p.set_value(43);
    HPX_TEST_EQ(pending.get(), 43);

    // a pending future after some data that is saved already
    hpx::promise<int> q;
    std::vector<hpx::future<int>> futures;
    futures.push_back(hpx::make_ready_future(1));
    futures.push_back(q.get_future());
    HPX_TEST(defers(futures));
    q.set_value(2);
    HPX_TEST(!defers(futures));

    // the preprocessing pass runs a deferred future, the skipped save does not
    bool ran = false;
    hpx::future<int> deferred = hpx::async(hpx::launch::deferred, [&ran]() {
        ran = true;
        return 44;
    });
    HPX_TEST(defers_preprocessing(deferred));
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
    HPX_TEST(defers(tg));
    HPX_TEST(defers_preprocessing(tg));

    p.set_value();
    tg.wait();
    HPX_TEST(!defers(tg));
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
    HPX_TEST(!defers(here));
    HPX_TEST(!defers_preprocessing(here));

    hpx::id_type managed = hpx::new_<test_server>(hpx::find_here()).get();
    HPX_TEST(managed.get_management_type() ==
        hpx::id_type::management_type::managed);

    // the preprocessing pass splits the credit, the skipped save does not
    std::int64_t const credit = get_credit(managed);
    HPX_TEST(credit != 0);
    HPX_TEST(defers(managed));
    HPX_TEST(defers_preprocessing(managed));
    HPX_TEST_EQ(get_credit(managed), credit);

    // the real pass moves all credits of this id, the skipped save does not
    hpx::id_type move_credit(
        hpx::naming::detail::split_credits_for_gid(managed.get_gid()),
        hpx::id_type::management_type::managed_move_credit);
    std::int64_t const moved = get_credit(move_credit);
    HPX_TEST(moved != 0);
    HPX_TEST(defers(move_credit));
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
