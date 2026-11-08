//  Copyright (c) 2026 Shivansh Singh
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/execution.hpp>
#include <hpx/future.hpp>
#include <hpx/init.hpp>
#include <hpx/modules/testing.hpp>

#include <utility>

namespace ex = hpx::execution::experimental;

int hpx_main()
{
    // P2300 sender that completes with non-exception error (int 42)
    auto snd = ex::just_error(42);

    // Bridge this sender to an hpx::future via as_future
    hpx::future<int> f = ex::as_future<int>(std::move(snd));

    bool caught_error = false;
    try
    {
        (void) f.get();
    }
    catch (int e)
    {
        HPX_TEST_EQ(e, 42);
        caught_error = true;
    }
    catch (...)
    {
        HPX_TEST_MSG(false, "Caught unexpected exception type");
    }

    HPX_TEST(caught_error);

    return hpx::local::finalize();
}

int main(int argc, char* argv[])
{
    HPX_TEST_EQ(hpx::local::init(hpx_main, argc, argv), 0);
    return hpx::util::report_errors();
}
