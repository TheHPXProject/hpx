//  Copyright (c) 2026 The STE||AR-Group
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This test is built without HPX_MODULE_STATIC_LINKING so that it includes
// any_sender.hpp the same way a normal consumer of the shared HPX libraries
// does (with dllimport on Windows). It only needs to compile and link.

#include <hpx/modules/execution_base.hpp>

int main()
{
    {
        hpx::execution::experimental::unique_any_sender<> s1;
        hpx::execution::experimental::unique_any_sender<> s2 = HPX_MOVE(s1);
        (void) s2;
    }

    {
        hpx::execution::experimental::any_sender<int> s1;
        hpx::execution::experimental::any_sender<int> s2 = s1;
        hpx::execution::experimental::any_sender<int> s3 = HPX_MOVE(s1);
        (void) s2;
        (void) s3;
    }

    return 0;
}
