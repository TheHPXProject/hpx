//  Copyright (c) 2026 Hartmut Kaiser
//  Copyright (c) 2026 Shivansh Singh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/execution.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <utility>

namespace ex = hpx::execution::experimental;

struct test_receiver
{
    using receiver_concept = ex::receiver_t;

    ex::run_loop& loop;
    std::atomic<bool>& executed;

    template <typename E>
    void set_error(E&&) && noexcept
    {
        HPX_TEST(false);
        loop.finish();
    }

    void set_stopped() && noexcept
    {
        HPX_TEST(false);
        loop.finish();
    }

    void set_value() && noexcept
    {
        executed.store(true);
        loop.finish();
    }
};

int hpx_main()
{
    ex::run_loop loop;
    auto sched = loop.get_scheduler();

    std::atomic<bool> executed{false};

    auto sender = ex::schedule(sched);
    auto op_state =
        ex::connect(std::move(sender), test_receiver{loop, executed});
    ex::start(op_state);

    loop.run();

    HPX_TEST(executed.load());

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
