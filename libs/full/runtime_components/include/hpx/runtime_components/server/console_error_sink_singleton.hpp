//  Copyright (c) 2007-2026 Hartmut Kaiser
//  Copyright (c)      2011 Bryce Lelbach
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <hpx/config.hpp>
#include <hpx/modules/concurrency.hpp>
#include <hpx/modules/functional.hpp>

#include <mutex>
#include <string>
#include <utility>

///////////////////////////////////////////////////////////////////////////////
namespace hpx::components::server {

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT class console_error_dispatcher
    {
    public:
        console_error_dispatcher(console_error_dispatcher const&) = delete;
        console_error_dispatcher(console_error_dispatcher&&) = delete;
        console_error_dispatcher& operator=(
            console_error_dispatcher const&) = delete;
        console_error_dispatcher& operator=(
            console_error_dispatcher&&) = delete;

    public:
        using mutex_type = util::spinlock;
        using sink_type = hpx::function<void(std::string const&)>;

        console_error_dispatcher() = default;
        ~console_error_dispatcher() = default;

        template <typename F>
        sink_type set_error_sink(F&& sink)
        {
            std::lock_guard<mutex_type> l(mtx_);
            sink_type old_sink = HPX_MOVE(sink_);
            sink_ = HPX_FORWARD(F, sink);
            return old_sink;
        }

        void operator()(std::string const& msg)
        {
            std::lock_guard<mutex_type> l(mtx_);
            if (sink_)
                sink_(msg);
        }

    private:
        mutex_type mtx_;
        sink_type sink_;
    };

    ///////////////////////////////////////////////////////////////////////////
    HPX_CXX_EXPORT HPX_EXPORT console_error_dispatcher& get_error_dispatcher();
}    // namespace hpx::components::server
