//  Copyright (c) 2026 Animesh Srivastava
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Test: count_down(n) and arrive_and_wait(n) with n larger than the current
// count (HPX_CONTRACT_ASSERT, fallback mode only)
//
// A non-aborting violation handler must be called exactly once per violating
// call and never for a valid one. Not registered in IGNORE mode, where the
// check compiles out.

#include <hpx/contracts.hpp>
#include <hpx/init.hpp>
#include <hpx/latch.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <string>

namespace {

    std::atomic<int> handler_call_count = 0;
    std::string last_condition;

    // if set, the handler takes the latch's spinlock through reset()
    hpx::lcos::local::latch* lock_probe = nullptr;

    void recording_handler(hpx::contracts::contract_violation const& info)
    {
        ++handler_call_count;
        last_condition = info.condition();

        // deadlocks if the violation is reported while the lock is held
        if (lock_probe != nullptr)
        {
            lock_probe->reset(0);
        }

        // deliberately does not abort so the test can continue
    }
}    // namespace

int hpx_main()
{
    hpx::contracts::set_violation_handler(recording_handler);

    // valid call: no violation
    {
        hpx::lcos::local::latch l(2);
        l.count_down(1);
        l.count_down(1);
        HPX_TEST(l.is_ready());
        HPX_TEST_EQ(handler_call_count.load(), 0);
    }

    // invalid call: update (2) exceeds the current count (1)
    {
        hpx::lcos::local::latch l(1);
        l.count_down(2);

        HPX_TEST_EQ(handler_call_count.load(), 1);
        HPX_TEST_EQ(last_condition, std::string("old_count >= update"));

        // the counter is now negative; restore it so the debug-mode check in
        // the destructor (counter_ == 0) does not fire
        l.reset(0);
    }

    // same for arrive_and_wait, which must report without holding the lock
    {
        hpx::lcos::local::latch l(1);
        lock_probe = &l;
        l.arrive_and_wait(2);
        lock_probe = nullptr;

        HPX_TEST_EQ(handler_call_count.load(), 2);
        HPX_TEST_EQ(last_condition, std::string("old_count >= update"));

        l.reset(0);
    }

    hpx::contracts::set_violation_handler(nullptr);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
