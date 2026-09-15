//  Copyright (c) 2026 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/hpx.hpp>
#include <hpx/hpx_init.hpp>
#include <hpx/modules/actions.hpp>

#include <atomic>
#include <chrono>

namespace {

    std::atomic<bool> consulted(false);
}    // namespace

bool was_consulted()
{
    return consulted.load();
}
HPX_PLAIN_ACTION(was_consulted, was_consulted_action)

void busy_work()
{
    consulted.store(true);
    hpx::this_thread::suspend(std::chrono::milliseconds(200));
}
HPX_PLAIN_ACTION(busy_work, busy_work_action)

int hpx_main()
{
    // The console locality disconnects this late-connected worker or shuts the
    // complete application down.
    return 0;
}

int main(int argc, char* argv[])
{
    hpx::init_params init_args;
    init_args.mode = hpx::runtime_mode::connect;

    return hpx::init(argc, argv, init_args);
}
