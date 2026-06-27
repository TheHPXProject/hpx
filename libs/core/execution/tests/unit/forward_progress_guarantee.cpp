//  Copyright (c) 2022 Shreyas Atre
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/modules/execution.hpp>
#include <hpx/modules/execution_base.hpp>
#include <hpx/modules/testing.hpp>

#include <exception>
#include <utility>

namespace mylib {

    // Using member query function (new stdexec API)
    struct inline_scheduler_0
    {
        constexpr auto query(
            hpx::execution::experimental::get_forward_progress_guarantee_t)
            const noexcept
        {
            return hpx::execution::experimental::forward_progress_guarantee::
                concurrent;
        }

    } scheduler{};

    // Using member query function (new stdexec API)
    struct inline_scheduler_1
    {
        constexpr auto query(
            hpx::execution::experimental::get_forward_progress_guarantee_t)
            const noexcept
        {
            return hpx::execution::experimental::forward_progress_guarantee::
                concurrent;
        }
    } scheduler_custom{};

}    // namespace mylib

int main()
{
    /*TODO: ADD PROPER TESTS WITH REAL SCHEDULERS*/
    using namespace mylib;
    static_assert(hpx::execution::experimental::get_forward_progress_guarantee(
                      scheduler) ==
            hpx::execution::experimental::forward_progress_guarantee::
                concurrent,
        "forward_progress_guarantee should return concurrent");

    static_assert(hpx::execution::experimental::get_forward_progress_guarantee(
                      scheduler_custom) ==
            hpx::execution::experimental::forward_progress_guarantee::
                concurrent,
        "CPO should invoke user tag_invoke");

    return hpx::util::report_errors();
}
