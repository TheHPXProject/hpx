//  Copyright (c) 2026 Animesh Srivastava
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Resetting the same latch from the handler detects reporting under its lock.

#include <hpx/contracts.hpp>
#include <hpx/init.hpp>
#include <hpx/latch.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <string>

namespace {

    std::atomic<int> handler_call_count = 0;
    std::string last_condition;

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
    }
}    // namespace

int hpx_main()
{
    hpx::contracts::set_violation_handler(recording_handler);

    {
        hpx::lcos::local::latch l(2);
        l.count_down(1);
        l.count_down(1);
        HPX_TEST(l.is_ready());
        HPX_TEST_EQ(handler_call_count.load(), 0);
    }

    {
        hpx::lcos::local::latch l(1);
        lock_probe = &l;
        l.count_down(2);
        lock_probe = nullptr;

        HPX_TEST_EQ(handler_call_count.load(), 1);
        HPX_TEST_EQ(last_condition, std::string("old_count >= update"));
    }

    {
        hpx::lcos::local::latch l(1);
        lock_probe = &l;
        l.arrive_and_wait(2);
        lock_probe = nullptr;

        HPX_TEST_EQ(handler_call_count.load(), 2);
        HPX_TEST_EQ(last_condition, std::string("old_count >= update"));
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
