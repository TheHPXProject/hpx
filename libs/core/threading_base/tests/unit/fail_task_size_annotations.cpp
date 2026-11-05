//  Copyright (c) 2026 Vansh Dobhal
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This test should fail to compile: a type must not have two different
// hpx::task_size annotations.

#include <hpx/modules/functional.hpp>

struct[[= hpx::task_size::small_, = hpx::task_size::large]] conflicting
{
    void operator()() const {}
};

int main()
{
    return static_cast<int>(hpx::traits::get_task_size_v<conflicting>);
}
