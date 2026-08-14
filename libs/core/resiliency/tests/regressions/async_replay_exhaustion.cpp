//  Copyright (c) 2026 Arpit Singh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/resiliency.hpp>
#include <hpx/modules/testing.hpp>

#include <atomic>
#include <stdexcept>

std::atomic<int> call_count(0);

int fail_once()
{
    if (call_count++ == 0)
    {
        throw std::runtime_error("First attempt fails");
    }
    return 42;
}

bool validate(int result)
{
    return result == 42;
}

int fail_once_validate()
{
    if (call_count++ == 0)
    {
        return 0;    // Fails predicate.
    }
    return 42;
}

int hpx_main()
{
    {
        // Test async_replay: fails on 1st attempt, succeeds on 2nd (final) attempt.
        // async_replay(1, ...) means 1 retry, so 2 total attempts.
        call_count = 0;
        hpx::future<int> f =
            hpx::resiliency::experimental::async_replay(1, &fail_once);

        try
        {
            HPX_TEST_EQ(f.get(), 42);
        }
        catch (hpx::resiliency::experimental::abort_replay_exception const&)
        {
            HPX_TEST_MSG(false,
                "async_replay threw abort_replay_exception on successful final "
                "attempt");
        }
    }

    {
        // Test async_replay_validate: fails on 1st attempt, succeeds on
        // 2nd (final) attempt.
        call_count = 0;
        hpx::future<int> f =
            hpx::resiliency::experimental::async_replay_validate(
                1, &validate, &fail_once_validate);

        try
        {
            HPX_TEST_EQ(f.get(), 42);
        }
        catch (hpx::resiliency::experimental::abort_replay_exception const&)
        {
            HPX_TEST_MSG(false,
                "async_replay_validate threw abort_replay_exception on "
                "successful final attempt");
        }
    }

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST(hpx::local::init(hpx_main, argc, argv) == 0);
    return hpx::util::report_errors();
}
