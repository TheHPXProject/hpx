//  Copyright (c) 2021 Gregor Daiss
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/executors/service_executors.hpp>
#include <hpx/modules/execution.hpp>

int main(int argc, char*[])
{
    // Compile-only regression test: the point is that this translation unit
    // builds with nvcc. The executor asserts on a null io_service_pool in
    // Debug builds, so keep the code path unexecuted (argc is always >= 1).
    if (argc < 0)
    {
        hpx::parallel::execution::detail::service_executor exec{nullptr};
        hpx::parallel::execution::async_execute(exec, [] {}).get();
    }
    return 0;
}
