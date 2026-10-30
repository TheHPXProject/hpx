//  Copyright (c) 2026 Hartmut Kaiser
//  Copyright (c) 2026 Shivansh Singh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Regression test for dual-ownership race in shared_mutex.
//
// An external thread calling try_lock() or lock() while an upgrade lock
// was being promoted via unlock_upgrade_and_lock() could erroneously acquire
// the exclusive lock concurrently.

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/synchronization.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>

#include <atomic>
#include <vector>

constexpr unsigned iterations = 5000;

void test_shared_mutex_dual_ownership()
{
    hpx::shared_mutex mtx;
    std::atomic<int> owners{0};
    std::vector<hpx::future<void>> tasks;

    // Task Group A: lock_upgrade -> unlock_upgrade_and_lock -> unlock
    for (int i = 0; i < 2; ++i)
    {
        tasks.push_back(hpx::async(hpx::launch::async, [&mtx, &owners]() {
            for (unsigned j = 0; j < iterations; ++j)
            {
                mtx.lock_upgrade();
                mtx.unlock_upgrade_and_lock();

                HPX_TEST_EQ(owners.fetch_add(1), 0);
                hpx::this_thread::yield();
                owners.fetch_sub(1);

                mtx.unlock();
            }
        }));
    }

    // Task Group B: lock -> unlock
    for (int i = 0; i < 2; ++i)
    {
        tasks.push_back(hpx::async(hpx::launch::async, [&mtx, &owners]() {
            for (unsigned j = 0; j < iterations; ++j)
            {
                mtx.lock();

                HPX_TEST_EQ(owners.fetch_add(1), 0);
                hpx::this_thread::yield();
                owners.fetch_sub(1);

                mtx.unlock();
            }
        }));
    }

    // Task Group C: try_lock -> unlock
    for (int i = 0; i < 2; ++i)
    {
        tasks.push_back(hpx::async(hpx::launch::async, [&mtx, &owners]() {
            for (unsigned j = 0; j < iterations; ++j)
            {
                if (mtx.try_lock())
                {
                    HPX_TEST_EQ(owners.fetch_add(1), 0);
                    hpx::this_thread::yield();
                    owners.fetch_sub(1);

                    mtx.unlock();
                }
                else
                {
                    hpx::this_thread::yield();
                }
            }
        }));
    }

    hpx::wait_all(tasks);
}

int hpx_main()
{
    test_shared_mutex_dual_ownership();
    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
