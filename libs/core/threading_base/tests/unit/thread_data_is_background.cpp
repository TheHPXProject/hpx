//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This test exercises hpx::threads::thread_data::is_background() /
// set_is_background() as well as the interaction of the "is background thread"
// flag with hpx::this_thread::suspend() and
// hpx::execution_base::this_thread::yield_k(), which now use
// hpx::threads::keep_alive_thread_id to conditionally keep the running thread
// alive depending on whether it is flagged as a background thread.

#define HPX_HAVE_FORCE_NO_CXX_MODULES

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>

#include <atomic>
#include <cstddef>
#include <mutex>
#include <set>

namespace {

    ///////////////////////////////////////////////////////////////////////////
    void test_is_background_default_false()
    {
        hpx::async([]() {
            auto const* data = hpx::threads::get_self_id_data();
            HPX_TEST(data != nullptr);
            HPX_TEST(!data->is_background());
        }).get();
    }

    void test_set_is_background()
    {
        hpx::async([]() {
            auto* data = hpx::threads::get_self_id_data();
            HPX_TEST(!data->is_background());

            data->set_is_background();
            HPX_TEST(data->is_background());

            // setting it again is idempotent
            data->set_is_background();
            HPX_TEST(data->is_background());
        }).get();
    }

    // A newly (re-)initialized thread must not carry over the is_background
    // flag from a previous use of the same thread_data instance (see
    // thread_data::rebind_base resetting is_background_ to false). Since
    // thread_data address reuse depends on allocator behavior and is not
    // guaranteed to happen on the very first allocation/deallocation cycle,
    // keep creating threads (alternating whether is_background is set) until a
    // reused address is actually observed, capped at max_iterations to avoid
    // looping forever if reuse never happens.
    void test_is_background_reset_on_rebind()
    {
        constexpr int max_iterations = 10000;

        hpx::mutex mtx;
        std::set<hpx::threads::thread_data const*> previous_data;

        std::atomic<bool> observed_reuse{false};

        for (int i = 0; i != max_iterations && !observed_reuse.load(); ++i)
        {
            bool const mark_background = (i % 2) == 0;

            hpx::async([&previous_data, &observed_reuse, &mtx,
                           mark_background]() {
                auto* data = hpx::threads::get_self_id_data();
                HPX_TEST(data != nullptr);

                {
                    std::scoped_lock lk(mtx);

                    if (previous_data.contains(data))
                    {
                        // this thread_data instance's storage has been reused;
                        // its is_background flag must have been reset to its
                        // default value (false) by thread_data::rebind_base
                        HPX_TEST(!data->is_background());
                        observed_reuse = true;
                    }

                    if (mark_background)
                    {
                        data->set_is_background();
                        HPX_TEST(data->is_background());
                        previous_data.insert(data);
                    }
                }
            }).get();
        }

        HPX_TEST_MSG(observed_reuse.load(),
            "expected thread_data address reuse to be observed within "
            "max_iterations");
    }

    ///////////////////////////////////////////////////////////////////////////
    // Regression test for
    // hpx::this_thread::suspend()/execution_agent::do_yield() correctly
    // handling threads that are flagged as background threads. Such threads
    // must not have an extra reference added/removed while suspended (the
    // scheduler is assumed to keep them alive instead), while regular,
    // non-background threads still get a temporary extra reference while
    // suspended.
    void test_suspend_resume_with_background_flag(bool const mark_background)
    {
        hpx::mutex mtx;
        hpx::condition_variable cond;
        bool running = false;
        bool woken_up = false;

        hpx::thread t([&mtx, &cond, &running, &woken_up, mark_background]() {
            auto* data = hpx::threads::get_self_id_data();
            if (mark_background)
            {
                data->set_is_background();
            }
            HPX_TEST_EQ(data->is_background(), mark_background);

            {
                std::lock_guard<hpx::mutex> lk(mtx);
                running = true;
                cond.notify_all();
            }

            // suspend and wait to be resumed from the outside
            hpx::this_thread::suspend(
                hpx::threads::thread_schedule_state::suspended);

            // the thread_data instance (and its flag) must have survived the
            // suspension unchanged
            HPX_TEST_EQ(hpx::threads::get_self_id_data()->is_background(),
                mark_background);

            woken_up = true;
        });

        // wait for the new thread to reach the suspend point
        {
            std::unique_lock<hpx::mutex> lk(mtx);
            // NOLINTNEXTLINE(bugprone-infinite-loop)
            while (!running)
                cond.wait(lk);
        }

        hpx::threads::thread_id_type const id = t.native_handle();
        hpx::threads::set_thread_state(
            id, hpx::threads::thread_schedule_state::pending);

        t.join();

        HPX_TEST(woken_up);
    }

    ///////////////////////////////////////////////////////////////////////////
    // execution_agent::yield_k is invoked (through
    // hpx::execution_base::this_thread::yield_k) on the current HPX thread
    // regardless of the is_background flag; verify its threshold behavior still
    // holds (guarding against regressions from the const-qualification
    // changes).
    void test_yield_k_threshold_behavior()
    {
        hpx::async([]() {
            // for small values of k, yield_k must not actually yield the thread
            for (std::size_t k = 0; k != 4; ++k)
            {
                HPX_TEST(!hpx::execution_base::this_thread::yield_k(k));
            }

            // for sufficiently large (even) k, the thread is actually suspended
            // and rescheduled, function returns true
            HPX_TEST(hpx::execution_base::this_thread::yield_k(32));

            // for odd k the "pending_boost" yield path is used, also returns
            // true
            HPX_TEST(hpx::execution_base::this_thread::yield_k(33));
        }).get();
    }

    void test_yield_k_with_background_flag()
    {
        hpx::async([]() {
            auto* data = hpx::threads::get_self_id_data();
            data->set_is_background();

            // must not crash/assert when yielding while flagged as background
            HPX_TEST(hpx::execution_base::this_thread::yield_k(32));
            HPX_TEST(data->is_background());
        }).get();
    }
}    // namespace

///////////////////////////////////////////////////////////////////////////////
int hpx_main()
{
    test_is_background_default_false();
    test_set_is_background();
    test_is_background_reset_on_rebind();

    test_suspend_resume_with_background_flag(false);
    test_suspend_resume_with_background_flag(true);

    test_yield_k_threshold_behavior();
    test_yield_k_with_background_flag();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    hpx::local::init(hpx_main, argc, argv);
    return hpx::util::report_errors();
}
