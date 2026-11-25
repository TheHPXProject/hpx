//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// A registered polling function is called by the workers of the scheduler
// with its context, its work count is part of the scheduler's polling work,
// and it isn't called anymore once removed (but for a round that had begun
// before). Registering the same function again does nothing. No more than
// max_polling_functions can be registered.

#include <hpx/init.hpp>
#include <hpx/modules/errors.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>
#include <hpx/thread.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>

namespace policies = hpx::threads::policies;
using policies::detail::polling_status;

namespace {

    constexpr std::size_t max_functions =
        policies::scheduler_base::max_polling_functions;

    struct poll_data
    {
        std::atomic<std::size_t> calls{0};
        std::atomic<std::size_t> work{0};
        std::atomic<bool> wrong_context{false};
    };

    // a worker may still call a function once after it was removed, so the
    // contexts and functions outlive the tests
    poll_data registered_data;
    std::array<poll_data, max_functions + 1> capacity_data;

    polling_status poll(void* context)
    {
        auto* data = static_cast<poll_data*>(context);
        ++data->calls;
        return data->work.load() != 0 ? polling_status::busy :
                                        polling_status::idle;
    }

    polling_status poll_registered(void* context)
    {
        if (context != &registered_data)
            registered_data.wrong_context = true;
        return poll(context);
    }

    std::size_t work_count(void* context)
    {
        return static_cast<poll_data*>(context)->work.load();
    }

    policies::detail::polling_function const registered{
        &poll_registered, &work_count, &registered_data};

    // functions without a work count
    std::array<policies::detail::polling_function, max_functions + 1>
        capacity_functions;

    // run rounds of this worker's scheduling loop until pred holds
    template <typename Pred>
    bool yield_until(Pred pred)
    {
        auto const deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(10);
        while (!pred())
        {
            if (std::chrono::steady_clock::now() > deadline)
                return false;
            hpx::this_thread::yield();
        }
        return true;
    }

    void yield_rounds(int rounds)
    {
        for (int i = 0; i != rounds; ++i)
        {
            hpx::this_thread::yield();
        }
    }

    policies::scheduler_base& scheduler()
    {
        return *hpx::threads::get_self_id_data()->get_scheduler_base();
    }

    void test_registered()
    {
        auto& sched = scheduler();
        HPX_TEST(sched.custom_polling_function() == polling_status::idle);
        HPX_TEST_EQ(sched.get_polling_work_count(), std::size_t(0));

        sched.add_polling_function(&registered);

        // the workers call it with its context
        HPX_TEST(yield_until([] { return registered_data.calls.load() > 10; }));
        HPX_TEST(!registered_data.wrong_context.load());

        // its work count and status
        HPX_TEST_EQ(sched.get_polling_work_count(), std::size_t(0));
        registered_data.work = 3;
        HPX_TEST_EQ(sched.get_polling_work_count(), std::size_t(3));
        HPX_TEST(sched.custom_polling_function() == polling_status::busy);

        // registering it again does nothing: its work counts once
        sched.add_polling_function(&registered);
        HPX_TEST_EQ(sched.get_polling_work_count(), std::size_t(3));
        registered_data.work = 0;

        // once removed, only rounds that had begun may still call it
        sched.remove_polling_function(&registered);
        yield_rounds(10);
        std::size_t const calls = registered_data.calls.load();
        yield_rounds(100);
        HPX_TEST_EQ(registered_data.calls.load(), calls);
        HPX_TEST(sched.custom_polling_function() == polling_status::idle);
        HPX_TEST_EQ(sched.get_polling_work_count(), std::size_t(0));

        // removing it again does nothing
        sched.remove_polling_function(&registered);
    }

    // no more than max_polling_functions at the same time
    void test_capacity()
    {
        auto& sched = scheduler();
        for (std::size_t i = 0; i != max_functions + 1; ++i)
        {
            capacity_functions[i] = {&poll, nullptr, &capacity_data[i]};
        }

        for (std::size_t i = 0; i != max_functions; ++i)
        {
            sched.add_polling_function(&capacity_functions[i]);
        }

        bool caught = false;
        try
        {
            sched.add_polling_function(&capacity_functions[max_functions]);
        }
        catch (hpx::exception const& e)
        {
            caught = true;
            HPX_TEST_EQ(e.get_error(), hpx::error::bad_parameter);
        }
        HPX_TEST(caught);

        // a registered function can be registered again when all slots are
        // taken
        sched.add_polling_function(&capacity_functions[0]);

        // functions without a work count add none
        HPX_TEST_EQ(sched.get_polling_work_count(), std::size_t(0));

        // a removed slot can be used again
        sched.remove_polling_function(&capacity_functions[0]);
        sched.add_polling_function(&capacity_functions[max_functions]);
        HPX_TEST(yield_until(
            [] { return capacity_data[max_functions].calls.load() > 0; }));

        for (std::size_t i = 1; i != max_functions + 1; ++i)
        {
            sched.remove_polling_function(&capacity_functions[i]);
        }
        yield_rounds(10);
        HPX_TEST(sched.custom_polling_function() == polling_status::idle);
    }
}    // namespace

int hpx_main()
{
    test_registered();
    test_capacity();

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
