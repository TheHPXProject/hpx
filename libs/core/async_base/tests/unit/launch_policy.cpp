//  Copyright (c) 2021-2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/hpx_main.hpp>
#include <hpx/modules/async_base.hpp>
#include <hpx/modules/coroutines.hpp>
#include <hpx/modules/serialization.hpp>
#include <hpx/modules/testing.hpp>

#include <cstdint>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
template <typename Launch>
void test_policy(Launch policy)
{
    HPX_TEST(policy.priority() == hpx::threads::thread_priority::default_);
    HPX_TEST(policy.stacksize() == hpx::threads::thread_stacksize::default_);
    HPX_TEST(
        policy.hint().mode == hpx::threads::thread_schedule_hint_mode::none);
    HPX_TEST_EQ(policy.hint().hint, std::int16_t(-1));

    policy.set_priority(hpx::threads::thread_priority::normal);
    HPX_TEST(policy.priority() == hpx::threads::thread_priority::normal);

    auto p = hpx::execution::experimental::with_priority(
        policy, hpx::threads::thread_priority::high);
    HPX_TEST(hpx::execution::experimental::get_priority(p) ==
        hpx::threads::thread_priority::high);

    policy.set_stacksize(hpx::threads::thread_stacksize::medium);
    HPX_TEST(policy.stacksize() == hpx::threads::thread_stacksize::medium);

    auto p1 = hpx::execution::experimental::with_stacksize(
        policy, hpx::threads::thread_stacksize::small_);
    HPX_TEST(hpx::execution::experimental::get_stacksize(p1) ==
        hpx::threads::thread_stacksize::small_);

    hpx::threads::thread_schedule_hint hint(0);
    policy.set_hint(hint);
    HPX_TEST(policy.hint().mode == hint.mode);
    HPX_TEST_EQ(policy.hint().hint, hint.hint);

    hpx::threads::thread_schedule_hint hint1(1);
    auto p2 = hpx::execution::experimental::with_hint(policy, hint1);
    HPX_TEST(hpx::execution::experimental::get_hint(p2).mode == hint1.mode);
    HPX_TEST(hpx::execution::experimental::get_hint(p2).hint == hint1.hint);
}

///////////////////////////////////////////////////////////////////////////////
// hpx::launch::task disables inline/child-mode execution, unlike
// hpx::launch::async which defaults to
// hpx::threads::default_runs_as_child_hint.
void test_task_policy_disables_child_mode()
{
    hpx::launch::task_policy t;
    HPX_TEST(t.hint().runs_as_child_mode() ==
        hpx::threads::thread_execution_hint::none);

    t.set_priority(hpx::threads::thread_priority::high);
    HPX_TEST(t.hint().runs_as_child_mode() ==
        hpx::threads::thread_execution_hint::none);

    t.set_stacksize(hpx::threads::thread_stacksize::medium);
    HPX_TEST(t.hint().runs_as_child_mode() ==
        hpx::threads::thread_execution_hint::none);

    hpx::launch::async_policy a;
    HPX_TEST(a.hint().runs_as_child_mode() ==
        hpx::threads::default_runs_as_child_hint);
}

///////////////////////////////////////////////////////////////////////////////
void test_launch_policy_bitmask()
{
    HPX_TEST_EQ(static_cast<int>(hpx::launch_policy::task), 0x04);
    HPX_TEST_EQ(static_cast<int>(hpx::launch_policy::async_policies), 0x15);
    HPX_TEST_EQ(static_cast<int>(hpx::launch_policy::sync_policies), 0x0a);

    HPX_TEST(hpx::launch::task != hpx::launch::async);

    // task is included in async_policies ...
    HPX_TEST((static_cast<int>(hpx::launch_policy::async_policies) &
                 static_cast<int>(hpx::launch_policy::task)) != 0);

    // ... but excluded from sync_policies
    HPX_TEST((static_cast<int>(hpx::launch_policy::sync_policies) &
                 static_cast<int>(hpx::launch_policy::task)) == 0);
}

///////////////////////////////////////////////////////////////////////////////
template <typename Policy>
void test_serialization(Policy const& p)
{
    std::vector<char> buffer;
    hpx::serialization::output_archive oarchive(buffer);
    oarchive << p;

    hpx::serialization::input_archive iarchive(buffer);
    Policy p1;
    iarchive >> p1;

    HPX_TEST_EQ(static_cast<int>(p1.policy()), static_cast<int>(p.policy()));
    HPX_TEST(p1.priority() == p.priority());
    HPX_TEST(p1.stacksize() == p.stacksize());
    HPX_TEST(p1.hint().mode == p.hint().mode);
    HPX_TEST_EQ(p1.hint().hint, p.hint().hint);
    HPX_TEST(p1.hint().runs_as_child_mode() == p.hint().runs_as_child_mode());
}

int main()
{
    static_assert(sizeof(hpx::launch::async_policy) <= sizeof(std::int64_t));
    static_assert(sizeof(hpx::launch::task_policy) <= sizeof(std::int64_t));
    static_assert(sizeof(hpx::launch::sync_policy) <= sizeof(std::int64_t));
    static_assert(sizeof(hpx::launch::deferred_policy) <= sizeof(std::int64_t));
    static_assert(sizeof(hpx::launch::fork_policy) <= sizeof(std::int64_t));
    static_assert(sizeof(hpx::launch::apply_policy) <= sizeof(std::int64_t));
    static_assert(sizeof(hpx::launch) <= sizeof(std::int64_t));

    test_policy(hpx::launch::async);
    test_policy(hpx::launch::task);
    test_policy(hpx::launch::sync);
    test_policy(hpx::launch::deferred);
    test_policy(hpx::launch::fork);
    test_policy(hpx::launch::apply);

    test_policy(hpx::launch());
    test_policy(hpx::launch(hpx::launch::async));
    test_policy(hpx::launch(hpx::launch::task));
    test_policy(hpx::launch(hpx::launch::sync));
    test_policy(hpx::launch(hpx::launch::deferred));
    test_policy(hpx::launch(hpx::launch::fork));
    test_policy(hpx::launch(hpx::launch::apply));

    test_task_policy_disables_child_mode();
    test_launch_policy_bitmask();

    test_serialization(hpx::launch::async_policy{});
    test_serialization(hpx::launch::task_policy{});
    test_serialization(hpx::launch::sync_policy{});
    test_serialization(hpx::launch::deferred_policy{});
    test_serialization(hpx::launch::fork_policy{});
    test_serialization(hpx::launch::apply_policy{});

    {
        hpx::launch::async_policy a;
        a.set_priority(hpx::threads::thread_priority::high);
        a.set_stacksize(hpx::threads::thread_stacksize::medium);
        a.set_hint(hpx::threads::thread_schedule_hint(3));
        test_serialization(a);

        hpx::launch::task_policy t;
        t.set_priority(hpx::threads::thread_priority::high);
        t.set_stacksize(hpx::threads::thread_stacksize::medium);
        t.set_hint(hpx::threads::thread_schedule_hint(3));
        test_serialization(t);
    }

    return 0;
}
