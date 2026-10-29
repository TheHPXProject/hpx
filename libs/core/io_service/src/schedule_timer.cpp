//  Copyright (c) 2026 Pratyksh Gupta
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <hpx/config.hpp>

#include <hpx/io_service/schedule_timer.hpp>

#include <asio/error.hpp>
#include <asio/io_context.hpp>
#include <asio/steady_timer.hpp>

#include <chrono>
#include <functional>
#include <memory>
#include <system_error>
#include <utility>

namespace hpx::io::experimental::detail {

    struct timer_operation::impl
    {
        explicit impl(::asio::io_context& context)
          : timer(context)
        {
        }

        ::asio::steady_timer timer;
    };

    timer_operation::timer_operation(::asio::io_context& context)
      : impl_(std::make_unique<impl>(context))
    {
    }

    timer_operation::~timer_operation() = default;

    void timer_operation::async_wait(
        std::chrono::steady_clock::time_point const time_point,
        callback_type callback)
    {
        impl_->timer.expires_at(time_point);
        impl_->timer.async_wait(
            [callback = HPX_MOVE(callback)](
                std::error_code const& error) { callback(error); });
    }

    void timer_operation::cancel(std::error_code& error) noexcept
    {
        impl_->timer.cancel(error);
    }

    bool timer_operation::is_operation_aborted(
        std::error_code const& error) noexcept
    {
        return error == ::asio::error::operation_aborted;
    }
}    // namespace hpx::io::experimental::detail
