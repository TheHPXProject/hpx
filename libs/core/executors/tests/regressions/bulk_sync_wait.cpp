//  Copyright (c) 2022 Steven R. Brandt
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// sync_wait() did not compile when used with an lvalue sender involving bulk

#include <hpx/execution.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>

namespace ex = hpx::execution::experimental;
namespace tt = hpx::this_thread::experimental;

int hpx_main()
{
    std::atomic<bool> called = false;

    ex::thread_pool_scheduler sch{};

    auto s = ex::starts_on(
        sch, ex::just() | ex::bulk(1, [&called](auto) { called = true; }));

    tt::sync_wait(s);

    HPX_TEST(called.load());

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ_MSG(hpx::local::init(hpx_main, argc, argv), 0,
        "HPX main exited with non-zero status");

    return hpx::util::report_errors();
}
