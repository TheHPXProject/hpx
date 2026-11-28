// Copyright (C) 2012 Hartmut Kaiser
// Copyright (C) 2017 Christopher Taylor
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>
#if !defined(HPX_COMPUTE_DEVICE_CODE)
#include <hpx/hpx_main.hpp>
#include <hpx/include/actions.hpp>
#include <hpx/include/runtime.hpp>
#include <hpx/modules/testing.hpp>
#include <hpx/modules/threading_base.hpp>

#include <cstring>
#include <vector>

///////////////////////////////////////////////////////////////////////////////
void test_small_stacksize()
{
    HPX_TEST(hpx::threads::get_self_ptr());
    // verify that sufficient stack has been allocated
    HPX_TEST_EQ(hpx::threads::get_ctx_ptr()->get_stacksize(),
        hpx::get_runtime().get_config().get_stack_size(
            hpx::threads::thread_stacksize::small_));

    // a frame larger than any HPX thread stack, so that using it overflows
    char array[HPX_SMALL_STACK_SIZE * HPX_THREADS_STACK_OVERHEAD];

    // The array never escapes, so an optimizing compiler may drop it together
    // with the memset and the test would return normally. Passing its address
    // to an asm statement that clobbers memory keeps both.
    std::memset(array, '\0', sizeof(array));
    __asm__ __volatile__("" : : "r"(array) : "memory");
}
HPX_DECLARE_ACTION(test_small_stacksize, test_small_stacksize_action)
HPX_ACTION_USES_SMALL_STACK(test_small_stacksize_action)
HPX_PLAIN_ACTION(test_small_stacksize, test_small_stacksize_action)

int main()
{
    std::vector<hpx::id_type> localities = hpx::find_all_localities();

    for (hpx::id_type const& id : localities)
    {
        {
            test_small_stacksize_action test_action;
            test_action(id);
        }
    }

    return hpx::util::report_errors();
}
#endif
