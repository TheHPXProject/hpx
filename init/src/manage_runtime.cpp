//  Copyright (c)      2025 Agustin Berge
//  Copyright (c) 2016-2022 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

/// \file manage_runtime.hpp

#include <hpx/config.hpp>

#include <hpx/assert.hpp>
#include <hpx/condition_variable.hpp>
#include <hpx/functional.hpp>
#include <hpx/manage_runtime.hpp>
#include <hpx/modules/init_runtime.hpp>
#include <hpx/modules/runtime_local.hpp>
#include <hpx/mutex.hpp>
#include <hpx/thread.hpp>

#include <hpx/hpx_start.hpp>

#include <mutex>

namespace hpx {

    bool manage_runtime::start(
        int argc, char** argv, init_params const& init_args)
    {
        HPX_ASSERT(!running_);

        std::function<int(int, char**)> start_function =
            bind_front(&manage_runtime::hpx_main, this);

        bool const ret = hpx::start(start_function, argc, argv, init_args);
        if (!ret)
            return ret;

        running_ = true;

        // wait for the main HPX thread (hpx_main below) to have started running
        std::unique_lock<std::mutex> lk(startup_mtx_);
        startup_cond_.wait(lk, [&] { return rts_ != nullptr; });

        return ret;
    }

    int manage_runtime::stop()
    {
        HPX_ASSERT(running_);

        running_ = false;

        // signal to `hpx_main` below to tear down the runtime
        {
            std::lock_guard<spinlock> lk(stop_mtx_);
            rts_ = nullptr;
        }
        stop_cond_.notify_one();

        // wait for the runtime to exit
        return hpx::stop();
    }

    int manage_runtime::hpx_main(int, char*[])
    {
        // signal to `start` that thread has started running.
        {
            std::lock_guard<std::mutex> lk(startup_mtx_);
            rts_ = hpx::get_runtime_ptr();
        }
        startup_cond_.notify_one();

        // wait for `stop` to be called.
        {
            std::unique_lock<spinlock> lk(stop_mtx_);
            stop_cond_.wait(lk, [&] { return rts_ == nullptr; });
        }

        // tell the runtime it's ok to exit
        return hpx::finalize();
    }
}    // namespace hpx
